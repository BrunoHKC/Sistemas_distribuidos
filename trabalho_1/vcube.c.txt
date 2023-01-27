/* --------------------------------------------------------------------------------------
   Implementacao o detector de falhas vCube no ambiente de simulação SMPL
   Objetivo: Mostrar a execucao do detector de falhas vCube evidenciando a quantidade de testes e latencia
   Restricoes: -

   Autor: Bruno Henrique Kamarowski de Carvalho
   Disciplina: Sistemas Distribuidos
   Data da ultima atualizacao: 20/01/2023
----------------------------------------------------------------------------------------*/


#include <stdio.h>
#include <stdlib.h>
#include "smpl.h"
#include "cis.h"

/*---- definicoes auxiliares ----*/
#define FALSE   0
#define TRUE  1
#define INTERVALO_TESTE 30.0
#define TEMPO_DE_SIMULACAO 210.0

/*---- aqui definimos os estados dos processos ----*/
#define UNKNOWN   -1
#define CORRETO  0
#define FALHO 1

/*---- aqui definimos os eventos ----*/
#define test   1
#define fault  2
#define recovery 3

/*---- declaramos agora o TipoProcesso ----*/
typedef struct {
   int id;       /* identificador de facility SMPL */
   /* variáveis locais do processo são declaradas aqui */
} TipoProcesso;

/*---- declaramos tipoEvento----*/
typedef struct{
    int tipoEvento; //2-fault;3-recovery
    float tempoEvento;
    int processoEvento;
    int latencia;
    int qtdTestes;
} TipoEvento;

/*---- declaramos algumas variaveis para gerencia----*/

TipoEvento *vetorEventos;
int inicioVetor;
int qtdEventos;

TipoProcesso *processo; 


void imprimeState(int** state,int i,int N)
{
    printf("State[%d] : {",i);
    for(int j = 0; j < N;j++)
    {
        printf(" %d ",state[i][j]);
    }
    printf("}\n");
}

int primeiroCorreto(node_set* testadores,int i,int* state,int N)
{
    int primeiro = TRUE;
    for(int k = 0;k < testadores->size;k++)
    {
        int j = testadores->nodes[k];
        //se o processo j existe
        if(j < N)
        {
            //se eh o primeiro processo nao falho,retorna verdadeiro
            if(j == i)
            {
                primeiro = TRUE;
                break;
            }

            // se o processo j esta correto ou com estado desconhecido, retorna falso
            if(state[j]%2 == 0 || state[j] == UNKNOWN)
            {
                primeiro = FALSE;
                break;
            }
        }
    }
    
    return primeiro;
}

void obtemInfo(int** state,int i,int j,int N)
{
    int* state_i = state[i];
    int* state_j = state[j];

    for(int k = 0; k < N; k++)
    {
        // pega o valor mais atualizado
        if(k == i) continue;
        state_i[k] = state_i[k] > state_j[k]? state_i[k]:state_j[k];
    }
}

void incrementaTestes(int N)
{
    for(int i = 0;i < qtdEventos;i++)
    {
        vetorEventos[(i+inicioVetor)%N].qtdTestes++;
    }
}

void incrementaLatencias(int N)
{
    for(int i = 0;i < qtdEventos;i++)
    {
        vetorEventos[(i+inicioVetor)%qtdEventos].latencia++;
    }
}

void verificaRodadaTeste(int * vetorTestadores,int N)
{
    int todosTestaram = TRUE;
    for(int i = 0; (i < N) && todosTestaram; i++)
    {
        //Verifica se o processo testou alguem ou esta falho
        todosTestaram &= vetorTestadores[i] || (status(processo[i].id) != 0);
    }

    //Se todos testaram, incrementa latencia
    if(todosTestaram)
    {
        printf("Terminou uma rodada de testes!!\n");
        for(int i = 0;i < N; i++)
        {
            vetorTestadores[i] = FALSE;
        }
        incrementaLatencias(N);
    }
}

