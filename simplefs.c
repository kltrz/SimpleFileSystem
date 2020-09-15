#pragma once
#include "disk_driver.c"

/*these are structures stored on disk*/

// header, occupies the first portion of each block in the disk
// represents a chained list of blocks
typedef struct {
  int previous_block; // chained list (previous block)
  int next_block;     // chained list (next_block)
  int block_in_file; // position in the file, if 0 we have a file control block
} BlockHeader;


// this is in the first block of a chain, after the header
typedef struct {
  int directory_block; // first block of the parent directory
  int block_in_disk;   // repeated position of the block on the disk
  char name[128];
  int  size_in_bytes;
  int size_in_blocks;
  int is_dir;          // 0 for file, 1 for dir
} FileControlBlock;

// this is the first physical block of a file
// it has a header
// an FCB storing file infos
// and can contain some data

/******************* stuff on disk BEGIN *******************/
typedef struct {
  BlockHeader header;
  FileControlBlock fcb;
  char data[BLOCK_SIZE-sizeof(FileControlBlock) - sizeof(BlockHeader)] ;
} FirstFileBlock;

// this is one of the next physical blocks of a file
typedef struct {
  BlockHeader header;
  char  data[BLOCK_SIZE-sizeof(BlockHeader)];
} FileBlock;

// this is the first physical block of a directory
typedef struct {
  BlockHeader header;
  FileControlBlock fcb;
  int num_entries;
  int available; //aggiungere numero di entries avalaible
  int file_blocks[ (BLOCK_SIZE
		   -sizeof(BlockHeader)
		   -sizeof(FileControlBlock)
		    -sizeof(int)-sizeof(int))/sizeof(int) ];
} FirstDirectoryBlock;

// this is remainder block of a directory
typedef struct {
  BlockHeader header;
  int file_blocks[ (BLOCK_SIZE-sizeof(BlockHeader)-sizeof(int))/sizeof(int) ];
  int available; //aggiungere numero di entries avalaible
} DirectoryBlock;
/******************* stuff on disk END *******************/





typedef struct {
  DiskDriver* disk;
  FirstDirectoryBlock *firstdb;
} SimpleFS;

// this is a file handle, used to refer to open files
typedef struct {
  SimpleFS* sfs;                   // pointer to memory file system structure
  FirstFileBlock* fcb;             // pointer to the first block of the file(read it)
  FirstDirectoryBlock* directory;  // pointer to the directory where the file is stored
  BlockHeader* current_block;      // current block in the file
  int pos_in_file;                 // position of the cursor
} FileHandle;

typedef struct {
  SimpleFS* sfs;                   // pointer to memory file system structure
  FirstDirectoryBlock* dcb;        // pointer to the first block of the directory(read it)
  FirstDirectoryBlock* directory;  // pointer to the parent directory (null if top level)
  BlockHeader* current_block;      // current block in the directory
  int pos_in_dir;                  // absolute position of the cursor in the directory
  int pos_in_block;                // relative position of the cursor in the block
} DirectoryHandle;

// initializes a file system on an already made disk
// returns a handle to the top level directory stored in the first block
DirectoryHandle* SimpleFS_init(SimpleFS* fs, DiskDriver* disk){

  fs->disk = (DiskDriver *) malloc(sizeof(DiskDriver));
  fs->firstdb = (FirstDirectoryBlock *) malloc(sizeof(FirstDirectoryBlock));

  fs->disk = disk;
  fs->firstdb = fs->disk->bitmap_data + (BLOCK_SIZE*2);

  DirectoryHandle *d = (DirectoryHandle *) malloc(sizeof(DirectoryHandle));

  d->sfs = fs;
  d->dcb = fs->firstdb;
  d->directory = NULL;
  d->current_block = d->dcb->fcb.block_in_disk;
  d->pos_in_dir = 0; //Aggiornare
  d->pos_in_block = 0; //Aggiornare



  return d;

}

