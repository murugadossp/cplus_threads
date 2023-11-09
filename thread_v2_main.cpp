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
    bool stop = false;
    std::thread worker_thread;

    void run() {
        while (true) {
            std::function<void()> task;
            {
                std::unique_lock<std::mutex> lock(this->queue_mutex);
                this->condition.wait(lock, [this]() { return !this->tasks.empty() || stop; });
                if (stop && this->tasks.empty()) {
                    break;
                }
                task = std::move(this->tasks.front());
                this->tasks.pop();
            }
            task(); // Execute the task
        }
    }

public:
    // Constructor
    Worker() : worker_thread(&Worker::run, this) {}

    // Destructor
    ~Worker() {
        {
            std::lock_guard<std::mutex> lock(queue_mutex);
            stop = true;
        }
        condition.notify_one();
        if (worker_thread.joinable()) {
            worker_thread.join();
        }
    }

    // Add task to the worker's queue
    void enqueue(std::function<void()> task) {
        {
            std::lock_guard<std::mutex> lock(queue_mutex);
            tasks.push(std::move(task));
        }
        condition.notify_one();
    }

    // Join the worker's thread (used for blocking tasks)
    void join() {
        if (worker_thread.joinable()) {
            worker_thread.join();
        }
    }
};

// Function definitions
void ping(int sleep_time) {
    sleep_for_seconds(sleep_time);
    std::lock_guard<std::mutex> lock(cout_mutex);
    std::cout << "I am in ping - " << get_timestamp() << std::endl;
}

void data_call_setup(int sleep_time) {
    sleep_for_seconds(sleep_time);
    std::lock_guard<std::mutex> lock(cout_mutex);
    std::cout << "I am in data_call_setup - " << get_timestamp() << std::endl;
}

void make_voice_call(int sleep_time) {
    sleep_for_seconds(sleep_time);
    std::lock_guard<std::mutex> lock(cout_mutex);
    std::cout << "I am in make_voice_call - " << get_timestamp() << std::endl;
}

void custom_sleep(int sleep_time) {
    sleep_for_seconds(sleep_time);
    std::lock_guard<std::mutex> lock(cout_mutex);
    std::cout << "I am in custom_sleep - " << get_timestamp() << std::endl;
}

int main() {
    // Job descriptions
    std::vector<std::string> inputs = {
        "data_call_setup,vpid1,non-blocking", 
        "make_voice_call,vpid2,non-blocking",
        "ping,vpid1,non-blocking",
        "custom_sleep,vpid0,blocking"
    };

    // Random device for generating sleep times
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(1, 5); // Sleep times between 1 and 5 seconds

    // Map of worker threads
    std::map<std::string, Worker> workers;

    // Function map
    std::map<std::string, std::function<void(int)>> function_map = {
        {"ping", ping},
        {"data_call_setup", data_call_setup},
        {"make_voice_call", make_voice_call},
        {"custom_sleep", custom_sleep} // Added custom sleep function
    };

    // Main thread loop to distribute tasks
    for (const auto& input : inputs) {
        std::istringstream iss(input);
        std::string function_name, vpid, mode;
        getline(iss, function_name, ',');
        getline(iss, vpid, ',');
        getline(iss, mode, ',');

        // Generate random sleep time
        int sleep_time = dis(gen);

        if (mode == "non-blocking") {
            // For non-blocking, enqueue the task to the worker
            workers[vpid].enqueue([=] { function_map[function_name](sleep_time); });
        } else {
            // For blocking, execute the task in the main thread
            workers[vpid].enqueue([=] { function_map[function_name](sleep_time); });
            workers[vpid].join(); // Wait for the task to complete
        }
    }

    // Wait for all non-blocking workers to complete their tasks before exiting
    for (auto& worker : workers) {
        worker.second.join();
    }

    std::cout << "All tasks are completed." << std::endl;
    return 0;
}

