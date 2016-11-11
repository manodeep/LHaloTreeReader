#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h> //for memcmp

/* for open*/
#include <sys/types.h> 
#include <sys/stat.h>
#include <fcntl.h>

/* for close*/
#include <unistd.h>

#include "utils.h"
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
    struct lhalotree *all_trees = read_entire_lhalotree(filename, &ntrees, &totnhalos, &nhalos_per_tree);

#if 0    
    /*
      An example for looping over all the trees and each halo within a tree
     */
    size_t offset = 0;
    for(int itree=0;itree<ntrees;itree++) {
        const int nhalos = nhalos_per_tree[itree];
        const struct lhalotree *tree  = &(all_trees[offset]);
        for(int i=0;i<nhalos;i++) {
            const struct lhalotree halo = tree[i];
            fprintf(stderr,"In tree=%d, halo=%d has Mvir = %lf and MostBoundID = %lld\n", itree, i, halo.Mvir, halo.MostBoundID);
        }
        offset += (size_t) nhalos;
    }
#endif
    
    //Read the first tree in the file -> should be identical to all_trees[0:nhalos-1]
    //the parameters.
    //Change to a random tree number within range to check 
    const int32_t treenum = 20;

    struct timespec t0, t1;
    current_utc_time(&t0);
    struct lhalotree *first = read_single_lhalotree(filename, treenum);
    current_utc_time(&t1);
    
    
    //Run some validation
    size_t offset = 0;
    for(int32_t i=0;i<treenum;i++) {
        offset += nhalos_per_tree[i];
    }
    const struct lhalotree *second = all_trees + offset;
    int exit_status = memcmp(first, second, sizeof(struct lhalotree)*nhalos_per_tree[treenum]);
    if(exit_status == 0) {
        fprintf(stderr,"Validation PASSED. treenum = %d read by the two different routines match. Time = %e ns\n", treenum, REALTIME_ELAPSED_NS(t0, t1));
    } else {
        fprintf(stderr,"ERROR: Validation FAILED. For treenum = %d, the trees read by the two different routines. Bug in code\n", treenum);
    }

    //Now check with pread. First zero out the tree 
    memset(first, 0, sizeof(*first)*nhalos_per_tree[treenum]);
    int fd = open(filename, O_RDONLY, O_NONBLOCK);
    if(fd < 0) {
        fprintf(stderr,"Error in opening input file\n");
        perror(NULL);
        exit_status = fd;
        goto fail;
    }
    const size_t offset_for_tree =  sizeof(int32_t) /* ntrees */
        + sizeof(int32_t)                           /* totnhalos */
        + sizeof(int32_t)*ntrees                    /* nhalos_per_tree */
        + offset*sizeof(struct lhalotree);

    current_utc_time(&t0);
    exit_status |= (pread_single_lhalotree_with_offset(fd, first, nhalos_per_tree[treenum], offset_for_tree)) << 8;
    current_utc_time(&t1);
    
    int cmp_status = memcmp(first, second, sizeof(struct lhalotree)*nhalos_per_tree[treenum]);
    if(cmp_status == 0) {
        fprintf(stderr,"Validation PASSED. treenum = %d read by the two different routines match. Time = %e ns \n", treenum, REALTIME_ELAPSED_NS(t0, t1));
    } else {
        fprintf(stderr,"ERROR: Validation FAILED. For treenum = %d, the trees read by the two different routines. Bug in code\n", treenum);
    }
    exit_status |= cmp_status << 16;
    close(fd);

    
    
    /* My first goto statement -- MS: 3rd Nov, 2016.
       Makes error-handling so much simpler */    
fail:
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


