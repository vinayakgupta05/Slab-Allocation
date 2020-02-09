
/**********************************************************************

File          : cmpsc473-mm.c

Description   : Slab allocation and defenses

***********************************************************************/
/**********************************************************************
Copyright (c) 2019 The Pennsylvania State University
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
* Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
* Neither the name of The Pennsylvania State University nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
***********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>

#include <unistd.h>
#include <time.h> 
#include "cmpsc473-format-66.h"   // TASK 1: student-specific
#include "cmpsc473-mm.h"

/* Globals */
heap_t *mmheap;
unsigned int canary;

/* Defines */
#define FREE_ADDR( slab ) ( (unsigned long)slab->start + ( slab->obj_size * slab->bitmap->free ))
slab_t *allocSlab(slab_cache_t *slab_cache, unsigned int size);
slab_t *freeSlab(slab_t *currentSlab);

/**********************************************************************

Function    : mm_init
Description : Initialize slab allocation
Inputs      : void
Outputs     : 0 if success, -1 on error

***********************************************************************/

int mm_init(void){
	int i;
	mmheap = (heap_t *)malloc(sizeof(heap_t));
	if (!mmheap) return -1;

	// TASK 2: Initialize heap memory (using regular 'malloc') and 
	//   heap data structures in prep for malloc/free
	//if (posix_memalign(mmheap->start, 16, HEAP_SIZE)) {return -1;}
	//mmheap->start = malloc(HEAP_SIZE + PAGE_SIZE - 1);
	//mmheap->start = mmheap->start + 8; //we have to align it to page size 4096, it will waste 4095 bytes. 
	//mmheap->start = (void *)((unsigned int)mmheap->start + PAGE_SIZE - ((unsigned int)mmheap->start % PAGE_SIZE));
	mmheap->start=myheap.slab_page;
	memset(mmheap->start, 0x0, HEAP_SIZE);
	mmheap->size = HEAP_SIZE / PAGE_SIZE;
	mmheap->bitmap = (bitmap_t *)malloc(sizeof(bitmap_t));
	memset(mmheap->bitmap, 0x0, sizeof(bitmap_t));
	mmheap->bitmap->map = (word_t *)malloc(sizeof(word_t)*((mmheap->size) / 8));
	memset(mmheap->bitmap->map, 0x0, (sizeof(word_t)*((mmheap->size) / 8)));
	mmheap->bitmap->size = mmheap->size;
	for (i = 0; i<mmheap->size; i++){
		clear_bit(mmheap->bitmap->map, i);
	}
	mmheap->bitmap->free = 0;

	//slabA//
	mmheap->slabA = (slab_cache_t *)malloc(sizeof(slab_cache_t));
	memset(mmheap->slabA, 0x0, sizeof(slab_cache_t));
	mmheap->slabA->current = allocSlab(mmheap->slabA, sizeof(struct A));
	mmheap->slabA->ct = 1;
	mmheap->slabA->obj_size = sizeof(allocA_t);
	mmheap->slabA->malloc_fn = my_mallocA;
	mmheap->slabA->free_fn = my_freeA;
	//mmheap->slabA->canary_fm = check_canaryA; *****

	//slabB//
	mmheap->slabB = (slab_cache_t *)malloc(sizeof(slab_cache_t));
	memset(mmheap->slabB, 0x0, sizeof(slab_cache_t));
	mmheap->slabB->current = allocSlab(mmheap->slabB, sizeof(struct B));
	mmheap->slabB->ct = 1;
	mmheap->slabB->obj_size = sizeof(allocB_t);
	mmheap->slabB->malloc_fn = my_mallocB;
	mmheap->slabB->free_fn = my_freeB;
	//mmheap->slabC->canary_fm = check_canaryB;****

	//slabC//
	mmheap->slabC = (slab_cache_t *)malloc(sizeof(slab_cache_t));
	memset(mmheap->slabC, 0x0, sizeof(slab_cache_t));
	mmheap->slabC->current = allocSlab(mmheap->slabC, sizeof(struct C));
	mmheap->slabC->ct = 1;
	mmheap->slabC->obj_size = sizeof(allocC_t);
	mmheap->slabC->malloc_fn = my_mallocC;
	mmheap->slabC->free_fn = my_freeC;
	//mmheap->slabC->canary_fm = check_canaryC;****
	//canary
	//printf("A %d B %d C %d \n",mmheap->slabA->current->bitmap->size,mmheap->slabB->current->bitmap->size,mmheap->slabC->current->bitmap->size);
	//printf("A %d B %d C %d \n",mmheap->slabA->current->obj_size,mmheap->slabB->current->obj_size,mmheap->slabC->current->obj_size);
	//printf("A %p B %p C %p \n",mmheap->slabA->current->bitmap,mmheap->slabB->current->bitmap,mmheap->slabC->current->bitmap);

	// initialize canary
	canary_init();

	//free(mmheap->start);
	//free(mmheap->slabA);
	//free(mmheap->slabB);
	//free(mmheap->slabC);
	//free(mmheap->bitmap->map);
	//free(mmheap->bitmap);
	//free(mmheap);
	return 0;
}
/**********************************************************************

Function    : allocSlab
Description : Allocate from slabs
Inputs      : size: amount of memory to allocate
currentSlab: slab address
Outputs     : address if success, NULL on error

***********************************************************************/

