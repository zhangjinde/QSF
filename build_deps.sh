sudo apt-get install liblua5.2-dev zlib1g-dev uuid-dev

# git clone https://github.com/jedisct1/libsodium
# git clone https://github.com/zeromq/zeromq4-x

# libsodium for zmq curve
cd deps/libsodium
sh ./autogen.sh
./configure
make
sudo make install
make clean
cd ../

# zmq from ubuntu repo is version 2.2, which is too old for us
cd libzmq
chmod +x ./configure
./configure
make
sudo make install
sudo cp include/zmq.hpp /usr/local/include/
make clean
cd ../

