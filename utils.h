#pragma once

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Helper function proto-types*/
void * my_malloc(size_t size, uint64_t N) __attribute__((malloc, alloc_size(1,2)));
size_t my_fread(void *ptr, size_t size, size_t nmemb, FILE *stream);
FILE * my_fopen(const char *fname,const char *mode);
int my_fseek(FILE *stream, long offset, int whence);

#ifdef __cplusplus
}
#endif

    
