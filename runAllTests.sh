#!/bin/bash

# TO USE:
# 1. give yourself permission to execute using: 
#       chmod u+x runAllTests.sh
#    in the terminal.
# 2. to run, use:
#       ./runAllTests.sh
#    inside the project folder directory

# reset project files
make clean
make

# run project for for all test cases
for policy in 1 2 
do
    for numFrames in 1 4 5 8 12 16
    do
        for inputNum in {1..12}
        do
            ./proj3 $policy $numFrames sample_input/input_$inputNum
        done
    done
done

# compares results with sample outputs
for policy in 1 2 
do
    for numFrames in 1 4 5 8 12 16
    do
        for inputNum in {1..12}
        do
            echo Testing:  Policy=$policy  NumFrames=$numFrames  Input=$inputNum
            diff output/result-$policy-$numFrames-input_$inputNum sample_output/result-$policy-$numFrames-input_$inputNum
        done
    done
done

