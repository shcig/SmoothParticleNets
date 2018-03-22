import os
import sys
# Add path to python source to path.
sys.path.append(os.path.join(os.path.dirname(os.path.dirname(
    os.path.abspath(__file__))), "python"))
import SmoothParticleNets as spn

import itertools
import numpy as np
import torch
import torch.autograd

from gradcheck import gradcheck
try:
    import pytest_args
except ImportError:
    print("Make sure to compile SmoothParticleNets before running tests.")
    raise


def test_particlecollision(cpu=True, cuda=True):
    if cpu:
        print("Testing CPU implementation of ParticleCollision...")
        eval_particlecollision(cuda=False)
        print("CPU implementation passed!")
        print("")

    if cuda:
        if pytest_args.with_cuda:
            print("Testing CUDA implementation of ParticleCollision...")
            eval_particlecollision(cuda=True)
            print("CUDA implementation passed!")
        else:
            print("Not compiled with CUDA, skipping CUDA test.")

def eval_particlecollision(cuda=False):
    BATCH_SIZE = 2
    N = 10
    NDIM = 2
    RADIUS = 0.6
    NCHANNELS = 2

    np.random.seed(0)

    locs = np.random.rand(BATCH_SIZE, N, NDIM).astype(np.float32)
    data = np.random.rand(BATCH_SIZE, N, NCHANNELS).astype(np.float32)

    gt_neighbors = np.ones((BATCH_SIZE, N, N), dtype=int)*-1
    for b in range(BATCH_SIZE):
        for i in range(N):
            for j in range(N):
                d = np.square(locs[b, i, :] - locs[b, j, :]).sum()
                if d <= RADIUS*RADIUS:
                    nc = min(np.where(gt_neighbors[b, i, :] < 0)[0])
                    gt_neighbors[b, i, nc] = j

    def use_cuda(x):
        if cuda:
            return x.cuda()
        else:
            return x
    def undo_cuda(x):
        if cuda:
            return x.cpu()
        else:
            return x

    olocs = locs
    odata = data
    locs = torch.autograd.Variable(use_cuda(torch.FloatTensor(locs.copy())), 
        requires_grad=False)
    data = torch.autograd.Variable(use_cuda(torch.FloatTensor(data.copy())), 
        requires_grad=False)

    coll = spn.ParticleCollision(NDIM, RADIUS)
    convsp = use_cuda(coll)

    idxs, neighbors = [undo_cuda(x) for x in coll(locs, data)]
    
    idxs = idxs.data.numpy().astype(int)
    neighbors = neighbors.data.numpy().astype(int)
    nlocs = undo_cuda(locs).data.numpy()
    ndata = undo_cuda(data).data.numpy()

    # First make sure all the indexes are in idxs.
    for b in range(BATCH_SIZE):
        for i in range(N):
            assert i in idxs[b, :]

    # Next make sure locs and data are in the order idxs says they're in.
    for b in range(BATCH_SIZE):
        for i, j in enumerate(idxs[b, :]):
            assert all(olocs[b, j, :] == nlocs[b, i, :])
            assert all(odata[b, j, :] == ndata[b, i, :])

    # Finally check the neighbor list.
    for b in range(BATCH_SIZE):
        for i in range(N):
            for j in neighbors[b, i, :]:
                if j < 0:
                    break
                assert idxs[b, j] in gt_neighbors[b, idxs[b, i], :]
            for j in gt_neighbors[b, idxs[b, i], :]:
                if j < 0:
                    break
                jj = np.where(idxs[b, :] == j)[0][0]
                assert jj in neighbors[b, i, :]



if __name__ == '__main__':
    import argparse
    parser = argparse.ArgumentParser()
    parser.add_argument('--cpu', dest='cpu', action="store_true", default=True)
    parser.add_argument('--no-cpu', dest='cpu', action="store_false")
    parser.add_argument('--cuda', dest='cuda', action="store_true", default=True)
    parser.add_argument('--no-cuda', dest='cuda', action="store_false")
    args = parser.parse_args()
    test_particlecollision(cpu=args.cpu, cuda=args.cuda)