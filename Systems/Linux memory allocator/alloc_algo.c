/*
 * alloc_algo.c
 *
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include “alloc_algo.h”
#include "memlib.h"

/* If you want debugging output, use the following macro.  When you hand
 * in, remove the #define DEBUG line. */
//#define DEBUG
#ifdef DEBUG
# define dbg_printf(...) printf(__VA_ARGS__)
#define dmm_checkheap(x) mm_checkheap(x)
#else
# define dbg_printf(...)
#define dmm_checkheap(x) 
#endif

#include "contracts.h"

/* do not change the following! */
#ifdef DRIVER
/* create aliases for driver tests */
#define malloc mm_malloc
#define free mm_free
#define realloc mm_realloc
#define calloc mm_calloc
#endif /* def DRIVER */

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8
#define DSIZE 8
#define WSIZE 4   // word size = 4 bytes
#define MINBLOCKSIZE 16  //hdr, footer, l byte block or 2 word address offsets
#define CHUNKSIZE (480)
#define EXT_INIT 0 
#define EXT_EXT 1 
#define GOODAMT 7
#define DIVISOR 3 //divides chunk to 16 and non 16 
#define SMALL 16
#define LARGE 18

/* Global Var */
static char* firstFBP_S ; // first free block bp in 16 free block list
static char* firstFBP_L ;  // in Large list 
static char* prologue ; //prologue block pointer
// static char* firstHBP ; // first block bp for allocation
static char* epilogue ; // epilogue block 
static char* heapStart ; //first address of heap i.e &heap[0]
//static int lastAlloced = 1 ; //set to 1 if last block in heap is alloced
//1 at first as prologue is alloced
/* Inline Function */
#define MIN(x,y) ( ((x)<=(y)) ? (x) : (y))  
#define MAX(x,y) ( ((x) >=(y)) ? (x) : (y))
/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(p) (((size_t)(p) + (ALIGNMENT-1)) & ~0x7)
#define PACK(size, alloc) ((size)|(alloc))
// size must already be double word aligned

/*Deref or write to address*/
#define GET(p) (*(unsigned int *)(p))
#define PUT(p, val) ((*(unsigned int*)(p)) = (val))

/*read size or alloc bit given ptr p */
#define GETSIZE(p) (GET(p) & (~0x7))
#define GETALLOC(p) (GET(p) & 0x1)

/*Given ptr to start of block bp, get hdr and ftr ptr*/
#define HDRP(bp) ((char*)(bp) - WSIZE)
#define FTRP(bp) ((char*)(bp) + (GETSIZE(HDRP(bp))) - DSIZE )

/* given block ptr bp , get next or prev block's block pointers */
#define PREV_BLKP(bp)  ((char *)(bp) - GETSIZE(((char *)(bp) - DSIZE)))
#define NEXT_BLKP(bp)  ((char *)(bp) + GETSIZE(((char *)(bp) - WSIZE)))
#define NEXT_BLKPHDR(bp) ((char *)(bp) + GETSIZE(((char *)(bp) - WSIZE))-WSIZE)

/* given bp get next field ptr of free block */
#define NEXT_FREE(bp) ((char*)(bp) + WSIZE)

/*deref next and prev fields of free block given bp*/

#define DEREF_PREV(bp) (*((unsigned int *)(bp)))
#define DEREF_NEXT(bp) (*((unsigned int*)(NEXT_FREE(bp))))

/*set next and prev fields of free block */
#define SET_NEXT(bp, val) (DEREF_NEXT(bp) = val)
#define SET_PREV(bp, val) (DEREF_PREV(bp) = val)

/* get address of next or prev free block */
#define GET_NEXT(bp) (((char*)(heapStart)) + DEREF_NEXT(bp))
#define GET_PREV(bp) (((char*)(heapStart)) + DEREF_PREV(bp))

/*set or unset, get  prev alloc bits of  bp */
#define SETPREVALLOC(bp) (*(int*)(HDRP(bp)) |= 2) 
// val |= 0b10
#define GETPREVALLOC(bp) (*(int*)(HDRP(bp)) & (0x2))
#define UNSETPREVALLOC(bp) (*(int*)(HDRP(bp)) &= ~(0x2))

