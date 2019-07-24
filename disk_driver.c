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

char *null = (char *) calloc(512, sizeof(char)); 
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
        printf("Dimensioni del disco: %d bytes\n", current);

        current = lseek(disk->fd, 0L, SEEK_SET);
        printf("SEEK_SET a 0?: %d\n\n", current);


        int *sizeh = (int *) calloc(1, sizeof(int));
        int *numbits = (int *) calloc(1, sizeof(int));
        int *sizebmdata = (int *) calloc(1, sizeof(int));
        int *bmblocks = (int *) calloc(1, sizeof(int));

        disk->header = (DiskHeader*) calloc(1, sizeof(DiskHeader));


        int sizeread = read(disk->fd, sizeh, 4);
        printf("Bytes letti per la dimensione del header: %d\n", sizeread);

        sizeread = read(disk->fd, disk->header, *sizeh);
        printf("Bytes letti per riempire il header: %d\n\n", sizeread);


        sizeread = read(disk->fd, numbits, 4);
        printf("Bytes letti per il numero di bits: %d\n", sizeread);

        disk->bitmap.num_bits =(*numbits);
        printf("Numero di bits inserito nella BitMap: %d\n\n", disk->bitmap.num_bits);

        sizeread = read(disk->fd, bmblocks, 4);
        printf("Bytes letti per il numero di blocchi: %d\n", sizeread);

        printf("Blocchi per la bitmap_data necessari: %d\n\n", *bmblocks);

        disk->bitmap.entries = (char*) calloc(*bmblocks, sizeof(char));
        disk->bitmap_data = (char*) calloc(*bmblocks, sizeof(char));

        sizeread = read(disk->fd, sizebmdata, 4);
        printf("Bytes letti per la dimensione della bitmap_data: %d\n", sizeread);

        sizeread = read(disk->fd, disk->bitmap.entries, *sizebmdata);
        if(sizeread == -1){
        fprintf(stderr, "Value of errno: %d\n", errno);
        fprintf(stderr, "Error opening file: %s\n", strerror(errno));
        }
        disk->bitmap_data = disk->bitmap.entries;
        printf("Bytes letti per riempire la bitmap_data: %d\n", sizeread);

        printf("Blocchi del disco nel Header: %d\n", disk->header->num_blocks);
        printf("Blocchi della BitMap nel Header: %d\n", disk->header->bitmap_blocks);
        printf("Bytes necessari per la BitMap nel Header: %d\n", disk->header->bitmap_entries);
        printf("Blocchi liberi nel Header: %d\n", disk->header->free_blocks);
        printf("Primo bloccho libero nel Header: %d\n", disk->header->first_free_block);

        current = lseek(disk->fd, 0L, SEEK_CUR);
        if(current == -1){
          fprintf(stderr, "Value of errno: %d\n", errno);
        fprintf(stderr, "Error opening file: %s\n", strerror(errno));


        }
        printf("\nCurrent position: %d\n",current);

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
  printf("Dimensione del disco (truncate): %d bytes\n ", current);

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
  
  h.bitmap_entries = h.bitmap_blocks*sizeof(char);
  h.free_blocks = num_blocks-1;
  h.first_free_block = 1;

  printf("Blocchi del disco nel Header: %d\n", h.num_blocks);
  printf("Blocchi della BitMap nel Header: %d\n", h.bitmap_blocks);
  printf("Bytes necessari per la BitMap nel Header: %d\n", h.bitmap_entries);
  printf("Blocchi liberi nel Header: %d\n", h.free_blocks);
  printf("Primo bloccho libero nel Header: %d\n", h.first_free_block);


  puts("\n|---Header riempito---|\n");

  /*puts("SIZE OF INT pointer");
  int *kk;
  int y = 5;
  kk = &y;
  printf("SIZE OF INT: %ld\n POINTER", sizeof(kk));
  printf("SIZE OF INT: %ld\n", sizeof(y));
  printf("ESTE %d\n", *kk);
  printf("ESTE2 %d\n", y);
*/
  //Creazione e riempimento della BitMap
  puts("\n|---Creazione BitMap---|\n");
  
  BitMap b;
  b.num_bits = num_blocks;
  b.entries = (char*) calloc(h.bitmap_blocks, sizeof(char));
  //disk->bitmap_data = b.entries;

  printf("Numero di bits nella Bitmap: %d\n", b.num_bits);
  printf("Numero di blocchi allocati nella Bitmap: %d\n", h.bitmap_blocks);


  puts("\n|---BitMap riempito---|\n");
  
  //disk->header = &h;
  //disk->bitmap = b;

  int sizeh = sizeof(h); //Dimensione del Header
  int sizeb = sizeof(b); //Dimensione della BitMap
  int sizebd = sizeof(b.entries); //Dimensione della BitMap_data
  int numbits = b.num_bits;
  int blocks = h.bitmap_entries;

  printf("Dimensione del header h: %d\n", sizeh);
  printf("Dimensione della Bitmap b: %d\n", sizeb);
  printf("Dimensione della BitMap_data: %d\n", sizebd);

  //BitMap_set(&(disk->bitmap), 0, 1);

  //Scrittura nel disco del Header e della Bitmap
  puts("\n|---Scrivendo Header e BitMap nel disco---|\n");

  //int sizewrite = write(disk->fd, &sizeh, 4); //Scrittua della dimensione del Header
  
  int sizewrite = write(disk->fd, &h, sizeh); //Scrittura del Header

  current = lseek(disk->fd, 0L, SEEK_CUR);
  puts("Scrittura del Header completata");
  printf("Current position dopo scrittura sizeheader e header :  %d\n\n",current);

  sizewrite = write(disk->fd, &numbits, 4); //Scrittura del numero di bits
  sizewrite = write(disk->fd, &blocks, 4); //Scrittura del numero di blocchi
  sizewrite = write(disk->fd, &sizebd, 4); //Scrtittura della dimensione della BitMap_data

  current = lseek(disk->fd, 0L, SEEK_CUR);
  puts("Scrittura del numero di bits, numero di blocchi e dimensione della BitMap_data completata");
  printf("Current position dopo scrittura dei numero di bits, numero di blocchi e della dimensione della BitMap_data:  %d\n\n",current);

  sizewrite = write(disk->fd,b.entries , sizebd);  //Scrittura della BitMap_data

  current = lseek(disk->fd, 0L, SEEK_CUR);
  puts("Scrittura della BitMap_data completata");
  printf("Current position dopo scrittura di BitMap data:  %d\n\n",current);


  current = lseek(disk->fd, 0L, SEEK_END);
  printf("Dimensione del disco:  %d\n",current);

  puts("\nFUERDA DEL DISKDRIVER\n");

  current = lseek(disk->fd, 0L, SEEK_SET);
  printf("Set a %d per procedere alla mmap\n",current);

  puts("\nGET PAGE SIZE\n");
  int pz = getpagesize();
  printf("Page size: %d\n", pz);

  char *file_memory = (char*) mmap(NULL, num_blocks*BLOCK_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, disk->fd, 0);

  disk->header = (DiskHeader*) mmap(NULL, sizeh, PROT_READ|PROT_WRITE, MAP_SHARED, disk->fd, 0);

  if(disk->header == MAP_FAILED){
        fprintf(stderr, "Value of errno: %d\n", errno);
        fprintf(stderr, "Error mapping header: %s\n", strerror(errno));
  }
  current = lseek(disk->fd, 0L, SEEK_CUR);
  printf("Seek dopo mmap: %d\n", current);

  puts("PRIMA TEST");
  printf("Test con Header numero di blocchi: %d\n", disk->header->num_blocks );
  printf("Test con Header blocchi della bitmap: %d\n", disk->header->bitmap_blocks );
  printf("Test con Header blocchi della entries: %d\n", disk->header->bitmap_entries);
  printf("Test con Header blocchi liberi: %d\n", disk->header->free_blocks );
  printf("Test con Header primo blocco libero: %d\n", disk->header->first_free_block);

  //b.entries = file_memory + (sizeof(char)*36);
  strncpy( b.entries, file_memory, sizebd);

  /*b.entries = (char*) mmap(NULL, sizebd, PROT_READ|PROT_WRITE, MAP_SHARED, disk->fd, 2);
  if(b.entries == MAP_FAILED){
        fprintf(stderr, "Value of errno: %d\n", errno);
        fprintf(stderr, "Error mapping BitMap_data: %s\n", strerror(errno));
  }*/


  puts("DOPO MAPPING");

  int lenght = strlen(file_memory);
  for(int i = 0; i<100; i++){
    printf("%c",b.entries[i]);
  }
}
}



