# QSF

QSF is ZeroMQ based multi-thread communicating framework for Lua.


## Installation

Prerequisites:

* [Premake5](http://premake.github.io/download.html) toolchain.

* A C99 compiler

* Install [lua5.3](http://www.lua.org/download.html), [libuv]( http://dist.libuv.org/dist), [jemalloc](https://github.com/jemalloc/jemalloc/releases/) and [zeromq](http://download.zeromq.org)


### Build on Windows (Windows 7 x64)

1. Run `msvc2013.bat` to generate Visual C++ 2013 solution files.
2. Use Visual Studio 2013 to compile executable binary.

### Build on Linux (Ubuntu 14.04 x64)

Type `premake5 gmake && make config=release`

### Build on OSX (Yosemite 10.10.5)

Type `premake5 --cc=clang gmake && make config=release`


## References

[1] [ZeroMQ The Guide](http://zguide.zeromq.org/page:all)

[2] [Lua 5.3 Manual Reference](http://www.lua.org/manual/5.3/)
