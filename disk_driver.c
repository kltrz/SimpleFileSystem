#pragma once
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <sys/mman.h>
#include <assert.h>
#include "disk_driver.h"


#define BLOCK_SIZE 512


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
	disk->fd = open(filename,O_CREAT | O_EXCL | O_RDWR, 0666);
	if(disk->fd == -1){
		    fprintf(stderr, "Value of errno: %d\n", errno);
        fprintf(stderr, "Error opening file: %s\n", strerror(errno));
        if(errno == 17){
          puts("\nDisco già esistente. Leggendo dati del disco\n");
        }

        //Leggo i dati del disco e riempio il Header e la Bitmap
        printf("Aprendo il disco in lettura/scrittura\n");
        disk->fd = open(filename,O_RDWR, 0666);

        int current = lseek(disk->fd, 0L, SEEK_END);
        printf("Dimensioni del disco già esistente: %d bytes\n", current);

        current = lseek(disk->fd, 0L, SEEK_SET);


        disk->header = (DiskHeader *) calloc(1, sizeof(DiskHeader));


        disk->header = (DiskHeader*) mmap(NULL, BLOCK_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, disk->fd, 0);
         if(disk->header == MAP_FAILED){
        fprintf(stderr, "Value of errno: %d\n", errno);
        fprintf(stderr, "Error mapping header: %s\n", strerror(errno));
    }

        printf("Blocchi del disco: %d\n", disk->header->num_blocks);
        printf("Blocchi della BitMap: %d\n", disk->header->bitmap_blocks);
        printf("Bytes necessari per la BitMap: %d\n", disk->header->bitmap_entries);
        printf("Blocchi liberi: %d\n", disk->header->free_blocks);
        printf("Primo bloccho libero: %d\n", disk->header->first_free_block);

        disk->bitmap_data = (char*) calloc(disk->header->num_blocks, sizeof(char));
        disk->bitmap_data = (char*) mmap(NULL, num_blocks*BLOCK_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, disk->fd, 0);
        if(disk->bitmap_data == MAP_FAILED){
        fprintf(stderr, "Value of errno: %d\n", errno);
        fprintf(stderr, "Error mapping BitMap_data: %s\n", strerror(errno));
    }



        disk->bitmap.entries = (char*) calloc(disk->header->num_blocks, sizeof(char));
        disk->bitmap.entries = disk->bitmap_data + 512;
        disk->bitmap.num_bits = num_blocks;




        current = lseek(disk->fd, 0L, SEEK_CUR);
        if(current == -1){
          fprintf(stderr, "Value of errno: %d\n", errno);
        fprintf(stderr, "Error opening file: %s\n", strerror(errno));


        }

        puts("\nLettura del disco completata\n");


	}
	//Riempio nuovo Header e Bitmap
  else{
  puts("\nNuovo disco: procedendo alla creazione\n");
  if(ftruncate(disk->fd, num_blocks*BLOCK_SIZE) == -1){
    printf("Errore nella chiamata ftruncate\n");
    exit(1);
  }
  int current = lseek(disk->fd, 0L, SEEK_END);
  printf("Dimensione del disco: %d bytes, %d blocchi\n ",current, num_blocks);

  current = lseek(disk->fd, 0L, SEEK_SET);





  //Creazione e riempimento del Header
  puts("\n|---Creazione del Header---|\n");

  disk->header = (DiskHeader *) calloc(1, sizeof(DiskHeader));

  DiskHeader h;
	h.num_blocks = num_blocks;
  if(num_blocks < 9){
    h.bitmap_blocks = 1;
  }
  else{
    int bitsadd = num_blocks%8;
    if(bitsadd > 0){
      h.bitmap_blocks = (num_blocks/8)+1;
    }
    else h.bitmap_blocks = num_blocks/8;
  }

  int bmblocks;

  if((h.bitmap_blocks/512) == 0){
  	bmblocks = 1;
  }
  else{
  	if((h.bitmap_blocks%512) > 0){
  		bmblocks = (h.bitmap_blocks/512) + 1;
  	}
  	else bmblocks = (h.bitmap_blocks/512);
  }

	h.bitmap_entries = h.bitmap_blocks*sizeof(char);
	h.free_blocks = num_blocks-(1+bmblocks);
	h.first_free_block = 1+bmblocks;
	strcpy(h.filename, filename);

  printf("Blocchi del disco: %d\n", h.num_blocks);
  printf("Blocchi della BitMap: %d\n", h.bitmap_blocks);
  printf("Bytes necessari per la BitMap: %d\n", h.bitmap_entries);
  printf("Blocchi liberi: %d\n", h.free_blocks);
  printf("Primo bloccho libero: %d\n", h.first_free_block);


  puts("\n|---Header riempito---|\n");


   //Creazione e riempimento della BitMap
  puts("\n|---Creazione BitMap---|\n");

  BitMap b;
  b.num_bits = num_blocks;
  b.entries = (char*) calloc(h.bitmap_blocks, sizeof(char));
  disk->bitmap_data = (char*) calloc(h.bitmap_blocks, sizeof(char));

  printf("Numero di bits nella Bitmap: %d\n", b.num_bits);
  printf("Numero di blocchi allocati nella Bitmap: %d\n", h.bitmap_blocks);


  puts("\n|---BitMap riempito---|\n");

  //Scrittura nel disco del Header e della Bitmap


  int sizewrite = write(disk->fd, &h, BLOCK_SIZE); //Scrittura del Header in un blocco di 512 bytes

  sizewrite = write(disk->fd,b.entries , bmblocks*BLOCK_SIZE);  //Scrittura della BitMap_data in 'bmblocks' blocchi di 512 bytes


  current = lseek(disk->fd, 0L, SEEK_SET);


  disk->header = (DiskHeader*) mmap(NULL, BLOCK_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, disk->fd, 0);

  if(disk->header == MAP_FAILED){
        fprintf(stderr, "Value of errno: %d\n", errno);
        fprintf(stderr, "Error mapping header: %s\n", strerror(errno));
  }

  disk->bitmap_data = (char*) mmap(NULL, num_blocks*BLOCK_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, disk->fd, 0);
  if(disk->bitmap_data == MAP_FAILED){
        fprintf(stderr, "Value of errno: %d\n", errno);
        fprintf(stderr, "Error mapping BitMap_data: %s\n", strerror(errno));
  }

  disk->bitmap.entries = disk->bitmap_data + 512;
  disk->bitmap.num_bits = num_blocks;





//Scrivo nella Bitmap i blocchi già occupati dal Header e della bitmap_data
  int res;
  for(int i = 0; i < bmblocks+1; i++){
  	res = BitMap_set(&(disk->bitmap), i, 1);
  }

  res = DiskDriver_flush(disk);


}
}



