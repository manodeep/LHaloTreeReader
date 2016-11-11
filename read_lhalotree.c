#define _FILE_OFFSET_BITS 64

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>
#include <unistd.h> //for pread
#include <sys/types.h> //for off_t

#include "utils.h"
#include "read_lhalotree.h"


int32_t read_ntrees_lhalotree(const char *filename)
{
    int32_t ntrees;
    FILE *fp = my_fopen(filename,"r");
    my_fread(&ntrees, sizeof(ntrees), 1, fp);
    fclose(fp);

    return ntrees;
}

/* Written with tree as a parameter to avoid malloc'ing repeatedly within the function */
size_t read_single_lhalotree_from_stream(FILE *fp, struct lhalotree *tree, const int32_t nhalos)
{
    return my_fread(tree, sizeof(*tree), nhalos, fp);
}

int pread_single_lhalotree_with_offset(int fd, struct lhalotree *tree, const int32_t nhalos, off_t offset)
{
    size_t bytes_to_read = sizeof(*tree) * nhalos;
    ssize_t bytes_left = bytes_to_read;
    size_t curr_offset=0;
    while(bytes_left > 0) {
        ssize_t bytes_read = pread(fd, tree + curr_offset, bytes_left, offset);
        if(bytes_read < 0) {
            fprintf(stderr,"Read error\n");
            perror(NULL);
            return -1;
        }
        offset += bytes_read;
        curr_offset += bytes_read/sizeof(*tree);
        bytes_left -= bytes_read;
    }

    /* I could return bytes_to_read but returning this serves as an independent check that
       too much data was not read. 
     */

    if((curr_offset * sizeof(*tree)) != bytes_to_read) {
        return -1;
    }
    
    return EXIT_SUCCESS;
}

int read_file_headers_lhalotree(const char *filename, int32_t *ntrees, int32_t *totnhalos, int32_t **nhalos_per_tree)
{
    int status=EXIT_SUCCESS;
    FILE *fp = my_fopen(filename,"r");
    size_t nitems = my_fread(ntrees, sizeof(*ntrees), 1, fp);
    if(nitems != 1) {
        status = EXIT_FAILURE;
    }
    nitems = my_fread(totnhalos, sizeof(*totnhalos), 1, fp);
    if(nitems != 1) {
        status = EXIT_FAILURE;
    }
    *nhalos_per_tree = my_malloc(sizeof(**nhalos_per_tree), (uint64_t) *ntrees);
    if(*nhalos_per_tree == NULL) {
        status = EXIT_FAILURE;
    } else {
        my_fread(*nhalos_per_tree, sizeof(**nhalos_per_tree), *ntrees, fp);
    }
    fclose(fp);
    
    return status;
}




struct lhalotree * read_entire_lhalotree(const char *filename, int32_t *ntrees, int32_t *totnhalos, int32_t **nhalos_per_tree)
{
    /*
      A simple reader (with error-checking) for standard LHaloTree binary files

      Bytes per element            |   Nelements             | Field
      -----------------------------|-------------------------|------------------------
      4 bytes                      |   1                     | ntrees     (number of trees in this file)
      4 bytes                      |   1                     | totnhalos  (total number of halos summer over all trees in this file)
      4 bytes                      |   ntrees                | nhalos_per_tree (number of halos in *each* tree in this file)
      sizeof(struct lhalotree)  |   totnhalos             | all_trees   (all the halos in this file, should be parsed one tree at a time)

     */


    FILE *fp = my_fopen(filename,"r");
    my_fread(ntrees, sizeof(*ntrees), 1, fp);
    my_fread(totnhalos, sizeof(*totnhalos), 1, fp);
    *nhalos_per_tree = my_malloc(sizeof(**nhalos_per_tree), (uint64_t) *ntrees);
    my_fread(*nhalos_per_tree, sizeof(**nhalos_per_tree), *ntrees, fp);
    struct lhalotree *all_trees = my_malloc(sizeof(*all_trees), *totnhalos);
    my_fread(all_trees, sizeof(*all_trees), *totnhalos, fp);
    fclose(fp);

    return all_trees;
}    


struct lhalotree * read_single_lhalotree(const char *filename, const int32_t treenum)
{
    /*
      Implementation for reading a single tree out of an LHaloTree file
      (Since the file is opened and closed repeatedly - this routine is very inefficient if many trees are to be read)
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
    my_fseek(fp, (long) (sizeof(struct lhalotree)*nhalos_before_this_tree), SEEK_CUR);//This fseek is relative. 

    //Essentially, if there was already a valid stream, then
    //this would be the body of the function for returning
    //the tree i) file pointer is correctly positioned ii) nhalos is known
    struct lhalotree *tree = my_malloc(sizeof(*tree), nhalos);
    my_fread(tree, sizeof(*tree), nhalos, fp);

    fclose(fp);

    return tree;
}    


