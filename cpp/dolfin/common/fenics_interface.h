/// This is a FEniCS interface.
/// This code is released into the public domain.
///
/// The FEniCS Project (http://www.fenicsproject.org/) 2006-2019.

#pragma once

#include <dolfin/common/fenics_geometry.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{

#if defined(__clang__)
#define restrict
#elif defined(__GNUC__) || defined(__GNUG__)
#define restrict __restrict__
#else
#define restrict
#endif // restrict
#endif // __cplusplus

  typedef enum
  {
    interval = 10,
    triangle = 20,
    quadrilateral = 30,
    tetrahedron = 40,
    hexahedron = 50,
    vertex = 60,
  } fenics_shape;

  /// Forward declarations
  typedef struct fenics_coordinate_mapping fenics_coordinate_mapping;
  typedef struct fenics_finite_element fenics_finite_element;
  typedef struct fenics_dofmap fenics_dofmap;

  /// Finite element
  typedef struct fenics_finite_element
  {
    /// String identifying the finite element
    const char* signature;

    /// Return the cell shape
    fenics_shape cell_shape;

    /// Return the topological dimension of the cell shape
    int topological_dimension;

    /// Return the geometric dimension of the cell shape
    int geometric_dimension;

    /// Return the dimension of the finite element function space
    int space_dimension;

    /// Return the rank of the value space
    int value_rank;

    /// Return the dimension of the value space for axis i
    int (*value_dimension)(int i);

    /// Return the number of components of the value space
    int value_size;

    /// Return the rank of the reference value space
    int reference_value_rank;

    /// Return the dimension of the reference value space for axis i
    int (*reference_value_dimension)(int i);

    /// Return the number of components of the reference value space
    int reference_value_size;

    /// Return the maximum polynomial degree of the finite element
    /// function space
    int degree;

    /// Return the family of the finite element function space
    const char* family;

    int (*evaluate_reference_basis)(double* restrict reference_values,
                                    int num_points, const double* restrict X);

    int (*evaluate_reference_basis_derivatives)(
        double* restrict reference_values, int order, int num_points,
        const double* restrict X);

    int (*transform_reference_basis_derivatives)(
        double* restrict values, int order, int num_points,
        const double* restrict reference_values, const double* restrict X,
        const double* restrict J, const double* restrict detJ,
        const double* restrict K, int cell_orientation);

    /// Map values of field from physical to reference space which has
    /// been evaluated at points given by
    /// tabulate_reference_dof_coordinates.
    int (*transform_values)(fenics_scalar_t* restrict reference_values,
                            const fenics_scalar_t* restrict physical_values,
                            const double* restrict coordinate_dofs,
                            int cell_orientation,
                            const fenics_coordinate_mapping* cm);

    // FIXME: change to 'const double* reference_dof_coordinates()'
    /// Tabulate the coordinates of all dofs on a reference cell
    int (*tabulate_reference_dof_coordinates)(
        double* restrict reference_dof_coordinates);

    /// Return the number of sub elements (for a mixed element)
    int num_sub_elements;

    /// Create a new finite element for sub element i (for a mixed
    /// element)
    fenics_finite_element* (*create_sub_element)(int i);

    /// Create a new class instance
    fenics_finite_element* (*create)(void);
  } fenics_finite_element;

  /// Degrees-of-freedom mapping
  typedef struct fenics_dofmap
  {

    /// Return a string identifying the dofmap
    const char* signature;

    /// Number of dofs with global support (i.e. global constants)
    int num_global_support_dofs;

    /// Dimension of the local finite element function space
    /// for a cell (not including global support dofs)
    int num_element_support_dofs;

    /// Number of dofs associated with each cell entity of
    /// dimension d
    int num_entity_dofs[4];

    /// Tabulate the local-to-local mapping of dofs on entity (d, i)
    void (*tabulate_entity_dofs)(int* restrict dofs, int d, int i);

    /// Return the number of sub dofmaps (for a mixed element)
    int num_sub_dofmaps;

    /// Create a new dofmap for sub dofmap i (for a mixed element)
    fenics_dofmap* (*create_sub_dofmap)(int i);

    /// Create a new class instance
    fenics_dofmap* (*create)(void);
  } fenics_dofmap;

  /// A representation of a coordinate mapping parameterized by a local
  /// finite element basis on each cell
  typedef struct fenics_coordinate_mapping
  {

    /// Return coordinate_mapping signature string
    const char* signature;

    /// Create object of the same type
    fenics_coordinate_mapping* (*create)(void);

    /// Return geometric dimension of the coordinate_mapping
    int geometric_dimension;

    /// Return topological dimension of the coordinate_mapping
    int topological_dimension;

    /// Return cell shape of the coordinate_mapping
    fenics_shape cell_shape;

    // FIXME: Remove and just use 'create'?
    // FIXME: Is this for a single coordinate component, or a vector?
    /// Create finite_element object representing the coordinate
    /// parameterization
    fenics_finite_element* (*create_coordinate_finite_element)(void);

    // FIXME: Remove and just use 'create'?
    // FIXME: Is this for a single coordinate component, or a vector?
    /// Create dofmap object representing the coordinate parameterization
    fenics_dofmap* (*create_coordinate_dofmap)(void);

    /// Compute physical coordinates x from reference coordinates X,
    /// the inverse of compute_reference_coordinates
    ///
    /// @param[out] x
    ///         Physical coordinates.
    ///         Dimensions: x[num_points][gdim]
    /// @param[in] num_points
    ///         Number of points.
    /// @param[in] X
    ///         Reference cell coordinates.
    ///         Dimensions: X[num_points][tdim]
    /// @param[in] coordinate_dofs
    ///         Dofs of the coordinate field on the cell.
    ///         Dimensions: coordinate_dofs[num_dofs][gdim].
    ///
    void (*compute_physical_coordinates)(
        double* restrict x, int num_points, const double* restrict X,
        const double* restrict coordinate_dofs);

    /// Compute reference coordinates X from physical coordinates x,
    /// the inverse of compute_physical_coordinates
    ///
    /// @param[out] X
    ///         Reference cell coordinates.
    ///         Dimensions: X[num_points][tdim]
    /// @param[in] num_points
    ///         Number of points.
    /// @param[in] x
    ///         Physical coordinates.
    ///         Dimensions: x[num_points][gdim]
    /// @param[in] coordinate_dofs
    ///         Dofs of the coordinate field on the cell.
    ///         Dimensions: coordinate_dofs[num_dofs][gdim].
    /// @param[in] cell_orientation
    ///         Orientation of the cell, 1 means flipped w.r.t. reference cell.
    ///         Only relevant on manifolds (tdim < gdim).
    ///
    void (*compute_reference_coordinates)(
        double* restrict X, int num_points, const double* restrict x,
        const double* restrict coordinate_dofs, int cell_orientation);

    /// Compute X, J, detJ, K from physical coordinates x on a cell
    ///
    /// @param[out] X
    ///         Reference cell coordinates.
    ///         Dimensions: X[num_points][tdim]
    /// @param[out] J
    ///         Jacobian of coordinate field, J = dx/dX.
    ///         Dimensions: J[num_points][gdim][tdim]
    /// @param[out] detJ
    ///         (Pseudo-)Determinant of Jacobian.
    ///         Dimensions: detJ[num_points]
    /// @param[out] K
    ///         (Pseudo-)Inverse of Jacobian of coordinate field.
    ///         Dimensions: K[num_points][tdim][gdim]
    /// @param[in] num_points
    ///         Number of points.
    /// @param[in] x
    ///         Physical coordinates.
    ///         Dimensions: x[num_points][gdim]
    /// @param[in] coordinate_dofs
    ///         Dofs of the coordinate field on the cell.
    ///         Dimensions: coordinate_dofs[num_dofs][gdim].
    /// @param[in] cell_orientation
    ///         Orientation of the cell, 1 means flipped w.r.t. reference cell.
    ///         Only relevant on manifolds (tdim < gdim).
    ///
    void (*compute_reference_geometry)(double* restrict X, double* restrict J,
                                       double* restrict detJ,
                                       double* restrict K, int num_points,
                                       const double* restrict x,
                                       const double* restrict coordinate_dofs,
                                       int cell_orientation);

    /// Compute Jacobian of coordinate mapping J = dx/dX at reference
    /// coordinates
    /// X
    ///
    /// @param[out] J
    ///         Jacobian of coordinate field, J = dx/dX.
    ///         Dimensions: J[num_points][gdim][tdim]
    /// @param[in] num_points
    ///         Number of points.
    /// @param[in] X
    ///         Reference cell coordinates.
    ///         Dimensions: X[num_points][tdim]
    /// @param[in] coordinate_dofs
    ///         Dofs of the coordinate field on the cell.
    ///         Dimensions: coordinate_dofs[num_dofs][gdim].
    ///
    void (*compute_jacobians)(double* restrict J, int num_points,
                              const double* restrict X,
                              const double* restrict coordinate_dofs);

    /// Compute determinants of (pseudo-)Jacobians J
    ///
    /// @param[out] detJ
    ///         (Pseudo-)Determinant of Jacobian.
    ///         Dimensions: detJ[num_points]
    /// @param[in] num_points
    ///         Number of points.
    /// @param[in] J
    ///         Jacobian of coordinate field, J = dx/dX.
    ///         Dimensions: J[num_points][gdim][tdim]
    /// @param[in] cell_orientation
    ///         Orientation of the cell, 1 means flipped w.r.t. reference cell.
    ///         Only relevant on manifolds (tdim < gdim).
    ///
    void (*compute_jacobian_determinants)(double* restrict detJ, int num_points,
                                          const double* restrict J,
                                          int cell_orientation);

    /// Compute (pseudo-)inverses K of (pseudo-)Jacobians J
    ///
    /// @param[out] K
    ///         (Pseudo-)Inverse of Jacobian of coordinate field.
    ///         Dimensions: K[num_points][tdim][gdim]
    /// @param[in] num_points
    ///         Number of points.
    /// @param[in] J
    ///         Jacobian of coordinate field, J = dx/dX.
    ///         Dimensions: J[num_points][gdim][tdim]
    /// @param[in] detJ
    ///         (Pseudo-)Determinant of Jacobian.
    ///         Dimensions: detJ[num_points]
    ///
    void (*compute_jacobian_inverses)(double* restrict K, int num_points,
                                      const double* restrict J,
                                      const double* restrict detJ);

    // FIXME: Remove? FFC implementation just calls other generated functions
    /// Combined (for convenience) computation of x, J, detJ, K from X and
    /// coordinate_dofs on a cell
    ///
    /// @param[out] x
    ///         Physical coordinates.
    ///         Dimensions: x[num_points][gdim]
    /// @param[out] J
    ///         Jacobian of coordinate field, J = dx/dX.
    ///         Dimensions: J[num_points][gdim][tdim]
    /// @param[out] detJ
    ///         (Pseudo-)Determinant of Jacobian.
    ///         Dimensions: detJ[num_points]
    /// @param[out] K
    ///         (Pseudo-)Inverse of Jacobian of coordinate field.
    ///         Dimensions: K[num_points][tdim][gdim]
    /// @param[in] num_points
    ///         Number of points.
    /// @param[in] X
    ///         Reference cell coordinates.
    ///         Dimensions: X[num_points][tdim]
    /// @param[in] coordinate_dofs
    ///         Dofs of the coordinate field on the cell.
    ///         Dimensions: coordinate_dofs[num_dofs][gdim].
    /// @param[in] cell_orientation
    ///         Orientation of the cell, 1 means flipped w.r.t. reference cell.
    ///         Only relevant on manifolds (tdim < gdim).
    ///
    void (*compute_geometry)(double* restrict x, double* restrict J,
                             double* restrict detJ, double* restrict K,
                             int num_points, const double* restrict X,
                             const double* restrict coordinate_dofs,
                             int cell_orientation);

    /// Compute x and J at midpoint of cell
    ///
    /// @param[out] x
    ///         Physical coordinates.
    ///         Dimensions: x[gdim]
    /// @param[out] J
    ///         Jacobian of coordinate field, J = dx/dX.
    ///         Dimensions: J[gdim][tdim]
    /// @param[in] coordinate_dofs
    ///         Dofs of the coordinate field on the cell.
    ///         Dimensions: coordinate_dofs[num_dofs][gdim].
    ///
    void (*compute_midpoint_geometry)(double* restrict x, double* restrict J,
                                      const double* restrict coordinate_dofs);

  } fenics_coordinate_mapping;

  /// Tabulate integral into tensor A with compiled quadrature rule
  ///
  /// @param[out] A
  /// @param[in] w
  ///         Coefficients attached to the form to which the tabulated integral
  ///         belongs.
  ///         Dimensions: w[coefficient][restriction][dof].
  ///         Restriction dimension applies to interior facet integrals, where
  ///         coefficients restricted to both cells sharing the facet must be provided.
  /// @param[in] c
  ///         Constants attached to the form to which the tabulated integral
  ///         belongs.
  ///         Dimensions: c[constant][dim].
  /// @param[in] coordinate_dofs
  ///         Values of degrees of freedom of coordinate element.
  ///         Defines the geometry of the cell.
  ///         Dimensions: coordinate_dofs[restriction][num_dofs][gdim].
  ///         Restriction dimension applies to interior facet integrals, where
  ///         cell geometries for both cells sharing the facet must be provided.
  /// @param[in] entity_local_index
  ///         Local index of mesh entity on which to tabulate.
  ///         This applies to facet integrals.
  /// @param[in] cell_orientation
  ///         Sign of orientation of the cell with respect to the consistent
  ///         orientation of the whole mesh.
  ///         0 means "up"
  ///         1 means "down" and scales det(J) with -1.0
  ///         Applies to the case of k-dimensional surface in n-dimensional space, where k < n.
  ///
  typedef void (fenics_tabulate_tensor)(fenics_scalar_t* restrict A,
      const fenics_scalar_t* w,
      const fenics_scalar_t* c,
      const double* restrict coordinate_dofs,
      const int* entity_local_index,
      const int* cell_orientation);

  /// Tabulate integral into tensor A with runtime quadrature rule
  ///
  /// @see fenics_tabulate_tensor
  ///
  typedef void (fenics_tabulate_tensor_custom)(fenics_scalar_t* restrict A,
      const fenics_scalar_t* w,
      const fenics_scalar_t* c,
      const double* restrict coordinate_dofs,
      int num_quadrature_points,
      const double* restrict quadrature_points,
      const double* restrict quadrature_weights,
      const double* restrict facet_normals,
      int cell_orientation);

  /// Integral
  typedef struct fenics_integral
  {
    /// True if coefficient is used in this integral
    const bool* enabled_coefficients;

    /// Tabulation kernel
    fenics_tabulate_tensor* tabulate_tensor;

  } fenics_integral;

  /// Custom (runtime quadrature) integral
  typedef struct fenics_custom_integral
  {
    /// True is coefficient is used in this integral
    const bool* enabled_coefficients;

    /// Tabulation kernel
    fenics_tabulate_tensor_custom* tabulate_tensor;

  } fenics_custom_integral;

  /// This class defines the interface for the assembly of the global
  /// tensor corresponding to a form with r + n arguments, that is, a
  /// mapping
  ///
  ///     a : V1 x V2 x ... Vr x W1 x W2 x ... x Wn -> R
  ///
  /// with arguments v1, v2, ..., vr, w1, w2, ..., wn. The rank r
  /// global tensor A is defined by
  ///
  ///     A = a(V1, V2, ..., Vr, w1, w2, ..., wn),
  ///
  /// where each argument Vj represents the application to the
  /// sequence of basis functions of Vj and w1, w2, ..., wn are given
  /// fixed functions (coefficients).
  typedef struct fenics_form
  {
    /// String identifying the form
    const char* signature;

    /// Rank of the global tensor (r)
    int rank;

    /// Number of coefficients (n)
    int num_coefficients;

    /// Number of constants
    int num_constants;

    /// Return original coefficient position for each coefficient
    ///
    /// @param i
    ///        Coefficient number, 0 <= i < n
    ///
    int (*original_coefficient_position)(int i);

    /// Return list of names of coefficients
    const char** (*coefficient_name_map)(void);

    /// Return list of names of constants
    const char** (*constant_name_map)(void);

    // FIXME: Remove and just use 'create_coordinate_mapping'
    /// Create a new finite element for parameterization of coordinates
    fenics_finite_element* (*create_coordinate_finite_element)(void);

    // FIXME: Remove and just use 'create_coordinate_mapping'
    /// Create a new dofmap for parameterization of coordinates
    fenics_dofmap* (*create_coordinate_dofmap)(void);

    /// Create a new coordinate mapping
    fenics_coordinate_mapping* (*create_coordinate_mapping)(void);

    /// Create a new finite element for argument function 0 <= i < r+n
    ///
    /// @param i
    ///        Argument number if 0 <= i < r
    ///        Coefficient number j=i-r if r+j <= i < r+n
    ///
    fenics_finite_element* (*create_finite_element)(int i);

    /// Create a new dofmap for argument function 0 <= i < r+n
    ///
    /// @param i
    ///        Argument number if 0 <= i < r
    ///        Coefficient number j=i-r if r+j <= i < r+n
    ///
    fenics_dofmap* (*create_dofmap)(int i);

    /// All ids for cell integrals
    void (*get_cell_integral_ids)(int* ids);

    /// All ids for exterior facet integrals
    void (*get_exterior_facet_integral_ids)(int* ids);

    /// All ids for interior facet integrals
    void (*get_interior_facet_integral_ids)(int* ids);

    /// All ids for vertex integrals
    void (*get_vertex_integral_ids)(int* ids);

    /// All ids for custom integrals
    void (*get_custom_integral_ids)(int* ids);

    /// Number of cell integrals
    int num_cell_integrals;

    /// Number of exterior facet integrals
    int num_exterior_facet_integrals;

    /// Number of interior facet integrals
    int num_interior_facet_integrals;

    /// Number of vertex integrals
    int num_vertex_integrals;

    /// Number of custom integrals
    int num_custom_integrals;

    /// Create a new cell integral on sub domain subdomain_id
    fenics_integral* (*create_cell_integral)(int subdomain_id);

    /// Create a new exterior facet integral on sub domain subdomain_id
    fenics_integral* (*create_exterior_facet_integral)(int subdomain_id);

    /// Create a new interior facet integral on sub domain subdomain_id
    fenics_integral* (*create_interior_facet_integral)(int subdomain_id);

    /// Create a new vertex integral on sub domain subdomain_id
    fenics_integral* (*create_vertex_integral)(int subdomain_id);

    /// Create a new custom integral on sub domain subdomain_id
    fenics_custom_integral* (*create_custom_integral)(int subdomain_id);

  } fenics_form;

  // FIXME: Formalise a fenics 'function space'.
  typedef struct fenics_function_space
  {
    // Pointer to factory function that creates a new fenics_finite_element
    fenics_finite_element* (*create_element)(void);

    // Pointer to factory function that creates a new fenics_dofmap
    fenics_dofmap* (*create_dofmap)(void);

    // Pointer to factory function that creates a new fenics_coordinate_mapping
    fenics_coordinate_mapping* (*create_coordinate_mapping)(void);
  } fenics_function_space;

#ifdef __cplusplus
#undef restrict
}
#endif
