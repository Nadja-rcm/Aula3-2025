#include "rr.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "msg.h"

#define TIME_SLICE_MS 500  // quantum de 500ms (definido no enunciado)

/*
 * Round-Robin Scheduling
 *
 * Cada tarefa tem direito a até 500ms de CPU.
 * Se acabar antes → remove-se.
 * Se não acabar → é devolvida ao fim da ready queue (preempção).
 */

void rr_scheduler(uint32_t current_time_ms, queue_t *rq, pcb_t **cpu_task) {
    static uint32_t slice_used = 0; // tempo já gasto do quantum pela tarefa atual

    if (*cpu_task) {
        (*cpu_task)->ellapsed_time_ms += TICKS_MS;
        slice_used += TICKS_MS;

        if ((*cpu_task)->ellapsed_time_ms >= (*cpu_task)->time_ms) {
            // tarefa terminou
            msg_t msg = {
                .pid = (*cpu_task)->pid,
                .request = PROCESS_REQUEST_DONE,
                .time_ms = current_time_ms
            };
            if (write((*cpu_task)->sockfd, &msg, sizeof(msg_t)) != sizeof(msg_t)) {
                perror("write");
            }
            free(*cpu_task);
            *cpu_task = NULL;
            slice_used = 0; // reset do quantum
        } else if (slice_used >= TIME_SLICE_MS) {
            // quantum expirou → devolve a tarefa à fila
            enqueue_pcb(rq, *cpu_task);
            *cpu_task = NULL;
            slice_used = 0; // reset
        }
    }

    if (*cpu_task == NULL) {
        // buscar próxima tarefa da fila
        *cpu_task = dequeue_pcb(rq);
        slice_used = 0;
    }
}
