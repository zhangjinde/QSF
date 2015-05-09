# QSF

QSF is a game server framework based on ZeroMQ and Lua 5.3, still under heavy development.

## Introduction

main features:

* Service-based threading model with message queue.

* Multi-platform(Linux, Windows) asynchronous networking.

* Lua module flexiblity


## Installation

To build the library and test suits:

First obtain [premake5](http://premake.github.io/download.html) toolchain.

### Build on Windows (Windows 7 x64)

1. Run `build-msvc2013.bat` to generate Visual C++ 2013 solution files.
2. Use Visual Studio 2013 to compile executable binary.

### Build on Linux (Ubuntu 14.04 x64)

1. Type `./build_deps.sh` to install dependency libraries.
2. Type `./build.sh` to generate makefiles and build executables.


## References

[1] [ZeroMQ The Guide](http://zguide.zeromq.org/page:all)

[2] [Lua 5.3 Manual Reference](http://www.lua.org/manual/5.3/)
