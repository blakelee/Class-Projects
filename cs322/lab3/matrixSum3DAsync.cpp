#include <iostream>
#include <future>
#include <sys/time.h>
#include <stdio.h>

double get_wallTime() {
	struct timeval tp;
	gettimeofday(&tp, NULL);
	return (double)(tp.tv_sec + tp.tv_usec / 1000000.0);
}

long double myFunction(double*** array3D, double dimLower, double dimUpper, int dim) {
	long double sum = 0.0;

	for(int i = dimLower * dim; i < dimUpper * dim; i++) {
		for(int j = dimLower * dim; j < dimUpper * dim; j++) {
			for(int k = dimLower * dim; k < dimUpper * dim; k++) {
				sum += array3D[i][j][k];
			}
		}
	}

	return sum;
}

int main() {

	//This is the number of threads we use to dynamically alter the number
	//with as little changing of code as possible
	int threads = 8;

	//Get the clock time, t1
	double t1 = get_wallTime();

	//Variable to change if we want to increase array size
	int dim = 1000;

	long double myOutput = 0.0;

	double ***my3DArray = new double**[dim];

	for(int i = 0; i < dim; i++) {
		my3DArray[i] = new double*[dim];

		for(int j = 0; j < dim; j++) {
			my3DArray[i][j] = new double[dim];

			for(int k = 0; k < dim; k++) {
				my3DArray[i][j][k] = 3.6;
			}
		}
	}

	//See how long it took to create the array
	double t2 = get_wallTime();

	std::cout << "Init time : " << t2 - t1 << "\n";

	//Part II - asynchronous summing

	double t3 = get_wallTime();

	auto thread1 = std::async(std::launch::async, myFunction, my3DArray, 0, 1 / threads, dim);
	auto thread2 = std::async(std::launch::async, myFunction, my3DArray, 1 / threads, 2 / threads, dim);
	auto thread3 = std::async(std::launch::async, myFunction, my3DArray, 2 / threads, 3 / threads, dim);
	auto thread4 = std::async(std::launch::async, myFunction, my3DArray, 3 / threads, 4 / threads, dim);
	auto thread5 = std::async(std::launch::async, myFunction, my3DArray, 4 / threads, 5 / threads, dim);
	auto thread6 = std::async(std::launch::async, myFunction, my3DArray, 5 / threads, 6 / threads, dim);
	auto thread7 = std::async(std::launch::async, myFunction, my3DArray, 6 / threads, 7 / threads, dim);
	auto thread8 = std::async(std::launch::async, myFunction, my3DArray, 7 / threads, 8 / threads, dim);

	myOutput = thread1.get() + thread2.get() + thread3.get() + thread4.get() + 
							thread5.get() + thread6.get() + thread7.get() + thread8.get();

	double t4 = get_wallTime();

	std::cout << "SumThread time : " << t4 - t3 << "\n";
	std::cout << "The sum of the matrix's entries, threaded calculation : " << myOutput << "\n";

	//Part III - synchronous summing

	myOutput = 0.0;

	double t5 = get_wallTime();

	for(int i = 0; i < threads; i++)
		myOutput += myFunction(my3DArray, i / threads, ((i + 1) / threads), dim);

	double t6 = get_wallTime();

	std::cout << "SumNonThread time : " << t6 - t5 << "\n";
	std::cout << "The sum of the matrix's entries, non-threaded calculation : " << myOutput << "\n";

	return 0;
}

