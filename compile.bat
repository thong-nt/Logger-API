@echo off
g++ -std=c++11 -pthread source/Logger.cpp source/Logger_async.cpp -o Logger
Logger.exe
@pause