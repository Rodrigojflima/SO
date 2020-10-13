/*
// Projeto SO - exercicio 2, version 3
// Sistemas Operativos, DEI/IST/ULisboa 2016-17
// Rodrigo Lima (83559) e Rui Miguel (79100)
*/

#include "commandlinereader.h"
#include "contas.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>
#include <pthread.h>
#include <semaphore.h>
#define COMANDO_DEBITAR "debitar"
#define COMANDO_CREDITAR "creditar"
#define COMANDO_LER_SALDO "lerSaldo"
#define COMANDO_SIMULAR "simular"
#define COMANDO_SAIR "sair"
#define COMANDO_AGORA "agora"
#define MAXARGS 3
#define BUFFER_SIZE 100

#define NUM_TRABALHADORAS 3
#define CMD_BUFFER_DIM (NUM_TRABALHADORAS * 2)
#define OP_LERSALDO 1
#define OP_CREDITAR 2
#define OP_DEBITAR 3
#define OP_SAIR 4
#define COMANDO_INFO "info"


typedef struct{
  int operacao;
  int idConta;
  int valor;
} comando_t;

comando_t cmd_buffer[CMD_BUFFER_DIM];

sem_t buffer_cheio;
sem_t buffer_vazio;
pthread_t tid[NUM_TRABALHADORAS];
pthread_mutex_t mut;

void gestor_buffer(int operacao, int idConta, int valor);
void *gestor_tarefas();

int buff_write_idx = 0, buff_read_idx = 0;
int totalcmds = 0;
int cmdsread = 0;

int main (int argc, char** argv) {
  char *args[MAXARGS + 1];
  char buffer[BUFFER_SIZE];
  int npids = 0, i; /* guarda o numero total de processos filhos existentes */
  
  pthread_mutex_init(&mut, NULL); /* Inicializacao do mutex */
  sem_init(&buffer_cheio, 0, CMD_BUFFER_DIM); /* Inicial. do sem -> verifica buffer cheio */ 
  sem_init(&buffer_vazio, 0, 0); /* Inicial. do sem -> verifica buffer vazio */
  
  inicializarContas();
  signal(SIGUSR1, abortar);
  
  printf("Bem-vinda/o ao i-banco\n\n");
  
  /* Lançamento das tarefas */
  for(i = 0; i < NUM_TRABALHADORAS; i++){
    if(pthread_create(&tid[i], 0, gestor_tarefas, NULL)){
      printf("Erro na criação da thread\n\n");
      exit(EXIT_FAILURE);
    }
  }
  
  while (1) {
    int numargs,j,k;
    numargs = readLineArguments(args, MAXARGS+1, buffer, BUFFER_SIZE);
    
    /* EOF (end of file) do stdin ou comando "sair" ou comando "sair agora" */
    if (numargs < 0 || (numargs > 0 && (strcmp(args[0], COMANDO_SAIR) == 0))){
      int status, wpid, i, vexit;
      printf("i-banco vai terminar.\n--\n");
      
      if(numargs == 2 && (strcmp(args[1],COMANDO_AGORA)==0)){
	kill(0, SIGUSR1); /* tendo 0 como argumento permite enviar signal a todos os processos filhos*/
	printf("Simulacao terminada por signal\n");
      }
      for(i = 0; i < npids; i++){
	wpid = wait(&status);
	vexit = WEXITSTATUS(status);
	if(WIFEXITED(status)){
	  printf("FILHO TERMINADO (PID=%d;valor=%d terminou normalmente)\n", wpid, vexit);
	}
	else{
	  printf("FILHO TERMINADO (PID=%d;valor= terminou abruptamente)\n", wpid);
	}
      }
      
      for(j = 0; j < NUM_TRABALHADORAS; j++){
	gestor_buffer(OP_SAIR, 0, 0);
      }
      
      for(k = 0; k < NUM_TRABALHADORAS; k++){
	pthread_join(tid[k], NULL);
      }
      
      printf("--\ni-banco terminou.\n");
      exit(EXIT_SUCCESS);
      npids = 0;
    }
    else if (numargs == 0)
      /* Nenhum argumento; ignora e volta a pedir */
      continue;
    
    /* Debitar */
    else if (strcmp(args[0], COMANDO_DEBITAR) == 0){
      int idConta, valor;
      if (numargs < 3) {
	printf("%s: Sintaxe inválida, tente de novo.\n", COMANDO_DEBITAR);
	continue;
      }
      idConta = atoi(args[1]);
      valor = atoi(args[2]);
      
      gestor_buffer(OP_DEBITAR, idConta, valor);
    }
    
    /* Creditar */
    else if (strcmp(args[0], COMANDO_CREDITAR) == 0){
      int idConta, valor;
      if (numargs < 3) {
	printf("%s: Sintaxe inválida, tente de novo.\n", COMANDO_CREDITAR);
	continue;
      }
      idConta = atoi(args[1]);
      valor = atoi(args[2]);
      
      gestor_buffer(OP_CREDITAR, idConta, valor);
    }
    
    /* Ler Saldo */
    else if (strcmp(args[0], COMANDO_LER_SALDO) == 0){
      int idConta;
      if (numargs < 2) {
	printf("%s: Sintaxe inválida, tente de novo.\n", COMANDO_LER_SALDO);
	continue;
      }
      idConta = atoi(args[1]);
      
      gestor_buffer(OP_LERSALDO, idConta, 0);
    }
    
    /* Simular */
    else if (strcmp(args[0], COMANDO_SIMULAR) == 0){
      int numAnos, pid;
      numAnos = atoi(args[1]);
      if(numAnos >= 0){
	pid = fork();
	npids++;
	if(pid == 0){
	  simular(numAnos);
	  exit(1);
	}else
	  continue;
      }
      else
	printf("Comando nao implementado\n");
    }
    
    /* Info */
    else if(strcmp(args[0], COMANDO_INFO) == 0){
      printf("Numero de comandos colocados no buffer: %d\n", totalcmds);
      printf("Estao %d comandos no buffer\n",cmdsread);
    }
    
    else
      printf("Comando desconhecido. Tente de novo.\n");
  } 
}