/* helper functions */

int isAligned(char* bp) ;
int isValidBlock(char *bp) ;
int inFreeList(char* bp, char* start) ;
int validFreeList(char *bp, int t) ;


static void FL_remove(char *bp, int from)
//remove block from free list given free block bp 
{
  ASSERT(bp != NULL) ;
  ASSERT(GETALLOC(HDRP(bp)) == 0) ;
  ASSERT(bp != epilogue && bp != prologue && bp != heapStart) ;
  char* next_bp; 
  char *prev_bp ;

  if(from == LARGE) {
    if(bp == NULL) return ;
    next_bp = (char*)GET_NEXT(bp) ;
    prev_bp = (char*)GET_PREV(bp) ;
    if(next_bp == heapStart && prev_bp == heapStart) {
      firstFBP_L = NULL ; // empty free list
      return ;
    }
    else if(next_bp == heapStart) { //prev_bp != heapStart
      SET_NEXT(prev_bp, 0) ; //if bp is end node in free list next->prev = 0
      return ;
    }
    else if (prev_bp == heapStart) {
      SET_PREV(next_bp, 0) ; // if bp is start node, next->prev = 0
      firstFBP_L = next_bp ; // change firstFBP to point to new bp
      ASSERT(DEREF_PREV(firstFBP_L) == 0) ;
      return ; 
    }

    // normal case 
    SET_PREV(next_bp, DEREF_PREV(bp)) ;
    SET_NEXT(prev_bp, DEREF_NEXT(bp)) ; // prev->next = curr->next 
    return ;  
  }
  else if(from ==  SMALL) {
    //removed block is size 16

    if(bp == NULL) return ;
    next_bp = (char*)GET_NEXT(bp) ;
    prev_bp = (char*)GET_PREV(bp) ;
    if(next_bp == heapStart && prev_bp == heapStart) {
      firstFBP_S = NULL ; // empty free list
      return ;
    }
    else if(next_bp == heapStart) { //prev_bp != heapStart
      SET_NEXT(prev_bp, 0) ; //if bp is end node in free list next->prev = 0
      return ;
    }
    else if (prev_bp == heapStart) {
      SET_PREV(next_bp, 0) ; // if bp is start node, next->prev = 0
      firstFBP_S = next_bp ; // change firstFBP to point to new bp
      ASSERT(DEREF_PREV(firstFBP_S) == 0) ;
      return ; 
    }
    // normal case 
    SET_PREV(next_bp, DEREF_PREV(bp)) ;
    SET_NEXT(prev_bp, DEREF_NEXT(bp)) ; // prev->next = curr->next 
    return ;
  }
  return ;
}

static void FL_insert(char *bp, int to)
//insert at start of free list
{
  ASSERT(bp != NULL) ;
  ASSERT(GETALLOC(HDRP(bp)) == 0) ;
  ASSERT(isValidBlock(bp)) ;
  if (to == LARGE) {
    ASSERT(GETSIZE(HDRP(bp))> MINBLOCKSIZE) ;
    if(firstFBP_L == NULL) {
      // free list is empty
      firstFBP_L = bp ;
      SET_PREV(bp, 0) ;
      SET_NEXT(bp, 0) ;
      return ;
    }
    else {
      unsigned int offset = firstFBP_L - heapStart ;
      SET_NEXT(bp, offset) ; //bp->next = firstFBP_L
      offset = (bp - heapStart) ;
      //  ASSERT(DEREF_PREV(firstFBP) == 0) ;
      SET_PREV(firstFBP_L, offset) ; // firstFBP->prev = bp
      //ASSERT(DEREF_PREV(firstFBP) == offset) ;
      SET_PREV(bp, 0) ; //bp->prev = NULL 
      firstFBP_L = bp ; //set bp as start of list       
      return ;   
    }
  }
  else if(to == SMALL) {
    //ASSERT(GETSIZE(HDRP(bp)) == 16) ;
    if(firstFBP_S == NULL) {
      firstFBP_S = bp ;
      SET_PREV(bp, 0) ;
      SET_NEXT(bp, 0) ;
      return ;
    }
    else {
      unsigned int offS = firstFBP_S - heapStart ;
      SET_NEXT(bp, offS) ;
      offS = (bp - heapStart) ;
      SET_PREV(firstFBP_S, offS) ;
      SET_PREV(bp, 0);
      firstFBP_S = bp ;
      return ;
    }  
  }
  return ;
}

