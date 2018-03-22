#include <TH/TH.h>

#ifdef CUDA
#undef CUDA
#endif
#include "common_funcs.h"
#include "constants.h"


int cpu_convsp(const float* locs, const float* data, const float* weight, const float* bias, 
    const int batch_size, const int N, const int nchannels, const int ndims, 
    const int nkernels, const int ncells, const float radius, const float* kernel_size, 
    const float* dilation, const int dis_norm, const int kernel_fn, float* out, 
    float* ddata, float* dweight);

int cpu_convsdf(const float* locs, const int batch_size, const int N, const int ndims, 
    const float* idxs, const float* poses, const float* scales, const int M, 
    const int pose_len, const float* sdfs, const float* sdf_offsets, const float* sdf_shapes, 
    const float* weight, const float* bias, const int nkernels, const int ncells, 
    const float* kernel_size, const float* dilation, const float max_distance, 
    float* out, float* dweight);


int spn_max_cartesian_dim(void)
{
    return MAX_CARTESIAN_DIM;
}

int spn_convsp_forward(const THFloatTensor* locs_t, const THFloatTensor* data_t, 
    const THFloatTensor* weight_t, const THFloatTensor* bias_t, const float radius, 
    const THFloatTensor* kernel_size_t, const THFloatTensor* dilation_t, 
    const int dis_norm, const int kernel_fn, THFloatTensor* out_t)
{

    const float* locs = THFloatTensor_data(locs_t);
    const float* data = THFloatTensor_data(data_t);
    const float* weight = THFloatTensor_data(weight_t);
    const float* bias = THFloatTensor_data(bias_t); 
    const int batch_size = locs_t->size[0];
    const int N = locs_t->size[1];
    const int nchannels = data_t->size[2];
    const int ndims = locs_t->size[2];
    const int nkernels = weight_t->size[0];
    const int ncells = weight_t->size[2];
    const float* kernel_size = THFloatTensor_data(kernel_size_t);
    const float* dilation = THFloatTensor_data(dilation_t);
    float* out = THFloatTensor_data(out_t);

    return cpu_convsp(locs, data, weight, bias, batch_size, N, nchannels, ndims,
        nkernels, ncells, radius, kernel_size, dilation, dis_norm, kernel_fn, out, NULL, 
        NULL);
}

int spn_convsp_backward(const THFloatTensor* locs_t, const THFloatTensor* data_t, 
    const THFloatTensor* weight_t, const THFloatTensor* bias_t, const float radius, 
    const THFloatTensor* kernel_size_t, const THFloatTensor* dilation_t, 
    const int dis_norm, const int kernel_fn, THFloatTensor* out_t, 
    THFloatTensor* ddata_t, THFloatTensor* dweight_t)
{

    const float* locs = THFloatTensor_data(locs_t);
    const float* data = THFloatTensor_data(data_t);
    const float* weight = THFloatTensor_data(weight_t);
    const float* bias = THFloatTensor_data(bias_t); 
    float* ddata = THFloatTensor_data(ddata_t);
    float* dweight = THFloatTensor_data(dweight_t);
    const int batch_size = locs_t->size[0];
    const int N = locs_t->size[1];
    const int nchannels = data_t->size[2];
    const int ndims = locs_t->size[2];
    const int nkernels = weight_t->size[0];
    const int ncells = weight_t->size[2];
    const float* kernel_size = THFloatTensor_data(kernel_size_t);
    const float* dilation = THFloatTensor_data(dilation_t);
    float* out = THFloatTensor_data(out_t);

    return cpu_convsp(locs, data, weight, bias, batch_size, N, nchannels, ndims,
        nkernels, ncells, radius, kernel_size, dilation, dis_norm, kernel_fn, out, 
        ddata, dweight);
}

int cpu_convsp(const float* locs, const float* data, const float* weight, const float* bias, 
    const int batch_size, const int N, const int nchannels, const int ndims, 
    const int nkernels, const int ncells, const float radius, const float* kernel_size, 
    const float* dilation, const int dis_norm, const int kernel_fn, float* out, 
    float* ddata, float* dweight)
{
    int b, n;
    for(b = 0; b < batch_size; ++b)
    {
        for(n = 0; n < N; ++n)
        {
            compute_kernel_cells(locs, data, weight, bias, batch_size, N, 
                nchannels, ndims, nkernels, ncells, radius, kernel_size, dilation, 
                dis_norm, kernel_fn, out, b, n, 0, N, ddata, dweight);
        }
    }
    return 1;
}




