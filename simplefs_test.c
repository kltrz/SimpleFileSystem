#include "simplefs.c"
#include <stdio.h>
#include "math.h"
int main(int agc, char** argv) {

  int scelta = -1;
  int check = -1;

  int blocchi = -1;

  DirectoryHandle *d = (DirectoryHandle *) malloc(sizeof(DirectoryHandle));
  FileHandle *f = (FileHandle *) malloc(sizeof(FileHandle));

  SimpleFS *fs = (SimpleFS *) malloc(sizeof(SimpleFS));
  fs->disk = (DiskDriver *) malloc(sizeof(DiskDriver));


  printf("\n\n\nCreazione Disco. Quanti blocchi deve avere il disco? (Se il disco già esiste verrà caricato): ");
  while ( scanf( "%d", &blocchi ) < 1 || blocchi < 4)
  {
    getchar();
    if(blocchi < 4){
      printf("\n\nAttenzione: il disco deve avere minimo 4 blocchi\n");
    }
    printf( "Valore non valido inserito, provare a inserire un valore valido per la creazione del disco: " );
  }
  DiskDriver_init(fs->disk, "DISK", blocchi);
  d = SimpleFS_init(fs, fs->disk);
  if(fs->disk->header->free_blocks == (blocchi - 2)){
    SimpleFS_format(fs);
    d = SimpleFS_init(fs, fs->disk);

  }

  printf("\nPremi un tasto per continuare\n");
  getchar();
  getchar();



  while(scelta != 0){



    printf("\n\n --- MENU SIMPLE FILE SYSTEM ---\n");

    if(d->dcb != NULL){
      printf("\033[1;34m"); //Set the text to the color red
      printf("\n     Directory: %s\n", d->dcb->fcb.name);
      printf("\033[0m"); //Resets the text to default color

    }
    if(f != NULL){
      if(f->fcb != NULL){
        printf("\033[0;36m"); //Set the text to the color red
        printf("     File: %s\n\n", f->fcb->fcb.name);
        printf("\033[0m"); //Resets the text to default color

      }
    }

    printf("\n 1. FORMATTAZIONE DEL DISCO\n");
    printf(" 2. CREA FILE NELLA DIRECTORY CORRENTE\n");
    printf(" 3. STAMPA IL NOME DI TUTTI I FILE DELLA DIRECTORY CORRENTE\n");
    printf(" 4. APRI FILE NELLA DIRECTORY CORRENTE\n");
    printf(" 5. CHIUDI FILE CORRENTE\n");
    printf(" 6. SCRIVI SUL FILE CORRENTE\n");
    printf(" 7. LEGGI IL FILE CORRENTE\n");
    printf(" 8. SPOSTA IL CURSORE DEL FILE CORRENTE\n");
    printf(" 9. CAMBIA DIRECTORY\n");
    printf(" 10. CREA UNA DIRECTORY NELLA DIRECTORY CORRENTE\n");
    printf(" 11. RIMUOVI FILE O DIRECTORY\n");
    printf("\t\n 0. CHIUDERE IL FILE SYSTEM\n\n");



    printf("Scegliere una opzione: ");

    while ( scanf( "%d", &scelta ) < 1 )
    {
      getchar();
      printf( "\nValore non numerico inserito, provare a inserire un valore valido: " );
    }

      switch(scelta){
        case 1: {
          SimpleFS_format(fs);
          d = SimpleFS_init(fs, fs->disk);
          f = NULL;
        }
        break;
        case 2: {
          if(d->dcb->fcb.name != NULL){
          char name[128];
          printf("\nNome del file: ");
          scanf("%s", name);
          f = SimpleFS_createFile(d, name);
          printf("\nDirectory corrente: %s\n", d->dcb->fcb.name);
          if(f != NULL){
            printf("\nFile creato: %s\n\n\n", f->fcb->fcb.name);
          } else printf("\nNessun file è stato creato!\n");
        }
        else{
          printf("ERRORE: NESSUNA DIRECTORY APERTA\n");
        }
          break;
          }
        case 3:{
          if(d != NULL){
          char **dest = (char **) malloc(sizeof(char *) * d->dcb->num_entries);
          int x = SimpleFS_readDir(dest, d);
          if(x == -1){
            break;
          }
          puts("\n\n");
          printf("\nFiles presenti in directory: %s\n\n", d->dcb->fcb.name);
          for(int i = 0; i < d->dcb->num_entries; i++){
            if(dest[i] != NULL){
              if(dest[i][0] == 'f'){
                printf("\033[0;36m"); //Set the text to the color red
                printf(" %s \n", dest[i]); //Display Hello in red
                printf("\033[0m"); //Resets the text to default color
              }
              else{
                printf("\033[1;34m"); //Set the text to the color red
                printf(" %s \n", dest[i]); //Display Hello in red
                printf("\033[0m"); //Resets the text to default color
              }

          }
          }
          printf("\nPremi un tasto per continuare\n");
          getchar();
          getchar();
          }
          else{
            printf("NESSUNA DIRECTORY APERTA\n");
          }
          break;
        }

        case 4:{
          char name[128];
          printf("\nNome del file da cercare: ");
          scanf("%s", name);
          f = SimpleFS_openFile(d, name);
          if(f != NULL){
            printf("FILE APERTO: %s\n", f->fcb->fcb.name);
          }
          else printf("\nNessun file è stato aperto!\n");
          break;
        }
        case 5: {
          if(f != NULL){
            if(f->fcb != NULL){
          int x = SimpleFS_close(f);
          f = NULL;
        }
        else {
          printf("\nNESSUN FILE APERTO\n");
        }
        }
        else {
          printf("\nNESSUN FILE APERTO\n");
        }
            break;
      }
        case 6:{
          if(f != NULL){
            if(f->fcb != NULL){
            printf("\nProcedendo alla scrittura, premere un tasto per continuare\n");
            getchar();
            getchar();
            char data[900];
            printf("Inserichi data da scrivere nel file: ");
            scanf("%[^\t\n]s", data);
            int x = SimpleFS_write(f, data, strlen(data));
          }
          else {
              printf("\nNESSUN FILE APERTO\n");
          }
          }
          break;
        }
        case 7: {
          if(f != NULL){
            if(f->fcb != NULL){
            if(f->fcb->fcb.size_in_bytes != 0){
              char data[f->fcb->fcb.size_in_bytes - f->pos_in_file];
              int x = SimpleFS_read(f, data, f->fcb->fcb.size_in_bytes);
              puts(data);
              printf("\nPremi un tasto per continuare\n");
              getchar();
              getchar();

            }
            else{
              printf("IL FILE NON CONTIENE DATI DA LEGGERE\n");
              printf("\nPremi un tasto per continuare\n");
              getchar();
              getchar();
            }
          }
          else {
              printf("\nNESSUN FILE APERTO\n");
          }
          }
          break;
        }
        case 8:{
          if(f != NULL){
        if(f->fcb != NULL){
            int x;
            int ret;
            printf("Posizione attuale del file-pointer: %d (A partire da 0)\n", f->pos_in_file);
            printf("Scegliere dove posizionare il file-pointer: ");
            scanf("%d", &x);
            ret = SimpleFS_seek(f, x);
            if(ret != -1){
            printf("Posizione raggiunta: %d (A partire da 0)\n", ret);
          }
          else {
            printf("\nErrore nello spostamento del file-pointer\n");
          }
        }
        else {
            printf("\nNESSUN FILE APERTO\n");
        }
          }
            break;
        }

        case 9: {
          if(d != NULL){
              if(d->dcb != NULL){
                char name[128];
                printf("\nNome della directory da cercare: ");
                scanf("%s", name);
                int ret = SimpleFS_changeDir(d, name);

                f = NULL;
          }
        }
        break;
      }
        case 10: {
          if(d != NULL){
              if(d->dcb != NULL){
                char name[128];
                printf("\nNome della directory da creare: ");
                scanf("%s", name);
                int ret = SimpleFS_mkDir(d, name);
          }
        }
        break;
      }
        case 11: {
          if(d != NULL){
              if(d->dcb != NULL){
                char name[128];
                printf("\nNome del file o directory da rimuovere: ");
                scanf("%s", name);
                int ret = SimpleFS_remove(d->sfs, name);
          }
        }
        break;
      }

        case 0: printf("\n File System chiuso \n\n");
                       break;
        default: printf("\nValore non valido. Scegliere una opzione tra 0 e 11\n\n");
                 break;
      }
    }
  }
