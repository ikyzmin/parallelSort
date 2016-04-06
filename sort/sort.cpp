// sort.cpp: определяет точку входа для консольного приложения.
//

#include "mpi.h"
#include <conio.h>
#include <iostream>
#include <time.h>

#define ARRAY_SIZE(array) (sizeof((array))/sizeof((array[0])))


using namespace std;
int N;
int *sortingArray;
int *npSortingArray;

void outputArray(int *arr, int N) {

	for (int i = 0; i < N; i++) {
		cout << arr[i]<<" ";
	}
	cout << "\n";
}

void arrayInit() {
	for (int i = 0; i < N; i++) {
		sortingArray[i] = rand() % 230;
		npSortingArray[i] = rand() % 230;
	}
}



void startSort(int *arr,int count) {
	int a = 0, b = 0;
	for (int i = 0; i < count; i++) {
		for (int j = i+1; j < count; j++) {
			if (arr[i] > arr[j]) {
				a = arr[j];
				arr[j] = arr[i];
				arr[i] = a;
			}
		}
	}
}


int main(int argc, char *argv[])
{
	int rank, count;
	N = 0;
	MPI_Status status;
	double t1, t2;
	int * basisArray = NULL;
	int *gathered = NULL;
	int *gatheredFinal = NULL;
	srand(time(NULL));
	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &count);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	if (rank == 0) {
		cout << "Input array size: ";
		cin >> N;
	}
	MPI_Bcast(&N, 1, MPI_INT, 0, MPI_COMM_WORLD);
	sortingArray = (int *)calloc(N, sizeof(int));
	npSortingArray = (int *)calloc(N, sizeof(int));
	if (rank == 0) {
		arrayInit();
		//outputArray(sortingArray, N);
	}
	t1 = MPI_Wtime();
	int *received = (int*)calloc(N / count, sizeof(int));
	MPI_Scatter(sortingArray, N / count, MPI_INT, received, N / count, MPI_INT, 0, MPI_COMM_WORLD);
	startSort(received, N / count);
	int m = N / pow(count, 2);
	int l = count + (count / 2) - 1;
	int * newArray = (int*)calloc(count, sizeof(int));
	int j = 0;
	for (int i = 0; i<N  && i / m != count; i += m) {
		newArray[j] = received[i];
		j++;
	}
	if (rank == 0) {
		gathered = (int*)calloc(count*count, sizeof(int));
	}
	MPI_Gather(newArray,count,MPI_INT,gathered,count,MPI_INT,0,MPI_COMM_WORLD);
	basisArray = (int*)calloc(count - 1, sizeof(int));
	if (rank == 0) {
		startSort(gathered, count*count);
		int k = 0;
		for (int i = l; i < count*count && i / l != count; i += count) {
			basisArray[k] = gathered[i];
			k++;
		}
	}
		MPI_Bcast(sortingArray,N,MPI_INT,0,MPI_COMM_WORLD);
		MPI_Bcast(basisArray, count - 1, MPI_INT, 0, MPI_COMM_WORLD);
		int realSize = 0;
		int *newArrayBasis = NULL;
		for (int i = 0; i <= count-1; i++) {
			if (rank == i) {
				newArrayBasis = (int*)calloc(N, sizeof(int));
				for (int j = 0; j < N; j++) {
					if (i - 1 < 0) {
						if (sortingArray[j] <= basisArray[i]) {
							newArrayBasis[realSize] = sortingArray[j];
							realSize++;
						}
					}
					else {
						if ((i - 1) >= 0 && i  <= count-2) {
							if (sortingArray[j] > basisArray[i - 1] && sortingArray[j] <= basisArray[i])
							{
								newArrayBasis[realSize] = sortingArray[j];
								realSize++;
							}
						}
						else {
							if (i>count-2) {
								if (sortingArray[j] > basisArray[i-1]) {
									newArrayBasis[realSize] = sortingArray[j];
									realSize++;
								}
							}
						}
					}
				}
				startSort(newArrayBasis, realSize);
				if (rank == 0) {
					gatheredFinal = (int*)calloc(N, sizeof(int));

					for (int i = 0; i < realSize; i++) {
						gatheredFinal[i] = newArrayBasis[i];
					}
					MPI_Status status;
					int size = 0;
					for (int i = 1; i < count; i++) {
						int * mergeArray = (int*)calloc(N, sizeof(int));
						MPI_Recv(mergeArray,N,MPI_INT,i,i,MPI_COMM_WORLD,&status);
						MPI_Get_count(&status, MPI_INT, &size);
							for (int k = realSize; k < realSize+size; k++) {
							gatheredFinal[k] = mergeArray[k-realSize];
						}
						realSize += size;
					}
					t2 = MPI_Wtime();
					cout << "sorted with "<<t2-t1<<" sec."<<"\n";
					//outputArray(gatheredFinal, N);
				}
				else {
					MPI_Send(newArrayBasis, realSize, MPI_INT, 0, rank, MPI_COMM_WORLD);
				}
			}
		}

		if (rank == 0) {
			double start = MPI_Wtime();
			startSort(npSortingArray,N);
			cout << "bubble sort time is " << (MPI_Wtime() - start) << "sec.";
		}
	MPI_Finalize();
    return 0;
}

