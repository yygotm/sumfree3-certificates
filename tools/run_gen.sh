#!/bin/sh
# usage: sh run_gen.sh cpu|gpu N
#   Generates the five remaining sequences up to N, one at a time, and looks for a period.
#   cpu: needs gcc.  gpu: needs ROCm's hipcc and an AMD GPU (tested: RX 9070 XT, gfx1201);
#        set ARCH for another GPU, e.g. ARCH=gfx1100.
#   Outputs: out/s_f_g_h_NN.bin (terms as little-endian uint32), out/period_NN.txt, out/sha256_NN.txt
set -e
cd "$(dirname "$0")"
MODE=$1; N=$2
mkdir -p out
gcc -O2 -o period_fast period_fast.c
if [ "$MODE" = gpu ]; then
  hipcc -O3 -std=c++17 --offload-arch=${ARCH:-gfx1201} sumfree3_hip.cpp -o sumfree3_hip
  GEN=./sumfree3_hip
else
  gcc -O3 -march=native -o sumfree3r_opt sumfree3r_opt.c
  GEN=./sumfree3r_opt
fi
for t in "4 6 17" "4 5 19" "7 9 26" "5 7 26" "6 7 27"; do
  set -- $t
  f=out/s_$1_$2_$3_N$N.bin
  $GEN $1 $2 $3 $N $f 2> $f.log
  ./period_fast $N $f | tee -a out/period_N$N.txt
  sha256sum $f | tee -a out/sha256_N$N.txt
done