static char *coalesce(char *bp) 
{
  ASSERT(bp != NULL) ;
 
  size_t prev_alloc = GETPREVALLOC(bp); //changed from using ftr
  size_t next_alloc = GETALLOC(HDRP(NEXT_BLKP(bp)));
  size_t size = GETSIZE(HDRP(bp));
  dmm_checkheap(229) ;

    if (prev_alloc && next_alloc) {            /* Case 1 */      
      int to = (GETSIZE(HDRP(bp)) == SMALL) ? SMALL : LARGE ;
      // find where to insert to 
      FL_insert(bp, to) ;
      
    }

    else if (prev_alloc && !next_alloc) {      
      /* Case 2 , bottom free*/
      int type = (GETSIZE(HDRP(NEXT_BLKP(bp))) == SMALL) ? SMALL : LARGE ;
      FL_remove((char*)(NEXT_BLKP(bp)), type) ; 
      //remove next block from free list of type type
      size += GETSIZE(HDRP(NEXT_BLKP(bp)));
      PUT(HDRP(bp), PACK(size, 0));
      SETPREVALLOC(bp) ; //set to alloc as prev block is not free
      PUT(FTRP(bp), PACK(size,0));
      FL_insert((char*)bp, LARGE) ; // insert new block into free list 
      //guaranteed to insert to large if coalesce takes place
    }
   
    else if (!prev_alloc && next_alloc) {      /* Case 3 */
      //prev is free, next is alloc
      int t ;
      dmm_checkheap(253) ;
   
      if ((GETSIZE(HDRP(PREV_BLKP(bp)))) == SMALL) t =  SMALL ;
      else t = LARGE ;
      FL_remove((char*)(PREV_BLKP(bp)), t) ; 
      // remove prev block from free list
      size += GETSIZE(HDRP(PREV_BLKP(bp)));
      PUT(FTRP(bp), PACK(size, 0));
      bp = PREV_BLKP(bp) ;
      int pA = (GETPREVALLOC(bp)) ; 
      PUT(HDRP(bp), PACK(size, 0));
      (pA) ? SETPREVALLOC(bp) : UNSETPREVALLOC(bp) ; // reinit prev alloc bit 
      FL_insert((char*)bp, LARGE) ; // add new block to free list
    }

    else {                                     /* Case 4 */
      //remove both top and bottom from free list
      int topType = (GETSIZE(HDRP(PREV_BLKP(bp))) == SMALL) ? SMALL : LARGE ;
      int botType = (GETSIZE(HDRP(NEXT_BLKP(bp))) == SMALL) ? SMALL : LARGE ;
      FL_remove((char*)(PREV_BLKP(bp)), topType) ;
      FL_remove((char*)(NEXT_BLKP(bp)), botType) ;
      int PA = GETPREVALLOC(PREV_BLKP(bp)) ;
      size += GETSIZE(HDRP(PREV_BLKP(bp))) + GETSIZE(HDRP(NEXT_BLKP(bp)));
      PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
      (PA) ? SETPREVALLOC(PREV_BLKP(bp)) : UNSETPREVALLOC(PREV_BLKP(bp)) ;
      // Reinits prev alloc bit
      PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
      bp = PREV_BLKP(bp);
      FL_insert((char*)bp, LARGE) ; // insert modified block back
    }
    return (char*)bp ;
}


