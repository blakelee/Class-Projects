/*
 *	Source file DriverTest.c
 *
 *	Test program for device driver used in CSCI 460 Assignment 3
 *
 *	David Bover, WWU, August 2010
 */

#include <stdio.h>
#include <time.h>
#include <stdlib.h>

#include "Driver.h"


int main() {

	char Data1[BYTES_PER_SECTOR];
	char Data2[BYTES_PER_SECTOR];

	DevFormat();
	printf("DevFormat: partition cleared\n");

	// Try some reads and writes with illegal parameters
	printf("this should cause an error\n");
	DevRead(-5, Data1);
	printf("this should cause an error\n");
	DevRead(40000, Data1);
	printf("\n");


	// Test it for 1 million reads and writes
	int block1, block2;
	int cyl2, head2, sct2;
	char result[BYTES_PER_SECTOR];
	int errors = 0;

	srand((unsigned)time(NULL));
	printf("\nTesting with 1,000,000 random reads and writes\n"); 
	for (int i = 0; i < 1000000; i++) {
		if (i % 10000 == 0) fprintf(stderr, ".");
		block1 = rand() % SECTORS;
		
		for (int j = 0; j < BYTES_PER_SECTOR; j++)
			Data1[j] = rand() % 128;
		if (!DevWrite(block1, Data1)) {
			printf("Write error\n");
			errors++;
		}

		// do another write
		do {
			block2 = rand() % SECTORS;
		}  while (block1 == block2);

		for (int j = 0; j < BYTES_PER_SECTOR; j++)
			Data2[j] = rand() % 128;
		if (!DevWrite(block2, Data2)) {
			printf("Write error\n");
			errors++;
		}


		// Now read back the first set of data
		int ok = 1;
		if (!DevRead(block1, result)) {
			printf("Read error\n");
			errors++;
		}
		else {
			for (int j = 0; j < BYTES_PER_SECTOR; j++)
				if (result[j] != Data1[j]) {
					
					ok = 0;
					printf("Wrote %d at %d, but got %d; 2nd write is %d\n", 
						Data1[j], block1, result[j], Data2[j]);
				}

				if (!ok) {
					errors++;
					printf("Readback error at %d 2nd write at %d\n", 
						block1, block2);
				}
		}
	}

	printf("\ntest complete: %d errors\n", errors);

	//char c = getchar(); 

	return 1;
}
