#include <iostream>
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <random>
#include <ctime>
#include <mutex>
#include <vector>
#include <future>
#include <math.h>

using namespace std;

// get the wall time
double get_wallTime() {
	struct timeval tp;
	gettimeofday(&tp, NULL);
	return (double)(tp.tv_sec + tp.tv_usec / 1000000.0);
}

// A vanilla random number generator
double genRandNum(double min, double max) {
	return min + (rand() / (RAND_MAX / (max - min)));
}

typedef struct container {
	double max;
	int i;
	int j;
} container;

//This thread coordinates all the other threads to set the largest number
/*
void master(int *cont, int *arrive, int *done, int numThreads) {
	//Done is used to signal when all the operations are complete
	int numBits = pow(2, numThreads) - 1;
	while(*done != numBits) {

		//Wait for all arrives to equal 1
		while(*arrive != numBits);
		//Set all cont[i] to 1
		*cont = numBits;
	}
}
*/

/* Uses mutual exclusion to calculate a critical region */
/*
void slave(int slaveNum, int *arrive, int *cont, double **myArray, int indexLow, 
		int indexHigh, double *largestEntry, int *largestIndexI, int *largestIndexJ, 
		mutex *myMutex, int numThreads, int *done) {

	for(int i = indexLow; i < indexHigh; i++) {
		for(int j = 0; j < indexHigh * numThreads; j++) {

			if(myArray[i][j] > *largestEntry) {
				(*myMutex).lock();
				if(myArray[i][j] > *largestEntry) {
					*largestEntry = myArray[i][j];
					*largestIndexI = i;
					*largestIndexJ = j;
				}
				(*myMutex).unlock();
			}
			//set arrive[i] to 1
			*arrive |= 1 << slaveNum;

			//wait for cont[i] == 1
			while((*cont >> slaveNum) & 1 == 0);

			//clear bit arrive[slaveNum] and cont[slaveNum]
			*arrive &= ~(1 << slaveNum);
			*cont &= ~(1 << slaveNum);
		}
	}
	//arrive[slaveNum] and done[slaveNum] == 1
	*done |= 1 << slaveNum;
	*arrive |= 1 << slaveNum;
}
*/

container threaded(double **myArray, int low, int high, int dim) {
	
	double largestEntry = 0.0;
	int largestIndexI = 0, largestIndexJ = 0;
	
	for(int i = low; i < high; i++) {
		for(int j = 0; j < dim; j++) {
			if(myArray[i][j] > largestEntry) {
				largestEntry = myArray[i][j];
				largestIndexI = i;
				largestIndexJ = j;
			}
		}
	}

	container c;
	c.max = largestEntry;
	c.i = largestIndexI;
	c.j = largestIndexJ;
	return c;
}

void noThreads(double *largestEntry, int *largestIndexI, int *largestIndexJ, double **myArray, int dim) {
	for(int i = 0; i<dim; i++) {
		for(int j = 0; j<dim; j++) {
			if(myArray[i][j] > *largestEntry) {
				*largestEntry = myArray[i][j];
				*largestIndexI = i;
				*largestIndexJ = j;
			}
		}
	}
}
// Main routine
int main(int argc, char **argv) {

	int numThreads = 8;
	int cont = 0, arrive = 0, done = 0, dim = 0;

	// Seed the random number generator
	srand(time(NULL));

	if(argc > 0)
		dim = atoi(argv[1]);
	else {
		cout << "You must pass in the dimension parameter\n";
		exit(1);
	}

	int chunkSize = dim / numThreads;

	// Specify max values
	double max = (double)(dim * dim * dim);
	double min = (double)(dim * dim * dim * -1.0);

	// Create a 2D array
	double **myArray = new double*[dim];
	for(int i = 0; i<dim; i++) {
		myArray[i] = new double[dim];
		for(int j = 0; j<dim; j++) {
			// generate random number
			myArray[i][j] = genRandNum(min, max);
		}
	}

	// Find the largest element
	double largestEntry = 0.0;
	int largestIndexI = -1, largestIndexJ = -1;

	double startNonThreadedTime = get_wallTime();
	noThreads(&largestEntry, &largestIndexI, &largestIndexJ, myArray, dim);
	double endNonThreadedTime = get_wallTime();

	// Output the largest entry and its indices
	cout << " The largest entry is " << myArray[largestIndexI][largestIndexJ] << endl;
	cout << " At indices " << largestIndexI << ", and " << largestIndexJ << endl;
	cout << " The amount of time taken is " << endNonThreadedTime - startNonThreadedTime << endl;

	largestEntry = 0.0;
	largestIndexI = largestIndexJ = -1;

	//vector<future<void>> slaveThreads;
	//mutex myMutex;

	vector<future<container>> threads;

	double startThreadedTime = get_wallTime();
	//auto controller = async(launch::async, master, &cont, &arrive, &done, numThreads);

	for(int thread = 0; thread < numThreads; thread++) {
		int indexStart = thread * chunkSize;
		int indexEnd = (thread + 1) * chunkSize;
		threads.emplace_back(async(launch::async, threaded, myArray, indexStart, indexEnd, dim));
		/*slaveThreads.emplace_back(async(launch::async, slave, slaveNum, &arrive, &cont, myArray,
			indexStart, indexEnd, &largestEntry, &largestIndexI, &largestIndexJ, &myMutex, 
			numThreads, &done));*/
	}

	//controller is last to finish
	//controller.get();
	for(auto&& thread : threads) {
		container c = thread.get();
		if(c.max > largestEntry) {
			largestEntry = c.max;
			largestIndexI = c.i;
			largestIndexJ = c.j;
		}
	}


	double endThreadedTime = get_wallTime();

	// Output the largest entry and its indices
	cout << "\n=========================================================\n\n";
	cout << " The largest entry is " << myArray[largestIndexI][largestIndexJ] << endl;
	cout << " At indices " << largestIndexI << ", and " << largestIndexJ << endl;
	cout << " The amount of time taken is " << endThreadedTime - startThreadedTime << endl;
}