static char* extend_heap(size_t words, int mode)
{
  size_t size ;
  char *bp ;
  size = (words%2) ? (words+1) * WSIZE : words * WSIZE ;
  ASSERT(size >= MINBLOCKSIZE) ;
  if( (long)(bp = mem_sbrk(size)) == -1) return NULL ;
  int lastAlloced = GETPREVALLOC(((char*)epilogue + WSIZE)) ;
  // check if last block is alloced 
  PUT(HDRP(bp), PACK(size, 0)) ; //change epilogue into hdr of 1 large block
  (lastAlloced) ? SETPREVALLOC(bp) : UNSETPREVALLOC(bp) ;
  PUT(FTRP(bp), PACK(size,0)) ; // init footer of free block
  PUT(HDRP(NEXT_BLKP(bp)), PACK(0,1)) ; //new epilogue header
  epilogue = HDRP(NEXT_BLKP(bp)) ; // set global eiplogue var

  if(mode == EXT_INIT) //if func called to init mem
    {
      // firstFBP = bp ; //set first free block global ptr to bp
      //(*((int *)(firstFBP))) = 0 ; //set prev field of first free block to NULL 
      //*((int*)(NEXT_FREE(firstFBP))) = 0 ; 
      //set next field of free block to NULL
      unsigned int sSize = size/DIVISOR ; //block to be placed in smallList
      PUT(HDRP(bp), PACK(sSize, 0)) ; //init block for small list
      PUT(FTRP(bp), PACK(sSize, 0)) ;
      char *end = NEXT_BLKP(bp) ;
      char *start = bp ;
      PUT(HDRP(start), PACK(SMALL,0)) ;
      (lastAlloced) ? SETPREVALLOC(bp) : UNSETPREVALLOC(bp) ; 
      //means last block before extend was set
      PUT(FTRP(start), PACK(SMALL,0)) ;
      FL_insert(start, SMALL) ;
      for(start =NEXT_BLKP(bp) ; start != end ; start = NEXT_BLKP(start))
	{// init blocks of size 16 and insert to list
	   PUT(HDRP(start), PACK(SMALL,0)) ;
	   PUT(FTRP(start), PACK(SMALL,0)) ;
	   FL_insert(start, SMALL) ;	   
	}
      //FL_insert(bp, SMALL) ;
      //bp = NEXT_BLKP(bp) ;
      //ASSERT(inFreeList(bp, firstFBP_S)) ;
      PUT(HDRP(end), PACK((size-sSize),0)) ;
      //init block for large list
      PUT(FTRP(end), PACK((size-sSize), 0)) ;
      FL_insert(end, LARGE) ; 
      dmm_checkheap(320) ;
      return (char *)bp;
    }
  else // mode == EXT_EXT i.e extend filled heap 
    {
      //FL_insert(bp) ; // insert bp into free list 
      //mm_checkheap(221) ;
      //ASSERT(validFreeList(firstFBP)) ;
      // return (char*)coalesce(bp) ; //set globals in coalesce
      (lastAlloced) ? SETPREVALLOC(bp) : UNSETPREVALLOC(bp) ; 
      //means last block before extend was set
      int type = (size == SMALL) ? SMALL : LARGE ;
      FL_insert((char*)bp, type) ;
      dmm_checkheap(331) ;
      return (char*)bp ;
    }

  dmm_checkheap(223) ; //line 223  CHECKHEAP!!!!!!!!!!!
  return NULL;
}

char *find_fit_s()
{
  if(firstFBP_S == NULL) return NULL ;
  //list is empty
  return firstFBP_S ;
}

