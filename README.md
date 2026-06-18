# Nucleo di calcolo per il prodotto tra una matrice densa ed un multivettore
Il progetto verte sulla realizzazione di un nucleo di calcolo per il prodotto tra una matrice sparsa ed un multivettore, che sia in grado di calcolare Y ← AX 
dove A è una matrice densa e X ed Y sono dei multivettori.
La parallelizzazione è gestita ibridamente tramite MPI per la gestione dei processi e CUDA per il calcolo locale tramite GPU

## Dettagli del progetto
- La matrice A è distribuita su una griglia di processi bidimensionale
- 4 varianti di kernel CUDA implementate
- tutta la configurazione dei parametri di input (numero di processi, numero di threads...) organizzata tramite script

## Esecuzione
Dalla cartella del progetto si esegue il test tramite il seguente comando:
```bash
./main.sh
```
Lo script esegue automaticamente tutte le possibili configurazioni inserite nel progetto per realizzare i vari test 
