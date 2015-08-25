# QSF

QSF is ZeroMQ based multi-thread communicating framework for Lua.


## Installation

Prerequisites:

* [Premake5](http://premake.github.io/download.html) toolchain.

* A C99 compiler, I recommend [Visual Studio 2013 Community](https://www.visualstudio.com/en-us/products/visual-studio-community-vs.aspx) for Windows folks.


### Build on Windows (Windows 7 x64)

1. Run `msvc2013.bat` to generate Visual C++ 2013 solution files.
2. Use Visual Studio 2013 to compile executable binary.

### Build on Linux (Ubuntu 14.04 x64)

1. Type `./build_deps.sh` to install dependency libraries.
2. Type `premake5 gmake && make config=release`


## References

[1] [ZeroMQ The Guide](http://zguide.zeromq.org/page:all)

[2] [Lua 5.3 Manual Reference](http://www.lua.org/manual/5.3/)
