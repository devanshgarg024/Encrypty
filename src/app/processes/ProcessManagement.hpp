#ifndef PROCESS_MANAGEMENT_HPP
#define PROCESS_MANAGEMENT_HPP

#include "Task.hpp"
#include <memory>
#include <mutex>
#include <atomic>
#include <queue>
#include <condition_variable>
#include <thread>
#include <vector>
#include <string>

class ProcessManagement
{
public:
    ProcessManagement();
    ~ProcessManagement();

    bool submitToQueue(std::unique_ptr<Task> task);

private:
    void executeTask();
    
    std::queue<std::string> taskQueue;
    std::mutex queueLock;
    std::condition_variable cv;
    std::vector<std::thread> threadPool;
    std::atomic<bool> stop;
};

#endif 