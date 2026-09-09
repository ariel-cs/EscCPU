#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_NAME 32
#define LOGIN "aco4"

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
        fprintf(stderr, "Tempo total invalido [ERRO]\n");
        return -1;
    }

    int capacity = 8, n = 0;
    Task *tasks = malloc(capacity * sizeof(Task));
    if (!tasks) { fprintf(stderr, "Falha de alocacao [ERRO]\n"); return -1; }

    Task t;
    int r;
    while ((r = fscanf(fp, "%31s %d %d %d", t.name, &t.period, &t.deadline, &t.burst)) == 4) {
        if (t.period <= 0 || t.deadline <= 0 || t.burst <= 0) {
            fprintf(stderr, "Tarefa '%s' com valor nao positivo [ERRO]\n", t.name);
            free(tasks); return -1;
        }
        if (t.deadline > t.period) {
            fprintf(stderr, "Tarefa '%s' viola D <= P (deadline %d, periodo %d) [ERRO]\n", t.name, t.deadline, t.period);
            free(tasks); return -1;
        }
        if (t.burst > t.deadline) {
            fprintf(stderr, "Tarefa '%s' viola C <= D (burst %d, deadline %d) [ERRO]\n", t.name, t.burst, t.deadline);
            free(tasks); return -1;
        }
        if (n == capacity) {
            capacity *= 2;
            Task *tmp = realloc(tasks, capacity * sizeof(Task));
            if (!tmp) { fprintf(stderr, "Falha de alocacao [ERRO]\n"); free(tasks); return -1; }
            tasks = tmp;
        }
        tasks[n++] = t;
    }
    if (!feof(fp) && r != EOF) {
        fprintf(stderr, "Linha malformada no arquivo de entrada [ERRO]\n");
        free(tasks); return -1;
    }
    if (n == 0) {
        fprintf(stderr, "Nenhuma tarefa valida encontrada [ERRO]\n");
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
        fprintf(stderr, "Algoritmo invalido '%s'. use 'rate' ou 'edf' [ERRO]\n", argv[1]);
        return 1;
    }
    const char *algo = argv[1];

    FILE *fp = fopen(argv[2], "r");
    if (fp == NULL) {
        fprintf(stderr, "Nao foi possivel abrir o arquivo '%s' [ERRO]\n", argv[2]);
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
    int *lost_count = calloc(n_tasks, sizeof(int));
    int *complete_count = calloc(n_tasks, sizeof(int));
    int *killed_count = calloc(n_tasks, sizeof(int));
    if (!rt || !lost_count || !complete_count || !killed_count) {
        fprintf(stderr, "Falha de alocacao [ERRO]\n");
        free(tasks); free(rt); free(lost_count); free(complete_count); free(killed_count);
        return 1;
    }
    for (int i = 0; i < n_tasks; i++) {
        rt[i].remaining = 0;
        rt[i].next_arrival = 0;
        rt[i].abs_deadline = 0;
    }

    char *out_buf = NULL;
    size_t out_size = 0;
    FILE *mem = open_memstream(&out_buf, &out_size);
    if (!mem) {
        fprintf(stderr, "Falha ao alocar buffer de saida [ERRO]\n");
        free(tasks); free(rt); free(lost_count); free(complete_count); free(killed_count);
        return 1;
    }

    fprintf(mem, "EXECUTION BY %s\n", strcmp(algo, "rate") == 0 ? "RATE" : "EDF");

    int current_task = -1;
    int run_start = 0;

    for (int t = 0; t < total_time; t++) {
        int lost_now = 0;

        for (int i = 0; i < n_tasks; i++) {
            if (rt[i].remaining > 0 && t == rt[i].abs_deadline) {
                rt[i].remaining = 0;
                lost_count[i]++;
                if (i == current_task) lost_now = 1;
            }
        }

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
                    fprintf(mem, "idle for %d units\n", length);
                } else {
                    char reason;
                    if (lost_now) {
                        reason = 'L';
                    } else if (rt[current_task].remaining == 0) {
                        reason = 'F';
                        complete_count[current_task]++;
                    } else {
                        reason = 'H';
                    }
                    fprintf(mem, "[%s] for %d units - %c\n", tasks[current_task].name, length, reason);
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
            fprintf(mem, "idle for %d units\n", length);
        } else {
            char reason;
            if (rt[current_task].remaining == 0) {
                reason = 'F';
                complete_count[current_task]++;
            } else {
                reason = 'K';
            }
            fprintf(mem, "[%s] for %d units - %c\n", tasks[current_task].name, length, reason);
        }
    }

    for (int i = 0; i < n_tasks; i++) {
        if (rt[i].remaining > 0) {
            killed_count[i] = 1;
        }
    }

    fprintf(mem, "\nLOST DEADLINES\n");
    for (int i = 0; i < n_tasks; i++) {
        fprintf(mem, "[%s] %d\n", tasks[i].name, lost_count[i]);
    }

    fprintf(mem, "\nCOMPLETE EXECUTION\n");
    for (int i = 0; i < n_tasks; i++) {
        fprintf(mem, "[%s] %d\n", tasks[i].name, complete_count[i]);
    }

    fprintf(mem, "\nKILLED\n");
    for (int i = 0; i < n_tasks; i++) {
        fprintf(mem, "[%s] %d\n", tasks[i].name, killed_count[i]);
    }

    fclose(mem);

    char outname[64];
    snprintf(outname, sizeof(outname), "%s_%s.out", algo, LOGIN);
    FILE *out = fopen(outname, "w");
    if (!out) {
        fprintf(stderr, "Nao foi possivel criar o arquivo de saida '%s' [ERRO]\n", outname);
        free(out_buf); free(tasks); free(rt);
        free(lost_count); free(complete_count); free(killed_count);
        return 1;
    }
    fwrite(out_buf, 1, out_size, out);
    fclose(out);

    free(out_buf);
    free(rt);
    free(tasks);
    free(lost_count);
    free(complete_count);
    free(killed_count);
    return 0;
}
