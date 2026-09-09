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

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "uso: %s <rate|edf> <arquivo_entrada>\n", argv[0]);
        return 1;
    }

    if (strcmp(argv[1], "rate") != 0 && strcmp(argv[1], "edf") != 0) {
        fprintf(stderr, "erro: algoritmo invalido '%s'. use 'rate' ou 'edf'\n", argv[1]);
        return 1;
    }

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
    free(tasks);
    return 0;
}
