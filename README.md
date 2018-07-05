# LHaloTreeReader
A skeleton C code to read LHaloTree output

# LHaloTree Structure
There are 5 indices to navigate around LHaloTree mergertrees. A value of `-1`
indicates a stopping condition, i.e., no more valid halos exist. All of these
indices are tree-local, i.e., if there 1000 halos in a tree, then any of these
indices can either be `-1`, or in the range [0, 999].

1. `Descendant` -- The index of what the halo will become in the future. This
   can either be the halo itself, or a different halo (signifying a merger)
2. `FirstProgenitor` -- tracks the progenitor in the past. Is equal to `-1`
   when the halo does not have **any** progenitors. 
3. `NextProgenitor`  -- Is attached to the `FirstProgenitor` halo and tracks the
   other progenitors. Can be in the past/present/future of `FirstProgenitor`,
   depending on if any one of the halos skip snapshots. **NOTE**
   `NextProgenitor` of any given halo (i.e., the `Descendant') is stored in the `FirstProgenitor` of
   that halo. Is equal to `-1` when there are no more progenitors. 
4. `FirstHaloInFOFGroup` -- The index for the `FOF` halo. The `FOF` halo points
   to itself. Can never be `-1`.
5. `NextHaloInFOFGroup` -- The index for the next subhalo in the `FOF`
   groups. Is equal to `-1` when there are no more subhalos in that `FOF` halo.

![lhalotree mergertree](lhalotree-mergertree-structure.png "Structure for the LHaloTree
mergertree")
