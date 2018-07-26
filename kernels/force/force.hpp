// Copyright (c) 2017, Lawrence Livermore National Security, LLC. Produced at
// the Lawrence Livermore National Laboratory. LLNL-CODE-734707. All Rights
// reserved. See files LICENSE and NOTICE for details.
//
// This file is part of CEED, a collection of benchmarks, miniapps, software
// libraries and APIs for efficient high-order finite element and spectral
// element discretizations for exascale applications. For more information and
// source code availability see http://github.com/ceed.
//
// The CEED research is supported by the Exascale Computing Project 17-SC-20-SC,
// a collaborative effort of two U.S. Department of Energy organizations (Office
// of Science and the National Nuclear Security Administration) responsible for
// the planning and preparation of a capable exascale ecosystem, including
// software, applications, hardware, advanced system engineering and early
// testbed platforms, in support of the nation's exascale computing imperative.
#ifndef LAGHOS_KERNELS_FORCE_HPP
#define LAGHOS_KERNELS_FORCE_HPP

void rForceMult(const int NUM_DIM,
                const int NUM_DOFS_1D,
                const int NUM_QUAD_1D,
                const int L2_DOFS_1D,
                const int H1_DOFS_1D,
                const int nzones,
                const double* L2QuadToDof,
                const double* H1DofToQuad,
                const double* H1DofToQuadD,
                const double* stressJinvT,
                const double* e,
                double* v);

void rForceMultTranspose(const int NUM_DIM,
                         const int NUM_DOFS_1D,
                         const int NUM_QUAD_1D,
                         const int L2_DOFS_1D,
                         const int H1_DOFS_1D,
                         const int nzones,
                         const double* L2QuadToDof,
                         const double* H1DofToQuad,
                         const double* H1DofToQuadD,
                         const double* stressJinvT,
                         const double* v,
                         double* e);

#endif // LAGHOS_KERNELS_FORCE_HPP
