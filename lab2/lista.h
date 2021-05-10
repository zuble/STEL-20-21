#include <stdio.h>
#include <stdlib.h>
#ifndef LIST_H
#define LIST_H

typedef struct{
	int tipo;
	double tempo;
	struct lista * proximo;
} lista;

// Função que remove o primeiro elemento da lista
lista * remover (lista * apontador);
// Função que adiciona novo elemento na lista, ordenando a mesma por tempo
lista * adicionar (lista * apontador, int n_tipo, double n_tempo);
// Função que imprime no ecra todos os elementos da lista
void imprimir (lista * apontador);

#endif
