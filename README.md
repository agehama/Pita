Pita
===
Constraint geometry processing language.

### Requirements
- Boost 1.57

### Build

    git clone --recursive https://github.com/agehama/Pita.git
    mkdir build
    cd build
    cmake ../Pita
    make -j

### Run examples

    ./pita ../Pita/examples/3rects.cgl
    #output result.svg

### Run test

    ctest -V