﻿#include <iostream>
#include <chrono>

#include "header/Logger_async.hh"

void example_logger_async(Logger_async &logger) {
    std::this_thread::sleep_for(std::chrono::seconds(1));

    // Log some messages from multiple threads
    std::thread t1([&] {
        std::thread::id thread_id = std::this_thread::get_id();
        logger.add_output(thread_id, Logger_async::Log_type::Console);
        logger.add_output(thread_id, Logger_async::Log_type::FileLog, "logs/log1_async.log", false);
        logger.add_output(thread_id, Logger_async::Log_type::CSVLog, "logs/log.csv", false);

        logger.add_log(thread_id, "Message from thread 1");
        std::this_thread::sleep_for(std::chrono::seconds(1));

        for (int i = 0; i < 10; i++)
            logger.add_log(thread_id, "Message from thread 1");

        logger.remove_thread_ouput(thread_id);
        });
        

    std::thread t2([&] {
        std::thread::id thread_id = std::this_thread::get_id();
        logger.add_output(thread_id, Logger_async::Log_type::Console);
        logger.add_output(thread_id, Logger_async::Log_type::FileLog, "logs/log.txt", true);

        logger.add_log(thread_id, "Message from thread 2");
        std::this_thread::sleep_for(std::chrono::seconds(3));

        for (int i = 0; i < 10; i++) 
            logger.add_log(thread_id, "Message from thread 2");

        logger.remove_thread_ouput(thread_id);
     });

    std::thread t3([&] {
        std::thread::id thread_id = std::this_thread::get_id();
        logger.add_output(thread_id, Logger_async::Log_type::Console);
        logger.add_output(thread_id, Logger_async::Log_type::FileLog, "logs/log.txt", true);

        logger.add_log(thread_id, "Message from thread 3");
        std::this_thread::sleep_for(std::chrono::seconds(3));
        for (int i = 0; i < 10; i++)
            logger.add_log(thread_id, "Message from thread 3");

        logger.remove_thread_ouput(thread_id);
        });

    // Wait for the threads to finish
    t1.join();
    t2.join();
    t3.join();

    // Wait for a bit to allow the logger time to process the messages
    std::this_thread::sleep_for(std::chrono::seconds(1));
}

//void example_logger_sync() {
//    Logger_sync logger;
//
//    // Add an output
//    logger.add_output(std::make_unique<Logger_sync::ConsoleOutput>());
//    logger.add_output(std::make_unique<Logger_sync::FileOutput>("log_sync.txt"));
//
//    // Create two threads
//    std::thread thread1([&logger] {
//        for (int i = 0; i < 1000; i++)
//            logger.log(Logger_sync::LogLevel::INFO, "This is a log message from thread 1.");
//        });
//    std::thread thread2([&logger] {
//        for (int i = 0; i < 1000; i++)
//            logger.log(Logger_sync::LogLevel::INFO, "This is a log message from thread 2.");
//        });
//
//    // Wait for the threads to finish
//    thread1.join();
//    thread2.join();
//
//}

int main() {
    Logger_async logger;
    //Running test
    example_logger_async(logger);
    //example_logger_sync();
    std::cout << "Press any key to continue..." << std::endl;
    getchar();
    return 0;
}

