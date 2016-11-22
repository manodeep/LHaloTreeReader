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
#include "progressbar.h"

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
    int interrupted=0;//for progressbar
    size_t offset;

    /*
      An example for looping over all the trees and each halo within a tree
     */
    offset = 0;
    fprintf(stderr,"\nValidating tree ....\n");
    init_my_progressbar(ntrees, &interrupted);
    for(int itree=0;itree<ntrees;itree++) {
        my_progressbar(itree, &interrupted);
        const int nhalos = nhalos_per_tree[itree];
        const struct lhalotree *tree  = &(all_trees[offset]);
        for(int i=0;i<nhalos;i++) {
            /* const struct lhalotree halo = tree[i]; */
            /* fprintf(stderr,"In tree=%d, halo=%d has Mvir = %lf and MostBoundID = %lld\n", itree, i, halo.Mvir, halo.MostBoundID); */
            if(tree[i].NextProgenitor == -1) continue;
            XRETURN(tree[i].NextProgenitor >=0 && tree[i].NextProgenitor < nhalos, EXIT_FAILURE,
                    "In tree = %d halo = %d, NextProgenitor = %d must be within [0, %d)\n", itree, i, tree[i].NextProgenitor, nhalos);

        }
        offset += (size_t) nhalos;
    }
    finish_myprogressbar(&interrupted);
    fprintf(stderr,"Validating tree .......done. Total number of halos = %zu\n\n", offset);

    /* Check that there is at least one FOF at the final snapshot */
    interrupted=0;
    fprintf(stderr,"Validating number of FOF halos....\n");
    init_my_progressbar(ntrees, &interrupted);
    offset = 0;
    for(int32_t itree=0;itree < ntrees;itree++) {
        my_progressbar(itree, &interrupted);
        const int nhalos = nhalos_per_tree[itree];
        int max_snap=-1;
        struct lhalotree *tree = all_trees + offset;
        for(int32_t i=0;i<nhalos;i++) {
            max_snap=tree[i].SnapNum > max_snap ? tree[i].SnapNum:max_snap;
        }

        int nfofs=0;
        for(int32_t i=0;i<nhalos;i++) {
            if(tree[i].SnapNum == max_snap && tree[i].FirstHaloInFOFgroup == i)  {
                nfofs++;
            }
        }
        if(nfofs == 0) {
            fprintf(stderr,ANSI_COLOR_RED"ERROR: Validation FAILED. For treenum = %d, A single LHalotree must contain *at least* ONE FOF halo at max. snapshot (=%d). "
                    "In the trees read, nfofs=%d"ANSI_COLOR_RESET"\n", itree, max_snap, nfofs);
            interrupted=1;
            break;
        }
        offset += nhalos;
    }
    finish_myprogressbar(&interrupted);
    fprintf(stderr,"Validating number of FOF halos.......done\n\n");
    
    //Read the first tree in the file -> should be identical to all_trees[0:nhalos-1]
    //the parameters.
    //Change to a random tree number within range to check 
    const int32_t treenum = 20;

    struct timespec t0, t1;
    current_utc_time(&t0);
    struct lhalotree *first = read_single_lhalotree(filename, treenum);
    current_utc_time(&t1);
    
    
    //Run some validation
    offset = 0;
    for(int32_t i=0;i<treenum;i++) {
        offset += nhalos_per_tree[i];
    }
    const struct lhalotree *second = all_trees + offset;
    int exit_status = memcmp(first, second, sizeof(struct lhalotree)*nhalos_per_tree[treenum]);
    if(exit_status == 0) {
        fprintf(stderr,ANSI_COLOR_GREEN"Validation PASSED. treenum = %d read by the two different routines match. Time = %e ns", treenum, REALTIME_ELAPSED_NS(t0, t1));
    } else {
        fprintf(stderr,ANSI_COLOR_RED"ERROR: Validation FAILED. For treenum = %d, the trees read by the two different routines. Bug in code (read_single_lhalotree)", treenum);
    }
    fprintf(stderr,ANSI_COLOR_RESET"\n\n");

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
        fprintf(stderr,ANSI_COLOR_GREEN"Validation PASSED. treenum = %d read by the two different routines match. Time = %e ns", treenum, REALTIME_ELAPSED_NS(t0, t1));
    } else {
        fprintf(stderr,ANSI_COLOR_RED"ERROR: Validation FAILED. For treenum = %d, the trees read by the two different routines. "
                "Bug in code (pread_single_lhalotree_with_offset)", treenum);
    }
    fprintf(stderr,ANSI_COLOR_RESET"\n\n");
    
    exit_status |= cmp_status << 16;
    /* close(fd); */



    interrupted=0;
    init_my_progressbar(ntrees, &interrupted);
    offset = 0;
    for(int32_t itree=0;itree < ntrees;itree++) {
        my_progressbar(itree, &interrupted);
        const int nhalos = nhalos_per_tree[itree];
        struct lhalotree *third = my_malloc(sizeof(struct lhalotree),  nhalos);
        const size_t tree_offset = sizeof(int32_t) /* ntrees */
            + sizeof(int32_t)                           /* totnhalos */
            + sizeof(int32_t)*ntrees                    /* nhalos_per_tree */
            + offset*sizeof(struct lhalotree);
        
        int pread_status = pread_single_lhalotree_with_offset(fd, third, nhalos, tree_offset);
        if(pread_status != EXIT_SUCCESS) {
            fprintf(stderr,ANSI_COLOR_RED"Error: Could not read tree = %d with %d halos"ANSI_COLOR_RESET"\n",itree, nhalos);
            return EXIT_FAILURE;
        }
        for(int i=0;i<nhalos;i++) {
            if( third[i].FirstProgenitor != -1 && (third[i].FirstProgenitor < 0 || third[i].FirstProgenitor >= nhalos)) {            
                /* fprintf(stderr,"In tree=%d, halo=%d has Mvir = %lf and MostBoundID = %lld\n", itree, i, halo.Mvir, halo.MostBoundID); */
                fprintf(stderr,"Error: halonum = %d with FirstProg = %d has invalid value. Should be within [0, %d)\n",i,
                        third[i].FirstProgenitor, nhalos);
                return EXIT_FAILURE;                
            }
        }
        const int test_sort = 1;
        int sort_status = sort_lhalotree_in_snapshot_and_fof_groups(third, nhalos, test_sort);
        if(sort_status != EXIT_SUCCESS) {
            fprintf(stderr,ANSI_COLOR_RED"ERROR: Validation FAILED. itree = %d does not correspond to the original tree"ANSI_COLOR_RESET"\n\n", itree);
            free(third);
            exit_status |= sort_status << 24;
            goto fail;
            /* break; */
        }
        free(third);
        offset += nhalos;
    }
    finish_myprogressbar(&interrupted);
    
    
    /* My first goto statement -- MS: 3rd Nov, 2016.
       Makes error-handling so much simpler */    
fail:
    free(first);
    free(nhalos_per_tree);
    free(all_trees);
    close(fd);
    
    return exit_status;
}    

void usage(int argc, char **argv)
{
    (void) argc;
    fprintf(stderr,"USAGE: `%s' <lhalotree file>\n",argv[0]);
}    


