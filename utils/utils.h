#ifndef UTILS_H
#define UTILS_H

//OPERATIONS
void genera_matrice(float* M, int x, int y);
void prodotto_seriale(float* A, float* X, int m, int n, int k, float* Y);
void matmul(float* A, float* X, float* Y, int M, int N, int K);
double frobenius_error(float* Y, float* Y_serial, int m, int k);

//CALCULATE
void save_results_to_csv(const char *filename, int M, int N, int K, int kernel, int threads,int p_r, int p_c, double serial_t, double avg_time, double flops, double speedup, double error);
double calculate_performance(float* A, float* X, int m, int n, int k, float* Y);

#endif
