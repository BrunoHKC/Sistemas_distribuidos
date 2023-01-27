/* --------------------------------------------------------------------------------------
   Header file da implementacao da funcao C(i,s) especificada emsala
   Objetivo: Retornar o cluster a ser testdo pelo processo i na iteracao s
   Restricoes: -

   Autor: Desconhecido, arquivo editado por Bruno Henrique Kamarowski de Carvalho
   Disciplina: Sistemas Distribuidos
   Data da ultima atualizacao: 12/01/2023
----------------------------------------------------------------------------------------*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define POW_2(num) (1<<(num))
#define VALID_J(j, s) ((POW_2(s-1)) >= j)

typedef struct node_set {
	int* nodes;
	ssize_t size;
	ssize_t offset;
} node_set;

static node_set* set_new(ssize_t size);
static void set_insert(node_set* nodes, int node);
static void set_merge(node_set* dest, node_set* source);
static void set_free(node_set* nodes);
node_set* cis(int i, int s);