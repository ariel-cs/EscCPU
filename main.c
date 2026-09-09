#include <stdio.h>
#include <string.h>

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

    fclose(fp);
    return 0;
}
