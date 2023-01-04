#include <thread>
#include <stdio.h>

#include <vector>
#include <string>

#include "logger_async.hh"

/**
 * @brief Test logger class for testing basic functions of the API.
 */
class Logger_test
{
    public:
        Logger_test();
        ~Logger_test();

        std::string get_time();
        void test_handle_output_err(Logger_async &logger);
        void test_file_output(Logger_async &logger);
        void test_logger_multithread(Logger_async &logger);
        void test_huge_logs_load(Logger_async &logger, int num_line=10000);
        void test_logger_create_file(Logger_async &logger);
        void test_report();

        template <typename T>  std::string convert_to_str(T data) {
            std::stringstream strm;
            strm << data;
            return strm.str();
        }
    
    private:
        void count_failed_test();
        void count_total_test();

        std::vector<std::string> list_test_file = {"logs/test1/test_handle_output_err.txt",
                                                    "logs/test2/test_file_output.txt",
                                                    "logs/test3/log_multithread.txt",
                                                    "logs/test4/log_hugeload.txt",
                                                    "logs/test5/test_logger_create_file.csv"
                                                };
        int failed_tests = 0;
        int total_tests = 0;
};


