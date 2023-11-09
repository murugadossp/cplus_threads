#include <iostream>
#include <thread>
#include <vector>
#include <map>
#include <mutex>
#include <condition_variable>

#include <chrono>
#include <ctime>
#include <iomanip>
#include <string>

std::string get_current_timestamp() {
  auto now = std::chrono::system_clock::now();
  auto in_time_t = std::chrono::system_clock::to_time_t(now);
  std::stringstream ss;
  ss << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d %X");
  return ss.str();
}

// Define a function to sleep for a specified number of seconds
void sleep_for_seconds(int seconds) {
  std::this_thread::sleep_for(std::chrono::seconds(seconds));
}

// Define the functions to be executed by the threads
void function1() {

  std::cout << get_current_timestamp() << " Start Function 1 executed by thread " << std::this_thread::get_id() << std::endl;
  sleep_for_seconds(5);
  std::cout << get_current_timestamp() << " Stop Function 1 executed by thread " << std::this_thread::get_id() << std::endl;
}

void function2() {
  std::cout << get_current_timestamp() << " Start Function 2 executed by thread " << std::this_thread::get_id() << std::endl;
  sleep_for_seconds(10);
    std::cout << get_current_timestamp() << " Stop Function 2 executed by thread " << std::this_thread::get_id() << std::endl;
}

void function3() {
  std::cout << get_current_timestamp() << " Start Function 3 executed by thread " << std::this_thread::get_id() << std::endl;
   sleep_for_seconds(5);
}

int main() {
  // Create an input array of function names and thread names
  std::vector<std::string> input = {"function1, vpid1", "function2, vpid2", 
  "function3, vpid1"
  };

  // Create a map of function names and function pointers
  std::map<std::string, void(*)()> functions = {
    {"function1", function1},
    {"function2", function2},
    {"function3", function3}
  };

  // Create a map of thread names and thread objects
  std::map<std::string, std::thread> threads;

  // Create a map of thread names and condition variables
  std::map<std::string, std::condition_variable> cvs;

  // Create a mutex to protect the condition variables
  std::mutex mtx;

  // Loop through the input array and create or submit jobs to the threads
  for (auto& s : input) {
    // Split the string by comma and trim the spaces
    std::string function_name = s.substr(0, s.find(','));
    std::string thread_name = s.substr(s.find(',') + 1);
    function_name = function_name.substr(function_name.find_first_not_of(' '));
    thread_name = thread_name.substr(thread_name.find_first_not_of(' '));

    // Check if the thread name already exists in the map
    if (threads.find(thread_name) != threads.end()) {
      // If yes, submit the function to the existing thread
      std::unique_lock<std::mutex> lock(mtx);
      cvs[thread_name].wait(lock); // wait for the thread to be ready
      threads[thread_name] = std::thread(functions[function_name]);
      cvs[thread_name].notify_one(); // notify the thread that the function is ready
    } else {
      // If not, create a new thread with the given name and function
      threads[thread_name] = std::thread(functions[function_name]);
      cvs[thread_name]; // create a new condition variable for the thread
    }
    sleep_for_seconds(1);
  }

  std::cout << get_current_timestamp() << " Wait for all thread to complete " << std::this_thread::get_id() << std::endl;

  // Wait for all the threads to complete
  for (auto& t : threads) {
    std::unique_lock<std::mutex> lock(mtx);
    cvs[t.first].wait(lock); // wait for the thread to be ready
    t.second.join();
  }
  std::cout << get_current_timestamp() << "All threads completed " << std::this_thread::get_id() << std::endl;

  return 0;
}
