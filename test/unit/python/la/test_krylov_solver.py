#!/usr/bin/env py.test

"""Unit tests for the KrylovSolver interface"""

# Copyright (C) 2014 Garth N. Wells
#
# This file is part of DOLFIN.
#
# DOLFIN is free software: you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# DOLFIN is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with DOLFIN. If not, see <http://www.gnu.org/licenses/>.

from __future__ import print_function
import pytest
from dolfin import *
from dolfin_utils.test import skip_if_not_PETSc, skip_in_parallel

@skip_if_not_PETSc
def test_krylov_samg_solver_elasticity():
    "Test PETScKrylovSolver with smoothed aggregation AMG"

    # Set backend
    previous_backend = parameters["linear_algebra_backend"]
    parameters["linear_algebra_backend"] = "PETSc"

    def build_nullspace(V, x):
        """Function to build null space for 2D elasticity"""

        # Create list of vectors for null space
        nullspace_basis = [x.copy() for i in range(3)]

        # Build translational null space basis
        V.sub(0).dofmap().set(nullspace_basis[0], 1.0);
        V.sub(1).dofmap().set(nullspace_basis[1], 1.0);

        # Build rotational null space basis
        V.sub(0).dofmap().set_x(nullspace_basis[2], -1.0, 1, V.mesh());
        V.sub(1).dofmap().set_x(nullspace_basis[2], 1.0, 0, V.mesh());

        for x in nullspace_basis:
            x.apply("insert")

        return VectorSpaceBasis(nullspace_basis)

    def amg_solve(N, method):
        # Elasticity parameters
        E = 1.0e9
        nu = 0.3
        mu = E/(2.0*(1.0 + nu))
        lmbda = E*nu/((1.0 + nu)*(1.0 - 2.0*nu))

        # Stress computation
        def sigma(v):
            return 2.0*mu*sym(grad(v)) + lmbda*tr(sym(grad(v)))*Identity(2)

        # Define problem
        mesh = UnitSquareMesh(N, N)
        V = VectorFunctionSpace(mesh, 'Lagrange', 1)
        bc = DirichletBC(V, Constant((0.0, 0.0)),
                         lambda x, on_boundary: on_boundary)
        u = TrialFunction(V)
        v = TestFunction(V)

        # Forms
        a, L = inner(sigma(u), grad(v))*dx, dot(Constant((1.0, 1.0)), v)*dx

        # Assemble linear algebra objects
        A, b = assemble_system(a, L, bc)

        # Create solution function
        u = Function(V)

        # Create near null space basis
        null_space = build_nullspace(V, u.vector())

        # Create PETSC smoothed aggregation AMG preconditioner,
        # attach near null space and create CG solver
        pc = PETScPreconditioner(method)
        pc.set_nullspace(null_space)
        solver = PETScKrylovSolver("cg", pc)

        # Set matrix operator
        solver.set_operator(A)

        # Compute solution and return number of iterations
        return solver.solve(u.vector(), b)


    # Set some multigrid smoother parameters
    PETScOptions.set("mg_levels_ksp_type", "chebyshev")
    PETScOptions.set("mg_levels_pc_type", "jacobi")

    # Improve estimate of eigenvalues for Chebyshev smoothing (PETSc
    # version<3.6 option)
    PETScOptions.set("mg_levels_est_ksp_type", "cg")
    PETScOptions.set("mg_levels_est_ksp_max_it", 50)
    PETScOptions.set("gamg_est_ksp_type", "cg")
    PETScOptions.set("gamg_est_ksp_max_it", 50)

    # Improve estimate of eigenvalues for Chebyshev smoothing (more
    # recent PETSc option)
    PETScOptions.set("mg_levels_ksp_chebyshev_esteig_steps", 50)

    # Build list of smoothed aggregation preconditioners
    methods = ["petsc_amg"]
    if "ml_amg" in PETScPreconditioner.preconditioners():
        methods.append("ml_amg")

    # Test iteration count with increasing mesh size for each
    # preconditioner
    for method in methods:
        for N in [4, 8, 16, 32, 64]:
            print("Testing method '{}' with {} x {} mesh".format(method, N, N))
            niter = amg_solve(N, method)
            assert niter < 18

    parameters["linear_algebra_backend"] = previous_backend


@skip_if_not_PETSc
def test_krylov_reuse_pc():
    "Test preconditioner re-use with PETScKrylovSolver"

    # Test requires PETSc version 3.5 or later. Use petsc4py to check
    # version number.
    try:
        from petsc4py import PETSc
    except ImportError:
        pytest.skip("petsc4py required to check PETSc version")
    else:
        if not PETSc.Sys.getVersion() >= (3, 5, 0):
            pytest.skip("PETSc version must be 3.5  of higher")

    # Define problem
    mesh = UnitSquareMesh(8, 8)
    V = FunctionSpace(mesh, 'Lagrange', 1)
    bc = DirichletBC(V, Constant(0.0), lambda x, on_boundary: on_boundary)
    u = TrialFunction(V)
    v = TestFunction(V)

    # Forms
    a, L = inner(grad(u), grad(v))*dx, dot(Constant(1.0), v)*dx

    A, P = PETScMatrix(), PETScMatrix()
    b = PETScVector()

    # Assemble linear algebra objects
    assemble(a, tensor=A)
    assemble(a, tensor=P)
    assemble(L, tensor=b)

    # Apply boundary conditions
    bc.apply(A)
    bc.apply(P)
    bc.apply(b)

    # Create Krysolv solver and set operators
    solver = PETScKrylovSolver("gmres", "bjacobi")
    solver.set_operators(A, P)

    # Solve
    x = PETScVector()
    num_iter_ref = solver.solve(x, b)

    # Change preconditioner matrix (bad matrix) and solve (PC will be
    # updated)
    a_p = u*v*dx
    assemble(a_p, tensor=P)
    bc.apply(P)
    x = PETScVector()
    num_iter_mod = solver.solve(x, b)
    assert num_iter_mod > num_iter_ref

    # Change preconditioner matrix (good matrix) and solve (PC will be
    # updated)
    a_p = a
    assemble(a_p, tensor=P)
    bc.apply(P)
    x = PETScVector()
    num_iter = solver.solve(x, b)
    assert num_iter == num_iter_ref

    # Change preconditioner matrix (bad matrix) and solve (PC will not
    # be updated)
    solver.set_reuse_preconditioner(True)
    a_p = u*v*dx
    assemble(a_p, tensor=P)
    bc.apply(P)
    x = PETScVector()
    num_iter = solver.solve(x, b)
    assert num_iter == num_iter_ref

    # Update preconditioner (bad PC, will increase iteration count)
    solver.set_reuse_preconditioner(False)
    x = PETScVector()
    num_iter = solver.solve(x, b)
    assert num_iter == num_iter_mod