void verificaEventosResolvidos(int **state, int N)
{
    for(int j = 0; j < qtdEventos;j++)
    {
        TipoEvento evento = vetorEventos[(j + inicioVetor)%N];

        int processoEvento = evento.processoEvento;
        int estadoEsperado = evento.tipoEvento==fault?FALHO:CORRETO;
        int todosProcessosAtualizados = TRUE;

        //Verifica se todos os vetores state de processos corretos possuem o estado correto
        for(int i = 0;i < N;i++)
        {
            if(status(processo[i].id) != 0) continue;   //Se processo falho, nao verifica vetor state
            todosProcessosAtualizados &= ((state[i][evento.processoEvento]%2) == estadoEsperado || i == evento.processoEvento);
            if(!todosProcessosAtualizados){break;}
        }

        //Se o estado estava correto em todos os vetores state
        if(todosProcessosAtualizados)
        {
            //remove evento do vetor
            for(int k = j;k >= 0;k--)
            {
                vetorEventos[k+inicioVetor] = vetorEventos[(k-1+inicioVetor)%N];
            }
            inicioVetor = (inicioVetor+1)%N;
            j--;
            qtdEventos--;

            //adiciona a resolucao do evento ao log
            printf("A latencia do evento de %s do processo %d foi de %d rodadas e contou com %d testes\n",evento.tipoEvento==fault?"falha":"recovery",evento.processoEvento,evento.latencia,evento.qtdTestes);
        }
    }
}

