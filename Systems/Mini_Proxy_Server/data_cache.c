#include <stdio.h> 
#include <string.h> 
#include "csapp.h" 
#include “data_cache.h"  
#include <assert.h> 

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

/* The header struct will be placed in a global variable. At the launch of each thread in proxy, the cache is checked for the presence of the requested web object. If it does not contain the element a buffer is made to store the object contents from the web server and inserted to the cache, evicting any objects if there is not enough space. Else, the object is simply fetched using cache_get and written to the client. Synchronization is maintained by initializing a semphore in eachcache object and the cache header. Cache blocks at the start have their prev field set to NULL while those at the end have their next field set to NULL*/



cache_t *Cache = NULL ;

/* functions */
void cache_init() ;
void cache_free() ;
void cache_insert(char* buf, size_t size, char*server, char*filepath) ;
int in_cache(char* server, char* filepath) ;
cache_line_t *cache_get(char* server, char*filepath) ;
void bring_to_front(cache_line_t *line) ;
void evict() ; 
void print_cache() ; // for debugging purposes

//cache_line_t *make_line() ; //init a new cache line and make its semaphore 

/* end functions */


/*initialize the Cache */
void cache_init()
{
  Cache = Calloc(1, sizeof(cache_t)) ;
  Cache->start = NULL ;
  Cache->end = NULL ;
  Cache->size = 0 ;
  sem_init(&(Cache->lock) , 0,1) ;
  //init semaphore at address of cache->lock
  return ; 
}

/*free cache after usage*/
void cache_free()
{
  if(Cache == NULL) return ;
  cache_line_t *curr = Cache->start ;
  cache_line_t *next ;
  while(curr != NULL)
    {
      //free each cache block
      next = curr->next ;
      //      sem_destroy(&(curr->lock));
      free(curr->object) ;
      free(curr) ;
      curr = next ;
    }
  sem_destroy(&(Cache->lock)) ;
  free(Cache);
  return ;
}

void free_line(cache_line_t *line)
{
  
  sem_destroy(&(line->lock)) ;
  free(line->object) ;
  free(line->server) ;
  free(line->filepath) ;
  free(line) ;
  return ;
}

/*checks if obj exist in Cache, locking will be done outside func */
int in_cache(char* server, char* file) 
{
  if(Cache == NULL) return 0;
  int res = 0 ;
  cache_line_t *curr = Cache->start ;
  while(curr != NULL) 
    {
      
      res = ( (strcmp(server, curr->server) ==0) && 
	     (strcmp(file,curr->filepath) == 0) );
      //checks if server and filepath matches
      if(res == 1) return res ; 
      curr = curr->next ;
      
    }

  return res ;
}



/* evicts objects until new object of size = size fits 
 locking of Cache will be done outside function*/
void evict(size_t size)
{
  size_t lSize ;
  cache_line_t *temp ;
  if(Cache == NULL) return ;
  cache_line_t *curr = Cache->end ; //curr stores last element in cache
  while(1)
    {
      if( ((Cache->size) + size ) <= MAX_CACHE_SIZE)
	{
	  return ;
	}
      lSize = curr->size ;

      P(&(curr->lock)) ; //can only evict if no other thread using the line

      Cache->size -= lSize ;//free up Cache space
      temp = curr ;
      curr = curr->prev ; 
      //removes end block from cache ;
      Cache->end = curr ;
      //set end to new end block            
      
      V(&(curr->lock)) ; // release lock on line as it is already removed
      free_line(temp) ;
    }

  return ;
}


/* inserts buf into the Cache, locking will be done outside */
void cache_insert(char *buf, size_t size, char* server, char*file)
{
  if(in_cache(server, file)) return ;
  cache_line_t * new = Calloc(1, sizeof(cache_line_t)) ;
  sem_init(&(new->lock),0,1) ;
  new->size = size ;
  new->object = buf ;
 
 //necessary steps to prevent string aliasing 
  new->server = Malloc(sizeof(char) * strlen(server)) ;
  new->filepath = Malloc(sizeof(char) * strlen(file)) ;
  strcpy(new->server, server) ;
  strcpy(new->filepath, file) ;

  evict(size) ; //evicts objects if no space, else does nothing
  new->next= Cache->start ;
  if(Cache->start != NULL) new->next->prev = new ;
  new->prev = NULL ;
  Cache->start = new ;
  Cache->size += size ;
  //insert at the start of the Cache linked list
  
  return ;
}

/* brings specified cache_line to the front of Cache. Locking 
   will be done outside function
*/
void bring_to_front(cache_line_t *line)
{
  if(line == NULL) return ;
  cache_line_t *p , *n ;
  n = line->next ;
  p = line->prev ;
  if(p == NULL) return ; //block is already at start of list
  else if(n == NULL && p != NULL) 
    {
      //line is last obj
      line->next = Cache->start ; 
      Cache->end = p ; //change end block to preceeding block
      p->next = NULL ; //terminate end block with NULL
     
      line->prev = NULL ; //terminate line->prev with NULL
      Cache->start->prev = line ; //update field of first block
      Cache->start = line ; //change start line
      
    }
  else //neither p nor n is NULL
    {
      p->next = n ;
      n->prev = p ; 
      //join p and n 
      Cache->start->prev = line ;
      line->next = Cache->start ;
      line->prev = NULL ;
      Cache->start = line ;
      
    }
 
  return ; 
  
} 



/*gets cache_line, lock object if match found so no evict can be done on it, return NULL if no match found. Lock released after a thread is done copying the object*/ 
cache_line_t* cache_get(char* server, char* file)
{
  if(Cache->start == NULL) return NULL; //Cache is empty
  cache_line_t *curr = Cache->start ;
  while(curr != NULL) 
    {
      if( (!strcmp(server, curr->server)) && 
	  (!strcmp(file, curr->filepath)) ) 
	{ 
	  //if match found
	  P(&(curr->lock)) ;
	  bring_to_front(curr) ;
	  return curr ;
	}
      curr = curr->next ;
    }
  return NULL ;
}


void print_cache()
{
  if(Cache == NULL) return ;
  cache_line_t * curr = Cache->start ;
  printf("CACHESIZE = %d\n",(int)Cache->size) ;
  while(curr != NULL)
    {
      printf("Cache CONTAINS S = %s, P = %s\n",curr->server, curr->filepath) ;
      curr = curr->next ;
    }
  return ;
}



