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
  fs->firstdb = (FirstDirectoryBlock *) (fs->disk->bitmap_data + (BLOCK_SIZE*2));
  fs->current_dblock = 2;

  DirectoryHandle *d = (DirectoryHandle *) malloc(sizeof(DirectoryHandle));

  d->sfs = fs;
  d->dcb = (FirstDirectoryBlock *) (fs->disk->bitmap_data + (BLOCK_SIZE*2));
  d->directory = NULL;
  d->current_block = (&(d->dcb->header));
  d->current_block_in_disk = d->dcb->fcb.block_in_disk;
  d->pos_in_block = 0;
  d->pos_in_dir = 0;

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
  fs->current_dblock = freeblock;


  strcpy(fs->firstdb->fcb.name, "/");


  fs->firstdb->fcb.size_in_bytes = BLOCK_SIZE;
  fs->firstdb->fcb.size_in_blocks = 1;
  fs->firstdb->fcb.is_dir = 1;

  fs->firstdb->num_entries = 0;


  fs->firstdb->available = MAX_FD_BLOCK;


  //aggiungere controllo
  if(DiskDriver_writeBlock(fs->disk, fs->firstdb, freeblock) == -1){
    printf("ERRORE: IMPOSSIBILE SCRIVERE LA TOP-LEVEL DIRECTORY\n");
  }

}

