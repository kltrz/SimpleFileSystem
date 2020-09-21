#pragma once
#include "simplefs.h"

#define MAX_FD_BLOCK (BLOCK_SIZE-sizeof(BlockHeader)-sizeof(FileControlBlock)-sizeof(int)-sizeof(int))/sizeof(int)
#define MAX_D_BLOCK (BLOCK_SIZE-sizeof(BlockHeader)-sizeof(int))/sizeof(int)
#define MAX_FFB (BLOCK_SIZE-sizeof(FileControlBlock) - sizeof(BlockHeader) - sizeof(int))
#define MAX_FB (BLOCK_SIZE-sizeof(BlockHeader)-sizeof(int))


// initializes a file system on an already made disk
// returns a handle to the top level directory stored in the first block
DirectoryHandle* SimpleFS_init(SimpleFS* fs, DiskDriver* disk){

  fs->disk = (DiskDriver *) malloc(sizeof(DiskDriver));
  fs->firstdb = (FirstDirectoryBlock *) malloc(sizeof(FirstDirectoryBlock));

  fs->disk = disk;
  fs->firstdb = fs->disk->bitmap_data + (BLOCK_SIZE*2);

  DirectoryHandle *d = (DirectoryHandle *) malloc(sizeof(DirectoryHandle));

  d->sfs = fs;
  d->dcb = fs->disk->bitmap_data + (BLOCK_SIZE*2);
  d->directory = NULL;
  d->current_block = (&(d->dcb->header));
  d->pos_in_dir = d->dcb->fcb.block_in_disk;; //Aggiornare
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

  //system("gnome-terminal");
  //aggiungere free delle malloc

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

  fs->firstdb->num_entries = 0;


  fs->firstdb->available = MAX_FD_BLOCK;


  //aggiungere controllo
  int res = DiskDriver_writeBlock(fs->disk, fs->firstdb, freeblock);
  printf("Scrittura corretta?: %d\n", res); //stampa di controllo

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


  int occupied_blocks = MAX_FD_BLOCK - dsource->available;


    //Controllo nel primo blocco della directory



    for(int i = 0; i < occupied_blocks ; i++) { //d->dcb->num_entries //Dsource (BLOCK_SIZE-sizeof(BlockHeader))/sizeof(int)
      if((dsource->file_blocks[i]) != 0){
        filedest = d->sfs->disk->bitmap_data + (BLOCK_SIZE*dsource->file_blocks[i]);
        if((strcmp((filedest->fcb.name), filename)) == 0) {
          printf("\nERRORE, IL FILE GIÀ ESISTE\n\n");
          return NULL;
        }
      }
  }

  //Cerco nei rimanendi blocchi della directory
  if(remaining_blocks > 1) {
    d->pos_in_dir = dsource->header.next_block;

    DirectoryBlock* bsource = (DirectoryBlock *) malloc(sizeof(DirectoryBlock));
    bsource = d->sfs->disk->bitmap_data + (BLOCK_SIZE*d->pos_in_dir);

    do{
      remaining_blocks--;

      occupied_blocks = MAX_D_BLOCK - bsource->available;

      for(int i = 0; i < occupied_blocks ; i++) { //d->dcb->num_entries //Dsource (BLOCK_SIZE-sizeof(BlockHeader))/sizeof(int)
        if((bsource->file_blocks[i]) != 0){
          filedest = d->sfs->disk->bitmap_data + (BLOCK_SIZE*bsource->file_blocks[i]);
          if((strcmp((filedest->fcb.name), filename)) == 0) {
            printf("\nERRORE, IL FILE GIÀ ESISTE\n\n");
            return NULL;
          }
        }

    }
    if((bsource->header.next_block) != -1) {
    d->pos_in_dir = bsource->header.next_block;
    bsource = d->sfs->disk->bitmap_data + (BLOCK_SIZE*d->pos_in_dir);
  }
  } while(remaining_blocks > 1);
  }


  //Riempio File (vuoto)
  FirstFileBlock *f = ( FirstFileBlock *) malloc(sizeof(FirstFileBlock));

  f->header.previous_block = -1;
  f->header.next_block = -1;
  f->header.block_in_file = 0;

  f->available = MAX_FFB;

  f->fcb.directory_block = d->dcb->fcb.block_in_disk;

  strcpy(f->fcb.name, filename);

  f->fcb.size_in_bytes = 0;

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
  printf("Scrittura corretta?: %d\n", res); //stampa di controllo


}
//Controllo se sono liberi i blocchi successivi al primo
else {
      if(d->pos_in_dir != d->dcb->fcb.block_in_disk) {

        DirectoryBlock* bsource = (DirectoryBlock *) malloc(sizeof(DirectoryBlock));
        bsource =  d->sfs->disk->bitmap_data + (BLOCK_SIZE * d->pos_in_dir);


        if(bsource->available > 0) {

          int freeblock = DiskDriver_getFreeBlock(d->sfs->disk, 0);
          f->fcb.block_in_disk = freeblock;

          occupied_blocks = MAX_D_BLOCK - bsource->available;

          bsource->file_blocks[occupied_blocks] = freeblock;

          d->dcb->num_entries++;

          bsource->available--;

          //Scrivo sul disco
          int res = DiskDriver_writeBlock(d->sfs->disk, f, freeblock);
          printf("Scrittura corretta?: %d\n", res); //stampa di controllo
        }

        else {

          if(d->sfs->disk->header->free_blocks < 2){
          printf("Errore: non ci sono blocci liberi nel disco\n");
          return NULL;
        }

          int freeblock = DiskDriver_getFreeBlock(d->sfs->disk, 0);
          DirectoryBlock *bsource = (DirectoryBlock *) malloc(sizeof(DirectoryBlock));
          bsource = d->sfs->disk->bitmap_data + (BLOCK_SIZE * d->pos_in_dir);
          bsource->header.next_block = freeblock;



          DirectoryBlock* dir = (DirectoryBlock *) malloc(sizeof(DirectoryBlock));


          //Header del nuovo blocco per la directory
          dir->header.previous_block = d->pos_in_dir;

          dir->header.next_block = -1;

          dir->header.block_in_file = dsource->header.block_in_file + 1;


          //Aggiorno lista del nuovo blocco della directory
          int blockforfile =  DiskDriver_getFreeBlock(d->sfs->disk, freeblock+1);

          f->fcb.block_in_disk = blockforfile;


          dir->file_blocks[0] = blockforfile;
          dir->available = MAX_D_BLOCK;


          dir->available = (dir->available) - 1;


          d->dcb->num_entries++;


          d->dcb->fcb.size_in_blocks = d->dcb->fcb.size_in_blocks + 1;


          d->dcb->fcb.size_in_bytes = d->dcb->fcb.size_in_bytes + BLOCK_SIZE;

          int res = DiskDriver_writeBlock(d->sfs->disk, dir, freeblock);
          printf("Scrittura corretta del blocco nuovo della directory?: %d\n", res); //stampa di controllo


          res = DiskDriver_writeBlock(d->sfs->disk, f, blockforfile);
          printf("Scrittura corretta del file: %d\n", res); //stampa di controllo


        }
      }
      else {
       //Aggiungo nuovo blocco alla directory
         if(d->sfs->disk->header->free_blocks < 2){
            printf("Errore: non ci sono blocci liberi nel disco\n");
            return NULL;
          }

          int freeblock = DiskDriver_getFreeBlock(d->sfs->disk, 0);

          FirstDirectoryBlock *bsource = (FirstDirectoryBlock *) malloc(sizeof(FirstDirectoryBlock));
          bsource = d->sfs->disk->bitmap_data + (BLOCK_SIZE*d->dcb->fcb.block_in_disk);
          bsource->header.next_block = freeblock;

          DirectoryBlock* dir = (DirectoryBlock *) malloc(sizeof(DirectoryBlock));

          //Header del nuovo blocco per la directory
          dir->header.previous_block = d->pos_in_dir;

          dir->header.next_block = -1;

          dir->header.block_in_file = dsource->header.block_in_file + 1;


          //Aggiorno lista del nuovo blocco della directory
          int blockforfile =  DiskDriver_getFreeBlock(d->sfs->disk, freeblock+1);

          f->fcb.block_in_disk = blockforfile;


          dir->file_blocks[0] = blockforfile;
          dir->available = MAX_D_BLOCK;


          dir->available = (dir->available) - 1;


          d->dcb->num_entries++;


          d->dcb->fcb.size_in_blocks = d->dcb->fcb.size_in_blocks + 1;


          d->dcb->fcb.size_in_bytes = d->dcb->fcb.size_in_bytes + BLOCK_SIZE;


          int res = DiskDriver_writeBlock(d->sfs->disk, dir, freeblock);
          printf("Scrittura corretta del blocco nuovo della directory?: %d\n", res); //stampa di controllo


          res = DiskDriver_writeBlock(d->sfs->disk, f, blockforfile);
          printf("Scrittura corretta del file: %d\n", res); //stampa di controllo


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


int num_of_entries = d->dcb->num_entries;

FirstDirectoryBlock *filedest = (FirstDirectoryBlock *) malloc(sizeof(FirstDirectoryBlock));
FirstDirectoryBlock *next_dest = (FirstDirectoryBlock *) malloc(sizeof(FirstDirectoryBlock));

next_dest = d->sfs->disk->bitmap_data + (BLOCK_SIZE*d->dcb->fcb.block_in_disk);

int remaining_blocks = d->dcb->fcb.size_in_blocks;


int file_index = MAX_FD_BLOCK - next_dest->available;


d->pos_in_dir = next_dest->header.next_block;

int counter = 0;


  for(int i = 0; i < file_index ; i++){
    if(( next_dest->file_blocks[i]) != 0){
      filedest = d->sfs->disk->bitmap_data + (BLOCK_SIZE * next_dest->file_blocks[i]);
      names[counter] = filedest->fcb.name;
      counter++;

    }
}

if(remaining_blocks > 1) {

  DirectoryBlock* bsource = (DirectoryBlock *) malloc(sizeof(DirectoryBlock));
  bsource = d->sfs->disk->bitmap_data + (BLOCK_SIZE*d->pos_in_dir);

  do{
    remaining_blocks--;

    file_index = MAX_D_BLOCK - bsource->available;

    for(int i = 0; i < file_index ; i++) {
      if((bsource->file_blocks[i]) != 0){
        filedest = d->sfs->disk->bitmap_data + (BLOCK_SIZE*bsource->file_blocks[i]);
        names[counter] = filedest->fcb.name;
        counter++;
      }
    }

    if((bsource->header.next_block) != -1) {
      d->pos_in_dir = bsource->header.next_block;
      bsource = d->sfs->disk->bitmap_data + (BLOCK_SIZE*d->pos_in_dir);
    }
  } while(remaining_blocks > 1);
}

}


// opens a file in the  directory d. The file should be exisiting
FileHandle* SimpleFS_openFile(DirectoryHandle* d, const char* filename){


  //Controllo se il file già esiste

  FirstDirectoryBlock *filedest = (FirstDirectoryBlock *) malloc(sizeof(FirstDirectoryBlock));
  FirstDirectoryBlock *dsource = (FirstDirectoryBlock *) malloc(sizeof(FirstDirectoryBlock));

  dsource = d->sfs->disk->bitmap_data + (BLOCK_SIZE*d->dcb->fcb.block_in_disk);

  int remaining_blocks = d->dcb->fcb.size_in_blocks;

  int occupied_blocks = MAX_FD_BLOCK - dsource->available;

  //Riempio FileHandle
  FileHandle *fh = (FileHandle *) malloc(sizeof(FileHandle));
  fh->sfs = d->sfs;
  fh->directory = d->sfs->disk->bitmap_data + (BLOCK_SIZE*d->dcb->fcb.block_in_disk);

  int blocco = -1;

    //Controllo nel primo blocco della directory

    for(int i = 0; i < occupied_blocks ; i++) { //d->dcb->num_entries //Dsource (BLOCK_SIZE-sizeof(BlockHeader))/sizeof(int)
      if((dsource->file_blocks[i]) != 0){
        filedest = d->sfs->disk->bitmap_data + (BLOCK_SIZE*dsource->file_blocks[i]);
        if((strcmp((filedest->fcb.name), filename)) == 0) {
          printf("\nFILE TROVATO\n\n");
          blocco = dsource->file_blocks[i];
          fh->fcb = d->sfs->disk->bitmap_data + (BLOCK_SIZE*blocco);
          fh->current_block = &(fh->fcb->header);
          fh->pos_in_file = fh->fcb->header.block_in_file;
          return fh;
        }
      }
  }

  //Cerco nei rimanendi blocchi della directory

  if(remaining_blocks > 1) {

    d->pos_in_dir = dsource->header.next_block;

    DirectoryBlock* bsource = (DirectoryBlock *) malloc(sizeof(DirectoryBlock));
    bsource = d->sfs->disk->bitmap_data + (BLOCK_SIZE*d->pos_in_dir);

    do{

      remaining_blocks--;

      occupied_blocks = MAX_D_BLOCK - bsource->available;

      for(int i = 0; i < occupied_blocks ; i++) {
        if((bsource->file_blocks[i]) != 0){
          filedest = d->sfs->disk->bitmap_data + (BLOCK_SIZE*bsource->file_blocks[i]);
          if((strcmp((filedest->fcb.name), filename)) == 0) {
            printf("\nFILE TROVATO\n\n");
            blocco = bsource->file_blocks[i];
            fh->fcb = d->sfs->disk->bitmap_data + (BLOCK_SIZE*blocco);
            fh->current_block = &(fh->fcb->header);
            fh->pos_in_file = fh->fcb->header.block_in_file;
            return fh;
          }
        }
      }

      if((bsource->header.next_block) != -1) {

        d->pos_in_dir = bsource->header.next_block;
        bsource = d->sfs->disk->bitmap_data + (BLOCK_SIZE*d->pos_in_dir);
      }
    } while(remaining_blocks > 1);
  }

  return NULL;
}


// closes a file handle (destroyes it)
int SimpleFS_close(FileHandle* f){

}

// writes in the file, at current position for size bytes stored in data
// overwriting and allocating new space if necessary
// returns the number of bytes written
int SimpleFS_write(FileHandle* f, void* data, int size){

  //Controllo se devo scrivere nel primo blocco del file o in uno successivo
  if(f->current_block->block_in_file == 0){

    FirstFileBlock *first = (FirstFileBlock *) malloc(sizeof(FirstFileBlock));

    first = f->sfs->disk->bitmap_data + (BLOCK_SIZE*f->fcb->fcb.block_in_disk);
    int available = first->available;

    int current_block_in_disk = f->fcb->fcb.block_in_disk;

    int bytes_written = size;

    int data_counter = 0;

    int off_set = f->pos_in_file;

    int i = 0;

    int limit = available < bytes_written? available:bytes_written;

    int bytes_return = 0;

    char *data_to_write = data;

    int has_next = f->current_block->next_block;

    for(i = 0; i < limit; i++){
      if(first->data[off_set] == '\0'){
        f->fcb->fcb.size_in_bytes++;
      }
      first->data[off_set] = data_to_write[data_counter];
      available--;
      first->available--;
      bytes_written--;
      data_counter++;
      f->pos_in_file++;
      bytes_return++;
      off_set++;
    }

    if(bytes_written > 0 && available == 0){

        while(bytes_written > 0){

          if(has_next != -1){

            FileBlock *file = (FileBlock *) malloc(sizeof(FileBlock));
            file = f->sfs->disk->bitmap_data + (BLOCK_SIZE*has_next);

            current_block_in_disk = has_next;

            f->current_block = (&(file->header));

            has_next = f->current_block->next_block;

            available = file->available;

            off_set = 0;

            limit = available < bytes_written? available:bytes_written;

              for(i = off_set; i < limit; i++){
                if (file->data[off_set] == '\0') {
                    f->fcb->fcb.size_in_bytes++;
                }
              file->data[off_set] = data_to_write[data_counter];
              available--;
              file->available--;
              bytes_written--;
              data_counter++;
              f->pos_in_file++;
              bytes_return++;
            }
          }
          else {
            //Aggiungo nuovo blocco al file
            if(f->sfs->disk->header->free_blocks == 0){
              printf("Errore: non ci sono blocchi liberi nel disco, impossibile continuare a leggere il file\n");
              return NULL;
            }

            int freeblock = DiskDriver_getFreeBlock(f->sfs->disk, 0);

            FileBlock *file = (FileBlock *) malloc(sizeof(FileBlock));

            f->current_block->next_block = freeblock;
            f->fcb->fcb.size_in_blocks++;


            file->header.previous_block = current_block_in_disk;
            file->header.next_block = -1;
            file->header.block_in_file = f->current_block->block_in_file + 1;

            file->available = MAX_FB;

            current_block_in_disk = freeblock;

            available = file->available;



            int res = DiskDriver_writeBlock(f->sfs->disk, file, freeblock);
            printf("Scrittura corretta del file: %d\n", res); //stampa di controllo

            file = f->sfs->disk->bitmap_data + (BLOCK_SIZE*current_block_in_disk);

            f->current_block = (&(file->header));

            has_next = f->current_block->next_block;

            limit = available < bytes_written? available:bytes_written;

            off_set = 0;

              for(i = 0; i < limit; i++){
              file->data[off_set] = data_to_write[data_counter];
              available--;
              file->available--;
              bytes_written--;
              data_counter++;
              f->pos_in_file++;
              bytes_return++;
              off_set++;
              f->fcb->fcb.size_in_bytes++;
            }
          }
        }
        return bytes_return;
      }
  }
  //Non sono nel primo blocco del file
  else {

    FileBlock *file = (FileBlock *) malloc(sizeof(FileBlock));

    file = f->sfs->disk->bitmap_data + (BLOCK_SIZE*f->current_block->previous_block);

    int current_block_in_disk = file->header.next_block;

    file = f->sfs->disk->bitmap_data + (BLOCK_SIZE*file->header.next_block);


    int remaining_blocks = f->fcb->fcb.size_in_blocks - 2;

    int off_set = f->pos_in_file - MAX_FFB - (MAX_FB*remaining_blocks);


    int available = file->available;

    int bytes_written = size;

    int data_counter = 0;

    int i = 0;

    int limit = available < bytes_written? available:bytes_written;

    int bytes_return = 0;

    char *data_to_write = data;

    int has_next = f->current_block->next_block;

    for(i = 0; i < limit; i++){
      if(file->data[off_set] == '\0'){
        f->fcb->fcb.size_in_bytes++;
      }
      file->data[off_set] = data_to_write[data_counter];
      available--;
      file->available--;
      bytes_written--;
      data_counter++;
      f->pos_in_file++;
      bytes_return++;
      off_set++;
    }

    if(bytes_written > 0 && available == 0){

        while(bytes_written > 0){

          if(has_next != -1){

            FileBlock *file = (FileBlock *) malloc(sizeof(FileBlock));
            file = f->sfs->disk->bitmap_data + (BLOCK_SIZE*has_next);

            current_block_in_disk = has_next;

            f->current_block = (&(file->header));

            has_next = f->current_block->next_block;

            available = file->available;

            off_set = 0;

            limit = available < bytes_written? available:bytes_written;

              for(i = 0; i < limit; i++){
                if(file->data[off_set] == '\0'){
                  f->fcb->fcb.size_in_bytes++;
                }
              file->data[off_set] = data_to_write[data_counter];
              available--;
              file->available--;
              bytes_written--;
              data_counter++;
              f->pos_in_file++;
              bytes_return++;
              off_set++;
            }
          }
          else {
            //Aggiungo nuovo blocco al file
            if(f->sfs->disk->header->free_blocks == 0){
              printf("Errore: non ci sono blocchi liberi nel disco, impossibile continuare a leggere il file\n");
              return NULL;
            }

            int freeblock = DiskDriver_getFreeBlock(f->sfs->disk, 0);

            FileBlock *file = (FileBlock *) malloc(sizeof(FileBlock));

            f->current_block->next_block = freeblock;
            f->fcb->fcb.size_in_blocks++;


            file->header.previous_block = current_block_in_disk;
            file->header.next_block = -1;
            file->header.block_in_file = f->current_block->block_in_file + 1;

            file->available = MAX_FB;

            current_block_in_disk = freeblock;

            available = file->available;



            int res = DiskDriver_writeBlock(f->sfs->disk, file, freeblock);
            printf("Scrittura corretta del file: %d\n", res); //stampa di controllo

            file = f->sfs->disk->bitmap_data + (BLOCK_SIZE*current_block_in_disk);

            f->current_block = (&(file->header));

            has_next = f->current_block->next_block;

            limit = available < bytes_written? available:bytes_written;

            off_set = 0;

              for(i = 0; i < limit; i++){
              file->data[off_set] = data_to_write[data_counter];
              available--;
              file->available--;
              bytes_written--;
              data_counter++;
              f->pos_in_file++;
              bytes_return++;
              off_set++;
              f->fcb->fcb.size_in_bytes++;
            }
          }
        }

        return bytes_return;
      }


  }

}

// writes in the file, at current position size bytes stored in data
// overwriting and allocating new space if necessary
// returns the number of bytes read
int SimpleFS_read(FileHandle* f, void* data, int size) {

  //Controllo se devo scrivere nel primo blocco del file o in uno successivo
  if(f->current_block->block_in_file == 0){

    FirstFileBlock *first = (FirstFileBlock *) malloc(sizeof(FirstFileBlock));

    first = f->sfs->disk->bitmap_data + (BLOCK_SIZE*f->fcb->fcb.block_in_disk);
    int occupied_blocks = MAX_FFB - first->available;

    int current_block_in_disk = f->fcb->fcb.block_in_disk;

    int bytes_read = size;

    int data_counter = 0;

    int off_set = f->pos_in_file;

    int i = 0;

    int limit = occupied_blocks < bytes_read? occupied_blocks:bytes_read;

    int bytes_return = 0;

    char *data_to_write = (char *) data; //TOCCA VEDERE COME FARLO FUNZIONARE

    int has_next = f->current_block->next_block;

    for(i = 0; i < limit; i++){
      data_to_write[data_counter] = first->data[off_set];
      bytes_read--;
      data_counter++;
      f->pos_in_file++;
      bytes_return++;
      off_set++;
      occupied_blocks--;
    }

    puts(data_to_write);

    if(bytes_read > 0 && occupied_blocks == 0){

        while(bytes_read > 0){

          if(has_next != -1){

            FileBlock *file = (FileBlock *) malloc(sizeof(FileBlock));
            file = f->sfs->disk->bitmap_data + (BLOCK_SIZE*has_next);

            current_block_in_disk = has_next;

            f->current_block = (&(file->header));

            has_next = f->current_block->next_block;

            occupied_blocks = MAX_FB - file->available;

            off_set = 0;

            limit = occupied_blocks < bytes_read? occupied_blocks:bytes_read;

              for(i = off_set; i < limit; i++){
              data_to_write[data_counter] = file->data[off_set];
              bytes_read--;
              data_counter++;
              f->pos_in_file++;
              bytes_return++;
              occupied_blocks--;
              off_set++;
            }
          }
        }
        return bytes_return;
      }
    }
    else {

      FileBlock *file = (FileBlock *) malloc(sizeof(FileBlock));

      file = f->sfs->disk->bitmap_data + (BLOCK_SIZE*f->current_block->previous_block);

      int current_block_in_disk = file->header.next_block;

      file = f->sfs->disk->bitmap_data + (BLOCK_SIZE*file->header.next_block);

      int remaining_blocks = f->fcb->fcb.size_in_blocks - 2;

      int off_set = f->pos_in_file - MAX_FFB - (MAX_FB*remaining_blocks);


      int occupied_blocks = MAX_FB - file->available;

      int bytes_read = size;

      int data_counter = 0;

      int i = 0;

      int limit = occupied_blocks < bytes_read? occupied_blocks:bytes_read;

      int bytes_return = 0;

      char *data_to_write =(char *) data;

      int has_next = f->current_block->next_block;

      for(i = 0; i < limit; i++){
        data_to_write[data_counter] = file->data[off_set];
        bytes_read--;
        data_counter++;
        f->pos_in_file++;
        bytes_return++;
        off_set++;
      }

      if(bytes_read > 0 && occupied_blocks == 0){

        while(bytes_read > 0){

          if(has_next != -1){

              FileBlock *file = (FileBlock *) malloc(sizeof(FileBlock));
              file = f->sfs->disk->bitmap_data + (BLOCK_SIZE*has_next);

              current_block_in_disk = has_next;

              f->current_block = (&(file->header));

              has_next = f->current_block->next_block;

              occupied_blocks = MAX_FB - file->available;

              off_set = 0;

              limit = occupied_blocks < bytes_read? occupied_blocks:bytes_read;

                for(i = 0; i < limit; i++){
                data_to_write[data_counter] = file->data[off_set];
                bytes_read--;
                data_counter++;
                f->pos_in_file++;
                bytes_return++;
                off_set++;
              }
            }
          }

          return bytes_return;
        }
      }
    }

// returns the number of bytes read (moving the current pointer to pos)
// returns pos on success
// -1 on error (file too short)
int SimpleFS_seek(FileHandle* f, int pos){

  //PRIMERO COMPRUEBO SI EL FILE ES TOO SHORT
  if(f->fcb->fcb.size_in_bytes < pos) {
    printf("ERRORE: FUORI GLI INDICI DEL FILE\n");
    return -1;
  }

  int test = pos - MAX_FFB;

  if(test < 0){
    //Sono nel primo blocco

    f->current_block = (&(f->fcb->header));
    f->pos_in_file = pos;
  }
  else {
    int blocco = test / MAX_FB;

    int off_set = test % MAX_FB;

    f->pos_in_file = MAX_FFB + (blocco*MAX_FB) + off_set;

    blocco++;

    FileBlock *file = (FileBlock *) malloc(sizeof(FileBlock));

    int next = f->fcb->header.next_block;

    while(blocco > 0){
      file = f->sfs->disk->bitmap_data + (BLOCK_SIZE*next);
      f->current_block = (&(file->header));
      next = f->current_block->next_block;
      blocco--;
    }

  }
  //DIVIDO pos E CERCO off_set. MI SPOSTO PER IL NUMERO DI BLOCCHI E pos_in_file SARA off_set
  return pos;
}

// seeks for a directory in d. If dirname is equal to ".." it goes one level up
// 0 on success, negative value on error
// it does side effect on the provided handle
 int SimpleFS_changeDir(DirectoryHandle* d, char* dirname){

//Salgo alla parent directory
   if(dirname == ".."){
     //Verifico se la parent è la top-level
     if(d->directory == NULL){
       printf("ERRORE: TOP-LEVEL DIRECTORY RAGGIUNTO\n");
       return -1;
     }
     else{
       int up = d->directory->fcb.directory_block;
       d->dcb = d->sfs->disk->bitmap_data + (BLOCK_SIZE*d->directory->fcb.block_in_disk);
       if(up != NULL){
         d->directory = d->sfs->disk->bitmap_data + (BLOCK_SIZE*up);
       }
       else{
         d->directory = NULL;
       }
     }
   }
//Cerco se esiste la directory
   else{

     FirstDirectoryBlock *filedest = (FirstDirectoryBlock *) malloc(sizeof(FirstDirectoryBlock));
     FirstDirectoryBlock *dsource = (FirstDirectoryBlock *) malloc(sizeof(FirstDirectoryBlock));

     dsource = d->sfs->disk->bitmap_data + (BLOCK_SIZE*d->dcb->fcb.block_in_disk);

     int remaining_blocks = d->dcb->fcb.size_in_blocks;

     int occupied_blocks = MAX_FD_BLOCK - dsource->available;

     int blocco = -1;

       //Controllo nel primo blocco della directory

       for(int i = 0; i < occupied_blocks ; i++) { //d->dcb->num_entries //Dsource (BLOCK_SIZE-sizeof(BlockHeader))/sizeof(int)
         if((dsource->file_blocks[i]) != 0){
           filedest = d->sfs->disk->bitmap_data + (BLOCK_SIZE*dsource->file_blocks[i]);
           puts(filedest->fcb.name);
           if((strcmp((filedest->fcb.name), dirname)) == 0) {
             printf("\nDIRECTORY TROVATA\n\n");
             blocco = dsource->file_blocks[i];
             d->dcb = d->sfs->disk->bitmap_data + (BLOCK_SIZE*blocco);
             int parent = d->dcb->fcb.directory_block;
             if(parent != -1){
               d->directory = d->sfs->disk->bitmap_data + (BLOCK_SIZE*parent);
             }
             else{
               d->directory = NULL;
             }
             d->current_block = (&(d->dcb->header));
             //AGGIORNARE POS IN DIR E IN BLOCK
             return 0;
           }
         }
     }

     //Cerco nei rimanendi blocchi della directory
     if(remaining_blocks > 1) {

       d->pos_in_dir = dsource->header.next_block;

       DirectoryBlock* bsource = (DirectoryBlock *) malloc(sizeof(DirectoryBlock));
       bsource = d->sfs->disk->bitmap_data + (BLOCK_SIZE*d->pos_in_dir);

       do{
         remaining_blocks--;

         occupied_blocks = MAX_D_BLOCK - bsource->available;
         for(int i = 0; i < occupied_blocks ; i++) {
           if((bsource->file_blocks[i]) != 0){
             filedest = d->sfs->disk->bitmap_data + (BLOCK_SIZE*bsource->file_blocks[i]);
             puts(filedest->fcb.name);
             if((strcmp((filedest->fcb.name), dirname)) == 0) {
               printf("\nDIRECTORY TROVATA\n\n");
               blocco = bsource->file_blocks[i];
               d->dcb = d->sfs->disk->bitmap_data + (BLOCK_SIZE*blocco);
               int parent = d->dcb->fcb.directory_block;
               if(parent != -1){
                 d->directory = d->sfs->disk->bitmap_data + (BLOCK_SIZE*parent);
               }
               else{
                 d->directory = NULL;
               }
               d->current_block = (&(d->dcb->header));
               //AGGIORNARE POS IN DIR E IN BLOCK
               return 0;
             }
           }
         }
         if((bsource->header.next_block) != -1) {
           d->pos_in_dir = bsource->header.next_block;
           bsource = d->sfs->disk->bitmap_data + (BLOCK_SIZE*d->pos_in_dir);
         }
       } while(remaining_blocks > 1);
     }
     return NULL;
   }
 }

// creates a new directory in the current one (stored in fs->current_directory_block)
// 0 on success
// -1 on error
int SimpleFS_mkDir(DirectoryHandle* d, char* dirname){
  //Controllo se ci sono blocchi liberi
  if(d->sfs->disk->header->free_blocks == 0){
    printf("Errore: non ci sono blocci liberi nel disco\n");
    return -1;
  }

  if(d->dcb->available > 0){

    int index = MAX_FD_BLOCK - d->dcb->available;

    FirstDirectoryBlock *nd = (FirstDirectoryBlock *) malloc(sizeof(FirstDirectoryBlock));
    //HEADER
    nd->header.previous_block = -1;
    nd->header.next_block = -1;
    nd->header.block_in_file = 0;

    //FCB
    nd->fcb.directory_block = d->dcb->fcb.block_in_disk;

    int freeblock = DiskDriver_getFreeBlock(d->sfs->disk, 0);
    nd->fcb.block_in_disk = freeblock;

    strcpy(nd->fcb.name, dirname);

    nd->fcb.size_in_bytes = 0;

    nd->fcb.size_in_blocks = BLOCK_SIZE;

    nd->fcb.is_dir = 1;

    //NUM_ENTRIES
    nd->num_entries = 0;

    //AVALAIBLE
    nd->available = MAX_FD_BLOCK;


    //AGGIORNO dcb
    //FCB...

    //NUM_ENTRIES
    d->dcb->num_entries++;

    //AVALAIBLE
    d->dcb->available--;

    //FILE BLOCKS
    d->dcb->file_blocks[index] = freeblock;

    int res = DiskDriver_writeBlock(d->sfs->disk, nd,freeblock);
    printf("SCRITTURA CORRETTA?: %d\n", freeblock);

    return 0;


  }
  //IL PRIMO BLOCCO DELLA DIRECTORY NON HA SPAZIO -> VEDO I PROSSIMI
  else{

    int has_next = d->current_block->next_block;
    int last = has_next;

    //ESISTE UN BLOCCO DELLA DIRECTORY
    if(has_next != -1){

      DirectoryBlock *db = (DirectoryBlock *) malloc(sizeof(DirectoryBlock));

      while(has_next != -1){

        db = d->sfs->disk->bitmap_data + (BLOCK_SIZE*has_next);

        d->current_block = (&(db->header));
        has_next = d->current_block->next_block;

        if(has_next != -1){
          last = has_next;
        }

      }

      //L'ULTIMO BLOCCO DELLA DIRECTORY HA SPAZIO LIBERO
      if(db->available > 0){

        int index = MAX_D_BLOCK - db->available;

        FirstDirectoryBlock *nd = (FirstDirectoryBlock *) malloc(sizeof(FirstDirectoryBlock));
        //HEADER
        nd->header.previous_block = -1;
        nd->header.next_block = -1;
        nd->header.block_in_file = 0;

        //FCB
        nd->fcb.directory_block = d->dcb->fcb.block_in_disk;

        int freeblock = DiskDriver_getFreeBlock(d->sfs->disk, 0);
        nd->fcb.block_in_disk = freeblock;

        strcpy(nd->fcb.name, dirname);

        nd->fcb.size_in_bytes = 0;

        nd->fcb.size_in_blocks = BLOCK_SIZE;

        nd->fcb.is_dir = 1;

        //NUM_ENTRIES
        nd->num_entries = 0;

        //AVALAIBLE
        nd->available = MAX_FD_BLOCK;


        //AGGIORNO dcb
        //FCB...

        //NUM_ENTRIES
        d->dcb->num_entries++;

        //AVALAIBLE
        db->available--;

        //FILE BLOCKS
        db->file_blocks[index] = freeblock;

        int res = DiskDriver_writeBlock(d->sfs->disk, nd,freeblock);
        printf("SCRITTURA CORRETTA?: %d\n", freeblock);

        return 0;
      }

      //L'ULTIMO BLOCCO DELLA DIRECTORY NON HA SPAZIO -> CREO NUOVO BLOCCO E POI LA NUOVA DIRECTORY
      else{

        if(d->sfs->disk->header->free_blocks < 2){
          printf("ERRORE: NON CI SONO BLOCCHI LIBERI SUFFICIENTI NEL DISCO\n");
          return -1;
        }

        int blockford = DiskDriver_getFreeBlock(d->sfs->disk, 0);
        int freeblock = DiskDriver_getFreeBlock(d->sfs->disk, blockford+1);

        DirectoryBlock *ndb = (DirectoryBlock *) malloc(sizeof(DirectoryBlock));

        DirectoryBlock *prev = (DirectoryBlock *) malloc(sizeof(DirectoryBlock));

        //HEADER
        ndb->header.previous_block = last;
        ndb->header.next_block = -1;
        ndb->header.block_in_file = d->current_block->block_in_file + 1;

        //AVALAIBLE
        ndb->available = MAX_D_BLOCK - 1;

        //FILE BLOCKS
        ndb->file_blocks[0] = freeblock;

        int res = DiskDriver_writeBlock(d->sfs->disk, ndb, blockford);
        printf("SCRITTURA CORRETTA?: %d\n", blockford);

        ndb = d->sfs->disk->bitmap_data + (BLOCK_SIZE*blockford);

        prev = d->sfs->disk->bitmap_data + (BLOCK_SIZE*last);

        prev->header.next_block = blockford;


        d->current_block = (&(ndb->header));

        FirstDirectoryBlock *nd = (FirstDirectoryBlock *) malloc(sizeof(FirstDirectoryBlock));
        //HEADER
        nd->header.previous_block = -1;
        nd->header.next_block = -1;
        nd->header.block_in_file = 0;

        //FCB
        nd->fcb.directory_block = d->dcb->fcb.block_in_disk;

        nd->fcb.block_in_disk = freeblock;

        strcpy(nd->fcb.name, dirname);

        nd->fcb.size_in_bytes = 0;

        nd->fcb.size_in_blocks = BLOCK_SIZE;

        nd->fcb.is_dir = 1;

        //NUM_ENTRIES
        nd->num_entries = 0;

        //AVALAIBLE
        nd->available = MAX_FD_BLOCK;


        //AGGIORNO dcb
        //FCB...

        //NUM_ENTRIES
        d->dcb->num_entries++;

        res = DiskDriver_writeBlock(d->sfs->disk, nd,freeblock);
        printf("SCRITTURA CORRETTA?: %d\n", freeblock);

        return 0;
      }
    }

    //NON ESISTE UN PROSSIMO BLOCCO DELLA DIRECTORY -> CREO NUOVO BLOCCO E POI CREO LA NUOVA DIRECTORY
    else {

      if(d->sfs->disk->header->free_blocks < 2){
        printf("ERRORE: NON CI SONO BLOCCHI LIBERI SUFFICIENTI NEL DISCO\n");
        return -1;
      }

      int blockford = DiskDriver_getFreeBlock(d->sfs->disk, 0);
      int freeblock = DiskDriver_getFreeBlock(d->sfs->disk, blockford+1);

      DirectoryBlock *ndb = (DirectoryBlock *) malloc(sizeof(DirectoryBlock));

      //HEADER
      ndb->header.previous_block = d->dcb->fcb.block_in_disk;
      ndb->header.next_block = -1;
      ndb->header.block_in_file = d->current_block->block_in_file + 1;

      //AVALAIBLE
      ndb->available = MAX_D_BLOCK - 1;

      //FILE BLOCKS
      ndb->file_blocks[0] = freeblock;

      int res = DiskDriver_writeBlock(d->sfs->disk, ndb, blockford);
      printf("SCRITTURA CORRETTA?: %d\n", blockford);

      ndb = d->sfs->disk->bitmap_data + (BLOCK_SIZE*blockford);

      d->dcb->header.next_block = blockford;

      d->current_block = (&(ndb->header));

      FirstDirectoryBlock *nd = (FirstDirectoryBlock *) malloc(sizeof(FirstDirectoryBlock));
      //HEADER
      nd->header.previous_block = -1;
      nd->header.next_block = -1;
      nd->header.block_in_file = 0;

      //FCB
      nd->fcb.directory_block = d->dcb->fcb.block_in_disk;

      nd->fcb.block_in_disk = freeblock;

      strcpy(nd->fcb.name, dirname);

      nd->fcb.size_in_bytes = 0;

      nd->fcb.size_in_blocks = BLOCK_SIZE;

      nd->fcb.is_dir = 1;

      //NUM_ENTRIES
      nd->num_entries = 0;

      //AVALAIBLE
      nd->available = MAX_FD_BLOCK;


      //AGGIORNO dcb
      //FCB...

      //NUM_ENTRIES
      d->dcb->num_entries++;

      res = DiskDriver_writeBlock(d->sfs->disk, nd,freeblock);
      printf("SCRITTURA CORRETTA?: %d\n", freeblock);

      return 0;

    }
  }

}

// removes the file in the current directory
// returns -1 on failure 0 on success
// if a directory, it removes recursively all contained files
int SimpleFS_remove(SimpleFS* fs, char* filename){


  int has_next;
  while (has_next != -1) {
  /* code */
  //next = file->next;
  //freebloc (has_next)
  //has_next = next;
}
}
