
typedef struct cache_node cache_line_t ;

struct cache_node {
  char* server ;
  char* filepath ;
  cache_line_t* prev ; //prev node
  cache_line_t* next ; //next node
  char* object ; //holds cache object 
  size_t size ; //object size  
  sem_t lock ; //locks access to node
};

struct cache_header {
  cache_line_t *start ;
  cache_line_t *end ;
  size_t size ; // cache size 
  sem_t lock ; //locks cache access

} ;

typedef struct cache_header cache_t ;




extern cache_t *Cache ;

/* initializes the cache */
void cache_init() ;

/*free cache after use */
void cache_free() ;

/*checks if page has been cached*/
int in_cache(char* server, char* filepath) ;

/* retrieve page from cache*/
cache_line_t *cache_get(char* server, char* filepath) ;

/*inserts elem into cache */
void cache_insert(char* elem, size_t size,  char* server, char* filepath) ; 



