#include "cache.h"
#include <stdio.h>

#define FIND_ENTRY_ID(cache, index) &cache->data[index*(sizeof(unsigned int) + cache->id_size + cache->entry_size) + sizeof(unsigned int)]

#define FIND_ENTRY(cache, index) &cache->data[(index+1)*(sizeof(unsigned int) + cache->id_size + cache->entry_size) - cache->entry_size]

#define FIND_BLOCK(cache, index) &cache->data[index*(sizeof(unsigned int) + cache->id_size + cache->entry_size)]

/* ###########################################################################
                            CACHE INTERNALS
   ########################################################################### */
struct Cache {
    unsigned int   num_lines;  /* number of entries stored in cache. */
    size_t         entry_size; /* size of an entry. */
    size_t         id_size;    /* size of an identifier. */
    
    unsigned char* data;       /* cache ! */
    unsigned char* buffer;     /* buffer for storage IO. */
    unsigned int   date;       /* current cache date [1]. */

    ext_read       call_read;  /* database read callbck. */
    ext_write      call_write; /* database write callback. */
    ext_cmp        call_cmp;   /* entries compare callback. */
};
/* [1] the cache date correspounds to the last cache access and enables
   to compute the best cache line to replace in case of a miss. */

unsigned int findOldestLine(struct Cache* this) {
    unsigned int i;
    unsigned int oldest_id = 0;
    size_t line_size = sizeof(unsigned int) + this->id_size + this->entry_size;
    void* current_line = this->data;
    void* oldest_line = this->data;

    printf("\tLooking for oldest cache line to replace it:\n");
    for(i=0; i<this->num_lines; i++, current_line += line_size) {
        printf("\t\t%u. Last access at %u", i, *((unsigned int*)current_line));
        if(*((unsigned int*)current_line) == 0) {
                printf(" - never used.\n");
                oldest_id = i;
                break;
        }
        printf("\n");

        if( *((unsigned int*)current_line) < *((unsigned int*)oldest_line) ) {
            oldest_line = current_line;
            oldest_id = i;
        }
    }
    printf("\tOldest cache line is %u.\n", oldest_id);

    return oldest_id;
}

void setCacheLine(struct Cache* this, unsigned int line_id, unsigned int last_hit, void* entry_id, void* entry_data) {
    void* line_start;

    line_start = &this->data[line_id * (this->entry_size+this->id_size+sizeof(unsigned int))];
    memcpy(line_start, &last_hit, sizeof(unsigned int));

    line_start += sizeof(unsigned int);
    memcpy(line_start, entry_id, this->id_size);

    line_start += this->id_size;
    memcpy(line_start, entry_data, this->entry_size);
}

unsigned int getLineById(struct Cache* this, void* id) {
    unsigned int i;

    printf("\tLooking for queried line in cache:\n");
    for(i=0; i < this->num_lines; i++) {
        printf("\t\tfound on line %d: ", i);
        
        if( *((unsigned int*)FIND_BLOCK(this, i)) != 0 && this->call_cmp(id, FIND_ENTRY_ID(this, i)) == 0 ) {
            printf("true\n");
            return i;
        }
        printf("false\n");
    }
    printf("\tLine is not present in cache.\n");

    printf("Searching for data in storage...");
    if(this->call_read(id, this->buffer) == 0) {
        printf("found.\n");

        printf("Replacing oldest cache line by new data.\n");
        i = findOldestLine(this);

        /* write memory line to storage */
        this->call_write(FIND_ENTRY_ID(this, i), FIND_ENTRY(this, i));
        
        /* create new memory line */
        setCacheLine(this, i, this->date, id, this->buffer);

        memset(this->buffer, 0, this->entry_size);

        return i;
    }

    printf("not found.\n");
    printf("Data was neither found in cache nor in memory... sorry :(\n");

    return this->num_lines;
}

/* ###########################################################################
                    CACHE INTERFACE IMPLEMENTATION
   ########################################################################### */
struct Cache* createCache(unsigned int num_lines, size_t entry_size, size_t id_size, ext_read reader, ext_write writer, ext_cmp comparator) {
    printf("createCache():\n");
    printf("\tAllocating %u byte(s) for cache structure... ", sizeof(struct Cache));
    struct Cache* cache = malloc(sizeof(struct Cache));

    if(cache != NULL) {
        printf("success.\n");

        printf("\tAllocatind %u byte(s) for cache data... ", num_lines * (sizeof(unsigned int) + id_size + entry_size));
        unsigned char* data = malloc(num_lines * (sizeof(unsigned int) + id_size + entry_size));
        if(data == NULL) {
            printf("failure, exiting.\n");
            free(cache);
            return NULL;
        }
        printf("success.\n");
        memset(data, 0, num_lines*(sizeof(unsigned int) + id_size + entry_size));

        printf("\tAllocating %zd byte(s) for entry buffer... ", entry_size);
        unsigned char* buffer = malloc(entry_size);
        if(buffer == NULL) {
            printf("failure, exiting.\n");
            free(data);
            free(cache);
            return NULL;
        }
        printf("success.\n");
        memset(buffer, 0, entry_size);

        struct Cache tmp = {
            num_lines,
            entry_size,
            id_size,
            data,
            buffer,
            1,
            reader,
            writer,
            comparator
        };
        *cache = tmp;
    }
    else
        printf("failure, exiting.\n");

    printf("\n");
    return cache;
}

void freeCache(struct Cache* cache) {
    if(cache != NULL) {
        /* TODO: write back cache */
        free(cache->data);
        free(cache->buffer);
        free(cache);
    }
}

void print_cache(struct Cache* t) {
    unsigned int i;
    unsigned char* p = t->data;
    for(i=0; i<t->num_lines; i++) {
        printf("line n.%u = {\n", i);
        printf("\tlast_access = %u\n", *((unsigned int*)p));

        p+=sizeof(unsigned int);
        printf("\tid = %u\n", *((unsigned int*)p));
        
        p+=t->id_size;
        printf("\tvalue = %u\n", *((unsigned int*)p));

        p+=t->entry_size;
        printf("}\n");
    }
}

void read(struct Cache* this, void* id, void* out) {
    unsigned int i;
    unsigned int date = this->date;

    printf("%d. read():\n", this->date++);
   
    i = getLineById(this, id);
    if( i < this->num_lines) { 
        printf("Writing line data to output buffer.\n");

        /* return the entry by writing to the result pointer */
        memcpy(out, FIND_ENTRY(this, i), this->entry_size);

        printf("no problem in cpying to output.\n");

        /* set the last access date of the entry to the current date */
        memcpy(FIND_BLOCK(this, i), &date, sizeof(unsigned int));

        return;
    }

    printf("\n\tLooking for new data in storage:");
    if( this->call_read(id, this->buffer) == 0) {
        printf(" found data, writing old line back to memory and returning new line data to user returning it to user.\n\n");

        return;
    }

    printf("did not found data in storage, we've lost a lot of time for nothing :(\n\n");
}

void write(struct Cache* this, void* id, void* data) {
    printf("%d. write():\n", this->date++);
    this->call_write(id, data);
}
