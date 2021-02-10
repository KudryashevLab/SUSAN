#ifndef GPU_FFT_H
#define GPU_FFT_H

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <cmath>

#include "datatypes.h"

#include "cuda.h"
#include "cuda_runtime_api.h"
#include "cufft.h"

namespace GpuFFT {

class XFFT_base {
protected:
    cufftHandle handler;

public:
	XFFT_base() {
        handler = 0;
    }

    ~XFFT_base() {
        if( handler != 0 )
            cufftDestroy(handler);
    }
    
    void set_stream(cudaStream_t strm) {
        cufftSetStream(handler, strm);
    }
    
};

class FFT1D_full : public XFFT_base {
public:
    void alloc(const int X, const int Y) {
        int rank = 1;
        int m[1] = {X};
        int idist = X;
        int odist = X;
        int inembed[] = {X};
        int onembed[] = {X};
        int istride = 1;
        int ostride = 1;

        if ( cufftPlanMany(&handler, rank, m, inembed, istride, idist, onembed, ostride, odist, CUFFT_R2C, Y) != CUFFT_SUCCESS ) {
            fprintf(stderr,"Error allocating forward FFT1D batch.\n");
            exit(1);

        }
    }
    
    void exec(float2*g_out, float*g_in) {
        cufftExecR2C(handler,g_in,g_out);
    }
};

class IFFT1D_full : public XFFT_base {
public:
    void alloc(const int X, const int Y) {
        int rank = 1;
        int m[1] = {X};
        int idist = X;
        int odist = X;
        int inembed[] = {X};
        int onembed[] = {X};
        int istride = 1;
        int ostride = 1;

        if ( cufftPlanMany(&handler, rank, m, inembed, istride, idist, onembed, ostride, odist, CUFFT_C2C, Y) != CUFFT_SUCCESS ) {
            fprintf(stderr,"Error allocating inverse FFT1D batch.\n");
            exit(1);

        }
    }

    void exec(float2*g_out, float2*g_in) {
        cufftExecC2C(handler,g_in,g_out,CUFFT_INVERSE);
    }
};

class FFT2D : public XFFT_base {
public:
    void alloc(const int X, const int Y, const int Z) {
        int rank = 2;
        int m[2] = {Y, Y};
        int idist = Y*Y;
        int odist = X*Y;
        int inembed[] = {Y, Y};
        int onembed[] = {Y, X};
        int istride = 1;
        int ostride = 1;

        if ( cufftPlanMany(&handler, rank, m, inembed, istride, idist, onembed, ostride, odist, CUFFT_R2C, Z) != CUFFT_SUCCESS ) {
            fprintf(stderr,"Error allocating forward FFT2D batch.\n");
            exit(1);

        }
    }

    void exec(float2*g_out, float*g_in) {
        cufftExecR2C(handler,g_in,g_out);
    }
};

class FFT3D : public XFFT_base {
public:
    void alloc(const int N) {
		
		if ( cufftPlan3d(&handler, N, N, N, CUFFT_R2C ) != CUFFT_SUCCESS ) {
            fprintf(stderr,"Error allocating forward FFT3D.\n");
            exit(1);
        }
    }

    void exec(float2*g_out, float*g_in) {
        cufftExecR2C(handler,g_in,g_out);
    }
};

class IFFT3D : public XFFT_base {
public:
    void alloc(const int N) {
		
		if ( cufftPlan3d(&handler, N, N, N, CUFFT_C2R ) != CUFFT_SUCCESS ) {
            fprintf(stderr,"Error allocating inverse FFT3D.\n");
            exit(1);
        }
    }

    void exec(float*g_out, float2*g_in) {
        cufftExecC2R(handler,g_in,g_out);
    }
};


}

#endif /// GPU_FFT_H
