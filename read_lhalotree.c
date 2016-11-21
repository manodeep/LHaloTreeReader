#define _FILE_OFFSET_BITS 64

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>
#include <unistd.h> //for pread
#include <sys/types.h> //for off_t
#include <limits.h>//for INT_MAX

#include "utils.h"
#include "read_lhalotree.h"
#include "sglib.h"

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


int sort_lhalotree_in_snapshot_and_fof_groups(struct lhalotree *tree, const int64_t nhalos, int test)
{
    if(nhalos > INT_MAX) {
        fprintf(stderr,"Error: nhalos=%"PRId64" can not be larger than INT_MAX=%d\n", nhalos, INT_MAX);
        return EXIT_FAILURE;
    }
    float *prog_mass=NULL, *desc_mass=NULL;
    int32_t *len=NULL, *foflen=NULL;
    if(test > 0) {
        prog_mass = my_malloc(sizeof(*prog_mass), nhalos);
        desc_mass = my_malloc(sizeof(*prog_mass), nhalos);
        len = my_malloc(sizeof(*len), nhalos);
        foflen = my_malloc(sizeof(*foflen), nhalos);

        if(prog_mass == NULL || desc_mass == NULL || len == NULL || foflen == NULL ) {
            fprintf(stderr,"Warning: malloc failure for LHalotree fields - disabling tests even though tests were requested\n");
            test = 0;
        }
    }
    
    
    /* Sort LHalotree into snapshot, FOF group, */
    int32_t *index = my_malloc(sizeof(*index), nhalos);
    if(index == NULL) {
        perror(NULL);
        fprintf(stderr,"Error: Could not allocate memory for the index array for a tree with nhalos = %"PRId64". "
                "Requested size = %zu\n",nhalos, sizeof(*index) * nhalos);
        return EXIT_FAILURE;
    }

    /* Care must be taken for the indices */
    for(int32_t i=0;i<nhalos;i++) {
        index[i] = i;//Keep track of the original indices
        if(test > 0) {
            len[i] = tree[i].Len;
            if(tree[i].FirstHaloInFOFgroup < 0 || tree[i].FirstHaloInFOFgroup >= nhalos){
                fprintf(stderr,"For halonum = %d fofhalo index = %d should be within limits [0, %"PRId64")",
                        i, tree[i].FirstHaloInFOFgroup, nhalos);
                return EXIT_FAILURE;
            }
            foflen[i] = tree[tree[i].FirstHaloInFOFgroup].Len;
            if(tree[i].FirstProgenitor == -1 || (tree[i].FirstProgenitor >= 0 && tree[i].FirstProgenitor < nhalos)) {
                prog_mass[i] = tree[i].FirstProgenitor == -1 ? -1.0:tree[tree[i].FirstProgenitor].Mvir;
            } else {
                fprintf(stderr,"Error. In %s: halonum = %d with FirstProg = %d has invalid value. Should be within [0, %"PRId64")\n",
                        __FUNCTION__,i,tree[i].FirstProgenitor, nhalos);
                return EXIT_FAILURE;
            }
            desc_mass[i] = tree[i].Descendant == -1 ? -1.0:tree[tree[i].Descendant].Mvir;
        }
    }

    
    /* Sort on snapshots, then sort on FOF groups, then ensure FOF halo comes first within group, then sort by subhalo mass  */
#define SNAPNUM_FOFHALO_MVIR_COMPARATOR(x, i, j)    ((x[i].SnapNum != x[j].SnapNum) ? (x[j].SnapNum - x[i].SnapNum):FOFHALO_MVIR_COMPARATOR(x, i, j))
#define FOFHALO_MVIR_COMPARATOR(x, i, j) ((x[i].FirstHaloInFOFgroup != x[j].FirstHaloInFOFgroup) ? (x[x[j].FirstHaloInFOFgroup].Mvir - x[x[i].FirstHaloInFOFgroup].Mvir):FOF_SUBMVIR_COMPARATOR(x,i, j))
#define FOF_SUBMVIR_COMPARATOR(x, i, j)     ((x[i].FirstHaloInFOFgroup == i) ? -1:( (x[j].FirstHaloInFOFgroup == j) ? 1: (x[j].Mvir - x[i].Mvir)) )
    
#define MULTIPLE_ARRAY_EXCHANGER(type,a,i,j) {                          \
        SGLIB_ARRAY_ELEMENTS_EXCHANGER(struct lhalotree, tree,i,j);     \
        SGLIB_ARRAY_ELEMENTS_EXCHANGER(int32_t, index, i, j);           \
    }
    
    SGLIB_ARRAY_HEAP_SORT_MULTICOMP(struct lhalotree, tree, nhalos, SNAPNUM_FOFHALO_MVIR_COMPARATOR, MULTIPLE_ARRAY_EXCHANGER);
    