char *find_fit(size_t size, int *from) 
//FIXED ISSUE WHERE NULL curr or not enough space during loop
//search free list to find first fit 
//return NULL if no fit found
//from contains type of free list used to obtain block
{
  dmm_checkheap(238) ;
  //ASSERT(validFreeList(firstFBP)) ;
  dmm_checkheap(240) ;
  char *curr;
  size_t bSize  ;
  if(size == SMALL && firstFBP_S != NULL) {
    *from = SMALL ;
    //if(NEXT_BLKP(firstFBP_S) >= epilogue) lastAlloced = 1;
    //set to 1 as last block is allocated
    return firstFBP_S ;
  }
  // immediate match
  curr = firstFBP_L ;
  while(curr != heapStart && curr != NULL)
    //while list not at end
    {
      unsigned nSize, pSize ;
      bSize = GETSIZE(HDRP(curr)) ;
      if(size <= bSize) {
	// implement good fit
	int count = 0;
	char* best = curr ;
	if(GETSIZE(HDRP(best)) == size) {
	  *from = LARGE ;
	  return best ;// best fit
	}
	while(count < GOODAMT && curr != heapStart )
	  {	   
	    pSize = GETSIZE(HDRP(curr)) ; //get size of prev block 
	    curr = (char*)(GET_NEXT(curr)) ;
	    if(curr == heapStart) {
	      *from = LARGE ;
	      return best ; 
	    }
	    nSize = GETSIZE(HDRP(curr)) ;
	    if( (nSize >= size) && (nSize < pSize) ) //nSize is better fit 
	      {
		best = curr ;
		if(nSize == size) {
		  *from = LARGE ;
		  return best ;//best fit achieved
		}
	      }	    
	     
	    count++ ;
	  }
	dmm_checkheap(280) ;
	*from = LARGE ;
	return best ;

      }
      ASSERT(GETALLOC(HDRP(curr)) == 0) ;
      curr = (char *)(GET_NEXT(curr)) ;     
    }
   
  //need more space to  alloc mem on heap  
  //ASSERT(GETSIZE(HDRP(curr)) >= size) ;
  //ASSERT(size%4 == 0) ; //not rounding down of words for extend_heap
  if(size == SMALL) {
    *from = SMALL ;
    extend_heap(CHUNKSIZE/WSIZE, EXT_INIT) ;
    curr = firstFBP_S ;
  }
  // init mode divides chunk into small and large
  else {
    *from = LARGE ;
    curr = extend_heap((MAX(size,CHUNKSIZE)/WSIZE), EXT_EXT) ; 
    //EXT mode just adds chunk to list 
  }
 
  // dbg_printf("NO FUCKING SPACE, firstFBPS = %d but size = %d\n",(int)GETSIZE(HDRP(firstFBP)), (int)size);
  ASSERT(size <= GETSIZE(HDRP(curr))) ;
  return curr ; // NULL if no space found after search 
}

void place(char *bp, size_t size, int from)
//allocate to block and update free list,split block if necessary
//place called after find_fit in MALLOC
//size is block including header and footer
{
  ASSERT(GETALLOC(HDRP(bp)) == 0) ;
  ASSERT(size <= GETSIZE(HDRP(bp))) ;
  ASSERT(size % ALIGNMENT == 0) ;
  dmm_checkheap(428) ;
  //bp is empty block 
  size_t bSize = GETSIZE(HDRP(bp)) ;
  int type = (from == SMALL) ? SMALL : LARGE ; //size or bSize
  int pA = GETPREVALLOC(bp) ;
  if(bSize - size < MINBLOCKSIZE)
    {
      FL_remove(bp, type) ; 
      PUT(HDRP(bp), PACK(bSize, 1)) ;
      (pA) ? SETPREVALLOC(bp) : UNSETPREVALLOC(bp) ; //reinit prevalloc 
      //      PUT(FTRP(bp), PACK(bSize, 1)) ;
      //DOnt have to insert ftr for alloc blocks
      //if(NEXT_BLKP(bp) >= epilogue) lastAlloced = 1;
      // only in this case is the last block alloced
      SETPREVALLOC(NEXT_BLKP(bp)) ;
      //set prev alloc bit of next block's header
    }
  else if(bSize - size == MINBLOCKSIZE) {
    FL_remove(bp, type) ;
    PUT(HDRP(bp) , PACK(size, 1)) ; //alloced no ftr
    (pA) ? SETPREVALLOC(bp) : UNSETPREVALLOC(bp) ; //reinit prevalloc 
    bp = NEXT_BLKP(bp) ; 
    PUT(HDRP(bp), PACK(SMALL, 0)) ;
    SETPREVALLOC(bp) ;
    // set the prev alloc bit of newly formed free block
    PUT(FTRP(bp), PACK(SMALL,0)) ;
    coalesce(bp) ;    
  }
  else if(bSize - size > MINBLOCKSIZE)
    {
      ASSERT((bSize-size) % ALIGNMENT == 0) ;
      // check if new block size will mess up alignment 
      FL_remove(bp, type) ; //remove bp from large free list
      PUT(HDRP(bp) ,PACK(size, 1)) ;
      (pA) ? SETPREVALLOC(bp) : UNSETPREVALLOC(bp) ; //reinit prevalloc 
      //PUT(FTRP(bp), PACK(size,1)) ;
      ASSERT(isValidBlock(bp)) ;
      bp = NEXT_BLKP(bp) ;
      PUT(HDRP(bp) ,PACK(bSize - size, 0));
      SETPREVALLOC(bp) ; // set prev alloc bit of new block
      PUT(FTRP(bp), PACK(bSize-size, 0)) ;
      ASSERT(isValidBlock(bp)) ;
      coalesce(bp) ; //insert split block back to free list
    }
  dmm_checkheap(278) ; //CHECKHEAP!!!!!!!!!!!!!!!!!!!!!
  return ;
}

