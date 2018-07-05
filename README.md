# LHaloTreeReader
A skeleton C code to read LHaloTree output

# LHaloTree Structure
There are 5 indices to navigate around LHaloTree mergertrees. A value of `-1`
indicates a stopping condition, i.e., no more valid halos exist. All of these
indices are tree-local, i.e., if there 1000 halos in a tree, then any of these
indices can either be `-1`, or in the inclusive range [0, 999].

1. `Descendant` -- The index of what the halo will become in the future. This
   can either be the halo itself, or a different halo (signifying a merger)
2. `FirstProgenitor` -- tracks the progenitor in the past. Is equal to `-1`
   when the halo does not have **any** progenitors. 
3. `NextProgenitor`  -- Is attached to the `FirstProgenitor` halo and tracks the
   other progenitors. Can be in the past/present/future of `FirstProgenitor`,
   depending on if any one of the halos skip snapshots. Is equal to `-1` when there are no more progenitors.
   
   **NOTE**
   `NextProgenitor` of any given halo (i.e., the `Descendant`) is stored in the `FirstProgenitor` of
   the (`Descendant`) halo. The following 
   
```c
   /* 
      The `tree` variable is an array containing `nhalos`  
      elements of type `struct lhalotree`. This represents the entire
      mergertree. Walking is accomplished by using the indices represented here
   */
   int32_t halonum = 42;/* some halo that you want to process */
   int32_t prog = tree[halonum].FirstProgenitor;
   while(prog != -1) {
      /* do something with prog here */
      process_progenitor(prog, tree); /* prog is an index into the entire tree */
      
      /* Note how you get the `NextProgenitor` index from 
         the `prog` index and **NOT** `halonum`  */
      prog = tree[prog].NextProgenitor; 
   }
```
   
4. `FirstHaloInFOFGroup` -- The index for the `FOF` halo. The `FOF` halo points
   to itself. Can never be `-1`.
5. `NextHaloInFOFGroup` -- The index for the next subhalo in the `FOF`
   groups. Is equal to `-1` when there are no more subhalos in that `FOF` halo.

# LHaloTree Schematic

![lhalotree mergertree](lhalotree-mergertree-structure.png "Structure for the LHaloTree
mergertree")

# Walking the Entire LHaloTree

```C
int32_t fully_walk_tree(const int start, struct lhalotree *tree)
{
    /*
      Fully walks the tree -- all reachable nodes will be returned exactly once
      Returns -1 when done visiting all nodes of the tree

    */

    int32_t curr_halo = start;
    if(tree[curr_halo].FirstProgenitor != -1) {
        return tree[curr_halo].FirstProgenitor;
    } else {
        if(tree[curr_halo].NextProgenitor != -1) {
            return tree[curr_halo].NextProgenitor;
        } else {
            while((tree[curr_halo].NextProgenitor == -1) && (tree[curr_halo].Descendant != -1)) {
                curr_halo = tree[curr_halo].Descendant;
            }

            /* can return -1 (i.e., when every node has been visited) */
            return tree[curr_halo].NextProgenitor;
        }
    }
    
    return -1;/* un-reachable */
}
```

In principle, each tree within a `lhalotree` mergertree file should contain **exactly** one `FOF` halo, and every halo contained in that tree should be reachable by those 5 mergertree indices. In such a case, the previous code *is* be sufficient to walk the entire tree. However, I have found that there exist multiple `FOF` halos at the final snapshot for a large number of trees. To ensure that you have indeed processed *all* halos in a given tree, you are better off with the following code

```c
const int maxsnap = -1;
for(int32_t i=0;i<nhalos_in_this_tree;i++) {
   maxsnap = tree[i].SnapNum > maxsnap ? tree[i].SnapNum:maxsnap;
}

for(int32_t i=0;i<nhalos_in_this_tree;i++) {
   /* Check if the halo is a FOF halo at the final snapshot */
   if(tree[i].SnapNum == maxsnap && tree[i].FirstHaloInFOFGroup == i) {
       int32_t halo = i;
       while(halo != -1) {
          process_halo(halo);
          halo = fully_walk_tree(i, tree);
       }
   }
}
```



