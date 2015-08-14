#!/bin/sh

sudo apt-get install p7zip-full zip unzip
sudo apt-get install build-essential autoconf pkg-config libtool cmake 
sudo apt-get install libreadline-dev zlib1g-dev uuid-dev libssl-dev libmysqlclient-dev python-dev

mkdir download
cd download

# premake
wget -c https://github.com/premake/premake-core/releases/download/v5.0.0.alpha4/premake-5.0.0.alpha4-linux.tar.gz
tar -xzvf premake-5.0.0.alpha4-linux.tar.gz

# lua5.3
wget -c http://www.lua.org/ftp/lua-5.3.1.tar.gz
tar -xzvf lua-5.3.1.tar.gz

#libsodium
wget -c https://download.libsodium.org/libsodium/releases/libsodium-1.0.3.tar.gz
tar -xzvf libsodium-1.0.3.tar.gz

# zeromq 
wget -c http://download.zeromq.org/zeromq-4.1.2.tar.gz
tar -xzvf zeromq-4.1.2.tar.gz

# libuv
wget -c http://dist.libuv.org/dist/v1.7.0/libuv-v1.7.0.tar.gz
tar -xzvf libuv-v1.6.1.tar.gz

# jemalloc
wget -c https://github.com/jemalloc/jemalloc/releases/download/3.6.0/jemalloc-3.6.0.tar.bz2
tar -xjvf jemalloc-3.6.0.tar.bz2