#include "ReadEnv.hpp" // Include the header file for the class
#include "IO.hpp"
#include <fstream>
#include <iostream>
#include <string>
#include <sstream>

// This is the IMPLEMENTATION of the getenv function declared in the header
std::string ReadEnv::getenv(const std::string& key_name) {
    std::string env_path = ".env";
    std::ifstream f_stream(env_path);
    if (!f_stream.is_open()) {
        return ""; // Return empty if .env is not found
    }

    std::string line;
    while (std::getline(f_stream, line)) {
        size_t delimiter_pos = line.find('=');
        if (delimiter_pos != std::string::npos) {
            std::string key = line.substr(0, delimiter_pos);
            if (key == key_name) {
                // Return the value part of the line
                return line.substr(delimiter_pos + 1);
            }
        }
    }
    return ""; // Return empty if the key was not found
}