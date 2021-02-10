#ifndef REC_ACC_H
#define REC_ACC_H

#include "datatypes.h"
#include "particles.h"
#include "tomogram.h"
#include "stack_reader.h"
#include "gpu.h"
#include "gpu_fft.h"
#include "gpu_rand.h"
#include "gpu_kernel.h"
#include "gpu_kernel_ctf.h"
#include "gpu_kernel_vol.h"
#include "substack_crop.h"
#include "mrc.h"
#include "io.h"
#include "points_provider.h"
#include "reconstruct_args.h"
#include <iostream>

class RecSubstack {
public:
	int N;
	int M;
	int P;
	int maxK;
	int NP;
	int MP;
	
	GPU::GArrSingle    ss_padded;
	GPU::GArrSingle3   ss_vec_r;
	GPU::GArrSingle2   ss_fourier;
	GPU::GTex2DSingle2 ss_tex;
	GPU::GTex2DSingle  ss_ctf;
	
	GpuFFT::FFT2D       fft2;
	//GpuRand::Randomizer rand;
	GpuRand::BoxMullerRand rand;
	
	RecSubstack(int m,int n,int k,int p,GPU::Stream&stream) : rand(n+p,n+p,k) {
		N  = n;
		M  = m;
		P  = p;
		NP = n+p;
		MP = (NP/2)+1;
		maxK = k;
		
		ss_padded.alloc(NP*NP*maxK);
		ss_fourier.alloc(MP*NP*maxK);
		
		ss_vec_r.alloc(MP*NP);
		
		ss_tex.alloc(MP,NP,maxK);
		ss_ctf.alloc(MP,NP,maxK);
		
		int3 ss_siz = make_int3(MP,NP,1);
		dim3 blk2D = GPU::get_block_size_2D();
		dim3 grd2D = GPU::calc_grid_size(blk2D,ss_siz.x,ss_siz.y,ss_siz.z);
		GpuKernelsCtf::create_vec_r<<<grd2D,blk2D>>>(ss_vec_r.ptr,ss_siz);
		
		fft2.alloc(MP,NP,maxK);
		fft2.set_stream(stream.strm);
	}
	
	void pad_zero(GPU::Stream&stream) {
		ss_padded.clear(stream.strm);
	}
	
	void pad_normal(GPU::GArrSingle2&g_avg_std,int k,GPU::Stream&stream) {
		int3 ss_pad = make_int3(NP,NP,k);
		rand.gen_normal(ss_padded.ptr,g_avg_std.ptr,ss_pad,stream.strm);
	}
	
	void add_data(GPU::GArrSingle&g_data,GPU::GArrProj2D&g_ali,int k,GPU::Stream&stream) {
		int3 ss_raw = make_int3(N,N,k);
		int3 ss_pad = make_int3(NP,NP,k);
		int3 ss_fou = make_int3(MP,NP,k);
		dim3 blk = GPU::get_block_size_2D();
		dim3 grd_raw = GPU::calc_grid_size(blk,N,N,k);
		dim3 grd_pad = GPU::calc_grid_size(blk,NP,NP,k);
		dim3 grd_fou = GPU::calc_grid_size(blk,MP,NP,k);
		GpuKernels::load_pad<<<grd_raw,blk,0,stream.strm>>>(ss_padded.ptr,g_data.ptr,P/2,ss_raw,ss_pad);
		GpuKernels::fftshift2D<<<grd_pad,blk,0,stream.strm>>>(ss_padded.ptr,ss_pad);
		fft2.exec(ss_fourier.ptr,ss_padded.ptr);
		GpuKernels::fftshift2D<<<grd_fou,blk,0,stream.strm>>>(ss_fourier.ptr,ss_fou);
		GpuKernels::subpixel_shift<<<grd_fou,blk,0,stream.strm>>>(ss_fourier.ptr,g_ali.ptr,ss_fou);
	}
	
	void set_no_ctf(float3 bandpass,int k,GPU::Stream&stream) {
		int3 ss = make_int3(MP,NP,k);
		dim3 blk = GPU::get_block_size_2D();
		dim3 grd = GPU::calc_grid_size(blk,MP,NP,k);
		GpuKernelsCtf::ctf_stk_no_correction<<<grd,blk,0,stream.strm>>>(ss_tex.surface,ss_ctf.surface,ss_fourier.ptr,bandpass,ss);
	}
	
