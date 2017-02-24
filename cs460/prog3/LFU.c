/*
 *	LFU.c
 *
 *	Opperating Systems LFU Assignment 3
 *
 *	Erik Lam
 *      Blake Oliveira
 */

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include "MemSim.h"

#define TABLESIZE	97		// size of hash table of processes
#define MAXPROC		16		// maximum number of processes in the system
#define PAGESIZE	4096	// system page size
#define MAXPAGE 	2048
#define INTERVAL	1000	// the interval at which we divide the reference count by 2
//#define DEBUG	

// data structure for process hash table entries
typedef struct procstruct {
	int pid;
	int ref;
	int page;						//The number of references this page has gotten
	struct procstruct* location;	//The location on the inverted page table where this is stored
	struct procstruct* next;		//This points to the next element in the hashmap
} proc;

void RemoveNode(int pid);
void printQueues();

proc* table[TABLESIZE] = {0};	// process hash table initialized to 0
proc* invertedTable;			// the inverted page table
proc* lastReplaced = 0;

int pageNum = 0;		
int procCount=0;			// number of processes
int numRead = 0;		// number of page faults requiring only a read
int numWrite = 0;		// number of page faults requiring both write and read
int refCount = 0;


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
			node->ref = node->ref+ 1;
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

void Reset(){
	proc *head= invertedTable;
	while(head){
		head->ref= head->ref/2;
		head = head->location; 
	}
	
	#ifdef DEBUG
		printf("reference count being divided by 2\n");
	#endif	
	
	refCount=0;
}

// called in response to a memory access attempt by process pid to memory address
int Access(int pid, int address, int write) {

	if(++refCount >= INTERVAL){
		Reset();
	}
	
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
	printf("LFU - MMU simulation completed\n");
	printf("%-*s %d\n", 15, "Write faults:", numWrite);
	printf("%-*s %d\n", 15, "Read faults:", numRead);
	printf("%-*s %d\n", 15, "Total faults:", numWrite + numRead);
}

// Finds and removes the node with the least references
void RemoveNode(int pid) {
	proc *curr, *prev = NULL;
	proc *least = invertedTable;
	proc *head = invertedTable;
	
	if (!pid)
		while(head) {
			
			if (head->ref < least->ref){
				least = head;
			}
			head = head->location;
		}
	else {
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
	printf("======================\n procCount: %d ", pageNum);
	
	for (head = invertedTable; head && count <= MAXPROC; head = head->location, count++)
		printf("%d(ref: %d) -> ", head->pid, head->ref);
	printf("\n======================\n\n");
	
	//This means there is a linkedlist loop
	if(count > MAXPAGE)
		exit(0);
}