/*
 * Initialize: return -1 on error, 0 on success.
 */
int mm_init(void) 
{

  if((heapStart = mem_sbrk(4 * WSIZE)) == (void *)-1) return -1 ;
  //allocation failed
  //heapStart contains first byte of heap, will not change
  PUT(heapStart, 0) ; //alignment padding
  PUT(heapStart + WSIZE , PACK(DSIZE, 1)) ; // prologue header
  PUT(heapStart + (2*WSIZE), PACK(DSIZE,1)) ; //prologue footer
  PUT(heapStart + (3*WSIZE), PACK(0,1)); //epilogue
  prologue = heapStart + WSIZE ;
  epilogue = heapStart + (3*WSIZE) ;
  SETPREVALLOC(epilogue+ WSIZE) ; 
  // sice prologue is alloced, set epilogue's prev alloc
  firstFBP_L = NULL ; // set first free block to epilogue
  firstFBP_S = NULL ;
  //will be free after extend_heap called
  
  extend_heap(CHUNKSIZE, EXT_INIT) ;
  dmm_checkheap(319);
  return 0;
}

/*
 * malloc
 */
void *malloc (size_t size) 
{
  if(size == 0 ) return NULL ;

  size_t aSize = size + (WSIZE) ; // add on hdr only
  if(aSize< MINBLOCKSIZE) aSize = MINBLOCKSIZE ;
  else {
    // aSize > MINBLOCKSIZE
    aSize = aSize + (ALIGNMENT - (aSize % ALIGNMENT)) ;
    ASSERT(aSize % ALIGNMENT == 0 ) ;    
  } 
  //int type = (aSize <= SMALL) ? SMALL : LARGE ; 
  int from ;
  char* bp = find_fit(aSize, &from) ;
  if(bp != NULL) {
    place(bp, aSize, from) ;
    SETPREVALLOC(NEXT_BLKP(bp)) ;
    dmm_checkheap(321) ; //CHECKHEAP!!!!!!!!!!!!!!!!!!!
    return (void*)bp ;
  }
  ASSERT(bp == NULL) ;
  dbg_printf("RETURNED NULL\n");
  return NULL;
}

/*
 * free
 */
void free (void *ptr) {
    if(ptr == NULL) return;
    else if((char*)ptr < (prologue + ALIGNMENT) || (char*)ptr >= epilogue) 
      return ;
    //pointer out of current heap bounds
    char *bp = (char*)ptr ;
    size_t size = GETSIZE(HDRP(bp)) ;
    if(GETALLOC(HDRP(bp)) == 0)  return ;
    ASSERT(GETALLOC(HDRP(bp)) == 1) ;
    // if(NEXT_BLKP(bp) >= epilogue) lastAlloced = 0 ; if freeing the last heap block
    UNSETPREVALLOC(NEXT_BLKP(bp)) ;
    //unset prev alloc bit of next block
    int prevAlloced = GETPREVALLOC(bp) ;
    //check if previos block is alloced
    PUT(HDRP(bp) , PACK(size, 0)) ;
    if(prevAlloced) SETPREVALLOC(bp) ;
    else UNSETPREVALLOC(bp) ; //restores prev alloc bit after PUT
    PUT(FTRP(bp), PACK(size,0));
    dmm_checkheap(545) ;
    coalesce(bp) ;
    dmm_checkheap(361) ;//CHECKHEAP!!!!!!!!!!!!!!!!!!!!!!
    return ;

}

/*
 * realloc - you may want to look at mm-naive.c
 */
