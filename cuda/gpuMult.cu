#include <iostream> 
#include <math.h>
#include <cuda_runtime.h>  

__global__ void firstNoShared(float* A, float* X, float* Y, int m, int n, int k) {
    //Coordinate globali del thread
    int row = blockIdx.y * blockDim.y + threadIdx.y;
    int col = blockIdx.x * blockDim.x + threadIdx.x;

    if (row < m && col < k) {
        float sum = 0.0f;

        for (int j = 0; j < n; j++) {
            sum += A[row * n + j] * X[j * k + col];
        }

        Y[row * k + col] = sum;
    }
}

//Kernel CUDA con shared memory
__global__ void firstShared(float* A, float* X, float* Y, int m, int n, int k, int tileSize){

    extern __shared__ float shared[];

    float* As = shared;
    float* Xs = &shared[tileSize * tileSize];

    int tx = threadIdx.x;
    int ty = threadIdx.y;

    int row = blockIdx.y * blockDim.y + ty;
    int col = blockIdx.x * blockDim.x + tx;

    float sum = 0.0f;

    //Numero di tile
    int numTiles = (n + tileSize - 1) / tileSize;

    for (int t = 0; t < numTiles; t++) {

        //Caricamento tile di A
        int tiledColA = t * tileSize + tx;

        if (row < m && tiledColA < n)
            As[ty * tileSize + tx] = A[row * n + tiledColA];
        else
            As[ty * tileSize + tx] = 0.0f;

        // =========================
        // Caricamento tile di X
        // =========================
        int tiledRowX = t * tileSize + ty;

        if (col < k && tiledRowX < n)
            Xs[ty * tileSize + tx] = X[tiledRowX * k + col];
        else
            Xs[ty * tileSize + tx] = 0.0f;

        __syncthreads();

        // =========================
        // Moltiplicazione tile
        // =========================
        for (int j = 0; j < tileSize; j++) {
            sum += As[ty * tileSize + j] *Xs[j * tileSize + tx];
        }

        __syncthreads();
    }

    // Scrittura risultato
    if (row < m && col < k){
        Y[row * k + col] = sum;
    }
}


//Kernel CUDA: 1 thread = 1 riga di C
__global__ void secondNoShared(float* A, float* X, float* Y, int m, int n, int k) {
    //Ogni thread gestisce una riga specifica di Y
    int row = blockIdx.x * blockDim.x + threadIdx.x;

    if (row < m) {
        //Allocazione dinamica basata sul parametro 'k' effettivo per evitare register spilling
        //Sfruttiamo i registri per le colonne del multivettore (k <= 32)
        float sum[32]; 
        
        #pragma unroll
        for (int j = 0; j < k; j++) {
            sum[j] = 0.0f;
        }

        //Loop sulla dimensione interna (colonne di A / righe di X)
        //Questo ciclo permette l'accesso coalescente ad A da parte del warp
        for (int i = 0; i < n; i++) {
            //COALESCED ACCESS: Thread adiacenti caricano elementi adiacenti sulla stessa colonna 'i'
            float a_val = A[row * n + i];

            //Aggiorna gli accumulatori per tutte le k colonne del multivettore
            #pragma unroll
            for (int j = 0; j < k; j++) {
                sum[j] += a_val * X[i * k + j];
            }
        }

        //Scrittura finale dei risultati in modo lineare
        #pragma unroll
        for (int j = 0; j < k; j++) {
            Y[row * k + j] = sum[j];
        }
    }
}