// creates an empty file in the directory d
// returns null on error (file existing, no free blocks)
// an empty file consists only of a block of type FirstBlock
FileHandle* SimpleFS_createFile(DirectoryHandle* d, const char* filename) {


  //Controllo di blocchi liberi nel disco
  if(d->sfs->disk->header->free_blocks == 0){
    printf("ERRORE: NON CI SONO BLOCCHI LIBERI NEL DISCO\n");
    return NULL;
  }

  d->current_block = (&(d->dcb->header));
  //Controllo se il file già esiste

  FirstDirectoryBlock *filedest = (FirstDirectoryBlock *) malloc(sizeof(FirstDirectoryBlock));
  FirstDirectoryBlock *dsource = (FirstDirectoryBlock *) malloc(sizeof(FirstDirectoryBlock));

  dsource = (FirstDirectoryBlock *) (d->sfs->disk->bitmap_data + (BLOCK_SIZE*d->dcb->fcb.block_in_disk));


  int remaining_blocks = d->dcb->fcb.size_in_blocks;


  int occupied_blocks = MAX_FD_BLOCK - dsource->available;

  int has_next = d->current_block->next_block;



    //Controllo nel primo blocco della directory




    for(int i = 0; i < MAX_FD_BLOCK; i++) {
      if((dsource->file_blocks[i]) > 0){
        filedest = (FirstDirectoryBlock *) (d->sfs->disk->bitmap_data + (BLOCK_SIZE*dsource->file_blocks[i]));
        if((strcmp((filedest->fcb.name), filename)) == 0) {
          printf("\nERRORE, IL FILE GIÀ ESISTE\n\n");
          return NULL;
        }
      }
  }

  //Cerco nei rimanendi blocchi della directory
  if((has_next != -1) && (remaining_blocks > 1)) {
    DirectoryBlock* bsource = (DirectoryBlock *) malloc(sizeof(DirectoryBlock));
    bsource = (DirectoryBlock *) (d->sfs->disk->bitmap_data + (BLOCK_SIZE*has_next));
    d->current_block = (&(bsource->header));
    d->current_block_in_disk = has_next;
    has_next = d->current_block->next_block;

    do{

      remaining_blocks--;
      occupied_blocks = MAX_D_BLOCK - bsource->available;

      for(int i = 0; i < MAX_D_BLOCK ; i++) {
        if((bsource->file_blocks[i]) > 0){
          filedest = (FirstDirectoryBlock *) (d->sfs->disk->bitmap_data + (BLOCK_SIZE*bsource->file_blocks[i]));
          if((strcmp((filedest->fcb.name), filename)) == 0) {
            printf("\nERRORE, IL FILE GIÀ ESISTE\n\n");
            return NULL;
          }
        }

    }
    if(has_next != -1) {
    d->current_block_in_disk = has_next;
    bsource = (DirectoryBlock *) (d->sfs->disk->bitmap_data + (BLOCK_SIZE*has_next));
    d->current_block = (&(bsource->header));
    has_next = d->current_block->next_block;
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
  if(DiskDriver_writeBlock(d->sfs->disk, f, freeblock) == -1){
    printf("ERRORE: IMPOSSIBILE CREARE FILE\n");
    return -1;
  }


}
//Controllo se sono liberi i blocchi successivi al primo
else {
      if(d->current_block_in_disk > 2) {


        DirectoryBlock* bsource = (DirectoryBlock *) malloc(sizeof(DirectoryBlock));
        bsource = (DirectoryBlock *)  (d->sfs->disk->bitmap_data + (BLOCK_SIZE * d->current_block_in_disk));
        d->current_block = (&(bsource->header));

        if(bsource->available > 0) {

          int freeblock = DiskDriver_getFreeBlock(d->sfs->disk, 0);
          f->fcb.block_in_disk = freeblock;

          occupied_blocks = MAX_D_BLOCK - bsource->available;

          bsource->file_blocks[occupied_blocks] = freeblock;

          d->dcb->num_entries++;

          bsource->available--;

          //Scrivo sul disco
          if(  DiskDriver_writeBlock(d->sfs->disk, f, freeblock) == -1){
            printf("ERRORE: IMPOSSIBILE CREARE FILE\n");
            return -1;
          }

        }

        else {
          if(d->sfs->disk->header->free_blocks < 2){
          printf("ERRORE: NON CI SONO BLOCCHI LIBERI NEL DISCO\n");
          return NULL;
        }

          int freeblock = DiskDriver_getFreeBlock(d->sfs->disk, 0);

          bsource->header.next_block = freeblock;
          d->current_block = (&(bsource->header));


          DirectoryBlock* dir = (DirectoryBlock *) malloc(sizeof(DirectoryBlock));


          //Header del nuovo blocco per la directory
          dir->header.previous_block = has_next;

          dir->header.next_block = -1;

          dir->header.block_in_file = dsource->header.block_in_file + 1;

          dir->available = MAX_D_BLOCK;

          if(DiskDriver_writeBlock(d->sfs->disk, dir, freeblock) == -1){
            printf("ERRORE: IMPOSSIBILE CREARE NUOVO BLOCCO DI DIRECTORY\n");
            return -1;
          }

          dir = (DirectoryBlock *) (d->sfs->disk->bitmap_data + (BLOCK_SIZE * freeblock));

          d->current_block = (&(dir->header));


          //Aggiorno lista del nuovo blocco della directory
          int blockforfile =  DiskDriver_getFreeBlock(d->sfs->disk, 0);

          f->fcb.block_in_disk = blockforfile;

          dir->file_blocks[0] = blockforfile;


          dir->available--;


          d->dcb->num_entries++;


          d->dcb->fcb.size_in_blocks = d->dcb->fcb.size_in_blocks + 1;


          d->dcb->fcb.size_in_bytes = d->dcb->fcb.size_in_bytes + BLOCK_SIZE;



          if(DiskDriver_writeBlock(d->sfs->disk, f, blockforfile) == -1){
            printf("ERRORE: IMPOSSIBILE CREARE FILE\n");
            return -1;
          }

          DiskDriver_flush(d->sfs->disk);


        }
      }
      else {
       //Aggiungo nuovo blocco alla directory
         if(d->sfs->disk->header->free_blocks < 2){
            printf("ERRORE: NON CI SONO BLOCCHI LIBERI NEL DISCO\n");
            return NULL;
          }

          int freeblock = DiskDriver_getFreeBlock(d->sfs->disk, 0);

          FirstDirectoryBlock *bsource = (FirstDirectoryBlock *) malloc(sizeof(FirstDirectoryBlock));
          bsource = (FirstDirectoryBlock *) (d->sfs->disk->bitmap_data + (BLOCK_SIZE*d->dcb->fcb.block_in_disk));
          d->current_block = (&(bsource->header));
          d->current_block->next_block = freeblock;
          d->dcb->header.next_block = freeblock;

          DiskDriver_flush(d->sfs->disk);

          DirectoryBlock* dir = (DirectoryBlock *) malloc(sizeof(DirectoryBlock));

          //Header del nuovo blocco per la directory
          dir->header.previous_block = d->current_block_in_disk;

          dir->header.next_block = -1;

          dir->header.block_in_file = dsource->header.block_in_file + 1;


          //Aggiorno lista del nuovo blocco della directory
          int blockforfile =  DiskDriver_getFreeBlock(d->sfs->disk, freeblock+1);

          f->fcb.block_in_disk = blockforfile;


          dir->file_blocks[0] = blockforfile;
          dir->available = MAX_D_BLOCK;


          dir->available--;


          d->dcb->num_entries++;


          d->dcb->fcb.size_in_blocks = d->dcb->fcb.size_in_blocks + 1;


          d->dcb->fcb.size_in_bytes = d->dcb->fcb.size_in_bytes + BLOCK_SIZE;

          if(DiskDriver_writeBlock(d->sfs->disk, dir, freeblock) == -1){
            printf("ERRORE: IMPOSSIBILE CREARE NUOVO BLOCCO DI DIRECTORY\n");
            return -1;
          }

          if(DiskDriver_writeBlock(d->sfs->disk, f, blockforfile) == -1){
            printf("ERRORE: IMPOSSIBILE CREARE FILE\n");
            return -1;
          }


        }
  }


  //Riempio FileHandle
  FileHandle *fh = (FileHandle *) malloc(sizeof(FileHandle));
  fh->sfs = d->sfs;
  fh->fcb = (FirstFileBlock *) (d->sfs->disk->bitmap_data + (BLOCK_SIZE*f->fcb.block_in_disk));
  fh->directory = (FirstDirectoryBlock *) (d->sfs->disk->bitmap_data + (BLOCK_SIZE*d->dcb->fcb.block_in_disk));
  fh->current_block = &(f->header);
  fh->pos_in_file = 0;


  return fh;






}

// reads in the (preallocated) blocks array, the name of all files in a directory
int SimpleFS_readDir(char** names, DirectoryHandle* d){


int num_of_entries = d->dcb->num_entries;

if(num_of_entries < 1) {
  printf("\n\nERRORE: NON CI SONO FILE DA LEGGERE IN QUESTA DIRECTORY\n");
  return -1;
}

FirstDirectoryBlock *filedest = (FirstDirectoryBlock *) malloc(sizeof(FirstDirectoryBlock));
FirstDirectoryBlock *next_dest = (FirstDirectoryBlock *) malloc(sizeof(FirstDirectoryBlock));

next_dest = (FirstDirectoryBlock *) (d->sfs->disk->bitmap_data + (BLOCK_SIZE*d->dcb->fcb.block_in_disk));

int remaining_blocks = d->dcb->fcb.size_in_blocks;


int file_index = MAX_FD_BLOCK - next_dest->available;


d->current_block_in_disk = next_dest->header.next_block;

int counter = 0;

  for(int i = 0; i < MAX_FD_BLOCK ; i++){
    if(( next_dest->file_blocks[i]) > 0){
      filedest = (FirstDirectoryBlock *) (d->sfs->disk->bitmap_data + (BLOCK_SIZE * next_dest->file_blocks[i]));
      if(filedest->fcb.is_dir == 0) { //IS FILE
        names[counter] = malloc(sizeof(char)*130);
        strcpy(names[counter], "f: ");
        strcat(names[counter], filedest->fcb.name);
        counter++;
      }
      else {
        names[counter] = malloc(sizeof(char)*130);
        strcpy(names[counter], "d: ");
        strcat(names[counter], filedest->fcb.name);
        counter++;
      }

    }
}

if(remaining_blocks > 1) {

  DirectoryBlock* bsource = (DirectoryBlock *) malloc(sizeof(DirectoryBlock));
  bsource = (DirectoryBlock *) (d->sfs->disk->bitmap_data + (BLOCK_SIZE*d->current_block_in_disk));

  do{
    remaining_blocks--;

    file_index = MAX_D_BLOCK - bsource->available;

    for(int i = 0; i < MAX_D_BLOCK ; i++) {
      if((bsource->file_blocks[i]) > 0){
        filedest = (FirstDirectoryBlock *) (d->sfs->disk->bitmap_data + (BLOCK_SIZE*bsource->file_blocks[i]));
        if(filedest->fcb.is_dir == 0) { //IS FILE
          names[counter] = malloc(sizeof(char)*130);
          strcpy(names[counter], "f: ");
          strcat(names[counter], filedest->fcb.name);
          counter++;
        }
        else {
          names[counter] = malloc(sizeof(char)*130);
          strcpy(names[counter], "d: ");
          strcat(names[counter], filedest->fcb.name);
          counter++;
        }
      }
    }

    if((bsource->header.next_block) != -1) {
      d->current_block_in_disk = bsource->header.next_block;
      bsource = (DirectoryBlock *) (d->sfs->disk->bitmap_data + (BLOCK_SIZE*d->current_block_in_disk));
    }
  } while(remaining_blocks > 1);
}

return counter;

}


// opens a file in the  directory d. The file should be exisiting
FileHandle* SimpleFS_openFile(DirectoryHandle* d, const char* filename){


  //Controllo se il file già esiste

  FirstDirectoryBlock *filedest = (FirstDirectoryBlock *) malloc(sizeof(FirstDirectoryBlock));
  FirstDirectoryBlock *dsource = (FirstDirectoryBlock *) malloc(sizeof(FirstDirectoryBlock));

  dsource = (FirstDirectoryBlock *) (d->sfs->disk->bitmap_data + (BLOCK_SIZE*d->dcb->fcb.block_in_disk));

  int remaining_blocks = d->dcb->fcb.size_in_blocks;

  int occupied_blocks = MAX_FD_BLOCK - dsource->available;

  //Riempio FileHandle
  FileHandle *fh = (FileHandle *) malloc(sizeof(FileHandle));
  fh->sfs = d->sfs;
  fh->directory = (FirstDirectoryBlock *) (d->sfs->disk->bitmap_data + (BLOCK_SIZE*d->dcb->fcb.block_in_disk));

  int blocco = -1;

    //Controllo nel primo blocco della directory

    for(int i = 0; i < MAX_FD_BLOCK ; i++) {
      if((dsource->file_blocks[i]) > 0){
        filedest = (FirstDirectoryBlock *) (d->sfs->disk->bitmap_data + (BLOCK_SIZE*dsource->file_blocks[i]));
        if((strcmp((filedest->fcb.name), filename)) == 0) {
          if(filedest->fcb.is_dir == 0){ // IS FILE
          blocco = dsource->file_blocks[i];
          fh->fcb = (FirstFileBlock *) (d->sfs->disk->bitmap_data + (BLOCK_SIZE*blocco));
          fh->current_block = &(fh->fcb->header);
          fh->pos_in_file = 0;
          return fh;
        }
        }
      }
  }

  //Cerco nei rimanendi blocchi della directory

  if(remaining_blocks > 1) {

    d->current_block_in_disk = dsource->header.next_block;

    DirectoryBlock* bsource = (DirectoryBlock *) malloc(sizeof(DirectoryBlock));
    bsource = (DirectoryBlock *) (d->sfs->disk->bitmap_data + (BLOCK_SIZE*d->current_block_in_disk));

    do{

      remaining_blocks--;

      occupied_blocks = MAX_D_BLOCK - bsource->available;

      for(int i = 0; i < MAX_D_BLOCK ; i++) {
        if((bsource->file_blocks[i]) > 0){
          filedest = (FirstDirectoryBlock *) (d->sfs->disk->bitmap_data + (BLOCK_SIZE*bsource->file_blocks[i]));
          if((strcmp((filedest->fcb.name), filename)) == 0) {
            if(filedest->fcb.is_dir == 0){ // IS FILE
            blocco = bsource->file_blocks[i];
            fh->fcb = (FirstFileBlock *) (d->sfs->disk->bitmap_data + (BLOCK_SIZE*blocco));
            fh->current_block = &(fh->fcb->header);
            fh->pos_in_file = 0;
            return fh;
          }
          }
        }
      }

      if((bsource->header.next_block) != -1) {

        d->current_block_in_disk = bsource->header.next_block;
        bsource = (DirectoryBlock *) (d->sfs->disk->bitmap_data + (BLOCK_SIZE*d->current_block_in_disk));
      }
    } while(remaining_blocks > 1);
  }

  return NULL;
}


// closes a file handle (destroyes it)
int SimpleFS_close(FileHandle* f){

  int ret = DiskDriver_flush(f->sfs->disk);
  free(f);
  return 0;
}

// writes in the file, at current position for size bytes stored in data
// overwriting and allocating new space if necessary
// returns the number of bytes written
int SimpleFS_write(FileHandle* f, void* data, int size){

  char *data_to_write = data;

  //Controllo se devo scrivere nel primo blocco del file o in uno successivo
  if(f->current_block->block_in_file == 0){

    FirstFileBlock *first = (FirstFileBlock *) malloc(sizeof(FirstFileBlock));

    first = (FirstFileBlock *) (f->sfs->disk->bitmap_data + (BLOCK_SIZE*f->fcb->fcb.block_in_disk));

    int current_block_in_disk = f->fcb->fcb.block_in_disk;

    int bytes_written = size;

    int data_counter = 0;

    int off_set = f->pos_in_file;

    int i = 0;

    int limit = MAX_FFB < bytes_written? MAX_FFB:bytes_written;

    int bytes_return = 0;

    int has_next = f->current_block->next_block;

    for(i = 0; (i < limit) && (off_set < MAX_FFB); i++){
      if(first->data[off_set] == '\0'){
        f->fcb->fcb.size_in_bytes++;
        first->available--;
      }
      first->data[off_set] = data_to_write[data_counter];
      bytes_written--;
      data_counter++;
      f->pos_in_file++;
      bytes_return++;
      off_set++;
    }

    if(bytes_written > 0 && first->available == 0){

      if(has_next == -1){
        //Aggiungo nuovo blocco al file
        if(f->sfs->disk->header->free_blocks == 0){
          printf("ERRORE: NON CI SONO BLOCCHI LIBERI NEL DISCO, IMPOSSIBILE CONTINUARE A SCRIVERE IL FILE\n");
          return -1;
        }

        int freeblock = DiskDriver_getFreeBlock(f->sfs->disk, 0);

        FileBlock *file = (FileBlock *) malloc(sizeof(FileBlock));

        f->current_block->next_block = freeblock;
        f->fcb->header.next_block = freeblock;
        f->fcb->fcb.size_in_blocks++;




        file->header.previous_block = f->fcb->fcb.block_in_disk;
        file->header.next_block = -1;
        file->header.block_in_file = f->current_block->block_in_file + 1;

        file->available = MAX_FB;

        current_block_in_disk = freeblock;

        int available = file->available;



        if(DiskDriver_writeBlock(f->sfs->disk, file, freeblock) == -1){
          printf("ERRORE: IMPOSSIBILE CREARE FILE\n");
          return -1;
        }

        file = (FileBlock *) (f->sfs->disk->bitmap_data + (BLOCK_SIZE*current_block_in_disk));

        f->current_block = (&(file->header));

        has_next = f->current_block->next_block;

        limit = available < bytes_written? available:bytes_written;


          for(i = 0; i < limit; i++){
          file->data[i] = data_to_write[data_counter];
          available--;
          file->available--;
          bytes_written--;
          data_counter++;
          f->pos_in_file++;
          bytes_return++;
          f->fcb->fcb.size_in_bytes++;
        }
      }

        while(bytes_written > 0){

          if(has_next != -1){
            FileBlock *file = (FileBlock *) malloc(sizeof(FileBlock));
            file = (FileBlock *) (f->sfs->disk->bitmap_data + (BLOCK_SIZE*has_next));

            current_block_in_disk = has_next;

            f->current_block = (&(file->header));

            has_next = f->current_block->next_block;


            off_set = 0;

            limit = bytes_written;

              for(i = 0, off_set = 0; i < limit && off_set < MAX_FB; i++, off_set++){
                if (file->data[off_set] == '\0') {
                    f->fcb->fcb.size_in_bytes++;
                    file->available--;
                }
              file->data[off_set] = data_to_write[data_counter];
              bytes_written--;
              data_counter++;
              f->pos_in_file++;
              bytes_return++;
            }

            DiskDriver_flush(f->sfs->disk);
          }
          else {
            //Aggiungo nuovo blocco al file
            if(f->sfs->disk->header->free_blocks == 0){
              printf("ERRORE: NON CI SONO BLOCCHI LIBERI NEL DISCO, IMPOSSIBILE CONTINUARE A SCRIVERE IL FILE\n");
              return -1;
            }

            int freeblock = DiskDriver_getFreeBlock(f->sfs->disk, 0);

            FileBlock *file = (FileBlock *) malloc(sizeof(FileBlock));
            FileBlock *from = (FileBlock *) malloc(sizeof(FileBlock));


            from = (FileBlock *) (f->sfs->disk->bitmap_data + (BLOCK_SIZE*current_block_in_disk));
            from->header.next_block = freeblock;
            f->current_block->next_block = freeblock;
            f->fcb->fcb.size_in_blocks++;




            file->header.previous_block = current_block_in_disk;
            file->header.next_block = -1;
            file->header.block_in_file = f->current_block->block_in_file + 1;

            file->available = MAX_FB;

            current_block_in_disk = freeblock;

            int available = file->available;



            if(DiskDriver_writeBlock(f->sfs->disk, file, freeblock) == -1){
              printf("ERRORE: IMPOSSIBILE CREARE FILE\n");
              return -1;
            }

            file = (FileBlock *) (f->sfs->disk->bitmap_data + (BLOCK_SIZE*current_block_in_disk));

            f->current_block = (&(file->header));

            has_next = f->current_block->next_block;

            limit = available < bytes_written? available:bytes_written;


              for(i = 0; i < limit; i++){
              file->data[i] = data_to_write[data_counter];
              available--;
              file->available--;
              bytes_written--;
              data_counter++;
              f->pos_in_file++;
              bytes_return++;
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

    file = (FileBlock *) (f->sfs->disk->bitmap_data + (BLOCK_SIZE*f->current_block->previous_block));

    int current_block_in_disk = file->header.next_block;

    file = (FileBlock *) (f->sfs->disk->bitmap_data + (BLOCK_SIZE*file->header.next_block));


    int fb_off_set = f->pos_in_file - MAX_FFB;

    int off_set = fb_off_set % MAX_FB;

    off_set--;

    int remaining_blocks = f->fcb->fcb.size_in_blocks - f->current_block->block_in_file;

    int bytes_written = size;

    int data_counter = 0;

    int i = 0;

    int limit = bytes_written;

    int bytes_return = 0;

    char *data_to_write = data;

    int has_next = f->current_block->next_block;

    for(i = 0; (i < limit) && (off_set < MAX_FB); i++){
      if(file->data[off_set] == '\0'){
        f->fcb->fcb.size_in_bytes++;
        file->available--;
      }
      file->data[off_set] = data_to_write[data_counter];
      bytes_written--;
      data_counter++;
      f->pos_in_file++;
      bytes_return++;
      off_set++;
    }

    if(bytes_written > 0 && file->available == 0){

        while(bytes_written > 0){

          if(has_next != -1){

            FileBlock *file = (FileBlock *) malloc(sizeof(FileBlock));
            file = (FileBlock *) (f->sfs->disk->bitmap_data + (BLOCK_SIZE*has_next));

            current_block_in_disk = has_next;

            f->current_block = (&(file->header));

            has_next = f->current_block->next_block;

            off_set = 0;

            limit = bytes_written;

              for(i = 0; (i < limit) && (off_set < MAX_FB); i++){
                if(file->data[off_set] == '\0'){
                  f->fcb->fcb.size_in_bytes++;
                  file->available--;
                }
              file->data[off_set] = data_to_write[data_counter];
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
              printf("ERRORE: NON CI SONO BLOCCHI LIBERI NEL DISCO, IMPOSSIBILE CONTINUARE A SCRIVERE IL FILE\n");
              return -1;
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

            int available = file->available;



            if(DiskDriver_writeBlock(f->sfs->disk, file, freeblock) == -1){
              printf("ERRORE: IMPOSSIBILE CREARE FILE\n");
              return -1;
            }

            file = (FileBlock *) (f->sfs->disk->bitmap_data + (BLOCK_SIZE*current_block_in_disk));

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


      }

          return bytes_return;

  }

}

// writes in the file, at current position size bytes stored in data
// overwriting and allocating new space if necessary
// returns the number of bytes read
int SimpleFS_read(FileHandle* f, void* data, int size) {

  if(f->fcb->fcb.size_in_bytes <= f->pos_in_file) {
    printf("ERRORE: FUORI GLI INDICI DEL FILE\n");
    return -1;
  }


  //Controllo se devo scrivere nel primo blocco del file o in uno successivo
  if(f->current_block->block_in_file == 0){

    FirstFileBlock *first = (FirstFileBlock *) malloc(sizeof(FirstFileBlock));

    first = (FirstFileBlock *) (f->sfs->disk->bitmap_data + (BLOCK_SIZE*f->fcb->fcb.block_in_disk));
    int occupied_blocks = MAX_FFB - f->pos_in_file;

    int current_block_in_disk = f->fcb->fcb.block_in_disk;

    int bytes_read = size - f->pos_in_file;

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
      if(off_set == f->fcb->fcb.size_in_bytes){
        break;
      }
    }

    if(bytes_read > 0 && occupied_blocks == 0){

            while(bytes_read > 0){


            FileBlock *file = (FileBlock *) malloc(sizeof(FileBlock));
            file = (FileBlock *) (f->sfs->disk->bitmap_data + (BLOCK_SIZE*has_next));

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
              if(off_set == f->fcb->fcb.size_in_bytes){
                break;
              }

            }

            if(has_next == -1) break;
          }


        DiskDriver_flush(f->sfs->disk);

      }
          return bytes_return;
    }
    else {

      FileBlock *file = (FileBlock *) malloc(sizeof(FileBlock));

      file = (FileBlock *) (f->sfs->disk->bitmap_data + (BLOCK_SIZE*f->current_block->previous_block));

      int current_block_in_disk = file->header.next_block;

      file = (FileBlock *) (f->sfs->disk->bitmap_data + (BLOCK_SIZE*file->header.next_block));

      int remaining_blocks = f->fcb->fcb.size_in_blocks - 2;


      int fb_off_set = f->pos_in_file - MAX_FFB;

      int off_set = fb_off_set % MAX_FB;

      int occupied_blocks = MAX_FB - off_set;

      int bytes_read = size - f->pos_in_file;

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
        occupied_blocks--;
        if(off_set == f->fcb->fcb.size_in_bytes){
          break;
        }
      }

      if(bytes_read > 0 && occupied_blocks == 0){

        while(bytes_read > 0){


          if(has_next != -1){

              FileBlock *file = (FileBlock *) malloc(sizeof(FileBlock));
              file = (FileBlock *) (f->sfs->disk->bitmap_data + (BLOCK_SIZE*has_next));

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
                occupied_blocks--;
                if(off_set == f->fcb->fcb.size_in_bytes){
                  break;
                }
              }
            }
          }

          DiskDriver_flush(f->sfs->disk);

        }

                  return bytes_return;
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

  if(test <= 0){
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
      file =  (FileBlock *) (f->sfs->disk->bitmap_data + (BLOCK_SIZE*next));
      f->current_block = (&(file->header));
      next = f->current_block->next_block;
      blocco--;
    }
  }
  return pos;
}

// seeks for a directory in d. If dirname is equal to ".." it goes one level up
// 0 on success, negative value on error
// it does side effect on the provided handle
 int SimpleFS_changeDir(DirectoryHandle* d, char* dirname){

//Salgo alla parent directory
   if((strcmp("..", dirname)) == 0) {
     //Verifico se la parent è la top-level
     if(d->directory == NULL){
       printf("ERRORE: TOP-LEVEL DIRECTORY RAGGIUNTO\n");
       return -1;
     }
     else{
       int up = d->directory->fcb.directory_block;
       d->dcb = (FirstDirectoryBlock *) (d->sfs->disk->bitmap_data + (BLOCK_SIZE*d->directory->fcb.block_in_disk));
       d->sfs->current_dblock = d->directory->fcb.block_in_disk;
       if(up != NULL){
         d->directory = (FirstDirectoryBlock *) (d->sfs->disk->bitmap_data + (BLOCK_SIZE*up));
       }
       else{
         d->directory = NULL;
       }
     }

     return 0;
   }
//Cerco se esiste la directory
   else{

     FirstDirectoryBlock *filedest = (FirstDirectoryBlock *) malloc(sizeof(FirstDirectoryBlock));
     FirstDirectoryBlock *dsource = (FirstDirectoryBlock *) malloc(sizeof(FirstDirectoryBlock));

     dsource = (FirstDirectoryBlock *) (d->sfs->disk->bitmap_data + (BLOCK_SIZE*d->dcb->fcb.block_in_disk));

     int remaining_blocks = d->dcb->fcb.size_in_blocks;

     int occupied_blocks = MAX_FD_BLOCK - dsource->available;

     int blocco = -1;

       //Controllo nel primo blocco della directory

       for(int i = 0; i < MAX_FD_BLOCK ; i++) {
         if((dsource->file_blocks[i]) > 0){
           filedest = (FirstDirectoryBlock *) (d->sfs->disk->bitmap_data + (BLOCK_SIZE*dsource->file_blocks[i]));
           if(filedest->fcb.is_dir == 1) {              // IS DIR
             if((strcmp((filedest->fcb.name), dirname)) == 0) {
             blocco = dsource->file_blocks[i];
             d->dcb = (FirstDirectoryBlock *) (d->sfs->disk->bitmap_data + (BLOCK_SIZE*blocco));
             d->sfs->current_dblock = blocco;
             int parent = d->dcb->fcb.directory_block;
             if(parent != -1){
                 d->directory = (FirstDirectoryBlock *) (d->sfs->disk->bitmap_data + (BLOCK_SIZE*parent));
               }
               else{
                 d->directory = NULL;
               }
               d->current_block = (&(d->dcb->header));
               return 0;
             }
             }

         }
     }

     //Cerco nei rimanendi blocchi della directory
     if(remaining_blocks > 1) {

       d->current_block_in_disk = dsource->header.next_block;

       DirectoryBlock* bsource = (DirectoryBlock *) malloc(sizeof(DirectoryBlock));
       bsource = (DirectoryBlock *) (d->sfs->disk->bitmap_data + (BLOCK_SIZE*d->current_block_in_disk));

       do{
         remaining_blocks--;

         occupied_blocks = MAX_D_BLOCK - bsource->available;
         for(int i = 0; i < MAX_D_BLOCK ; i++) {
           if((bsource->file_blocks[i]) > 0){
             filedest = (FirstDirectoryBlock *) (d->sfs->disk->bitmap_data + (BLOCK_SIZE*bsource->file_blocks[i]));
            if(filedest->fcb.is_dir == 1){ // IS DIR
             if((strcmp((filedest->fcb.name), dirname)) == 0) {
               blocco = bsource->file_blocks[i];
               d->dcb = (FirstDirectoryBlock *) (d->sfs->disk->bitmap_data + (BLOCK_SIZE*blocco));
               d->sfs->current_dblock = blocco;
               int parent = d->dcb->fcb.directory_block;
               if(parent != -1){
                 d->directory = (FirstDirectoryBlock *) (d->sfs->disk->bitmap_data + (BLOCK_SIZE*parent));
               }
               else{
                 d->directory = NULL;
               }
               d->current_block = (&(d->dcb->header));
               return 0;
             }
           }
           }
         }
         if((bsource->header.next_block) != -1) {
           d->current_block_in_disk = bsource->header.next_block;
           bsource = (DirectoryBlock *) (d->sfs->disk->bitmap_data + (BLOCK_SIZE*d->current_block_in_disk));
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
    printf("ERRORE: NON CI SONO BLOCCHI LIBERI SUL DISCO\n");
    return -1;
  }

  d->current_block = (&(d->dcb->header));
  //Controllo se la directory già ESISTE
  FirstDirectoryBlock *filedest = (FirstDirectoryBlock *) malloc(sizeof(FirstDirectoryBlock));
  FirstDirectoryBlock *dsource = (FirstDirectoryBlock *) malloc(sizeof(FirstDirectoryBlock));

  dsource = (FirstDirectoryBlock *) (d->sfs->disk->bitmap_data + (BLOCK_SIZE*d->dcb->fcb.block_in_disk));

  int remaining_blocks = d->dcb->fcb.size_in_blocks;

  int occupied_blocks = MAX_FD_BLOCK - dsource->available;

  int blocco = -1;

  int has_next = d->current_block->next_block;



    //Controllo nel primo blocco della directory

    for(int i = 0; i < MAX_FD_BLOCK ; i++) {
      if((dsource->file_blocks[i]) > 0){
        filedest = (FirstDirectoryBlock *) (d->sfs->disk->bitmap_data + (BLOCK_SIZE*dsource->file_blocks[i]));
        if(filedest->fcb.is_dir == 1){ //IS DIR
        if((strcmp((filedest->fcb.name), dirname)) == 0) {
          printf("ERRORE: LA DIRECTORY GIÀ ESISTE\n");
          return -1;
        }
      }
      }
  }

  //Cerco nei rimanendi blocchi della directory
  if((has_next != -1) && (remaining_blocks > 1)) {
    DirectoryBlock* bsource = (DirectoryBlock *) malloc(sizeof(DirectoryBlock));
    bsource = (DirectoryBlock *) (d->sfs->disk->bitmap_data + (BLOCK_SIZE*has_next));
    d->current_block = (&(bsource->header));
    d->current_block_in_disk = has_next;
    has_next = d->current_block->next_block;

    do{

      remaining_blocks--;
      occupied_blocks = MAX_D_BLOCK - bsource->available;

      for(int i = 0; i < MAX_D_BLOCK ; i++) {
        if((bsource->file_blocks[i]) > 0){
          filedest = (FirstDirectoryBlock *) (d->sfs->disk->bitmap_data + (BLOCK_SIZE*bsource->file_blocks[i]));
          if((strcmp((filedest->fcb.name), dirname)) == 0) {
            printf("\nERRORE, IL FILE GIÀ ESISTE\n\n");
            return NULL;
          }
        }

    }
    if(has_next != -1) {
    d->current_block_in_disk = has_next;
    bsource = (DirectoryBlock *) (d->sfs->disk->bitmap_data + (BLOCK_SIZE*has_next));
    d->current_block = (&(bsource->header));
    has_next = d->current_block->next_block;
  }
  } while(remaining_blocks > 1);
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

    nd->fcb.size_in_blocks = 1;

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

    if(DiskDriver_writeBlock(d->sfs->disk, nd,freeblock) == -1){
      printf("ERRORE: IMPOSSIBILE CREARE DIRECTORY\n");
      return -1;
    }
    return 0;


  }
  //IL PRIMO BLOCCO DELLA DIRECTORY NON HA SPAZIO -> VEDO I PROSSIMI
  else{
    //ESISTE UN BLOCCO DELLA DIRECTORY
    if(d->current_block_in_disk > 2){

      DirectoryBlock *db = (DirectoryBlock *) malloc(sizeof(DirectoryBlock));
      db = (DirectoryBlock *) (d->sfs->disk->bitmap_data + (BLOCK_SIZE*d->current_block_in_disk));
      d->current_block = (&(db->header));

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

        nd->fcb.size_in_blocks = 1;

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

        if(DiskDriver_writeBlock(d->sfs->disk, nd,freeblock) == -1){
          printf("ERRORE: IMPOSSIBILE CREARE DIRECTORY\n");
          return -1;
        }

        return 0;
      }

      //L'ULTIMO BLOCCO DELLA DIRECTORY NON HA SPAZIO -> CREO NUOVO BLOCCO E POI LA NUOVA DIRECTORY
      else{

        if(d->sfs->disk->header->free_blocks < 2){
          printf("ERRORE: NON CI SONO BLOCCHI LIBERI SUFFICIENTI NEL DISCO\n");
          return -1;
        }


        int freeblock = DiskDriver_getFreeBlock(d->sfs->disk, 0);


        DirectoryBlock *ndb = (DirectoryBlock *) malloc(sizeof(DirectoryBlock));

        db->header.next_block = freeblock;
        d->current_block->next_block = freeblock;

        db->available--;




        //HEADER
        ndb->header.previous_block = d->current_block_in_disk;
        ndb->header.next_block = -1;
        ndb->header.block_in_file = d->current_block->block_in_file + 1;

        //AVALAIBLE
        ndb->available = MAX_D_BLOCK;
        ndb->available--;



        if(DiskDriver_writeBlock(d->sfs->disk, ndb, freeblock) == -1){
          printf("ERRORE: IMPOSSIBILE CREARE DIRECTORY\n");
          return -1;
        }

        ndb = (DirectoryBlock *) (d->sfs->disk->bitmap_data + (BLOCK_SIZE*freeblock));

        int blockford = DiskDriver_getFreeBlock(d->sfs->disk, 0);

        //FILE BLOCKS
        ndb->file_blocks[0] = blockford;

        DiskDriver_flush(d->sfs->disk);


        d->current_block = (&(ndb->header));

        FirstDirectoryBlock *nd = (FirstDirectoryBlock *) malloc(sizeof(FirstDirectoryBlock));


        //HEADER
        nd->header.previous_block = -1;
        nd->header.next_block = -1;
        nd->header.block_in_file = 0;

        //FCB
        nd->fcb.directory_block = d->dcb->fcb.block_in_disk;

        nd->fcb.block_in_disk = blockford;

        strcpy(nd->fcb.name, dirname);

        nd->fcb.size_in_bytes = 0;

        nd->fcb.size_in_blocks = 1;

        nd->fcb.is_dir = 1;

        //NUM_ENTRIES
        nd->num_entries = 0;

        //AVALAIBLE
        nd->available = MAX_FD_BLOCK;


        //AGGIORNO dcb
        //FCB...

        //NUM_ENTRIES
        d->dcb->num_entries++;

        d->dcb->fcb.size_in_blocks++;

        if(DiskDriver_writeBlock(d->sfs->disk, nd,blockford) == -1){
          printf("ERRORE: IMPOSSIBILE CREARE DIRECTORY\n");
          return -1;
        }

        return 0;
      }
    }

    //NON ESISTE UN PROSSIMO BLOCCO DELLA DIRECTORY -> CREO NUOVO BLOCCO E POI CREO LA NUOVA DIRECTORY
    else {

      if(d->sfs->disk->header->free_blocks < 2){
        printf("ERRORE: NON CI SONO BLOCCHI LIBERI SUFFICIENTI NEL DISCO\n");
        return -1;
      }

      int freeblock = DiskDriver_getFreeBlock(d->sfs->disk, 0);

      DirectoryBlock *ndb = (DirectoryBlock *) malloc(sizeof(DirectoryBlock));

      //HEADER
      ndb->header.previous_block = d->dcb->fcb.block_in_disk;
      ndb->header.next_block = -1;
      ndb->header.block_in_file = d->current_block->block_in_file + 1;

      //AVALAIBLE
      ndb->available = MAX_D_BLOCK;
      ndb->available--;

      //FILE BLOCKS


      if(DiskDriver_writeBlock(d->sfs->disk, ndb, freeblock) == -1){
        printf("ERRORE: IMPOSSIBILE CREARE DIRECTORY\n");
        return -1;
      }

      ndb = (DirectoryBlock *) (d->sfs->disk->bitmap_data + (BLOCK_SIZE*freeblock));

      int blockford = DiskDriver_getFreeBlock(d->sfs->disk, 0);

      ndb->file_blocks[0] = blockford;

      d->dcb->header.next_block = freeblock;

      d->current_block->next_block = freeblock;



      FirstDirectoryBlock *nd = (FirstDirectoryBlock *) malloc(sizeof(FirstDirectoryBlock));
      //HEADER
      nd->header.previous_block = -1;
      nd->header.next_block = -1;
      nd->header.block_in_file = 0;

      //FCB
      nd->fcb.directory_block = d->dcb->fcb.block_in_disk;

      nd->fcb.block_in_disk = blockford;

      strcpy(nd->fcb.name, dirname);

      nd->fcb.size_in_bytes = 0;

      nd->fcb.size_in_blocks = 1;

      nd->fcb.is_dir = 1;

      //NUM_ENTRIES
      nd->num_entries = 0;

      //AVALAIBLE
      nd->available = MAX_FD_BLOCK;


      //AGGIORNO dcb
      //FCB...

      //NUM_ENTRIES
      d->dcb->num_entries++;

      d->dcb->fcb.size_in_blocks++;

      DiskDriver_flush(d->sfs->disk);

      if(DiskDriver_writeBlock(d->sfs->disk, nd,blockford) == -1){
        printf("ERRORE: IMPOSSIBILE CREARE DIRECTORY\n");
        return -1;
      }

      return 0;

    }
  }

}

// removes the file in the current directory
// returns -1 on failure 0 on success
// if a directory, it removes recursively all contained files
int SimpleFS_remove(SimpleFS* fs, char* filename){

  FirstDirectoryBlock *filedest = (FirstDirectoryBlock *) malloc(sizeof(FirstDirectoryBlock));
  FirstDirectoryBlock *dsource = (FirstDirectoryBlock *) malloc(sizeof(FirstDirectoryBlock));

  dsource = (FirstDirectoryBlock *) (fs->disk->bitmap_data + (BLOCK_SIZE*fs->current_dblock));

  int remaining_blocks = dsource->fcb.size_in_blocks;

  int occupied_blocks = MAX_FD_BLOCK - dsource->available;

  int blocco = -1;

  int current_block = -1;

  int ret;

    //Controllo nel primo blocco della directory


    for(int i = 0; i < MAX_FD_BLOCK ; i++) {
      if((dsource->file_blocks[i]) > 0){
        filedest = (FirstDirectoryBlock *) (fs->disk->bitmap_data + (BLOCK_SIZE*dsource->file_blocks[i]));
        if((strcmp((filedest->fcb.name), filename)) == 0) {
          printf("\nCANCELLANDO FILE O DIRECTORY: %s\n", filedest->fcb.name);
          dsource->num_entries--;
          dsource->available++;
          dsource->file_blocks[i] = 0;
          if(filedest->fcb.is_dir == 0){ //Is FILE
            int has_next = filedest->header.next_block;
            int prev = has_next;
            if(DiskDriver_freeBlock(fs->disk, filedest->fcb.block_in_disk) == -1){
              printf("ERRORE: IMPOSSIBILE LIBERARE BLOCCO DEL DISCO\n");
              return -1;
            }
            while (has_next != -1) {
              FileBlock *file_next = (FileBlock *) malloc(sizeof(FileBlock));
              file_next = (FileBlock *) (fs->disk->bitmap_data + (BLOCK_SIZE*has_next));
              has_next = file_next->header.next_block;
              if(DiskDriver_freeBlock(fs->disk, prev) == -1){
                printf("ERRORE: IMPOSSIBILE LIBERARE BLOCCO DEL DISCO\n");
                return -1;
              }
              prev = has_next;
            }
          }
          else { //IS DIR
            int free_this = filedest->fcb.block_in_disk;
            int has_next = filedest->header.next_block;
            int prev = has_next;
            if(Remove_Directory(fs, free_this) == -1){
              printf("ERRORE: IMPOSSIBILE RIMUOVERE DIRECTORY\n");
              return -1;
            }
            if(DiskDriver_freeBlock(fs->disk, free_this) == -1){
              printf("ERRORE: IMPOSSIBILE LIBERARE BLOCCO DEL DISCO\n");
              return -1;

            }
            while (has_next != -1) {
              DirectoryBlock *d_next = (DirectoryBlock *) malloc(sizeof(DirectoryBlock));
              d_next = (DirectoryBlock *) (fs->disk->bitmap_data + (BLOCK_SIZE*has_next));
              has_next = d_next->header.next_block;
              if(DiskDriver_freeBlock(fs->disk, prev) == -1){
                printf("ERRORE: IMPOSSIBILE LIBERARE BLOCCO DEL DISCO\n");
                return -1;
              }
              prev = has_next;
            }
          }
          return 0;
        }
      }
  }

  //Cerco nei rimanendi blocchi della directory
  if(remaining_blocks > 1) {
    current_block = dsource->header.next_block;

    DirectoryBlock* bsource = (DirectoryBlock *) malloc(sizeof(DirectoryBlock));
    bsource = (DirectoryBlock *) (fs->disk->bitmap_data + (BLOCK_SIZE*current_block));

    do{
      remaining_blocks--;

      occupied_blocks = MAX_D_BLOCK - bsource->available;
      for(int i = 0; i < MAX_D_BLOCK ; i++) {
        if((bsource->file_blocks[i]) > 0){
          filedest = (FirstDirectoryBlock *) (fs->disk->bitmap_data + (BLOCK_SIZE*bsource->file_blocks[i]));
          if((strcmp((filedest->fcb.name), filename)) == 0) {
            dsource->num_entries--;
            dsource->available++;
            printf("\nCANCELLANDO FILE O DIRECTORY: %s\n", filedest->fcb.name);
            bsource->file_blocks[i] = 0;
            if(filedest->fcb.is_dir == 0){ //Is FILE
              int has_next = filedest->header.next_block;
              int prev = has_next;
              if(DiskDriver_freeBlock(fs->disk, filedest->fcb.block_in_disk) == -1){
                printf("ERRORE: IMPOSSIBILE LIBERARE BLOCCO DEL DISCO\n");
                return -1;
              }
              while (has_next != -1) {
                FileBlock *file_next = (FileBlock *) malloc(sizeof(FileBlock));
                file_next = (FileBlock *) (fs->disk->bitmap_data + (BLOCK_SIZE*has_next));
                has_next = file_next->header.next_block;
                if(DiskDriver_freeBlock(fs->disk, prev) == -1){
                  printf("ERRORE: IMPOSSIBILE LIBERARE BLOCCO DEL DISCO\n");
                  return -1;
                }
                prev = has_next;
              }
            }
            else { //IS DIR
              int free_this = filedest->fcb.block_in_disk;
              int has_next = filedest->header.next_block;
              int prev = has_next;
              if(Remove_Directory(fs, free_this) == -1){
                printf("ERRORE: IMPOSSIBILE RIMUOVERE DIRECTORY\n");
                return -1;
              }
              if(DiskDriver_freeBlock(fs->disk, free_this) == -1){
                printf("ERRORE: IMPOSSIBILE LIBERARE BLOCCO DEL DISCO\n");
                return -1;
              }
              while (has_next != -1) {
                DirectoryBlock *d_next = (DirectoryBlock *) malloc(sizeof(DirectoryBlock));
                d_next = (DirectoryBlock *) (fs->disk->bitmap_data + (BLOCK_SIZE*has_next));
                has_next = d_next->header.next_block;
                if(DiskDriver_freeBlock(fs->disk, prev) == -1){
                  printf("ERRORE: IMPOSSIBILE LIBERARE BLOCCO DEL DISCO\n");
                  return -1;
                }
                prev = has_next;
              }
            }

          }

        }

    }
    if((bsource->header.next_block) != -1) {
    current_block= bsource->header.next_block;
    bsource = (DirectoryBlock *) (fs->disk->bitmap_data + (BLOCK_SIZE*current_block));
  }
  } while(remaining_blocks > 1);
        return 0;
}
}

int Remove_Directory(SimpleFS* fs, int index){


  FirstFileBlock *filedest = (FirstFileBlock *) malloc(sizeof(FirstFileBlock));
  FirstDirectoryBlock *fdb = (FirstDirectoryBlock *) malloc(sizeof(FirstDirectoryBlock));

  fdb = (FirstDirectoryBlock *) (fs->disk->bitmap_data + (BLOCK_SIZE*index));

  int remaining_blocks = fdb->fcb.size_in_blocks;
  int occupied_blocks = MAX_FD_BLOCK - fdb->available;

  int blocco = -1;

  int current_block = -1;

  int ret;


    //Controllo nel primo blocco della directory


    for(int i = 0; i < MAX_FD_BLOCK ; i++) {
      if((fdb->file_blocks[i]) > 0){
        filedest = (FirstFileBlock *) (fs->disk->bitmap_data + (BLOCK_SIZE*fdb->file_blocks[i]));
        fdb->available++;
        fdb->num_entries--;
        printf("\nCANCELLANDO FILE O DIRECTORY: %s\n", filedest->fcb.name);
        fdb->file_blocks[i] = 0;
        if(filedest->fcb.is_dir == 0){ //Is FILE
          int has_next = filedest->header.next_block;
          int prev = has_next;
          if(DiskDriver_freeBlock(fs->disk, filedest->fcb.block_in_disk) == -1){
            printf("ERRORE: IMPOSSIBILE LIBERARE BLOCCO DEL DISCO\n");
            return -1;
          }
          while (has_next != -1) {
            FileBlock *file_next = (FileBlock *) malloc(sizeof(FileBlock));
            file_next = (FileBlock *) (fs->disk->bitmap_data + (BLOCK_SIZE*has_next));
            has_next = file_next->header.next_block;
            if(DiskDriver_freeBlock(fs->disk, prev) == -1){
              printf("ERRORE: IMPOSSIBILE LIBERARE BLOCCO DEL DISCO\n");
              return -1;
            }
            prev = has_next;
            file_next = NULL;
          }
        }
        else { //IS DIR
          int free_this = filedest->fcb.block_in_disk;
          int has_next = filedest->header.next_block;
          int prev = has_next;
          if(Remove_Directory(fs, free_this) == -1){
            printf("ERRORE: IMPOSSIBILE RIMUOVERE DIRECTORY\n");
            return -1;
          }
          if(DiskDriver_freeBlock(fs->disk, free_this) == -1){
            printf("ERRORE: IMPOSSIBILE LIBERARE BLOCCO DEL DISCO\n");
            return -1;
          }
          while (has_next != -1) {
            DirectoryBlock *d_next = (DirectoryBlock *) malloc(sizeof(DirectoryBlock));
            d_next = (DirectoryBlock *) (fs->disk->bitmap_data + (BLOCK_SIZE*has_next));
            has_next = d_next->header.next_block;
            if(DiskDriver_freeBlock(fs->disk, prev) == -1){
              printf("ERRORE: IMPOSSIBILE LIBERARE BLOCCO DEL DISCO\n");
              return -1;
            }
            prev = has_next;
            d_next = NULL;
          }
        }
      }

      return 0;
    }

  //Cerco nei rimanendi blocchi della directory
  if(remaining_blocks > 1) {
    current_block = fdb->header.next_block;

    DirectoryBlock* bsource = (DirectoryBlock *) malloc(sizeof(DirectoryBlock));
    bsource = (DirectoryBlock *) (fs->disk->bitmap_data + (BLOCK_SIZE*current_block));

    do{
      remaining_blocks--;

      occupied_blocks = MAX_D_BLOCK - bsource->available;

      for(int i = 0; i < MAX_D_BLOCK ; i++) {
        if((bsource->file_blocks[i]) > 0){
          filedest = (FirstFileBlock *) (fs->disk->bitmap_data + (BLOCK_SIZE*bsource->file_blocks[i]));
          printf("\nCANCELLANDO FILE O DIRECTORY: %s\n", filedest->fcb.name);
          bsource->available++;
          fdb->num_entries--;
          bsource->file_blocks[i] = 0;
          if(filedest->fcb.is_dir == 0){ //Is FILE
            int has_next = filedest->header.next_block;
            int prev = has_next;
            if(DiskDriver_freeBlock(fs->disk, filedest->fcb.block_in_disk) == -1){
              printf("ERRORE: IMPOSSIBILE LIBERARE BLOCCO DEL DISCO\n");
              return -1;
            }
            while (has_next != -1) {
              FileBlock *file_next = (FileBlock *) malloc(sizeof(FileBlock));
              file_next = (FileBlock *) (fs->disk->bitmap_data + (BLOCK_SIZE*has_next));
              has_next = file_next->header.next_block;
              if(DiskDriver_freeBlock(fs->disk, prev) == -1){
                printf("ERRORE: IMPOSSIBILE LIBERARE BLOCCO DEL DISCO\n");
                return -1;
              }
              prev = has_next;
            }
          }
          else { //IS DIR
            int free_this = filedest->fcb.block_in_disk;
            int has_next = filedest->header.next_block;
            int prev = has_next;
            if(Remove_Directory(fs, free_this) == -1){
              printf("ERRORE: IMPOSSIBILE RIMUOVERE DIRECTORY\n");
              return -1;
            }
            if(DiskDriver_freeBlock(fs->disk, free_this) == -1){
              printf("ERRORE: IMPOSSIBILE LIBERARE BLOCCO DEL DISCO\n");
              return -1;
            }
            while (has_next != -1) {
              DirectoryBlock *d_next = (DirectoryBlock *) malloc(sizeof(DirectoryBlock));
              d_next = (DirectoryBlock *) (fs->disk->bitmap_data + (BLOCK_SIZE*has_next));
              has_next = d_next->header.next_block;
              if(DiskDriver_freeBlock(fs->disk, prev) == -1){
                printf("ERRORE: IMPOSSIBILE LIBERARE BLOCCO DEL DISCO\n");
                return -1;
              }
              prev = has_next;
            }
          }
        }

        return 0;
      }


    if((bsource->header.next_block) != -1) {
    current_block= bsource->header.next_block;
    bsource = (DirectoryBlock *) (fs->disk->bitmap_data + (BLOCK_SIZE*current_block));
  }
  } while(remaining_blocks > 1);
  }

  fdb = NULL;
  return 0;
}
