#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

#include<time.h>

#ifdef __MACH__ // OS X does not have clock_gettime, use clock_get_time
#include <mach/mach_time.h> /* mach_absolute_time -> really fast */
#endif

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


/*
  Can not remember where I (MS) got this from. Fairly sure
  stackoverflow was involved.
  Finally taken from http://stackoverflow.com/a/6719178/2237582 */
void current_utc_time(struct timespec *ts)
{

#ifdef __MACH__ // OS X does not have clock_gettime, use clock_get_time
    static mach_timebase_info_data_t    sTimebaseInfo = {.numer=0, .denom=0};
    uint64_t start = mach_absolute_time();
    if ( sTimebaseInfo.denom == 0 ) {
        mach_timebase_info(&sTimebaseInfo);
    }

    ts->tv_sec = 0;//(start * sTimebaseInfo.numer/sTimebaseInfo.denom) * tv_nsec;
    ts->tv_nsec = start * sTimebaseInfo.numer / sTimebaseInfo.denom;

#if 0
    //Much slower implementation for clock
    //Slows down the code by up to 4x
    clock_serv_t cclock;
    mach_timespec_t mts;
    host_get_clock_service(mach_host_self(), CALENDAR_CLOCK, &cclock);
    clock_get_time(cclock, &mts);
    mach_port_deallocate(mach_task_self(), cclock);
    ts->tv_sec = mts.tv_sec;
    ts->tv_nsec = mts.tv_nsec;
#endif

#else
    //On linux, simply call clock_gettime (link with -lrt, for real-time library)
    clock_gettime(CLOCK_REALTIME, ts);
#endif
}


