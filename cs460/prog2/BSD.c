/*
 *  Demonstration of CPU scheduler simulator
 *  using 4BSD scheduler
 *
 *  Blake Oliveira, WWU Fall 2016
 *
 */


#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>
#include "Dispatcher.h"
#include "SchedSim.h"


#define PRIORITY	16
#define DEFAULT		8
#define TIMESLICE	100

typedef struct processDetails {
	int pid;
	int priority;
	clock_t time; //the time we put it on the readyqueue
	clock_t cputime;
	clock_t iotime;
	struct processDetails *next;
} proc;

proc *NewProc(int pid);
void InsertReady(proc *tempproc);
void InsertProc(proc *tempproc);
void RemoveProc(int pid);
void RemoveReady(proc *item);
proc *GetProc(int pid);

proc *readyqueue = 0;	//The queue that holds the processes to be 
proc *table = 0;	//Holds a list of all processes that are waiting or running
proc *curproc = 0;

int totproc = 0;
clock_t minIoTime = 0;

void printQueues() {
	int count = 0;
	printf("======================\n");
	if (curproc)
		printf("curproc: %d(%d)\n", curproc->pid, curproc->priority);
	proc *head = readyqueue;
	printf("readyqueue: ");
	while (head) {
		if (count++ > 15) break;
		printf("%d(%d) -> ", head->pid, head->priority);
		//sleep(1);
		head = head->next;
	}
	printf("\n");
	count = 0;
	proc *head2 = table;
	printf("table: ");
	while (head2) {
		if (count++ > 15) break;
		printf("%d(%d) -> ", head2->pid, head2->priority);
		head2 = head2->next;
		//sleep(1);
	}
	printf("\n");
	printf("======================\n\n");
}

// Informs the student's code that a new process has been created, with process id = pid
// The new process should be added to the ready queue
void NewProcess(int pid) {
	Ready(pid, 0);
}

// Requests from the student's code the pid of the process to be changed to the running state.
// The process should be removed from the ready queue and added to the table
void Dispatch(int *pid) {
	//Should only stay 0 if there's nothing in the ready queue
	*pid = 0;

	//Means we have at least one process running
	if(!curproc)
		curproc = readyqueue;

	if (curproc) {
		*pid = curproc->pid;
		printf("process %d dispatched\n", *pid);

		//New current process is the next one in the ready queue	
		proc *oldcurr = curproc;
		curproc = curproc->next;
	
		RemoveReady(oldcurr);
		InsertProc(oldcurr);

	}
	//printQueues();
}

// Informs the student's code that the process with id = pid has changed from the running state
// or waiting state to the ready state.  
// CPUtimeUsed is the CPU time that the process used.
// The process should be added to the ready queue
void Ready(int pid, int CPUtimeUsed) {

	//NewProcess add to ready queue with priority 8
	if(!CPUtimeUsed) {
		proc *new = NewProc(pid);
		printf("new process %d(%d) created\n", pid, new->priority);
		InsertReady(new);
	}

	else {
		//Process should be in process table already
		printf("process %d added to ready queue after using %d msec\n", pid, CPUtimeUsed);
		proc* tempproc = GetProc(pid);

		if (CPUtimeUsed >= TIMESLICE) {
			if (tempproc->priority < PRIORITY - 1)
				tempproc->priority = tempproc->priority + 1;
		}

		RemoveProc(pid);
		InsertReady(tempproc);
	}

	//printQueues();
}

//Finds a process in the table and gives it a higher priority
void Waiting(int pid) {
	printf("process %d waiting\n", pid);
	proc *tempproc = GetProc(pid);
	
	if (tempproc && tempproc->priority > 0)
		tempproc->priority = tempproc->priority - 1;
}

//Needs to remove our process from the table and add the time to proctime
void Terminate(int pid) {
	proc *TempProc = GetProc(pid);
	RemoveProc(pid);
	free(TempProc);
	printf("process %d terminated\n", pid);
}
// Informs the student's code that the process with id = pid has terminated

//Go through the readyqueue list and insert it to the end of the priority
void InsertReady(proc *tempproc) {
	proc *curr = readyqueue;
	
	//Special case for the first item
	if (!curr || curr->priority > tempproc->priority) {
		tempproc->next = readyqueue;
		readyqueue = tempproc;
	}

	else {
		for (curr = readyqueue; curr->next!= NULL && tempproc->priority >= curr->next->priority; curr = curr->next);
		
		tempproc->next = curr->next;
		curr->next = tempproc;
	}

	if(!curproc)
		curproc = readyqueue;
}

void InsertProc(proc *tempproc) {
	tempproc->next = table;
	table = tempproc;
}


//Returns the process in the processtable if there is one
proc *GetProc(int pid) {
	proc *curr = table;

	while(curr && curr->pid != pid)
		curr = curr->next;

	if(!curr)
		return 0;
	return curr;
}

//Makes a new proc
proc *NewProc(int pid) {
	proc *temp = malloc(sizeof(proc));
	temp->pid = pid;
	temp->priority = DEFAULT;
	temp->time = 0;
	temp->cputime = 0;
	temp->iotime = 0;
	temp->next = NULL;
	return temp;
}

//Remove process from the table
void RemoveProc(int pid) {
	proc *curr, *prev = NULL;	

	for(curr = table; curr != NULL; prev = curr, curr = curr->next) {
		if (curr->pid == pid) {
			if(prev == NULL) 
				table = table->next;
			else
				prev->next = curr->next;

			return;
		}
	}
}

void RemoveReady(proc *item) {
	proc *curr, *prev = NULL;	

	for(curr = readyqueue; curr != NULL; prev = curr, curr = curr->next) {
		if (curr->pid == item->pid) {
			if(prev == NULL) 
				readyqueue = readyqueue->next;
			else
				prev->next = curr->next;

			return;
		}
	}
}

void main(){
	clock_t startTime = clock();
	Simulate(768, TIMESLICE);
	clock_t totalTime = clock() - startTime;

	fprintf(stdout, "End simulation\n");
}
