#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include "lista.h"

#define LAMBDA 5
#define DELTA 0.04
#define TOTAL_EVENTS 100000
#define RAND_MAX 2147483647
#define HIST_SIZE 25
//delta =(1/5)*(1/lambda)

#define ARRIVAL 1
#define DEPARTURE 0

int print_csv( char * csv_file , int * histogram){
  FILE *file_out;
  file_out = fopen(csv_file, "w+");

  if(file_out == NULL){
      perror("fopen");
      return -1;
  }
  printf("\nwrite of data to %s started \n" , csv_file);
  fprintf(file_out, "position, Time, arrival Calls of %d calls)\n", TOTAL_EVENTS);

  for (int j = 0; j < HIST_SIZE; j++){
    fprintf(file_out, "%d, %lf, %d\n", j, (2*j+1)/(float)(HIST_SIZE*2), histogram[j]);
  }

  fclose(file_out);
  return 0;
}

float rand01(){ return (double)rand() / (double)((unsigned)RAND_MAX + 1); }

int main(int argc, char* argv[]){

  if(argc < 2)  {
    perror("need file name to save data");
    return -1;
  }

  //check if the csv file exists
  char filename1[100];
  snprintf(filename1, sizeof filename1 , "%s%s", argv[1],".csv");
  if ( access( filename1 , F_OK ) != 0 ) {
    printf("%s does not exist" , argv[1] );
    return -1;
  }

  int n_events = 0;
  float c = 0 , curr_time = 0 , prev_time = 0;
  float prob = LAMBDA*1e-3;
  int *histogram = (int*)malloc(HIST_SIZE*sizeof(int));
  lista *event = NULL;

  srand(time(NULL));

  for (int j = 0; n_events < TOTAL_EVENTS; j++) {
    if( rand01() < prob) {
      if(event != NULL) event = rem(event);

      curr_time = j * 1e-3 ;
      event = adicionar(event, ARRIVAL, curr_time - prev_time );
      c = (curr_time - prev_time);
      
      for(int i = 0; i < HIST_SIZE; i++) {
        if((c > i*DELTA) && (c <= (i+1)*DELTA)) {
          histogram[i]++;
          break;
        }
      }
      prev_time = curr_time;
      n_events++;
    }
  }

  printf("curr_time : %f\n", curr_time  );
  printf("média do intervalo entre chamadas consecutivas = %lf\n", (curr_time / n_events) );

  if(print_csv(filename1, histogram) != 0 ){
      perror("hist print error");
      return -1;
  }

  puts("\nwrite of data complete");
  return 0;
}
