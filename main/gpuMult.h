#ifndef GPUMULT_H
#define GPUMULT_H

#ifdef __cplusplus
extern "C" {
#endif

void run_kernel(float* A, float* X, float* Y,
               int m, int n, int k, int kernel, int block_th);

#ifdef __cplusplus
}
#endif

#endif
