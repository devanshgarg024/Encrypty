#ifndef READENV_HPP
#define READENV_HPP

#include <string>

class ReadEnv {
public:
    // This function looks for a key (e.g., "CRYPTION_PASSWORD") in a .env file 
    // and returns the corresponding value.
    std::string getenv(const std::string& key_name);
};

#endif // READENV_HPP