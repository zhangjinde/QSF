# QSF

QSF is ZeroMQ based multi-thread communicating framework for Lua.

## Feature highlights

* Message queue based IPC model.

* Multi-platform(Linux, Windows, OSX) asynchronous networking.

* LuaJIT FFI flexibility for writing C bindings.


## Installation

Prerequisites:

* [Premake5](http://premake.github.io/download.html) toolchain.

* A C99 compiler, I recommend [Visual Studio 2013 Community](https://www.visualstudio.com/en-us/products/visual-studio-community-vs.aspx) for Windows folks.

* Install [LuaJIT](http://luajit.org/download/LuaJIT-2.0.3.tar.gz), [libuv](http://libuv.org/dist/v1.5.0/libuv-v1.5.0.tar.gz), [zeromq](http://download.zeromq.org/zeromq-4.0.5.zip) and [jemalloc](http://www.canonware.com/download/jemalloc/jemalloc-3.6.0.tar.bz2) (for Linux folks)

### Build on Windows (Windows 7 x64)

1. Run `build-msvc2013.bat` to generate Visual C++ 2013 solution files.
2. Use Visual Studio 2013 to compile executable binary.

### Build on Linux (Ubuntu 14.04 x64)

1. Type `./build_deps.sh` to install dependency libraries.
2. Type `./build.sh` to generate makefiles and build executables.


## References

[1] [ZeroMQ The Guide](http://zguide.zeromq.org/page:all)

[2] [Lua 5.1 Manual Reference](http://www.lua.org/manual/5.1/)
