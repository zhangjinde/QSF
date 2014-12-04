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


## References

[1] [ZeroMQ The Guide](http://zguide.zeromq.org/page:all)

[2] [Lua 5.2 Manual Reference](http://www.lua.org/manual/5.2/)

[3] [MySQL 5.5 C API Function Overview](http://dev.mysql.com/doc/refman/5.5/en/c-api-function-overview.html)

[4] [Redis Command List](http://redis.io/commands)