int spn_convsdf_forward(const THFloatTensor* locs_t, const THFloatTensor* idxs_t, 
    const THFloatTensor* poses_t, const THFloatTensor* scales_t, 
    const THFloatTensor* sdfs_t, const THFloatTensor* sdf_offsets_t,
    const THFloatTensor* sdf_shapes_t, const THFloatTensor* weight_t, 
    const THFloatTensor* bias_t, const THFloatTensor* kernel_size_t, 
    const THFloatTensor* dilation_t, const float max_distance,
    THFloatTensor* out_t)
{

    const float* locs = THFloatTensor_data(locs_t);
    const float* idxs = THFloatTensor_data(idxs_t);
    const float* poses = THFloatTensor_data(poses_t);
    const float* scales = THFloatTensor_data(scales_t);
    const float* sdfs = THFloatTensor_data(sdfs_t);
    const float* sdf_offsets = THFloatTensor_data(sdf_offsets_t);
    const float* sdf_shapes = THFloatTensor_data(sdf_shapes_t);
    const float* weight = THFloatTensor_data(weight_t);
    const float* bias = THFloatTensor_data(bias_t); 
    const int batch_size = locs_t->size[0];
    const int N = locs_t->size[1];
    const int ndims = locs_t->size[2];
    const int M = idxs_t->size[1];
    const int pose_len = poses_t->size[2];
    const int nkernels = weight_t->size[0];
    const int ncells = weight_t->size[1];
    const float* kernel_size = THFloatTensor_data(kernel_size_t);
    const float* dilation = THFloatTensor_data(dilation_t);
    float* out = THFloatTensor_data(out_t);

    return cpu_convsdf(locs, batch_size, N, ndims, idxs, poses, scales, M, pose_len, sdfs, 
        sdf_offsets, sdf_shapes, weight, bias, nkernels, ncells, kernel_size, dilation, 
        max_distance, out, NULL);
}

int spn_convsdf_backward(const THFloatTensor* locs_t, const THFloatTensor* idxs_t, 
    const THFloatTensor* poses_t, const THFloatTensor* scales_t, const THFloatTensor* sdfs_t, 
    const THFloatTensor* sdf_offsets_t, const THFloatTensor* sdf_shapes_t, 
    const THFloatTensor* weight_t, const THFloatTensor* bias_t, 
    const THFloatTensor* kernel_size_t, const THFloatTensor* dilation_t, 
    const float max_distance, THFloatTensor* out_t, THFloatTensor* dweight_t)
{

    const float* locs = THFloatTensor_data(locs_t);
    const float* idxs = THFloatTensor_data(idxs_t);
    const float* poses = THFloatTensor_data(poses_t);
    const float* scales = THFloatTensor_data(scales_t);
    const float* sdfs = THFloatTensor_data(sdfs_t);
    const float* sdf_offsets = THFloatTensor_data(sdf_offsets_t);
    const float* sdf_shapes = THFloatTensor_data(sdf_shapes_t);
    const float* weight = THFloatTensor_data(weight_t);
    const float* bias = THFloatTensor_data(bias_t); 
    float* dweight = THFloatTensor_data(dweight_t); 
    const int batch_size = locs_t->size[0];
    const int N = locs_t->size[1];
    const int ndims = locs_t->size[2];
    const int M = idxs_t->size[1];
    const int pose_len = poses_t->size[2];
    const int nkernels = weight_t->size[0];
    const int ncells = weight_t->size[1];
    const float* kernel_size = THFloatTensor_data(kernel_size_t);
    const float* dilation = THFloatTensor_data(dilation_t);
    float* out = THFloatTensor_data(out_t);

    return cpu_convsdf(locs, batch_size, N, ndims, idxs, poses, scales, M, pose_len, sdfs, 
        sdf_offsets, sdf_shapes, weight, bias, nkernels, ncells, kernel_size, dilation, 
        max_distance, out, dweight);
}

