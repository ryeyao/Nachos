/*
 * tid.cc
 * Created by Rye as a part of OS Lab_1.
 * Rye
 * rye.y.cn@gmail.com
 * 2012/09/23
 */

#include "tid.h"

static int last_tid = -1;
static BitMap *_tidmap = new BitMap(TID_MAX_DEFAULT);

void free_tidmap(int tid) {
    //_tidmap->~BitMap();
}
int alloc_tidmap() {
    int tid_max, last, tid;
    tid_max = TID_MAX_DEFAULT;
    last = last_tid;
    tid = last + 1;

    if(tid >= tid_max) {
	printf("tid is %d, out of range\n",tid);
        return -1;
    }
   
    BitMap *tidmap = _tidmap;
    // printf("last tid is %d \n",last);    
    if(!tidmap->NumClear()) {//No more clear bits
	printf("No more clear bits!\n");
	return -1;//I do not schedule threads now
    }
    int _return = find_next_zero_bit(tid);
    //printf("return %d\n",_return);
//    return find_next_zero_bit(tid);    
    return _return;
}

void clear_bit(int tid) {
    _tidmap->Clear(tid);
}

int find_next_zero_bit(int tid) {
    BitMap *tidmap = _tidmap;
    int tid_max = TID_MAX_DEFAULT;
    if(!tidmap->NumClear()) {
        return -1;
    }
    for(int i = tid,j = 0;i%tid_max < tid_max && j < tid_max; i++) {
	
    	if(!tidmap->Test(i)) {
	    tidmap->Mark(i);
            last_tid = i;
            return i;
        }
	if(j == tid_max - 1){
            break;
        }
	
    }
    return -1;
}
