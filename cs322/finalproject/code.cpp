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

#include "eventSystem.h"

using namespace std;


//GLOBAL
const int dim = 1000;
int numDone = 0;
int numTimes = 10;

void calculate() {
	int array[dim][dim];
	for(int i = 0; i < dim; i++) {
		for(int j = 0; j < dim; j++)
			array[i][j] = i * j;
	}
	numDone++;
}

double get_wallTime() {
	struct timeval tp;
	gettimeofday(&tp, NULL);
	return (double)(tp.tv_sec + tp.tv_usec / 1000000.0);
}

void doCalculations(eventSystem *es, bool isSequential) {
	bool done = false;
	double startTime = get_wallTime();
	//Gets the time of running all 
	for(int i = 0; i < numTimes; i++) {
		if(isSequential)
			calculate();
		else
			es->addFunction(calculate);
	}

	if(!isSequential) {
		es->execute();
		//Simulate calling events
		for(int i = 0; i < numTimes; i++)
			es->callFunction(i);

		es->checkFunction();
	}

	cout << "time taken is " << get_wallTime() - startTime << endl;
}

int main() {

	eventSystem es = eventSystem();

	//Get event system times
	doCalculations(&es, false);

	//Get sequential times
	doCalculations(&es, true);

	return 0;
}
