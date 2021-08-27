This repo contains two parallel implementations of bucket sort algorithm using MPI. Both implementations use the same binning mechanism, but they differ in terms of the source of the input values.

1) Single source bucket sort (bucket_sort_v1.c):
* generate: create the array at the root process
* bin: root process determines buckets -- which data belongs to which processor
* distribute: root sends buckets to appropriate processes
* local sort: each process sorts the data locally using quicksort
* gather: finally, results are gathered at the root process
* verification: root process should verify the correctness of the result by scanning the list from start to end and make sure elements are in increasing order.

2) All source bucket source (bucket_sort_v2.c):
* generate: create N/P elements randomly on each process (using uniform distribution),
* bin: each process determines buckets - which of their data belongs to which processor,
* distribute: each process sends its buckets to corresponding processes,
* local sort: each process sorts their data locally using quicksort,
* gather: finally, results are gathered back at the root process.
* verification: root process should again verify the correctness of the result.