# Escalonador de Tarefas Críticas de Voo

## Descrição
O **Scheduler** é um simulador implementado em C que compara dois algoritmos clássicos de escalonamento para tarefas críticas e periódicas: *rate-monotonic* (rate) e *earliest-deadline-first* (edf). Ambas as abordagens são preemptivas, ou seja, a cada instante a tarefa pronta de maior prioridade interrompe a execução das demais. 

Em caso de empate de prioridade, vence a tarefa que aparece primeiro no arquivo de entrada. O sistema garante a regra de validação em que o tempo de execução (burst) deve ser menor ou igual ao prazo (deadline), que por sua vez deve ser menor ou igual ao período ($C \le D \le P$). Caso uma instância de tarefa não termine até seu deadline absoluto, a rajada restante é descartada e a tarefa é contabilizada como perdida (Lost).

## Sistemas Operacionais Homologados
- Linux (Ubuntu/Debian)
- Unix / macOS

## Arquivos e Módulos
- **main.c**: Arquivo único com todo o código-fonte do simulador. Ele é responsável por ler e validar a formatação do arquivo de texto de entrada, executar o loop temporal da simulação (avaliando chegadas, preempções e deadlines perdidos), e gerar o arquivo final de saída. Também realiza o tratamento de erros operacionais, escrevendo mensagens coerentes em `stderr` e encerrando com código diferente de zero caso encontre entradas inválidas.
- **Makefile**: Script de automação utilizado para a compilação do executável e limpeza dos binários e arquivos gerados (arquivos `.out`).

## Como compilar
Para compilar o projeto, garantindo que você tenha o compilador `gcc` e o comando `make` instalados, utilize no terminal:
```bash
# Compilar o executável "scheduler"
make

# Limpar o executável e arquivos de saída (.out)
make clean
