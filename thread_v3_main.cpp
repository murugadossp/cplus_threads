#include <iostream>
#include <thread>
#include <vector>
#include <chrono>
#include <sstream>
#include <functional>
#include <map>
#include <random>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <future>
#include <iomanip>

std::mutex cout_mutex;

// Function to get the current timestamp as a string
std::string get_timestamp() {
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d %X");
    return ss.str();
}

// Function to sleep for a given number of seconds
void sleep_for_seconds(int seconds) {
    std::this_thread::sleep_for(std::chrono::seconds(seconds));
}

// Worker class that handles its own queue of tasks
class Worker {
private:
    std::queue<std::function<void()>> tasks;
    std::mutex queue_mutex;
    std::condition_variable condition;
    std::thread worker_thread;
    bool stop = false;

    void run() {
        while (true) {
            std::function<void()> task;
            {
                std::unique_lock<std::mutex> lock(this->queue_mutex);
                this->condition.wait(lock, [this]() { return !this->tasks.empty() || this->stop; });
                if (this->stop && this->tasks.empty()) {
                    break;
                }
                task = std::move(this->tasks.front());
                this->tasks.pop();
            }
            task(); // Execute the task
        }
    }

public:
    Worker() : worker_thread(&Worker::run, this) {}

    ~Worker() {
        {
            std::lock_guard<std::mutex> lock(this->queue_mutex);
            this->stop = true;
        }
        this->condition.notify_all();
        if (this->worker_thread.joinable()) {
            this->worker_thread.join();
        }
    }

    void enqueue(std::function<void()> task) {
        {
            std::lock_guard<std::mutex> lock(this->queue_mutex);
            this->tasks.push(std::move(task));
        }
        this->condition.notify_one();
    }

    // New method to only signal the stop condition
    void signal_stop() {
        std::lock_guard<std::mutex> lock(this->queue_mutex);
        this->stop = true;
        this->condition.notify_all();
    }

    void join() {
        if (worker_thread.joinable()) {
            worker_thread.join();
        }
    }
};

// Function to print a message with a timestamp
void log_with_timestamp(const std::string& function_name, const std::string& message) {
    std::lock_guard<std::mutex> lock(cout_mutex);
    std::cout  << get_timestamp() << " - "<<  function_name << " - " << message << std::endl;
}

// Modify the task functions to use the new logging function
void ping(int sleep_time) {
    std::string message = "Task Starting : Sleeping for " + std::to_string(sleep_time);
    log_with_timestamp("ping", message);
    sleep_for_seconds(sleep_time);
    log_with_timestamp("ping", "Task completed");
}

void data_call_setup(int sleep_time) {
    std::string message = "Task Starting : Sleeping for " + std::to_string(sleep_time);
    log_with_timestamp("data_call_setup", message);
    sleep_for_seconds(sleep_time);
    log_with_timestamp("data_call_setup", "Task completed");
}

void make_voice_call(int sleep_time) {
    std::string message = "Task Starting : Sleeping for " + std::to_string(sleep_time);
    log_with_timestamp("make_voice_call", message);
    sleep_for_seconds(sleep_time);
    log_with_timestamp("make_voice_call", "Task completed");
}

void custom_sleep(int sleep_time) {
    std::string message = "Task Starting : Sleeping for " + std::to_string(sleep_time);
    log_with_timestamp("custom_sleep",  message);
    sleep_for_seconds(sleep_time);
    log_with_timestamp("custom_sleep", "Task completed");
}

int main() {
    std::vector<std::string> inputs = {
            "data_call_setup,vpid1,blocking,1",
            "make_voice_call,vpid2,non-blocking,5",
            "ping,vpid1,non-blocking,5",
            "custom_sleep,vpid0,blocking,3"
    };

    std::map<std::string, Worker> workers;
    std::map<std::string, std::function<void(int)>> function_map = {
            {"ping", ping},
            {"data_call_setup", data_call_setup},
            {"make_voice_call", make_voice_call},
            {"custom_sleep", custom_sleep}
    };

    // Execute tasks in the order they appear in the inputs list
    for (const auto& input : inputs) {
        std::istringstream iss(input);
        std::string function_name, vpid, mode;
        int sleep_time;

        getline(iss, function_name, ',');
        getline(iss, vpid, ',');
        getline(iss, mode, ',');
        iss >> sleep_time; // Directly read the sleep time as an integer

        if (mode == "non-blocking") {
            // For non-blocking, enqueue the task to the worker
            workers[vpid].enqueue([=, &function_map] { function_map.at(function_name)(sleep_time); });
        } else {
            // For blocking, execute the task in the main thread
            function_map.at(function_name)(sleep_time);
        }
//        sleep_for_seconds(1);
    }

    // Signal all workers that no more tasks will be enqueued
    for (auto& worker_pair : workers) {
        worker_pair.second.signal_stop();
    }

    // Wait for all non-blocking workers to complete their tasks
    for (auto& worker_pair : workers) {
        if (worker_pair.first != "vpid0") { // Do not join the worker responsible for blocking tasks
            worker_pair.second.join();
        }
    }

    log_with_timestamp("main", "All tasks are completed.");
    return 0;
}

/***
The output for the inputs

  std::vector<std::string> inputs = {
            "data_call_setup,vpid1,blocking,1",
            "make_voice_call,vpid2,non-blocking,5",
            "ping,vpid1,non-blocking,5",
            "custom_sleep,vpid0,blocking,3"
    };

2023-11-10 07:45:25 - data_call_setup - Task Starting : Sleeping for 1
2023-11-10 07:45:26 - data_call_setup - Task completed
2023-11-10 07:45:26 - custom_sleep - Task Starting : Sleeping for 3
2023-11-10 07:45:26 - make_voice_call - Task Starting : Sleeping for 5
2023-11-10 07:45:26 - ping - Task Starting : Sleeping for 5
2023-11-10 07:45:29 - custom_sleep - Task completed
2023-11-10 07:45:31 - make_voice_call - Task completed
2023-11-10 07:45:31 - ping - Task completed
2023-11-10 07:45:31 - main - All tasks are completed.

 */