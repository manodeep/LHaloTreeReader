#pragma once
#include <stdio.h>

/* Definition of the LHaloTree struct */
#include "datatype.h"

#ifdef __cplusplus
extern "C" {
#endif

    /* Actual useful functions*/
    int32_t read_ntrees_lhalotree(const char *filename)__attribute__((warn_unused_result));
    struct output_dtype * read_entire_lhalotree(const char *filename, int *ntrees, int *totnhalos, int **nhalos_per_tree);
    struct output_dtype * read_single_lhalotree(const char *filename, const int32_t treenum);

#ifdef __cplusplus
}
#endif
    
