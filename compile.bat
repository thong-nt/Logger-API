@echo off
g++ -std=c++11 -pthread Logger.cpp -o Logger
start Logger.exe
@pause