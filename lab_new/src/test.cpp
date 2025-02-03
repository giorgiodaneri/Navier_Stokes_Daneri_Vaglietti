#include "NSSolver.hpp"

// Main function.
int
main(int argc, char *argv[])
{
  Utilities::MPI::MPI_InitFinalize mpi_init(argc, argv);

  const std::string  mesh_file_name  = "../mesh/2dMeshCylinder.msh";
  const unsigned int degree_velocity = 3;
  const unsigned int degree_pressure = 2;

  NSSolver problem(mesh_file_name, degree_velocity, degree_pressure, 4.0, 0.25);

  problem.setup();
  problem.solve();

  return 0;
}