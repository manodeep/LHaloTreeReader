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

#define MAXLEN (1000)

#include "utils.h"
#include "read_lhalotree.h"
#include "progressbar.h"

void usage(int argc, char **argv);

int main(int argc, char **argv)
{
    if(argc < 2) {
        usage(argc, argv);
        fprintf(stderr,"exiting\n");
        exit(EXIT_FAILURE);
    }
    struct lhalotree *tree = NULL;
    int32_t *nhalos_per_tree=NULL;
    int fd, status=EXIT_FAILURE;
    
    for(int iarg=1;iarg<argc;iarg++) {
        char *filename = argv[iarg];
        int32_t ntrees=0;
        int32_t totnhalos = 0;
        int interrupted=0;//for progressbar
        
        fd = open(filename, O_RDONLY, O_NONBLOCK);
        if(fd < 0) {
            fprintf(stderr,"Error in opening input file `%s'\n", filename);
            perror(NULL);
            status = fd;
            goto fail;
        }

        status = read_file_headers_lhalotree(filename, &ntrees, &totnhalos, &nhalos_per_tree);
        if(status != EXIT_SUCCESS) {
            goto fail;
        } 

        size_t offset = 0;
        fprintf(stderr,"Reading from file `%s'...\n", filename);
        char buf[MAXLEN];
        my_snprintf(buf, MAXLEN, "%s_fofs_last_snap",filename);
        FILE *fp=fopen(buf,"w");
        if(fp == NULL) {
            fprintf(stderr,"Could not open file `%s'\n",filename);
            goto fail;
        }

        init_my_progressbar(ntrees, &interrupted);
        for(int32_t itree=0;itree<ntrees;itree++) {
            my_progressbar(itree, &interrupted);
            const int32_t nhalos = nhalos_per_tree[itree];
            tree  = my_malloc(sizeof(*tree), nhalos);
            if(tree == NULL) {
                fprintf(stderr,"malloc failed for tree with nhalos = %d\n", nhalos);
                goto fail;
            }
            const size_t offset_for_tree =  sizeof(int32_t) /* ntrees */
                + sizeof(int32_t)                           /* totnhalos */
                + sizeof(int32_t)*ntrees                    /* nhalos_per_tree */
                + offset*sizeof(struct lhalotree);

            /* Read a single tree */
            status = pread_single_lhalotree_with_offset(fd, tree, nhalos, offset_for_tree);
            if(status != EXIT_SUCCESS) {
                fprintf(stderr,"pread failed\n");
                goto fail;
            }

            int32_t nfofs=0;
            int32_t max_snap=-1;
            for(int32_t i=0;i<nhalos;i++) {
                max_snap = tree[i].SnapNum > max_snap ? tree[i].SnapNum:max_snap;
            }
            
            double mmh=-1.0;//most massive halo at last snapshot
            int32_t maxlen=-1;
            for(int32_t i=0;i<nhalos;i++) {

                /* Find the number of FOFs at the last snapshot. ConvertCtrees (Consistent-Trees to LHaloTree converter, https://github.com/manodeep/ConvertCTrees)
                   makes the MostBoundID negative to indicate a root FOF halo  */
                if((tree[i].SnapNum == max_snap && tree[i].FirstHaloInFOFgroup == i) || (tree[i].SnapNum == max_snap && tree[i].MostBoundID < 0)) {
                    nfofs++;
                    if(tree[i].Len > maxlen) {
                        maxlen = tree[i].Len;//number of particles in most massive host at last snap
                        mmh = tree[i].Mvir;//most massive host at last snap
                    }
                }
            }
            
            fprintf(fp,"%15.5e %9d %8d\n", mmh, maxlen, nfofs);
            offset += (size_t) nhalos;
            free(tree);
            tree=NULL;
        }
        fclose(fp);
        free(nhalos_per_tree);
        nhalos_per_tree=NULL;
        finish_myprogressbar(&interrupted);
    }

fail:
    free(nhalos_per_tree);
    free(tree);
    close(fd);
    
    return status;
}    

void usage(int argc, char **argv)
{
    (void) argc;
    fprintf(stderr,"USAGE: `%s' <lhalotree file>\n",argv[0]);
}    