slab_t *allocSlab(slab_cache_t *slab_cache, unsigned int size){
	slab_t *newSlab, *currentSlab;
	int i;
	unsigned int nsize, mobj_size, mnum_objs, slabDatasize, slabMapsize;
	currentSlab = slab_cache->current;

	nsize = size + sizeof(unsigned int) + sizeof(unsigned int);//canary and ct
	if ((nsize % 16)>0){
		mobj_size = ((nsize / 16) + 1) * 16;
	}
	else {
		mobj_size = nsize;
	}
	slabMapsize = (PAGE_SIZE / 16) / sizeof(word_t);
	slabDatasize = sizeof(slab_t) + sizeof(bitmap_t) + slabMapsize; //slab_t+bitmap_t+bitmap->map

	mnum_objs = (PAGE_SIZE - slabDatasize) / mobj_size;
//from here 
	//search for empty slab page 
	for (i = 0; i<mmheap->size; i++){
		if (!get_bit(mmheap->bitmap->map, i)){
			break;
		}
	}
	if (i == (mmheap->size)){
		return NULL;
	}//ERROR : HEAP EXHAUSTED
	set_bit(mmheap->bitmap->map, i); //page allotted from heap for slab
// to here can be replaced with following but

/*  //following code not working test3 fails
	if (mmheap->bitmap->free == (mmheap->size)){
		return NULL;
	}//ERROR : HEAP EXHAUSTED
	set_bit(mmheap->bitmap->map, mmheap->bitmap->free); //page allotted from heap for slab

	//search for empty slab page 
	for (i = 0; i<mmheap->size; i++){
		if (!get_bit(mmheap->bitmap->map, i)){
			break;
		}
	}
	printf("allocated slab %d next free %d \n",mmheap->bitmap->free,i);
	//assign next free slab page
	mmheap->bitmap->free = i;
*/
//currentSlab = myheap.slab_page[i].slab;

//currentSlab->start = myheap.slab_page[i].data;

//currentSlab->bitmap = myheap.slab_page[i].bitmap;

//currentslab->bitmap->map = myheap.slab_page[i].map;

//mmheap->slabA->current = myheap.slab_page[i].slab;

	newSlab = &(myheap.slab_page[i].slab);
	//mmheap->start + (i*PAGE_SIZE) + (PAGE_SIZE - slabDatasize); //slab_t data
	newSlab->start = myheap.slab_page[i].data;
	//mmheap->start + (i*PAGE_SIZE);
	printf("%p newslab start %p i %d \n",newSlab,newSlab->start,i);
	//memset(newSlab->start,0x0,PAGE_SIZE-SLABDATA_SIZE); //not required
//	newSlab->start = mmheap->start + (i*PAGE_SIZE); //again becuase of memset done
	newSlab->state = SLAB_EMPTY;
	//doubly linked list assignments
	if (currentSlab != NULL){
		//printf("Current %p Current-Prev %p Current -Next %p \n",currentSlab,currentSlab->prev,currentSlab->next);
		if (currentSlab->next != currentSlab) {
			currentSlab->next->prev = newSlab;
		}
		if (currentSlab->next != currentSlab) {
			newSlab->next = currentSlab->next;
		}
		else {
			newSlab->next = currentSlab;
		}
		currentSlab->next = newSlab;
		newSlab->prev = currentSlab;
		if (currentSlab->prev == currentSlab)
		{
			currentSlab->prev = newSlab;
		}
		//printf("Now New %p New-Prev %p New -Next %p \n",newSlab,newSlab->prev,newSlab->next);
	}
	else {
		newSlab->prev = newSlab;
		newSlab->next = newSlab;
	}
	//printf("New %p New - Prev %p New - Next %p \n",newSlab,newSlab->prev,newSlab->next);
	newSlab->bitmap = &(myheap.slab_page[i].bitmap);
	//(bitmap_t *)((unsigned int)newSlab + sizeof(slab_t)); //?
	memset(newSlab->bitmap, 0x0, sizeof(bitmap_t));
	newSlab->bitmap->map = myheap.slab_page[i].map;
	//(word_t *)((unsigned int)newSlab + sizeof(slab_t) + sizeof(bitmap_t));
	memset(newSlab->bitmap->map, 0x0, slabMapsize);
	newSlab->bitmap->size = mnum_objs;
	newSlab->bitmap->free = 0;
	printf("slab d map %d size %d newSlab %p bitmap %p map %p diff %d %d\n", slabDatasize,slabMapsize,mnum_objs,newSlab,newSlab->bitmap,newSlab->bitmap->map,(unsigned int) newSlab->bitmap-(unsigned int) newSlab,(unsigned int) newSlab->bitmap->map-(unsigned int) newSlab);
	newSlab->ct = 0;
	newSlab->real_size = size;
	newSlab->obj_size = mobj_size;
	newSlab->num_objs = mnum_objs;
	printf("Slaballocation %p slabData %p real_size %d,obj_size %d,aligned size %d number of objects %d \n",newSlab->start,newSlab,size,nsize,mobj_size,mnum_objs);
	return newSlab;
}
/**********************************************************************

Function    : freeSlab
Description : deallocate from slabs
Inputs      : currentSlab: slab address
Outputs     : address if success, NULL on error

***********************************************************************/
slab_t* freeSlab(slab_t *currentSlab){

	slab_t *prevSlab;
	//set doubly linked list pointers after freeing this slab page ?
	//printf("free slab %p prev %p next %p \n",currentSlab,currentSlab->prev, currentSlab->next);
	prevSlab = currentSlab->prev;
	prevSlab->next = currentSlab->next;
	currentSlab->next->prev = currentSlab->prev;
	currentSlab->state = SLAB_UNASSIGNED;
	//printf("new prev slab %p prev %p next %p \n",prevSlab,prevSlab->prev, prevSlab->next);

	return prevSlab;
}
/**********************************************************************

Function    : my_malloc
Description : Allocate from slabs
Inputs      : size: amount of memory to allocate
Outputs     : address if success, NULL on error

***********************************************************************/

