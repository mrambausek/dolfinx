# Copyright (C) 2017-2019 Chris N. Richardson and Michal Habera
#
# This file is part of DOLFINX (https://www.fenicsproject.org)
#
# SPDX-License-Identifier:    LGPL-3.0-or-later
"""Simple mesh generation module"""

import typing

import numpy

import ufl
from dolfinx import cpp, fem

__all__ = [
    "IntervalMesh", "UnitIntervalMesh", "RectangleMesh", "UnitSquareMesh",
    "BoxMesh", "UnitCubeMesh"
]


def IntervalMesh(comm,
                 nx: int,
                 points: list,
                 ghost_mode=cpp.mesh.GhostMode.none):
    """Create an interval mesh

    Parameters
    ----------
    comm
        MPI communicator
    nx
        Number of cells
    point
        Coordinates of the end points
    ghost_mode

    Note
    ----
    Coordinate mapping is not attached
    """
    element = ufl.VectorElement("Lagrange", "interval", 1, 1)
    domain = ufl.Mesh(element)
    cmap = fem.create_coordinate_map(domain)
    return cpp.generation.IntervalMesh.create(comm, nx, points, cmap, ghost_mode)


def UnitIntervalMesh(comm,
                     nx,
                     ghost_mode=cpp.mesh.GhostMode.none):
    """Create a mesh on the unit interval with coordinate mapping attached

    Parameters
    ----------
    comm
        MPI communicator
    nx
        Number of cells

    """
    mesh = IntervalMesh(comm, nx, [0.0, 1.0], ghost_mode)
    return mesh


def RectangleMesh(comm,
                  points: typing.List[numpy.array],
                  n: list,
                  cell_type=cpp.mesh.CellType.triangle,
                  ghost_mode=cpp.mesh.GhostMode.none,
                  diagonal: str = "right"):
    """Create rectangle mesh

    Parameters
    ----------
    comm
        MPI communicator
    points
        List of `Points` representing vertices
    n
        List of number of cells in each direction
    diagonal
        Direction of diagonal

    Note
    ----
    Coordinate mapping is not attached

    """
    element = ufl.VectorElement("Lagrange", cpp.mesh.to_string(cell_type), 1, 2)
    domain = ufl.Mesh(element)
    cmap = fem.create_coordinate_map(domain)
    return cpp.generation.RectangleMesh.create(comm, points, n, cell_type, cmap,
                                               ghost_mode, diagonal)


def UnitSquareMesh(comm,
                   nx,
                   ny,
                   cell_type=cpp.mesh.CellType.triangle,
                   ghost_mode=cpp.mesh.GhostMode.none,
                   diagonal="right"):
    """Create a mesh of a unit square with coordinate mapping attached

    Parameters
    ----------
    comm
        MPI communicator
    nx
        Number of cells in "x" direction
    ny
        Number of cells in "y" direction
    diagonal
        Direction of diagonal

    """
    mesh = RectangleMesh(
        comm, [numpy.array([0.0, 0.0, 0.0]),
               numpy.array([1.0, 1.0, 0.0])], [nx, ny], cell_type, ghost_mode,
        diagonal)
    return mesh


def BoxMesh(comm,
            points: typing.List[numpy.array],
            n: list,
            cell_type=cpp.mesh.CellType.tetrahedron,
            ghost_mode=cpp.mesh.GhostMode.none):
    """Create box mesh

    Parameters
    ----------
    comm
        MPI communicator
    points
        List of points representing vertices
    n
        List of cells in each direction

    Note
    ----
    Coordinate mapping is not attached
    """
    return cpp.generation.BoxMesh.create(comm, points, n, cell_type,
                                         ghost_mode)


def UnitCubeMesh(comm,
                 nx,
                 ny,
                 nz,
                 cell_type=cpp.mesh.CellType.tetrahedron,
                 ghost_mode=cpp.mesh.GhostMode.none):
    """Create a mesh of a unit cube with coordinate mapping attached

    Parameters
    ----------
    comm
        MPI communicator
    nx
        Number of cells in "x" direction
    ny
        Number of cells in "y" direction
    nz
        Number of cells in "z" direction

    """
    element = ufl.VectorElement("Lagrange", cpp.mesh.to_string(cell_type), 1, 3)
    domain = ufl.Mesh(element)
    cmap = fem.create_coordinate_map(domain)
    mesh = BoxMesh(
        comm, [numpy.array([0.0, 0.0, 0.0]),
               numpy.array([1.0, 1.0, 1.0])], [nx, ny, nz], cell_type, cmap,
        ghost_mode)
    return mesh
