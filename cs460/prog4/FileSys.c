/*
Blake Oliveira
Erik Lam

Assignment 4
Operating Systems CSCI 460

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "Driver.h"
#define BYTES_PER_SECTOR	64
#define	SECTORS				32 * 1024	// the number of blocks we have
#define INDEXBLOCKS			16

typedef struct inodeblock_st {
	int inodes[INDEXBLOCKS];
} inodeblock;

typedef struct fileblock_st {
	char *filename;
	int filesize;				//number of bytes the file is
	int size;					//number of data blocks
	int totSize;				//data blocks + inodeblocks
	int block_num;				//location of direct inodeblock
	struct fileblock_st *next;
} FileBlock;

typedef struct span_st {
	int start;	// the starting point of the free blocks
	int num;	// how many blocks are available after the start
	int end;
	struct span_st *next;
} span;

typedef struct superblock_st {
	FileBlock *files;		
	int numFiles;			//the number of files in the fileblock
	int totBlocks;			//equal to SECTORS
	int numBlocks;			//the number of blocks currently used
	span *freeBlocks;		//a span showing which blocks are free
} SuperBlock;


SuperBlock *sb;

int ReadDelete (char *FileName, int MaxSize, char *Data, int flag);
void PrintSpan();

//Gets a block from the span
int GetBlock() {
	int block = sb->freeBlocks->start;
	sb->freeBlocks->start = sb->freeBlocks->start + 1;
	sb->freeBlocks->num = sb->freeBlocks->num - 1;
	if (sb->freeBlocks->num < 0)
		sb->freeBlocks = sb->freeBlocks->next;
	
	return block;
}

int WriteHelper(int blocknum, char *Data) {
	//Write inodeblock to file
	char tempdata[BYTES_PER_SECTOR] = {0};
	memcpy( tempdata, Data, BYTES_PER_SECTOR);
	if (!DevWrite(blocknum, tempdata))
		return 0;
	return 1;
}

int WriteInode(int blocknum, inodeblock *Data) {
	char tempdata[BYTES_PER_SECTOR] = {0};
	memcpy(tempdata, Data, BYTES_PER_SECTOR); 
	if (!DevWrite(blocknum, tempdata))
		return 0;
	return 1;
}

int ReadHelper(int blockNum, int pos, int *bytesLeft, char *Data) {
	
	char tempData[BYTES_PER_SECTOR] = {0};
	
	//Reads a full block
	if(!(DevRead (blockNum, tempData))) return 0;
	
	//Last block to read. Put in null char
	if (*bytesLeft <= BYTES_PER_SECTOR) {
		tempData[*bytesLeft] = '\0';
		memcpy(Data + pos, tempData, *bytesLeft);
		*bytesLeft = 0;
	}
	//More blocks to read
	else {
		*bytesLeft = *bytesLeft - BYTES_PER_SECTOR;
		memcpy(Data + pos, tempData, BYTES_PER_SECTOR);
	}
		
	return 0;
}

int CSCI460_Format ( ) {			// Formats the file system
	span * oldSpan;
	if (DevFormat()) {
		
		//Delete superblock if already allocated
		if (sb){
						//free inodeblocks if allocated
			free(sb->files);
			
			//free freeBlocks span
			while(sb->freeBlocks) {
				oldSpan = sb->freeBlocks;
				sb->freeBlocks = sb->freeBlocks->next;
				free(oldSpan);
			}
			
			free(sb);
		}
		
		sb = (SuperBlock *)malloc(sizeof(SuperBlock));
		sb->files = 0;
		sb->numFiles = 0;
		sb->totBlocks = SECTORS;
		sb->numBlocks = 0;
		span *newSpan = (span *)malloc(sizeof(span));
		newSpan->start = 0;
		newSpan->num = sb->totBlocks;
		newSpan->end = sb->totBlocks;
		newSpan->next = 0;
		sb->freeBlocks = newSpan;
		return 1;
	}
		
	return 0;
}

int CSCI460_Delete(	char *FileName) {
	ReadDelete(FileName, 0, 0, 1);	// Deletes the file

	FileBlock *curr, *prev = NULL;
	
	for(curr = sb->files; curr != NULL; prev = curr, curr = curr->next) {
		if (!strcmp(curr->filename, FileName)) {
			if(prev == NULL) 
				sb->files = sb->files->next;
				
			else
				prev->next = curr->next;
				
			PrintSpan();
			return 1;
		}
	}
	return 0;
}

int DeleteSpan(	span *Block) {					// Deletes the file
	span *curr, *prev = NULL;
	
	for(curr = sb->freeBlocks; curr != NULL; prev = curr, curr = curr->next) {
		if (curr->start == Block->start) {
			if(prev == NULL) 
				sb->freeBlocks = sb->freeBlocks->next;
			else
				prev->next = curr->next;
			free(curr);
			return 1;
		}
	}
	return 0;
}

int removeNode(int block_num, int pos, int *bytesLeft, char *Data){
	span * blocks = sb->freeBlocks;
	span * block1;
	span * block2;
	int flag1,flag2=0;
	while(blocks){
		if(blocks->start == block_num+1){
			blocks->start=blocks->start-1;
			blocks->num= blocks->num+1;
			block1=blocks;
		}
		else if(blocks->end == block_num-1){
			blocks->end=blocks->end+1;
			blocks->num= blocks->num+1;
			block2= blocks;
		}
		blocks = blocks->next;
	}
	if(block1 && block2){
		block1->end=block2->end;
		block1->num= block1->num+block2->num-1;
		DeleteSpan(block2);
	}
	else if(!(block1 || block2)){
		span *newSpan = (span *)malloc(sizeof(span));
		newSpan->start = block_num;
		newSpan->num = 1;
		newSpan->end = block_num;
		newSpan->next = sb->freeBlocks;
		sb->freeBlocks = newSpan;
		
	}
	return 1;
}

//Gets the total number of blocks used to write a file
int GetNumBlocks(int Size) {
	int num = 1; //Initial head block
	int blocksToWrite = ceil((double)Size / BYTES_PER_SECTOR); //Logical blocks to write

	num += blocksToWrite;

	//indirect
	if (blocksToWrite >= 13)
		num++; //new block in direct block chain

	//double indirect
	if (blocksToWrite >= 13 + INDEXBLOCKS) {
		num++; //new block in direct block chain

		int remainder = blocksToWrite - 13 - INDEXBLOCKS;
		remainder = ceil((double)remainder / INDEXBLOCKS);
		

		if (remainder > INDEXBLOCKS)
			num += INDEXBLOCKS;
		else {
			num += remainder;
		}
	}

	//triple indirect
	if (blocksToWrite >= 13 + INDEXBLOCKS + (INDEXBLOCKS * INDEXBLOCKS)) {
		num++;	//new block in direct block chain

		int remainder = blocksToWrite - 13 - INDEXBLOCKS - (INDEXBLOCKS * INDEXBLOCKS);

		//gets number of double indirects and triple indirects
		num += ceil((double)remainder / INDEXBLOCKS) + ceil((double)(((double)remainder / INDEXBLOCKS) / INDEXBLOCKS));
	}

	return num;
}

/*
Writes Data, an array of Size characters to the file FileName. If the file already
exists in the file system, the existing file is to be deleted and a new one written
with the specified data. Return value: 0 = failure, 1 = success.
This function must fail if there is no file system, if the file system is full, or if the
device driver returns an error code.
Note that, for simplicity, this function writes the entire file in one function call.
*/
int CSCI460_Write (	char *FileName, int Size, char *Data) {
	FileBlock *head = NULL;
	FileBlock *newFile = NULL;
	inodeblock *inode;
	
	if(!sb) {
		printf("No file system\n");
		return 0;
	}
	head = sb->files;
	while (head) {
		if(!strcmp(head->filename, FileName)) {
			CSCI460_Delete(FileName);
			break;
		}
		head = head->next;
	}
	
	// have to write minimum of 1
	int blocksToWrite = ceil((double)Size / (double)BYTES_PER_SECTOR);
	int totBlocks = GetNumBlocks(Size);
	
	if (sb->numBlocks + totBlocks >= sb->totBlocks) {
		printf("Not enough disk space\n");
		return 0;
	}
	
	
	//create file
	newFile = (FileBlock*)malloc(sizeof(FileBlock));
	newFile->filename = FileName;
	newFile->filesize = Size;
	newFile->size = blocksToWrite;
	newFile->totSize = totBlocks;
	newFile->block_num = GetBlock();

	sb->numFiles = sb->numFiles + 1;
	
	inodeblock *headinode = (inodeblock *)calloc(1, sizeof(inodeblock));
	inodeblock *indirect = (inodeblock *)calloc(1, sizeof(inodeblock));
	inodeblock *double_ = (inodeblock *)calloc(1, sizeof(inodeblock));
	inodeblock *triple = (inodeblock *)calloc(1, sizeof(inodeblock));
	int i = 0, j, k, l;
	
	//Direct inode
	if(i < 13) {
		for(i; i < 13 && i < blocksToWrite; i++) {
			// Write the data
			if(!(WriteHelper(headinode->inodes[i] = GetBlock(), (Data + (i * BYTES_PER_SECTOR))))) return 0;
		}
	}
	
	//indirect
	if (i == 13) {
			//make the new block and set it to 0
			memset(indirect, 0, sizeof(inodeblock));
			
			//write data partition
			for(j = 0; i < blocksToWrite && j < INDEXBLOCKS; j++, i++) {
				if(!(WriteHelper(indirect->inodes[j] = GetBlock(), (Data + (i * BYTES_PER_SECTOR))))) return 0;
			}
			if (!WriteInode(headinode->inodes[13] = GetBlock(), indirect)) return 0;
		}
	
	//double indirect
	if (i == 13 + INDEXBLOCKS) {
		
		memset(indirect, 0, sizeof(inodeblock));
		for(j = 0; i < blocksToWrite && j < INDEXBLOCKS; j++) {
			memset(double_, 0, sizeof(inodeblock));
			for(k = 0; i < blocksToWrite && k < INDEXBLOCKS; k++, i++) {
				if(!(WriteHelper(double_->inodes[k] = GetBlock(), (Data + (i * BYTES_PER_SECTOR))))) return 0;
			}
			if (!WriteInode(indirect->inodes[j] = GetBlock(), double_)) return 0;
		}
		if (!WriteInode(headinode->inodes[14] = GetBlock(), indirect)) return 0;
	}
	
	//triple indirect
	if (i == 13 + INDEXBLOCKS + (INDEXBLOCKS * INDEXBLOCKS)) {
		memset(indirect, 0, sizeof(inodeblock));
		for(j = 0; i < blocksToWrite && j < INDEXBLOCKS; j++) {
			memset(double_, 0, sizeof(inodeblock));
			for(k = 0; i < blocksToWrite && k < INDEXBLOCKS; k++) {
				memset(triple, 0, sizeof(inodeblock));
				for(l = 0; i < blocksToWrite && l < INDEXBLOCKS; i++, l++) {
					if(!(WriteHelper(triple->inodes[l] = GetBlock(), (Data + (i * BYTES_PER_SECTOR))))) return 0;
				}
				if (!WriteInode(double_->inodes[k] = GetBlock(), triple)) return 0; 
			}
			if (!WriteInode(indirect->inodes[j] = GetBlock(), double_)) return 0; 
		}
		if (!WriteInode(headinode->inodes[15] = GetBlock(), indirect)) return 0; 
	}

	if (!WriteInode(newFile->block_num, headinode)) return 0; 

	free(headinode);
	free(indirect);
	free(double_);
	free(triple);

	newFile->next = sb->files;
	sb->files = newFile;
	sb->numBlocks += totBlocks;
	
	return 1; //successful
}