void *my_malloc(unsigned int size)
{
	void *addr = (void *)NULL;
	int i;
	slab_cache_t *slab_cache;
	// TASK 2: implement malloc function for slab allocator
	switch (size) {
	case sizeof(struct A) :
		slab_cache = mmheap->slabA;
		break;
	case sizeof(struct B) :
		slab_cache = mmheap->slabB;
		break;
	case sizeof(struct C) :
		slab_cache = mmheap->slabC;
		break;
	default:
		printf("unknown size for allocation %d A %d B %d C %d \n", size, sizeof(struct A), sizeof(struct B), sizeof(struct C));
		return NULL;
	}
		//search for another slab in this cache having empty slot if any
	if (slab_cache->current->state == SLAB_FULL){
		slab_t *nextSlab = slab_cache->current->next;
		while (nextSlab!=slab_cache->current) {
			if (nextSlab->state != SLAB_FULL) {
				slab_cache->current = nextSlab;
				break;
			}
			else {
				nextSlab = nextSlab->next;
			}
		}
	}

		//allocate new slab if all slabs are full in cache
	if (slab_cache->current->state == SLAB_FULL){
		slab_cache->current = allocSlab(slab_cache,size);
		//printf("allocated slab %p \n",slab_cache->current);
		if (slab_cache->current == NULL) {
			printf("Error-Heap[ exhuasted \n");
			return NULL; //ERROR - Heap Exhausted
		}
		else {
					slab_cache->ct++;
		}
	}
	addr = (void *) FREE_ADDR(slab_cache->current); 
 	set_bit(slab_cache->current->bitmap->map, slab_cache->current->bitmap->free);
	slab_cache->current->ct++;
	// for removing lower 4 bits from address and put count here
	addr = slab_cache->malloc_fn(slab_cache->current, addr);

	//search for next empty slot in slab  
	for (i=0; i<slab_cache->current->bitmap->size;i++){
		if (!get_bit(slab_cache->current->bitmap->map, i)){
			break;
		}
	}
	slab_cache->current->bitmap->free = i;
	//printf("slab slot %d size %d \n",i,slab_cache->current->bitmap->size);
	if (slab_cache->current->bitmap->free==slab_cache->current->bitmap->size) {
		slab_cache->current->state = SLAB_FULL;
	}
	else {
			slab_cache->current->state=SLAB_PARTIAL;
	}
	
	return addr;
}
/**********************************************************************

Function    : my_mallocA
Description : Allocate from slab A
Inputs      : size: amount of memory to allocate
Outputs     : address if success, NULL on error

***********************************************************************/
void *my_mallocA(slab_t *slab, void *addr)
{
	allocA_t caddr;
	unsigned int mask;
//	caddr = (allocA_t *)addr;
	caddr.canary = canary;
	if (caddr.ct < 0xf) { caddr.ct++; }
	else { caddr.ct = 1; }
	memcpy(addr, &caddr, sizeof(caddr));
	mask = ALIGN_MASK + caddr.ct;
	addr = (void *)((unsigned int)addr & mask);
	return addr;
}
/**********************************************************************

Function    : my_mallocB
Description : Allocate from slab B
Inputs      : size: amount of memory to allocate
Outputs     : address if success, NULL on error

***********************************************************************/
void *my_mallocB(slab_t *slab, void *addr)
{
	allocB_t caddr;
	unsigned int mask;
//	caddr = (allocA_t *)addr;
	caddr.canary = canary;
	if (caddr.ct < 0xf) { caddr.ct++; }
	else { caddr.ct = 1; }
	memcpy(slab->start + slab->bitmap->free*sizeof(allocB_t), &caddr, sizeof(caddr));
	mask = ALIGN_MASK + caddr.ct;
	addr = (void *)((unsigned int)addr & mask);
	return addr;
}
/**********************************************************************

Function    : my_mallocC
Description : Allocate from slab C
Inputs      : size: amount of memory to allocate
Outputs     : address if success, NULL on error

***********************************************************************/
void *my_mallocC(slab_t *slab, void *addr)
{
allocC_t caddr;
	unsigned int mask;
//	caddr = (allocA_t *)addr;
	caddr.canary = canary;
	if (caddr.ct < 0xf) { caddr.ct++; }
	else { caddr.ct = 1; }
	memcpy(slab->start + slab->bitmap->free*sizeof(allocC_t), &caddr, sizeof(caddr));
	mask = ALIGN_MASK + caddr.ct;
	addr = (void *)((unsigned int)addr & mask);
	return addr;
}

