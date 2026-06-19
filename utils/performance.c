#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>   
#include <omp.h> 

#include "utils.h"  

int compare(const void *a, const void *b) {
    double da = *(const double*)a;
    double db = *(const double*)b;

    if (da < db) return -1;
    if (da > db) return 1;
    return 0;
}



void save_results_to_csv(const char *filename, int M, int N, int K, int kernel, int threads,int p_r, int p_c, double serial_t, double avg_time, double flops, double speedup, double error) {

    FILE *file = fopen(filename, "a");
    if (file == NULL) {
        perror("Errore apertura CSV");
        return;
    }

    //Header se file vuoto
    fseek(file, 0, SEEK_END);
    if (ftell(file) == 0) {
        fprintf(file, "M,N,K,Kernel,Threads,PR,PC,SerialTime(s),AvgTime(s),FLOPS,Speedup,Error\n");
    }

    fprintf(file, "%d,%d,%d,%d,%d,%d,%d,%.3f,%.3f,%e,%e,%e\n",
            M, N, K, kernel,
            threads, p_r, p_c,
            serial_t, avg_time, 
            flops, speedup, error);

    fclose(file);
}




double calculate_performance(float* A, float* X, int m, int n, int k, float* Y) {

    double start_time, end_time, diff_time;
    double total_time = 0.0;

    const int REPS = 5;

               
    float* tmp = calloc(m*k, sizeof(float));
    //Warm-up 
    prodotto_seriale(A, X, m, n, k, tmp);
    free(tmp);

    double times[REPS];
    
    for (int i = 0; i < REPS; i++) {
	memset(Y, 0, m*k*sizeof(float));
        start_time = omp_get_wtime();
        prodotto_seriale(A, X, m, n, k, Y);
        end_time = omp_get_wtime();
        
        diff_time = end_time - start_time;
        times[i] = diff_time;
        total_time += diff_time;
       
    }
    
    qsort(times, REPS, sizeof(double), compare);

    double avg_time = total_time / REPS;

    return avg_time;
}
