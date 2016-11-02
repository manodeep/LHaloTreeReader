#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>

#include "utils.h"
#include "read_lhalotree.h"

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


