
#ifdef __cplusplus
extern "C" {
#endif

#include <float.h>
#include <math.h>
#include <stdio.h>

#define CUDA
#include "common_funcs.h"
#include "constants.h"
#include "gpu_kernels.h"

#define MAX_BLOCKS 65535




/* UTIL FUNCTIONS */
__device__ __host__
float4 normalize_quaternion(float4 q)
{
    float m = sqrtf(q.x*q.x + q.y*q.y + q.z*q.z + q.w*q.w);
    q.x /= m;
    q.y /= m;
    q.z /= m;
    q.w /= m;
    return q;
}

__device__ __host__
float4 quaternion_conjugate(float4 quat)
{
    float4 ret = quat;
    ret.x *= -1.0;
    ret.y *= -1.0;
    ret.z *= -1.0;
    return ret;
}

__device__ __host__
float4 quaternion_mult(float4 q1, float4 q2)
{
    float4 ret;
    ret.w = q1.w*q2.w - q1.x*q2.x - q1.y*q2.y - q1.z*q2.z;
    ret.x = q1.w*q2.x + q1.x*q2.w + q1.y*q2.z - q1.z*q2.y;
    ret.y = q1.w*q2.y + q1.y*q2.w + q1.z*q2.x - q1.x*q2.z;
    ret.z = q1.w*q2.z + q1.z*q2.w + q1.x*q2.y - q1.y*q2.x;
    return ret;
}

__device__ __host__
float trilinear_interp(float* grid, float3 grid_lower, int3 grid_dims, 
	float3 grid_steps, int data_dims, int data_dim, float4 point)
{
	float px = point.x;
	float py = point.y;
	float pz = point.z;
	float pi = (px - grid_lower.x)/grid_steps.x - 0.5;
	float pj = (py - grid_lower.y)/grid_steps.y - 0.5;
	float pk = (pz - grid_lower.z)/grid_steps.z - 0.5;
	float ii = pi - floorf(pi);
	float jj = pj - floorf(pj);
	float kk = pk - floorf(pk);
	float dd = 0;
	for(int di = 0; di < 2; ++di)
	{
		for(int dj = 0; dj < 2; ++dj)
		{
			for(int dk = 0; dk < 2; ++dk)
			{
				int ci = (int)fmaxf(0, fminf(grid_dims.x - 1, floorf(pi + di)));
				int cj = (int)fmaxf(0, fminf(grid_dims.y - 1, floorf(pj + dj)));
				int ck = (int)fmaxf(0, fminf(grid_dims.z - 1, floorf(pk + dk)));
				float v = grid[ci*grid_dims.y*grid_dims.z*data_dims + 
							   cj*grid_dims.z*data_dims + 
							   ck*data_dims + 
							   data_dim];
				dd += v*(di ? ii : 1 - ii)*(dj ? jj : 1 - jj)*(dk ? kk : 1 - kk);
			}
		}
	}
	return dd;
}

__global__
void kernel_cudaFloatMemset(float* ptr, float value, int nvals)
{
	int index = blockIdx.x * blockDim.x + threadIdx.x;
	int stride = blockDim.x * gridDim.x;
	for (int i = index; i < nvals; i += stride)
	{
		ptr[i] = value;
	}
}
void cudaFloatMemset(float* ptr, float value, int nvals, cudaStream_t stream)
{
	int nops = nvals/256;
    int numBlocks = ceil(nops * (1.0/256));
    dim3 blocks(numBlocks);
    dim3 threads(256);
    kernel_cudaFloatMemset<<<blocks, threads, 0, stream>>>(ptr, value, nvals);
}

__device__ 
float atomicMin(float *addr, float value)
{
    float old = *addr;
    float assumed;
    do
    {
    	if(old <= value) 
    		return old;
        assumed = old;
        old = atomicCAS((unsigned int*)addr, __float_as_int(assumed), __float_as_int(value));
    }while(old != assumed);
    return old;
}

/* Layer Funcs */

__global__
void kernel_convsp(float* locs, float* data, float* density, float* weight, float* bias, 
	int batch_size, int N, int nchannels, int ndims, int nkernels, int ncells, 
	float radius, float* kernel_size, float* dilation, float* out, float* ddata,
	float* dweight, int num_blocks)
{
	int block_size = ceilf(1.0f*N/num_blocks);

	int index = blockIdx.x * blockDim.x + threadIdx.x;
    int stride = blockDim.x * gridDim.x;
    for (int i = index; i < N*batch_size*num_blocks; i += stride)
    {
    	int b = i/(N*num_blocks);
    	int n = (i % (N*num_blocks))/num_blocks;
    	int block = i % num_blocks;
    	int start = block*block_size;

    	compute_kernel_cells(locs, data, density, weight, bias, batch_size, N, 
    		nchannels, ndims, nkernels, ncells, radius, kernel_size, dilation, 
    		out, b, n, start, start + block_size, ddata, dweight);
    }
}
int cuda_convsp(float* locs, float* data, float* density, float* weight, float* bias, 
	int batch_size, int N, int nchannels, int ndims, int nkernels, int ncells, 
	float radius, float* kernel_size, float* dilation, float* out, float* ddata,
	float* dweight, cudaStream_t stream)
{
	const int NUM_BLOCKS = 4;
	int nops = batch_size*N*NUM_BLOCKS;
    int numBlocks = ceil(nops * (1.0/256));
    dim3 blocks(numBlocks);
    dim3 threads(256);

	kernel_convsp<<<blocks, threads, 0, stream>>>(locs, data, density, weight, bias,
		batch_size, N, nchannels, ndims, nkernels, ncells, radius, kernel_size, 
		dilation, out, ddata, dweight, NUM_BLOCKS);
	cudaDeviceSynchronize();
	// check for errors
	cudaError_t err = cudaGetLastError();
	if (err != cudaSuccess) {
	printf("error in cuda_convsp_forward: %s\n", cudaGetErrorString(err));
		return 0;
	}
	return 1;
}



#ifdef __cplusplus
}
#endif
