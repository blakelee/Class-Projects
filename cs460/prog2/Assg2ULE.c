/*
 *  Demonstration of CPU scheduler simulator
 *  using ULE scheduler
 *
 *  Blake Oliveira, WWU Fall 2016
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <assert.h>
#include "Dispatcher.h"
#include "SchedSim.h"

#define TIMESLICE	100
#define READY		0
#define WAITING		1
#define RUNNING		2

typedef struct processDetails {
	int pid;
	int state;
	double timeStart;
	clock_t time; //the time we put it on the readyqueue
	clock_t cputime;
	clock_t iotime;
	struct processDetails *next;
} proc;

proc *current = 0;	//The queue that holds the processes to be 
proc *next = 0;	        //Holds a list of all processes that are waiting or running
proc *table = 0;
int procTime = 0;

void PushBackNode(proc *node, proc **queue);
void InsertProcTable(proc *node);
void RemoveNode(proc *node, proc **queue);
proc *GetProc(int pid);
proc *NewProc(int pid);

void printQueues() {
	int count = 0;
	proc *head;
	printf("======================\n");
	printf("current: ");
	for (head = current; head; head = head->next)
		printf("%d -> ", head->pid);
	printf("\nnext: ");
	for (head = next; head; head = head->next)
		printf("%d -> ", head->pid);
	printf("\nproctable: ");
	for (head = table; head; head = head->next)
		printf("%d -> ", head->pid);
	printf("\n======================\n\n");
}

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
	if (!current && next) {
		current = next;
		next = 0;
	}

	else if(current) {
		*pid = current->pid;
		proc *oldcurr = current;
		current = current->next;
		RemoveNode(oldcurr, &current);
		InsertProcTable(oldcurr);
		printf("process %d dispatched\n", *pid);
	} 

	printQueues();
}

// Informs the student's code that the process with id = pid has changed from the running state
// or waiting state to the ready state
// The process should be added to the current queue
void Ready(int pid, int CPUtimeUsed) {

	if(!CPUtimeUsed) {
		proc *newproc = NewProc(pid);
		PushBackNode(newproc, &current);
	}

	else {
		printf("process %d added to current queue after using %d msec\n", pid, CPUtimeUsed);
		procTime += CPUtimeUsed;
		proc* tempproc = GetProc(pid);	//Get the process from the process table
		RemoveNode(tempproc, &table);		//Remove from process table

		if (CPUtimeUsed >= TIMESLICE)
			PushBackNode(tempproc, &next);		//Put on next queue
		else
			PushBackNode(tempproc, &current);	//Put on current queue
	}

	printQueues();
}

void Waiting(int pid) {
// Informs the student's code that the process with id = pid has changed from the running state
// to the waiting state
	printf("process %d waiting\n", pid);
	GetProc(pid)->state = WAITING;
}


void Terminate(int pid) {
// Informs the student's code that the process with id = pid has terminated

	proc *node = GetProc(pid);
	RemoveNode(node, &table);
	free(node);
	printf("process %d terminated\n", pid); 
}

//Pushes to the end of the queue
void PushBackNode(proc *node, proc **queue) {
	if (!*queue){
		*queue = node;
		node->next = NULL;
	}

	else {
		proc *head = *queue;
		for(head; head->next; head = head->next);
		head->next = node;
		node->next = NULL;
	}
}

void InsertProcTable(proc *node) {
	node->next = table;
	table = node;
}

void RemoveNode(proc *node, proc **queue) {
	proc *curr, *prev = NULL;

	for(curr = *queue; curr != NULL; prev = curr, curr = curr->next) {
		if (curr->pid == node->pid) {
			if(prev == NULL) 
				*queue = (*queue)->next;
			else
				prev->next = curr->next;

			return;
		}
	}
}

//Returns the process in the proctable if there is one
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
	proc *temp = (proc *)malloc(sizeof(proc));
	temp->pid = pid;
	temp->time = 0;
	temp->cputime = 0;
	temp->iotime = 0;
	temp->next = NULL;
	return temp;
}

double get_wallTime() {
    struct timeval tp;
	gettimeofday(&tp, NULL);
	return (double)(tp.tv_sec + tp.tv_usec / 1000000.0);
}

int main() {
	// Simulate for 100 rounds, timeslice=100
    double startT = get_wallTime();
	Simulate(1000, TIMESLICE);
	double endT = get_wallTime();
	printf("Process time: %f\nTotal time:%f\nOverhead time: %f\n", (double)procTime/1000.0, endT - startT, endT - startT - (double)procTime/1000.0);
}
