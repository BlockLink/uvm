#!/bin/bash
docker-compose build 
container_id="uvm_uvmbuild_dev"
project_dir=`pwd`
$image="uvm_uvmbuild"
docker run -t -d -v $project_dir:/code --name $container_name $image /bin/bash

