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

#define	CHEGADA 0
#define	PARTIDA 1
#define	ESPECIFICO 0
#define	GERAL 1

#define hist_tam 25

int main(int argc, char const *argv[]) {

    if(argc != 5){
        printf("Usage ./... total_chamadas   total_operadores_geral   capacidade_fespera_geral   total_operadores_especifico\n");
        return 0;
    }

	/************************************************************************************************************/
    int sensib_lambda_min = 0, sensib_lambda_max = 0, sensib_lambda_salto = 0, sensib_lambda_aux = 0;

	printf("\n\n\tVALORES SENSIBILIDADE LAMBDA \n");
	do {
		printf("\tlambda minimo: ");
		scanf("%d", &sensib_lambda_min);
	} while (sensib_lambda_min < 0);

	do {
		printf("\tlambda maximo: ");
		scanf("%d", &sensib_lambda_max);
	} while (sensib_lambda_max < sensib_lambda_min);

	if( sensib_lambda_min == sensib_lambda_max ){
		sensib_lambda_salto = 1;
	}
	else {
		do {
		printf("\tvalor dos saltos entre extremos de lambda: ");
		scanf("%d", &sensib_lambda_salto);
		} while (sensib_lambda_salto <= 0);
	}

	
	/************************************************************************************************************/

	int total_chamadas 				= 	atoi(argv[1]);
	int total_operador_geral		= 	atoi(argv[2]);
	int capacidade_fespera_geral 	= 	atoi(argv[3]);
	int total_operador_espcf		= 	atoi(argv[4]);

	FILE *f1 = fopen("atraso.txt","wb");
	if( f1 == NULL ){ printf("Erro a abrir o ficheiro atraso\n"); return -1; }
	FILE *f1raw = fopen("atraso_raw.txt","wb");
	if( f1raw == NULL ){ printf("Erro a abrir o ficheiro atrasoraw\n"); return -1; }
	FILE *f2 = fopen("prev_pos.txt","wb");
	if( f2 == NULL ){ printf("Erro a abrir o ficheiro prev_pos\n"); return -1; }
	FILE *f2raw = fopen("prev_raw.txt","wb");
	if( f2raw == NULL ){ printf("Erro a abrir o ficheiro prev_raw\n"); return -1; }
	FILE *f3 = fopen("prev_neg.txt","wb");
	if( f3 == NULL ){ printf("Erro a abrir o ficheiro prev_neg\n"); return -1; }
	FILE *sens = fopen("sensibilidade.txt", "wb");
	if( sens == NULL ){ printf("Erro a abrir o ficheiro sensibilidade\n"); return -1; }
	
	
	for (sensib_lambda_aux = sensib_lambda_min; sensib_lambda_aux <= sensib_lambda_max; sensib_lambda_aux += sensib_lambda_salto) {

		printf("\n***********************************************************************************************************************");
		printf("\n*************  MIN : %d  ******************   INICIO LAMBDA = %d   ********************  MAX : %d  ********************" , sensib_lambda_min , sensib_lambda_aux , sensib_lambda_max);
		printf("\n***********************************************************************************************************************");
		printf("\n*****************************************    TOTAL.CHAMADAS = %d    **************************************************" , total_chamadas );
		printf("\n***********************************************************************************************************************");
		printf("\n*********    GERAL : n.operadores = %d , tam f.espera = %d    ******    ESPECIFICO : n.operadores = %d    ************", total_operador_geral , capacidade_fespera_geral , total_operador_espcf);
		printf("\n***********************************************************************************************************************\n\n");
		
		lista  *geral = NULL;
		lista  *fespera_geral = NULL;
		lista  *especifico = NULL;
		lista  *fespera_especifico = NULL;


		//PRINTS INICIAIS NOS FICHEIROS
		exportacao_aux(f1raw, 0 , sensib_lambda_aux , 0);
		exportacao_aux(f2raw, 0 , sensib_lambda_aux , 0);
		exportacao_aux(f1 , 0 , sensib_lambda_aux , 0);
		exportacao_aux(f2 , 0 , sensib_lambda_aux , 0);
		exportacao_aux(f3 , 0 , sensib_lambda_aux , 0);

		double delta = (double) 0.2 * (1/((double)sensib_lambda_aux/3600)) ;// delta = 9 para lambda = 80 c/h
		int *hist_atraso = (int *)malloc(hist_tam*sizeof(int));
		int *hist_prev_pos = (int *)malloc(hist_tam*sizeof(int));
		int *hist_prev_neg = (int *)malloc(hist_tam*sizeof(int)); 

		srand(time (NULL) );

		//VAR GERAIS
		int operador_geral_ocupado = 0, operador_espcf_ocupado =0 , area_aux = 0;
		int chamadas_geral_atrasadas = 0 , chamadas_espcf = 0, chamadas_espcf_atrasadas = 0 , chamadas_perdidas = 0;;
		int tamanho_fespera_geral = 0;
		double d_aux = 0;

		//VAR ESTATISTICA
		double erro_rel_sum = 0, erro_abs_sum = 0 , erro_abs = 0, erro_rel = 0;
		double atraso_aux = 0 , total_atraso_geral = 0 , atraso_espcf_aux = 0 , total_atraso_espcf = 0 ;
		double avg_run = 0 , tempo_medio_de_cham_espcf_em_filas = 0 , erro_abs_medio = 0;
		int erro_cont = 0;
		
		//VETOR PARA CALC DO TEMPO MEDIO EM FILAS DE CHAMADAS ESPECIFICAS
		Vetor Vetor_atrasos;
		ini_vetor(&Vetor_atrasos,10);


		int chamadas_cont = 0;

		while(chamadas_cont < total_chamadas) {

			if( chamadas_cont == 0 ) geral = adicionar(geral, CHEGADA, ESPECIFICO, 0, 0);

			// CHEGADA
			if( geral->tipo == CHEGADA ){

				//GERA A PROXIMA CHAMADA
				chamadas_cont++;
				area_aux = area();
				d_aux = tempo_entre_chamadas((float)sensib_lambda_aux/3600); // entrega lambda 
				geral = adicionar(geral, CHEGADA, area_aux, 0, (geral->tempo + d_aux));

				//PROCESSA A CHAMADA QUE CHEGOU CASO HAJA OPERADORES DISPONIVEIS
				if(operador_geral_ocupado < total_operador_geral) {

					operador_geral_ocupado++;
					d_aux = duracao_chamada_geral(geral->area);
					geral = adicionar(geral, PARTIDA, geral->area, 0, (geral->tempo + d_aux));

				//SENAO COLACA-OS NA FILA DE ESPERA CASO HAJA DISPONIBLIDADE NA FILA DE ESPERA
				} else if(tamanho_fespera_geral < capacidade_fespera_geral) { 

					fespera_geral = adicionar(fespera_geral, CHEGADA, geral->area, 0, geral->tempo);
					tamanho_fespera_geral++;
					chamadas_geral_atrasadas++;
					//printf("Running average of sample %d_aux\t %lf \n", chamadas_cont, avg_run);

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

					erro_abs = atraso_aux-avg_run; //fabs
					erro_cont++;
					erro_rel_sum += erro_rel;
					erro_abs_sum += erro_abs;

					//ADICONA ATRASO NO HISTOGRAMA E FICHEIRO RAW 
					exportacao_aux(f1raw, atraso_aux , 0 , 2);
					hist_atraso = histograma_insere(atraso_aux, hist_tam , hist_atraso, delta);

					//ADICONA ERRO DE PREVISAO DA CHAMADA ATUAL COM O RUNNING AVG DA CHAMADA ANTERIOR NO HISTOGRAMA E FICHEIRO RAW 
					exportacao_aux(f2raw, erro_abs , 0 , 2); 
					if( erro_abs < 0)
						hist_prev_neg = histograma_insere(fabs(erro_abs), hist_tam, hist_prev_neg, delta);
					else
						hist_prev_pos = histograma_insere(erro_abs, hist_tam, hist_prev_pos, delta);


					//RUNNING AVG ATUAL 
					avg_run = running_average(erro_cont, atraso_aux, avg_run); /********************/

					// GERA O FIM DA CHAMADA QUE ESTA NA FILA DE ESPERA GERAL E REMOVE-A
					d_aux = duracao_chamada_geral(fespera_geral->area);
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
								d_aux = duracao_chamada_espcf();
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
								d_aux = duracao_chamada_espcf();
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
		
	
		//EXPORTACAO
		exportacao_hist(f1 , hist_atraso , hist_tam , delta,1);
		exportacao_hist(f2 , hist_prev_pos , hist_tam , delta,1);
		exportacao_hist(f3 , hist_prev_neg , hist_tam , delta,-1);
		

		//DEBBUG
		printf("erro_cont : %d || chamadas geral atrasadas : % d  || hist_conta_atrasos : %d \n" , erro_cont , chamadas_geral_atrasadas , histograma_conta(hist_atraso) );
		printf("hist_conta_prev_pos : %d || hist_conta_prev_neg : %d\n", histograma_conta(hist_prev_pos) , histograma_conta(hist_prev_neg) );
		printf("vetor.tamanho : %ld  |  n chamadas espcf : %d\n\n" , Vetor_atrasos.tamanho , chamadas_espcf);
		
		/*******************************************************************************************************************/
    	/***********************************************    print resultados   *********************************************/
    	/*******************************************************************************************************************/

		printf("\t [a1]\t Probablidade de uma chamada geral ser perdida :\t %lf%%\n",(double)chamadas_perdidas/total_chamadas *100);
		printf("\t [a1]\t Probablidade de uma chamada geral sofrer atraso :\t %lf%%\n",(double)chamadas_geral_atrasadas/total_chamadas *100);
		printf("\t[extra]\t Probablidade de uma chamada especifica sofrer atraso :\t %lf%%\n\n",(double)chamadas_espcf_atrasadas/chamadas_espcf *100);
		printf("\t [a2]\t Atraso médio das chamadas geral atrasadas :\t %lf segundos\n",(double)total_atraso_geral/chamadas_geral_atrasadas); // dar print ao avg_run
		printf("\t[extra]\t Atraso médio das chamadas especifica atrasadas :\t %lf segundos \n\n", (double)total_atraso_espcf/chamadas_espcf_atrasadas);

		erro_abs_medio =  (double)erro_abs_sum/erro_cont;
		erro_rel = fabs(erro_abs / ( (double)total_atraso_geral/chamadas_geral_atrasadas ) );
		printf("\t [a3]\t Média erro absoluto de previsao:\t %lf\n", erro_abs_medio);
		printf("\t [a3]\t Média erro relativo de previsao:\t %lf%%\n\n", erro_rel *100);

		tempo_medio_de_cham_espcf_em_filas = calc_media(Vetor_atrasos.vetor, Vetor_atrasos.tamanho);
		printf("\t [b]\t Tempo médio entre chegada a geral até ser atendida especifico  :\t %lf segundos \n", tempo_medio_de_cham_espcf_em_filas);

		printf("\n------------------------------------------------------------------------------------------------------------------------------\n\n");

		double desv_padrao = calc_desvio_standard(Vetor_atrasos.vetor , Vetor_atrasos.tamanho, tempo_medio_de_cham_espcf_em_filas);
		double erro_padrao_media = desv_padrao/(sqrt(Vetor_atrasos.tamanho));
		double limite_confi = 1.65;
		double interv_confi = limite_confi*erro_padrao_media;
		
		printf("\t\tAnálise de sensibilidade para %.0d chamadas/hora = %.4f chamadas/seg\n\n", sensib_lambda_aux , (float)sensib_lambda_aux/3600);
    	printf("\t\t[b]Atraso total das chamadas especificas(90%%): %.3lf +- %.3lf seg\n\n\n", tempo_medio_de_cham_espcf_em_filas , interv_confi);
		//printf("\n\t\tConfidance Interval, with limite of 90%% %lf\n\n", interv_confi);

		/*******************************************************************************************************************/		
		
		//SENSIBILIDADE
		exportacao_aux(sens , (double)tempo_medio_de_cham_espcf_em_filas , sensib_lambda_aux ,3);

		//LIBERTAÇAO DE MEMORIA
		rem_vetor(&Vetor_atrasos);
		free(fespera_geral);
		free(geral);
		free(fespera_especifico);
		free(especifico);
		free(hist_atraso);
		free(hist_prev_neg);
		free(hist_prev_pos);
	}
	
	fclose(f1);
	fclose(f2);
	fclose(f3);
	fclose(f1raw);
	fclose(f2raw);
	fclose(sens);
	
    return 1;
}