#include "sjf.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "msg.h"

/*
 * SJF não-preemptivo que NÃO acede aos campos internos da queue.
 * Estratégia:
 *  - descarrega todas as PCBs da ready queue usando dequeue_pcb(rq)
 *  - encontra a PCB com menor time_ms
 *  - atribui essa PCB ao CPU (*cpu_task)
 *  - re-enfileira as restantes (preservando ordem relativa)
 */

void sjf_scheduler(uint32_t current_time_ms, queue_t *rq, pcb_t **cpu_task) {
    if (*cpu_task) {
        (*cpu_task)->ellapsed_time_ms += TICKS_MS;
        if ((*cpu_task)->ellapsed_time_ms >= (*cpu_task)->time_ms) {
            msg_t msg = {
                .pid = (*cpu_task)->pid,
                .request = PROCESS_REQUEST_DONE,
                .time_ms = current_time_ms
            };
            if (write((*cpu_task)->sockfd, &msg, sizeof(msg_t)) != sizeof(msg_t)) {
                perror("write");
            }
            free((*cpu_task));
            (*cpu_task) = NULL;
        }
    }

    if (*cpu_task == NULL) {
        pcb_t *p;
        pcb_t **arr = NULL;
        size_t n = 0, cap = 0;

        /* Descarrega tudo para um array dinâmico */
        while ((p = dequeue_pcb(rq)) != NULL) {
            if (n == cap) {
                size_t newcap = cap ? cap * 2 : 8;
                pcb_t **tmp = realloc(arr, newcap * sizeof(pcb_t *));
                if (!tmp) {
                    perror("realloc");
                    /* tentar re-enfileirar o que já temos de volta e sair */
                    for (size_t i = 0; i < n; i++) enqueue_pcb(rq, arr[i]);
                    free(arr);
                    return;
                }
                arr = tmp;
                cap = newcap;
            }
            arr[n++] = p;
        }

        if (n > 0) {
            /* encontra índice do mais curto (time_ms) */
            size_t min_idx = 0;
            for (size_t i = 1; i < n; ++i) {
                if (arr[i]->time_ms < arr[min_idx]->time_ms) {
                    min_idx = i;
                }
            }

            /* atribui a mais curta ao CPU */
            *cpu_task = arr[min_idx];

            /* re-enfileira as restantes na ready queue, preservando ordem relativa */
            for (size_t i = 0; i < n; ++i) {
                if (i == min_idx) continue;
                enqueue_pcb(rq, arr[i]);
            }
        }

        free(arr);
    }
}
