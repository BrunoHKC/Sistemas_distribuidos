/* --------------------------------------------------------------------------------------
   Implementacao o detector de falhas vCube no ambiente de simulação SMPL
   Objetivo: Mostrar a execucao do detector de falhas vCube com chance de realizar falsas suspeitas
   Restricoes: -

   Autor: Bruno Henrique Kamarowski de Carvalho
   Disciplina: Sistemas Distribuidos
   Data da ultima atualizacao: 08/02/2023
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

int *vetorFalsoSuspeito;
int *vetorQtdRodadas;
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
        state_i[k] = state_i[k] > state_j[k]? state_i[k]:state_j[k];
    }
}


void incrementaRodadas(int N)
{
    for(int i = 0;i < N;i++)
    {
        if(vetorFalsoSuspeito[i])
        {
            vetorQtdRodadas[i]++;
        }
    }
}


int pertence(int* pilha,int topo,int x)
{

    for(int i = 0; i < topo; i++)
    {
        if(pilha[i] == x)
            return TRUE;
    }
    return FALSE;
}


int configuacaoInteressante(int** state,int N)
{
    int* componente = malloc(N*sizeof(int));
    for(int i = 0; i < N; i++)
    {
        if(status(processo[i].id) == 0)
            componente[i] = -1;
        else
            componente[i] = -2;
    }
    int* pilha = malloc(N*sizeof(int));
    int topo = -1;
    int root = -1;

    for(int i = 0; i < N;i++)
    {
        if(componente[i] == -1)
        {
            root = i;
            topo++;
            pilha[topo] = root;
        }

        while(topo >= 0)
        {
            int current = pilha[topo];
            topo--;
            componente[current] = root;

            for(int j = 0; j < N;j++)
            {
                if(j == current) continue;

                if(state[current][j]%2 == 0)
                {
                    if(!pertence(pilha,topo,j) && componente[i] == -1)
                    {
                        topo++;
                        pilha[topo] = j;
                        componente[j] = root;
                    }
                }
            }

        }        
    }

    int first = -1;
    int recorrentFirst = 0;
    int recorrentSecond = 0;
    int second = -1;
    int three = FALSE;
    for(int i = 0; i < N;i++)
    {
        if(componente[i] != -1 && componente[i] !=-2 && first == -1)
        {
            first = componente[i];
        }
        else
        {
            if(componente[i] == first)
                recorrentFirst = TRUE;
            if(second != -1 && componente[i] == second)
                recorrentSecond = TRUE;
            
            if(first != -1 && second != -1 && componente[i] != -1 && componente[i] !=-2)
                three = TRUE;

            if(componente[i] != -1 && componente[i] !=-2 && componente[i] != first && second == -1)
            {
                second = componente[i];
            }


            if((recorrentFirst && recorrentSecond) || three)
            {
                printf("componentes: {");
                for(int l = 0; l < N;l++)
                {
                    printf(" %d ",componente[l]);
                }
                printf("}\n");
                return TRUE;
            }   
        }
    }
    
    return FALSE;
}


int configInteressante = FALSE;
void verificaRodadaTeste(int * vetorTestadores,int N,int** state)
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
        incrementaRodadas(N);

        /*
        if(configuacaoInteressante(state,N))
        {
            if(!configInteressante)
                configInteressante = TRUE;
            else
                printf("ACONTECEU UM EVENTO INTERESSANTE NO TEMPO %5.1f!!\n",time());
        }
        */
    }
}



