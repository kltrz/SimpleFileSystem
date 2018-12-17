#pragma once
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
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
  BitMap bitmap;  // mmapped (bitmap)
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
	/*if(ftruncate(disk.fd, num_blocks*BLOCK_SIZE) == -1){
		printf("Errore nella chiamata ftruncate\n");
		exit(1);
	}*/

	disk.header.num_blocks = num_blocks;
	disk.header.bitmap_blocks = num_blocks/8;
	disk.header.bitmap_entries = disk.header.bitmap_blocks*sizeof(char);

	disk.header.free_blocks = num_blocks;
	disk.header.first_free_block = 0;

	disk.bitmap.num_bits = num_blocks;
    disk.bitmap.entries = (char*) calloc(disk.header.bitmap_blocks, sizeof(char));
}



// reads the block in position block_num
// returns -1 if the block is free accrding to the bitmap
// 0 otherwise
int DiskDriver_readBlock(DiskDriver* disk, void* dest, int block_num){
  BitMapEntryKey key = BitMap_blockToIndex(block_num);
  char bitsofchar[8];
  char c = disk.bitmap_data[key.entry_num];
  chartobin(c, bitsofchar);
  if(bitsofchar[k.bit_num] == '0'){
    printf("Errore, il blocco richiesto libero\n");
    return -1;
  }


  if(lseek(disk.fd, block_num*BLOCK_SIZE, SEEK_SET) < 0){
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
  if(lseek(disk.fd, block_num*BLOCK_SIZE, SEEK_SET) < 0){
    printf("Errore nella chiamata lseek\n");
    return -1;
    } 
  if(write(disk.fd, src, BLOCK_SIZE) == -1){
    printf("Errore nella chiamata read\n");
    return -1;
  }
  return BitMap_set(&(disk.bitmap), block_num, 1);
}

// frees a block in position block_num, and alters the bitmap accordingly
// returns -1 if operation not possible
int DiskDriver_freeBlock(DiskDriver* disk, int block_num){
  if(lseek(disk.fd, block_num*BLOCK_SIZE, SEEK_SET) < 0){
    printf("Errore nella chiamata lseek\n");
    return -1;
    }
    DiskDriver_writeBlock(disk.fd, 0, block_num);
    return BitMap_set(&(disk.bitmap),block_num, 0);

}

// returns the first free block in the disk from position (checking the bitmap)
int DiskDriver_getFreeBlock(DiskDriver* disk, int start){
  return BitMap_get(&(disk.bitmap),start, 0);
  
}

// writes the data (flushing the mmaps)
int DiskDriver_flush(DiskDriver* disk){
  return -1;
}
