#!/bin/sh

rmmod transcoder_pcie

cd transcoder-pcie
insmod transcoder_pcie.ko
cd -


