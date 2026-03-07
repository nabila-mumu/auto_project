#!/bin/bash

# 1. Initial Setup (Run only the first time)
# cd omnetpp-6.3.0
# source setenv
# cd samples/
# cd auto_project/
#------------------------------

# 2. Clean and Rebuild
echo "Cleaning and generating Makefile..."
make clean
opp_makemake -f --deep -o auto_project

echo "Compiling..."
make -j$(nproc) # -j$(nproc) uses all CPU cores for faster building

# 3. Check if build was successful before running
if [ -f "./auto_project" ]; then
    echo "Running simulation..."
    ./auto_project -u Cmdenv -n . -f simulations/omnetpp.ini
else
    echo "Error: Build failed. Simulation not started."
    exit 1
fi