// reads the block in position block_num
// returns -1 if the block is free accrding to the bitmap
// 0 otherwise
int DiskDriver_readBlock(DiskDriver* disk, void* dest, int block_num){
  if(block_num > disk->header->num_blocks){
    printf("Errore: indice oltre il limite\n");
    return -1;
  }
  BitMapEntryKey key = BitMap_blockToIndex(block_num);
  char bitsofchar[8];
  char c = disk->bitmap_data[key.entry_num];
  chartobin(c, bitsofchar);
  if(bitsofchar[key.bit_num] == '0'){
    printf("Errore, il blocco richiesto è libero\n");
    return -1;
  }


  if(lseek(disk->fd, block_num*BLOCK_SIZE, SEEK_SET) < 0){
    printf("Errore nella chiamata lseek (readBlock)\n");
    exit(1);
    } 
  if(read(disk->fd, dest, BLOCK_SIZE) == -1){
    printf("Errore nella chiamata read\n");
    exit(1);
  }
  return 0;
}

// writes a block in position block_num, and alters the bitmap accordingly
// returns -1 if operation not possible
int DiskDriver_writeBlock(DiskDriver* disk, void* src, int block_num){
  if(block_num > disk->header->num_blocks){
    printf("Errore: indice oltre il limite\n");
    return -1;
  }
  if(lseek(disk->fd, block_num*BLOCK_SIZE, SEEK_SET) < 0){
    printf("Errore nella chiamata lseek (writeBlock)\n");
    return -1;
    } 
  if(write(disk->fd, src, BLOCK_SIZE) == -1){
    printf("Errore nella chiamata read\n");
    return -1;
  }
  return BitMap_set(&(disk->bitmap), block_num, 1);
}

// frees a block in position block_num, and alters the bitmap accordingly
// returns -1 if operation not possible
int DiskDriver_freeBlock(DiskDriver* disk, int block_num){
  if(block_num > disk->header->num_blocks){
    printf("Errore: indice oltre il limite\n");
    return -1;
  }
  if(lseek(disk->fd, block_num*BLOCK_SIZE, SEEK_SET) < 0){
    printf("Errore nella chiamata lseek\n");
    return -1;
    }
  char *null = (char *) calloc(512, sizeof(char)); 
  DiskDriver_writeBlock(disk, null, block_num);
    return BitMap_set(&(disk->bitmap),block_num, 0);

}

// returns the first free block in the disk from position (checking the bitmap)
int DiskDriver_getFreeBlock(DiskDriver* disk, int start){
  return BitMap_get(&(disk->bitmap),start, 0);
  
}

// writes the data (flushing the mmaps)
int DiskDriver_flush(DiskDriver* disk){
  return -1;
}
