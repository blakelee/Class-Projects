#include <iostream>
#include <future>
#include <sys/time.h>
#include <stdio.h>
#include <vector>
#include <stdio.h>
#include <string.h>

using namespace std;

//Global variable to set the number of threads we will run
const int numthreads = 8;

double get_wallTime() {
	struct timeval tp;
	gettimeofday(&tp, NULL);
	return (double)(tp.tv_sec + tp.tv_usec / 1000000.0);
}

int multiplyparallel(double **a, double **b, double **c, int low, int hi, int threads) {

	for(int i = low; i < hi; i += threads) {
		for(int j = 0; j < hi; j++) {
			c[i][j] = 0.0;

			for(int k = 0; k < hi; k++) {
				c[i][j] = c[i][j] + a[i][k] * b[k][j];
			}
		}
	}
	return 0;
}

int multiply(double **a, double **b, double **c, int hi) {

	for(int i = 0; i < hi; i++) {
		for(int j = 0; j < hi; j++) {
			c[i][j] = 0.0;

			for(int k = 0; k < hi; k++) {
				c[i][j] = c[i][j] + a[i][k] * b[k][j];
			}
		}
	}
	return 0;
}

int main(int argc, char **argv) {

	bool threaded = false;
	int invocations = atoi(argv[1]);
	int dim = atoi(argv[3]);

	//Check valid threads or invocations
	if(numthreads <= 0) {
		dprintf(2, "Number of times invoked need to be greater than 0\n");
		exit(1);
	}

	if(!strcmp(argv[2], "yes"))
		threaded = true;

	if(atoi(argv[3]) <= 0) {
		dprintf(2, "Dimension needs to be greater than 0\n");
		exit(1);
	}

	double **a = new double*[dim];
	double **b = new double*[dim];
	double **c = new double*[dim];

	for(int i = 0; i < dim; i++) {
		a[i] = new double[dim];
		b[i] = new double[dim];
		c[i] = new double[dim];

		for(int j = 0; j < dim; j++) {
				a[i][j] = 3.6;
				b[i][j] = 1.1;
		}
	}

	vector<future<int>> threads;

	double startTime = get_wallTime();

	if(threaded) {

		/* Starts a variable number of threads based on what numthreads was set to on the commandline
		 * It computes the rows in parallel
		 */
		for(int i = 0; i < numthreads; i++)
			for(int j = 0; j < invocations; j++)
				threads.emplace_back(async(launch::async, multiplyparallel, a, b, c, i, dim, numthreads));

		//Automatically gets the threads when they are done
		for(auto&& num : threads)
			num.get();

		cout << "Time taken for threaded is " << get_wallTime() - startTime <<
			"\nThe value of result[" << dim / 2 << "][" << dim / 2 << "] = " << c[dim / 2][dim / 2] << endl;
	}
	else {
		//Simple non-threaded iteration of results
		for(int i = 0; i < invocations; i++) {
			multiply(a, b, c, dim);
		}
		cout << "Time taken for non-threaded is " << get_wallTime() - startTime <<
			"\nThe value of result[" << dim / 2 << "][" << dim / 2 << "] = " << c[dim / 2][dim / 2] << endl;
	}

	return 0;
}

