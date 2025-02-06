#!/usr/bin/env python3
import os
import sys

# read the mesh name from the command line
if len(sys.argv) < 2:
    print('Usage: python3 generate_mesh.py <mesh_name>')
    sys.exit(1)
    
mesh_name = sys.argv[1]

# generate the mesh
os.system(f'gmsh -2 -format msh2 ../mesh/{mesh_name}.geo -o ../mesh/{mesh_name}.msh')