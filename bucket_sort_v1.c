#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <mpi.h>



// Comparison function used by qsort
int compare_dbls(const void* arg1, const void* arg2)
{
    double a1 = *(double *) arg1;
    double a2 = *(double *) arg2;
    if (a1 < a2) return -1;
    else if (a1 == a2) return 0;
    else return 1;
}


// verification function
int verify(double *array, int array_len)
{
   int i;
   for (i=0 ; i < array_len-1 ; i++)
   {
      if (array[i] > array[i+1])
         return -1;
   }
   return 1;
}



// Sort the array in place
void qsort_dbls(double *array, int array_len)
{
    qsort(array, (size_t)array_len, sizeof(double),
          compare_dbls);
}



int main( int argc, char *argv[] )
{
   
   MPI_Init(&argc, &argv);

   int n, i;
   double *input_array;   //the input array
   double *bucketlist;    //this array will contain input elements in order of the processors
                          //e.g elements of process 0 will be stored first, then elements of process 1, and so on
   int *scounts;          //This array will contain the counts of elements each processor will receive
   int *dspls;            //The rel. offsets in bucketlist where the elements of different processes will be stored
   double *local_array;   //This array will contain the elements in each process
   int *bin_elements;     //it will keep track of how many elements have been included in the pth bin
   double *sorted_array;  //final sorted array

   double tstart, t1, t2, t3, t4, t5, t6, tend;

   int p, rank;
   MPI_Comm_size(MPI_COMM_WORLD, &p);
   MPI_Comm_rank(MPI_COMM_WORLD, &rank);

   scounts = malloc(p*sizeof(int));
   dspls = malloc(p*sizeof(int));


   tstart = MPI_Wtime();
   
   if (rank == 0)
   {

      if (argc == 1)
      {
         fprintf(stderr, "ERROR: Please specify the number of elements.\n");
         exit(1);
      }
    
      //n = strtoimax(argv[1], NULL, 10);
      n = atoi(argv[1]);
      
      input_array = malloc(n*sizeof(double));
      bucketlist = malloc(n*sizeof(double));

      bin_elements = malloc(p*sizeof(int));


      t1 = MPI_Wtime();
      for(i = 0 ; i < n ; i++)
      {
        input_array[i] = ((double) rand()/RAND_MAX);
      }
      t2 = MPI_Wtime();
       
      for(i = 0 ; i < p ; i++)
      {
         scounts[i] = 0 ;
      }
    
      //counting the elements in each processor
      for(i = 0 ; i < n ; i++)
      {
         scounts[(int)(input_array[i]/(1.0/p))]++;
      }

      for(i = 0 ; i<p ; i++)
      {
         bin_elements[i] = scounts[i];
      }
    
      dspls[0] = 0;
      for(i = 0 ; i< p-1 ;i++)
      {
         dspls[i+1] = dspls[i] + scounts[i];
      }
        
      int bin;
      int pos;
      for(i = 0 ; i < n ; i++)
      {
         bin = (int)(input_array[i]/(1.0/p));
         pos = dspls[bin] + scounts[bin] - bin_elements[bin];
         bucketlist[pos] = input_array[i];
         bin_elements[bin]--;
      }
      t3 = MPI_Wtime();
   
   }

 
   t4 = MPI_Wtime();  
   MPI_Bcast(scounts, p, MPI_INT, 0, MPI_COMM_WORLD); 
   MPI_Bcast(dspls, p, MPI_INT, 0, MPI_COMM_WORLD); 
   MPI_Bcast(&n, 1, MPI_INT, 0, MPI_COMM_WORLD);

   local_array = malloc(scounts[rank]*sizeof(double));
   sorted_array = malloc(n*sizeof(double));

   MPI_Scatterv(bucketlist, scounts, dspls, MPI_DOUBLE,
                local_array, scounts[rank], MPI_DOUBLE, 0, MPI_COMM_WORLD);
   t5 = MPI_Wtime();

   /*printf("rank:%d\n",rank);
   for (i=0; i<scounts[rank]; i++)
      printf("%f\n", local_array[i]);*/
   qsort_dbls(local_array, scounts[rank]);
   t6 = MPI_Wtime();

   MPI_Gatherv(local_array, scounts[rank], MPI_DOUBLE, 
               sorted_array, scounts, dspls, MPI_DOUBLE,0, MPI_COMM_WORLD); 
   tend = MPI_Wtime();

   
   if (rank == 0)
   {
      /*printf("rank:%d\n",rank);
      for (i=0; i<n; i++)
	printf("%f\n", sorted_array[i]);*/

      if (verify(sorted_array, n))
         printf("Successful sort\n");
      else
        printf("Unsuccessful sort\n");

      printf("data generation took %.2f\n",t2-t1);
      printf("binning took %.2f\n",t3-t2);
      printf("distribution took %.2f\n",t5-t4);
      printf("local sort took %.2f\n",t6-t5);
      printf("data gathering took %.2f\n",tend-t6);
      printf("total time: %.2f\n",tend-tstart);

   }

  
   MPI_Finalize();
   return 0;
   
}
