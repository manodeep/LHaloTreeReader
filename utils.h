#pragma once

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

#define REALTIME_ELAPSED_NS(t0, t1)     ((t1.tv_sec - t0.tv_sec)*1000000000.0 + (t1.tv_nsec - t0.tv_nsec))

#ifdef NDEBUG
#define XRETURN(EXP, VAL, ...)                                do{} while(0)
#else
#define XRETURN(EXP, VAL, ...)                                           \
     do { if (!(EXP)) {                                                 \
             fprintf(stderr,"Error in file: %s\tfunc: %s\tline: %d with expression `"#EXP"'\n", __FILE__, __FUNCTION__, __LINE__); \
             fprintf(stderr,__VA_ARGS__);                               \
             fprintf(stderr,ANSI_COLOR_BLUE "Hopefully, input validation. Otherwise, bug in code: please email Manodeep Sinha <manodeep@gmail.com>"ANSI_COLOR_RESET"\n"); \
             return VAL;                                                \
         }                                                              \
     } while (0)
#endif
/* Macro Constants */
//Just to output some colors
#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_RESET   "\x1b[0m"

    
/* Helper function proto-types*/
void * my_malloc(size_t size, uint64_t N) __attribute__((malloc, alloc_size(1,2)));
size_t my_fread(void *ptr, size_t size, size_t nmemb, FILE *stream);
FILE * my_fopen(const char *fname,const char *mode);
int my_fseek(FILE *stream, long offset, int whence);

    char * get_time_string(struct timeval t0,struct timeval t1);
    int my_snprintf(char *buffer,int len,const char *format, ...);
    
/*Nano-second timer*/ 
void current_utc_time(struct timespec *ts);
#ifdef __cplusplus
}
#endif

    
