
#include <deal.II/base/conditional_ostream.h>
#include <deal.II/base/function.h>
#include <deal.II/base/quadrature_lib.h>
#include <deal.II/base/tensor.h>
#include <deal.II/base/utilities.h>

#include <deal.II/distributed/fully_distributed_tria.h>

#include <deal.II/dofs/dof_handler.h>
#include <deal.II/dofs/dof_renumbering.h>
#include <deal.II/dofs/dof_tools.h>

#include <deal.II/fe/fe_q.h>
#include <deal.II/fe/fe_system.h>
#include <deal.II/fe/fe_values.h>
#include <deal.II/fe/fe_simplex_p.h>

#include <deal.II/grid/grid_generator.h>
#include <deal.II/grid/grid_in.h>
#include <deal.II/grid/grid_refinement.h>
#include <deal.II/grid/grid_tools.h>
#include <deal.II/grid/tria.h>

#include <deal.II/lac/affine_constraints.h>
#include <deal.II/lac/block_sparse_matrix.h>
#include <deal.II/lac/block_vector.h>
#include <deal.II/lac/dynamic_sparsity_pattern.h>
#include <deal.II/lac/full_matrix.h>
#include <deal.II/lac/precondition.h>
#include <deal.II/lac/solver_cg.h>
#include <deal.II/lac/solver_gmres.h>
#include <deal.II/lac/sparse_direct.h>
#include <deal.II/lac/sparse_ilu.h>
#include <deal.II/lac/trilinos_block_sparse_matrix.h>
#include <deal.II/lac/trilinos_parallel_block_vector.h>
#include <deal.II/lac/trilinos_precondition.h>
#include <deal.II/lac/trilinos_sparse_matrix.h>

#include <deal.II/numerics/data_out.h>
#include <deal.II/numerics/error_estimator.h>
#include <deal.II/numerics/matrix_tools.h>
#include <deal.II/numerics/solution_transfer.h>
#include <deal.II/numerics/vector_tools.h>

#include <fstream>
#include <iostream>

using namespace dealii;

// Navier-Stokes problem class. Give us 6 points

class NSSolver
{
public:
  // Physical dimension
  static constexpr unsigned int dim = 2;

  class InletVelocity : public Function<dim>
  {
  public:
    InletVelocity()
        : Function<dim>(dim + 1)
    {
    }

    virtual void
    vector_value(const Point<dim> &p, Vector<double> &values) const override
    {
      for (unsigned int i = 0; i < dim + 1; ++i)
        values[i] = 1.0;
    }

    virtual double
    value(const Point<dim> &p, const unsigned int component = 0) const override
    {
      return 1.0;
    }
  };

  // Block-triangular preconditioner.
  class PreconditionBlockTriangular
  {
  public:
    // Initialize the preconditioner, given the velocity stiffness matrix, the
    // pressure mass matrix.
    void
    initialize(const TrilinosWrappers::SparseMatrix &velocity_stiffness_,
               const TrilinosWrappers::SparseMatrix &pressure_mass_,
               const TrilinosWrappers::SparseMatrix &B_)
    {
      velocity_stiffness = &velocity_stiffness_;
      pressure_mass      = &pressure_mass_;
      B                  = &B_;

      preconditioner_velocity.initialize(velocity_stiffness_);
      preconditioner_pressure.initialize(pressure_mass_);
    }

    // Application of the preconditioner.
    void
    vmult(TrilinosWrappers::MPI::BlockVector       &dst,
          const TrilinosWrappers::MPI::BlockVector &src) const
    {
      SolverControl                           solver_control_velocity(100000,
                                            1e-2 * src.block(0).l2_norm());
      SolverCG<TrilinosWrappers::MPI::Vector> solver_cg_velocity(
        solver_control_velocity);
      solver_cg_velocity.solve(*velocity_stiffness,
                               dst.block(0),
                               src.block(0),
                               preconditioner_velocity);

      tmp.reinit(src.block(1));
      B->vmult(tmp, dst.block(0));
      tmp.sadd(-1.0, src.block(1));

      SolverControl                           solver_control_pressure(100000,
                                            1e-2 * src.block(1).l2_norm());
      SolverCG<TrilinosWrappers::MPI::Vector> solver_cg_pressure(
        solver_control_pressure);
      solver_cg_pressure.solve(*pressure_mass,
                               dst.block(1),
                               tmp,
                               preconditioner_pressure);
    }