void *realloc(void *oldptr, size_t size) {
 

  size_t oldsize;
  char *newptr;
  
  /* If size == 0 then this is just free, and we return NULL. */
  if(size == 0) {
    free(oldptr);
    return 0;
  }

  /* If oldptr is NULL, then this is just malloc. */
  if(oldptr == NULL) {
    return malloc(size);
  }
  ASSERT(GETALLOC(HDRP(oldptr)) == 1) ;
  ASSERT(isValidBlock((char*)oldptr));
  newptr = (char*)malloc(size);

  /* If realloc() fails the original block is left untouched  */
  if(!newptr) {
    return 0;
  }

  /* Copy the old data. */
  oldsize = GETSIZE(HDRP(oldptr)) - WSIZE; //remove size of hdr only
  size = MIN(oldsize, size) ; 
  //ASSERT(GETSIZE(HDRP(newptr)) >= GETSIZE(HDRP(oldptr))) ;
  char* Oldptr = (char*)oldptr;
  ASSERT(size < GETSIZE(HDRP(newptr)) ) ;
  for(size_t i = 0; i < size; i++)
    {
      *(newptr +i) = *(Oldptr + i) ; // copy data
    }
  
  /* Free the old block. */
  free(oldptr);
  
  return newptr;
}

/*
 * calloc - you may want to look at mm-naive.c
 * This function is not tested by mdriver, but it is
 * needed to run the traces.
 */
void *calloc (size_t nmemb, size_t size) {
  size_t mSize = nmemb * size ;
  char * start = malloc(mSize) ;
  if(start == NULL) return NULL ; 
  for(size_t i = 0 ; i<mSize; i++) {
    start[i] = 0 ;
  }
    return start;
}


/*
 * Return whether the pointer is in the heap.
 * May be useful for debugging.
 */
static int in_heap(const void *p) {
    return p <= mem_heap_hi() && p >= mem_heap_lo();
}

/*
 * Return whether the pointer is aligned.
 * May be useful for debugging.
 */
static int aligned(const void *p) {
    return (size_t)ALIGN(p) == (size_t)p;
}

/*
 * mm_checkheap
 */

int hdequalft(char *bp)
//ensures header and footer are same
{
  unsigned int *hdr =(unsigned int *)( HDRP(bp)) ;
  unsigned int *ftr = (unsigned int *)(FTRP(bp)) ;
  if( hdr != NULL && ftr != NULL && (*hdr != *ftr))
    {
      dbg_printf("hdrS = %x , ftrS = %x \n",GETSIZE(hdr), GETSIZE(ftr)) ;
    }
  return ((hdr != NULL && ftr != NULL) && (GETALLOC(hdr) == GETALLOC(ftr)) &&
	  ( GETSIZE(hdr) == GETSIZE(ftr))) ;
					  
  //hdr and ftr have same size and alloc
}

int isAligned(char *bp)
{
  //checks if bp address is aligned and size is aligned 
  int size = GETSIZE(bp) ;
  ASSERT(!(size % ALIGNMENT)) ;
  if((size % ALIGNMENT) != 0) {
    dbg_printf("Not correct size\n") ;
    return 0 ;
  }
  if(GETSIZE(HDRP(bp)) == 0) //at epilogue block
    {
      //dbg_printf("isAligned Failed") ;
      //if((long)bp % ALIGNMENT != WSIZE) dbg_printf("isAligned epillogue \n");
      return (((long)bp % ALIGNMENT) == WSIZE) ;
      //expect epilogue to be 4 byte but not 8 byte aligned
    }
  ASSERT((long)bp % ALIGNMENT ==0) ;
  ASSERT((long)NEXT_BLKP(bp)%ALIGNMENT == 0) ;
  return (((long)bp % ALIGNMENT) == 0)&&((long)NEXT_BLKP(bp)%ALIGNMENT== 0);
  //current bp is aligned and next block bp is aligned      
    
}

int validCoalescing(char * bp)
{
  int isFree = ! (GETALLOC(HDRP(bp))) ;
  if(isFree) // if block is free
    {
      char *next = (NEXT_BLKP(bp)) ; 
      ASSERT(GETALLOC(HDRP(bp)) == 0) ;
      return (GETALLOC(HDRP(next)) == 1) || (HDRP(next) == epilogue);
      // if next block is free/(alloc == 0) return false
      // if next block is 0,is valid as block is last in list
    }
  return 1 ;
}

