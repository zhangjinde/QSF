#!/bin/sh

sudo apt-get install p7zip-full zip unzip
sudo apt-get install build-essential autoconf libtool cmake 
sudo apt-get install libreadline-dev zlib1g-dev uuid-dev libssl-dev libmysqlclient-dev python-dev

mkdir download
cd download

# premake
wget -c http://ncu.dl.sourceforge.net/project/premake/5.0/premake-5.0-alpha3-linux.tar.gz
tar -xzvf premake-5.0-alpha3-linux.tar.gz

# lua5.3
wget -c http://www.lua.org/ftp/lua-5.3.0.tar.gz
tar -xzvf lua-5.3.0.tar.gz

# zeromq 
wget -c http://download.zeromq.org/zeromq-4.0.5.zip
unzip zeromq-4.0.5.zip

# libuv
wget -c http://libuv.org/dist/v1.5.0/libuv-v1.5.0.tar.gz
tar -xzvf libuv-v1.5.0.tar.gz

# jemalloc
wget -c http://www.canonware.com/download/jemalloc/jemalloc-3.6.0.tar.bz2
tar -xjvf jemalloc-3.6.0.tar.bz2