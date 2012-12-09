/*
 * tid.h
 * Created by Rye as a part of OS Lab_1.
 * Rye
 * rye.y.cn@gmail.com
 * 2012/09/21
 */
#ifndef TID_H
#define TID_H
#include "bitmap.h"
#define CONFIG_BASE_SMALL		0
//#define TID_MAX_DEFAULT			(CONFIG_BASE_SMALL ? 0x1000 : 0x8000)
//#define TID_MAX_DEFAULT			0x8000 //2^15=32768, max thread id number.
#define TID_MAX_DEFAULT			0x80 //2^7=128
#define PAGE_SHIFT 			12 //2^12
#define PAGE_SIZE 			(1UL << PAGE_SHIFT) //4k
#define BITS_PER_BYTE			8
#define BITS_PER_PAGE			(PAGE_SIZE * 8)
#define BITS_PER_PAGE_MASK		(BITS_PER_PAGE - 1)

#define TID_MAX_LIMIT 			(CONFIG_BASE_SMALL ? PAGE_SIZE * 8 : \
        				(sizeof(long) > 4 ? 4 * 1024 * 1024 : TID_MAX_DEFAULT))
#define RESERVED_TIDS			300

//Atomical macro code
typedef struct {
	int counter;
} atomic_t;
#define ATOMIC_INIT(i)			{(i)}
#define atomic_read(v)			(*(volatile int *)&(v)->counter)
#define atomic_set(v,i)			(((v)->counter) = (i))

//Optimization for C++ compiling.
#define likely(x)			__builtin_expect((x),1)//most likely to be x
#define unlikely(x)			__builtin_expect((x),0)//most unlikely to be x

//Definition part of tid.cc
struct tidmap {
    atomic_t nr_free;
    void *page;
};
#define TIDMAP_ENTRIES 			((TID_MAX_LIMIT + 8*PAGE_SIZE - 1)/PAGE_SIZE/8)

extern void free_tidmap(int tid);
extern int alloc_tidmap();
extern void clear_bit(int tid);
extern int find_next_zero_bit(int tid);

#endif
