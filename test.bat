@echo off
g++ -std=c++11 -pthread source/unit_test.cpp source/Logger_test.cpp source/Logger_async.cpp -o Logger_test
Logger_test.exe
@pause