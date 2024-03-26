#!/bin/bash

set -xe

clang -O3 -o ccloc -pthread ccloc.c