/**********************************************************************

Function    : my_free
Description : deallocate from slabs
Inputs      : buf: full pointer (with counter) to deallocate
Outputs     : address if success, NULL on error

***********************************************************************/

void my_free(void *buf)
{
	unsigned int heap_pos, slab_pos, npos ; //, slabDatasize, slabMapsize;
	slab_t *slab = NULL;
	//void *slabStart;
	slab_cache_t *slab_cache;
	//buf = (void *)USE(buf);
	// TASK 2: Implement free function for slab allocator
	if (buf < mmheap->start || buf >= mmheap->start + HEAP_SIZE || buf == NULL)
	{   printf("could not identify address %p to free between  heap start %p and end %p heap size %d \n",buf,mmheap->start,mmheap->start+HEAP_SIZE,HEAP_SIZE);
		return; //error pointer does not belong to heap
	}
	heap_pos = buf - mmheap->start;
	slab_pos = heap_pos / PAGE_SIZE;
	slab = &(myheap.slab_page[slab_pos].slab);
	npos = (buf - slab->start) / slab->obj_size;
	//slabStart = mmheap->start + slab_pos * PAGE_SIZE;
	//printf("free %p heap_pos %d slab_pos %d heapstart %p slabstart %p \n",buf,heap_pos,slab_pos,mmheap->start,slabStart);
	//find out the slot no to run clearbit
	//slabMapsize = (PAGE_SIZE / 16) / sizeof(word_t);
	//slabDatasize = sizeof(slab_t) + sizeof(bitmap_t) + slabMapsize; //slab_t+bitmap_t+bitmap->map
	//slab = &(myheap.slab_page[slab_pos].slab);
	//npos = (buf - slab->start) / slab->obj_size;
	//slabStart + PAGE_SIZE - slabDatasize;
	switch (slab->real_size) {
	case sizeof(struct A) :
		slab_cache = mmheap->slabA;
		break;
	case sizeof(struct B) :
		slab_cache = mmheap->slabB;
		break;
	case sizeof(struct C) :
		slab_cache = mmheap->slabC;
		break;
		
	default:
		//printf("unknown size for free %d A %d B %d C %d \n", slab->real_size, sizeof(struct A), sizeof(struct B), sizeof(struct C));
		return;
	}

	//printf("free %d ct %d buf %p heapstart %p slab_pos %d slabstart %p diff %d objsize %d\n",npos,slab->ct,buf,mmheap->start,slab_pos,slab->start,(unsigned int) buf-(unsigned int) slab->start,slab->obj_size);
	clear_bit(slab->bitmap->map, npos); //next free ?
	slab->bitmap->free = npos;
	slab->ct--;
	if (slab->ct > 0){
		slab->state = SLAB_PARTIAL;
	}
	else{
		slab->state = SLAB_EMPTY;
	}

	if (slab->state == SLAB_EMPTY){
		if (slab->prev != slab || slab->next != slab){
			freeSlab(slab);
			clear_bit(mmheap->bitmap->map, slab_pos); //when empty
			slab_cache->ct--;
			mmheap->bitmap->free = slab_pos;
			//printf("free slab page %d \n",slab_pos);
		}
	}
	slab_cache->free_fn(slab, buf);//not really required. can be implemented here as well
	return;
}