void gestor_buffer(int operacao, int idConta, int valor){
  sem_wait(&buffer_cheio);
  totalcmds++;
  cmdsread++;
  cmd_buffer[buff_write_idx].operacao = operacao;
  cmd_buffer[buff_write_idx].idConta = idConta;
  cmd_buffer[buff_write_idx].valor = valor;
  if(buff_write_idx++ >= CMD_BUFFER_DIM - 1){
    buff_write_idx = 0;
  }
  sem_post(&buffer_vazio);
}

void *gestor_tarefas(){
  int operacao, idConta, valor, saldo;
  
  do{
    sem_wait(&buffer_vazio);
    pthread_mutex_lock(&mut); /* fecha() */
    operacao = cmd_buffer[buff_read_idx].operacao;
    idConta = cmd_buffer[buff_read_idx].idConta;
    valor = cmd_buffer[buff_read_idx].valor;
    if(buff_read_idx++ >= CMD_BUFFER_DIM - 1){
      buff_read_idx = 0;
    }
    cmdsread--;
    pthread_mutex_unlock(&mut);
    
    if(operacao == OP_DEBITAR){
      if (debitar (idConta, valor) < 0){
	printf("%s(%d, %d): Erro\n\n", COMANDO_DEBITAR, idConta, valor);
      }
      else
	printf("%s(%d, %d): OK\n\n", COMANDO_DEBITAR, idConta, valor);
    }
    else if(operacao == OP_CREDITAR){
      if (creditar (idConta, valor) < 0){
	printf("%s(%d, %d): Erro\n\n", COMANDO_CREDITAR, idConta, valor);
      }
      else
	printf("%s(%d, %d): OK\n\n", COMANDO_CREDITAR, idConta, valor);
    }
    else if(operacao == OP_LERSALDO){
      saldo = lerSaldo(idConta);
      if (saldo < 0){
	printf("%s(%d): Erro.\n\n", COMANDO_LER_SALDO, idConta);
      }
      else
	printf("%s(%d): O saldo da conta é %d.\n\n", COMANDO_LER_SALDO, idConta, saldo);
    }
    else if(operacao == OP_SAIR){
      pthread_exit(NULL);
    }
    sem_post(&buffer_cheio);
  } while(1);
}

