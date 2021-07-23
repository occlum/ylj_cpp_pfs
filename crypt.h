#include <openssl/aes.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <string>
#include <sstream>
#include <vector>
#include <iostream>

#include <chrono>

void aes_init();
void aes_release();
void encrypt(char *text, size_t text_len, char *cipher_text, char *key, char *mac);
int decrypt(char **text, size_t text_len, char *cipher_text, char *key, char *mac);
void encrypt0(char *text, size_t text_len, char *cipher_text, char *mac);