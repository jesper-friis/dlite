dlite -- lightweight library for working with scientific data
=============================================================
*dlite* is a small cross-platform C library under development, for
working with and sharing scientific data in an interoperable way.  It
is strongly inspired by [SOFT][1], with the aim to be a lightweight
replacement in cases where Windows portability is a showstopper for
using SOFT.

*dlite* shares the [metadata model of SOFT][2] and generic data stored
with SOFT can be read with dlite and vice verse.  However, apart from
*dlite* being much less complete, there are also some differences.
See [doc/concepts.md](doc/concepts.md) for details.

The main components of *dlite* are:
  - [dlite-storage](src/dlite-storage.h): a generic handle
    encapsulating actual storage formats.  So far only reading and
    writing HDF5 files is implemented.
  - [dlite-entity](src/dlite-entity.h): API for instances (actual data) and
    entities (description of data).

*dlite* also includes [uuid][3] (a small library for generating UUIDs)
as a git submodule.


Dependencies
------------
*dlite* has the following dependencies:
  - [cmake][4], required for building
  - [hdf5][5], required (cmake will automatically download and built hdf5
    if it is not found)
  - [jansson][6], required, provides json
  - [doxygen][7], optional, used for documentation generation
  - [valgrind][8], optional, used for memory checking (Linux only)


Download
--------
Download dlite with git, using

    git clone ssh://git@git.code.sintef.no/sidase/dlite.git

To initialize the uuid submodule, you may also have to run

    git submodule update --init


Building
--------

## Windows with Visual Studio

Create a "build" subfolder and run cmake:

    mkdir build
    cd build
    cmake ..

It is possible that you have to run cmake as
`cmake  -G "Visual Studio 11 2012 Win64" ..`.

Run or double-click on `dlite.sln`, which will open Visual Studio.

## Windows with git bash

Install `wget` on Windows, needed to download hdf5 library from git bash (from the command line):

- Download wget from the URL: https://eternallybored.org/misc/wget/
- Download latest version and copy wget.exe in the git folder:

   - C:\Program Files (x86)\Git\bin (for wget in 32-bit version)
   - C:\Program Files\Git\bin (for wget in 64-bit version)

Launch git bash (from the windows menu search "git bash"), and change your directory to your working directory:

```sh
# goto your working directory
cd /C/Users/tco/Documents/Programs/PRECIMS/
# create a folder to install the libraries
mkdir local
```

Download, build, and install HDF5 library:

```sh
cd /C/Users/tco/Documents/Programs/PRECIMS/
wget https://support.hdfgroup.org/ftp/HDF5/current/src/hdf5-1.10.1.tar.gz
tar -xzvf hdf5-1.10.1.tar.gz
cd hdf5-1.10.1
mkdir build && cd build
cmake -DCMAKE_INSTALL_PREFIX:PATH=/C/Users/tco/Documents/Programs/PRECIMS/local/ ..
cmake --build . --config Debug --target install
cmake --build . --config Release --target install
```

Download, build, and install jansson library (json file reader/writer):

```sh
cd /C/Users/tco/Documents/Programs/PRECIMS/
git clone https://github.com/akheron/jansson.git
cd jansson
mkdir build && cd build
cmake -DCMAKE_INSTALL_PREFIX:PATH=/C/Users/tco/Documents/Programs/PRECIMS/local/ ..
cmake --build . --config Debug --target install
cmake --build . --config Release --target install
```

Download and build dlite:

```sh
cd /C/Users/tco/Documents/Programs/PRECIMS/
git clone ssh://git@git.code.sintef.no/sidase/dlite.git
git submodule update --init
cd dlite
mkdir build && cd build
cmake -DHDF5_ROOT:PATH=/C/Users/tco/Documents/Programs/PRECIMS/local/ ..
cmake --build . --config Debug --target install
cmake --build . --config Release --target install
```


## Linux

Build with:

    mkdir build
    cd build
    cmake ..
    make

Before running make, you may wish to configure some options with
`ccmake ..`

To run the tests, do

    make test        # same as running `ctest`
    make memcheck    # runs all tests with memory checking (requires valgrind)

To generate code documentation, do

    make doc         # direct your browser to doc/html/index.html

To install dlite locally, do

    make install


The future of dlite
-------------------
Ideally dlite will be merged into SOFT when SOFT compiles well on Windows.
Until then, it will remain as a simple and mostly compatible alternative.


---

*dlite* is developed with the hope that it will be a delight to work with.

[1]: https://stash.code.sintef.no/projects/SOFT/repos/soft5/
[2]: https://github.com/NanoSim/Porto/blob/porto/Preview-Final-Release/doc/manual/02_soft_introduction.md#soft5-features
[3]: https://stash.code.sintef.no/projects/sidase/repos/uuid/
[4]: https://cmake.org/
[5]: https://support.hdfgroup.org/HDF5/
[6]: http://www.digip.org/jansson/
[7]: http://www.doxygen.org/
[8]: http://valgrind.org/
[9]: https://github.com/petervaro/sodyll
