# Nucleo di calcolo per il prodotto tra una matrice densa ed un multivettore
Il progetto verte sulla realizzazione di un nucleo di calcolo per il prodotto tra una matrice sparsa ed un multivettore, che sia in grado di calcolare Y ← AX 
dove A è una matrice densa e X ed Y sono dei multivettori. Un multivettore e una matrice densa di dimensione n×k, con k piccolo.
La parallelizzazione è gestita ibridamente tramite MPI per la gestione dei processi e CUDA per il calcolo locale tramite GPU
