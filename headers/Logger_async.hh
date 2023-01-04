#ifndef LOGGER_ASYNC_HH 
#define LOGGER_ASYNC_HH

#include <thread>
#include <ctime>
#include <cassert>

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

#include <unordered_map>
#include <vector>
#include <deque>

#include <memory>
#include <mutex>
#include <condition_variable>

using API_command = std::string;

/**
 * @brief Asynchronous logger class for outputting application log messages to multiple outputs.
 *
 * The logger supports multiple threads and can output to multiple outputs, such as the console or a file.
 * The logger can also output user-defined types by using operator<< overloads.
 * The log messages are produced asynchronously by using a daemon thread.
 *
 * Example:
 * 
 * @code
 *   Logger logger;
 *   std::thread::id thread_id = std::this_thread::get_id();
 *   logger.add_output(thread_id, Logger_async::Log_type::Console);
 *   logger.add_output(thread_id, Logger_async::Log_type::File, "log1_async.txt", false);
 *   logger.add_log(thread_id, "Message from thread 1");
 * @endcode
 */

class Logger_async {
    public:
        Logger_async();
        ~Logger_async();

        /**
        * @brief Enum for type of log.
        */
        enum class Log_type {
            Console,
            FileLog,
            CSVLog
        };

        /**
         * @brief Based output interface for log messages.
         */
        class Output {
            public:
                virtual ~Output() = default;
                virtual void write_log(const std::string& message) = 0;
        };

        /**
         * @brief Output to the console.
         */
        class Console_Log : public Output {
            public:
                void write_log(const std::string& message) override;
        };

        /**
        * @brief Output to a text/log file.
        */
        class File_Log : public Output {
            public:
                File_Log(std::string& filename, bool append_ = false);
                ~File_Log();
                void write_log(const std::string& message) override;
            private:
                std::ofstream file_;
        };

        class CSV_Log : public Output {
            public:
                CSV_Log(std::string& filename, bool append_ = false);
                ~CSV_Log();
                void write_log(const std::string& message) override;

            private:
                std::vector<std::string> split(const std::string& message, char delimiter);
                std::ofstream file_;
        };

        void add_output(std::thread::id thread_id, Log_type _log = Log_type::Console, std::string path = "", bool append_ = true);
        void remove_thread_ouput(std::thread::id thread_id);
        bool add_log(std::thread::id thread_id, std::string message);

    private:
        template <typename T> std::string convert_to_str(T data);
        std::string get_time();
        void daemon_thread();

        std::unordered_map<std::thread::id, std::vector<std::shared_ptr<Output>>> outputs_;     
        std::mutex mutexlock_;      
        std::mutex mutex_queue;                                                                
        std::deque<std::pair<std::thread::id, std::string>> messages_queue;                     
        std::condition_variable condition_; 
        std::condition_variable condition_queue;                                                     
        std::thread daemonthread_;                                                              
        bool stop_daemon = false;                                                               
        
        API_command const Lg_START = "Logger_START";
        API_command const Lg_STOP = "Logger_STOP";
        API_command const Thread_REMOVE = "Thread_RM";                                                               
};

#endif // DATASTRUCTURES_HH