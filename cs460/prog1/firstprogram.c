/*
Blake Oliveira
Operating Systems CS 460
October 18, 2016
Homework 1
*/


#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <fcntl.h>

#define LINELEN 1024

int programA();
int programB();

const char *name = "dataSharing";

int main(int argc, char *argv[]) {

	if(!strcmp(argv[1], "a"))
		programA();
	else if(!strcmp(argv[1], "b"))
		programB();

	return 0;
}

int programA() {
	pid_t child;
	int status, shm_fd;
	char buffer[LINELEN];
	void *ptr;

	// GET NUMBER TO GIVE TO CHILD 
	fprintf(stdout, "Type in a number: ");
	if (fgets (buffer, LINELEN, stdin) != buffer)
      return -1;

  	// FORK CHILD PROCESS
 	if ((child = fork()) < 0) {
  		perror("fork");
  		return 1;
  	}

	// THIS IS THE CHILD
  	if (child == 0) {

  		//Create a new shared memory object
  		shm_fd = shm_open(name, O_CREAT | O_RDWR, 0777);
  		ftruncate(shm_fd, LINELEN);
  		ptr = mmap(0, LINELEN, PROT_WRITE, MAP_SHARED, shm_fd, 0);

  		//Set the shared memory object to the number we read
  		sprintf(ptr, "%s", buffer);
  		ptr += strlen(buffer);
  		exit(0);
  	}

  	// HAVE THE PARENT WAIT FOR CHILD TO COMPLETE
  	if (wait(&status) < 0) {
  		perror("wait");
  		return 1;
  	}

  	return 0;
}

int programB() {
	pid_t child;
	int status, shm_fd, num, fd[2], len;
	char buffer[LINELEN];
	void *ptr;

	// OPEN A PIPE FOR PARENT/CHILD COM.
	pipe(fd);

  	// FORK CHILD PROCESS
  	child = fork();

  	if (child < 0) {
  		perror("fork");
  		return 1;
  	}

	// THIS IS THE CHILD
  	if (child == 0) {
  		//Close read end of pipe on child
  		close(fd[0]);

  		//Open shared memory object
  		if((shm_fd = shm_open(name, O_RDONLY, 0777)) < 0) {
		    perror("shm_open");
		    exit(1);
		}
		  
  		ptr = mmap(0, LINELEN, PROT_READ, MAP_SHARED, shm_fd, 0);

  		//Get our number
  		num = atoi((char *)ptr);

  		//Write to the pipe
  		if (write(fd[1], &num, sizeof(num)) < 0){
  			perror("write");
  			exit(1);
  		}

  		//Close write end of pipe on child
  		close(fd[1]);

  		exit(0);
  	}

  	// HAVE THE PARENT WAIT FOR CHILD TO COMPLETE
  	if (wait(&status) < 0) {
  		perror("wait");
  		return 1;
  	}

	//Check to see if the child exited properly
	if((WEXITSTATUS(status)) != 0)
	  exit(0);
  	  
  	//Get the number from the child if there is any
  	if ((len = read(fd[0], &num, sizeof(num))) == 0)
  		fprintf(stderr, "Read EOL from pipe\n");

	
  	//Print our number modulo 26
  	fprintf(stdout, "%d\n", num % 26);

  	//Close the pipes
  	close(fd[0]);
  	close(fd[1]);

  	//delete our shared memory object
	munmap(ptr, LINELEN);
  	shm_unlink(name);

  	return 0;
}