// creates the inital structures, the top level directory
// has name "/" and its control block is in the first position
// it also clears the bitmap of occupied blocks on the disk
// the current_directory_block is cached in the SimpleFS struct
// and set to the top level directory
void SimpleFS_format(SimpleFS* fs){

  if(remove(fs->disk->header->filename) == -1){
    printf("Errore nella chiamata remove\n");
    fprintf(stderr, "Value of errno: %d\n", errno);
    fprintf(stderr, "Error closing file: %s\n", strerror(errno));
  }


  //aggiungere free delle malloc

  //Formatazione Disco
  DiskDriver_init(fs->disk, fs->disk->header->filename, fs->disk->header->num_blocks);


  fs->firstdb = (FirstDirectoryBlock *) malloc(sizeof(FirstDirectoryBlock));


  //Riempio header del FirstDirectoryBlock

  fs->firstdb->header.previous_block = -1;
  fs->firstdb->header.next_block = -1;
  fs->firstdb->header.block_in_file = 0;

  //Riempio FCB del FirstDirectoryBlock

  fs->firstdb->fcb.directory_block = NULL;

  int freeblock = DiskDriver_getFreeBlock(fs->disk, 0);

  fs->firstdb->fcb.block_in_disk = freeblock;


  strcpy(fs->firstdb->fcb.name, "/");


  fs->firstdb->fcb.size_in_bytes = BLOCK_SIZE;
  fs->firstdb->fcb.size_in_blocks = 1;
  fs->firstdb->fcb.is_dir = 1;

  //FirstDirectoryBlock del File System fs riempito

  fs->firstdb->num_entries = 0;

  fs->firstdb->available = (BLOCK_SIZE-sizeof(BlockHeader)-sizeof(FileControlBlock)-sizeof(int)-sizeof(int))/sizeof(int);


  //Scrivo sul disco

  //aggiungere controllo?
  int res = DiskDriver_writeBlock(fs->disk, fs->firstdb, freeblock);

}

// creates an empty file in the directory d
// returns null on error (file existing, no free blocks)
// an empty file consists only of a block of type FirstBlock
FileHandle* SimpleFS_createFile(DirectoryHandle* d, const char* filename) {


  //Controllo di blocchi liberi nel disco
  if(d->sfs->disk->header->free_blocks == 0){
    printf("Errore: non ci sono blocci liberi nel disco\n");
    return NULL;
  }



  //Controllo se il file già esiste

  FirstDirectoryBlock *filedest = (FirstDirectoryBlock *) malloc(sizeof(FirstDirectoryBlock));
  FirstDirectoryBlock *dsource = (FirstDirectoryBlock *) malloc(sizeof(FirstDirectoryBlock));

  dsource = d->sfs->disk->bitmap_data + (BLOCK_SIZE*d->dcb->fcb.block_in_disk);

  int remaining_blocks = d->dcb->fcb.size_in_blocks;

  int occupied_blocks = ((BLOCK_SIZE-sizeof(BlockHeader)-sizeof(FileControlBlock)-sizeof(int)-sizeof(int))/sizeof(int)) - dsource->available;

  do{
    remaining_blocks--;

    for(int i = 0; i < occupied_blocks ; i++){ //d->dcb->num_entries //Dsource (BLOCK_SIZE-sizeof(BlockHeader))/sizeof(int)
      if((dsource->file_blocks[i]) != 0){
        filedest = d->sfs->disk->bitmap_data + (BLOCK_SIZE*dsource->file_blocks[i]);
        if((strcmp((filedest->fcb.name), filename)) == 0) {
          printf("\nERRORE, IL FILE GIÀ ESISTE\n\n");
          return NULL;
        }
      }
  }

  if(remaining_blocks != 0) {
    dsource = d->sfs->disk->bitmap_data + (BLOCK_SIZE*dsource->header.next_block);
    }
  }
  while(remaining_blocks > 0);


  //Riempio File nuovo (vuoto)
  FirstFileBlock *f = ( FirstFileBlock *) malloc(sizeof(FirstFileBlock));

  f->header.previous_block = -1;
  f->header.next_block = -1;
  f->header.block_in_file = 0;

  f->fcb.directory_block = d->dcb->fcb.block_in_disk;

  strcpy(f->fcb.name, filename);

  f->fcb.size_in_bytes = BLOCK_SIZE;

  f->fcb.size_in_blocks = 1;

  f->fcb.is_dir = 0;


  //Aggiorno la lista nella directory;

  //Controllo se serve un nuovo blocco nella directory
  if(d->dcb->available > 0) {

  int freeblock = DiskDriver_getFreeBlock(d->sfs->disk, 0);
  f->fcb.block_in_disk = freeblock;

  d->dcb->file_blocks[occupied_blocks] = freeblock;

  d->dcb->num_entries++;

  d->dcb->available--;

  //Scrivo sul disco
  int res = DiskDriver_writeBlock(d->sfs->disk, f, freeblock);

} else { //Controllo se sono liberi i blocchi successivi al primo

    remaining_blocks = d->dcb->fcb.size_in_blocks - 1;

    dsource = d->sfs->disk->bitmap_data + (BLOCK_SIZE*d->dcb->fcb.block_in_disk);

    int current;
    while(remaining_blocks > 0) {

      if(dsource->header.next_block =! -1) {
        current = dsource->header.next_block;
        dsource = d->sfs->disk->bitmap_data + (BLOCK_SIZE*dsource->header.next_block);
    }
      remaining_blocks--;
    }

    if(dsource->available > 0) {

      int freeblock = DiskDriver_getFreeBlock(d->sfs->disk, 0);
      f->fcb.block_in_disk = freeblock;

      occupied_blocks = ((BLOCK_SIZE-sizeof(BlockHeader)-sizeof(int))/sizeof(int)) - dsource->available;

      dsource->file_blocks[occupied_blocks] = freeblock;

      d->dcb->num_entries++;

      dsource->available--;

      //Scrivo sul disco
      int res = DiskDriver_writeBlock(d->sfs->disk, f, freeblock);
    }
    else { //Aggiungo nuovo blocco alla directory
      if(d->sfs->disk->header->free_blocks < 2){
        printf("Errore: non ci sono blocci liberi nel disco\n");
        return NULL;
      }
      DirectoryBlock* dir = (DirectoryBlock *) malloc(sizeof(DirectoryBlock));

      int freeblock = DiskDriver_getFreeBlock(d->sfs->disk, 0);

      //Header del nuovo blocco per la directory
      dir->header.previous_block = current;
      dir->header.next_block = -1;
      dir->header.block_in_file = dsource->header.block_in_file++;

      dsource->header.next_block = freeblock;

        //Aggiorno lista del nuovo blocco della directory
      int blockforfile =  DiskDriver_getFreeBlock(d->sfs->disk, freeblock+1);

      f->fcb.block_in_disk = blockforfile;

      dir->file_blocks[0] = blockforfile;
      dir->available = ((BLOCK_SIZE-sizeof(BlockHeader)-sizeof(int))/sizeof(int)) - 1;

      d->dcb->num_entries++;

      int res = DiskDriver_writeBlock(d->sfs->disk, dir, freeblock);

      res = DiskDriver_writeBlock(d->sfs->disk, f, blockforfile);
    }
}

  //Riempio FileHandle
  FileHandle *fh = (FileHandle *) malloc(sizeof(FileHandle));
  fh->sfs = d->sfs;
  fh->fcb = d->sfs->disk->bitmap_data + (BLOCK_SIZE*f->fcb.block_in_disk);
  fh->directory = d->sfs->disk->bitmap_data + (BLOCK_SIZE*d->dcb->fcb.block_in_disk);
  fh->current_block = &(f->header);
  fh->pos_in_file = f->header.block_in_file;


  return fh;
}

