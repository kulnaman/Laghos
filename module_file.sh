#!/bin/bash
if [[ "$hostname" == lassen* ]]; then
module load cuda/12.2
module load gcc/12
elif [[ "$hostname" == tioga* ]]; then
module load rocm/6.0.3 
module load rocmcc/6.0.3-cce-17.0.1-magic
elif [[ "$hostname" =~ "palmetto" ]];then
  module load cuda/12.3.0
  module load gcc/12.3.0
  module load openmpi/5.0.1
fi
