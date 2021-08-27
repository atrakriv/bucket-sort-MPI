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
   double *local_input_array;   //The input array for each processor
   double *local_bucketlist;    //This array will contain input elements in order of the processors for each processor
                                //E.g elements of process 0 will be stored first, then elements of process 1, and so on
   int *local_sscounts;         //This array will contain the counts of elements each processor will send to others
   int *local_rscounts;         //This array will contain the counts of elements each processor will receive from others
   int *local_sdspls;           //The offsets in bucketlist where the elements of different processes will be stored - send side
   int *local_rdspls;           //The offsets in local_array where the elements of different processes will be stored - receive side
   double *local_array;         //This array will contain the corrsponding elements in each process
   int *local_bin_elements;     //It will keep track of how many elements have been included in the pth bin
   int *local_array_sizes;      //number of ellements each process has to sort
   int *fdspls;                 //final offset in sorted_array
   double *sorted_array;        //final sorted array



   double tstart, t1, t2, t3, t4, t5, tend;
   double t_gen, t_bin, t_dist, t_srt, t_gath, t_total;

   int p, rank;
   MPI_Comm_size(MPI_COMM_WORLD, &p);
   MPI_Comm_rank(MPI_COMM_WORLD, &rank);

   local_sscounts = malloc(p*sizeof(int));
   local_rscounts = malloc(p*sizeof(int));
   local_sdspls = malloc(p*sizeof(int));
   local_rdspls = malloc(p*sizeof(int));
   fdspls = malloc(p*sizeof(int));

   tstart = MPI_Wtime();
   
   srand(time(NULL) + rank);
   
   if (rank == 0)
   {

      if (argc == 1)
      {
         fprintf(stderr, "ERROR: Please specify the number of elements.\n");
         exit(1);
      }
      n = atoi(argv[1]);
   }

   MPI_Bcast(&n, 1, MPI_INT, 0, MPI_COMM_WORLD);
      
   local_input_array = malloc(n/p*sizeof(double));
   local_bucketlist = malloc(n/p*sizeof(double));

   local_bin_elements = malloc(p*sizeof(int));
   local_array_sizes = malloc(p*sizeof(int));

   
   t1 = MPI_Wtime();
   for(i = 0 ; i < n/p ; i++)
   {
     local_input_array[i] = ((double) rand()/RAND_MAX);
   }
   t2 = MPI_Wtime();
       
   for(i = 0 ; i < p ; i++)
   {
      local_sscounts[i] = 0 ;
   }
   
   //counting the elements in each processor
   for(i = 0 ; i < n/p ; i++)
   {
      local_sscounts[(int)(local_input_array[i]/(1.0/p))]++;
   }
   
   for(i = 0 ; i<p ; i++)
   {
      local_bin_elements[i] = local_sscounts[i];
   }
    
   local_sdspls[0] = 0;
   for(i = 0 ; i< p-1 ;i++)
   {
      local_sdspls[i+1] = local_sdspls[i] + local_sscounts[i];
   }
        
   int bin;
   int pos;
   for(i = 0 ; i < n/p ; i++)
   {
      bin = (int)(local_input_array[i]/(1.0/p));
      pos = local_sdspls[bin] + local_sscounts[bin] - local_bin_elements[bin];
      local_bucketlist[pos] = local_input_array[i];
      local_bin_elements[bin]--;
   }
   t3 = MPI_Wtime();


   MPI_Allreduce(local_sscounts, local_array_sizes, p, MPI_INT, MPI_SUM, MPI_COMM_WORLD);
   local_array = malloc(local_array_sizes[rank]*sizeof(double));

   MPI_Alltoall(local_sscounts,1, MPI_INT,local_rscounts,1, MPI_INT,MPI_COMM_WORLD);
   
   local_rdspls[0] = 0;
   for(i = 0 ; i< p-1 ;i++)
   {
      local_rdspls[i+1] = local_rdspls[i] + local_rscounts[i];
   }
   
   /*printf("rank:%d\n",rank);
   for (i=0; i < p; i++)
      printf("s%d\n", local_sscounts[i]);

   printf("rank:%d\n",rank);
   for (i=0; i <n/p; i++)
      printf("i%f\n", local_bucketlist[i]);*/

   

   MPI_Alltoallv(local_bucketlist, local_sscounts, local_sdspls, MPI_DOUBLE, local_array,local_rscounts, local_rdspls, MPI_DOUBLE, MPI_COMM_WORLD);

   
  /*int l_scounts [4] = {1,2,1,0};
  int l_dspls [4] = {0,0,0,0};
  MPI_Scatterv(local_bucketlist, local_scounts, local_dspls, MPI_DOUBLE,
                     local_array, local_scounts[rank], MPI_DOUBLE, 0, MPI_COMM_WORLD);*/

  /*MPI_Scatterv(local_bucketlist, local_scounts, local_dspls, MPI_DOUBLE,
                     local_array, local_scounts[rank], MPI_DOUBLE, 0, MPI_COMM_WORLD);*/

  /*printf("rank:%d\n",rank);
  for (i=0; i <local_array_sizes[rank]; i++)
      printf("o%f\n", local_array[i]);*/

 
   t4 = MPI_Wtime();  
   qsort_dbls(local_array, local_array_sizes[rank]);
   t5 = MPI_Wtime();

   sorted_array = malloc(n*sizeof(double));


   fdspls[0] = 0;
   for(i = 0 ; i< p-1 ;i++)
   {
      fdspls[i+1] = fdspls[i] + local_array_sizes[i];
   }
   MPI_Gatherv(local_array, local_array_sizes[rank], MPI_DOUBLE, 
               sorted_array, local_array_sizes, fdspls, MPI_DOUBLE,0, MPI_COMM_WORLD); 
   tend = MPI_Wtime();


   t_gen = t2-t1;
   t_bin = t3-t2;
   t_dist = t4-t3;
   t_srt = t5-t4;
   t_gath = tend-t5;
   t_total = tend-tstart;

   double times[] = {t_gen, t_bin, t_dist, t_srt, t_gath, t_total};
   double times_sum[6];
   double times_min[6];
   double times_max[6];

   MPI_Reduce(times, times_sum, 6, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
   MPI_Reduce(times, times_min, 6, MPI_DOUBLE, MPI_MIN, 0, MPI_COMM_WORLD);
   MPI_Reduce(times, times_max, 6, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);

   if (rank == 0)
   {
      /*printf("sorted:\n");
      for (i = 0; i < n ; i++)
         printf("%f\n",sorted_array[i]);*/

      if (verify(sorted_array, n-n%p))
         printf("Successful sort\n");
      else
        printf("Unsuccessful sort\n");

      printf("data generation took %.2f\n",t2-t1);
      printf("binning took %.2f\n",t3-t2);
      printf("distribution took %.2f\n",t4-t3);
      printf("local sort took %.2f\n",t5-t4);
      printf("data gathering took %.2f\n",tend-t5);
      printf("total time: %.2f\n",tend-tstart);
   
      printf("total time: min: %.2f, avg: %.2f, max: %.2f\n",times_min[5], times_sum[5]/p, times_max[5]);
   }


  
   MPI_Finalize();
   return 0;
   
}
