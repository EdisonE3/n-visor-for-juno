#!/bin/bash

export KBUILD_OUTPUT=${PWD}/out/juno/mobile_oe/
make -j12 ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu-