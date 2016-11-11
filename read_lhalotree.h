#pragma once
#include <unistd.h>
#include <stdio.h>

/* Definition of the LHaloTree struct */
#include "lhalotree.h"

#ifdef __cplusplus
extern "C" {
#endif

    /* Actual useful functions*/
    size_t read_single_lhalotree_from_stream(FILE *fp, struct lhalotree *tree, const int32_t nhalos);
    int pread_single_lhalotree_with_offset(int fd, struct lhalotree *tree, const int32_t nhalos, off_t offset);
    int read_file_headers_lhalotree(const char *filename, int32_t *ntrees, int32_t *totnhalos, int32_t **nhalos_per_tree);
    int32_t read_ntrees_lhalotree(const char *filename)__attribute__((warn_unused_result));
    struct lhalotree * read_entire_lhalotree(const char *filename, int *ntrees, int *totnhalos, int **nhalos_per_tree);
    struct lhalotree * read_single_lhalotree(const char *filename, const int32_t treenum);

#ifdef __cplusplus
}
#endif
    
