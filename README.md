# QSF

QSF is a game server framework based on zeromq, still under heavy development.



## Installation

To build the library and test suits:

### Build on Windows (Windows 7 x64)

1. Obtain boost library(http://boost.org) 
2. Set environment variable `BOOST_ROOT` as directory path of boost library
3. Build `boost.system` `($BOOST_ROOT)/stage/lib`
4. Run `build-msvc2013.bat` to generate Visual C++ 2013 solution files


### Build on Linux (Ubuntu 12.04 x64)

1. Type `./build_deps.sh` to install dependency libraries
2. Type `./build.sh` to generate makefiles and build executables

