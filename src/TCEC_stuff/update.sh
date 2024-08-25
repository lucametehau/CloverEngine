#!/bin/bash
git clone https://github.com/lucametehau/CloverEngine.git --depth 1
echo "Download complete..."

cd CloverEngine/src
make avx512
echo "Compilation complete..."

echo "Benching..."
EXE=$PWD/Clover.7.1.10-avx512
$EXE bench
