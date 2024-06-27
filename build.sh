#!/bin/bash

build(){
local cpu_enable=$1
set -e 
set -o pipefail
make clean
if [ $cpu_enable = true ];then
  make setup 
else
source module_file.sh
make setup MFEM_BUILD=pcuda
fi
make -j 
module list
if [ $cpu_enable = false ];then
make install PREFIX=$BENCHMARK_PATH/benchmarks/Laghos/install_lassen
else
make install PREFIX=$BENCHMARK_PATH/benchmarks/Laghos/install_lassen_cpu
  
fi
}
if [ $# -eq 0 ];then 
  build false
fi
if [ "$1" == "cpu" ];then 
  build true
fi 


