# SUbStack ANalysis (SUSAN): High-performance Subtomogram Averaging
`SUSAN` is a sub-tomogram averaging (StA) workflow for CryoET based on sub-stacks of images instead sub-volumes of tomograms. Such an approach substantially lowers computational complexity to speed up StA processing.

## About this repository

This is a fork of the [`SUSAN` original repository](https://github.com/rkms86/SUSAN) which was developed by Ricardo Miguel Sanchez Loyaza in the [Independent Research Group (Sofja Kovaleskaja) of Dr. Misha Kudryashev](https://www.biophys.mpg.de/2149775/members) at the Department of Structural Biology at [MPIBP (Max Planck Institute of Biophysics)](https://www.biophys.mpg.de/en) in Frankfurt (Hesse), Germany.

This fork repository was created for `SUSAN` usage support by members of established in August 2021
[In situ Structural Biology Group of Dr. Misha Kudryashev](https://www.mdc-berlin.de/kudryashev) at the [MDCMM (Max Delbrück Center of Molecular Medicine)](https://www.mdc-berlin.de/) in Berlin, Germany.

## Documentation

`SUSAN` description, documentation on usage & other useful information you may [download here](https://raw.githubusercontent.com/KudryashevLab/SUSAN/main/%2BSUSAN/doc/susan_documentation.pdf).

`SUSAN` is an Open Source project (AGPLv3.0).

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

## Setup of MATLAB-based tutorial

Here we provide up-to-date instructions on how to prepare the data to be able to run the tutorial.

1. Prepare a tutorial folder.
Copy a folder `LOCAL_REPOSITORY_PATH/SUSAN/+SUSAN/tutorial_01` somewhere to store & process tutorial dataset by
```bash
cp -R LOCAL_REPOSITORY_PATH/SUSAN/+SUSAN/tutorial_01 TUTORIAL_PATH
```
where `TUTORIAL_PATH` is a full path to the folder where to store the content of the `tutorial_01` folder.
2. Prepare a tutorial raw data.
Go to `TUTORIAL_PATH/data` and perform the following steps
  1. Download the tutorial data (`wget` should be installed) by running
  ```bash
  ./download_data.sh
  ```
  2. Produce aligned unbinned & binned stacks (you need to [install `IMOD`](https://bio3d.colorado.edu/imod/) for that) by running  
  ```bash
  ./create_binned_aligned_stacks.sh
  ```
  3. Unpack .gz reference file and corresponding mask (`gunzip` should be installed) by running
  ```bash
  gzip -d emd_3420_b4.mrc.gz
  gzip -d mask_sph_b4.mrc.gz
  ```
3. Open `MATLAB` and setup `SUSAN` path.
Perform step No.3 from the above section `MATLAB package setup & compilation`.
4. Enjoy the tutorial!
In `MATLAB` instance, where you activated `SUSAN` on the previous step, go to folder `TUTORIAL_PATH/susan_projects`, where you may find two scripts:
- `workflow_basic.m` - this is a script for the `Tomograms Info and CTF estimation` and `Basic subtomogram averaging on binned data` sections of the tutorial from `SUSAN` documentation;
- `workflow.m` - that is a more recent, refined and extended, but yet undocumented, tutorial from the [`SUSAN` original repository](https://github.com/rkms86/SUSAN), which you are free to try as well!

Thus, we suggest you to start with `workflow_basic.m` to follow tutorial instructions from the `SUSAN` documentation which you may [download here](https://raw.githubusercontent.com/KudryashevLab/SUSAN/main/%2BSUSAN/doc/susan_documentation.pdf).

To visualize results and generate another reference or mask you may [install and use `Dynamo`](https://wiki.dynamo.biozentrum.unibas.ch/w/index.php/Main_Page).
