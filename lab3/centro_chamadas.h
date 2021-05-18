#include <stdio.h>
#include <stdlib.h>

#define	lambda 0.022f
#define	dm 120.0f
#define	CHEGADA 0
#define	PARTIDA 1
#define	ESPECIFICO 0
#define	GERAL 1
//valores relativos á distr gaussiana da duracao duma chamada especifica até ser atendida por um operador espcf
#define st_desv 20
#define mean 60

#ifndef M_PI
#define M_PI (3.14159265358979323846264338327950288)
#endif


typedef struct {
  double *vetor;
  size_t tamanho;
  size_t capacidade;
} Vetor;

// Definição da estrutura da lista
typedef struct{
	int tipo; //CHEGADA ou PARTIDA
	int area; //GERAL Ou ESPECIFICO
	double atraso_sofrido;
	double tempo;
	struct lista * proximo;
} lista;


// Função que remove o primeiro elemento da lista
lista * remover (lista * apontador){
  
	lista * lap = (lista *)apontador -> proximo;
	free(apontador);
	return lap;
}

// Função que adiciona novo elemento à lista, ordenando a mesma por tempo
lista * adicionar (lista * apontador, int n_tipo, int area, double atraso_sofrido, double n_tempo){

	lista * lap = apontador;
	lista * ap_aux, * ap_next;
	if(apontador == NULL)
	{
		apontador = (lista *) malloc(sizeof (lista));
		apontador -> proximo = NULL;
		apontador -> tipo = n_tipo;
		apontador -> area = area;
		apontador -> atraso_sofrido = atraso_sofrido;
		apontador -> tempo = n_tempo;
		return apontador;
	}
	else
	{
		if (apontador->tempo > n_tempo) {
	        ap_aux = (lista *) malloc(sizeof (lista));
	        ap_aux -> tipo = n_tipo;
	        ap_aux -> area = area;
			ap_aux -> atraso_sofrido = atraso_sofrido;
            ap_aux -> tempo = n_tempo;
            ap_aux -> proximo = (struct lista *) apontador;
            return ap_aux;
	    }

		ap_next = (lista *)apontador -> proximo;
		while(apontador != NULL)
		{
			if((ap_next == NULL) || ((ap_next -> tempo) > n_tempo))
				break;
			apontador = (lista *)apontador -> proximo;
			ap_next = (lista *)apontador -> proximo;
		}
		ap_aux = (lista *)apontador -> proximo;
		apontador -> proximo = (struct lista *) malloc(sizeof (lista));
		apontador = (lista *)apontador -> proximo;
		if(ap_aux != NULL)
			apontador -> proximo = (struct lista *)ap_aux;
		else
			apontador -> proximo = NULL;
		apontador -> tipo = n_tipo;
		apontador -> area = area;
		apontador -> atraso_sofrido = atraso_sofrido;
		apontador -> tempo = n_tempo;
		return lap;
	}
}

// Função que imprime no ecra todos os elementos da lista
void imprimir (lista * apontador){

	if(apontador == NULL)
		printf("Lista vazia!\n");
	else
	{
		while(apontador != NULL)
		{
			printf("Tipo=%d\tArea=%d\tTempo=%lf\n", apontador -> tipo, apontador -> area, apontador -> tempo);
			apontador = (lista *)apontador -> proximo;
		}
	}
}

// /*return random number between zero and one */
double generate_random() {
	return ((double) rand()+1)/RAND_MAX;
}

// /*returns time between calls*/
double time_between_calls(){
	double u = generate_random(), r;
	return r = -(1/lambda)*log(u);
}

// /*returns if a call is general or general+specific*/
int get_call_area(){

	double p = generate_random();
	int x;
	if (p <= 0.3) x = GERAL;
	else x = ESPECIFICO;
	return x;
}

// returns duration of the call in the area specific chhannel*/
double duration_of_call_general(int area){
	double r;

	if( area == GERAL ){
		//exponential avg 120, min 60 and max 300
		double u =generate_random();
		r =(double) 60 -dm*log(u);
	
		if( r > (double) 300) r = (double) 300;

	} 
	if( area == ESPECIFICO ){
		//gaus avg 60, std 20, min 30, max 120
		double u2 , u;
		double teta;

		do{
			u2=generate_random();
			u=generate_random();
			teta = 2*M_PI*u;
			r = (sqrt(-2*log(u2))*cos(teta));
			r = ((r*st_desv) + mean);
		}while( r < 30 );

		if( r > (double) 120) r = (double) 120;
	
	}

	return r;
}

// /*returns duration of the call in the area specific chhannel*/
double duration_of_call_specific(){
	double u , r, d = 150.0;

	do{
		u = generate_random();
		r = -d * log(u);
	}while( r < 60);

	return r;
}

// /*returns running media delay*/
double running_average(int n, double current_sample, double previous_avg){
		return (previous_avg*(n-1) + current_sample)/n;
}

double media(double* data, int n){
    int i;
    double s = 0;

    for ( i = 0; i < n; i++ )
        s += data[i];
    return s / n;
}

double stddev(double* data, int n, double media){
    int i;
    double s = 0;

    for ( i = 0; i < n; i++ )
        s += (data[i] - media) * (data[i] - media);
    return sqrt(s / (n - 1));
}

void ini_vetor(Vetor *v, size_t tam_inicial) {
  v->vetor = (double *)malloc(tam_inicial * sizeof(double));
  v->tamanho = 0;
  v->capacidade = tam_inicial;
}

void ins_vetor(Vetor *v, int valor) {
  if (v->tamanho == v->capacidade) {
    v->capacidade *= 2;
    v->vetor = (double *)realloc(v->vetor, v->capacidade * sizeof(double));
  }
  v->vetor[v->tamanho++] = valor;
}

void rem_vetor(Vetor *v) {
  free(v->vetor);
  v->vetor = NULL;
  v->tamanho = 0;
  v->capacidade = 0;
}

/*
int exportacao_CSV(int *histograma , int type){
	FILE* f1;

	if( type == 0 ){
		f1 = fopen("atraso.csv","w+");

		if(f1 == NULL) 
			printf("Erro a abrir o ficheiro\n");
		else
			fprintf(stdout, "\n\tA exportar para atraso.csv");
		
	}
  	else {
		f1 = fopen("previsao.csv","w+");  

		if(f1 == NULL) 
			printf("Erro a abrir o ficheiro\n");
		else
			fprintf(stdout, "\n\tA exportar para previsao.csv");
	}

	for (int i = 0; i < sizeof(histograma); i++)
      	fprintf(f1, "%d , %d\n", i, histograma[i]);
    
	fprintf(stdout, "\n\t\tFIM\n");
}
*/

void exportacao_TXT(FILE *f1, double data){
    fprintf(f1 , "%lf\n" , data);
}