/**********************************************************************

Function    : my_freeA
Description : deallocate from slab A
Inputs      : buf: full pointer (with counter) to deallocate
Outputs     : address if success, NULL on error

***********************************************************************/
void my_freeA(slab_t *slab, void *addr)
{
	if (slab == mmheap->slabA->current){
		mmheap->slabA->current = slab->prev;
		//	printf("current slab A  changed %p",mmheap->slabA->current);
	}

	return;
}
/**********************************************************************

Function    : my_freeB
Description : deallocate from slab B
Inputs      : buf: full pointer (with counter) to deallocate
Outputs     : address if success, NULL on error

***********************************************************************/
void my_freeB(slab_t *slab, void *addr)
{
	if (slab == mmheap->slabB->current){
		mmheap->slabB->current = slab->prev;
	}
	return;
}
/**********************************************************************

Function    : my_freeC
Description : deallocate from slab C
Inputs      : buf: full pointer (with counter) to deallocate
Outputs     : address if success, NULL on error

***********************************************************************/
void my_freeC(slab_t *slab, void *addr)
{
	if (slab == mmheap->slabC->current){
		mmheap->slabC->current = slab->prev;
	}
	return;
}


/**********************************************************************

Function    : canary_init
Description : Generate random number for canary - fresh each time
Inputs      :
Outputs     : void

***********************************************************************/

