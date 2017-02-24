// Filip Jagodzinski
// CSCI322
#include <iostream>
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <random>
#include <ctime>
#include <vector>
#include <future>

using namespace std;

// get the wall time
double get_wallTime() {
  struct timeval tp;
  gettimeofday(&tp,NULL);
  return (double) (tp.tv_sec + tp.tv_usec/1000000.0);
}

// A vanilla random number generator
double genRandNum(double min, double max){
  return min + (rand() / (RAND_MAX / (max - min)));
}

// A ridiculous calculation
double ridiculousCalc(double* array1, double* array2,
		      double* array3, int low, int hi){
	int index = hi - low;
double* resultArray1 = new double[index];
	double* resultArray2 = new double[index];
	double* resultArray3 = new double[index];
	double* resultArray4 = new double[index];
	double* resultArray5 = new double[index];
	double* outputArray = new double[index];
	double factor = 0.000000001;

	for (int i=0; i<index; i++){
		resultArray1[i] = array1[low + i] / array2[low + i] * array3[low + i] * -1.456;
		resultArray2[i] = resultArray1[i] / array3[low + i] * array3[low + i];
		resultArray3[i] = array3[low + i] * array2[low + i] + array1[low + i] * array2[low + i] / (array2[low + i] * -0002.7897);
		resultArray4[i] = resultArray3[i] * array2[low + i] / array1[low + i];
		resultArray5[i] = resultArray4[i] * array2[low + i] / array1[low + i]
		  * resultArray4[i] * 0.0000000023;
		outputArray[i] = resultArray1[i]*factor + resultArray2[i]*factor 
		  + resultArray3[i]*factor + resultArray4[i]*factor 
		  + resultArray5[i]*factor;
	  }

	double output = 0.0;
	for (int i=0; i<index; i++){
		output += outputArray[i];
	}  

	return output;

}

double ridiculousCalcOptimized(double* array1, double* array2,
	double* array3, int low, int hi) {

	int index = hi - low;
	double* resultArray1 = new double[index];
	double* resultArray3 = new double[index];
	double* outputArray = new double[index];
	double factor = 0.000000001;

	/*My code*/
	for(int i = 0; i < index; i++) {
		resultArray1[i] = array1[low + i] / array2[low + i] * array3[low + i] * -1.456; //No dependance
		outputArray[i] = factor * (resultArray1[i] + resultArray1[i]); /*ra1 & ra2*/
	}


	for(int i = 0; i < index; i++) {
		resultArray3[i] = array3[low + i] * array2[low + i] + array1[low + i] / -0002.7897; //array2 is redundant
		double temp = array2[low + i] / array1[low + i];
		double ra3 = resultArray3[i];
		outputArray[i] += factor * (
			+ ra3 /*ra3*/
			+ ra3 * temp /*ra4*/
			+ ra3 * ra3 * temp * temp * temp * 0.0000000023); /*ra5*/
	}

	/*End my code*/

	double output = 0.0;
	for(int i = 0; i<index; i++) {
		output += outputArray[i];
	}

	return output;

}

// main routine
int main(){

	// dimension of the array
	int dim = 10000000;
	int numThreads = 8;
	int chunkSize = dim / numThreads;
  
	// seed the random number generator
	srand( time(NULL));

	// create the arrays and populate them 
	// time the entire event
	double createStart = get_wallTime();
	static double *array1 = new double[dim];
	static double *array2 = new double[dim];
	static double *array3 = new double[dim];
	for (int i=0; i<dim; i++){
		array1[i] = genRandNum(0, 1000000);
		array2[i] = genRandNum(0, 1000000);
		array3[i] = genRandNum(0, 1000000);
	}
	double createEnd = get_wallTime();
	 cout << "\n Time needed to create arrays                           : "
		<< createEnd - createStart << endl;
	cout << " ========================================================"
		<< endl;

	// perform non-optimized calculations
	double ridiculousStart = get_wallTime();
	double output1 = ridiculousCalc(array1, array2, array3, 0, dim);
	double ridiculousEnd = get_wallTime();

	double optimizedStart = get_wallTime();
	double output2 = ridiculousCalcOptimized(array1, array2, array3, 0, dim);
	double optimizedEnd = get_wallTime();

	vector<future<double>> nonOptimizedThreadedPool;

	double threadedRidiculousStart = get_wallTime();
	for(int i = 0; i < numThreads; i++) {
		int indexStart = i * chunkSize;
		int indexEnd = (i + 1) * chunkSize;
		nonOptimizedThreadedPool.emplace_back(async(launch::async, ridiculousCalc, array1, array2, array3, indexStart, indexEnd));
	} 

	double output3 = 0.0;
	//Automatically gets the threads when they are done
	for(auto&& num : nonOptimizedThreadedPool)
		output3 += num.get();
	double threadedRidiculousEnd = get_wallTime();

	vector<future<double>> optimizedThreadedPool;

	double threadedOptimizedStart = get_wallTime();
	for(int i = 0; i < numThreads; i++) {
		int indexStart = i * chunkSize;
		int indexEnd = (i + 1) * chunkSize;
		optimizedThreadedPool.emplace_back(async(launch::async, ridiculousCalcOptimized, array1, array2, array3, indexStart, indexEnd));
	}

	double output4 = 0.0;
	//Automatically gets the threads when they are done
	for(auto&& num : optimizedThreadedPool)
		output4 += num.get();
	double threadedOptimizedEnd = get_wallTime();

	cout << " Time needed to complete ridiculous calculation         : "
		<< ridiculousEnd - ridiculousStart << endl;
	cout << " Ridiculous calculation output                          : "
		<< output1 << endl;
	cout << " ========================================================"
		<< endl;
	cout << " Time needed to complete optimized calculation          : "
		<< optimizedEnd - optimizedStart << endl;
	cout << " Ridiculous calculation output                          : "
		<< output2 << endl;
	cout << " ========================================================"
		<< endl;
	cout << " Number of threads available:                           : "
		<< numThreads << endl;
	cout << " ========================================================"
		<< endl;
	cout << " Time needed to complete threaded ridiculous calculation: "
		<< threadedRidiculousEnd - threadedRidiculousStart << endl;
	cout << " Ridiculous calculation output                          : "
		<< output3 << endl;
	cout << " ========================================================"
		<< endl;
	cout << " Time needed to complete threaded optimized calculation : "
		<< threadedOptimizedEnd - threadedOptimizedStart << endl;
	cout << " Ridiculous calculation output                          : "
		<< output4 << endl;

	return 0;
}