	void set_phase_flip(const CtfConst ctf_const,GPU::GArrDefocus&g_def,float3 bandpass,int k,GPU::Stream&stream) {
		int3 ss = make_int3(MP,NP,k);
		dim3 blk = GPU::get_block_size_2D();
		dim3 grd = GPU::calc_grid_size(blk,MP,NP,k);
		GpuKernelsCtf::ctf_stk_phase_flip<<<grd,blk,0,stream.strm>>>(ss_tex.surface,ss_ctf.surface,ss_fourier.ptr,ctf_const,g_def.ptr,bandpass,ss);                                    
	}
	
	void set_wiener(const CtfConst ctf_const,GPU::GArrDefocus&g_def,float3 bandpass,int k,GPU::Stream&stream) {
		int3 ss = make_int3(MP,NP,k);
		dim3 blk = GPU::get_block_size_2D();
		dim3 grd = GPU::calc_grid_size(blk,MP,NP,k);
		GpuKernelsCtf::ctf_stk_wiener<<<grd,blk,0,stream.strm>>>(ss_tex.surface,ss_ctf.surface,ss_fourier.ptr,ctf_const,g_def.ptr,bandpass,ss);                                    
	}
	
	void set_wiener_ssnr(const CtfConst ctf_const,GPU::GArrDefocus&g_def,float3 bandpass,float2 ssnr,int k,GPU::Stream&stream) {
		// float2  ssnr; /// x=F; y=S;
		single ssnr_f = -100*ssnr.x/(NP*ctf_const.apix);
        single ssnr_s = pow(10,3*ssnr.y);
		int3 ss = make_int3(MP,NP,k);
		dim3 blk = GPU::get_block_size_2D();
		dim3 grd = GPU::calc_grid_size(blk,MP,NP,k);
		GpuKernelsCtf::ctf_stk_wiener_ssnr<<<grd,blk,0,stream.strm>>>(ss_tex.surface,ss_ctf.surface,ss_fourier.ptr,ctf_const,g_def.ptr,ssnr_f,ssnr_s,bandpass,ss);                                    
	}
	
};

class RecAcc {
	
public:
	int maxK;
	int NP;
	int MP;
	
	dim3 blk;
	dim3 grd;
	
	GPU::GArrDouble2 vol_acc;
	GPU::GArrDouble  vol_wgt;
	
	void alloc(const int x,const int y,const int z) {
		MP = x;
		NP = y;
		maxK = z;
		
		vol_acc.alloc(MP*NP*NP);
		vol_wgt.alloc(MP*NP*NP);
		
		vol_acc.clear();
		vol_wgt.clear();
		
		blk = GPU::get_block_size_2D();
		grd = GPU::calc_grid_size(blk,MP,NP,NP);
	}
	
	void clear() {
		vol_acc.clear();
		vol_wgt.clear();
	}
	
	void insert(GPU::GTex2DSingle2&ss_stk,GPU::GTex2DSingle&ss_wgt,GPU::GArrProj2D&g_ali,float3 bandpass,int k,GPU::Stream&stream) {
		GpuKernelsVol::insert_stk<<<grd,blk,0,stream.strm>>>(vol_acc.ptr,vol_wgt.ptr,ss_stk.texture,ss_wgt.texture,g_ali.ptr,bandpass,MP,NP,k);
	}
};

class RecInvWgt {
public:
	int N;
	int M;
	
	int3 siz;
	dim3 blk;
	dim3 grd;
	
	int   inv_iter;
	float inv_gstd;
	
	int n_krnl;
	GPU::GArrDouble  g_wgt;
	GPU::GArrDouble  g_tmp;
	GPU::GArrDouble  g_conv;
	GPU::GArrSingle4 g_krnl;
	
	RecInvWgt(int n,int m,int in_iter,float in_gstd) {
		N = n;
		M = m;
		siz = make_int3(M,N,N);
		blk = GPU::get_block_size_2D();
		grd = GPU::calc_grid_size(blk,M,N,N);
		
		n_krnl   = 0;
		inv_iter = in_iter;
		inv_gstd = in_gstd;
		
		create_kernel();
		alloc();
	}
	