#undef SNAPNUM_FOFHALO_MVIR_COMPARATOR
#undef FOFHALO_MVIR_COMPARATOR
#undef FOF_MVIR_COMPARATOR
#undef MULTIPLE_ARRAY_EXCHANGER    

    
    /* But I have to first create another array that tracks the current position of the original index
       The original linear index was like so: [0 1 2 3 4 ....]
       Now, the index might look like [4 1 3 2 0 ...]
       What I need is the location of "0" -> which would 4 (the "0" index from original array is in the
       4'th index within this new array order)
       
       This requires another array, identical in size to index.
       
       This array will have to be sorted based on index -> this new sorted array can be directly
       used with the mergertree indices, e.g., FirstProgenitor, to provide correct links.
       
    */

    /* fixed mergertree indices from sorting into snapshot, FOF group, mvir order */
    int status = fix_mergertree_index(tree, nhalos, index);
    if(status != EXIT_SUCCESS) {
        return status;
    }

    if(test > 0) {
        status = EXIT_FAILURE;
        /* Run tests. First generate the array for the mapping between old and new values */
        int32_t *index_for_old_order = my_malloc(sizeof(*index_for_old_order), nhalos);
        if(index_for_old_order == NULL) {
            return EXIT_FAILURE;
        }
        
        for(int32_t i=0;i<nhalos;i++) {
            index_for_old_order[index[i]] = i;
        }
        
        /* Now run the tests. Progenitor/Descendant masses must agree. */
        for(int32_t i=0;i<nhalos;i++) {
            const int32_t old_index = index[i];
            if(len[old_index] != tree[i].Len) {
                fprintf(stderr,"Error: tree[%d].Len = %d now. Old index claims len = %d\n",
                        i, tree[i].Len, len[old_index]);
                return EXIT_FAILURE;
            }

            if(foflen[old_index] != tree[tree[i].FirstHaloInFOFgroup].Len) {
                fprintf(stderr,"Error: tree[%d].FirstHaloInFOFgroup = %d fofLen = %d now. Old index = %d claims len = %d (nhalos=%"PRId64")\n",
                        i, tree[i].FirstHaloInFOFgroup, tree[tree[i].FirstHaloInFOFgroup].Len,
                        old_index,foflen[old_index], nhalos);
                fprintf(stderr,"%d %d %d %d\n",i,tree[i].FirstHaloInFOFgroup,index[i],old_index);
                return EXIT_FAILURE;
            }


            int32_t desc = tree[i].Descendant;
            if(desc == -1) {
                if(desc_mass[old_index] != -1.0){
                    fprintf(stderr,"Error: tree[%d].descendant = %d (should be -1) now but old descendant mass = %f\n",
                            i, tree[i].Descendant, desc_mass[old_index]);
                    return EXIT_FAILURE;
                }
            } else {
                assert(desc >= 0 && desc < nhalos);
                if(desc_mass[old_index] != tree[desc].Mvir) {
                    fprintf(stderr,"Error: tree[%d].Descendant (Mvir) = %f (desc=%d) now but old descendant mass = %f\n",
                            i, tree[desc].Mvir, desc, desc_mass[old_index]);
                    return EXIT_FAILURE;
                }
            }


            int32_t prog = tree[i].FirstProgenitor;
            if(prog == -1) {
                if(prog_mass[old_index] != -1.0){
                    fprintf(stderr,"Error: tree[%d].FirstProgenitor = %d (should be -1) now but old FirstProgenitor mass = %f\n",
                            i, tree[i].FirstProgenitor, desc_mass[old_index]);
                    return EXIT_FAILURE;
                }
            } else {
                if( prog < 0 || prog >= nhalos) {
                    fprintf(stderr,"WEIRD: prog = %d for i=%d is not within [0, %"PRId64")\n",prog, i, nhalos);
                }
                assert(prog >=0 && prog < nhalos);
                if(prog_mass[old_index] != tree[prog].Mvir) {
                    fprintf(stderr,"Error: tree[%d].FirstProgenitor (Mvir) = %f (prog=%d) now but old FirstProgenitor mass = %f\n",
                            i, tree[prog].Mvir, prog, prog_mass[old_index]);
                    return EXIT_FAILURE;
                }
            }
        }

        free(index_for_old_order);
        free(prog_mass); free(desc_mass);
        free(len);free(foflen);
    }
    free(index);

    
    return EXIT_SUCCESS;
}


