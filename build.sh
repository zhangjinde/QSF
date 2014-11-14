mkdir bin

# lua
cd deps/lua
make clean
make linux
mv src/liblua.a ../../bin/liblua.a

# 0mq
cd ../zeromq
chmod +x configure
./configure
make install

# zlib
cd ../zlib
chmod +x configure
./configure
make install

premake4 gmake
make config=release64

