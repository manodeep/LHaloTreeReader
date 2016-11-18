#pragma once
#include <unistd.h>
#include <stdio.h>

/* Definition of the LHaloTree struct */
#include "lhalotree.h"

#ifdef __cplusplus
extern "C" {
#endif

    /* Actual useful functions*/
    extern size_t read_single_lhalotree_from_stream(FILE *fp, struct lhalotree *tree, const int32_t nhalos);
    extern int pread_single_lhalotree_with_offset(int fd, struct lhalotree *tree, const int32_t nhalos, off_t offset);
    extern int read_file_headers_lhalotree(const char *filename, int32_t *ntrees, int32_t *totnhalos, int32_t **nhalos_per_tree);
    extern int32_t read_ntrees_lhalotree(const char *filename)__attribute__((warn_unused_result));
    extern struct lhalotree * read_entire_lhalotree(const char *filename, int *ntrees, int *totnhalos, int **nhalos_per_tree);
    extern struct lhalotree * read_single_lhalotree(const char *filename, const int32_t treenum);

    /* Sorting an LHalotree output into a new order */
    extern int sort_lhalotree_in_snapshot_and_fof_groups(struct lhalotree *tree, const int64_t nhalos, int test);
    /* And fixing lhalotree mergertree indices from a generic sort */
    extern int fix_mergertree_index(struct lhalotree *tree, const int64_t nhalos, const int32_t *index);

    
#ifdef __cplusplus
}
#endif
    