int main (int argc, char *argv[]) {
    static int N,      /* number of nodes is parameter */
               logN,   /* Numero de clusters */
               token,  /* node identifier, natural number */ 
               event,
               r,
               i,
               j;

    static char fa_name[5];

    int *vetorTestadores;  //True se o processo testou ao menos um processo
    int **state;

    if (argc != 2) {
      puts("Uso correto: vcube <num-processos>");
      exit(1);
    }

    N = atoi(argv[1]);
    //calcula log_2(N)
    int aux = N;
    logN = 0;
    while(aux != 0)
    {
        logN++;
        aux = aux/2;
    }
    smpl(0, "Simulacao do vCube");
    reset();
    stream(1);

    printf("=============================================================================\n");
    printf("Inicio da execucao: programa que implementa o detector de falhas vCube\n");
    printf("Prof. Elias P. Duarte Jr.  -  Disciplina Sistemas Distribuidos\n");
    printf("=============================================================================\n");
 
/*----- inicializacao -----*/

    //Inicializa processos
    processo = (TipoProcesso *) malloc(sizeof(TipoProcesso)*N);

    for (i=0; i<N; i++) {
       memset (fa_name, '\0', 5);
       sprintf(fa_name, "%d", i);
       processo[i].id = facility(fa_name,1);
       // printf("fa_name = %s, processo[%d].id = %d\n", fa_name, i, processo[i].id);
    } /* end for */

    printf("Inicializei %d processos\n",N);

    //Inicializa vetor de eventos
    vetorEventos = (TipoEvento*)malloc(N *sizeof(TipoEvento));
    vetorTestadores = (int*)malloc(N*sizeof(int));
    for(int i = 0; i < N;i++)
    {
        vetorTestadores[i] = FALSE;
    }
    qtdEventos = 0;
    inicioVetor = 0;

    //Aloca vetores State[]
    state = (int **)malloc(N*sizeof(int*));
    for(i=0;i<N;i++)
    {
        state[i] = (int *)malloc(N*sizeof(int));
    }

    //Inicializa vetores State[]
    for(i=0;i<N;i++)
    {
        for(j=0;j<N;j++)
        {
            state[i][j] = CORRETO;
        }
    }
    printf("Inicializei o vetor State[] de cada processo\n");


/*----- vamos escalonar os eventos iniciais -----*/

    for (i=0; i<N; i++) {
       schedule(test, INTERVALO_TESTE, i);
     }
    for(i = 0; 2*i < N;i++)
    {
        schedule(fault, 31.0, 2*i);
    }
    for(i = 0; 2*i < N;i++)
    {
        schedule(recovery, 91.0, 2*i);
    }
    for(i = N/2; i < N;i++)
    {
        schedule(fault, 121.0, i);
    }
    for(i = N/2; i < N;i++)
    {
        schedule(recovery, 181.0, i);
    }
    printf("Escalonei os testes iniciais de todos os processos\n");
    printf("Escalonei uma falha de todos os processo pares no tempo 31.0\n");
    printf("Escalonei um recovery de todos os processo pares no tempo 91.0\n");
    printf("Escalonei uma falha do processo %d ao %d no tempo 121.0\n",N/2,N-1);
    printf("Escalonei uma falha do recovery %d ao %d no tempo 181.0\n",N/2,N-1);

    
/*----- agora vem o loop principal do simulador -----*/

    printf("Inicio dos eventos da simulacao\n");

    while (time() < TEMPO_DE_SIMULACAO) {
      cause(&event, &token);
      switch(event) {
       case test: 
            if (status(processo[token].id) != 0) break;  // processo falho não testa!
            for(int s = 1; s <= logN;s++)
            {
                // cria o conjunto dos nodos a serem testados
                node_set* testados = cis(token,s);
                for(int k = 0;k < testados->size;k++)
                {
                    // se eh um nodo que existe
                    int j = testados->nodes[k];
                    if(j < N)
                    {
                        // calcula nodos testadores do nodo j
                        node_set* testadores = cis(j,s);
                        //se eh o primeiro correto
                        if(primeiroCorreto(testadores,token,state[token],N))
                        {
                            //testa processo j
                            incrementaTestes(N);               // contabiliza teste na contagem
                            vetorTestadores[token] = TRUE;     // registra que o processo testou alguem
                            switch(status(processo[j].id))     // testa o proximo processo
                            {
                                case 0:
                                    //Processo testado esta correto
                                    if(state[token][j] == UNKNOWN)
                                    {
                                        state[token][j] = CORRETO;
                                    }
                                    else if(state[token][j]%2 == 1)
                                    {
                                        state[token][j]++;
                                    }
                                    obtemInfo(state, token, j, N);
                                    printf("o processo %d eh o primeiro correto em C(%d,%d) e testou o processo %d no tempo %5.1f e ele estava correto\n",token,j, s , j,time());
                                    break;
                                default:
                                    //Processo testado esta falho
                                    if(state[token][j] == UNKNOWN)
                                    {
                                        state[token][j] = FALHO;
                                    }
                                    else if(state[token][j]%2 == 0)
                                    {
                                        state[token][j]++;
                                    }
                                    printf("o processo %d eh o primeiro correto em C(%d,%d) e testou o processo %d no tempo %5.1f e ele estava falho\n",token,j, s , j,time());
                                    break;
                            }
                            //Apos um teste,verifica se resolveu o evento
                            verificaEventosResolvidos(state,N);   
                        }
                    }
                }
            }

            imprimeState(state,token,N);
            verificaRodadaTeste(vetorTestadores,N);
            
            schedule(test, INTERVALO_TESTE, token);
            break;
       case fault:
            r = request(processo[token].id, token, 0);
            printf("o processo %d falhou no tempo %5.1f\n", token, time());
            //registra dados do evento
            vetorEventos[(inicioVetor+qtdEventos)%N].processoEvento = token;
            vetorEventos[(inicioVetor+qtdEventos)%N].tempoEvento = time();
            vetorEventos[(inicioVetor+qtdEventos)%N].tipoEvento = fault;
            vetorEventos[(inicioVetor+qtdEventos)%N].latencia = 1;
            vetorEventos[(inicioVetor+qtdEventos)%N].qtdTestes = 0;
            qtdEventos++;
            break;
       case recovery:
            release(processo[token].id, token);
            schedule(test, 1.0, token);
            printf("o processo %d recuperou no tempo %5.1f\n", token, time());
            //registra dados do evento
            vetorEventos[(inicioVetor+qtdEventos)%N].processoEvento = token;
            vetorEventos[(inicioVetor+qtdEventos)%N].tempoEvento = time();
            vetorEventos[(inicioVetor+qtdEventos)%N].tipoEvento = recovery;
            vetorEventos[(inicioVetor+qtdEventos)%N].latencia = 1;
            vetorEventos[(inicioVetor+qtdEventos)%N].qtdTestes = 0;
            qtdEventos++;
            break;
      } /* end switch */
    } /* end while */
} /* end tempo.c */