__global__ void secondShared(float* A, float* X, float* Y, int m, int n, int k, int tileK) {
    // La shared memory conterrà una porzione ("piastrella") del multivettore X
    // Dimensione necessaria: tileK * k elementi float
    extern __shared__ float Xs[];

    int row = blockIdx.x * blockDim.x + threadIdx.x;
    int tx = threadIdx.x;

    // Accumulatori locali nei registri del thread
    float sum[32];
    #pragma unroll
    for (int j = 0; j < k; j++) {
        sum[j] = 0.0f;
    }

    // Iterazione sui blocchi (tile) della dimensione interna N
    int numTiles = (n + tileK - 1) / tileK;
    for (int t = 0; t < numTiles; t++) {

        // 1. CARICAMENTO COALESCENTE IN SHARED MEMORY (Tutti i thread partecipano!)
        // Dobbiamo caricare una matrice di dimensione (tileK x k)
        int totalElementsToLoad = tileK * k;
        int threadsInBlock = blockDim.x;

        // Approccio cooperativo: ogni thread carica uno o più elementi distribuiti linearmente
        for (int i = tx; i < totalElementsToLoad; i += threadsInBlock) {
            int localRowX = i / k; // Riga locale della tile di X (da 0 a tileK-1)
            int localColX = i % k; // Colonna locale di X (da 0 a k-1)

            int globalRowX = t * tileK + localRowX;

            if (globalRowX < n && localColX < k) {
                Xs[localRowX * k + localColX] = X[globalRowX * k + localColX];
            } else {
                Xs[localRowX * k + localColX] = 0.0f;
            }
        }

        // Sincronizzazione fondamentale: assicurarsi che tutta la tile di X sia caricata
        __syncthreads();

        // 2. CALCOLO UTILIZZANDO LA SHARED MEMORY
        if (row < m) {
            for (int i = 0; i < tileK; i++) {
                int k_idx = t * tileK + i;
                if (k_idx < n) {
                    // Accesso coalescente alla memoria globale per la matrice A
                    float a_val = A[row * n + k_idx];

                    #pragma unroll
                    for (int j = 0; j < k; j++) {
                        // Accesso rapido alla Shared Memory senza Bank Conflict distruttivi
                        sum[j] += a_val * Xs[i * k + j];
                    }
                }
            }
        }

        // Sincronizzazione prima della prossima iterazione per evitare sovrascritture della tile
        __syncthreads();
    }

    // 3. SCRITTURA FINALE IN MEMORIA GLOBALE
    if (row < m) {
        #pragma unroll
        for (int j = 0; j < k; j++) {
            Y[row * k + j] = sum[j];
        }
    }
}

extern "C" void run_kernel(float* A, float* X, float* Y, int m, int n, int k, int kernel, int block_th){
     dim3 block(sqrt(block_th), sqrt(block_th));
     dim3 grid((k + block.x - 1) / block.x, (m + block.y - 1) / block.y);
     
     int threadsPerBlockSecond = block_th;
     int numBlocksSecond = (m + threadsPerBlockSecond - 1) / threadsPerBlockSecond;
     
     switch(kernel){
       case 1:
       
          firstNoShared<<<grid, block>>>(A, X, Y, m, n, k);
          break;
       case 2:{
	  size_t sharedMemSize = 2 * block_th * sizeof(float);
          firstShared<<<grid, block, sharedMemSize>>>(A, X, Y, m, n, k, sqrt(block_th));
          break;}
       
        case 3:
            // Chiamata corretta al kernel 1D ottimizzato
            secondNoShared<<<numBlocksSecond, threadsPerBlockSecond>>>(A, X, Y, m, n, k);
            break;
       
        case 4: {
            // Impostiamo la dimensione della piastrella (tileK). Usiamo 32 per un warp intero.
            int tileK = 32;             
            // La dimensione della memoria condivisa deve contenere la tile di X (tileK * k)
            size_t sharedSize = tileK * k * sizeof(float);
            
            secondShared<<<numBlocksSecond, threadsPerBlockSecond, sharedSize>>>(A, X, Y, m, n, k, tileK);
            break;
        }
      
       default:
      
          printf("Kernel non valido! Uso default (naive)\n");
          firstNoShared<<<grid, block>>>(A, X, Y, m, n, k);  
          break;
    }
}
