/*
 *  Demonstration of CPU scheduler simulator
 *  using cheap, limited ready queue
 *
 *  David Bover, WWU Computer Science, June 2007
 *
 */


#include <stdio.h>
#include <stdlib.h>
#include "Dispatcher.h"
#include "SchedSim.h"

#define SIZE	97
#define DEFAULT	8

typedef struct procstruct {  // process structure
	int id;
	clock_t iotime;
	clock_t cputime;
	clock_t waitingtime;
	int priority;
	struct procstruct *next;
} proc;

proc readyproc[SIZE];
proc proctable[SIZE];
int numready = 0;
int first = 0;
int last = 0;

int totproc = 0; 	//Every process terminated will add to totproc
clock_t avgproctime;
clock_t avgcputime;
clock_t avgiotime;
clock_t lowproctime;
clock_t lowcputime;
clock_t lowiotime;
clock_t highproctime;
clock_t highcputmie;
clock_t highiotime;

void NewProcess(int pid) {
// Informs the student's code that a new process has been created, with process id = pid
// The new process should be added to the ready queue
	printf("new process %d created\n", pid);
	Ready(pid, 0);				// note: you need to invoke Ready()
}

void Dispatch(int *pid) {
// Requests the pid of the process to be changed to the running state
// The process should be removed from the ready queue
	*pid = 0;				// zero pid implies an empty ready queue
							// this should never happen, but ...
	if (numready > 0) {
		numready--;
		*pid = readyproc[last++];
		if (last >= SIZE) last = 0;
		printf("process %d dispatched\n", *pid);
	} 
}

void Ready(int pid, int CPUtimeUsed) {
// Informs the student's code that the process with id = pid has changed from the running state
// or waiting state to the ready state
// The process should be added to the ready queue

	if(!CPUtimeUsed) {
		
	}
		
	if (numready < SIZE) {
		numready++;
		readyproc[first++] = pid;// if the ready queue fills, we start to lose pids
								// this is for demo purposes only!
		if (first >= SIZE) first = 0;
		printf("process %d added to ready queue after using %d msec\n", pid, CPUtimeUsed);
	}
}

void Waiting(int pid) {
// Informs the student's code that the process with id = pid has changed from the running state
// to the waiting state
	printf("process %d waiting\n", pid);
}


void Terminate(int pid) {
// Informs the student's code that the process with id = pid has terminated

	totalproc++;
	
	printf("process %d terminated\n", pid);
}

//Used when removing from ready queue but not terminating
int AddToProcTable(int pid, time_t time, int priority) {
	proc* node =  (proc*)malloc(sizeof(proc));
	if (node == NULL) return 0;

	int index = pid % SIZE;
		
	node->id = pid;
	node->priority = priority;
	node->time = time;
	node->next = proctable[index];

	proctable[index] = node;
	return 1;
}

//Used when adding to ready queue or terminating
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

//Gets the process from the proctable
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

int main() {
	// Simulate for 100 rounds, timeslice=100
	Simulate(1000, 100);
}
