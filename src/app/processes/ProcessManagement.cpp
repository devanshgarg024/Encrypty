#include <iostream>
#include "ProcessManagement.hpp"
#include <cstring>
#include "../encryptDecrypt/Cryption.hpp"


ProcessManagement::ProcessManagement() : stop(false) {
    unsigned int numThreads = std::thread::hardware_concurrency();
    if (numThreads == 0) numThreads = 4;

    for (unsigned int i = 0; i < numThreads; ++i) {
        threadPool.emplace_back(&ProcessManagement::executeTask, this);
    }
}

ProcessManagement::~ProcessManagement() {
    {
        std::unique_lock<std::mutex> lock(queueLock);
        stop = true;
    }
    cv.notify_all();
    for (std::thread &th : threadPool) {
        if (th.joinable()) {
            th.join();
        }
    }
}

bool ProcessManagement::submitToQueue(std::unique_ptr<Task> task) {
    {
        std::unique_lock<std::mutex> lock(queueLock);
        taskQueue.push(task->toString());
    }
    cv.notify_one();
    return true;
}

void ProcessManagement::executeTask() {
    while (true) {
        std::string taskData;
        {
            std::unique_lock<std::mutex> lock(queueLock);
            cv.wait(lock, [this] { return stop.load() || !taskQueue.empty(); });
            
            if (stop.load() && taskQueue.empty()) return;
            
            taskData = std::move(taskQueue.front());
            taskQueue.pop();
        }
        
        char taskStr[256];
        std::strncpy(taskStr, taskData.c_str(), 256);
        taskStr[255] = '\0';
        executeCryption(taskStr);
    }
}