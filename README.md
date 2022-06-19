# SUbStack ANalysis (SUSAN): High-performance Subtomogram Averaging
`SUSAN` is a sub-tomogram averaging (StA) workflow for CryoET based on sub-stacks of images instead sub-volumes of tomograms. Such an approach substantially lowers computational complexity to speed up StA processing.

## About this repository

This is a fork of the original `SUSAN` repository (https://github.com/rkms86/SUSAN) which was developed by Ricardo Miguel Sanchez Loyaza in the [Independent Research Group (Sofja Kovaleskaja) of Dr. Misha Kudryashev](https://www.biophys.mpg.de/2149775/members) at the [MPIBP (Max Planck Institute of Biophysics)](https://www.biophys.mpg.de/en) in Frankfurt, Hesse.

## Documentation

`SUSAN` description, documentation on usage & other useful information you may [download here](https://raw.githubusercontent.com/KudryashevLab/SUSAN/main/%2BSUSAN/doc/susan_documentation.pdf).

`SUSAN` is an Open Source project (GPLv3.0).

## Build instructions

Here we provide build instructions for quick `SUSAN` start-up.

### Dependencies
- `CUDA` libraries (https://docs.nvidia.com/cuda/cuda-installation-guide-linux/index.html#package-manager-installation)
- `Eigen` library (https://gitlab.com/libeigen/eigen.git)
- `gcc-9.x` and upper
(with C++14/C++17 support for `Eigen` library features; to build gcc-9.x you may use instructions provided here: https://github.com/darrenjs/howto)
- `cmake-3.22.x` (https://cmake.org/download/)

Optional:
- `OpenMPI` libraries (https://www.open-mpi.org/)

### Initial setup & compilation
1. Install listed above dependencies.
2. Clone `SUSAN` from this repository locally to some location (let's name it `LOCAL_REPOSITORY_PATH`)
```bash
cd LOCAL_REPOSITORY_PATH
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

### MATLAB package setup & compilation
This part assumes that you already performed all the steps above in from `Initial setup & compilation` section & was able to successfully compile `SUSAN`.

1. To setup `MATLAB` package of `SUSAN` you need to install `MATLAB` and make sure, that you setted up all pathes to your `MATLAB` installation correctly and may source `mex` compiler.

2. To be able to use `SUSAN` from `MATLAB` just run the following lines:
```bash
cd LOCAL_REPOSITORY_PATH/SUSAN/+SUSAN
make
```
assuming that `LOCAL_REPOSITORY_PATH` is the path to the locally cloned repository location.

3. To use `SUSAN` in `MATLAB` we have to add the location of the package to its working path. So, run `MATLAB` instance and execute in `MATLAB` command line the following:
```
>> addpath LOCAL_REPOSITORY_PATH/SUSAN/
```
To verify the correct installation of the `SUSAN` package in the
current `MATLAB` instance, check if the documentation can be accessed:
```
>> help SUSAN
```

## Tutorial

Here we provide up-to-date tutorial instructions to teach you how to use `SUSAN`.

This section will be updated soon...
