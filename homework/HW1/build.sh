#!/bin/bash

# Compile Autojudge program
gcc autojudge.c -o autojudge

# Check if compilation was successful
if [ $? -eq 0 ]; then
    echo -e "Compilation successful \nRun Autojudge with the following command: ./autojudge -i <inputdir> -a <answerdir> -t <timelimit> <targetsrc> "
else
    echo "Compilation failed"
fi

