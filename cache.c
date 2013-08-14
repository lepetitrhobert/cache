#include "cache.h"
#include <stdio.h>

/* ###########################################################################
                            CACHE INTERNALS
   ########################################################################### */

static const unsigned char GUARD_VALUE = 0xaa;

struct LineHeader {
    unsigned int   last_hit;          /* date of the last access to this cache line. */
    unsigned int   db_value_checksum; /* checksum of the database value */
};

struct LineFooter {
    unsigned char  guard; /* the value of a guard is always set to GUARD_VALUE. if it's value is different, then it means
                             an overflow in writing the data occured.*/
};

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
    size_t line_size = sizeof(struct LineHeader) + this->id_size + this->entry_size + sizeof(struct LineFooter);
    
    unsigned int currLineIndex = 0;
    unsigned int currLineLastHit;
    void* currLineHeader = this->data;
    
    unsigned int oldestLineIndex = 0;
    unsigned int oldestLineLastHit = this->date;    

    for(; currLineIndex < this->num_lines; currLineIndex++) {
        currLineLastHit = ((struct LineHeader*)currLineHeader)->last_hit;
        if(currLineLastHit < oldestLineLastHit) {
            oldestLineLastHit = currLineLastHit;
            oldestLineIndex = currLineIndex;
        }

        currLineHeader += line_size;
    }

    printf("{oldest found is %d}", oldestLineIndex);
    return oldestLineIndex;
}

// accessors to different elements of the cache line
struct LineHeader* getHeaderAddress(struct Cache* this, unsigned int index) {
    return (struct LineHeader*) &this->data[index * (sizeof(struct LineHeader) + this->id_size + this->entry_size + sizeof(struct LineFooter))];
}

void* getIdAddress(struct Cache* this, unsigned int index) {
    return (void*) &this->data[index * (sizeof(struct LineHeader) + this->id_size + this->entry_size + sizeof(struct LineFooter)) + sizeof(struct LineHeader)];
}

void* getEntryAddress(struct Cache* this, unsigned int index) {
    return (void*) &this->data[index * (sizeof(struct LineHeader) + this->id_size + this->entry_size + sizeof(struct LineFooter)) + sizeof(struct LineHeader) + this->id_size];
}

struct LineFooter* getFooterAddress(struct Cache* this, unsigned int index) {
    return (struct LineFooter*) &this->data[(index+1) * (sizeof(struct LineHeader) + this->id_size + this->entry_size + sizeof(struct LineFooter)) - sizeof(struct LineFooter)];
}

unsigned int findIdIndex(struct Cache* this, void* id) {
    unsigned int index;
    struct LineHeader* currHeader = NULL;
    void* curr_id = NULL;

    for(index=0; index<this->num_lines; index++) {
        curr_id = getIdAddress(this, index);
        currHeader = getHeaderAddress(this, index);

        if(currHeader->last_hit != 0 && this->call_cmp(curr_id, id) == EQ) {
            return index;
        }
    }

    return this->num_lines;
}

unsigned int checksum(void* data, size_t size) {
    unsigned int sum = 0;
    size_t i;

    for(i=0; i<size; i++) {
        sum += ((unsigned char*)data)[i];
    }
    return sum;
}

void replaceLine(struct Cache* this, unsigned int index, void* id, void* data) {
    this->call_write(getIdAddress(this, index), getEntryAddress(this, index));
    
    getHeaderAddress(this, index)->db_value_checksum = checksum(data, this->entry_size);
    memcpy(getIdAddress(this, index), id, this->id_size);
    memcpy(getEntryAddress(this, index), data, this->entry_size);
    getFooterAddress(this, index)->guard = GUARD_VALUE;
}

void writeEntryToBuffer(struct Cache* this, unsigned int index, void* buffer) {
    void* lineData = getEntryAddress(this, index);
    memcpy(buffer, lineData, this->entry_size);
}

void writeBufferToEntry(struct Cache* this, unsigned int index, void* buffer) {
    void* lineData = getEntryAddress(this, index);
    memcpy(lineData, buffer, this->entry_size);
}

void notifyAccess(struct Cache* this, unsigned int index) {
    struct LineHeader* lineHeader = getHeaderAddress(this, index);
    lineHeader->last_hit = ++this->date;
}

unsigned int isIndexValid(struct Cache* this, unsigned int index) {
    return index < this->num_lines ? 1 : 0;
}

void loadFromStorage(struct Cache* this, void* id) {
    unsigned int replacedIndex = findOldestLine(this);

    if(this->call_read(id, this->buffer) != NO_ERROR) {
        return;
    }
    
    replaceLine(this, replacedIndex, id, this->buffer);
    notifyAccess(this, replacedIndex);
}

void replaceCacheLine(struct Cache* this, unsigned int index, void* id, void* entry) {
    struct LineHeader* header = getHeaderAddress(this, index);
    header->last_hit = this->date;
    header->db_value_checksum = checksum(entry, this->entry_size);

    memcpy( getIdAddress(this, index), id, this->id_size);
    
    memcpy( getEntryAddress(this, index), entry, this->id_size);

    struct LineFooter* footer = getFooterAddress(this, index);
    footer->guard = GUARD_VALUE; // TODO : add overflow checking
}

/* ###########################################################################
                    CACHE INTERFACE IMPLEMENTATION
   ########################################################################### */

struct Cache* createCache(unsigned int num_lines, size_t entry_size, size_t id_size, ext_read reader, ext_write writer, ext_cmp comparator) {
    struct Cache* cache = malloc(sizeof(struct Cache));

    if(cache != NULL) {
        size_t line_size = sizeof(struct LineHeader) + id_size + entry_size + sizeof(struct LineFooter);
        unsigned char* data = malloc(num_lines * line_size);

        if(data == NULL) {
            free(cache);
            return NULL;
        }
        memset(data, 0, num_lines*line_size);

        unsigned char* buffer = malloc(entry_size);
        if(buffer == NULL) {
            free(data);
            free(cache);
            return NULL;
        }
        memset(buffer, 0, entry_size);

        struct Cache tmp = {
            num_lines,
            entry_size,
            id_size,
            data,
            buffer,
            0,
            reader,
            writer,
            comparator
        };
        *cache = tmp;
    }

    return cache;
}

void freeCache(struct Cache* cache) {
    if(cache != NULL) {
        unsigned int index = 0;
        for(; index < cache->num_lines; index++) {
            if(getHeaderAddress(cache, index)->last_hit > 0) {
                cache->call_write(getIdAddress(cache, index), getEntryAddress(cache, index));
            }
        }

        free(cache->data);
        free(cache->buffer);
        free(cache);
    }
}

void print_cache(struct Cache* this, void (*ext_print)(void*, void*)) {
    unsigned int i;
    
    for(i=0; i<this->num_lines; i++) {
        ext_print(getIdAddress(this, i), getEntryAddress(this, i));
    }
}

unsigned int read(struct Cache* this, void* id, void* out) {
    unsigned int index = findIdIndex(this, id);
    if(isIndexValid(this, index) != 0) {
        writeEntryToBuffer(this, index, out);
        notifyAccess(this, index);
        return 1;        
    }

    loadFromStorage(this, id);
    index = findIdIndex(this, id);
    if(isIndexValid(this, index)) {
        return read(this, id, out);
    }

    return 0;
}

unsigned int write(struct Cache* this, void* id, void* data) {
    if(read(this, id, this->buffer) == 0) {
        return 0;
    }

    unsigned int index = findIdIndex(this, id);    
    writeBufferToEntry(this, index, data);
    notifyAccess(this, index);

    return 1;
}
