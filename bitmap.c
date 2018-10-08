#pragma once
#include <string.h>

typedef struct{
  int num_bits;
  char* entries;
}  BitMap;

typedef struct {
  int entry_num;
  char bit_num;
} BitMapEntryKey;

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
	char bitsofchar[8];
	char stat = (char) status;
	BitMapEntryKey bmkey = BitMap_blockToIndex(start);
	char c = bmap.entries[bmkey.entry_num];
	chartobin(c, bitsofchar);
	int len = strlen(bitsofchar);
	int bmlen = strlen(bmap.entries);
	int counter = bmkey.entry_num+1;
	for(int i = bmkey.bit_num; i < len-bmap.bit_num; i++){
		if(bitsofchar[i] == stat){
		return start+i;
	}
}
	while(counter < bmlen){
	 c = bmap.entries[counter];
	 chartobin(c, bitsofchar);
	 for(i = 0; i<len; i++){
	 	if(bitsofchar[i] == stat){
		return counter+i;
	    }
	
}
}
}
// sets the bit at index pos in bmap to status
int BitMap_set(BitMap* bmap, int pos, int status){

	char bitsofchar[8];
	char stat = (char) status;
	BitMapEntryKey bmkey = BitMap_blockToIndex(pos);
	char c = bmap.entries[bmkey.entry_num];
	chartobin(c, bitsofchar);
	bitsofchar[bmkey.bit_num] = stat;
	bmap.entries[bmkey.entry_num] = strtol(bitsofchar, 0, 2);
	return pos;

}

