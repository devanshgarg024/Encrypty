#include <iostream>
#include <filesystem>
#include "./src/app/processes/ProcessManagement.hpp"
#include "./src/app/processes/Task.hpp"
#include <ctime>
#include <iomanip>
#include <sys/types.h>
#include <vector>   
#include <sys/wait.h>

namespace fs = std::filesystem;

int main(int argc, char* argv[]) {
    std::string directory;
    std::string action;

    std::cout << "Enter the directory path: ";
    std::getline(std::cin, directory);

    std::cout << "Enter the action (encrypt/decrypt): ";
    std::getline(std::cin, action);

    try {
        if (fs::exists(directory) && fs::is_directory(directory)) {
            ProcessManagement processManagement;
            std::vector<pid_t> child_pids;

            for (const auto& entry : fs::recursive_directory_iterator(directory)) {
                if (entry.is_regular_file()) {
                    std::string filePath = entry.path().string();
                    IO io(filePath);
                    std::fstream f_stream = std::move(io.getFileStream());

                    if (f_stream.is_open()) {
                        Action taskAction = (action == "encrypt") ? Action::ENCRYPT : Action::DECRYPT;
                        auto task = std::make_unique<Task>(std::move(f_stream), taskAction, filePath);
                        
                            std::time_t t = std::time(nullptr);
                            std::tm* now = std::localtime(&t);
                            std::cout << "Starting the " <<((action == "encrypt") ? "encryption " : "decryption ")<<"at: " << std::put_time(now, "%Y-%m-%d %H:%M:%S") << std::endl;
                            pid_t child_pid = processManagement.submitToQueue(std::move(task));
                            if (child_pid > 0) { // A valid PID is positive
                                child_pids.push_back(child_pid);
                            }
                    } else {
                        std::cout << "Unable to open file: " << filePath << std::endl;
                    }
                }
            }

            std::cout << "\nAll tasks submitted. Waiting for completion..." << std::endl;
            for (pid_t pid : child_pids) {
                waitpid(pid, nullptr, 0); // The parent process waits here
            }
            std::cout << "All processes have finished." << std::endl;

        } else {
            std::cout << "Invalid directory path!" << std::endl;
        }
    } catch (const fs::filesystem_error& ex) {
        std::cout << "Filesystem error: " << ex.what() << std::endl;
    }

    return 0;
}