// reads in the (preallocated) blocks array, the name of all files in a directory
int SimpleFS_readDir(char** names, DirectoryHandle* d){
//LEGGERE QUANTE ENTRIES CI SONO IN 'd' E ALLOCARE LO SPAZIO

}


// opens a file in the  directory d. The file should be exisiting
FileHandle* SimpleFS_openFile(DirectoryHandle* d, const char* filename){

}


// closes a file handle (destroyes it)
int SimpleFS_close(FileHandle* f){

}

// writes in the file, at current position for size bytes stored in data
// overwriting and allocating new space if necessary
// returns the number of bytes written
int SimpleFS_write(FileHandle* f, void* data, int size){

}

// writes in the file, at current position size bytes stored in data
// overwriting and allocating new space if necessary
// returns the number of bytes read
int SimpleFS_read(FileHandle* f, void* data, int size){

}

// returns the number of bytes read (moving the current pointer to pos)
// returns pos on success
// -1 on error (file too short)
int SimpleFS_seek(FileHandle* f, int pos){

}

// seeks for a directory in d. If dirname is equal to ".." it goes one level up
// 0 on success, negative value on error
// it does side effect on the provided handle
 int SimpleFS_changeDir(DirectoryHandle* d, char* dirname){

 }

// creates a new directory in the current one (stored in fs->current_directory_block)
// 0 on success
// -1 on error
int SimpleFS_mkDir(DirectoryHandle* d, char* dirname){

}

// removes the file in the current directory
// returns -1 on failure 0 on success
// if a directory, it removes recursively all contained files
int SimpleFS_remove(SimpleFS* fs, char* filename){

}
