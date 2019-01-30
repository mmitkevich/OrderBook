#!/bin/bash
clang++ -g -O3 -std=c++14 -o test1 test1.cpp
./test1 test_cases/input001.txt > test_cases/my001.txt
diff test_cases/my001.txt test_cases/output001.txt