// reads the block in position block_num
// returns -1 if the block is free accrding to the bitmap
// 0 otherwise
int DiskDriver_readBlock(DiskDriver* disk, void* dest, int block_num){
  if(block_num > disk->header->num_blocks){
    printf("Errore: indice oltre il limite del disco\n");
    return -1;
  }
  BitMapEntryKey key = BitMap_blockToIndex(block_num);
  char bitsofchar[8];
  char c = disk->bitmap.entries[key.entry_num];
  chartobin(c, bitsofchar);
  if(bitsofchar[key.bit_num] == '0'){
    printf("Errore, il blocco richiesto è libero\n");
    return -1;
  }


  if(lseek(disk->fd, block_num*BLOCK_SIZE, SEEK_SET) < 0){
    printf("Errore della chiamata lseek nella funzione di lettura\n");
    exit(1);
    }
  if(read(disk->fd, dest, BLOCK_SIZE) == -1){
    printf("Errore nella chiamata di lettura\n");
    exit(1);
  }
  return 0;
}

// writes a block in position block_num, and alters the bitmap accordingly
// returns -1 if operation not possible
int DiskDriver_writeBlock(DiskDriver* disk, void* src, int block_num){
	if(block_num > disk->header->num_blocks){
    printf("Errore: indice oltre il limite del disco\n");
    return -1;
  }
	if(DiskDriver_getFreeBlock(disk, block_num) == block_num){
		  disk->header->free_blocks--;
	}
  if(lseek(disk->fd, block_num*BLOCK_SIZE, SEEK_SET) < 0){
    printf("Errore della chiamata lseek nella funzione di scrittura\n");
    return -1;
    }
  if(write(disk->fd, src, BLOCK_SIZE) == -1){
    printf("Errore nella chiamata di scrittura\n");
    return -1;
  }

  int res = BitMap_set(&(disk->bitmap), block_num, 1);
  disk->header->first_free_block = DiskDriver_getFreeBlock(disk, 0);
  DiskDriver_flush(disk);
  return res;
}

// frees a block in position block_num, and alters the bitmap accordingly
// returns -1 if operation not possible
int DiskDriver_freeBlock(DiskDriver* disk, int block_num){
  if(block_num > disk->header->num_blocks){
    printf("Errore: indice oltre il limite del disco\n");
    return -1;
  }
  if(lseek(disk->fd, block_num*BLOCK_SIZE, SEEK_SET) < 0){
    printf("Errore della chiamata lseek nella funzione di liberazione blocco\n");
    return -1;
    }
  char *null = (char *) calloc(512, sizeof(char));
  DiskDriver_writeBlock(disk, null, block_num);

  int res = BitMap_set(&(disk->bitmap),block_num, 0);
  disk->header->free_blocks++;
  disk->header->first_free_block = DiskDriver_getFreeBlock(disk, 0);
  DiskDriver_flush(disk);
  return res;

}

// returns the first free block in the disk from position (checking the bitmap)
int DiskDriver_getFreeBlock(DiskDriver* disk, int start){
	if(start > disk->header->num_blocks){
    printf("Errore: indice oltre il limite del disco\n");
    return -1;
  }
  return BitMap_get(&(disk->bitmap),start, 0);
}

// writes the data (flushing the mmaps)
int DiskDriver_flush(DiskDriver* disk){

	if(msync(disk->header, BLOCK_SIZE, MS_SYNC) == -1){
        perror("Errore della chiamata sync del header");
        return -1;
    }
    if(msync(disk->bitmap_data,disk->header->num_blocks*BLOCK_SIZE, MS_SYNC) == -1){
        perror("Errore della chiamata sync della bitmap");
        return -1;
    }
  return 1;
}
