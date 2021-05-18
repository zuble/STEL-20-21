/**
*
* Lambda = 80 calls/hour
* 30% general purpose / 70% area-specific
*
* GENERAL PURPOSE:
* Exponential distribution with average of 2 min
* Min duration -> 1 min
* Max duration -> 5 min
* If not in limits, re-roll
*
* AREA-SPECIFIC:
* Before being area-specific, the call is general purpose with these params:
* Gaussian distribution with average of 1 min and std dev = 20 sec
* Min duration -> 30 sec
* Max duration -> 120 sec
* ----
* After being transferd to area-specific:
* Exponential distribution with average of 2 min 30 sec
* Min duration -> 1 min
* Max duration -> No Max
*
*
* QUEUES:
* General purposes has a waiting queue with finite lenght
* Area-specific has infinite lenght waiting queue
*
* Prediction of the average waiting time in the queue by using running
* average of the time spent waiting by other callers
*
**/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include "centro_chamadas.h"

#define	lambda 0.022f
#define	dm 120.0f
#define	CHEGADA 0
#define	PARTIDA 1
#define	ESPECIFICO 0
#define	GERAL 1

int main(int argc, char const *argv[]) {

  if(argc != 5){
      printf("Usage ./... total_chamadas total_operadores_geral capacidade_fespera_geral total_operadores_especifico\n");
      return 0;
  }

  int total_chamadas 				= 	atoi(argv[1]);
  int total_operador_geral		= 	atoi(argv[2]);
  int capacidade_fespera_geral 	= 	atoi(argv[3]);
  int total_operador_espcf		= 	atoi(argv[4]);

  lista  *geral = NULL;
  lista  *fespera_geral = NULL;
  lista  *especifico = NULL;
  lista  *fespera_especifico = NULL;

  double delta = (0.2)*(1/lambda);
  double max_delta = 5 * (1/lambda);

  srand(time(0));
  int size = (max_delta/delta);
  int operador_geral_ocupado = 0, operador_espcf_ocupado =0 , area_aux = 0;
  int chamadas_geral_atrasadas = 0 , chamadas_espcf = 0, chamadas_espcf_atrasadas = 0;
  int tamanho_fespera_geral = 0;
  int chamadas_perdidas = 0;
  double d_aux = 0;


  //VAR ESTATISTICA
	double erro_rel_sum = 0, erro_abs_sum = 0 , erro_abs = 0, erro_rel = 0;
  double atraso_aux = 0 , total_atraso_geral = 0 , atraso_espcf_aux = 0 , total_atraso_espcf = 0 ;
	double avg = 0;
	int erro_cont = 0;


  //Dynamically growing Vetor for the "time spend in queues" atraso_aux
  Vetor Vetor_atrasos;
  ini_vetor(&Vetor_atrasos,10);


	//VAR HISTOGRAMA
  int *histograma_geral = (int *)malloc(size*sizeof(int));
  int *histograma_previsao = (int *)malloc(size*sizeof(int));
  FILE *f1 = fopen("atraso.txt","w+");
  if(f1 == NULL)
  	printf("Erro a abrir o ficheiro atraso\n");
  FILE *f2 = fopen("previsao.txt","w+");
  if(f1 == NULL)
  	printf("Erro a abrir o ficheiro previsao\n");


  int eventos_cont = 0;

  while(eventos_cont < total_chamadas) {

  	if( eventos_cont == 0 ) geral = adicionar(geral, CHEGADA, ESPECIFICO, 0, 0);

		// CHEGADA
		if( geral->tipo == CHEGADA ){

    	//GERA A PROXIMA CHAMADA
			eventos_cont++;
			area_aux = get_call_area();
			d_aux = time_between_calls();
			geral = adicionar(geral, CHEGADA, area_aux, 0, (geral->tempo + d_aux));

			//PROCESSA A CHAMADA QUE CHEGOU CASO HAJA OPERADORES DISPONIVEIS
			if(operador_geral_ocupado < total_operador_geral) {

				operador_geral_ocupado++;
				d_aux = duration_of_call_general(geral->area);
				geral = adicionar(geral, PARTIDA, geral->area, 0, (geral->tempo + d_aux));

     //SENAO COLACA-OS NA FILA DE ESPERA CASO HAJA DISPONIBLIDADE NA FILA DE ESPERA
			} else if(tamanho_fespera_geral < capacidade_fespera_geral) {

				fespera_geral = adicionar(fespera_geral, CHEGADA, geral->area, 0, geral->tempo);
				tamanho_fespera_geral++;
				chamadas_geral_atrasadas++;
				//printf("Running average of sample %d_aux\t %lf \n", eventos_cont, avg);

		//SENAO CHAMADA É PERDIDA
			} else
			chamadas_perdidas++; //QUEUE FULL -> BLOCKED

		}

    // PARTIDA
		if( geral->tipo == PARTIDA ){

			operador_geral_ocupado--;

			// PROCESSA EVENTUAIS CHAMADAS NA FILA DE ESPERA GERAL
			if(fespera_geral != NULL && capacidade_fespera_geral > 0) {

				operador_geral_ocupado++;
				atraso_aux = geral->tempo - fespera_geral->tempo;
				fespera_geral->atraso_sofrido += atraso_aux;
				total_atraso_geral += atraso_aux;

				erro_abs = fabs(atraso_aux-avg);
				erro_cont++;
				erro_rel_sum += erro_rel;
				erro_abs_sum += erro_abs;

				// ADD PREVIOUS (eventos_cont-1) PREDICTION ERROR
				//histograma_previsao = histogram(erro_abs, size, histograma_previsao, delta);
	      //histograma_geral = histogram(atraso_aux, size, histograma_geral, delta);

	      exportacao_TXT(f1, atraso_aux );
        exportacao_TXT(f2, erro_abs );

				// COMPUTE RUNNING AVG FOR CURRENT I
				avg = running_average(erro_cont, atraso_aux, avg);

	      // GERA O FIM DA CHAMADA QUE ESTA NA FILA DE ESPERA GERAL E REMOVE-A
				d_aux = duration_of_call_general(fespera_geral->area);
				geral = adicionar(geral, PARTIDA, fespera_geral->area, fespera_geral->atraso_sofrido, (geral->tempo + d_aux));
				fespera_geral = remover(fespera_geral);
				tamanho_fespera_geral--;

			}

  		// ESPECIFICO
  		if(geral->area == ESPECIFICO) {

  			chamadas_espcf++;

  			// ADICIONA EVENTO EM GERAL Á LISTA ESPECIFICO
  			especifico = adicionar(especifico, CHEGADA, ESPECIFICO, geral->atraso_sofrido, geral->tempo);

  			// SEGURANÇA PARA LISTA ESPECIFICO TER CERTEZA QUE ALCANÇA A LISTA GERAL
  			while(especifico->tempo < geral->tempo && total_operador_espcf > 0 && especifico != NULL) {

  				if( especifico->tipo == PARTIDA ){
  					operador_espcf_ocupado--;

  					// PROCESSA EVENTUAIS CHAMADAS NA FILA DE ESPERA ESPECIFICA
  					if(fespera_especifico != NULL){

  						operador_espcf_ocupado++;

  						atraso_espcf_aux = (especifico->tempo - fespera_especifico->tempo);
  						fespera_especifico->atraso_sofrido += atraso_espcf_aux;
  						total_atraso_espcf += atraso_espcf_aux; //ACUMULA O TEMPO QUE A CHAMADA ESTEVE NA FILA DESPERA ESPECIFICA

  						// ATUALIZA VETOR DE ATRASOS E GERA PARTIDA DA CHAMADA ESPECIFICA EM FILA DESPERA
  						d_aux = duration_of_call_specific();
  						ins_vetor(&Vetor_atrasos, fespera_especifico->atraso_sofrido); //INSERE ATRASO NA F.ESPERA ESPECIFICA + EVENTUAL ATRASO NA F.ESPERA GERAL
  						especifico = adicionar(especifico, PARTIDA, ESPECIFICO, fespera_especifico->atraso_sofrido, (especifico->tempo + d_aux));
  						fespera_especifico = remover(fespera_especifico);
  					}

  				}

  				if( especifico->tipo == CHEGADA ){

  					// CASO HAJA OPERADORES DISPONIVEIS PROCESSA A CHAMADA
  					if(operador_espcf_ocupado < total_operador_espcf) {

  						// ATUALIZA VETOR DE ATRASOS E GERA PARTIDA DA CHAMADA ESPECIFICA
  						operador_espcf_ocupado++;
  						d_aux = duration_of_call_specific();
  						ins_vetor(&Vetor_atrasos, especifico->atraso_sofrido); //INSERE EVENTUAL ATRASO NA F.ESPERA GERAL
  						especifico = adicionar(especifico, PARTIDA, ESPECIFICO, especifico->atraso_sofrido, (especifico->tempo + d_aux));

  					// SENAO COLOCA CHAMADA NA FILA DE ESPERA ESPECIFICA ( INFINITA )
  					} else {

  						fespera_especifico = adicionar(fespera_especifico, CHEGADA, ESPECIFICO, especifico->atraso_sofrido, especifico->tempo);
  						chamadas_espcf_atrasadas++;

  					}
  				}

  				especifico = remover(especifico);

  			}
  		}
  	}
    geral = remover(geral);
  }

  //printGraph(histograma_previsao,25);
	printf("\n\tN total chamadas = %d | Tamanho fila espera geral = %d | N operadores geral = %d | N operadores especifico = %d \n\n", total_chamadas , capacidade_fespera_geral , total_operador_geral, total_operador_espcf);


  printf("Probablidade de uma chamada geral ser perdida :\t %lf%%\n",(double)chamadas_perdidas/total_chamadas *100);
  printf("Probablidade de uma chamada geral sofrer atraso :\t %lf%%\n",(double)chamadas_geral_atrasadas/total_chamadas *100);
	printf("Probablidade de uma chamada especifica sofrer atraso :\t %lf%%\n\n",(double)chamadas_espcf_atrasadas/chamadas_espcf *100);

  printf("Atraso médio das chamadas geral atrasadas :\t %lf segundos\n",(double)total_atraso_geral/chamadas_geral_atrasadas);
  printf("Atraso médio das chamadas especifica atrasadas :\t %lf segundos \n", (double)total_atraso_espcf/chamadas_espcf_atrasadas);

	double avg_queue_time_spec = media(Vetor_atrasos.vetor, Vetor_atrasos.tamanho);
	//printf("vetor.tamanho : %ld  |  n chamadas espcf : %d\n" , Vetor_atrasos.tamanho , chamadas_espcf);
  printf("Tempo médio em fila de espera geral + especifica das chamadas especificas  :\t %lf segundos \n", avg_queue_time_spec);


	printf("\n---------------------------------------------\n");

	double avg_abs =  (double)erro_abs_sum/erro_cont;
	erro_rel = fabs(erro_abs/ ( (double)total_atraso_geral/chamadas_geral_atrasadas ) );
  printf("Avg Abs Prediction Error:\t %lf\n", avg_abs);
  printf("Avg Rel Prediction Error:\t %lf%%\n\n", erro_rel *100);


	double desv_padrao = stddev(Vetor_atrasos.vetor , Vetor_atrasos.tamanho, avg_queue_time_spec);
	double erro_padrao_media = desv_padrao/(sqrt(Vetor_atrasos.tamanho));

	double limite_confi = 1.65;
	double interv_confi = limite_confi*erro_padrao_media;
	printf("\nConfidance Interval, with limite of 90%% %lf\n\n", interv_confi);

	rem_vetor(&Vetor_atrasos);

	/*
	printf("\n---------------------------------------------\n");
	exportacao_CSV( histograma_geral , 0);
	exportacao_CSV( histograma_previsao , 1);
	*/

  return 1;
}