/* This is a more generic function (accepts trees sorted into an arbitary order + original indices as shuffled along with the tree */
int fix_mergertree_index(struct lhalotree *tree, const int64_t nhalos, const int32_t *index)
{
    if(nhalos > INT_MAX) {
        fprintf(stderr,"Error: nhalos=%"PRId64" can not be larger than INT_MAX=%d\n", nhalos, INT_MAX);
        return EXIT_FAILURE;
    }

    int32_t *current_index_for_old_order = my_malloc(sizeof(*current_index_for_old_order), nhalos);
    if(current_index_for_old_order == NULL) {
        return EXIT_FAILURE;
    }


    /* The individual mergertree indices contain references to the old order -> as to where they were and need to
       be updated to where the halo is now. So, we need an array that can tells us, for any index in the old order,
       the location for that halo in the new order. 
      
       index[i] contains where the halo was in the old order and I need the opposite information. The following
       lines contain this inverting proces -- only applicable because *ALL* values in index[i] are unique (i.e.,
       this loop can be vectorized with a #pragma simd style). The value on the RHS, i, is the *CURRENT* index
       while the key, on the LHS, is the *OLD* index. Thus, current_index_for_old_order is an array that tells
       us where *ANY* halo index from the *OLD* order can be found in the *NEW* order.

       Looks deceptively simple, it isn't. Took 3-days of my time + 2 hours of YQ's to nail this down and have
       validations pass. - MS 19/11/2016
     */
    for(int32_t i=0;i<nhalos;i++) {
        current_index_for_old_order[index[i]] = i;
    }

    
    //the array current_index_for_old_order now contains the current positions for the older index pointers
#define UPDATE_LHALOTREE_INDEX(FIELD) {                                 \
        const int32_t ii = this_halo->FIELD;                            \
        if(ii >=0 && ii < nhalos) {                                     \
            const int32_t dst = current_index_for_old_order[ii];        \
            this_halo->FIELD = dst;                                     \
        }                                                               \
    }

    //Now fix *all* the mergertree indices
    for(int64_t i=0;i<nhalos;i++) {
        struct lhalotree *this_halo = &(tree[i]);
        UPDATE_LHALOTREE_INDEX(FirstProgenitor);
        UPDATE_LHALOTREE_INDEX(NextProgenitor);
        UPDATE_LHALOTREE_INDEX(Descendant);
        UPDATE_LHALOTREE_INDEX(FirstHaloInFOFgroup);
        UPDATE_LHALOTREE_INDEX(NextHaloInFOFgroup);
    }
#undef UPDATE_LHALOTREE_INDEX

    free(current_index_for_old_order);
    return EXIT_SUCCESS;
}


