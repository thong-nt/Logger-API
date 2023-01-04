#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <mutex>
#include <ctime>

class Logger_sync {
    public:
        // Enum for log levels
        enum class LogLevel {
            DEBUG,
            INFO,
            WARNING,
            ERROR,
        };

        // Output interface for log messages
        class Output {
            public:
                virtual ~Output() = default;
                virtual void write(const std::string& message) = 0;
            };

        // Output to console
        class ConsoleOutput : public Output {
            public:
                void write(const std::string& message) override {
                    std::cout << message << std::endl;
                }
            };

        // Output to file
        class FileOutput : public Output {
            public:
                FileOutput(const std::string& filename, bool append_ = false) {
                    if(append_)
                        file_.open(filename, std::ios::out | std::ios::app);
                    else
                        file_.open(filename);
                }

                void write(const std::string& message) override {
                    file_ << message << std::endl;
                }

            private:
                std::ofstream file_;
        };

        // Constructor
        Logger_sync() : log_level_(LogLevel::DEBUG) {
            outputs_.push_back(std::make_unique<ConsoleOutput>());
        }

        // Add an output source
        void add_output(std::unique_ptr<Output> output) {
            std::lock_guard<std::mutex> lock(mutex_);
            outputs_.push_back(std::move(output));
        }

        // Set the log level
        void set_log_level(LogLevel log_level) {
            log_level_ = log_level;
        }

        //Get the current time
        std::string get_time() {
            time_t current_time = time(0);
            // Convert the time to a string
            char time_str[26];
            errno_t error = ctime_s(time_str, sizeof(time_str), &current_time);

            return time_str;
        }

        // Log a message
        template <typename T>
        void log(LogLevel level, T message) {
            // Check if the log level is high enough
            if (level < log_level_) {
                return;
            }

            std::lock_guard<std::mutex> lock(mutex_);

            // Format the message
            std::string level_str;
            switch (level) {
            case LogLevel::DEBUG:
                level_str = "DEBUG";
                break;
            case LogLevel::INFO:
                level_str = "INFO";
                break;
            case LogLevel::WARNING:
                level_str = "WARNING";
                break;
            case LogLevel::ERROR:
                level_str = "ERROR";
                break;
            }

            std::string time_str = get_time();
            time_str.erase(time_str.end() - 1);

            std::stringstream stream;
            stream << message;

            std::string formatted_message = "[" + time_str + "] - [" + level_str + "]:\t" + stream.str();

            // write the message to all outputs
            for (auto& output : outputs_) {
                output->write(formatted_message);
            }
        }

    private:
        std::vector<std::unique_ptr<Output>> outputs_;
        LogLevel log_level_;
        std::mutex mutex_;
};