int cpu_convsdf(const float* locs, const int batch_size, const int N, const int ndims, 
    const float* idxs, const float* poses, const float* scales, const int M, 
    const int pose_len, const float* sdfs, const float* sdf_offsets, const float* sdf_shapes, 
    const float* weight, const float* bias, const int nkernels, const int ncells, 
    const float* kernel_size, const float* dilation, const float max_distance, 
    float* out, float* dweight)
{
    int* isdf_cache = (int*)malloc(sizeof(int)*M);
    float* fsdf_cache = (float*)malloc(sizeof(float)*M);
    int b, n, outk;
    for(b = 0; b < batch_size; ++b)
    {
        for(n = 0; n < N; ++n)
        {
            for(outk = 0; outk < nkernels; ++outk)
            {
                compute_sdf_kernel_cells(locs, batch_size, N, ndims, idxs, poses, 
                    scales, M, pose_len, sdfs, sdf_offsets, sdf_shapes, weight, bias, 
                    nkernels, ncells, kernel_size, dilation, max_distance, out, b, n,
                    outk, dweight, isdf_cache, fsdf_cache);
            }
        }
    }
    free(isdf_cache);
    free(fsdf_cache);
    return 1;
}


int spn_compute_collisions(THFloatTensor* locs_t, 
                           THFloatTensor* data_t, 
                           THFloatTensor* lower_bounds_t,
                           THFloatTensor* grid_dims_t,
                           THFloatTensor* cellIDs_t,
                           THFloatTensor* idxs_t,
                           THFloatTensor* cellStarts_t,
                           THFloatTensor* cellEnds_t,
                           THFloatTensor* collisions_t,
                           const float cellEdge,
                           const float radius)
{
    float* locs = THFloatTensor_data(locs_t);
    float* data = THFloatTensor_data(data_t);
    float* low = THFloatTensor_data(lower_bounds_t);
    float* grid_dims = THFloatTensor_data(grid_dims_t);
    float* cellIDs = THFloatTensor_data(cellIDs_t);
    float* idxs = THFloatTensor_data(idxs_t);
    float* cellStarts = THFloatTensor_data(cellStarts_t);
    float* cellEnds = THFloatTensor_data(cellEnds_t);
    float* collisions = THFloatTensor_data(collisions_t);
    const int batch_size = locs_t->size[0];
    const int N = locs_t->size[1];
    const int ndims = locs_t->size[2];
    const int nchannels = data_t->size[2];
    const int max_collisions = collisions_t->size[2];
    const int ncells = cellStarts_t->size[1];

    int b, i, j, d;

    // Mark each particle with it's cell ID.
    for(b = 0; b < batch_size; ++b)
    {
        for(i = 0; i < N; ++i)
        {
            int hash = 0;
            for(d = 0; d < ndims; ++d)
                hash += partial_grid_hash(
                    loc2grid(locs[b*N*ndims + i*ndims + d], low[b*ndims + d], cellEdge), 
                    grid_dims[b*ndims + d], d);
            cellIDs[b*N + i] = hash;
            idxs[b*N + i] = i;
        }
    }

    // Sort the particles by cell ID.
    for(b = 0; b < batch_size; ++b)
    {
        // Going to use slow selection sort because it's easy to implement.
        for(i = 0; i < N; ++i)
        {
            int minID = cellIDs[b*N + i];
            int minidx = i;
            for(j = i + 1; j < N; ++j)
            {
                // Rather than swapping in place now, do it later for efficiency.
                if(cellIDs[b*N + j] < minID)
                {
                    minID = cellIDs[b*N + j];
                    minidx = j;
                }
            }

            if(minidx != i)
            {
                swapf(locs + b*N*ndims + i*ndims, locs + b*N*ndims + minidx*ndims, ndims);
                swapf(data + b*N*nchannels + i*nchannels, 
                    data + b*N*nchannels + minidx*nchannels, nchannels);
                swapf(cellIDs + b*N + i, cellIDs + b*N + minidx, 1);
                swapf(idxs + b*N + i, idxs + b*N + minidx, 1);
            }
        }
    }

    // Create the cell start and end lists.
    for(b = 0; b < batch_size; ++b)
    {
        for(i = 0; i < N; ++i)
        {
            int c = cellIDs[b*N + i];
            if (i == 0)
            {
                cellStarts[b*ncells + c] = i;
            }
            else
            {
                int p = cellIDs[b*N + i-1];

                if (c != p)
                {
                    cellStarts[b*ncells + c] = i;
                    cellEnds[b*ncells + p] = i;
                }
            }
            
            if (i == N-1)
            {
                cellEnds[b*ncells + c] = i+1;
            }
        }
    }

    // Make collision lists.
    for(b = 0; b < batch_size; ++b)
    {
        for(i = 0; i < N; ++i)
        {
            compute_collisions(
                locs,
                cellStarts,
                cellEnds,
                batch_size,
                N,
                ndims,
                ncells,
                low,
                grid_dims,
                cellEdge,
                radius*radius,
                collisions,
                max_collisions,
                b,
                i);
        }
    }

    return 1;
}

 