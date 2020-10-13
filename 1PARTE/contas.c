#include "contas.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

#define atrasar() sleep(ATRASO)
#define taxaJuro 0.1
#define custoManutencao 1
#define MAX(x, y) (((x) > (y)) ? (x) : (y))

#include <pthread.h>

int contasSaldos[NUM_CONTAS];
int sair_agora;

pthread_mutex_t mutlist[NUM_CONTAS];

/* Verifica se conta existe */
int contaExiste(int idConta) {
  return (idConta > 0 && idConta <= NUM_CONTAS);
}

/* Poe os saldos de todas as contas a zero */
void inicializarContas() {
  int i;
  for (i=0; i<NUM_CONTAS; i++)
    contasSaldos[i] = 0;
}

/* Debita um determinado valor da conta identificada */
int debitar(int idConta, int valor) {
  atrasar();
  if (!contaExiste(idConta))
    return -1;
  if (contasSaldos[idConta - 1] < valor)
    return -1;
  atrasar();
  pthread_mutex_lock(&mutlist[idConta - 1]); /* fecha() */
  contasSaldos[idConta - 1] -= valor;
  pthread_mutex_unlock(&mutlist[idConta - 1]); /* abre() */
  return 0;
}

/* Credita um determinado valor na conta identificada */
int creditar(int idConta, int valor) {
  atrasar();
  if (!contaExiste(idConta))
    return -1;
  pthread_mutex_lock(&mutlist[idConta - 1]); /* fecha() */
  contasSaldos[idConta - 1] += valor;
  pthread_mutex_unlock(&mutlist[idConta - 1]); /* abre() */
  return 0;
}

/* Imprime no stdout o saldo atual da conta identificada */
int lerSaldo(int idConta) {
  int aux;
  atrasar();
  if (!contaExiste(idConta))
    return -1;
  pthread_mutex_lock(&mutlist[idConta - 1]); /* fecha() */
  aux = contasSaldos[idConta - 1];
  pthread_mutex_unlock(&mutlist[idConta - 1]); /* abre() */
  return aux;
}

/* Executa as simulacoes */
void simular(int numAnos) {
    int i, j, novoSaldo;
    sair_agora = 0;
    for(i = 0; i <= numAnos; i++){
		printf("SIMULACAO: Ano %d\n", i);
		printf("=================\n");
		for(j = 0; j < NUM_CONTAS; j++){
			if(i > 0){
				novoSaldo = MAX(lerSaldo(j+1)*(1+taxaJuro)-custoManutencao, 0);
				contasSaldos[j]= novoSaldo;
			}
			printf("Conta %d, Saldo %d\n", j+1, lerSaldo(j+1));
		}
		printf("\n");
		if(sair_agora){
		  exit(5);
		  break;
		}
	}
}

void abortar(int signum){
	if(signum == SIGUSR1){
		sair_agora = 1;
	}
	else{
		perror("erro de signal\n");
	}
}
