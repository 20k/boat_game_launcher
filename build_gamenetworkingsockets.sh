rm ./libGameNetworkingSockets.dll
rm -r ./deps/GameNetworkingSockets/build
cd deps
cd GameNetworkingSockets
mkdir build
cd build
cmake -G "Ninja" ../
ninja
cd ..
cd ..
cd ..
cp ./deps/GameNetworkingSockets/build/bin/libGameNetworkingSockets.dll ./