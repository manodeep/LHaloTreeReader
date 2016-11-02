#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>

#include "read_lhalotree.h"

void usage(int argc, char **argv);

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

void usage(int argc, char **argv)
{
    (void) argc;
    fprintf(stderr,"USAGE: `%s' <lhalotree file>\n",argv[0]);
}    


