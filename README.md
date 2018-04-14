Pita
===

[![Build Status](https://travis-ci.org/agehama/Pita.svg?branch=master)](https://travis-ci.org/agehama/Pita)

Constraint geometry processing language.

### Requirements
- Boost 1.57
- CMake 3.1
- C++ compiler that supports C++14

### Build

    git clone --recursive https://github.com/agehama/Pita.git
    mkdir build
    cd build
    cmake ../Pita
    make -j

### Run examples

    ./pita ../Pita/examples/3rects.cgl -o 3rects.svg

### Run test

    ctest -V
