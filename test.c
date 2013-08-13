#include <stdlib.h>
#include <stdio.h>

#include "cache.h"

#define LEN 20

int data[LEN] = {'.'};

unsigned int read_impl(void* v_id, void* out) {
    int id = *((int*)v_id);

    if(id < 0 || id >= LEN) 
        return INVALID_ID;
    else {
        *((int*)out) = data[id];
        return NO_ERROR;
    }
}

unsigned int write_impl(void* v_id, void* in) {
    int id = *((int*)v_id);
    if(id < 0 || id >= LEN) 
        return INVALID_ID;
    else {
        data[id] = *((char*)in);
        return NO_ERROR;
    }
}

unsigned int compare(void* a, void* b) {
    printf(" {{ %d == %d }} ", *((int*)a), *((int*)b));
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

int main(int argc, char** argv) {
    argc++; argc--;
    argv[0] = argv[1];

    struct Cache* cache = createCache(5, sizeof(int), sizeof(unsigned int), read_impl, write_impl, compare);

    int i;
    int out=0;
    for(i=0; i<5; i++) {
        printf("id = %d\n", i);
        read(cache, &i, &out);
    }

    printf("\n########################################################\n");
    print_cache(cache, NULL);
    printf("########################################################\n");

    for(i=0; i<5; i++) {
        printf("id = %d\n", i);
        read(cache, &i, &out);
    }

    printf("\n########################################################\n");
    print_cache(cache, NULL);
    printf("########################################################\n");

    freeCache(cache);

    return EXIT_SUCCESS;
}
