/*	Source file SchedSim.cpp
 *
 *	Simulator of process creation, execution, waiting and termination
 *	to be used in CSCI 460 CPU scheduling exercise
 *
 *	David Bover, WWU Computer Science, July, 2007
 *	modified July 2010
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include "Dispatcher.h"

#define READY	1
#define RUNNING	2
#define WAITING	3
#define SIZE	97
#define SWITCH	10				// msec for context switch
#define STEP	10				// msec for time steps
#define MIN_PROCS	3			// minimum number of processes

#define MIN_IOBOUND			10	// minimum burst time (msec) for IO bound process
#define RNG_IOBOUND			50	// range of burst times (msec) for IO bound process
#define MIN_BALANCED		40	// minimum burst time (msec) for balanced process
#define RNG_BALANCED		200	// range of burst times (msec) for balanced process
#define MIN_CPUBOUND		200	// minimum burst time (msec) for CPU bound process
#define RNG_CPUBOUND		800	// range of burst times (msec) for CPU bound process
#define IOBOUND_PERCENT		50	// percentage of jobs that are IO bound
#define BALANCED_PERCENT	10	// percentage of jobs that are balanced
#define CPUBOUND_PERCENT	40	// percentage of jobs that are CPU bound
#define MIN_WAIT			100	// minimum wait time (msec)
#define RNG_WAIT			300 // range of wait times (msec)

typedef struct procstruct {  // process structure
	int id;
	int timeleft;
	int waittime;
	int minburst;
	int burstrange;
	int state;
	struct procstruct *next;
} proc;


proc* proctable[SIZE];

int AddToProcTable(int pid) {
	int sample;
	proc* node =  (proc*)malloc(sizeof(proc));
	if (node == NULL) return 0;

	int index = pid % SIZE;
	int exectime = 100 + rand() % 500;  // normal exec time .1-.6 seconds
	if (rand() % 100 > 90) 
		exectime = 500 + rand() % 1000; // long exec time .5-1.5 seconds
		
	node->id = pid;
	node->timeleft = exectime;
	node->waittime = 0;
	
	// determine burst characteristics
	sample = rand();
	if (sample % 100 < BALANCED_PERCENT) {	// a balanced job
		node->minburst = MIN_BALANCED;
		node->burstrange = RNG_BALANCED;
	} else if ( sample % 100 < BALANCED_PERCENT + CPUBOUND_PERCENT ) { // a CPU bound job
		node->minburst = MIN_CPUBOUND;
		node->burstrange = RNG_CPUBOUND;
	} else {								// a IO bound job
		node->minburst = MIN_IOBOUND;
		node->burstrange = RNG_IOBOUND;
	}	
		
	node->state = READY;
	node->next = proctable[index];
	proctable[index] = node;
	return 1;
}

void RemoveFromProcTable(int pid) {
	int index = pid % SIZE;
	proc* curr = proctable[index];
	proc* prev = NULL;
	while (curr != NULL && curr->id != pid) {
		prev = curr;
		curr = curr->next;
	}

	if (curr != NULL) {
		if (prev == NULL)
			proctable[index]  = curr->next;
		else
			prev->next = curr->next;
		free(curr);
	}
}

proc* FindProc(int pid) {
	int index = pid % SIZE;
	proc* curr = proctable[index];
	proc* prev = NULL;
	while (curr != NULL && curr->id != pid) {
		prev = curr;
		curr = curr->next;
	}
	return curr;
}

typedef struct waitstruct{
	int id;
	int remain;
	int timeused;
	struct waitstruct *next;
} waitnode;

waitnode* head = NULL;



int ready = 0;
int waiting = 0;



void DumpWait() {
	waitnode* curr = head;
	printf("Wait list: ");
	while (curr != NULL) {
		printf(" %d(%d)", curr->id, curr->remain);
		curr = curr->next;
	}
	printf("\n");
}


void AddToWaitList (int pid, int wait, int runtime) {
	//printf("adding %d to wait list for %d\n", pid, wait);
	waitnode* node = (waitnode*) malloc(sizeof(waitnode));
	if (node == NULL) {
		printf("unable to allocate wait node\n");
		return;
	}

	node->id = pid;
	node->remain = wait;
	node->timeused = runtime;
	node->next = head;
	head = node;
	//DumpWait();
}

void AgeWaitList(int diff) {
	waitnode* prev = NULL;
	waitnode* curr = head;
	waitnode* temp;
	while (curr != NULL) {
		curr->remain -= diff;
		if (curr->remain <= 0) {
			if (prev == NULL) 
				head = curr->next;
			else 
				prev->next = curr->next;
			temp = curr->next;
			waiting--;
			ready++;
			Ready(curr->id, curr->timeused);
			free(curr);
			curr = temp;
		}
		else {
			prev = curr;
			curr = curr->next;
		}
	}
}

void Simulate(int rounds, int timeslice) {

	int newpid = 1000;
	int pid;
	int priority;
	int npid;
	int runtime;
	int length;
	proc* runner = NULL;
	
	// seed random number generator
	srand(time(NULL));
	
	// initialize process table
	for (int i = 0; i < SIZE; i++) proctable[i] = NULL;

	// start some new processes
	while (ready < MIN_PROCS) {
		npid = newpid++;
		if (AddToProcTable(npid)) {
			NewProcess(npid);
			ready++;
		}
	}

	// for each round of the simulation
	for (int i = 0; i < rounds; i++) {
		//printf("Ready %d, waiting %d\n", ready, waiting);
		
		// get valid values for pid
		Dispatch(&pid);
		
		// Look for this pid in the table
		runner = FindProc(pid);
		if (runner == NULL) {
			printf("CPU idle\n");
			while (ready < MIN_PROCS) {
				npid = newpid++;
				if (AddToProcTable(npid)) {
					NewProcess(npid);
					ready++;
				}
			}
			continue;
		}

		if (pid == 0) continue;

		ready--;

		// determine run time for the process
		runtime = rand() % runner->burstrange + runner->minburst;
		if (runtime > timeslice)
			runtime = timeslice;
		if (runtime > runner->timeleft)
			runtime = runner->timeleft;

		//printf("pid %d running for %d, %d left, ready %d, waiting %d\n", 
					//pid, runtime, runner->timeleft, ready, waiting);

		usleep(SWITCH * 1000L);  // context switch penalty

		for (int tick = 0; tick < runtime; tick += STEP) {

			length = (runtime < STEP) ? runtime : STEP;
			usleep(length * 1000L);

			// check for processes completing wait state
			AgeWaitList(length);

		}
		// check for process termination
		runner->timeleft -= runtime;
		if (runner->timeleft <= 0) {
			Terminate(pid);
			RemoveFromProcTable(pid);
		}

		// maybe the process goes into a wait
		else {
			if (runtime < timeslice) {
				AddToWaitList(pid, rand() % RNG_WAIT + MIN_WAIT, runtime);
				Waiting(pid);
				waiting++;
			}
			else  {
				Ready(pid, runtime);
				ready++;
			}
		}

		// if the ready queue is getting low, add some new processes
		while (ready < MIN_PROCS) {
			npid = newpid++;
			if (AddToProcTable(npid)) {
				NewProcess(npid);
				ready++;
			}
		}

	}
	
}



