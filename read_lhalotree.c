#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <assert.h>
#include <string.h>

/* Definition of the LHaloTree struct */
#include "datatype.h"

/* Helper function proto-types*/
void * my_malloc(size_t size, uint64_t N) __attribute__((malloc, alloc_size(1,2)));
size_t my_fread(void *ptr, size_t size, size_t nmemb, FILE *stream);
FILE * my_fopen(const char *fname,const char *mode);
int my_fseek(FILE *stream, long offset, int whence);
void usage(int argc, char **argv);

/* Actual useful functions*/
struct output_dtype * read_entire_lhalotree(const char *filename, int *ntrees, int *totnhalos, int **nhalos_per_tree);
struct output_dtype * read_single_lhalotree(const char *filename, const int32_t treenum);

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

void usage(int argc, char **argv)
{
    (void) argc;
    fprintf(stderr,"USAGE: `%s' <lhalotree file>\n",argv[0]);
}    

struct output_dtype * read_entire_lhalotree(const char *filename, int32_t *ntrees, int32_t *totnhalos, int32_t **nhalos_per_tree)
{
    /*
      A simple reader (with error-checking) for standard LHaloTree binary files

      Bytes per element            |   Nelements             | Field
      -----------------------------|-------------------------|------------------------
      4 bytes                      |   1                     | ntrees     (number of trees in this file)
      4 bytes                      |   1                     | totnhalos  (total number of halos summer over all trees in this file)
      4 bytes                      |   ntrees                | nhalos_per_tree (number of halos in *each* tree in this file)
      sizeof(struct output_dtype)  |   totnhalos             | all_trees   (all the halos in this file, should be parsed one tree at a time)

     */


    FILE *fp = my_fopen(filename,"r");
    my_fread(ntrees, sizeof(*ntrees), 1, fp);
    my_fread(totnhalos, sizeof(*totnhalos), 1, fp);
    *nhalos_per_tree = my_malloc(sizeof(**nhalos_per_tree), (uint64_t) *ntrees);
    my_fread(*nhalos_per_tree, sizeof(**nhalos_per_tree), *ntrees, fp);
    struct output_dtype *all_trees = my_malloc(sizeof(*all_trees), *totnhalos);
    my_fread(all_trees, sizeof(*all_trees), *totnhalos, fp);
    fclose(fp);

    return all_trees;
}    


struct output_dtype * read_single_lhalotree(const char *filename, const int32_t treenum)
{
    /*
      Implementation for reading a single tree out of an LHaloTree file
      (Since the file is opened and closed repeatedly - this routine iss very inefficient if many trees are to be read)
    */

    FILE *fp = my_fopen(filename, "r");
    int32_t ntrees;
    my_fread(&ntrees, sizeof(ntrees), 1, fp);
    if(treenum >= ntrees) {
        fprintf(stderr,"Requested tree number = %d is outside the range of possible tree values = [0, %d)\n",
                treenum, ntrees);
        exit(EXIT_FAILURE);
    }
    int32_t totnhalos=0;
    my_fread(&totnhalos, sizeof(totnhalos), 1, fp);
    if(totnhalos <= 0) {
        fprintf(stderr,"Some read error occurred - totnhalos = %d must be positive", totnhalos);
        exit(EXIT_FAILURE);
    }

    size_t nhalos_before_this_tree=0;
    for(int32_t i=0;i<treenum;i++) {
        int32_t nhalos = 0;
        my_fread(&nhalos, sizeof(nhalos), 1, fp);
        if(nhalos <= 0) {
            fprintf(stderr,"Some read error occurred - nhalos = %d for treenum = %d must be positive", nhalos, i);
            exit(EXIT_FAILURE);
        }
        nhalos_before_this_tree += (size_t) nhalos;
    }
    //Read the number of halos in this tree.
    int32_t nhalos = 0;
    my_fread(&nhalos, sizeof(nhalos), 1, fp);
    
    const long bytes_to_tree = (long) (sizeof(int32_t) + sizeof(int32_t) + sizeof(int32_t)*ntrees);//ntrees, totnhalos, nhalos_per_tree in that order
    my_fseek(fp, bytes_to_tree, SEEK_SET);//look closely -> set from the beginning of the file. Then, I don't have to worry about off-by-one errors

    //Now seek to the halos in the actual tree wanted
    my_fseek(fp, (long) (sizeof(struct output_dtype)*nhalos_before_this_tree), SEEK_CUR);//This fseek is relative. 

    //Essentially, if there was already a valid stream, then
    //this would be the body of the function for returning
    //the tree i) file pointer is correctly positioned ii) nhalos is known
    struct output_dtype *tree = my_malloc(sizeof(*tree), nhalos);
    my_fread(tree, sizeof(*tree), nhalos, fp);

    fclose(fp);

    return tree;
}    


int main(int argc, char **argv)
{
    if(argc != 2) {
        usage(argc, argv);
        fprintf(stderr,"exiting\n");
        exit(EXIT_FAILURE);
    }

    char *filename = argv[1];
    int32_t ntrees=0;
    int32_t totnhalos = 0;
    int32_t *nhalos_per_tree=NULL;
    struct output_dtype *all_trees = read_entire_lhalotree(filename, &ntrees, &totnhalos, &nhalos_per_tree);

#if 0    
    /*
      An example for looping over all the trees and each halo within a tree
     */
    size_t offset = 0;
    for(int itree=0;itree<ntrees;itree++) {
        const int nhalos = nhalos_per_tree[itree];
        const struct output_dtype *tree  = &(all_trees[offset]);
        for(int i=0;i<nhalos;i++) {
            const struct output_dtype halo = tree[i];
            fprintf(stderr,"In tree=%d, halo=%d has Mvir = %lf and MostBoundID = %lld\n", itree, i, halo.Mvir, halo.MostBoundID);
        }
        offset += (size_t) nhalos;
    }
#endif
    
    //Read the first tree in the file -> should be identical to all_trees[0:nhalos-1]
    //the parameters.
    //Change to a random tree number within range to check 
    const int32_t treenum = 190;
    struct output_dtype *first = read_single_lhalotree(filename, treenum);
    
    //Run some validation
    size_t offset = 0;
    for(int32_t i=0;i<treenum;i++) {
        offset += nhalos_per_tree[i];
    }
    const struct output_dtype *second = all_trees + offset;
    int exit_status = memcmp(first, second, sizeof(struct output_dtype)*nhalos_per_tree[treenum]);
    if(exit_status == 0) {
        fprintf(stderr,"Validation PASSED. treenum = %d read by the two different routines match. \n", treenum);
    } else {
        fprintf(stderr,"ERROR: Validation FAILED. For treenum = %d, the trees read by the two different routines. Bug in code\n", treenum);
    }
    free(first);
    free(nhalos_per_tree);
    free(all_trees);
    
    return exit_status;
}    
    