void canary_init(void)
{
	// This program will create different sequence of  
	// random numbers on every program run  
	srand(time(0));
	canary = rand();   // fix this 
	//printf("canary is %d\n", canary);
}


char my_type(void *buf)
{
	unsigned int slab_pos;//,slabDatasize, slabMapsize,  slabStart;
	slab_t *slab = NULL;
	if (buf < mmheap->start || buf >= mmheap->start + HEAP_SIZE || buf == NULL)
	{
		return 'U'; //error pointer does not belong to heap
	}
	slab_pos = (buf - mmheap->start) / PAGE_SIZE;

	slab = &(myheap.slab_page[slab_pos].slab); 
	//(slab_t *)(slabStart + PAGE_SIZE - slabDatasize);
	switch (slab->real_size) {
	case sizeof(struct A) :
		return 'A';
	case sizeof(struct B) :
		return 'B';
	case sizeof(struct C) :
		return 'C';
	default:
		return 'U';
	}
}


/**********************************************************************

Function    : check_canary
Description : Find canary for obj and check against program canary
Inputs      : addr: address of object
size: size of object to find cache
Outputs     : 0 for success, -1 for failure

***********************************************************************/

int check_canary(void *addr)
{
	// TASK 3: Implement canary defense
	char typ;
	unsigned int heap_pos, slab_pos, npos;
	slab_t *slab;
	allocA_t aaddr; allocB_t baddr; allocC_t caddr;
	heap_pos = addr - mmheap->start;
	slab_pos = heap_pos / PAGE_SIZE;
	slab = &(myheap.slab_page[slab_pos].slab);
	npos = (addr - slab->start) / slab->obj_size;
	typ = my_type(addr);
	//printf("canary %c\n", typ);
	switch (typ) {
	case 'A':
		memcpy(&aaddr, slab->start+npos*sizeof(allocA_t) , sizeof(allocA_t));
		if (aaddr.canary != canary) return -1;
		break;
	case 'B':
		memcpy(&baddr, slab->start+npos*sizeof(allocB_t) , sizeof(allocB_t));
		if (baddr.canary != canary) return -1;
		break;
	case 'C':
		memcpy(&caddr, slab->start+npos*sizeof(allocC_t) , sizeof(allocC_t));
		if (caddr.canary != canary) return -1;
		break;
	}
	return 0;
}


/**********************************************************************

Function    : check_type
Description : Verify type requested complies with object
Inputs      : addr: address of object
type: type requested
Outputs     : 0 on success, -1 on failure

***********************************************************************/

int check_type(void *addr, char type)
{
	// TASK 3: Implement type confusion defense
	if (my_type(addr) != type) return -1;
	return 0;
}


/**********************************************************************

Function    : check_count
Description : Verify that pointer count equals object count
Inputs      : addr: address of pointer (must include metadata in pointer)
Outputs     : 0 on success, or -1 on failure

***********************************************************************/

int check_count(void *addr)
{
	// TASK 3: Implement free count defense
	char typ; unsigned int count;
	unsigned int heap_pos, slab_pos, npos;
	slab_t *slab;
	allocA_t aaddr; allocB_t baddr; allocC_t caddr;
	heap_pos = addr - mmheap->start;
	slab_pos = heap_pos / PAGE_SIZE;
	slab = &(myheap.slab_page[slab_pos].slab);
	npos = (addr - slab->start) / slab->obj_size;
	typ = my_type(addr);
	count = (unsigned int)addr & 0x00000000000000000f; //lower 4 bits from addr
	switch (typ) {
	case 'A':
		memcpy(&aaddr, slab->start+npos*sizeof(allocA_t) , sizeof(allocA_t));
		if (aaddr.ct != count) return -1;
		break;
	case 'B':
		memcpy(&baddr, slab->start+npos*sizeof(allocA_t) , sizeof(allocA_t));
		if (baddr.ct != count) return -1;
		break;
	case 'C':
		memcpy(&caddr, slab->start+npos*sizeof(allocA_t) , sizeof(allocA_t));
		if (caddr.ct != count) return -1;
		break;
	}
	return 0;
}



