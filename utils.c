#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

#include "utils.h"

void * my_malloc(size_t size, uint64_t N)
{
    void *x = NULL;
    x = malloc(N*size);
    if (x==NULL){
        fprintf(stderr,"malloc for %"PRId64" elements with %zu bytes failed..aborting\n",N,size);
        perror(NULL);
        exit(EXIT_FAILURE);
    }
        
    return x;
}

FILE * my_fopen(const char *fname,const char *mode)
{
    FILE *fp=NULL;
    fp = fopen(fname,mode);
    if(fp == NULL) {
        fprintf(stderr,"Could not open file `%s'\n",fname);
        perror(NULL);
        exit(EXIT_FAILURE);
    }
    return fp;
}

size_t my_fread(void *ptr, size_t size, size_t nmemb, FILE *stream)
{
    size_t nread;
    nread = fread(ptr, size, nmemb, stream);
    if(nread != nmemb) {
        fprintf(stderr,"I/O error (fread) has occured.\n");
        fprintf(stderr,"Instead of reading nmemb=%zu, I got nread = %zu ..exiting\n",nmemb,nread);
        perror(NULL);
        exit(EXIT_FAILURE);
    }
    return nread;
}

int my_fseek(FILE *stream, long offset, int whence)
{
    int err=fseek(stream,offset,whence);
    if(err != 0) {
        fprintf(stderr,"ERROR: Could not seek `%ld' bytes into the file..exiting\n",offset);
        exit(EXIT_FAILURE);
    }
    return err;
}

