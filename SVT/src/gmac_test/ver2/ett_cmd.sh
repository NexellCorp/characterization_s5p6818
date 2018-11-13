#!/bin/bash

arm-cortex_a9-linux-gnueabi-gcc -Wall -Wextra -O2 -static -o ett ethtool.c -DSTANDALONE
