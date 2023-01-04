#include "../headers/Logger_async.hh"
#include "../headers/Logger_test.hh"

int main()
{
    Logger_async logger;
    Logger_test test;

    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    test.test_handle_output_err(logger);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    test.test_file_output(logger);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    test.test_logger_multithread(logger);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    test.test_huge_logs_load(logger, 10000);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    test.test_logger_create_file(logger);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    test.test_report();

    return 0;
}