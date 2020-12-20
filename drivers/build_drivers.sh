#!/bin/sh

cd transcoder-pcie
make clean all
if [ $? != 0 ]; then echo "build driver error";exit 1; fi
cp transcoder.h ../
cd -