  protected:
    // Velocity stiffness matrix.
    const TrilinosWrappers::SparseMatrix *velocity_stiffness;

    // Preconditioner used for the velocity block.
    TrilinosWrappers::PreconditionILU preconditioner_velocity;

    // Pressure mass matrix.
    const TrilinosWrappers::SparseMatrix *pressure_mass;

    // Preconditioner used for the pressure block.
    TrilinosWrappers::PreconditionILU preconditioner_pressure;

    // B matrix.
    const TrilinosWrappers::SparseMatrix *B;

    // Temporary vector.
    mutable TrilinosWrappers::MPI::Vector tmp;
  };
  
  NSSolver(const std::string &mesh_path_,
           const unsigned int &degree_velocity_,
           const unsigned int &degree_pressure_,
           const double viscosity_)
      : mesh_path(mesh_path_), degree_velocity(degree_velocity_), degree_pressure(degree_pressure_), mpi_size(Utilities::MPI::n_mpi_processes(MPI_COMM_WORLD)), mpi_rank(Utilities::MPI::this_mpi_process(MPI_COMM_WORLD)), viscosity(viscosity_), gamma(1.0), pcout(std::cout, mpi_rank == 0), mesh(MPI_COMM_WORLD)
  {
  }

  void
  run();

  void
  output();

  void
  setup();

protected:

  void
  setup_mesh();

  void
  setup_finite_element();

  void
  setup_dofs();

  void
  setup_system();

  void
  print_line();

  void
  assemble(const bool, const bool);

  void
  assemble_system(const bool);

  void
  assemble_rhs(const bool);

  void
  solve(const bool);

  void
  apply_dirichlet(TrilinosWrappers::MPI::BlockVector);

  void
  newton_iteration(const double, const unsigned int, const bool, const bool);

  void
  compute_initial_guess(double); 

  // Generic variables
  const std::string mesh_path;
  const unsigned int degree_velocity;
  const unsigned int degree_pressure;
  const unsigned int mpi_size;
  const unsigned int mpi_rank;

  // Viscosity
  double viscosity;

  // Gamma
  const double gamma;

  // POut
  const double p_out = 1.0;

  // Int iter
  int iter = 0;

  // Inlet Velocity
  InletVelocity inlet_velocity;

  // Parallel output stream.
  ConditionalOStream pcout;

  // Mesh
  parallel::fullydistributed::Triangulation<dim> mesh;

  // System matrices
  TrilinosWrappers::BlockSparseMatrix system_matrix;
  TrilinosWrappers::BlockSparseMatrix pressure_mass_matrix;

  // System vectors
  TrilinosWrappers::MPI::BlockVector solution_owned;
  TrilinosWrappers::MPI::BlockVector solution;
  TrilinosWrappers::MPI::BlockVector newton_update;
  TrilinosWrappers::MPI::BlockVector system_rhs;
  TrilinosWrappers::MPI::BlockVector evaluation_point;

  // Finite element space.
  std::unique_ptr<FiniteElement<dim>> fe;

  // DoF handler.
  DoFHandler<dim> dof_handler;

  // DoFs owned by current process.
  IndexSet locally_owned_dofs;

  // DoFs relevant to the current process (including ghost DoFs).
  IndexSet locally_relevant_dofs;

  // DoFs owned by current process in the velocity and pressure blocks.
  std::vector<IndexSet> block_owned_dofs;

  // DoFs relevant to current process in the velocity and pressure blocks.
  std::vector<IndexSet> block_relevant_dofs;

  // Quadrature formula.
  std::unique_ptr<Quadrature<dim>> quadrature;

  // Quadrature formula for face integrals.
  std::unique_ptr<Quadrature<dim - 1>> quadrature_face;

  // To store boundary ids
  std::vector<types::boundary_id>           boundary_ids;
};