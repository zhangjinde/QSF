sudo apt-get install liblua5.2-dev zlib1g-dev libboost1.55-dev

# zmq 2.2 is too old, compile version 4.0 for our own self
cd deps/libzmq
chmod +x ./configure
./configure
make
sudo make install
sudo cp include/zmq.hpp /usr/local/include/
make clean

