mkdir download && cd download

sudo apt-get install p7zip-full zip unzip
sudo apt-get install build-essential autoconf libtool cmake 
sudo apt-get install libreadline-dev zlib1g-dev uuid-dev libssl-dev libmysqlclient-dev

# libsodium
wget -c http://download.libsodium.org/libsodium/releases/libsodium-1.0.1.tar.gz
tar -xzvf libsodium-1.0.1.tar.gz
cd libsodium-1.0.1
sh ./autogen.sh && ./configure && make
sudo make install
cd ..

# zeromq 4.x
wget -c http://download.zeromq.org/zeromq-4.0.5.zip
unzip zeromq-4.0.5.zip
cd zeromq-4.0.5
./configure && make
sudo make install
cd ..

cd ..