int main (int argc, char *argv[]) {
    static int N,      /* number of nodes is parameter */
               probFalha, /* probabilidade de realizar uma falsa suspeita*/
               logN,   /* Numero de clusters */
               token,  /* node identifier, natural number */ 
               event,
               r,
               i,
               j;

    static char fa_name[5];

    int *vetorTestadores;  //True se o processo testou ao menos um processo
    int **state;

    if (argc != 4) {
      puts("Uso correto: vcube <num-processos> <percentual-falsa-suspeita> <semente-aleatoria>");
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

    probFalha = atoi(argv[2]);

    if(probFalha < 0 || probFalha > 100)
    {
        puts("A probabilidade de falha deve ser um valor entre 0 e 100");
        exit(1);
    }
    
    smpl(0, "Simulacao do vCube assincrono");
    reset();
    stream(1);

    printf("================================================================================================\n");
    printf("Inicio da execucao: programa que implementa o detector de falhas vCube em um sistema assincrono\n");
    printf("Prof. Elias P. Duarte Jr.  -  Disciplina Sistemas Distribuidos\n");
    printf("===============================================================================================\n");
 
/*----- inicializacao -----*/


    srand(atoi(argv[3]));   // Initialization, should only be called once.

    //Inicializa processos
    processo = (TipoProcesso *) malloc(sizeof(TipoProcesso)*N);

    for (i=0; i<N; i++) {
       memset (fa_name, '\0', 5);
       sprintf(fa_name, "%d", i);
       processo[i].id = facility(fa_name,1);
    } /* end for */

    printf("Inicializei %d processos\n",N);
    printf("A probabilidade de cometer uma falsa suspeita eh de %d%% \n",probFalha);

    //Inicializa vetor dos falsos suspeitos e de qtd de rodadas que esta falso suspeito
    vetorFalsoSuspeito = (int*)malloc(N*sizeof(int));
    vetorQtdRodadas = (int*)malloc(N*sizeof(int));
    for(int i = 0; i < N;i++)
    {
        vetorFalsoSuspeito[i] = FALSE;
        vetorQtdRodadas[i] = 0;
    }

    //Inicializa vetor dos processos testados
    vetorTestadores = (int*)malloc(N*sizeof(int));
    for(int i = 0; i < N;i++)
    {
        vetorTestadores[i] = FALSE;
    }

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
    printf("Escalonei os testes iniciais de todos os processos\n");
    
/*----- agora vem o loop principal do simulador -----*/

    printf("Inicio dos eventos da simulacao\n");

    while (time() < TEMPO_DE_SIMULACAO) {
      cause(&event, &token);
      int encerrouExecucao = FALSE;
      switch(event) {
       case test: 
            if (status(processo[token].id) != 0) break;  // processo falho não testa!
            for(int s = 1; s <= logN && !encerrouExecucao;s++)
            {
                // cria o conjunto dos nodos a serem testados
                node_set* testados = cis(token,s);

                for(int k = 0;k < testados->size && !encerrouExecucao;k++)
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
                            vetorTestadores[token] = TRUE;     // registra que o processo testou alguem

                            //processo i testa processo j
                            int estadoProcessoJ = (status(processo[j].id) == 0) && (probFalha <= (rand()%100))? CORRETO: FALHO;
                            int falsaSuspeita = (status(processo[j].id) == 0 && (estadoProcessoJ != CORRETO));

                            if(falsaSuspeita)
                            {
                                vetorFalsoSuspeito[token] = TRUE;
                            }

                            //computabiliza resultado do teste
                            if(estadoProcessoJ == CORRETO)
                            {
                                //Processo testado esta correto
                                if(state[token][j] == UNKNOWN)
                                {
                                    state[token][j] = CORRETO;
                                }
                                else if(state[token][j]%2 == 1)
                                {
                                    state[token][j]++;
                                }
                                printf("O processo %d testou o processo %d no tempo %5.1f e ele estava correto\n",token,j,time());
                                obtemInfo(state, token, j, N);

                                //Se foi vitma de falsa suspeita
                                if(state[token][token] > 0)
                                {
                                    printf("O processo %d descobriu no tempo %5.1f que foi vitima de falsa suspeita durante %d rodadas\n",token,time(),vetorQtdRodadas[token]);
                                    encerrouExecucao = TRUE;
                                    r = request(processo[token].id, token, 0);
                                    printf("O processo %d encerrou sua execucao no tempo %5.1f\n",token,time());
                                }
                            }
                            else
                            {
                                //Processo testado esta falho
                                if(state[token][j] == UNKNOWN)
                                {
                                    state[token][j] = FALHO;
                                }
                                else if(state[token][j]%2 == 0)
                                {
                                    state[token][j]++;
                                }
                                if(falsaSuspeita)
                                {
                                    printf("O processo %d testou o processo %d no tempo %5.1f e possui uma falsa suspeita dele\n",token,j, time());
                                }
                                else
                                {
                                    printf("O processo %d testou o processo %d no tempo %5.1f e possui uma suspeita legitima dele\n",token,j, time());
                                }
                            }
                        }
                    }
                }
            }

            if(!encerrouExecucao)
            {
                schedule(test, INTERVALO_TESTE, token);
                imprimeState(state,token,N);
            }
            verificaRodadaTeste(vetorTestadores,N,state);
            break;
       case fault:
            r = request(processo[token].id, token, 0);
            printf("O processo %d falhou no tempo %5.1f\n", token, time());

            break;
       case recovery:
            release(processo[token].id, token);
            schedule(test, 1.0, token);
            printf("O processo %d recuperou no tempo %5.1f\n", token, time());
            break;
      } /* end switch */
    } /* end while */
} /* end tempo.c */
