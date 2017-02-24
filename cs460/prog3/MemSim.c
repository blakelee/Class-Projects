/*
 *	MMUsim.c
 *
 *	Simulator for CSCI 460 memory management exercise
 *
 *	David Bover, Computer Science, WWU, May 2007
 */

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include "Assg3.h"

#define MAXPROC		2048
#define DELTA		32		// number of pages in process working set
#define	DEATHPROB	10		// probability (0.01%) of process termination
#define SAMEPROB	50		// probability (%) of accessing same page next time
#define	MAXPAGES	2048		// number of pages in memory
#define PAGESIZE	4096		// bytes
#define FEEDNEWPROB	90		// probability (%) of feeding new processes in each round
#define PROCPAGES	2048		// maximum pages for any process
#define BACKOFFCOUNT	100		// delay in process creation when memory full
#define	WRITEPROB	20		// probability (20%) of write access to a page

typedef struct procStruct {		// information on an active process
	int pid;			// process id
	int pages[DELTA];		// array of pages recently accessed
	int pageCount;			// number of entries in pages array
	int changeProb;			// probability (%) of change in working set
	int procPages;			// pages in process memory
	int prevPage;			// previous page accessed
} proc;

typedef struct newProcStruct {		// information on a new process, building up its page list
	int proc;			// index into procData array
	int count;			// how many pages still to be added to its page list
	struct newProcStruct* next;	// next pointer for doubly linked list
	struct newProcStruct* prev;	// prev pointer for doubly linked list
} newProc;




// functions for process manipulation

void generateAccess(int procNum);

void createProcess(int procNum);

void terminateProcess(int procNum);

// functions for management of new processes

void addToList(int procNum, int initCount);

void removeFromList(newProc* node);

void updateNew(int index);

void findAndRemove(int procNum);



proc procData[MAXPROC];			// the main data array for processes
int nextpid = 1001;			// unique pid for each process

newProc* list = NULL;			// circular linked list of new processes
int newProcCount = 0;			// count of new processes in list

int rejectCountdown = 0;		// to cause back-off from process creation

void initProcData() {
	int i;
	for (i = 0; i < MAXPROC; i++)
		procData[i].pid = 0;
}





// functions for manipulation of circular linked list of new processes

void addToList(int procNum, int initCount) {
	newProc* node = (newProc*) malloc(sizeof(proc));
	node->proc = procNum;
	node->count = initCount;
	if (list == NULL) {
		node->next = node;
		node->prev = node;
	} else {
		node->next = list;
		node->prev = list->prev;
		list->prev->next = node;
		list->prev = node;
	}
	list = node;
	newProcCount++;		
}


void updateNew(int index) {
	newProc* node = list;
	int i;
	if (node == NULL) return;

	for (i = 0; i < index; i++) 
		node = node->next;
	generateAccess(node->proc);
	node->count--;
	if (node->count <= 0)
		removeFromList(node);
}

void removeFromList(newProc* node) {
	if (node->next == node)
		list = NULL;
	else {
		node->prev->next = node->next;
		node->next->prev = node->prev;
		if (list == node)
			list = node->next;
	}
	free(node);
	newProcCount--;
}

void findAndRemove(int procNum) {
	newProc* node = list;
	if (list == NULL) return;
	while (node->proc != procNum) {
		node = node->next;
		if (node == list) 		// it's not in the list	
			return;
	}
	removeFromList(node);
}


void createProcess(int procNum) {	// initialize data for a new process
	int i;
	procData[procNum].pid = nextpid++;
	//printf("create process %d\n", procData[procNum].pid);
	for (i = 0; i < DELTA; i++)
		procData[procNum].pages[i] = -1;
	procData[procNum].pageCount = 0;
	procData[procNum].changeProb = 10 + rand() % 10;
	do {
		procData[procNum].procPages = rand() % PROCPAGES;	
	} while (procData[procNum].procPages == 0);
	addToList(procNum, rand() % (2 * DELTA));
	generateAccess(procNum);
}

void generateAccess(int procNum) {
	int pageIndex = rand() % DELTA;
	int page;
	int address;

	if (rand() % 100 < SAMEPROB)
		page = procData[procNum].prevPage;
	else if (procData[procNum].pages[pageIndex] == -1) {	// page not in working set
		page = rand() % procData[procNum].procPages;
		procData[procNum].pages[pageIndex] = page;
	} else {
		if (rand() % 100 < procData[procNum].changeProb) {	// jump to new page
			page = rand() % procData[procNum].procPages;
			procData[procNum].pages[pageIndex] = page;
		} else {
			page = procData[procNum].pages[pageIndex];
			//printf("been here before\n");
		}			
	}
	procData[procNum].prevPage = page;
	address = page * PAGESIZE + rand() % PAGESIZE;
	if (!Access(procData[procNum].pid, address, rand() % 100 < WRITEPROB ))
		rejectCountdown = BACKOFFCOUNT;;
}

void terminateProcess(int procNum) {
	Terminate(procData[procNum].pid);
	procData[procNum].pid = 0;
	findAndRemove(procNum);
}

void Simulate(int rounds) {		// the function called by student programs to simulate
					// process memory demands
	int i;
	int procNum;
	

	srand(time(NULL));
	
	for (i = 0; i < rounds; i++) {	// for each round of the simulation

		// do we feed new processes or pick a random process?
		if (rand() % 100 < FEEDNEWPROB && newProcCount > 0) {
			// feed a new process
			//printf("feeding\n");
			updateNew(rand() % newProcCount);

		} else {
			// pick a random process
			procNum = rand() % MAXPROC;		// choose a process index
			while (procData[procNum].pid == 0  && rejectCountdown > 0)
				procNum = rand() % MAXPROC;
			//printf("process slot %d\n", procNum);
			if (rejectCountdown > 0) rejectCountdown--;

			if (procData[procNum].pid == 0)		
				createProcess(procNum);		// a new process
			else {
				generateAccess(procNum);	// existing process
				if (rand() % 10000 < DEATHPROB)
					terminateProcess(procNum);
			}
		}
	}


}
