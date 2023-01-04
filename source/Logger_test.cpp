#include "../headers/logger_async.hh"
#include "../headers/logger_test.hh"

#include <thread>
#include <stdio.h>
#include <cassert>

/**
 * @brief Constructor of the test.
 */
Logger_test::Logger_test()
{
    std::cout << "\nUnit Tests start!\n" << std::endl;
}

/**
 * @brief Destructor of the test.
 */
Logger_test::~Logger_test()
{
    std::cout << "\nUnit Tests end!" << std::endl;
    getchar();
}

/**
 * @brief  Get the current time.
 */
std::string Logger_test::get_time() {
    time_t current_time = time(0);

    char time_str[26];
    errno_t error = ctime_s(time_str, sizeof(time_str), &current_time);

    std::string time_ = convert_to_str(time_str);
    time_.pop_back();

    return time_;
}

/**
 * @brief           Testing handling of API when user forgets to register output method. 
 * @param logger    Logger to output message.
 */
void Logger_test::test_handle_output_err(Logger_async &logger){
    std::thread::id thread1_id;
    std::thread thread1([&] {
    thread1_id = std::this_thread::get_id();
    logger.add_output(thread1_id, Logger_async::Log_type::FileLog, Logger_test::list_test_file[0], true);
    bool check = logger.add_log(thread1_id, "Message");

    Logger_test::count_total_test();
    if (check){
        std::cout << "test_handle_output_err: Passed" << std::endl;
    }else{
        std::cout << "test_handle_output_err: Failed\n" ;
        Logger_test::count_failed_test();
    }
    
    logger.remove_thread_ouput(thread1_id);
    });
    thread1.join();
}

/**
 * @brief           Testing if API can log correct message from the users. 
 * @param logger    Logger to output message.
 */
void Logger_test::test_file_output(Logger_async &logger) {
    std::string expected_output = "";
    std::thread t1([&] {
        std::thread::id thread_id = std::this_thread::get_id();
        logger.add_output(thread_id, Logger_async::Log_type::FileLog, Logger_test::list_test_file[1], false);
        logger.add_log(thread_id, "Test message");

        expected_output =  "[" + get_time() + "]"
                                        + " - [" + convert_to_str(thread_id) + "]\t-"
                                        + " Test message";
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        
        std::ifstream file(Logger_test::list_test_file[1],std::ios::in);
        std::string line;

        if (file.is_open()){
            getline(file, line);
    }

    Logger_test::count_total_test();
    if(line == expected_output){
        std::cout << "test_file_output: Passed" << std::endl;
    }
    else{
        std::cout <<  "test_file_output: Failed" << std::endl;
        Logger_test::count_failed_test();
    }

    file.close();
    logger.remove_thread_ouput(thread_id);
    });
    t1.join();
}

/**
 * @brief           Testing if API can log correct message from multiple threads. 
 * @param logger    Logger to output message.
 */
void Logger_test::test_logger_multithread(Logger_async &logger) {
    std::vector<std::thread> threads;
    std::string file_path = Logger_test::list_test_file[2];

    for (int i = 0; i < 10; i++) {
        threads.push_back(std::thread([&logger, &file_path] {
            logger.add_output(std::this_thread::get_id(), Logger_async::Log_type::FileLog, file_path, true);
        }));
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    for (auto& thread : threads) {
        logger.add_log(thread.get_id(), "Log message from thread");
    }

    std::this_thread::sleep_for(std::chrono::seconds(1));

    std::ifstream log_file(Logger_test::list_test_file[2]);
    std::string line;
    int line_count = 0;
    while (std::getline(log_file, line)) {
        line_count++;
    }
    for (auto& thread : threads) {
        logger.remove_thread_ouput(thread.get_id());
        thread.join();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    log_file.close();
    
    Logger_test::count_total_test();
    if(line_count == 10){
        std::cout << "test_logger_multithread: Passed" << std::endl;
    }
    else{
        std::cout <<  "test_logger_multithread: Failed" << std::endl;
        Logger_test::count_failed_test();
    }
}

/**
 * @brief           Testing if API can handle large number of messages. 
 * @param logger    Logger to output message.
 */
void Logger_test::test_huge_logs_load(Logger_async &logger, int num_line){
    std::thread::id thread1_id;
    std::thread::id thread2_id;

    std::thread thread1([&] {
    thread1_id = std::this_thread::get_id();
    logger.add_output(thread1_id, Logger_async::Log_type::FileLog, Logger_test::list_test_file[3], true);

    for (int i = 0; i < num_line; i++)
        logger.add_log(thread1_id, "Message from thread 1");
    });
    

    std::thread thread2([&] {
        thread2_id = std::this_thread::get_id();
        logger.add_output(thread2_id, Logger_async::Log_type::FileLog, Logger_test::list_test_file[3], true);

        for (int i = 0; i < num_line; i++) {
            logger.add_log(thread2_id, "Message from thread 2");
        }
     });

    thread1.join();
    thread2.join();

    std::this_thread::sleep_for(std::chrono::seconds(5));

    std::ifstream log_file(Logger_test::list_test_file[3]);
    std::string line;
    int line_count = 0;
    while (std::getline(log_file, line)) {
        line_count++;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    log_file.close();

    Logger_test::count_total_test();
    if(line_count == num_line*2){
        std::cout << "test_huge_logs_load: Passed" << std::endl;
    }
    else{
        std::cout <<  "test_huge_logs_load: Failed" << std::endl;
        Logger_test::count_failed_test();
    }

    logger.remove_thread_ouput(thread1_id);
    logger.remove_thread_ouput(thread2_id);
}

/**
 * @brief           Testing if API can create correct target log files. 
 * @param logger    Logger to output message.
 */
void Logger_test::test_logger_create_file(Logger_async &logger){
    std::thread t1([&] {
        std::thread::id thread_id = std::this_thread::get_id();
        logger.add_output(thread_id, Logger_async::Log_type::CSVLog, Logger_test::list_test_file[4], false);
        logger.add_log(thread_id, "Message from thread 1");
        logger.remove_thread_ouput(thread_id);
    });
    t1.join();

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    bool err = false;

    for (auto file : Logger_test::list_test_file){
        int result = remove(file.c_str());
        if (result != 0) 
            err = true;
    }

    Logger_test::count_total_test();
    if(!err){
        std::cout << "test_logger_create_file: Passed" << std::endl;
    }
    else{
        std::cout <<  "test_logger_create_file: Failed" << std::endl;
        Logger_test::count_failed_test();
    }
}

/**
 * @brief           Count the number of failed tests.
 */
void Logger_test::count_failed_test(){
    failed_tests++;
}

/**
 * @brief           Count the total number of performing tests.
 */
void Logger_test::count_total_test(){
    total_tests++;
}

/**
 * @brief           Report result of the test.
 */
void Logger_test::test_report(){
    std::cout << "\nTotal test: " << total_tests << std::endl;
    std::cout << "Passed test: " << total_tests-failed_tests << std::endl;
    std::cout << "Failed test: " << failed_tests << std::endl;
}
