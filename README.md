# Navier Stoke Solver
### Giorgio Daneri, Jacopo Palumbo, Elia Vaglietti

## Overview

This project is a Navier-Stokes solver designed to simulate fluid dynamics. The solver is implemented using the ```deal.II``` library, which provides extensive tools for finite element analysis. The project includes both stationary and time-dependent solvers for the Navier-Stokes equations.

## Features

- **Stationary Solver**: Solves the steady-state Navier-Stokes equations.
- **Time-Dependent Solver**: Solves the transient Navier-Stokes equations.
- **Mesh Generation**: Supports both internal mesh generation and reading meshes from files.
- **Preconditioners**: Includes various preconditioners like block diagonal, block triangular, and aSIMPLE.
- **Solvers**: Supports multiple solvers including GMRES, FGMRES, and BiCGStab.

## Dependencies

- **deal.II**: A finite element library.
- **MPI**: For parallel computations.
- **CMake**: For building the project.

## Building the Project

1. **Clone the repository**:
    ```sh
    git clone https://github.com/giorgiodaneri/Navier_Stokes_Daneri_Vaglietti.git
    cd navier_stokes_solver
    ```

2. **Create a build directory**:
    ```sh
    module load dealii
    mkdir build
    cd build
    ```

3. **Run CMake**:
    ```sh
    cmake ..
    ```

4. **Build the project**:
    ```sh
    make
    ```

## Running the Solver

### Stationary Solver

To run the stationary solver, use the following command:
```sh
mpirun -n <number_of_processes> ./StationaryNSSolver [options]
```
### Generate mesh from .geo file
We prepared a simple script to generate a mesh from a .geo file. To use it, run the following command:
```sh
cd src/
python3 generate_mesh.py <name_of_geo_file>
```
E.g.
```sh
python3 generate_mesh.py 2dCoarseMesh.geo
```
The script will generate the corresponding .msh file and position it in the mesh folder.
### Options

- `-M, --read-mesh-from-file`: Provide mesh file path to load it instead or generating it inside the program
- `-m, --mesh-size X,Y`: Set mesh size (two integers separated by a comma).
- `-v, --viscosity D` : Set viscosity value (floating point value).
- `-s, --solver N`: Select solver (0: GMRES, 1: FGMRES, 2: BiCGStab).
- `-t, --tolerance D`: Set tolerance (floating point value).
- `-p, --preconditioner N`: Select preconditioner (0: blockDiagonal, 1: blockTriangular, 2: aSIMPLE).
- `-h, --help`: Display help message.

Only for the unsteady version:
- `-T, --time-span and time-step T,D`: Set time span and time step (two floating point values separated by a comma).

Note that by not specifying the -M flag, the solver will use higher order polynomial for the velocity and pressure fields, of degree 3 and 2 respectively. It employs the scalar Lagrange $Q_p$ finite elements on hypercube cells. 
By specifying the -M flag, the solver will use simplex elements, i.e. triangles in 2D, by the means of *FE_SimplexP*. This is because the mesh read from file use a triangulation with simplex elements, while the mesh generated internally uses hypercube elements. 

## Example

```sh
mpirun -n 8 ./StationaryNSSolver -M ../mesh/2dMeshCoarse.msh -v 0.01 -s 1 -t 0.000000001 -p 1
```

This command runs the stationary solver with a mesh size of 300x100, viscosity value of 0.01, using the FGMRES solver, a tolerance of 1e-10, and the blockTriangular preconditioner.

### Running the code on a cluster
If you have access to a cluster without deal.II and all the other libraries installed, you can leverage Singularity to run the code. You can used the latest version of the MK modules in order to create the container (2024 version). The following command allows to build a container from a given URI: 
```sh
singularity pull docker://pcafrica/mk:latest
```
Then launch the container:
```sh
singularity run mk\_{version}.sif
```
which allows us to compile the files as usual. Once inside the container, load the modules:
```sh
source /u/sw/etc/profile \&\& module load gcc-glibc dealii
```
and then compile the files. Then exit from the container and execute the Singularity Image Format:
```sh
singularity -s exec mk\_{version}.sif /bin/bash -c 'source /u/sw/etc/profile \&\& module load gcc-glibc dealii \&\& mpiexec -n 128 {exec\_path} [options]
```
The above command spawns 128 MPI processes to solve the system in parallel thanks to the Trilinos wrappers for MPI. In the scripts folder, we also provide slurm scripts to test both the steady and unsteady versions with configurable parameters. Such scripts produce a csv file containing the execution time and the parameters used for the simulation. We used them to conduct the scalability analysis on the Aion cluster of the University of Luxembourg. 