#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
    
struct lhalotree
{
    // merger tree pointers
    //Points to the future
    union {
        int Descendant;
        int Parent;//HINGE lingo (well, almost. in HINGE this is a pointer rather than an index)
    };
    union{
        int FirstProgenitor;
        int BigChild;//in the past
    };
    
    union{
        int NextProgenitor;
        int Sibling;//At the same snapshot
    };
    
    union{
        int FirstHaloInFOFgroup;
        int MostMassiveHost;//in keeping with Consistent-Trees style (for uparent)
    };
    
    int NextHaloInFOFgroup;

    // properties of halo
    int Len;
    float M_Mean200, Mvir, M_TopHat;  // for Millennium, Mvir=M_Crit200
    float Pos[3];
    float Vel[3];
    float VelDisp;
    float Vmax;
    float Spin[3];
    long long MostBoundID;

    // original position in simulation tree files
    int SnapNum;
    int FileNr;
    int SubhaloIndex;
    float SubHalfMass;
};

#ifdef __cplusplus
}
#endif
