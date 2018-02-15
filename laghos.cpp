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
//
//                     __                __
//                    / /   ____  ____  / /_  ____  _____
//                   / /   / __ `/ __ `/ __ \/ __ \/ ___/
//                  / /___/ /_/ / /_/ / / / / /_/ (__  )
//                 /_____/\__,_/\__, /_/ /_/\____/____/
//                             /____/
//
//             High-order Lagrangian Hydrodynamics Miniapp
//
// Laghos(LAGrangian High-Order Solver) is a miniapp that solves the
// time-dependent Euler equation of compressible gas dynamics in a moving
// Lagrangian frame using unstructured high-order finite element spatial
// discretization and explicit high-order time-stepping. Laghos is based on the
// numerical algorithm described in the following article:
//
//    V. Dobrev, Tz. Kolev and R. Rieben, "High-order curvilinear finite element
//    methods for Lagrangian hydrodynamics", SIAM Journal on Scientific
//    Computing, (34) 2012, pp.B606–B641, https://doi.org/10.1137/120864672.
//
// Sample runs:
//    mpirun -np 8 laghos -p 0 -m data/square01_quad.mesh -rs 3 -tf 0.75
//    mpirun -np 8 laghos -p 0 -m data/square01_tri.mesh  -rs 1 -tf 0.75
//    mpirun -np 8 laghos -p 0 -m data/cube01_hex.mesh    -rs 1 -tf 2.0
//    mpirun -np 8 laghos -p 1 -m data/square01_quad.mesh -rs 3 -tf 0.8
//    mpirun -np 8 laghos -p 1 -m data/square01_quad.mesh -rs 0 -tf 0.8 -ok 7 -ot 6
//    mpirun -np 8 laghos -p 1 -m data/cube01_hex.mesh    -rs 2 -tf 0.6
//    mpirun -np 8 laghos -p 2 -m data/segment01.mesh     -rs 5 -tf 0.2
//    mpirun -np 8 laghos -p 3 -m data/rectangle01_quad.mesh -rs 2 -tf 2.5
//    mpirun -np 8 laghos -p 3 -m data/box01_hex.mesh        -rs 1 -tf 2.5
//
// Test problems:
//    p = 0  --> Taylor-Green vortex (smooth problem).
//    p = 1  --> Sedov blast.
//    p = 2  --> 1D Sod shock tube.
//    p = 3  --> Triple point.


#include "laghos_solver.hpp"
#include <memory>
#include <iostream>
#include <fstream>
#include <sys/time.h>

using namespace std;
using namespace mfem;
using namespace mfem::hydrodynamics;

// Choice for the problem setup.
int problem = 0;

void display_banner(ostream & os);

