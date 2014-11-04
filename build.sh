mkdir bin
cd deps/lua
make clean
make linux
mv src/liblua.a ../../bin/liblua.a

#
cd ../deps/zeromq
chmod +x configure
./configure
make

premake4 gmake
make config=release64

