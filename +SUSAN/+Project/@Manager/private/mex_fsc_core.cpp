#define MW_NEEDS_VERSION_H /// For Matlab > R2018b
#include "mex.h"
#include "math.h"

#include "matlab.h"
#include "mrc.h"
#include "datatypes.h"

#include <cstring>

using namespace Matlab;

void mexFunction(int nOut, mxArray *pOut[], int nIn, const mxArray *pIn[]) {

    if( nOut != 1 ) {
        mexErrMsgTxt("[mex_fsc_core] One output required");
    }

    if( nIn  != 3 ) {
        mexErrMsgTxt("[mex_fsc_core] Three inputs required");
    }
    
    float *num,*denA,*denB;
    double*output,*countA,*countB;
    mwSize X,Y,Z;
  
    if( is_single(pIn[0]) && is_single(pIn[1]) && is_single(pIn[2]) ) {
        
        get_array(pIn[0],num,X,Y,Z);
        get_array(pIn[1],denA);
        get_array(pIn[2],denB);
        mwSize L = X/2 + 1;
        allocate(pOut[0], L, 1, 1, mxDOUBLE_CLASS, mxREAL);
        get_array(pOut[0],output);
        countA= (double*)malloc(L*sizeof(double));
        countB= (double*)malloc(L*sizeof(double));
        memset((void*)output,0,L*sizeof(double));
        memset((void*)countA,0,L*sizeof(double));
        memset((void*)countB,0,L*sizeof(double));
        
        float c_x = float(X)/2;
        float c_y = float(Y)/2;
        float c_z = float(Z)/2;

        float x,y,z;
        
        mwSize i,j,k;
        
        for(k=0;k<Z;k++) {

            z = ((float)k) - c_z;

            for(i=0;i<X;i++) {

                x = ((float)i) - c_x;

                for(j=0;j<Y;j++) {

                    y = ((float)j) - c_y;

                    float R = sqrt( x*x + y*y + z*z );

                    int r0 = (int)(floor(R));
                    int r1 = r0 + 1;

                    float w1 = R - floor(R);
                    float w0 = 1 - w1;

                    float val_n = (double)num [j+i*X+k*X*Y];
                    float val_a = (double)denA[j+i*X+k*X*Y];
                    float val_b = (double)denB[j+i*X+k*X*Y];

                    if( r0 >= 0 && r0 < L ) {
                        output[r0] += val_n;
                        countA[r0] += val_a;
                        countB[r0] += val_b;
                    }

                    //if( r1 >= 0 && r1 < L ) {
                    //    output[r1] += val_n*w1;
                    //    countA[r1] += val_a*w1;
                    //    countB[r1] += val_b*w1;
                    //}
                }
            }
        }
        
        for(i=0;i<L;i++) {
            output[i] = output[i]/sqrt(countA[i]*countB[i]);
        }
                
        free(countA);
        free(countB);
    }
    else {
        mexErrMsgTxt("[CtfEstimator.full_radial_avg] Inputs must be (real) floats.");
    }  
}