	void invert(GPU::GArrDouble&vol_wgt) {
		if( inv_iter > 0 ) {
			/// gWgt = gVolWgt;
			/// gVolWgt = sphere(N/2,N);
			/// for ...
			///    pTemp   = gVolWgt.*pWgt;
			///    pConv   = convn(pTemp,pKernel);
			///    gVolWgt = gVolWgt./max( abs(pConv), 0.0000001 );
			cudaMemcpy( (void*)g_wgt.ptr, (const void*)(vol_wgt.ptr), sizeof(double)*N*N*M, cudaMemcpyDeviceToDevice);
			GpuKernelsVol::inv_wgt_ite_sphere<<<grd,blk>>>(vol_wgt.ptr,siz);
			for(int i=0;i<inv_iter;i++) {
				GpuKernelsVol::inv_wgt_ite_multiply<<<grd,blk>>>(g_tmp.ptr  , vol_wgt.ptr, g_wgt.ptr , siz);
				GpuKernelsVol::inv_wgt_ite_convolve<<<grd,blk>>>(g_conv.ptr , g_tmp.ptr  , g_krnl.ptr, n_krnl, siz);
				GpuKernelsVol::inv_wgt_ite_divide  <<<grd,blk>>>(vol_wgt.ptr, g_conv.ptr , siz);
			}
		}
		else
			GpuKernelsVol::invert_wgt<<<grd,blk>>>(vol_wgt.ptr,siz);
	}
	
protected:
	void create_kernel() {
		if( inv_iter > 0 ) {
			float work_std = 2*inv_gstd*inv_gstd;
			float acc = 0;
			uint32 tmp;
			Vec3*c_filt = PointsProvider::circle(tmp,2,2);
			n_krnl = tmp;
			float4 *c_krnl = new float4[n_krnl];
			for(int i=0;i<n_krnl;i++) {
				c_krnl[i].x = c_filt[i].x;
				c_krnl[i].y = c_filt[i].y;
				c_krnl[i].z = c_filt[i].z;
				c_krnl[i].w = c_krnl[i].x*c_krnl[i].x + c_krnl[i].y*c_krnl[i].y + c_krnl[i].z*c_krnl[i].z;
				c_krnl[i].w = exp(-c_krnl[i].w/work_std);
				acc += c_krnl[i].w;
			}
			for(int i=0;i<n_krnl;i++) {
				c_krnl[i].w = c_krnl[i].w/acc;
			}
			g_krnl.alloc(n_krnl);
			cudaMemcpy( (void*)g_krnl.ptr, (const void*)(c_krnl), sizeof(float4)*n_krnl, cudaMemcpyHostToDevice);
			delete [] c_krnl;
			delete [] c_filt;
		}
	}

	void alloc() {
		if( inv_iter > 0 ) {
			g_wgt.alloc(N*N*M);
			g_tmp.alloc(N*N*M);
			g_conv.alloc(N*N*M);
		}
	}
};

class RecInvVol {
public:
	GPU::GArrSingle2 vol_fou;
	GPU::GArrSingle  vol_pad;
	GpuFFT::IFFT3D ifft3;
	
	int NP;
	int MP;
	int N;
	
	RecInvVol(int n,int p) {
		N  = n;
		NP = n+p;
		MP = (NP/2) + 1;
		vol_fou.alloc(NP*NP*MP);
		vol_pad.alloc(NP*NP*NP);
		ifft3.alloc(NP);
	}
	
	void apply_inv_wgt(GPU::GArrDouble2&vol_acc,GPU::GArrDouble&vol_wgt) {
		int3 siz = make_int3(MP,NP,NP);
		dim3 blk = GPU::get_block_size_2D();
		dim3 grd = GPU::calc_grid_size(blk,MP,NP,NP);
		GpuKernels::multiply<<<grd,blk>>>(vol_fou.ptr,vol_acc.ptr,vol_wgt.ptr,siz,1.0/(double)(NP*NP*NP));
	}
	
	void invert_and_extract(GPU::GArrSingle&vol_out) {
		dim3 blk = GPU::get_block_size_2D();
		dim3 grd = GPU::calc_grid_size(blk,MP,NP,NP);
		GpuKernels::fftshift3D<<<grd,blk>>>(vol_fou.ptr,MP,NP);
		ifft3.exec(vol_pad.ptr,vol_fou.ptr);
		GpuKernels::fftshift3D<<<grd,blk>>>(vol_pad.ptr,NP);
		
                //dim3 grdP = GPU::calc_grid_size(blk,NP,NP,NP);
                //GpuKernelsVol::grid_correct<<<grdP,blk>>>(vol_pad.ptr,NP);
		
		int3 siz_raw = make_int3(N,N,N);
		int3 siz_pad = make_int3(NP,NP,NP);
		dim3 grd3 = GPU::calc_grid_size(blk,N,N,N);
		GpuKernels::remove_pad_vol<<<grd3,blk>>>(vol_out.ptr,vol_pad.ptr,(NP-N)/2,siz_raw,siz_pad);
	}
	
	
	
};

#endif /// REC_ACC_H

