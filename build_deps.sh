#!/bin/bash
apt-get install -y libncurses5-dev
git submodule update --init --recursive
cd deps/jsondiff-cpp && cmake -DCMAKE_BUILD_TYPE=Release . && make && cd ../..
cd deps/fc && git submodule update --init --recursive && cmake -DCMAKE_BUILD_TYPE=Release . && make && make install && cd ../..
cd deps/fc/vendor/secp256k1-zkp && ./autogen.sh && ./configure && make && make check && make install && cd ../../../..
