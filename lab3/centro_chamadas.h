#include <stdio.h>
#include <stdlib.h>

#define	dm 120.0f
#define	CHEGADA 0
#define	PARTIDA 1
#define	ESPECIFICO 0
#define	GERAL 1
//valores relativos á distr gaussiana da duracao duma chamada especifica até ser atendida por um operador espcf
#define stdrt_dev_gauss 20
#define media_gauss 60

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
double criacao_random() {
	return ((double) rand()+1)/RAND_MAX;
}

// /*returns time between calls*/
double tempo_entre_chamadas(double lambda){
	double u = criacao_random(), r;
	return 	r = -(1/lambda)*log(u);
}

// /*returns if a call is general or general+specific*/
int area(){

	double p = criacao_random();
	int x;
	if (p <= 0.3) x = GERAL;
	else x = ESPECIFICO;
	return x;
}

// returns duration of the call in the area specific chhannel*/
double duracao_chamada_geral(int area){
	double r;

	if( area == GERAL ){
		//exponential avg 120, min 60 and max 300
		double u = criacao_random();
		r =(double) 60 -dm*log(u);
	
		if( r > (double) 300) r = (double) 300;
		
	} 
	if( area == ESPECIFICO ){
		//gaus avg 60, std 20, min 30, max 120
		double u2 , u;
		double teta;

		do{
			u2=criacao_random();
			u=criacao_random();
			teta = 2*M_PI*u;
			r = (sqrt(-2*log(u2))*cos(teta));
			r = ((r*stdrt_dev_gauss) + media_gauss);
		}while( r < 30 );

		if( r > (double) 120) r = (double) 120;
	
	}

	return r;
}

// /*returns duration of the call in the area specific chhannel*/
double duracao_chamada_espcf(){
	double u , r, d = 150.0;

	do{
		u = criacao_random();
		r = -d * log(u);
	}while( r < 60);

	return r;
}

// /*returns running media delay*/
double running_average(int n, double atraso_atual, double avg_anterior){
		return (avg_anterior*(n-1) + atraso_atual)/n;
}

double calc_media(double* data, int n){
    int i;
    double s = 0;

    for ( i = 0; i < n; i++ )
        s += data[i];
    return s / n;
}

double calc_desvio_standard(double* data, int n, double media){
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

int *histograma_insere(double data, int size, int * histogram, double delta){

	
	for(int z = 0; z < size; z++){
		if(z == 24 && data >= (z+1)*delta)
			histogram[z]++;
		else if(data >= z*delta && data < (z+1)*delta) {
			histogram[z]++;
		}
		
	}
	
	return histogram;
}

int histograma_conta( int * h){
	int u = 0;
	for(int z = 0; z < 25; z++){
		u += h[z];
	}
	return u;
}

void exportacao_hist (FILE *f1 , int *histogram , int tam , double delta , int h ){
	if( h == -1 ) delta = -delta;

	for (int i=0; i < tam; i++){
		fprintf(f1, "%d, %lf, %d\n", i, i*delta , histogram[i]);
	}
}

void exportacao_aux(FILE *f1, double data , int lambdaa , int aux ){

	if( aux == 0 ){ // print inical dos histogramas para f1/f2/f3
		fprintf(f1 , "*********************\n");
		fprintf(f1 , "LAMBDA : %d\n" , lambdaa);
	}

	if( aux == 2 ){ // print dos valores para f1raw/f2raw
		fprintf(f1 , "%lf\n" , data);
	}

}

int exportacao_sensib( double * sens_atrasa , int * sens_lamba , int sens_atrasa_tam){
	
	FILE *sens = fopen("sensibilidade.txt", "wb");
	if(sens == NULL) {
		printf("Erro a abrir o ficheiro sensibilidade\n");
		return -1;
	}
	fprintf(sens, "Lambda, Atraso total chamadas espcf [b] \n");
	for (int k = 0; k <= sens_atrasa_tam; k++) 
		fprintf(sens, "%d, %lf\n", sens_lamba[k], sens_atrasa[k]);

	fclose(sens);	

}

/* double delta_max = 5 * (double)1/((double)(sensib_lambda_aux/3600));
int hist_tam = (int)(delta_max/delta); // hist_tam = 25 para lambda = 80 c/h */