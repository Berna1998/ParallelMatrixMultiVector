#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "utils.h" 

void genera_matrice(float* M, int x, int y){
  int i,j;
  for(i=0;i<x;i++){
    for(j=0;j<y;j++){
       //M[i][j] = rand() % 100;
       M[i*y + j] = (float)rand() / (float)RAND_MAX;
    }
  }
  
}


void prodotto_seriale(float* A, float* X, int m, int n, int k, float* Y){
    int i,j,p;
    
    /*
    int** Y = malloc(sizeof(int*)*n);
    for(i=0;i<n;i++){
      Y[i] = malloc(sizeof(int*)*k[z]);
    }*/
    //messa la calloc così che Y sia inizializzato con tutti valori pari a 0

    for(i=0;i<m;i++){
      for(p=0;p<n;p++){
        float a = A[i*n + p];
        for(j=0;j<k;j++){
          Y[i*k + j] += a * X[p*k + j];
        }
      }
    }

}

// Funzione matmul: prodotto matrice-vettore/matrice
void matmul(float* A, float* X, float* Y, int M, int N, int K){
    for(int i=0; i<M; i++){
        for(int j=0; j<K; j++){
            Y[i*K + j] = 0;
            for(int k=0; k<N; k++){
                Y[i*K + j] += A[i*N + k] * X[k*K + j];
            }
        }
    }
}

double frobenius_error(float* Y, float* Y_serial, int m, int k) {
    double num = 0.0;
    double den = 0.0;

    for (int i = 0; i < m * k; i++) {
        double diff = (double)Y[i] - (double)Y_serial[i];
       // printf("SERIALE: %e,  PARALLELO: %e\n", Y_serial[i], Y[i]);

        num += diff * diff;
        den += (double)Y_serial[i] * (double)Y_serial[i];
    }

    if (den < 1e-15)
        return 0.0;

    return sqrt(num) / sqrt(den);
}
