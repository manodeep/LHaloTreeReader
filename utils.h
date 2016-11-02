#pragma once

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

#define REALTIME_ELAPSED_NS(t0, t1)     ((t1.tv_sec - t0.tv_sec)*1000000000.0 + (t1.tv_nsec - t0.tv_nsec))
    
/* Helper function proto-types*/
void * my_malloc(size_t size, uint64_t N) __attribute__((malloc, alloc_size(1,2)));
size_t my_fread(void *ptr, size_t size, size_t nmemb, FILE *stream);
FILE * my_fopen(const char *fname,const char *mode);
int my_fseek(FILE *stream, long offset, int whence);

/*Nano-second timer*/ 
void current_utc_time(struct timespec *ts);
#ifdef __cplusplus
}
#endif

    
