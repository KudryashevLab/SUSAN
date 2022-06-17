# SUbStack ANalysis (SUSAN): High-performance Subtomogram Averaging
`SUSAN` is a sub-tomogram averaging workflow for CryoET based on sub-stacks of images instead sub-volumes of tomograms.

## Documentation

`SUSAN` description, documentation on usage & other useful information you may [download here](https://raw.githubusercontent.com/KudryashevLab/SUSAN/main/%2BSUSAN/doc/susan_documentation.pdf).

`SUSAN` is an Open Source project (GPLv3.0).

## Dependencies
- `CUDA` libraries (https://docs.nvidia.com/cuda/cuda-installation-guide-linux/index.html#package-manager-installation)
- `Eigen` library (https://gitlab.com/libeigen/eigen.git)
- `gcc-9.x` and upper
(with C++14/C++17 support for `Eigen` library features; to build gcc-9.x you may use instructions provided here: https://github.com/darrenjs/howto)
- `cmake-3.22.x` (https://cmake.org/download/)

Optional:
- `OpenMPI` libraries (https://www.open-mpi.org/)

## Setup & Build
1. Install listed above dependencies.
2. Clone `SUSAN` from this repository locally
```bash
cd /path/to/folder/where/to/clone
git clone https://github.com/KudryashevLab/SUSAN
```
3. Clone `Eigen` to the `SUSAN/dependencies`
```bash
cd SUSAN
mkdir dependencies
cd dependencies
git clone https://gitlab.com/libeigen/eigen.git eigen
```
4. Compile `SUSAN`
```bash
cd ../SUSAN/+SUSAN
mkdir bin
cd bin
cmake ..
make -j
```
5. Enjoy!
