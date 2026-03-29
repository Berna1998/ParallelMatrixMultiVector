#include <stdio.h>
#include <stdlib.h>

int main(){

   /* int m = 4;
    int n = 3;
    int k = 2;
    int A[4][3] = {{1,2,3},{4,5,6},{7,8,9},{10,11,12}};
    int X[3][2] = {{1,4},{2,5},{3,6}};
    int Y[4][2];
    */
    
    int m,n;
    int k[]={3,6,8,20,32};
    int i,j,p;
    
    int scelta=0;
    printf("Inserisci 1, 2 o 3 per uno di questi casi:\n");
    
    printf("1) M=N\n");
    printf("2) M=3*N\n");
    printf("3) N=2*M\n");
    printf("Inserisci: ");
    scanf("%d",&scelta);
    
    if(scelta == 1){
      printf("Caso 1, M=N. Inserisci M: ");
      scanf("%d", &m);
      n=m;
      
    }else if(scelta == 2){
      printf("Caso 2, M=3*N. Inserisci N: ");
      scanf("%d", &n);    
      m=3*n;  
   
    }else if(scelta == 3){
      printf("Caso 3, N=2*M. Inserisci M: ");
      scanf("%d", &m); 
      n=2*m;    
    } 
    int z=0;
    
    printf("M vale: %d, N vale:%d, K vale: %d\n", m, n, k[z]);
    
    int A[m][n];
    int X[n][k[z]];
    int Y[m][k[z]];
    srand(42);
    for(i=0;i<m;i++){
      for(j=0;j<n;j++){
         A[i][j] = rand() % 100;
      }
    }
    
    for(i=0;i<n;i++){
      for(j=0;j<k[z];j++){
        X[i][j] = rand() % 100;
      } 
    }
    
    printf("LA MATRICE A E':\n");
    for(i=0;i<m;i++){
      for(j=0;j<n;j++){
         printf("%d ",A[i][j]); 
      }
      printf("\n");
    }
    printf("\n");
    printf("LA MATRICE X E':\n");
    for(i=0;i<n;i++){
      for(j=0;j<k[z];j++){
         printf("%d ",X[i][j]); 
      }
      printf("\n");
    }
    
    printf("\n");
    
    for(i=0;i<m;i++){
       for(j=0;j<k[z];j++){
         int sum = 0;
         for(p=0;p<n;p++){
          sum+=A[i][p]*X[p][j];
         }
         Y[i][j] = sum; 
       }
    }
    
    for(i=0;i<m;i++){
      for(j=0;j<k[z];j++){
        printf("%d ",Y[i][j]); 
      }
      printf("\n");
    }
    
    
    
    return 0;	
}
