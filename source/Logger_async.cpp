#include "../headers/Logger_async.hh"

/**
 * @brief Constructor of the logger, start the daemon thread.
 */
Logger_async::Logger_async() {
    assert(__cplusplus >= 201103L && "This API needs at least a C++11 compliant compiler");

    add_output(std::this_thread::get_id(), Logger_async::Log_type::Console);
    add_output(std::this_thread::get_id(), Logger_async::Log_type::FileLog);
    messages_queue.push_back(std::make_pair(std::this_thread::get_id(), Lg_START));
    stop_daemon = false;
    daemonthread_ = std::thread(&Logger_async::daemon_thread, this);
}

/**
 * @brief Destructor of the logger, stop the daemon thread.
 */
Logger_async::~Logger_async() {
    if (daemonthread_.joinable())
    {
        std::lock_guard<std::mutex> lock(mutexlock_);
        messages_queue.push_back(std::make_pair(std::this_thread::get_id(), Lg_STOP));
    }
    condition_.notify_all();
    daemonthread_.join();
}

/**
* @brief            Write a log message to the console.
* @param message    The log message to write.
*/
void Logger_async::Console_Log::write_log(const std::string& message)  {
    std::cout << message << std::endl;
}

/**
* @brief            Setting up a file output.
* @param filename   The name of the file to output to.
* @param append_    Set mode for output - delete old text or append text.
*/
Logger_async::File_Log::File_Log(std::string& filename, bool append_) {
    if (filename == "") filename = "logs/log.txt";
    if (append_) file_.open(filename, std::ios::out | std::ios::app);
    else         file_.open(filename);
}

/**
* @brief            Destructor of the output streams - Close the file.
*/
Logger_async::File_Log::~File_Log(){
    file_.close();
}

/**
* @brief            Write a log message to the file.
* @param message    The log message to write.
*/
void Logger_async::File_Log::write_log(const std::string& message) {
    file_ << message << std::endl;
}

/**
* @brief            Setting up a CSV file output.
* @param filename   The name of the CSV file to output to.
* @param append_    Set mode for output - delete old text or append text.
*/
Logger_async::CSV_Log::CSV_Log(std::string& filename, bool append_) {
    if (filename == "") filename = "logs/log.csv";
    if (append_) file_.open(filename, std::ios::out | std::ios::app);
    else         file_.open(filename);
}

/**
* @brief            Destructor of the output streams - Close the file.
*/
Logger_async::CSV_Log::~CSV_Log(){
    file_.close();
}

/**
* @brief            Write a log message and value to the CSV file.
* @param message    The log message to write.
*/
void Logger_async::CSV_Log::write_log(const std::string& message) {
    std::vector<std::string> data = Logger_async::CSV_Log::split(message,'-');
    file_ << data[0] << "," << data[1] << "," << data[2] << std::endl;
}

/**
* @brief                Slipt string to small parts.
* @param s              A string needs to be slipted.
* @param delimiter      Seperator.
*/
std::vector<std::string> Logger_async::CSV_Log::split(const std::string& message, char delimiter) {
    std::vector<std::string> data;
    std::string token;
    std::stringstream tokenStream(message);
    while (std::getline(tokenStream, token, delimiter)) {
        data.push_back(token);
    }
    return data;
}

/**
 * @brief               Interface method - Add an output source.
 * @param thread_id     Id of the thread needs to be logged.
 * @param _log          The log type.
 * @param path          The file of path if log type is file output.
 * @param append_       Set mode for output - delete old text or append text.
 */
void Logger_async::add_output(std::thread::id thread_id, Log_type _log, std::string path, bool append_) {
    std::shared_ptr<Output> _output = NULL;

    if (_log == Log_type::Console)
        _output = std::make_shared<Console_Log>();
    else if (_log == Log_type::FileLog)
        _output = std::make_shared<File_Log>(path, append_);
    else if (_log == Log_type::CSVLog)
        _output = std::make_shared<CSV_Log>(path, append_);

    std::lock_guard<std::mutex> lock(mutexlock_);
    auto search = outputs_.find(thread_id);

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
bool Logger_async::add_log(std::thread::id thread_id, std::string message) {
    std::lock_guard<std::mutex> lock(mutexlock_);
    auto search = outputs_.find(thread_id);
    if (search != outputs_.end()) {
        messages_queue.push_back(std::make_pair(thread_id, message));
        condition_.notify_one();
        return true;
    }
    else {
        std::cout << "Thread [" << convert_to_str(thread_id) << ("] Error while trying to log message! Check if output method is registered or not.\n");
        condition_.notify_one();
        return false;
    }
}

/**
 * @brief               Log a message.
 * @param thread_id     Id of the thread needs to be logged.
 */
void Logger_async::remove_thread_ouput(std::thread::id thread_id) {
    std::lock_guard<std::mutex> lock(mutexlock_);
    messages_queue.push_back(std::make_pair(thread_id, Thread_REMOVE));
    condition_.notify_one();
}

/**
 * @brief          Convert all data type to string.
 * @param data     Data needs to be converted.
 */
template <typename T> std::string Logger_async::convert_to_str(T data) {
    std::stringstream strm;
    strm << data;
    return strm.str();
}

/**
 * @brief  Get the current time.
 */
std::string Logger_async::get_time() {
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
void Logger_async::daemon_thread() {
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
            if (message.second == Thread_REMOVE)
            {
                outputs_.erase(message.first);
            }
        }

        if (message.second == Lg_STOP)
        {
            stop_daemon=true;
        }
    }
}