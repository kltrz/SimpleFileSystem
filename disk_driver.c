#pragma once
#include "bitmap.h"

#define BLOCK_SIZE 512
// this is stored in the 1st block of the disk
typedef struct {
  int num_blocks;
  int bitmap_blocks;   // how many blocks in the bitmap
  int bitmap_entries;  // how many bytes are needed to store the bitmap
  
  int free_blocks;     // free blocks
  int first_free_block;// first block index
} DiskHeader; 

typedef struct {
  DiskHeader* header; // mmapped
  char* bitmap_data;  // mmapped (bitmap)
  int fd; // for us
} DiskDriver;

/**
   The blocks indices seen by the read/write functions 
   have to be calculated after the space occupied by the bitmap
*/

// opens the file (creating it if necessary_
// allocates the necessary space on the disk
// calculates how big the bitmap should be
// if the file was new
// compiles a disk header, and fills in the bitmap of appropriate size
// with all 0 (to denote the free space);
void DiskDriver_init(DiskDriver* disk, const char* filename, int num_blocks){
	disk.fd = open(filename,O_CREAT | O_EXCL, 0666);
	if(disk.fd == -1){
		printf("Errore nella chiamata open\n");
		exit(1);
	}
	if(ftruncate(disk.fd, num_blocks*BLOCK_SIZE) == -1){
		printf("Errore nella chiamata ftruncate\n");
		exit(1);
	}
	disk.header.num_blocks = num_blocks;
	disk.header.bitmap_blocks = (num_blocks*sizeof(char))/BLOCK_SIZE;
	disk.header.bitmap_entries = num_blocks*sizeof(char);

	disk.header.free_blocks = num_blocks;
	disk.header.first_free_block = 8733;

	disk.bitmap_data = (char*) calloc(num_blocks, sizeof(char));
}



// reads the block in position block_num
// returns -1 if the block is free accrding to the bitmap
// 0 otherwise
int DiskDriver_readBlock(DiskDriver* disk, void* dest, int block_num){
  if(disk.bitmap_data[block_num] == '0'){
    printf("Errore, blocco numero ... è libero\n");
    exit(1);
  }
  if(lseek(disk.fd, block_num*BLOCK_SIZE, 0) < 0){
    printf("Errore nella chiamata lseek\n");
    exit(1);
    } 
  if(read(disk.fd, dest, BLOCK_SIZE) == -1){
    printf("Errore nella chiamata read\n");
    exit(1);
  }
  return 0;
}

// writes a block in position block_num, and alters the bitmap accordingly
// returns -1 if operation not possible
int DiskDriver_writeBlock(DiskDriver* disk, void* src, int block_num){
  if(lseek(disk.fd, block_num*BLOCK_SIZE, 0) < 0){
    printf("Errore nella chiamata lseek\n");
    exit(1);
    } 
  if(write(disk.fd, src, BLOCK_SIZE) == -1){
    printf("Errore nella chiamata read\n");
    exit(1);
  }
  
  disk.bitmap_data[block_num] = '1';
  return 0;
}

// frees a block in position block_num, and alters the bitmap accordingly
// returns -1 if operation not possible
int DiskDriver_freeBlock(DiskDriver* disk, int block_num){
  if(lseek(disk.fd, block_num*BLOCK_SIZE, 0) < 0){
    printf("Errore nella chiamata lseek\n");
    exit(1);
    } 

}

// returns the first free block in the disk from position (checking the bitmap)
int DiskDriver_getFreeBlock(DiskDriver* disk, int start){
  int lenght = strlen(disk.bitmap_data);
  for(i = start; i<lenght; i++){
    if(disk.bitmap_data[i] == '0'){
    	return i;
    }
    return -1;
  }
}

// writes the data (flushing the mmaps)
int DiskDriver_flush(DiskDriver* disk){
  
}
