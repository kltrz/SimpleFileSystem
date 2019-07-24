#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "bitmap.h"


void chartobin(char c, char r[])
{ 
    for (int i = 7; i >= 0; --i){
    	r[7-i] = (c & (1 << i)) ? '1' : '0';
    }

}

// converts a block index to an index in the array,
// and a char that indicates the offset of the bit inside the array
BitMapEntryKey BitMap_blockToIndex(int num){
	BitMapEntryKey ret;
	ret.entry_num = num/8;
	ret.bit_num = (unsigned char)num%8;
	return ret;
}



// converts a bit to a linear index
int BitMap_indexToBlock(int entry, unsigned char bit_num){
	int ret = entry * 8;
	ret += bit_num;
	return ret;
}

// returns the index of the first bit having status "status"
// in the bitmap bmap, and starts looking from position start
int BitMap_get(BitMap* bmap, int start, int status){
	/*poner que el indice no sea mayor de la dimension de la bimap (HACER UN CONTROL)*/
	
	/*if(start > bmlen){
		printf("Errore: limite di posizione passato\n");
		exit(1);
	}*/
	int bmlen = ((*bmap).num_bits)/8;
	char bitsofchar[8];
	char stat = status + '0';
	BitMapEntryKey bmkey = BitMap_blockToIndex(start);
	char c = (*bmap).entries[bmkey.entry_num];
	chartobin(c, bitsofchar);
	int len =strlen(bitsofchar);
	for(int i = bmkey.bit_num; i < len; i++){
		if(bitsofchar[i] == stat){
		return BitMap_indexToBlock(bmkey.entry_num,i);
	}
}
    int counter = bmkey.entry_num+1;
	while(counter < bmlen){
	 c = (*bmap).entries[counter];
	 chartobin(c, bitsofchar);
	 for(int i = 0; i<len; i++){
	 	if(bitsofchar[i] == stat){
		return BitMap_indexToBlock(counter,i);
	    }
	}
	counter++;
}
return -1;
}

// sets the bit at index pos in bmap to status
int BitMap_set(BitMap* bmap, int pos, int status){
	/*poner que el indice no sea mayor de la dimension de la bimap (HACER UN CONTROL)*/
	char bitsofchar[8];
	char stat = status +'0';
	BitMapEntryKey bmkey = BitMap_blockToIndex(pos);
	char c = (*bmap).entries[bmkey.entry_num];
	chartobin(c, bitsofchar);
	bitsofchar[bmkey.bit_num] = stat;
	(*bmap).entries[bmkey.entry_num] = strtol(bitsofchar, 0, 2);
	return pos;


}

void Stampa_BitMap(BitMap* bmap){
	puts("\n***** Stampa della BitMap *****\n");
	int bitmaplen;
	printf("Numbits: %d\n",(bmap->num_bits));
	int bitsadd = bmap->num_bits%8;
	if(bmap->num_bits < 9){
		bitmaplen = 1;
	}
	else{
    if(bitsadd > 0){
      bitmaplen = (bmap->num_bits/8)+1;
    }
    else{ 
    	bitmaplen = (bmap->num_bits)/8;
        }
	}
	/*printf("%s%ld\n","Size of entries: ", strlen(bmap->entries));
	printf("%s%d\n","Valore del bitmaplen: ", bitmaplen);*/
	int counter = 0;
	char bitsofchar[8];
	char c;
	int len =8;
	/*printf("Valore del bitmaplen: %d\n", bitmaplen );
	while(counter < bitmaplen){
	printf("%s%d\n","Valore del 'Counter' in While: ", counter);
	 c = bmap->entries[counter];
	 chartobin(c, bitsofchar);
	 for(int i = 0; i<len; i++){
	 	printf("%c", bitsofchar[i]);
	    }
	    printf("\n");
	    counter++;
	}*/
	counter = 0;
	puts(" ");
	puts("BITMAP: ");
	while(counter < bitmaplen){
	 if(counter == (bitmaplen-1) && (bitsadd != 0)){
	 	c = bmap->entries[counter];
	    chartobin(c, bitsofchar);
	    for(int i = 0; i<bitsadd; i++){
	    	printf("%c", bitsofchar[i]);
	    }
	    printf(" ");
	    counter++;
		}
	 else{
	 c = bmap->entries[counter];
	 chartobin(c, bitsofchar);
	 for(int i = 0; i<len; i++){
	 	printf("%c", bitsofchar[i]);
	    }
	    printf(" ");
	    counter++;
	}
}
puts("\n");
}


