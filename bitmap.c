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

// converts a block index to an index in the array,
// and a char that indicates the offset of the bit inside the array
BitMapEntryKey BitMap_blockToIndex(int num){
	BitMapEntryKey ret;
	ret->entry_num = num/8;
	ret->bit_num = (char)num%8;
	return ret;
}

// converts a bit to a linear index
int BitMap_indexToBlock(int entry, uint8_t bit_num){
	int ret = entry * 8;
	ret += bit_num;
	return ret;
}

// returns the index of the first bit having status "status"
// in the bitmap bmap, and starts looking from position start
int BitMap_get(BitMap* bmap, int start, int status){
	char *cstatus = status + '0';
	char *ret = strchr(bmap.entries, cstatus);
	if(ret){
		int index = ret - bmap.entries;
	}
	return index;

}

// sets the bit at index pos in bmap to status
int BitMap_set(BitMap* bmap, int pos, int status){
	bmap.entries[pos] = status + '0';
}
