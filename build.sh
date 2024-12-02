#!/bin/bash

build(){
local cpu_enable=$1
set -e 
set -o pipefail
rm -rf build
hostname=$(hostname)
make clean
if [ $cpu_enable = true ];then
  make setup 
else
source module_file.sh
fi
if [[ "$hostname" == lassen* ]]; then
make setup MFEM_BUILD=pcuda
INSTALL_PATH=$BENCHMARK_PATH/benchmarks/Laghos/install_lassen
elif [[ "$hostname" == tioga* ]]; then
make setup MFEM_BUILD=phip HIP_ARCH=gfx90a
INSTALL_PATH=$BENCHMARK_PATH/benchmarks/Laghos/install_tioga
elif [[ "$hostname" =~ "palmetto" ]]; then
  echo "Test"
make setup MFEM_BUILD=pcuda
INSTALL_PATH=$BENCHMARK_PATH/benchmarks/Laghos/install_palmetto
fi
if [ $cpu_enable = true ];then
  INSTALL_PATH+="_cpu"
fi
make -j 
make install PREFIX=$INSTALL_PATH 
}
if [ $# -eq 0 ];then 
  build false
fi
if [ "$1" == "cpu" ];then 
  build true
fi 