int main(int argc, char *argv[])
{
    // Initialize MPI.
   MPI_Session mpi(argc, argv); 
   int myid = mpi.WorldRank();
   
   // Print the banner.
   if (mpi.Root()) { display_banner(cout); }

   // Parse command-line options.
   const char *mesh_file = "data/square01_quad.mesh";
   int rs_levels = 0;
   int rp_levels = 0;
   int order_v = 2;
   int order_e = 1;
   int ode_solver_type = 4;
   double t_final = 0.5;
   double cfl = 0.5;
   double cg_tol = 1e-8;
   int cg_max_iter = 300;
   int max_tsteps = -1;
   bool p_assembly = true;
   bool visualization = false;
   int vis_steps = 5;
   bool visit = false;
   bool gfprint = false;
   bool dot = false;
   bool cuda = false;
   bool uvm = false;
   bool share = false;
   bool like_occa = false;
   
#ifdef __NVCC__
#ifndef __RAJA__
   cuda=true;
#endif
#endif
   const char *basename = "results/Laghos";

   OptionsParser args(argc, argv);
   args.AddOption(&mesh_file, "-m", "--mesh",
                  "Mesh file to use.");
   args.AddOption(&rs_levels, "-rs", "--refine-serial",
                  "Number of times to refine the mesh uniformly in serial.");
   args.AddOption(&rp_levels, "-rp", "--refine-parallel",
                  "Number of times to refine the mesh uniformly in parallel.");
   args.AddOption(&problem, "-p", "--problem", "Problem setup to use.");
   args.AddOption(&order_v, "-ok", "--order-kinematic",
                  "Order (degree) of the kinematic finite element space.");
   args.AddOption(&order_e, "-ot", "--order-thermo",
                  "Order (degree) of the thermodynamic finite element space.");
   args.AddOption(&ode_solver_type, "-s", "--ode-solver",
                  "ODE solver: 1 - Forward Euler,\n\t"
                  "            2 - RK2 SSP, 3 - RK3 SSP, 4 - RK4, 6 - RK6.");
   args.AddOption(&t_final, "-tf", "--t-final",
                  "Final time; start time is 0.");
   args.AddOption(&cfl, "-cfl", "--cfl", "CFL-condition number.");
   args.AddOption(&cg_tol, "-cgt", "--cg-tol",
                  "Relative CG tolerance (velocity linear solve).");
   args.AddOption(&cg_max_iter, "-cgm", "--cg-max-steps",
                  "Maximum number of CG iterations (velocity linear solve).");
   args.AddOption(&max_tsteps, "-ms", "--max-steps",
                  "Maximum number of steps (negative means no restriction).");
   args.AddOption(&p_assembly, "-pa", "--partial-assembly", "-fa",
                  "--full-assembly",
                  "Activate 1D tensor-based assembly (partial assembly).");
   args.AddOption(&visualization, "-vis", "--visualization", "-no-vis",
                  "--no-visualization",
                  "Enable or disable GLVis visualization.");
   args.AddOption(&vis_steps, "-vs", "--visualization-steps",
                  "Visualize every n-th timestep.");
   args.AddOption(&visit, "-visit", "--visit", "-no-visit", "--no-visit",
                  "Enable or disable VisIt visualization.");
   args.AddOption(&gfprint, "-print", "--print", "-no-print", "--no-print",
                  "Enable or disable result output (files in mfem format).");
   args.AddOption(&basename, "-k", "--outputfilename",
                  "Name of the visit dump files");
   args.AddOption(&dot, "-dot", "--dot", "-no-dot", "--no-dot",
                  "Enable or disable DOT test kernels.");
   args.AddOption(&uvm, "-uvm", "--uvm", "-no-uvm", "--no-uvm",
                  "Enable or disable CUDA managed alloc.");
   args.AddOption(&cuda, "-cuda", "--cuda", "-no-cuda", "--no-cuda",
                  "Enable or disable CUDA kernels.");
   args.AddOption(&share, "-share", "--share", "-no-share", "--no-share",
                  "Enable or disable SHARE kernels.");
   args.AddOption(&like_occa, "-like-occa", "--like-occa", "-not-like-occa", "--no-like-occa",
                  "Enable or disable like-occa test kernels.");
   args.Parse();
   if (!args.Good())
   {
      if (mpi.Root()) { args.PrintUsage(cout); }
      return 1;
   }
   if (mpi.Root()) { args.PrintOptions(cout); }

   // CUDA set device & tweak options
   // **************************************************************************
   rconfig::Get().Setup(mpi.WorldRank(),mpi.WorldSize(),cuda,uvm,share,like_occa);
   
   // **************************************************************************
   if (dot){
     cuProfilerStart();
     struct timeval st, et;
     int size = 0x400;
     for (int lev = 0; lev < rs_levels; lev++) size<<=1;
     //Vector h_a(size); h_a=1.0/M_PI;
     //Vector h_b(size); h_b=M_PI;
     gettimeofday(&st, NULL);
     RajaVector a(size);a=1.0/M_PI;//h_a;//a.Print();
     RajaVector b(size);b=M_PI;//(h_b); //b.Print();
     //RajaVector c(size); c=0.0;
     gettimeofday(&et, NULL);
     const double setTime = ((et.tv_sec-st.tv_sec)*1000.0+(et.tv_usec-st.tv_usec)/1000.0);
     printf("\033[32m[laghos] Set in \033[1m%12.6e(s)\033[m\n",setTime/1000.0);
     gettimeofday(&st, NULL);
     //double dt = a*b;
     //c+=1.0;
     a+=b;
     gettimeofday(&et, NULL);
     //assert(dt == (double)size);
     const double alltime = ((et.tv_sec-st.tv_sec)*1000.0+(et.tv_usec-st.tv_usec)/1000.0);
     printf("\033[32m[laghos] Ker (%d) in \033[1m%12.6e(s)\033[m\n",size,alltime/1000.0);
     return 0;
   }

   // Read the serial mesh from the given mesh file on all processors.
   // Refine the mesh in serial to increase the resolution.
   Mesh *mesh = new Mesh(mesh_file, 1, 1);
   const int dim = mesh->Dimension();
   for (int lev = 0; lev < rs_levels; lev++) { mesh->UniformRefinement(); }

   if (p_assembly && dim == 1)
   {
      p_assembly = false;
      if (mpi.Root())
      {
         cout << "Laghos does not support PA in 1D. Switching to FA." << endl;
      }
   }

   // Parallel partitioning of the mesh.
   ParMesh *pmesh = NULL;
   const int num_tasks = mpi.WorldSize();
   const int partitions = floor(pow(num_tasks, 1.0 / dim) + 1e-2);
   int *nxyz = new int[dim];
   int product = 1;
   for (int d = 0; d < dim; d++)
   {
      nxyz[d] = partitions;
      product *= partitions;
   }
   if (product == num_tasks)
   {
      if (myid == 0)
        printf("\033[32m[laghos] \033[32;1mCartesian\033[m\033[32m partitioning will be used\033[m\n");
      int *partitioning = mesh->CartesianPartitioning(nxyz);
      pmesh = new ParMesh(MPI_COMM_WORLD, *mesh, partitioning);
      delete[] partitioning;
   }
   else
   {
      if (myid == 0)
      {
        printf("\033[32m[laghos] Non-Cartesian partitioning through METIS will be used\033[m\n");
#ifndef MFEM_USE_METIS
         cout << "MFEM was built without METIS. "
              << "Adjust the number of tasks to use a Cartesian split." << endl;
#endif
      }
#ifndef MFEM_USE_METIS
      return 1;
#endif
      pmesh = new ParMesh(MPI_COMM_WORLD, *mesh);
   }
   delete [] nxyz;
   delete mesh;
   

   // Refine the mesh further in parallel to increase the resolution.
   for (int lev = 0; lev < rp_levels; lev++) { pmesh->UniformRefinement(); }

   // **************************************************************************
   //cuProfilerStart();
   push(Tan);

   // Define the parallel finite element spaces. We use:
   // - H1 (Gauss-Lobatto, continuous) for position and velocity.
   // - L2 (Bernstein, discontinuous) for specific internal energy.
   //dbg()<<"\033[7mDefine the parallel finite element spaces";
   L2_FECollection L2FEC(order_e, dim, BasisType::Positive);
   H1_FECollection H1FEC(order_v, dim);
   //dbg()<<"\033[7mL2FESpace RajaFiniteElementSpace";
   RajaFiniteElementSpace L2FESpace(pmesh, &L2FEC);
   //dbg()<<"\033[7mH1FESpace RajaFiniteElementSpace";
   RajaFiniteElementSpace H1FESpace(pmesh, &H1FEC, pmesh->Dimension());

   // Boundary conditions: all tests use v.n = 0 on the boundary, and we assume
   // that the boundaries are straight.
   //dbg()<<"\033[7mBoundary conditions";
   Array<int> essential_tdofs;
   {
      Array<int> ess_bdr(pmesh->bdr_attributes.Max()), tdofs1d;
      for (int d = 0; d < pmesh->Dimension(); d++)
      {
         // Attributes 1/2/3 correspond to fixed-x/y/z boundaries, i.e., we must
         // enforce v_x/y/z = 0 for the velocity components.
         ess_bdr = 0; ess_bdr[d] = 1;
         H1FESpace.GetEssentialTrueDofs(ess_bdr, tdofs1d, d);
         essential_tdofs.Append(tdofs1d);
      }
   }

   // Define the explicit ODE solver used for time integration.
   //dbg()<<"\033[7mDefine the explicit ODE solver";
   RajaODESolver *ode_solver = NULL;
   switch (ode_solver_type) {
   case 1: ode_solver = new RajaForwardEulerSolver; break;
   case 2: ode_solver = new RajaRK2Solver(0.5); break;
   case 3: ode_solver = new RajaRK3SSPSolver; break;
   case 4: ode_solver = new RajaRK4Solver; break;
   case 6: ode_solver = new RajaRK6Solver; break;
   default:
     if (myid == 0)
     {
       cout << "Unknown ODE solver type: " << ode_solver_type << '\n';
     }
     delete pmesh;
     MPI_Finalize();
     pop();
     return 3;
   }

   HYPRE_Int glob_size_l2 = L2FESpace.GlobalTrueVSize();
   HYPRE_Int glob_size_h1 = H1FESpace.GlobalTrueVSize();

   if (mpi.Root())
   {
      cout << "Number of kinematic (position, velocity) dofs: "
           << glob_size_h1 << endl;
      cout << "Number of specific internal energy dofs: "
           << glob_size_l2 << endl<< endl;
   }

   int Vsize_l2 = L2FESpace.GetVSize();
   int Vsize_h1 = H1FESpace.GetVSize();

   // The monolithic BlockVector stores unknown fields as:
   // - 0 -> position
   // - 1 -> velocity
   // - 2 -> specific internal energy
   //dbg()<<"[7mS monolithic BlockVector";
   Array<int> true_offset(4);
   true_offset[0] = 0;
   true_offset[1] = true_offset[0] + Vsize_h1;
   true_offset[2] = true_offset[1] + Vsize_h1;
   true_offset[3] = true_offset[2] + Vsize_l2;
   RajaVector S(true_offset[3]);

   // Define GridFunction objects for the position, velocity and specific
   // internal energy.  There is no function for the density, as we can always
   // compute the density values given the current mesh position, using the
   // property of pointwise mass conservation.
   //dbg()<<"[7mParGridFunction: x,v,e";
   ParGridFunction x_gf(&H1FESpace);
   ParGridFunction v_gf(&H1FESpace);
   ParGridFunction e_gf(&L2FESpace);

   //dbg()<<"[7mRajaGridFunction: d_x_gf";
   RajaGridFunction d_x_gf(H1FESpace, S.GetRange(true_offset[0], true_offset[1]));
   //dbg()<<"[7mRajaGridFunction: d_v_gf";
   RajaGridFunction d_v_gf(H1FESpace, S.GetRange(true_offset[1], true_offset[2]));
   //dbg()<<"[7mRajaGridFunction: d_e_gf";
   RajaGridFunction d_e_gf(L2FESpace, S.GetRange(true_offset[2], true_offset[3]));
   
   // Initialize x_gf using the starting mesh coordinates. This also links the
   // mesh positions to the values in x_gf.
   //dbg()<<"[7mSetNodalGridFunction";
   pmesh->SetNodalGridFunction(&x_gf);
   //dbg()<<"[7md_x_gf = x_gf;";
   d_x_gf = x_gf;
   
   // Initialize the velocity.
   //dbg()<<"[7mInitialize the velocity";
   VectorFunctionCoefficient v_coeff(pmesh->Dimension(), v0);
   v_gf.ProjectCoefficient(v_coeff);
   d_v_gf = v_gf;

   // Initialize density and specific internal energy values. We interpolate in
   // a non-positive basis to get the correct values at the dofs.  Then we do an
   // L2 projection to the positive basis in which we actually compute. The goal
   // is to get a high-order representation of the initial condition. Note that
   // this density is a temporary function and it will not be updated during the
   // time evolution.
   //dbg()<<"[7mInitialize density and specific internal energy";
   ParGridFunction rho(&L2FESpace);
   FunctionCoefficient rho_coeff(hydrodynamics::rho0);
   L2_FECollection l2_fec(order_e, pmesh->Dimension());
   RajaFiniteElementSpace l2_fes(pmesh, &l2_fec);
   ParGridFunction l2_rho(&l2_fes), l2_e(&l2_fes);
   l2_rho.ProjectCoefficient(rho_coeff);
   rho.ProjectGridFunction(l2_rho);
   RajaGridFunction d_rho(L2FESpace);
   d_rho = rho;
   
   //dbg()<<"[7mproblem 1 or else[m";
   if (problem == 1)
   {
      // For the Sedov test, we use a delta function at the origin.
      DeltaCoefficient e_coeff(0, 0, 0.25);
      l2_e.ProjectCoefficient(e_coeff);
   }
   else
   {
      FunctionCoefficient e_coeff(e0);
      l2_e.ProjectCoefficient(e_coeff);
   }
   e_gf.ProjectGridFunction(l2_e);
   d_e_gf = e_gf;

   //dbg()<<"[7mSpace-dependent ideal gas coefficient over the Lagrangian mesh.";
   Coefficient *material_pcf = new FunctionCoefficient(hydrodynamics::gamma);

   //dbg()<<"[7mAdditional details, depending on the problem.";
   int source = 0; bool visc=false;
   switch (problem)
   {
      case 0: if (pmesh->Dimension() == 2) { source = 1; }
         visc = false; break;
      case 1: visc = true; break;
      case 2: visc = true; break;
      case 3: visc = true; break;
      default: MFEM_ABORT("Wrong problem specification!");
   }

   //dbg()<<"[7mLagrangianHydroOperator oper";
   LagrangianHydroOperator oper(S.Size(), H1FESpace, L2FESpace,
                                essential_tdofs, d_rho, source, cfl, material_pcf,
                                visc, p_assembly, cg_tol, cg_max_iter);

   //dbg()<<"[7msocketstream";
   socketstream vis_rho, vis_v, vis_e;
   char vishost[] = "localhost";
   int  visport   = 19916;

   ParGridFunction rho_gf;
   if (visualization || visit) { oper.ComputeDensity(rho_gf); }

   if (visualization)
   {
      // Make sure all MPI ranks have sent their 'v' solution before initiating
      // another set of GLVis connections (one from each rank):
      MPI_Barrier(pmesh->GetComm());

      vis_rho.precision(8);
      vis_v.precision(8);
      vis_e.precision(8);

      int Wx = 0, Wy = 0; // window position
      const int Ww = 350, Wh = 350; // window size
      int offx = Ww+10; // window offsets

      VisualizeField(vis_rho, vishost, visport, rho_gf,
                     "Density", Wx, Wy, Ww, Wh);
      Wx += offx;
      VisualizeField(vis_v, vishost, visport, v_gf,
                     "Velocity", Wx, Wy, Ww, Wh);
      Wx += offx;
      VisualizeField(vis_e, vishost, visport, e_gf,
                     "Specific Internal Energy", Wx, Wy, Ww, Wh);
   }

   // Save data for VisIt visualization
   VisItDataCollection visit_dc(basename, pmesh);
   if (visit)
   {
      visit_dc.RegisterField("Density",  &rho_gf);
      visit_dc.RegisterField("Velocity", &v_gf);
      visit_dc.RegisterField("Specific Internal Energy", &e_gf);
      visit_dc.SetCycle(0);
      visit_dc.SetTime(0.0);
      visit_dc.Save();
   }

   // Perform time-integration (looping over the time iterations, ti, with a
   // time-step dt). The object oper is of type LagrangianHydroOperator that
   // defines the Mult() method that used by the time integrators.
   ode_solver->Init(oper);
   oper.ResetTimeStepEstimate();
   
   //dbg()<<"[7mResetTimeStepEstimate, GetTimeStepEstimate";
   double t = 0.0, dt = oper.GetTimeStepEstimate(S), t_old;
   bool last_step = false;
   int steps = 0;
   //dbg()<<"[7mS_old(S)";
   RajaVector S_old(S);//D2D
   
   //dbg("[7mfor(last_step)");
   for (int ti = 1; !last_step; ti++)
   {
      if (t + dt >= t_final)
      {
         dt = t_final - t;
         last_step = true;
      }
      if (steps == max_tsteps) { last_step = true; }

      S_old = S;//D2D
      t_old = t;
      //dbg("[7mResetTimeStepEstimate");
      oper.ResetTimeStepEstimate();

      // S is the vector of dofs, t is the current time, and dt is the time step
      // to advance.
      //dbg()<<"[7mRode_solver->Step";
      cuProfilerStart();
      ode_solver->Step(S, t, dt);
      steps++;
      //cuProfilerStop();
//#warning exit
//      MPI_Finalize();
//      exit(0);

      // Make sure that the mesh corresponds to the new solution state.
      //dbg("[7mMake sure that the mesh corresponds to the new solution state.");
      //#warning NewNodes x_gf
      x_gf = d_x_gf;
      pmesh->NewNodes(x_gf, false);

      // Adaptive time step control.
      //dbg()<<"[7mAdaptive time step control";
      const double dt_est = oper.GetTimeStepEstimate(S);
      if (dt_est < dt)
      {
         // Repeat (solve again) with a decreased time step - decrease of the
         // time estimate suggests appearance of oscillations.
         dt *= 0.85;
         if (dt < numeric_limits<double>::epsilon())
         { MFEM_ABORT("The time step crashed!"); }
         t = t_old;
         S = S_old;
         oper.ResetQuadratureData();
         if (mpi.Root()) { cout << "Repeating step " << ti << endl; }
         ti--; continue;
      }
      else if (dt_est > 1.25 * dt) { dt *= 1.02; }
      

      if (last_step || (ti % vis_steps) == 0)
      {
         double loc_norm = d_e_gf * d_e_gf, tot_norm;
         MPI_Allreduce(&loc_norm, &tot_norm, 1, MPI_DOUBLE, MPI_SUM,
                       pmesh->GetComm());
         if (mpi.Root())
         {
            cout << fixed;
            cout << "step " << setw(5) << ti
                 << ",\tt = " << setw(5) << setprecision(4) << t
                 << ",\tdt = " << setw(5) << setprecision(6) << dt
                 << ",\t|e| = " << setprecision(10)
                 << sqrt(tot_norm) << endl;
         }

         // Make sure all ranks have sent their 'v' solution before initiating
         // another set of GLVis connections (one from each rank):
         MPI_Barrier(pmesh->GetComm());

         if (visualization || visit || gfprint) { oper.ComputeDensity(rho_gf); }
         if (visualization)
         {
            int Wx = 0, Wy = 0; // window position
            int Ww = 350, Wh = 350; // window size
            int offx = Ww+10; // window offsets

            VisualizeField(vis_rho, vishost, visport, rho_gf,
                           "Density", Wx, Wy, Ww, Wh);
            Wx += offx;
            VisualizeField(vis_v, vishost, visport,
                           v_gf, "Velocity", Wx, Wy, Ww, Wh);
            Wx += offx;
            VisualizeField(vis_e, vishost, visport, e_gf,
                           "Specific Internal Energy", Wx, Wy, Ww,Wh);
            Wx += offx;
         }

         if (visit)
         {
            visit_dc.SetCycle(ti);
            visit_dc.SetTime(t);
            visit_dc.Save();
         }

         if (gfprint)
         {
            ostringstream mesh_name, rho_name, v_name, e_name;
            mesh_name << basename << "_" << ti
                      << "_mesh." << setfill('0') << setw(6) << myid;
            rho_name  << basename << "_" << ti
                      << "_rho." << setfill('0') << setw(6) << myid;
            v_name << basename << "_" << ti
                   << "_v." << setfill('0') << setw(6) << myid;
            e_name << basename << "_" << ti
                   << "_e." << setfill('0') << setw(6) << myid;

            ofstream mesh_ofs(mesh_name.str().c_str());
            mesh_ofs.precision(8);
            pmesh->Print(mesh_ofs);
            mesh_ofs.close();

            ofstream rho_ofs(rho_name.str().c_str());
            rho_ofs.precision(8);
            rho_gf.Save(rho_ofs);
            rho_ofs.close();

            ofstream v_ofs(v_name.str().c_str());
            v_ofs.precision(8);
            v_gf.Save(v_ofs);
            v_ofs.close();

            ofstream e_ofs(e_name.str().c_str());
            e_ofs.precision(8);
            e_gf.Save(e_ofs);
            e_ofs.close();
         }
      }
   }

   switch (ode_solver_type)
   {
      case 2: steps *= 2; break;
      case 3: steps *= 3; break;
      case 4: steps *= 4; break;
      case 6: steps *= 6;
   }
   oper.PrintTimingData(mpi.Root(), steps);

   if (visualization)
   {
      vis_v.close();
      vis_e.close();
   }

   // Free the used memory.
   delete ode_solver;
   delete pmesh;
   delete material_pcf;
   //RajaDofQuadMaps::delRajaDofQuadMaps();
   pop();
   return 0;
}