int isValidBlock(char *bp)
{
  int size = GETSIZE(HDRP(bp)) ;
  int validLength = ((bp + size) == (NEXT_BLKP(bp)));
  // bp + size gives address of next block hdr
  validLength = validLength && (size >= MINBLOCKSIZE) ;
  ASSERT(isAligned(bp)) ;
  //ASSERT(hdequalft(bp)) ;
  //ASSERT(validCoalescing(bp)) ;
  ASSERT(validLength);
  return isAligned(bp)  && validLength;
}

int validFreeList(char *bp, int t)
{
#ifdef DEBUG
  char *hdr ;
  int prevOffset ;
  // int offset = (int)(((long)(HDRP(firstFBP))) - (long)(heapStart)) ;
  // offset between first block and heap start
  char * curr = (t == SMALL) ?firstFBP_S :  firstFBP_L ;
  if(curr == NULL) return 1 ;
  //int nextOffset = DEREF_NEXT(curr) ;
  while(curr != heapStart) //curr == heapStart if next field is NULL/0
    {
      hdr = HDRP(curr) ;
      if(GETALLOC(hdr)) return 0 ; // block is not free 
      if(!isValidBlock(curr)) return 0 ;
      prevOffset = DEREF_PREV(curr) ;
      if((prevOffset != 0 && GETALLOC(HDRP(GET_PREV(curr)))) || 
	 (DEREF_NEXT(curr) != 0 && GETALLOC(HDRP(GET_NEXT(curr)))))
	{ // if prev or next is not free
	  return 0 ;
	}
      ASSERT((GET_NEXT(curr)==heapStart) || GET_PREV(GET_NEXT(curr)) == curr) ;
         
      curr = GET_NEXT(curr) ;
      // dbg_printf("Curr->next  = %d \n", DEREF_NEXT(curr)) ; 
      // nextOffset =  DEREF_NEXT(curr) ;
      //no effect if curr == heapStart
    }
  return 1 ;
#else
  return 1 ;
#endif
}

int inFreeList(char * bp, char *start)
{
  if(bp != NULL && start == NULL) return 0 ;
  while(start != heapStart) {
    if(bp == start) return 1 ;
    start = GET_NEXT(start) ;
  }
  return 0 ;
}

void mm_checkheap(int lineno)
{
#ifdef DEBUG
  if(!validFreeList(NULL, SMALL) || !validFreeList(NULL, LARGE)) exit(1) ;

  if(prologue == NULL || epilogue == NULL) 
    {
      dbg_printf("Pro / Ei is NULL at line %d\n", lineno);
      exit(1) ;
    }
  if((GETSIZE(epilogue)) != 0 || (GETALLOC(epilogue)) != 1) {
    dbg_printf("EPILOGUE not set correctly at line %d\n",lineno) ;
    dbg_printf("Epilogue Size = %d , Alloc = %d \n",GETSIZE(epilogue),GETALLOC(epilogue));
    exit(1) ;
  }
  //if epilogue size is not 0 or alloc is not 1 
  char *proFtr = prologue + (GETSIZE(prologue))/2 ;
  //prologue should be only hdr and ftr by text book convention
  if(GETSIZE(prologue) != GETSIZE(proFtr)) {
    dbg_printf("Prlogue not set correctly at line %d\n",lineno);
    exit(1);
  }
  if(GETALLOC(prologue) != GETALLOC(proFtr)) 
    { dbg_printf("Prologue Alloc not set correctly at line %d\n",lineno);
      exit(1) ;
    }
  char *bp = heapStart + (WSIZE * 4) ; // set to first bp on heap
  while(1)
    {
      if(HDRP(bp) == epilogue) return  ; //reached end of heap
      if(! isValidBlock(bp)) {
	dbg_printf("Not Valid Block at line %d\n",lineno);
	exit(1) ;
      }
      bp = NEXT_BLKP(bp) ;
      
    }
  return ;
#else
  return ;
#endif
}
