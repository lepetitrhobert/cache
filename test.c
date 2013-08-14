#include <stdlib.h>
#include <stdio.h>

#include "cache.h"

#define LEN 5

int data[LEN] = {0, 1, 2, 3, 4};

unsigned int read_impl(void* v_id, void* out) {
    int id = *((int*)v_id);

    printf("{read %d: ", id);

    if(id < 0 || id >= LEN) {
        printf("not ok}"); 
        return INVALID_ID;
    }
    else {
        *((int*)out) = data[id];
        printf("ok}");
        return NO_ERROR;
    }
}

unsigned int write_impl(void* v_id, void* in) {
    int id = *((int*)v_id);
    
    printf("{write %d: ", id);
    
    if(id < 0 || id >= LEN) {
        printf("not ok}"); 
        return INVALID_ID;
    }
    else {
        printf("ok}");
        data[id] = *((char*)in);
        return NO_ERROR;
    }
}

unsigned int compare(void* a, void* b) {
    int diff = (*((int*)a) - *((int*)b));

    if(diff > 0) {
        return GT;
    }
    else if(diff < 0) {
        return LT;
    }
    else {
        return EQ;
    }
}

void print_line(void* id, void* value) {
    printf("cache[%d] = %d\n", *(int*)id, *(int*)value);
}

int main(int argc, char** argv) {
    argc++; argc--;
    argv[0] = argv[1];
    
    int i;
    
    printf("print database:\n");
    for(i=0; i<5; i++) {
        printf("\tdatabase[%d] = %d\n", i, data[i]); 
    }

    struct Cache* cache = createCache(5, sizeof(int), sizeof(unsigned int), read_impl, write_impl, compare);

    int out=0;
    printf("\nwrites:\n");
    for(i=0; i<5; i++) {
        out = 4-i;
        write(cache, &i, &out);
        printf("\twrite(%d, %d)\n", i, (4-i));
    }

    printf("\nprint cache:\n");
    print_cache(cache, print_line);
    
    printf("\nreads:\n");
    for(i=0; i<5; i++) {
        read(cache, &i, &out);
        printf("\tread(%d) = %d\n", i, out);
    }
   
    printf("\nprint cache:\n"); 
    print_cache(cache, print_line);

    freeCache(cache);

    printf("\nprint database:\n");
    for(i=0; i<5; i++) {
        printf("\tdatabase[%d] = %d\n", i, data[i]); 
    }

    return EXIT_SUCCESS;
}
