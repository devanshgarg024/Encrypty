#include "Cryption.hpp"
#include "../processes/Task.hpp"
#include "../fileHandling/ReadEnv.hpp"
#include <iostream>
#include <vector>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <cstdint>

// OpenSSL Headers
#include <openssl/evp.h>
#include <openssl/rand.h>

// Helper to read/write files
std::vector<uint8_t> read_file(std::fstream& fs) {
    fs >> std::noskipws;
    return {std::istream_iterator<uint8_t>(fs), std::istream_iterator<uint8_t>()};
}
void write_file(const std::string& path, const std::vector<uint8_t>& data) {
    std::ofstream os(path, std::ios::binary);
    os.write(reinterpret_cast<const char*>(data.data()), data.size());
}

// AES-256-GCM Encryption Function
void encrypt(const std::string& password, const std::vector<uint8_t>& plaintext, std::vector<uint8_t>& output) {
    const int KEY_SIZE = 32; // 256 bits
    const int IV_SIZE = 12;  // 96 bits is standard for GCM
    const int SALT_SIZE = 16;
    const int TAG_SIZE = 16; // GCM authentication tag

    std::vector<uint8_t> salt(SALT_SIZE);
    std::vector<uint8_t> iv(IV_SIZE);
    std::vector<uint8_t> key(KEY_SIZE);
    std::vector<uint8_t> tag(TAG_SIZE);
    
    // 1. Generate random salt and IV
    RAND_bytes(salt.data(), SALT_SIZE);
    RAND_bytes(iv.data(), IV_SIZE);

    // 2. Derive key from password using PBKDF2 (a standard KDF)
    PKCS5_PBKDF2_HMAC(password.c_str(), password.length(),
                      salt.data(), salt.size(), 4096, EVP_sha256(),
                      KEY_SIZE, key.data());

    std::vector<uint8_t> ciphertext(plaintext.size());
    int len = 0;

    // 3. Encrypt
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), NULL, NULL, NULL);
    EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, IV_SIZE, NULL);
    EVP_EncryptInit_ex(ctx, NULL, NULL, key.data(), iv.data());
    EVP_EncryptUpdate(ctx, ciphertext.data(), &len, plaintext.data(), plaintext.size());
    int ciphertext_len = len;
    EVP_EncryptFinal_ex(ctx, ciphertext.data() + len, &len);
    ciphertext_len += len;
    
    // 4. Get the authentication tag
    EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, TAG_SIZE, tag.data());
    EVP_CIPHER_CTX_free(ctx);

    // 5. Assemble the output: salt + iv + tag + ciphertext
    output.clear();
    output.insert(output.end(), salt.begin(), salt.end());
    output.insert(output.end(), iv.begin(), iv.end());
    output.insert(output.end(), tag.begin(), tag.end());
    output.insert(output.end(), ciphertext.begin(), ciphertext.end());
}

// AES-256-GCM Decryption Function
void decrypt(const std::string& password, const std::vector<uint8_t>& input, std::vector<uint8_t>& output) {
    const int KEY_SIZE = 32;
    const int IV_SIZE = 12;
    const int SALT_SIZE = 16;
    const int TAG_SIZE = 16;

    if (input.size() < SALT_SIZE + IV_SIZE + TAG_SIZE) {
        throw std::runtime_error("Invalid encrypted file format.");
    }
    
    // 1. Extract components from the input file
    std::vector<uint8_t> salt(input.begin(), input.begin() + SALT_SIZE);
    std::vector<uint8_t> iv(input.begin() + SALT_SIZE, input.begin() + SALT_SIZE + IV_SIZE);
    std::vector<uint8_t> tag(input.begin() + SALT_SIZE + IV_SIZE, input.begin() + SALT_SIZE + IV_SIZE + TAG_SIZE);
    std::vector<uint8_t> ciphertext(input.begin() + SALT_SIZE + IV_SIZE + TAG_SIZE, input.end());
    
    std::vector<uint8_t> key(KEY_SIZE);
    
    // 2. Re-derive the key from the password and extracted salt
    PKCS5_PBKDF2_HMAC(password.c_str(), password.length(),
                      salt.data(), salt.size(), 4096, EVP_sha256(),
                      KEY_SIZE, key.data());
                      
    output.resize(ciphertext.size());
    int len = 0;
    int plaintext_len = 0;

    // 3. Decrypt and Authenticate
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(), NULL, NULL, NULL);
    EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, IV_SIZE, NULL);
    EVP_DecryptInit_ex(ctx, NULL, NULL, key.data(), iv.data());
    EVP_DecryptUpdate(ctx, output.data(), &len, ciphertext.data(), ciphertext.size());
    plaintext_len = len;

    // 4. Set the authentication tag before finalizing
    EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, TAG_SIZE, tag.data());

    // Finalize: This step fails if the tag does not match (data was tampered with)
    if (EVP_DecryptFinal_ex(ctx, output.data() + len, &len) <= 0) {
        std::cerr << "\n\n*** AUTHENTICATION FAILED! The file is corrupt or the key is wrong. ***\n\n" << std::endl;
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("Decryption failed: data is corrupt or key is wrong.");
    }
    plaintext_len += len;
    output.resize(plaintext_len);
    EVP_CIPHER_CTX_free(ctx);
}


int executeCryption(const std::string& taskData) {
    try {
        Task task = Task::fromString(taskData);
        ReadEnv env;
        std::string password = env.getenv("CRYPTION_PASSWORD");
        if (password.empty()) {
            throw std::runtime_error("CRYPTION_PASSWORD not found in .env file");
        }
        
        std::vector<uint8_t> file_content = read_file(task.f_stream);
        std::vector<uint8_t> processed_content;

        if (task.action == Action::ENCRYPT) {
            encrypt(password, file_content, processed_content);
        } else {
            decrypt(password, file_content, processed_content);
        }
        
        write_file(task.filePath, processed_content);

    } catch (const std::exception& e) {
        std::cerr << "An error occurred: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}