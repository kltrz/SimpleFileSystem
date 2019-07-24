#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include "disk_driver.c"



#define BLOCK_SIZE 512


int main() {
  BitMapEntryKey k = BitMap_blockToIndex(5);
  printf("%s\n", "Bit numero 5");
  printf("%d\n", k.entry_num);
  printf("%d\n", k.bit_num);
  printf("%s%d\n","Index to Block: ", BitMap_indexToBlock(k.entry_num,k.bit_num));
  printf("\n\n");

  k = BitMap_blockToIndex(27);
  printf("%s\n", "Bit numero 27");
  printf("%d\n", k.entry_num);
  printf("%d\n", k.bit_num);
  printf("%s%d\n","Index to Block: ", BitMap_indexToBlock(k.entry_num,k.bit_num));
  printf("\n\n");

  k = BitMap_blockToIndex(35);
  printf("%s\n", "Bit numero 35");
  printf("%d\n", k.entry_num);
  printf("%d\n", k.bit_num);
  printf("%s%d\n","Index to Block: ", BitMap_indexToBlock(k.entry_num,k.bit_num));
  printf("\n\n");

  k = BitMap_blockToIndex(40);
  printf("%s\n", "Bit numero 40");
  printf("%d\n", k.entry_num);
  printf("%d\n", k.bit_num);
  printf("%s%d\n","Index to Block: ", BitMap_indexToBlock(k.entry_num,k.bit_num));
  printf("\n\n");

  puts("<----------Prueba ChartoBin y STRTOL---------->");
  puts("CHARTOBIN");
  char bitsofchar[8];
  char bitsofcharnew[8];
  char c = '0';
  chartobin(c,bitsofchar);
  printf("%s%c\n", "Char convertida: ",c);
  printf("%s%s\n\n", "bits del char: ",bitsofchar);

  puts("STRTOL");
  char y = strtol(bitsofchar,0, 2);
  printf("%s%s\n", "bits del char: ",bitsofchar);
  printf("%s%c\n", "Char convertida: ",c);
  printf("%s%c\n", "Char nueva y: ",y);
  chartobin(y, bitsofcharnew);
  printf("%s%s\n\n", "bits del char convertido: ",bitsofcharnew);


  puts("CHARTOBIN");
  c = 'u';
  chartobin(c,bitsofchar);
  printf("%s%c\n", "Char convertida: ",c);
  printf("%s%s\n\n", "bits del char: ",bitsofchar);

  puts("STRTOL");
  y = strtol(bitsofchar,0, 2);
  printf("%s%s\n", "bits del char: ",bitsofchar);
  printf("%s%c\n", "Char convertida: ",c);
  printf("%s%c\n", "Char nueva y: ",y);
  chartobin(y, bitsofcharnew);
  printf("%s%s\n\n", "bits del char convertido: ",bitsofcharnew);

  puts("<----------Status to int---------->");
  int l = 1;
  char s = l + '0';
  printf("%s%d\n", "Status in int: ",l);
  printf("%s%c\n\n", "Status in char: ",s);

  l = 0;
  s = l + '0';
  printf("%s%d\n", "Status in int: ",l);
  printf("%s%c\n\n", "Status in char: ",s);


puts("<----------Prueba BITMAP_GET---------->");
BitMap bm;
bm.entries = (char *)calloc(3, sizeof(char));
bm.num_bits = 3*8;
int x = BitMap_set(&bm, 26, 1);
printf("%s%d\n\n","Valor del BitMap_set: ", x);
x = BitMap_get(&bm, 0, 1);
printf("%s%d\n\n","Valor de retorno del BitMap_get: ", x);
puts(bm.entries);
bm.entries[2] = '0';
Stampa_BitMap(&bm);
x = BitMap_get(&bm, 0, 1);
printf("%s%d\n\n","Valor de retorno del BitMap_get: ", x);
x = BitMap_set(&bm, 17, 1);
printf("%s%d\n\n","Valor del BitMap_set: ", x);
x = BitMap_get(&bm, 0, 1);
printf("%s%d\n\n","Valor de retorno del BitMap_get: ", x);
bm.entries[0] = '0';
Stampa_BitMap(&bm);
x = BitMap_get(&bm, 0, 1);
printf("%s%d\n\n","Valor de retorno del BitMap_get: ", x);
puts(bm.entries);
x = BitMap_set(&bm, 2, 0);
puts(bm.entries);
printf("%s%d\n\n","Valor de retorno del BitMap_set: ", x);
x = BitMap_get(&bm, 0, 1);
printf("%s%d\n\n","Valor de retorno del BitMap_get: ", x);
puts(bm.entries);

puts("<----------Prueba BITMAP_SET---------->");
x = BitMap_set(&bm, 5, 1);
puts(bm.entries);
printf("%d\n", x);
x = BitMap_set(&bm, 5, 0);
puts(bm.entries);
printf("%d\n", x);
x = BitMap_set(&bm, 17, 1);
puts(bm.entries);
printf("%d\n", x);
x = BitMap_set(&bm, 17, 0);
puts(bm.entries);
printf("%d\n\n", x);

puts("<----------Prueba con DiskDriver---------->");

DiskDriver disk;

DiskDriver_init(&disk, "DISK", 25);
puts("BitMap all'inizio del file: ");
Stampa_BitMap(&(disk.bitmap));


int freeb = DiskDriver_getFreeBlock(&disk, 2);
printf("\nBlocco libero trovato a partire di posizione 2: %d\n", freeb);

char *buf = "SCRITTURA";
char *buf2 = "NEL 24";
char *read = (char*) calloc(BLOCK_SIZE, sizeof(char));
char *let = (char*) calloc(BLOCK_SIZE, sizeof(char));
printf("Scrivendo in posizione 2\n");
int res = DiskDriver_writeBlock(&disk, buf, 2);
printf("Scrivendo in posizione 18\n");
res = DiskDriver_writeBlock(&disk, buf2, 18);

res = DiskDriver_readBlock(&disk, read, 18);
res = DiskDriver_readBlock(&disk, let, 2);

puts(read);
puts(let);

printf("Liberando posizione 2\n");
res = DiskDriver_freeBlock(&disk, 2);
res = DiskDriver_readBlock(&disk, let, 2);

printf("Liberando posizione 23\n");
res = DiskDriver_writeBlock(&disk, buf, 23);

freeb = DiskDriver_getFreeBlock(&disk, 0);
printf("\nBlocco libero trovato a partire di posizione 0: %d\n", freeb);
  // calculating the size of the file 

  /*long int res = lseek(disk.fd, 0L, SEEK_END);
  printf("Size of the file just created is %ld bytes \n", res);
  res = lseek(disk.fd, 0L, SEEK_SET);
  printf("Seteado a set: %ld\n", res);
  int *buff = (int*) calloc(1, sizeof(int));
  int lf = read(disk.fd, buff, 4);
  if(lf == -1){
    fprintf(stderr, "Value of errno: %d\n", errno);
    fprintf(stderr, "Error opening file: %s\n", strerror(errno));
  }
  printf("LEctura de lf: %d\n", lf);
  printf("LEctura de buff: %d\n", *buff);

  res = lseek(disk.fd, 0L, SEEK_CUR);
  printf("Valore del SEEK_CUR: %ld\n", res);
  lf = read(disk.fd, h, *buff);
  printf("LEctura de lf2: %d\n", lf);
  if(lf == -1){
    fprintf(stderr, "Value of errno: %d\n", errno);
    fprintf(stderr, "Error opening file: %s\n", strerror(errno));
  }
  printf("Valore del disk.h: %d\n", h->num_blocks);
  printf("Valore del disk.h2: %d\n", h->bitmap_blocks);
  printf("Valore del disk.h3: %d\n", h->bitmap_entries);
  printf("Valore del disk.h4: %d\n", h->free_blocks);
  printf("Valore del disk.h4: %d\n", h->first_free_block);

  BitMap *bmk = (BitMap*) calloc(1, sizeof(BitMap));
  lf = read(disk.fd, buff, 4);
  printf("LEctura de lf 3: %d\n", lf);
  lf = read(disk.fd, bmk, *buff);
  res = lseek(disk.fd, 0L, SEEK_CUR);
  printf("Valore del SEEK_CUR: %ld\n", res);
  printf("Stampa bits della BitMap: %d\n", bmk->num_bits );
  Stampa_BitMap(bmk);*/

  //Riempimento Header e Disk
  /*h.num_blocks = 16;
  h.bitmap_blocks = 16/8;
  h.bitmap_entries = (h.bitmap_blocks)*sizeof(char);
  h.free_blocks = 16;
  h.first_free_block = 0;
  disk.header = &h;

  BitMap btmap;
  btmap.num_bits = 32;
  btmap.entries = (char*) calloc(4, sizeof(char));

  disk.bitmap = &btmap;*/
  
  
  /*printf("Size of the the DiskDriver header (opening) is %ld bytes \n", sizeof(disk.header));
  printf("Size of the the Disk Bitmap (opening) is %ld bytes \n", sizeof(disk.bitmap));
  printf("Size of the the Disk Bitmap num_bits (opening) is %ld bytes \n", sizeof(disk.bitmap->num_bits));
  printf("Size of the the Disk Bitmap entries (opening) is %ld bytes \n", sizeof(disk.bitmap->entries));
  res = lseek(disk.fd, 0L, SEEK_END);
  printf("Size of the file just created is %ld bytes \n", res);*/
  printf("NUMBITS: %d\n", disk.bitmap.num_bits);
  printf("Blocchi del disco nel Header: %d\n", disk.header->num_blocks);
  printf("Blocchi della BitMap nel Header: %d\n", disk.header->bitmap_blocks);
  printf("Bytes necessari per la BitMap nel Header: %d\n", disk.header->bitmap_entries);
  printf("Blocchi liberi nel Header: %d\n", disk.header->free_blocks);
  printf("Primo bloccho libero nel Header: %d\n\n", disk.header->first_free_block);






  /*res = lseek(disk.fd, 0L, SEEK_SET);
  printf("SEEK_SET a 0--> %ld\n", res);

char *scr0 = "This will be output to testfile.txt (FRASE1)";
char *src1 = "FRASE 2";
char *src2 = "FRASE 3";
char *src3 = "FRASE 4";
char src4[1024];

/*if(write(disk.fd, scr0, BLOCK_SIZE) == -1){
    printf("Errore nella chiamata write\n");
    fprintf(stderr, "Value of errno: %d\n", errno);
    fprintf(stderr, "Error opening file: %s\n", strerror(errno));
    return -1;
  }
  if(write(disk.fd, src1, BLOCK_SIZE) == -1){
    printf("Errore nella chiamata write\n");
    fprintf(stderr, "Value of errno: %d\n", errno);
    fprintf(stderr, "Error opening file: %s\n", strerror(errno));
    return -1;
  }
  if(write(disk.fd, src2, BLOCK_SIZE) == -1){
    printf("Errore nella chiamata write\n");
    fprintf(stderr, "Value of errno: %d\n", errno);
    fprintf(stderr, "Error opening file: %s\n", strerror(errno));
    return -1;
  }
long int position = lseek(disk.fd, 0, SEEK_CUR);
res = lseek(disk.fd, 0L, SEEK_END);
printf("DOPO SEEK_CURR (Write di 3 frasi) %ld bytes \n", position);
printf("File size %ld bytes (sigue siendo) \n", res);
if(write(disk.fd, "LOCO", BLOCK_SIZE) == -1){
    printf("Errore nella chiamata write\n");
    fprintf(stderr, "Value of errno: %d\n", errno);
    fprintf(stderr, "Error opening file: %s\n", strerror(errno));
    return -1;
  }
  res = lseek(disk.fd, 0L, SEEK_END);
  long int end = res;
  printf("DOPO SEEK END 2 %ld bytes (Write di bitmap) \n", res);
  printf("\nSCRITTURA DELLE FRASI COMPLETATA\n\n");
  
  puts("++++++  LETTURA DEL FILE ++++++");

  res = lseek(disk.fd, 0L, SEEK_SET);
  position = lseek(disk.fd, 0, SEEK_CUR);
  printf("Valore SEEK_SET prima della lettura FILE: %ld\n", res);
  char *buffer = (char *) calloc(512, sizeof(char));
  int elem;
  int bytes_read = 0;
  while(bytes_read < end){
  printf("\nValore del FINE file: %ld\n", end);
  printf("Valore del file: %ld\n", res);
  elem = read(disk.fd, buffer, 512);
  position = lseek(disk.fd, 0, SEEK_CUR);
  printf("Valore dela current position: %ld\n", position);
  printf("Valore del ELEM: %d\n", elem);
  puts(buffer);
  bytes_read += elem;
  printf("Bytes_READ: %d\n", bytes_read);
}
puts("\n++++++ FINE LETTURA DEL FILE ++++++\n");

  res = lseek(disk.fd, 256, SEEK_SET);
  printf("Valore del file: %ld, valore aspettato : 256\n", res);
  puts("NUOVO READ");

  if(read(disk.fd, src4, BLOCK_SIZE) == -1){
    printf("Errore nella chiamata read\n");
    fprintf(stderr, "Value of errno: %d\n", errno);
    fprintf(stderr, "Error opening file: %s\n", strerror(errno));
    return -1;
  }
  position = lseek(disk.fd, 0, SEEK_CUR);
  printf("Valore dela current position dopo read: %ld\n", position);
  src4[17] = '\0';
  puts(src4);*/

  
    // calculating the size of the file
/*res = lseek(disk.fd, 0, SEEK_END);*/
/*printf("Size of the file (before closing) is %ld bytes \n", res);
printf("Size of the the DiskDriver (before closing) is %ld bytes \n", sizeof(disk));
printf("Size of the the DiskDriver Bitmap (before closing) is %ld bytes \n", sizeof(disk.bitmap));
printf("Size of the the DiskDriver header (before closing) is %ld bytes \n", sizeof(disk.header));
int blocoffset = (2048%512);
printf("Bytes che avanzano: %d\n", blocoffset);
int bloctouse = (2048/512);
if(blocoffset != 0) bloctouse++;
printf("Blocchi occupati secondo la divisione: %d\n", bloctouse);
puts("\nPRUEBA DI BLOCCHI\n");*/

/*printf("NUMBITS: %d\n", disk.bitmap.num_bits);
//disk.bitmap.num_bits = 32;
printf("NUMBITS: %d\n", disk.bitmap.num_bits);
puts(disk.bitmap.entries);
BitMap b1;
b1.num_bits = 32;
b1.entries = (char *) calloc(4, sizeof(char));
//disk.bitmap.num_bits = 34;
//disk.bitmap.entries = (char *) calloc(4, sizeof(char));
printf("Numero di num_bits en disk.bitmap recien hecho: %d\n", disk.bitmap.num_bits);*/

Stampa_BitMap(&(disk.bitmap));

if(close(disk.fd) == -1){
    printf("Errore nella chiamata close\n");
    fprintf(stderr, "Value of errno: %d\n", errno);
    fprintf(stderr, "Error closing file: %s\n", strerror(errno));
    return -1;
  }
  
  if(remove("DISK") == -1){
    printf("Errore nella chiamata remove\n");
    fprintf(stderr, "Value of errno: %d\n", errno);
    fprintf(stderr, "Error closing file: %s\n", strerror(errno));
    return -1;
  }

}