namespace mfem
{

namespace hydrodynamics
{

double rho0(const Vector &x)
{
   switch (problem)
   {
      case 0: return 1.0;
      case 1: return 1.0;
      case 2: if (x(0) < 0.5) { return 1.0; }
         else { return 0.1; }
      case 3: if (x(0) > 1.0 && x(1) <= 1.5) { return 1.0; }
         else { return 0.125; }
      default: MFEM_ABORT("Bad number given for problem id!"); return 0.0;
   }
}

double gamma(const Vector &x)
{
   switch (problem)
   {
      case 0: return 5./3.;
      case 1: return 1.4;
      case 2: return 1.4;
      case 3: if (x(0) > 1.0 && x(1) <= 1.5) { return 1.4; }
         else { return 1.5; }
      default: MFEM_ABORT("Bad number given for problem id!"); return 0.0;
   }
}

void v0(const Vector &x, Vector &v)
{
   switch (problem)
   {
      case 0:
         v(0) =  sin(M_PI*x(0)) * cos(M_PI*x(1));
         v(1) = -cos(M_PI*x(0)) * sin(M_PI*x(1));
         if (x.Size() == 3)
         {
            v(0) *= cos(M_PI*x(2));
            v(1) *= cos(M_PI*x(2));
            v(2) = 0.0;
         }
         break;
      case 1: v = 0.0; break;
      case 2: v = 0.0; break;
      case 3: v = 0.0; break;
      default: MFEM_ABORT("Bad number given for problem id!");
   }
}

double e0(const Vector &x)
{
   switch (problem)
   {
      case 0:
      {
         const double denom = 2.0 / 3.0;  // (5/3 - 1) * density.
         double val;
         if (x.Size() == 2)
         {
            val = 1.0 + (cos(2*M_PI*x(0)) + cos(2*M_PI*x(1))) / 4.0;
         }
         else
         {
            val = 100.0 + ((cos(2*M_PI*x(2)) + 2) *
                           (cos(2*M_PI*x(0)) + cos(2*M_PI*x(1))) - 2) / 16.0;
         }
         return val/denom;
      }
      case 1: return 0.0; // This case in initialized in main().
      case 2: if (x(0) < 0.5) { return 1.0 / rho0(x) / (gamma(x) - 1.0); }
         else { return 0.1 / rho0(x) / (gamma(x) - 1.0); }
      case 3: if (x(0) > 1.0) { return 0.1 / rho0(x) / (gamma(x) - 1.0); }
         else { return 1.0 / rho0(x) / (gamma(x) - 1.0); }
      default: MFEM_ABORT("Bad number given for problem id!"); return 0.0;
   }
}

} // namespace hydrodynamics

} // namespace mfem

void display_banner(ostream & os)
{
   os << endl
      << "       __                __                 " << endl
      << "      / /   ____  ____  / /_  ____  _____   " << endl
      << "     / /   / __ `/ __ `/ __ \\/ __ \\/ ___/ " << endl
      << "    / /___/ /_/ / /_/ / / / / /_/ (__  )    " << endl
      << "   /_____/\\__,_/\\__, /_/ /_/\\____/____/  " << endl
      << "               /____/                       " << endl << endl;
}