/* 
Reads characters from the file FileName into the array Data. At most MaxSize-1
characters will be read and a null character put at the end of the string.
Return value: 0 = failure. A non-zero return value indicates the number of
characters read from file.
This function must fail if there is no file system, if the file does not exist, or if the
device driver returns an error code.
Note that, for simplicity, this function reads the entire file in one function call. 
*/
int CSCI460_Read (	char *FileName, int MaxSize, char *Data) {  // Reads entire file
	return ReadDelete(FileName, MaxSize, Data, 0);
}

int ReadDelete (char *FileName, int MaxSize, char *Data, int flag) {
	
	int (*Operation)(int, int , int *, char *);
	if (flag)
		Operation = removeNode;
	else
		Operation = ReadHelper;
	
	int CharsLeft = MaxSize - 1;
	int blocksLeft = ceil((double)MaxSize / BYTES_PER_SECTOR);
	FileBlock *head;
	
	if(!sb) {
		printf("No file system\n");
		return 0;
	}
	head = sb->files;
	while (head) {
		if(!strcmp(head->filename, FileName)) {
			break; //File found
		}
		head = head->next;
	}
	
	if (!head) {
		printf("File not found\n");
		return 0; //File not found
	}

	//can't read in more memory than we have
	if (MaxSize - 1 > head->filesize) {
		CharsLeft = head->filesize;
		blocksLeft = ceil((double)CharsLeft / BYTES_PER_SECTOR);
	}

	inodeblock *headinode = (inodeblock *)calloc(1, sizeof(inodeblock));
	inodeblock *indirect = (inodeblock *)calloc(1, sizeof(inodeblock));
	inodeblock *double_ = (inodeblock *)calloc(1, sizeof(inodeblock));
	inodeblock *triple = (inodeblock *)calloc(1, sizeof(inodeblock));
	
	if(!(DevRead (head->block_num, (char *)headinode))) return 0;
	int i = 0, j, k, l;
	
	//Direct inode
	if(i < 13) {
		for(i; i < 13 && i < blocksLeft; i++) {
			Operation(headinode->inodes[i], i * BYTES_PER_SECTOR, &CharsLeft, Data);
		}
	}
	
	//indirect
	if (i == 13) {
			if(!(DevRead (headinode->inodes[13], (char *)indirect))) return 0;
			
			//write data partition
			for(j = 0; i < blocksLeft && j < INDEXBLOCKS; j++, i++) {
				Operation(indirect->inodes[j], i * BYTES_PER_SECTOR, &CharsLeft, Data);
			}
	}
	
	//double indirect
	if (i == 13 + INDEXBLOCKS) {
		
		if(!(DevRead (headinode->inodes[14], (char *)indirect))) return 0;
		for(j = 0; i < blocksLeft && j < INDEXBLOCKS; j++) {
			if(!(DevRead (indirect->inodes[j], (char *)double_))) return 0;
			for(k = 0; i < blocksLeft && k < INDEXBLOCKS; k++, i++) {
				Operation(double_->inodes[k], i * BYTES_PER_SECTOR, &CharsLeft, Data);
			}
		}
	}
	
	//triple indirect
	if (i == 13 + INDEXBLOCKS + (INDEXBLOCKS * INDEXBLOCKS)) {
		if(!(DevRead (headinode->inodes[15], (char *)indirect))) return 0;
		for(j = 0; i < blocksLeft && j < INDEXBLOCKS; j++) {
			if(!(DevRead (indirect->inodes[j], (char *)double_))) return 0;
			for(k = 0; i < blocksLeft && k < INDEXBLOCKS; k++) {
				if(!(DevRead (double_->inodes[k], (char *)triple))) return 0;
				for(l = 0; i < blocksLeft && l < INDEXBLOCKS; i++, l++) {
					Operation(triple->inodes[l], i * BYTES_PER_SECTOR, &CharsLeft, Data);
				}
			}
		}
	}

	//doesn't work sometimes in ReadHelper
	if(!flag)
	  Data[head->filesize] = '\0';
	
	return 1;
}

void PrintSpan() {
	span *temp = sb->freeBlocks;
	while(temp) {
		printf("[%d - %d] ", temp->start, temp->start + temp->num);
		temp = temp->next;
	}
	printf("\n");
}