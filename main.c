#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <string.h> 

#include <cuda_runtime.h>
#include "utils.h"
#include "gpuMult.h"


int main(int argc, char *argv[]){

    int m,n;
    int k;
    int i,j,p;
    float* A = NULL;
    float* X = NULL;
    float* Y_serial = NULL;
    int kernel;
    int p_r, p_c;
    int values_block[] = {64, 256, 576, 1024};

    double serial_time;
    
    int z=0;
    m = atoi(argv[1]);
    n = atoi(argv[2]);
    k = atoi(argv[3]);

    p_r = atoi(argv[4]);
    p_c = atoi(argv[5]);
    kernel = atoi(argv[6]);

    MPI_Init(&argc,&argv);
    
    
    int size;  //numero processi specificati in input
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    int rank;  //numero specifico del processo
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if (size != p_r * p_c) {
       if (rank == 0){
           printf("Errore: numero processi != P_r * P_c\n");}
       MPI_Abort(MPI_COMM_WORLD, 1);
    }


    int dims[2] = {p_r, p_c};
 
    int periods[2] = {0, 0};
    int reorder = 0;          // MPI può riordinare i rank per ottimizzare
    MPI_Comm cart_comm;       // nuovo communicator cartesiano
    
    MPI_Cart_create(MPI_COMM_WORLD, 2, dims, periods, reorder, &cart_comm);
    
    int coords[2];
    int cart_rank;
    MPI_Comm_rank(cart_comm, &cart_rank);

    MPI_Cart_coords(cart_comm, cart_rank, 2, coords); //è l'equivalente di MPI_Comm_rank per le coordinate cartesiane
    
    
   
    if(rank == 0){
 
        int z=0;

        Y_serial = calloc(m*k, sizeof(float));

        A = malloc(m*n*sizeof(float));
        X = malloc(n*k*sizeof(float));
    
    
        srand(42);
    
        genera_matrice(A, m, n);
        genera_matrice(X, n, k);

        serial_time = calculate_performance(A, X, m, n, k, Y_serial);

    
/* dimensioni locali */
    int M_local = m / dims[0];
    int N_local = n / dims[1];

    int row_start = coords[0] * (m / dims[0]);
    int col_start = coords[1] * (n / dims[1]);

/* aggiustamento resto */
    if(coords[0] == dims[0]-1){
       M_local += m % dims[0];
    }
    if(coords[1] == dims[1]-1){
       N_local += n % dims[1];
    }
  //  printf("Rank %d (coords [%d,%d]) ha blocco locale %d x %d da [%d,%d]\n", rank, coords[0], coords[1], M_local, N_local, row_start, col_start);

/* ============================= */
/* NUOVA PARTE CORRETTA 2D       */
/* ============================= */

/* comunicatori di riga e colonna */
    MPI_Comm row_comm, col_comm;

    MPI_Comm_split(cart_comm, coords[0], coords[1], &row_comm);
    MPI_Comm_split(cart_comm, coords[1], coords[0], &col_comm);

/* allocazioni corrette */
    float* A_local = malloc(M_local * N_local * sizeof(float));
    float* X_local = malloc(N_local * k * sizeof(float));
    float* Y_local = calloc(M_local * k, sizeof(float));
    
    
    float *d_A, *d_X, *d_Y;

    size_t sizeA = M_local * N_local * sizeof(float);
    size_t sizeX = N_local * k * sizeof(float);
    size_t sizeY = M_local * k * sizeof(float);

    cudaMalloc((void**)&d_A, sizeA);
    cudaMalloc((void**)&d_X, sizeX);
    cudaMalloc((void**)&d_Y, sizeY);

/* ============================= */
/* DISTRIBUZIONE MATRICE A (2D)  */
/* ============================= */

    if(rank == 0){

       for(int p = 0; p < size; p++){

         int c[2];
         MPI_Cart_coords(cart_comm, p, 2, c);

         int rows = m / dims[0];
         int cols = n / dims[1];
 
         if(c[0] == dims[0]-1){
            rows += m % dims[0];
         }
         if(c[1] == dims[1]-1){
            cols += n % dims[1];
         }

         int r_start = c[0] * (m / dims[0]);
         int c_start = c[1] * (n / dims[1]);

         if(p == 0){

            for(int i = 0; i < rows; i++){
                memcpy(&A_local[i*N_local], &A[(r_start+i)*n + c_start], cols*sizeof(float));
            }
         }
         else{

            float* buffer = malloc(rows*cols*sizeof(float));

            for(int i = 0; i < rows; i++){
                memcpy(&buffer[i*cols], &A[(r_start+i)*n + c_start], cols*sizeof(float));
            }
            MPI_Send(buffer, rows*cols, MPI_FLOAT, p, 0, MPI_COMM_WORLD);

            free(buffer);
         }
       }
    }else{

       MPI_Recv(A_local, M_local*N_local, MPI_FLOAT, 0, 0, MPI_COMM_WORLD,MPI_STATUS_IGNORE);
    }

/* ============================= */
/* DISTRIBUZIONE MATRICE X       */
/* Scatterv sulla prima riga     */
/* poi broadcast nella colonna   */
/* ============================= */

/* communicator della prima riga */
    MPI_Comm first_row_comm;

    if(coords[0] == 0){
       MPI_Comm_split(cart_comm, 0, coords[1], &first_row_comm);
    }else{
       MPI_Comm_split(cart_comm, MPI_UNDEFINED, coords[1], &first_row_comm);
    }

/* solo i processi della prima riga partecipano */
    if(coords[0] == 0){

      int *sendcounts = NULL;
      int *displs = NULL;

    /* rank locale nel communicator della prima riga */
      int row_rank;
      MPI_Comm_rank(first_row_comm, &row_rank);

      if(row_rank == 0){

        sendcounts = malloc(p_c * sizeof(int));
        displs     = malloc(p_c * sizeof(int));

        int offset = 0;

        for(int c = 0; c < p_c; c++){

            int cols = n / dims[1];

            if(c == p_c - 1)
                cols += n % dims[1];

            /* numero di float da mandare */
            sendcounts[c] = cols * k;

            /* displacement in float */
            displs[c] = offset;

            offset += cols * k;
        }
     }

    /* Scatterv dei blocchi di X */
      MPI_Scatterv(
        X,                 /* send buffer */
        sendcounts,
        displs,
        MPI_FLOAT,
        X_local,           /* recv buffer */
        N_local * k,
        MPI_FLOAT,
        0, first_row_comm);

      if(row_rank == 0){
        free(sendcounts);
        free(displs);
      }
    }

/* broadcast lungo le colonne */
    MPI_Bcast(X_local, N_local * k, MPI_FLOAT, 0, col_comm );
          
    //VEDI FINO A QUA--------------------------------------------------
    
    // Calcolo locale del prodotto
    double total[] = {0.0, 0.0, 0.0, 0.0};
    int block;
        
    /* COPY H2D */
    cudaMemcpy(d_A, A_local, sizeA, cudaMemcpyHostToDevice);
    cudaMemcpy(d_X, X_local, sizeX, cudaMemcpyHostToDevice);

    for(j=0;j<4;j++){
      block = values_block[j];
      for(i = 0; i<6; i++){
      //PARTE BARRIER MATMUL
         MPI_Barrier(cart_comm);
         double start = MPI_Wtime();

         /* EVENT TIMING CUDA 
          cudaEvent_t s, e;
         cudaEventCreate(&s);
         cudaEventCreate(&e);

         cudaEventRecord(s);*/
         //CHIAMO GPU
         run_kernel(d_A, d_X, d_Y, M_local, N_local, k, kernel, block);
         
         cudaDeviceSynchronize();
         MPI_Barrier(cart_comm);
         double local = MPI_Wtime() - start;

       //  cudaEventRecord(e);
       //  cudaEventSynchronize(e);

       //  float gpu_ms = 0;
      //   cudaEventElapsedTime(&gpu_ms, s, e);
      
         double global_max;
         MPI_Reduce(&local, &global_max, 1, MPI_DOUBLE, MPI_MAX, 0, cart_comm);

         if (rank == 0 && i > 0){
           total[j] += global_max;}
      }
    }
    
      /* COPY D2H */
    cudaMemcpy(Y_local, d_Y, sizeY, cudaMemcpyDeviceToHost);
    
/* ============================= */
/* RIDUZIONE LUNGO LE RIGHE      */
/* ============================= */

    float* Y_row = NULL;

    if(coords[1] == 0){
       Y_row = malloc(M_local * k* sizeof(float));
    }
    MPI_Reduce(Y_local, Y_row, M_local * k,MPI_FLOAT, MPI_SUM,0,row_comm);

/* ============================= */
/* RACCOLTA FINALE CON GATHERV   */
/* ============================= */

    float* Y = NULL;

/* communicator della prima colonna */
    MPI_Comm first_col_comm;

    if(coords[1] == 0){
      MPI_Comm_split(cart_comm, 0, coords[0], &first_col_comm);
    }else{
      MPI_Comm_split(cart_comm, MPI_UNDEFINED, coords[0], &first_col_comm);
    }

/* solo i processi della prima colonna partecipano */
    if(coords[1] == 0){

      int *recvcounts = NULL;
      int *displs = NULL;

      int col_rank;
      MPI_Comm_rank(first_col_comm, &col_rank);

    /* root della prima colonna */
      if(col_rank == 0){

        Y = malloc(m * k * sizeof(float));

        recvcounts = malloc(p_r * sizeof(int));
        displs     = malloc(p_r * sizeof(int));

        int offset = 0;

        for(int r = 0; r < p_r; r++){

            int rows = m / dims[0];

            if(r == p_r - 1){
                rows += m % dims[0];
            }

            recvcounts[r] = rows * k;
            displs[r]     = offset;

            offset += rows * k;
        }
      }

      MPI_Gatherv(
        Y_row,                 /* send buffer */
        M_local * k,           /* send count */
        MPI_FLOAT,
        Y,                     /* recv buffer */
        recvcounts,
        displs,
        MPI_FLOAT,
        0,                     /* root nel communicator */
        first_col_comm);

      if(col_rank == 0){
        free(recvcounts);
        free(displs);
      }
    }
    
    if (rank == 0){
       for(j=0; j<4; j++){
      //   printf("NUMERO DI THREAD %d\n", values_block[j]);
         double avg_time = total[j] / 5;
  
         double flops;
         if (avg_time > 1e-12) {
            flops = (2.0 * m * n * k) / avg_time;
         } else {
            flops = 0.0;
         }
       
         double speedup = serial_time/avg_time;
    
      //   printf("\nTempo medio: %f s\n", avg_time);
     //    printf("FLOPS: %e\n", flops);
     //    printf("SPEEDUP: %e\n", speedup);
       
         double efficency = speedup/size;
       
         double error = frobenius_error(Y, Y_serial, m, k);

       //  printf("ERRORE FROBENIUS: %e\n", error);
         
         char filename[50];
         if(m==n){
            snprintf(filename, sizeof(filename), "results_M=N.csv");
         }else if (m==3*n){
            snprintf(filename, sizeof(filename), "results_M=3N.csv");
         }else{
            snprintf(filename, sizeof(filename), "results_N=2M.csv");
         }
         save_results_to_csv(filename, m, n, k, kernel, values_block[j], p_r, p_c, serial_time, avg_time, flops, speedup, error);
       }
    }

/* ============================= */
/* STAMPA RISULTATO              */
/* ============================= */
/*
    if(rank == 0){

       printf("\nRISULTATO Y = A * X:\n");

       for(int i = 0; i < m; i++){
         for(int j = 0; j < k; j++){
            printf("%d ", Y[i*k + j]);
         }
         printf("\n");
       }
    }
*/
/* ============================= */
/* PULIZIA MEMORIA               */
/* ============================= */

    free(A_local);
    free(X_local);
    free(Y_local);

    if(coords[1] == 0){
      free(Y_row);
    }
    if(rank == 0){
      free(A);
      free(X);
      free(Y);
      free(Y_serial);
    }
    
    
    cudaFree(d_A);
    cudaFree(d_X);
    cudaFree(d_Y);

   // cudaEventDestroy(s);
  //  cudaEventDestroy(e);

    MPI_Finalize();
    return 0;
}
