#include <stdio.h>
#include <unistd.h>

#define QUANTUM 4

void process1() { printf("[PID 1] Hello\n"); }
void process2() { printf("[PID 2] Buna siua\n"); }
void process3() { printf("[PID 3] Nihau\n"); }

int nr_procese = 3;
void (*processes[])() = {process1, process2, process3};
int remaining[] = {5, 3, 8};
int current = 0;
int slice = 0;
int preemptive_mode = 0;

void yield() {
    if (preemptive_mode) return;
    printf("[Yield] Process %d yielding\n", current + 1);
    current = (current + 1) % nr_procese;
    slice = 0;
}

void scheduler_tick() {
    if (nr_procese == 0) return;

    processes[current]();
    remaining[current]--;
    slice++;

    if (remaining[current] <= 0) {
        printf("[Done] Process %d finished\n", current + 1);

        for (int i = current; i < nr_procese - 1; i++) {
            processes[i] = processes[i + 1];
            remaining[i] = remaining[i + 1];
        }
        nr_procese--;
        if (nr_procese == 0) {
            printf("All processes are finished!\n");
            return;
        }
        current = (current + 1) % nr_procese;
        slice = 0;
        return;
    }

    if (preemptive_mode && slice >= QUANTUM) {
        printf("[Preemption] Time finished PID %d - next process\n", current + 1);
        current = (current + 1) % nr_procese;
        slice = 0;
    }
}

int main() {

    printf("\nCooperative Scheduling\n");
    for (int t = 0; t < 10; t++) {
        scheduler_tick();
        if (t == 3) yield();
        usleep(200000);
    }

    preemptive_mode = 1;
    printf("\nPreemptive Scheduling\n");

    while (nr_procese > 0) {
        scheduler_tick();
        usleep(200000);
    }

    printf("\nDone!\n");
    return 0;
}