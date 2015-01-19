sudo apt-get install libreadline-dev zlib1g-dev uuid-dev

mkdir build && cd build

# libsodium
mkdir libsodium && cd libsodium
wget -c https://github.com/jedisct1/libsodium/archive/master.zip -O libsodium.zip
unzip libsodium.zip
sh ./autogen.sh && ./configure && make
sudo make install
cd ..

# ubuntu zmq version is version 2.2, which is too old
mkdir libzmq && cd libzmq
wget -c https://github.com/zeromq/zeromq4-x/archive/master libzmq.zip
unzip libsodium.zip
./configure && make
sudo make install
cd ..

cd ..