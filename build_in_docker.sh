#!/bin/bash
container_id="uvm_uvmbuild_dev"
docker exec $container_id ./build_deps.sh
docker exec $container_id cmake .
docker exec $container_id make

