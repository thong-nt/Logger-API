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

/**
 * @brief Asynchronous logger class for outputting application log messages to multiple outputs.
 *
 * The logger supports multiple threads and can output to multiple outputs, such as the console or a file.
 * The logger can also output user-defined types by using operator<< overloads.
 * The log messages are produced asynchronously by using a daemon thread.
 *
 * Example:
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
        /**
         * @brief Constructor of the logger, start the daemon thread.
         */
        Logger_async() {
            assert(__cplusplus >= 201103L && "This API needs at least a C++11 compliant compiler");

            add_output(std::this_thread::get_id(), Logger_async::Log_type::Console);
            add_output(std::this_thread::get_id(), Logger_async::Log_type::FileLog);
            messages_queue.push_back(std::make_pair(std::this_thread::get_id(), "Logger_START"));
            stop_daemon = false;
            daemonthread_ = std::thread(&Logger_async::daemon_thread, this);
        }

        /**
         * @brief Destructor of the logger, stop the daemon thread.
         */
        ~Logger_async() {
            if (daemonthread_.joinable())
            {
                std::lock_guard<std::mutex> lock(mutexlock_);
                messages_queue.push_back(std::make_pair(std::this_thread::get_id(), "Logger_STOP"));
                stop_daemon = true;
            }
            condition_.notify_all();
            daemonthread_.join();
        }

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

                /**
                * @brief            Write a log message to the output.
                * @param message    The log message to write.
                */
                virtual void write_log(const std::string& message) = 0;
        };

        /**
         * @brief Output to the console.
         */
        class Console_Log : public Output {
            public:
                /**
                * @brief            Write a log message to the console.
                * @param message    The log message to write.
                */
                void write_log(const std::string& message) override {
                    std::cout << message << std::endl;
                }
        };

        /**
        * @brief Output to a file.
        */
        class File_Log : public Output {
            public:
                /**
                * @brief            Setting up a file output.
                * @param filename   The name of the file to output to.
                * @param append_    Set mode for output - delete old text or append text.
                */
                File_Log(std::string& filename, bool append_ = false) {
                    if (filename == "") filename = "logs/log.txt";
                    if (append_) file_.open(filename, std::ios::out | std::ios::app);
                    else         file_.open(filename);
                }

                /**
                * @brief            Destructor of the output streams - Close the file.
                */
                ~File_Log(){
                    file_.close();
                }

                /**
                * @brief            Write a log message to the file.
                * @param message    The log message to write.
                */
                void write_log(const std::string& message) override {
                    file_ << message << std::endl;
                }

            private:
                std::ofstream file_;
        };

        class CSV_Log : public Output {
            public:
                /**
                * @brief            Setting up a CSV file output.
                * @param filename   The name of the CSV file to output to.
                * @param append_    Set mode for output - delete old text or append text.
                */
                CSV_Log(std::string& filename, bool append_ = false) {
                    if (filename == "") filename = "logs/log.csv";
                    if (append_) file_.open(filename, std::ios::out | std::ios::app);
                    else         file_.open(filename);
                }

                /**
                * @brief            Destructor of the output streams - Close the file.
                */
                ~CSV_Log(){
                    file_.close();
                }

                /**
                * @brief            Write a log message and value to the CSV file.
                * @param message    The log message to write.
                */
                void write_log(const std::string& message) override {
                    std::vector<std::string> data = split(message,'-');
                    file_ << data[0] << "," << data[1] << "," << data[2] << std::endl;
                }

            private:
                /**
                * @brief                Slipt string to small parts.
                * @param s              A string needs to be slipted.
                * @param delimiter      Seperator.
                */
                std::vector<std::string> split(const std::string& message, char delimiter) {
                    std::vector<std::string> data;
                    std::string token;
                    std::stringstream tokenStream(message);
                    while (std::getline(tokenStream, token, delimiter)) {
                        data.push_back(token);
                    }
                    return data;
                }

                std::ofstream file_;
        };

        /**
         * @brief               Interface method - Add an output source.
         * @param thread_id     Id of the thread needs to be logged.
         * @param _log The      Log type.
         * @param path The      File of path if log type is file output.
         * @param append_       Set mode for output - delete old text or append text.
         */
        void add_output(std::thread::id thread_id, Log_type _log = Log_type::Console, std::string path = "", bool append_ = true) {
            std::shared_ptr<Output> _output = NULL;

            if (_log == Log_type::Console)
                _output = std::make_shared<Console_Log>();
            else if (_log == Log_type::FileLog)
                _output = std::make_shared<File_Log>(path, append_);
            else if (_log == Log_type::CSVLog)
                _output = std::make_shared<CSV_Log>(path, append_);

            std::lock_guard<std::mutex> lock(mutexlock_);
            std::unordered_map<std::thread::id, std::vector<std::shared_ptr<Output>>>::iterator search = outputs_.find(thread_id);

            if (search != outputs_.end()) {
                search->second.push_back(std::move(_output));
            }
            else {
                std::vector<std::shared_ptr<Output>> thread_output_type;
                thread_output_type.push_back(_output);
                outputs_.insert(std::make_pair(thread_id, std::move(thread_output_type)));
            }

        }

        /**
         * @brief               Log a message.
         * @param thread_id     Id of the thread needs to be logged.
         * @param message       The message to log.
         */
        template <typename T>
        void add_log(std::thread::id thread_id, T message) {
            std::lock_guard<std::mutex> lock(mutexlock_);
            messages_queue.push_back(std::make_pair(thread_id, std::move(message)));
            condition_.notify_one();
        }

        /**
         * @brief               Log a message.
         * @param thread_id     Id of the thread needs to be logged.
         */
        void remove_thread_ouput(std::thread::id thread_id) {
            std::lock_guard<std::mutex> lock(mutexlock_);
            messages_queue.push_back(std::make_pair(thread_id, "Thread_destroy"));
            condition_.notify_one();
        }

    private:

        /**
         * @brief          Convert all data type to string.
         * @param data     Data needs to be converted.
         */
        template <typename T>
        std::string convert_to_str(T data) {
            std::stringstream strm;
            strm << data;
            return strm.str();
        }

        /**
         * @brief  Get the current time.
         */
        std::string get_time() {
            time_t current_time = time(0);

            char time_str[26];
            errno_t error = ctime_s(time_str, sizeof(time_str), &current_time);

            std::string time_ = convert_to_str(time_str);
            time_.pop_back();

            return time_;
        }

        /**
         * @brief  Daemon thread for outputting log messages.
         */
        void daemon_thread() {
            while (!stop_daemon) {
                std::unique_lock<std::mutex> lock(mutexlock_);
                condition_.wait(lock, [&] { return !messages_queue.empty(); });

                std::pair<std::thread::id, std::string> message = messages_queue.front();
                messages_queue.pop_front();
                lock.unlock();
                
                std::string log_message = "[" + get_time() + "] - "+
                                          "[" + convert_to_str(message.first) + "]"
                                          +"\t- " + convert_to_str(message.second);

                std::lock_guard<std::mutex> output_lock(mutexlock_);
                std::unordered_map<std::thread::id, std::vector<std::shared_ptr<Output>>>::iterator search = outputs_.find(message.first);
                if (search != outputs_.end()) {
                    for (std::shared_ptr<Output> output : search->second) {
                        output->write_log(log_message);
                    }
                    if (message.second == "Thread_destroy")
                    {
                        outputs_.erase(message.first);
                    }
                }
            }
        }

        std::unordered_map<std::thread::id, std::vector<std::shared_ptr<Output>>> outputs_;     ///< Output sources
        std::mutex mutexlock_;                                                                  ///< Mutex for synchronizing 
        std::deque<std::pair<std::thread::id, std::string>> messages_queue;                     ///< Queue of log messages
        std::condition_variable condition_;                                                     ///< Condition variable for synchronizing access
        std::thread daemonthread_;                                                              ///< Daemon thread for outputting log messages
        bool stop_daemon = false;                                                               ///< Flag for stopping the daemon thread
};