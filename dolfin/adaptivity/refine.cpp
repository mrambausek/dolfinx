// Copyright (C) 2010 Garth N. Wells.
// Licensed under the GNU LGPL Version 2.1.
//
// Modified by Anders Logg, 2010-2011.
//
// First added:  2010-02-10
// Last changed: 2011-01-29

#include <boost/shared_ptr.hpp>

#include <dolfin/common/NoDeleter.h>
#include <dolfin/mesh/LocalMeshRefinement.h>
#include <dolfin/mesh/Mesh.h>
#include <dolfin/mesh/MeshEntity.h>
#include <dolfin/mesh/MeshFunction.h>
#include <dolfin/mesh/UniformMeshRefinement.h>
#include <dolfin/function/FunctionSpace.h>
#include <dolfin/function/Function.h>
#include <dolfin/fem/FiniteElement.h>
#include <dolfin/fem/DofMap.h>
#include <dolfin/fem/Form.h>
#include "refine.h"

using namespace dolfin;

//-----------------------------------------------------------------------------
dolfin::Mesh dolfin::refine(const Mesh& mesh)
{
  Mesh refined_mesh;
  refine(refined_mesh, mesh);
  return refined_mesh;
}
//-----------------------------------------------------------------------------
void dolfin::refine(Mesh& refined_mesh, const Mesh& mesh)
{
  UniformMeshRefinement::refine(refined_mesh, mesh);
}
//-----------------------------------------------------------------------------
dolfin::Mesh dolfin::refine(const Mesh& mesh,
                            const MeshFunction<bool>& cell_markers)
{
  Mesh refined_mesh;
  refine(refined_mesh, mesh, cell_markers);
  return refined_mesh;
}
//-----------------------------------------------------------------------------
void dolfin::refine(Mesh& refined_mesh,
                    const Mesh& mesh,
                    const MeshFunction<bool>& cell_markers)
{
  // Count the number of marked cells
  uint n0 = mesh.num_cells();
  uint n = 0;
  for (uint i = 0; i < cell_markers.size(); i++)
    if (cell_markers[i])
      n++;
  info("%d cells out of %d marked for refinement (%.1f%%).",
       n, n0, 100.0 * static_cast<double>(n) / static_cast<double>(n0));

  // Call refinement algorithm
  LocalMeshRefinement::refineRecursivelyByEdgeBisection(refined_mesh,
                                                        mesh,
                                                        cell_markers);

  // Report the number of refined cells
  uint n1 = refined_mesh.num_cells();
  info("Number of cells increased from %d to %d (%.1f%% increase).",
       n0, n1, 100.0 * (static_cast<double>(n1) / static_cast<double>(n0) - 1.0));
}
//-----------------------------------------------------------------------------
dolfin::FunctionSpace dolfin::refine(const FunctionSpace& space)
{
  // Refine mesh
  const Mesh& mesh = space.mesh();
  boost::shared_ptr<Mesh> refined_mesh(new Mesh());
  refine(*refined_mesh, mesh);

  // Refine space
  FunctionSpace refined_space = refine(space, *refined_mesh);

  return refined_space;
}
//-----------------------------------------------------------------------------
dolfin::FunctionSpace dolfin::refine(const FunctionSpace& space,
                                     const MeshFunction<bool>& cell_markers)
{
  // Refine mesh
  const Mesh& mesh = space.mesh();
  boost::shared_ptr<Mesh> refined_mesh(new Mesh());
  refine(*refined_mesh, mesh, cell_markers);

  // Refine space
  FunctionSpace refined_space = refine(space, *refined_mesh);

  return refined_space;
}
//-----------------------------------------------------------------------------
dolfin::FunctionSpace dolfin::refine(const FunctionSpace& space,
                                     const Mesh& refined_mesh)
{
#ifndef UFC_DEV
  info("UFC_DEV compiler flag is not set.");
  error("Refinement of function spaces relies on the development version of UFC.");
  return space;
#else

  // Get DofMap (GenericDofMap does not know about ufc::dof_map)
  const DofMap* dofmap = dynamic_cast<const DofMap*>(&space.dofmap());
  if (!dofmap)
  {
    info("FunctionSpace is defined by a non-stand dofmap.");
    error("Unable to refine function space.");
  }

  // Create new copies of UFC finite element and dofmap
  boost::shared_ptr<ufc::finite_element> ufc_element(space.element().ufc_element()->create());
  boost::shared_ptr<ufc::dof_map> ufc_dofmap(dofmap->ufc_dofmap()->create());

  // Create DOLFIN finite element and dofmap
  boost::shared_ptr<FiniteElement> refined_element(new FiniteElement(ufc_element));
  boost::shared_ptr<DofMap> refined_dofmap(new DofMap(ufc_dofmap, refined_mesh));

  // Create new function space
  FunctionSpace refined_space(reference_to_no_delete_pointer(refined_mesh),
                  refined_element,
                  refined_dofmap);

  return refined_space;

#endif
}
//-----------------------------------------------------------------------------
dolfin::Function dolfin::refine(const Function& function,
                                const FunctionSpace& refined_space)
{
  // Create function
  Function refined_function(refined_space);

  // Interpolate function on coarse mesh
  refined_function.interpolate(function);

   return refined_function;
 }
 //-----------------------------------------------------------------------------
 dolfin::Form dolfin::refine(const Form& form,
                             const Mesh& refined_mesh)
 {
   cout << "Refining form" << endl;

   // Get form data
   std::vector<boost::shared_ptr<const FunctionSpace> > spaces = form.function_spaces();
   std::vector<const GenericFunction*> coefficients = form.coefficients();
   boost::shared_ptr<const ufc::form> ufc_form = form.ufc_form_shared_ptr();

   // Refine function spaces and keep track of function spaces that may
   // appear multiple times in the definition of a form.
   typedef std::map<boost::shared_ptr<const FunctionSpace>, boost::shared_ptr<FunctionSpace> > space_map_type;
   space_map_type space_map;
   std::vector<boost::shared_ptr<const FunctionSpace> > refined_spaces;
   for (uint i = 0; i < spaces.size(); i++)
   {
     cout << "Checking function space " << i << endl;
     boost::shared_ptr<const FunctionSpace> space = spaces[i];
     space_map_type::iterator it = space_map.find(space);
     if (it == space_map.end())
     {
       cout << "Function space not seen before, refining" << endl;
       boost::shared_ptr<FunctionSpace> refined_space(new FunctionSpace(refine(*space, refined_mesh)));
       space_map[space] = refined_space;
       refined_spaces.push_back(refined_space);
     }
     else
     {
       cout << "Seen before, reusing" << endl;
       refined_spaces.push_back(it->second);
     }
   }



   Form refined_form(2, 0);
   return refined_form;


  /*
    /// Create form (constructor used from Python interface)
    Form(const ufc::form& ufc_form,
         const std::vector<const FunctionSpace*>& function_spaces,;
         const std::vector<const GenericFunction*>& coefficients);





    // The mesh (needed for functionals when we don't have any spaces)
    boost::shared_ptr<const Mesh> _mesh;

    // Function spaces (one for each argument)
    std::vector<boost::shared_ptr<const FunctionSpace> > _function_spaces;

    // Coefficients
    std::vector<boost::shared_ptr<const GenericFunction> > _coefficients;

    // The UFC form
    boost::shared_ptr<const ufc::form> _ufc_form;

  */
}
//-----------------------------------------------------------------------------
