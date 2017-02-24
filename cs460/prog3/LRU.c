/*
 *	LRU.c
 *
 *	Opperating Systems LFU Assignment 3
 *
 *	Erik Lam
 *  Blake Oliveira
 */

#include <stdio.h>
#include <stdlib.h>
#include "MemSim.h"

#define TABLESIZE	97		// size of hash table of processes
#define MAXPROC		16		// maximum number of processes in the system
#define PAGESIZE	4096	// system page size
#define MAXPAGE		2048	// number of pages in page table
//#define DEBUG				// uncomment to enable debug comments

// data structure for process hash table entries
typedef struct procstruct {
	int pid;
	int page;
	int ref;						//The reference bit either 0 or 1
	int mod;						//The modify bit either 0 or 1
	struct procstruct* location;	//The location on the inverted page table where this is stored
	struct procstruct* next;		//This points to the next element in the hashmap
} proc;

void RemoveNode(int pid);
void printQueues();

proc* table[TABLESIZE] = {0};	// process hash table initialized to 0
proc* invertedTable;			// the inverted page table
proc* lastReplaced = 0;

int procCount = 0;		// number of processes
int pageNum = 0;		// number of pages
int numRead = 0;		// number of page faults requiring only a read
int numWrite = 0;		// number of page faults requiring both write and read

// look for pid in the hash table
// if pid found, return 1
// if pid not found and number of processes < MAXPROC, add pid to hash table and return 1
// if pid not found and number of processes >= MAXPROC, return 0
int find(int pid, int write, int page) {
	int index = pid % TABLESIZE;
	proc* node = table[index];
	proc* prev = NULL;

	// look along the chain for this hash table index
	while (node != NULL && node->pid != pid) {
		prev = node;
		node = node->next;
	}

	// pid not found in hash table
	if (node == NULL) {
		if (procCount >= MAXPROC)
			return 0;		// too many processes

		else {				// add new process
			node = (proc*) malloc(sizeof(proc));
			node->pid = pid;
			node->page= page;
			node->ref = 0;
			node->mod = 0;
			node->next = NULL;
			procCount++;
			if (prev == NULL)
				table[index] = node;
			else
				prev->next = node;
			
			//Insert the new node to the front of the page table
			if(pageNum>=MAXPAGE){
				RemoveNode(0);
			}
			pageNum++;
			node->location = invertedTable;
			invertedTable = node;
			if(write){
				numWrite++;
				
			}
			else{
				numRead++;
			}
		}
	}

//if we found the process in the normal hash we check
//to see if the page/pid combo is inside the inverted page table
	else {
		node = invertedTable;
		while(node!=NULL && !(node->pid == pid && node->page == page)){
			node = node->location;
		}
		if(node==NULL){
			if(pageNum>=MAXPAGE){
				RemoveNode(0);
			}
			node = (proc*) malloc(sizeof(proc));
			node->pid = pid;
			node->page= page;
			node->ref = 0;
			node->mod = 0;
			node->location = invertedTable;
			pageNum++;			
			invertedTable = node;
			if(write){
				numWrite++;
				
			}
			else{
				numRead++;
			}
			
		}
		else{
			node->ref = 1;
			if(write){
				node->mod=1;
			}
		}
	}

	return 1;
}

// remove a pid from the hash table
void Remove(int pid) {
	int index = pid % TABLESIZE;
	proc* node = table[index];
	proc* prev = NULL;

	// look along the chain for this hash table index
	while (node != NULL && node->pid != pid) {
		prev = node;
		node = node->next;
	}

	// if pid found, remove it
	if (node != NULL) {
		if (prev == NULL)
			table[index] = node->next;
		else
			prev->next = node->next;
		free(node);
	}
}

//Resets the reference bits to 0 for processes between start and end
void Reset(proc *start, proc *end){
	do {
		start->ref = 0;
		if(start->pid == end->pid)
			break;
			
		start = start->location;
		if(!start){
			start = invertedTable;
		}
	} while(start->pid != end->pid);
}


// called in response to a memory access attempt by process pid to memory address
int Access(int pid, int address, int write) {
	
	if (find(pid, write, address/PAGESIZE)) {
		printf("pid %d wants %s access to address %d on page %d\n", 
		       pid, (write) ? "write" : "read", address, address/PAGESIZE);
		
		#ifdef DEBUG
		printQueues();	//Used for debugging purposes
		#endif

		return 1;
	} else {
		printf("pid %d refused\n", pid);
		#ifdef DEBUG
		printQueues();	//Used for debugging purposes
		#endif
		
		return 0;
	}
}

// called when process terminates
void Terminate(int pid) {
	printf("pid %d terminated\n", pid);
	RemoveNode(pid);
}

int main() {
	printf("MMU simulation started\n");
	Simulate(1000);
	printf("LRU - MMU simulation completed\n");
	printf("%-*s %d\n", 15, "Write faults:", numWrite);
	printf("%-*s %d\n", 15, "Read faults:", numRead);
	printf("%-*s %d\n", 15, "Total faults:", numWrite + numRead);
}

// Finds and removes the node with the least references
void RemoveNode(int pid) {
	
	proc *curr, *prev = NULL;
	proc *least = invertedTable;
	proc *end = lastReplaced;
	proc *start = lastReplaced;
	
	if(!start) {
		start = end = invertedTable;
	}
	
	if (!pid && invertedTable){
		printf("we are full freeing up space\n");
		int pref = 1000000;
		
		//Search for the frame to modify
		do {
			int newPref = ((0 | end->ref) << 1) + (end->mod); //Gets 0, 1, 2, or 3
				
				if (newPref < pref) {
					pref = newPref;
					least = end;
					if(!pref)
						break; //Found 0, break from while
				}
			end = end->location;
			if(!end)
				end = invertedTable;
		} while (end->pid != start->pid); //Looping through whole list unless we find a 0 newPref
		pageNum--;
		Reset(start, end);
	}
	
	//Otherwise we are terminating
	else {
		proc *head = invertedTable;

		while(head && head->pid != pid) {
			head = head->location;
		}
		least = head;

	}
	
	for(curr = invertedTable; curr != NULL; prev = curr, curr = curr->location) {
		if (curr->pid == least->pid) {
			pageNum--;
			if(prev == NULL)
				invertedTable = invertedTable->location;
			else
				prev->location = curr->location;
			
			if(!least->location){
				lastReplaced = invertedTable;
			}
			else
				lastReplaced = least->location;
				
			if(!pid){
				return;
			}
		}
	}
	return;
}

void printQueues() {
	proc *head;
	int count = 0;
	printf("======================\n pageCount: %d ", pageNum);
	
	for (head = invertedTable; head && count <= MAXPAGE; head = head->location, count++)
		printf("%d/%d(ref: %d mod:%d) -> ", head->pid, head->page, head->ref, head->mod);
	printf("\n======================\n\n");
	
	//This means there is a linkedlist loop
//	if(count > MAXPAGE)
//		exit(0);
}
