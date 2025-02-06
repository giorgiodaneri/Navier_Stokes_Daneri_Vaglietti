#include "NSSolverStationary.hpp"
#include <getopt.h>
#include <iostream>
#include <cstdlib>

// Print help message
void print_help() {
    std::cout << "Usage: ./NSSolver [options]\n\n"
              << "Options:\n"
              << "  -M, --read-mesh-from-file Provide mesh file path to load it instead or generating it inside the program\n"
              << "  -m, --mesh-size X,Y       Set mesh size (two integers separated by a comma)\n"
              << "  -v, --viscosity D         Set viscosity value (floating point value)\n"
              << "  -s, --solver N            Select solver (valid values: 0: GMRES, 1: FGMRES, 2: Bicgstab)\n"
              << "  -t, --tolerance D         Set tolerance (floating point value)\n"
              << "  -p, --preconditioner N    Select preconditioner (valid values: 0: blockDiagonal, 1: blockTriangular, 2: aSIMPLE)\n"
              << "  -h, --help                Display this help message\n";
}

int main(int argc, char *argv[]) {
    Utilities::MPI::MPI_InitFinalize mpi_init(argc, argv);

    // Default parameters
    bool read_mesh_from_file = false;
    unsigned int degree_velocity = 3;
    unsigned int degree_pressure = 2;
    std::string mesh_path = "";
    double nu = 0.1;
    int mesh_size_x = 100, mesh_size_y = 100;
    int solver_type = 1;
    double tolerance = 1e-6;
    int preconditioner = 0;

    // Define long options
    static struct option long_options[] = {
        {"read-mesh-from-file", required_argument, 0, 'M'},
        {"mesh-size", required_argument, 0, 'm'},
        {"viscosity", required_argument, 0, 'v'},
        {"solver", required_argument, 0, 's'},
        {"tolerance", required_argument, 0, 't'},
        {"preconditioner", required_argument, 0, 'p'},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };

    int opt;
    // Modified getopt_long string to match the required format
    while ((opt = getopt_long(argc, argv, "M:m:v:s:t:p:h", long_options, NULL)) != -1) {
        switch (opt) {
            case 'M':
                read_mesh_from_file = true;
                if (optarg) {
                    mesh_path = optarg;  
                    if(Utilities::MPI::this_mpi_process(MPI_COMM_WORLD) == 0)
                        std::cout << "Mesh file path: " << mesh_path << "\n";
                } else {
                    if(Utilities::MPI::this_mpi_process(MPI_COMM_WORLD) == 0)
                        std::cerr << "Error: No file path provided for -M option.\n";
                    return 1;
                }
                degree_velocity = 2;
                degree_pressure = 1;
                break;
            case 'm': {
                char* comma = strchr(optarg, ',');
                if (comma) {
                    *comma = '\0';
                    mesh_size_x = std::atoi(optarg);
                    mesh_size_y = std::atoi(comma + 1);
                } else {
                    if(Utilities::MPI::this_mpi_process(MPI_COMM_WORLD) == 0)
                        std::cerr << "Error: mesh-size requires two values separated by comma\n";
                    return 1;
                }
                break;
            }
            case 'v':
                nu = std::atof(optarg);
                break;
            case 's':
                solver_type = std::atoi(optarg);
                break;
            case 't':
                tolerance = std::atof(optarg);
                break;
            case 'p':
                preconditioner = std::atoi(optarg);
                break;
            case 'h':
                if(Utilities::MPI::this_mpi_process(MPI_COMM_WORLD) == 0)
                    print_help();
                return 0;
            default:
                if(Utilities::MPI::this_mpi_process(MPI_COMM_WORLD) == 0)
                    print_help();
                return 1;
        }
    }

    // Tolerance validation 
    if (tolerance <= 0) {
        if(Utilities::MPI::this_mpi_process(MPI_COMM_WORLD) == 0)
            std::cerr << "Error: tolerance must be positive\n";
        return 1;
    }

    // Print the parsed values
    if(Utilities::MPI::this_mpi_process(MPI_COMM_WORLD) == 0)
    {
        std::cout << "--------- CONFIGURATION PARAMETERS --------- \n";
        std::cout << "Mesh size: " << mesh_size_x << "x" << mesh_size_y << "\n";
        std::cout << "Viscosity: " << nu << "\n";
        std::cout << "Solver type: ";
        if (solver_type == 0) {
        std::cout << "GMRES\n";
        }
        else if (solver_type == 1) {
        std::cout << "FGMRES\n";
        }
        else if (solver_type == 2) {
        std::cout << "Bicgstab\n";
        }
        std::cout << "Tolerance: " << tolerance << "\n";
        std::cout << "Preconditioner: ";
        if(preconditioner == 0) {
        std::cout << "blockDiagonal\n";
        }
        else if(preconditioner == 1) {
        std::cout << "blockTriangular\n";
        }
        else if(preconditioner == 2) {
        std::cout << "aSIMPLE\n";
        }
        std::cout << "-----------------------------------------------\n";
    }
    
    NSSolverStationary problem(mesh_path, degree_velocity, degree_pressure, mesh_size_x, mesh_size_y, solver_type, tolerance, preconditioner, nu, read_mesh_from_file);

    problem.setup();
    problem.solve_newton();
    problem.output();
    problem.compute_lift_drag();
    problem.print_lift_coeff();
    problem.print_drag_coeff();

  return 0;
}