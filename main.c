#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_NAME 32

typedef struct {
    char name[MAX_NAME];
    int period;
    int deadline;
    int burst;
} Task;

typedef struct {
    int remaining;
    int next_arrival;
    int abs_deadline;
} RuntimeState;

static int read_tasks(FILE *fp, int *total_time, Task **tasks_out, int *n_tasks_out) {
    if (fscanf(fp, "%d", total_time) != 1 || *total_time <= 0) {
        fprintf(stderr, "erro: tempo total de simulacao invalido\n");
        return -1;
    }

    int capacity = 8, n = 0;
    Task *tasks = malloc(capacity * sizeof(Task));
    if (!tasks) { fprintf(stderr, "erro: falha de alocacao\n"); return -1; }

    Task t;
    int r;
    while ((r = fscanf(fp, "%31s %d %d %d", t.name, &t.period, &t.deadline, &t.burst)) == 4) {
        if (t.period <= 0 || t.deadline <= 0 || t.burst <= 0) {
            fprintf(stderr, "erro: tarefa '%s' com valor nao positivo\n", t.name);
            free(tasks); return -1;
        }
        if (t.deadline > t.period) {
            fprintf(stderr, "erro: tarefa '%s' viola D <= P (deadline %d, periodo %d)\n", t.name, t.deadline, t.period);
            free(tasks); return -1;
        }
        if (t.burst > t.deadline) {
            fprintf(stderr, "erro: tarefa '%s' viola C <= D (burst %d, deadline %d)\n", t.name, t.burst, t.deadline);
            free(tasks); return -1;
        }
        if (n == capacity) {
            capacity *= 2;
            Task *tmp = realloc(tasks, capacity * sizeof(Task));
            if (!tmp) { fprintf(stderr, "erro: falha de alocacao\n"); free(tasks); return -1; }
            tasks = tmp;
        }
        tasks[n++] = t;
    }
    if (!feof(fp) && r != EOF) {
        fprintf(stderr, "erro: linha malformada no arquivo de entrada\n");
        free(tasks); return -1;
    }
    if (n == 0) {
        fprintf(stderr, "erro: nenhuma tarefa valida encontrada\n");
        free(tasks); return -1;
    }
    *tasks_out = tasks; *n_tasks_out = n;
    return 0;
}

static int choose_next(const char *algo, Task *tasks, RuntimeState *rt, int n_tasks) {
    int best = -1;
    for (int i = 0; i < n_tasks; i++) {
        if (rt[i].remaining <= 0) continue;
        if (best == -1) { best = i; continue; }
        int i_wins;
        if (strcmp(algo, "rate") == 0) {
            i_wins = tasks[i].period < tasks[best].period;
        } else {
            i_wins = rt[i].abs_deadline < rt[best].abs_deadline;
        }
        if (i_wins) best = i;
    }
    return best;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "uso: %s <rate|edf> <arquivo_entrada>\n", argv[0]);
        return 1;
    }

    if (strcmp(argv[1], "rate") != 0 && strcmp(argv[1], "edf") != 0) {
        fprintf(stderr, "erro: algoritmo invalido '%s'. use 'rate' ou 'edf'\n", argv[1]);
        return 1;
    }
    const char *algo = argv[1];

    FILE *fp = fopen(argv[2], "r");
    if (fp == NULL) {
        fprintf(stderr, "erro: nao foi possivel abrir o arquivo '%s'\n", argv[2]);
        return 1;
    }

    int total_time, n_tasks;
    Task *tasks;
    if (read_tasks(fp, &total_time, &tasks, &n_tasks) != 0) {
        fclose(fp);
        return 1;
    }
    fclose(fp);

    RuntimeState *rt = malloc(n_tasks * sizeof(RuntimeState));
    if (!rt) { fprintf(stderr, "erro: falha de alocacao\n"); free(tasks); return 1; }
    for (int i = 0; i < n_tasks; i++) {
        rt[i].remaining = 0;
        rt[i].next_arrival = 0;
        rt[i].abs_deadline = 0;
    }

    // DEBUG TEMPORARIO
    int current_task = -1;
    int run_start = 0;

    for (int t = 0; t < total_time; t++) {
        for (int i = 0; i < n_tasks; i++) {
            if (t == rt[i].next_arrival) {
                rt[i].remaining = tasks[i].burst;
                rt[i].abs_deadline = t + tasks[i].deadline;
                rt[i].next_arrival += tasks[i].period;
            }
        }

        int choice = choose_next(algo, tasks, rt, n_tasks);

        if (choice != current_task) {
            int length = t - run_start;
            if (length > 0) {
                if (current_task == -1) {
                    printf("idle for %d units\n", length);
                } else {
                    char reason = (rt[current_task].remaining == 0) ? 'F' : 'H';
                    printf("[%s] for %d units - %c\n", tasks[current_task].name, length, reason);
                }
            }
            current_task = choice;
            run_start = t;
        }

        if (choice != -1) {
            rt[choice].remaining--;
        }
    }

    int length = total_time - run_start;
    if (length > 0) {
        if (current_task == -1) {
            printf("idle for %d units\n", length);
        } else {
            char reason = (rt[current_task].remaining == 0) ? 'F' : 'H';
            printf("[%s] for %d units - %c\n", tasks[current_task].name, length, reason);
        }
    }

    free(rt);
    free(tasks);
    return 0;
}
