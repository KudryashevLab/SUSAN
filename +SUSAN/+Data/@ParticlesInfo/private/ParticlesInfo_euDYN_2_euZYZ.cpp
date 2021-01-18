#define MW_NEEDS_VERSION_H /// For Matlab > R2018b
#include "mex.h"
#include "math.h"

#include "matlab.h"
#include "datatypes.h"
#include "math_cpu.h"

#include "Eigen/Geometry"


using namespace Eigen;
using namespace Matlab;

#define FUNC_NAME "ParticlesInfo.euDYN_2_euZYZ"


/// euZYZ = euDYN_2_euZYZ(eu_dynamo)
void mexFunction(int nOut, mxArray *pOut[], int nIn, const mxArray *pIn[]) {

    if( nOut != 1 ) {
        mexErrMsgTxt("[" FUNC_NAME "] One output required");
    }

    ///
    if( nIn  != 1 ) {
        mexErrMsgTxt("[" FUNC_NAME "] One input required");
    }

    if( is_single(pIn[0]) ) {

        mwSize   X,Y,Z;
        float    *p_euDYN;
        float    *p_euZYZ;

        /// Read inputs:
        get_array(pIn[0],p_euDYN,X,Y,Z);
        if( Y != 3 || Z != 1 ) {
			mexErrMsgTxt("[" FUNC_NAME "] Wrong angles matrix dimensions");
		}

		/// Allocate output:
        allocate_and_get(p_euZYZ,pOut[0],X,3,1);

        /// Convert:
        V3f  euZXZ, euZYZ;
        M33f R;

        for(int z=0;z<Z;z++) {

            for(int x=0;x<X;x++) {
                euZXZ(2) = p_euDYN[x    ];
                euZXZ(1) = p_euDYN[x+  X];
                euZXZ(0) = p_euDYN[x+2*X];

                euZXZ *= M_PI/180;
                Math::eZXZ_Rmat(R,euZXZ);
                Math::Rmat_eZYZ(euZYZ,R);
                euZYZ *= 180/M_PI;

                p_euZYZ[x    ] = euZYZ(0);
                p_euZYZ[x+  X] = euZYZ(1);
                p_euZYZ[x+2*X] = euZYZ(2);
            }
        }

    }
    else {
        mexErrMsgTxt("[" FUNC_NAME "] Wrong Inputs' types.");
    }
}



