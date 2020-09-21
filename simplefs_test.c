#include "simplefs.c"
#include <stdio.h>
int main(int agc, char** argv) {
  int scelta = -1;

  DirectoryHandle *d = (DirectoryHandle *) malloc(sizeof(DirectoryHandle));
  FileHandle *f = (FileHandle *) malloc(sizeof(FileHandle));

  SimpleFS *fs = (SimpleFS *) malloc(sizeof(SimpleFS));
  fs->disk = (DiskDriver *) malloc(sizeof(DiskDriver));

  DiskDriver *disk = (DiskDriver *) malloc(sizeof(DiskDriver));
  printf("\n\nCREO DISCO DI 50 BLOCCHI\n");
  DiskDriver_init(disk, "DISK", 50);


  while(scelta != 0){

    printf("\n--- MENU SIMPLE FILE SYSTEM ---\n");
    printf("\n1. INIZIALIZZA FILE SYSTEM IN DISCO GIÀ ESISTENTE\n");
    printf("2. FORMATTAZIONE DEL FILE SYSTEM\n");
    printf("3. CREA FILE NELLA DIRECTORY CORRENTE\n");
    printf("4. STAMPA IL NOME DI TUTTI I FILE DELLA DIRECTORY CORRENTE\n");
    printf("5. APRI FILE DELLA DIRECTORY CORRENTE\n");
    printf("6. CHIUDI FILE CORRENTE\n");
    printf("7. SCRIVI SUL FILE CORRENTE\n");
    printf("8. LEGGI IL FILE CORRENTE\n");
    printf("9. SPOSTA IL CURSORE DEL FILE CORRENTE\n");
    printf("10. CAMBIA DIRECTORY\n");
    printf("11. CREA UNA DIRECTORY NELLA DIRECTORY CORRENTE\n");
    printf("12. RIMUOVI FILE O DIRECTORY\n");
    printf("0. CHIUDERE IL FILE SYSTEM\n\n");

    printf("Scegliere una opzione: ");
    scanf("%d", &scelta);

      switch(scelta){
        case 1:
        if(disk == NULL){
          printf("ERRORE: DISCO NON INIZIALIZZATO\n");
          return NULL;
        }
        else{
          d = SimpleFS_init(fs, disk);
          printf("DIRECTORY CORRENTE: %s\n", d->dcb->fcb.name);
        }
        break;
        case 2: SimpleFS_format(fs);
                break;
        case 3:
        f = SimpleFS_createFile(d, "primo_file");
        printf("\nDIRECTORY CORRENTE: %s\n", d->dcb->fcb.name);
        printf("\nFILE CORRENTE: %s\n\n\n", f->fcb->fcb.name);
        break;
        case 4:
        if(d != NULL){
        char **dest = (char **) malloc(sizeof(char *) * d->dcb->num_entries);
        printf("DIRECTORY: %s\n", d->dcb->fcb.name);
        printf("ENTRIES: %d\n", d->dcb->num_entries);
        int x = SimpleFS_readDir(dest, d);
        puts("\n\n");
        puts("STAMPO LA LISTA DEI NOMI: ");
        for(int i = 0; i < d->dcb->num_entries; i++){
          puts(dest[i]);
        }
      }

                break;
        case 5:
        printf("DIRECTORY: %s\n", d->dcb->fcb.name);
        printf("ENTRIES: %d\n", d->dcb->num_entries);
        f = SimpleFS_openFile(d, "primo_file");
        printf("FILE APERTO: %s\n", f->fcb->fcb.name);
        break;
        case 6:
        case 7:
        break;
        case 8:
        case 9:
        case 10:
        case 11:
        case 12:

        default: printf("CHIUDO FILE SYSTEM\n");
                 break;
      }
    }
  }
