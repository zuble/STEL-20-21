#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <fcntl.h>
#include "lista.h"

#define DELTA 1E-3
#define CHEGADA 0
#define PARTIDA 1
#define N_EVENTOS 10000
#define lambda 200 // frequência de chegada chamadas/ms
#define dm 0.008  // tempo médio de serviço , ms

void importacao_variaveis(int* NN , int * LL , double* axx) {

  	int aux = 0;

	while( aux == 0){

    	fprintf(stdout, "\n\tNumero total de canais/servidores (N) || Tamanho do buffer/queue (L)\n\t");
    	scanf("%d %d", NN , LL );

    	if ( *NN < 1 || *LL < 0) printf("\t N >= 1 || L = 0 (Earlang-B) , L=100000 (Earlang-C)");
    	else aux = 1;
  	}

	fprintf(stdout, "\n\tvalor do atraso (ax) para o calculo da P( A > ax )    ||    introduzir 0 para ignorar\n\t");
    scanf("%le", axx );

}

int contador(lista *tempo_atual) { 
    int cont = 0;  // Initialize cont 
    while (tempo_atual != NULL) { 
        cont++; 
        tempo_atual = (lista *)tempo_atual -> proximo; 
    } 
    return cont; 
} 

float media(float *vetor,int tam){
	float soma;
	for(int i=0;i < tam;i++) soma+=vetor[i];
	return (soma/tam);
}

int exportacao_CSV(int *histograma , int tam){
  	FILE* nome_ficheiro;
  	nome_ficheiro = fopen("data.csv","w+");

  	if(nome_ficheiro == NULL) printf("Erro a abrir o ficheiro\n");
	
	fprintf(stdout, "\n\tA exportar para data.csv");
  	
	for (int i = 0; i < tam; i++)
      	fprintf(nome_ficheiro, "%lf,%d\n", (2*i+1)/(float)(tam*2), histograma[i]);
    
	fprintf(stdout, "\n\t\tFIM\n");
}

float calculo_tempo(int tipo) {
  float u, C, D;
  u = (float)(rand()+1)/RAND_MAX;

  if (tipo == CHEGADA) {
    C = -(log(u)/(float)lambda);
    return C;

  } else { 
    D = -(log(u)*(float)dm);
    return D;
  }
}

int main(){

	int N_canais_em_uso = 0 , N_eventos_chegada = 0 , N_eventos_partida = 0 , N_eventos_perdidos = 0 , N_eventos_atrasados = 0 , N_eventos_atrasados_ax = 0;
 
	lista  * eventos , *buffer;
	eventos = NULL;
	buffer = NULL;
	
	int N_canais_total , N_buffers_total ;
	double ax ; 
  	importacao_variaveis(&N_canais_total , &N_buffers_total , &ax);
	
	//D = duracao da chamada , C = tempo entre uma chamadas e chamada seguinte
	double delta , D = 0.0 , C = 0.0 , tempo_atual = 0.0;
	
	delta = (float)(1.0/lambda);// valor max para delta
	delta *=(float)1/5;
	int N = 1/delta;

	int seed = clock();
	srand(seed);

	float *atrasados= malloc(sizeof(double)*(N_EVENTOS+2));
	int *histograma=malloc(sizeof(double)*(N_EVENTOS+2));

	memset(histograma,0,0);
	memset(atrasados,0,0);

	D = calculo_tempo(PARTIDA);
	eventos = adicionar(eventos, CHEGADA, 0);
	eventos = adicionar(eventos, PARTIDA , D); 

	for ( int i = 0; i < N_EVENTOS; i++ ){
		
		C = calculo_tempo(CHEGADA);
		D = calculo_tempo(PARTIDA); 
			
		if( eventos->tipo == PARTIDA ){ //fim da chamada
			
			tempo_atual = eventos->tempo;
			eventos = remover(eventos);
			N_canais_em_uso--;
			++N_eventos_partida;

			if( buffer != NULL ){ // se há elementos no buffer
				atrasados[N_eventos_atrasados-1] = tempo_atual - buffer->tempo;

				if( atrasados[N_eventos_atrasados-1] > ax )//se chamas tem atraso maior que o introduzido
					N_eventos_atrasados_ax++;
					
				buffer = remover(buffer);
				eventos = adicionar(eventos,PARTIDA,tempo_atual+D);
				++N_canais_em_uso;
				
			}
		}

		else if ( eventos->tipo == CHEGADA ){ //inicio da chamada
			
			N_eventos_chegada++;
			tempo_atual = eventos->tempo;
			eventos = remover(eventos);
			eventos = adicionar(eventos, CHEGADA, tempo_atual+C); 

			if( N_canais_em_uso < N_canais_total ){ // existe servidores disponiveis
				N_canais_em_uso++;

				if( (contador(buffer)) == 0 )
					eventos = adicionar(eventos, PARTIDA, tempo_atual+D); 
				
			}

			else if( (contador(buffer)) < N_buffers_total ){ // existe buffers disponiveis
				++N_eventos_atrasados;			 
				buffer = adicionar(buffer, CHEGADA, tempo_atual);
			}

			else  // servidor e buffer ocupado siginifica evento perdido
				++N_eventos_perdidos;
							 
		}

		if( N_canais_em_uso < 0 ) N_canais_em_uso=0;
		
	}

	float media_atraso_chamadas= media(atrasados,N_eventos_atrasados);
	float prob_bloqueio = (float) N_eventos_atrasados/N_eventos_chegada*100;
	float prob_perda = (float) N_eventos_perdidos/N_eventos_chegada*100;
    
	printf("\n\tNúmero eventos de chegada :%d\n",N_eventos_chegada);
	printf("\tNúmero eventos de partida :%d\n",N_eventos_partida);
	printf("\tNúmero de chamadas bloqueadas : %d\n",N_eventos_atrasados);
	printf("\tNúmero de chamadas perdidas : %d\n",N_eventos_perdidos);
	printf("\tProbabilidade de perda (B) : %f%%\n",prob_perda);
	printf("\tProbabilidade de atraso (Pa) : %f%%\n",prob_bloqueio);
	if( ax != 0)
		printf("\tProbabilidade de chamada ter atraso > ax = %lf : %f%% \n", ax , ((float)(N_eventos_atrasados_ax*1.0)/N_eventos_atrasados)*100 );
	printf("\tMédia de atraso das chamadas (Am) : %f\n",media_atraso_chamadas);
    
	for (int j = 0; j < N_eventos_atrasados-1; j++){
		for (int i = 0;i < N/4;i++){
			if((atrasados[j] >= i*delta) && (atrasados[j] < (i+1)*delta))
				histograma[i]++;
		}
	}

  	exportacao_CSV(histograma,N);

}