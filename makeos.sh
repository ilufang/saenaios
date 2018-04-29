#!/bin/bash
./createfs -i fsdir -o student-distrib/filesys_img
cd student-distrib
./debug.sh
cd ..
