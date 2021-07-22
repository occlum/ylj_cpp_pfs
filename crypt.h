#include <openssl/aes.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <string>
#include <sstream>
#include <vector>
#include <iostream>

#include <chrono>

void aes_init();
std::vector<unsigned char> aes_128_gcm_encrypt(std::string plaintext, std::string key);
std::string aes_128_gcm_decrypt(std::vector<unsigned char> ciphertext, std::string key);
void aes_128_gcm_encrypt0(const char* plaintext, size_t plaintext_len,
                                const char * key,
                                unsigned char ** out, size_t & out_len);
void aes_128_gcm_decrypt0(const char* ciphertext, size_t ciphertext_len,
                                const char * key,
                                unsigned char ** out, size_t & out_len);