/**********************************************************************

Function    : set/clear/get_bit
Description : Bit manipulation functions
Inputs      : words: bitmap
n: index in bitmap
Outputs     : cache if success, or NULL on failure

***********************************************************************/

void set_bit(word_t *words, int n) {
	words[WORD_OFFSET(n)] |= (1 << BIT_OFFSET(n));
}

void clear_bit(word_t *words, int n) {
	words[WORD_OFFSET(n)] &= ~(1 << BIT_OFFSET(n));
}

int get_bit(word_t *words, int n) {
	word_t bit = words[WORD_OFFSET(n)] & (1 << BIT_OFFSET(n));
	return bit != 0;
}


/**********************************************************************

Function    : print_cache_slabs
Description : Print current slab list of cache
Inputs      : cache: slab cache
Outputs     : void

***********************************************************************/

int print_cache_slabs(slab_cache_t *cache)
{
	slab_t *slab = cache->current;
	int count = 0;
	printf("Cache %p has %d slabs\n", cache, cache->ct);
	do {
		printf("slab: %p; prev: %p; next: %p\n", slab, slab->prev, slab->next);
		count += 1;
		slab = slab->next;
	} while (slab != cache->current && count <(cache->ct + 5));
	return count;
}


/**********************************************************************

Function    : get_stats/slab_counts
Description : Print stats on slab page and object allocations
Outputs     : void

***********************************************************************/

void slab_counts(slab_cache_t *cache, unsigned int *slab_count, unsigned int *object_count){
	slab_t *slab = cache->current;
	int i;
	unsigned int orig_count;

	*slab_count = 0;
	*object_count = 0;
	//printf("%p now next slab %p cachecurrent %p;\n",slab,slab->next,cache->current);
	if (slab == NULL) {
		return;
	}
	do {
		(*slab_count)++;

		// set orig to test objects per slab
		orig_count = *object_count;

		// count objects in slab
		for (i = 0; i < slab->bitmap->size; i++) {
			if (get_bit(slab->bitmap->map, i)) {

				//printf("*** Object count in object count in slab %p: %d:%d\n", 
				//    slab, *object_count - orig_count, slab->ct);
				(*object_count)++;
			}
		}

		if ((*object_count - orig_count) != slab->ct) {
			printf("*** Discrepancy in object count in slab %p: %d:%d\n",
				slab, *object_count - orig_count, slab->ct);
		}

		//printf("%p now next slab %p cachecurrent %p;\n",slab,slab->next,cache->current);
		slab = slab->next;
	} while (slab != cache->current);

	if (*slab_count != cache->ct) {
		printf("*** Discrepancy in slab page count in cache %p: %d:%d\n", cache, *slab_count, cache->ct);
	}
}

void get_stats(){
	unsigned int slab_count, object_count;
	printf("--- Cache A ---\n");
	//print_cache_slabs(mmheap->slabA);
	slab_counts(mmheap->slabA, &slab_count, &object_count);
	printf("Number of slab pages:objects in Cache A: %d:%d\n", slab_count, object_count);
	printf("--- Cache B ---\n");
	slab_counts(mmheap->slabB, &slab_count, &object_count);
	printf("Number of slab pages:objects in Cache B: %d:%d\n", slab_count, object_count);
	printf("--- Cache C ---\n");
	slab_counts(mmheap->slabC, &slab_count, &object_count);
	printf("Number of slab pages:objects in Cache C: %d:%d\n", slab_count, object_count);
}
