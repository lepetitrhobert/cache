#include <stdlib.h>
#include <string.h>

typedef unsigned int (*ext_read)(void*, void*);
typedef unsigned int (*ext_write)(void*, void*);
typedef unsigned int (*ext_cmp)(void*, void*);

static const unsigned int NO_ERROR        = 0;
static const unsigned int INTERNAL_ERROR  = 1;
static const unsigned int INVALID_ID      = 2;

static const unsigned int EQ              = 0;
static const unsigned int GT              = 1;
static const unsigned int LT              = 2;

struct Cache;

/** createCache
 *  Creates a cache structure by specifying the
 *  number of entries stored in cache, the size 
 *  of an entry, the size of an identifier  and
 *  three functions: read and write
 *  and compare.
 *
 *  The read function takes two pointers as parameter:
 *  the entry indentifier and a pointer to write the
 *  entry to it. 
 *  It implements the memory reading and is called
 *  on cache miss.
 *
 *  The write function takes two pointes as parameter:
 *  the entry identifier and a pointer to the entry
 *  the entry identifier and the entry pointer.
 *  It implements the memory writes and is called when
 *  cache entry is updated.
 *
 *  These two function must return 0 if there is no error,
 *  1 if an error occured.
 *
 *  The compare function takes two entries pointers as 
 *  parameters and return 0 if the entries are equal.
 *
 *  @param unsigned int   - number of entries in cache.
 *  @param size_t         - size of an entry.
 *  @param size_t         - size of an identifier.
 *  @param ext_read       - read function.
 *  @param ext_write      - write function.
 *  @param ext_cmp        - compare function.
 *  @return struct Cache* - the fucking cache object !
 */
struct Cache* createCache(unsigned int, size_t, size_t, ext_read, ext_write, ext_cmp);

/** freeCache
 *  Free the memory allocated for the cache and the
 *  cache itself.
 *
 *  @param struct Cache* - cache to free.
 */
void freeCache(struct Cache*);

/** read
 *  Read element identified by "param1".
 *
 *  @param  struct Cache* - database's cache. 
 *  @param  void*         - data identifier.
 *  @param  void*         - data out.
 */
unsigned int read(struct Cache*, void*, void*);

/** write
 *  Write "param2" to database element identi-
 *  fied by "param3"
 *  
 *  @param struct Cache* - database's cache. 
 *  @param void*         - data identifier.
 *  @param void*         - data.
 */
unsigned int write(struct Cache*, void*, void*);

/** print_cache
 *  Call the ext_print function for every cache line. The ext_print
 *  function receives a pointer to the ID and the content of a cache
 *  line as a parameter and should print it's content.
 *  
 *  @param struct Cache*                   - database's cache. 
 *  @param void (*ext_print)(void*, void*) - function printing the data.
 */
void print_cache(struct Cache*, void (*ext_print)(void*, void*));
