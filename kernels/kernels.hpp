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
#ifndef LAGHOS_KERNELS_KERNELS_HPP
#define LAGHOS_KERNELS_KERNELS_HPP

// *****************************************************************************
#include <math.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>

// *****************************************************************************
#ifdef __NVCC__
#include <cuda.h>
#include <helper_cuda.h>
#include <helper_functions.h>
#endif // __NVCC__

// *****************************************************************************
#ifdef __RAJA__
#include <cuda.h>
#include "RAJA/RAJA.hpp"
#include "RAJA/policy/cuda.hpp"
#endif // __RAJA__

// *****************************************************************************
#include "force/force.hpp"
#include "include/forall.hpp"
#include "include/offsets.hpp"

#endif // LAGHOS_KERNELS_KERNELS_HPP
