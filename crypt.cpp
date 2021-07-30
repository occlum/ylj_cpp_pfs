#include <openssl/aes.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <sstream>
#include <vector>
#include <iostream>
#include <chrono>

using std::string;
using std::vector;
using std::cout;
using std::endl;
using namespace std;
using namespace chrono;


EVP_CIPHER_CTX * e_ctx = nullptr;
EVP_CIPHER_CTX *d_ctx = nullptr;

void aes_init()
{
	static int init = 0;
	if (init == 0)
	{
		e_ctx = EVP_CIPHER_CTX_new();
		d_ctx = EVP_CIPHER_CTX_new();

		//initialize openssl ciphers
		OpenSSL_add_all_ciphers();

		//initialize random number generator (for IVs)
		int rv = RAND_load_file("/dev/urandom", 32);

		init++;
	}
	
}

void aes_release()
{
	EVP_CIPHER_CTX_free(e_ctx);
    EVP_CIPHER_CTX_free(d_ctx);
}

void aes_128_gcm_encrypt0(const char* plaintext, size_t plaintext_len,
				const char * key,
				unsigned char ** out, size_t & out_len) {

	size_t output_length = AES_BLOCK_SIZE + AES_BLOCK_SIZE + plaintext_len;
	unsigned char *output = (unsigned char*)malloc(output_length);
	for (int i = 0; i < output_length; i++)
		output[i] = 0;
	//RAND_bytes(output+16, 16);

	int actual_size = 0, final_size = 0;
	
	EVP_EncryptInit(e_ctx, EVP_aes_128_gcm(), (const unsigned char*)key, output + 16);
	EVP_EncryptUpdate(e_ctx, &output[32], &actual_size, (const unsigned char*)plaintext, plaintext_len);
	EVP_EncryptFinal(e_ctx, &output[32 + actual_size], &final_size);
	EVP_CIPHER_CTX_ctrl(e_ctx, EVP_CTRL_GCM_GET_TAG, 16, output);
	*out = output;
	out_len = output_length;
}

void encrypt0(char *text, size_t text_len, char **cipher_text, char *mac)
{
	char key[16];
	for (int i = 0; i < 16; i++)
		key[i] = 0;
	size_t out_len;
	char* _cipher;
	aes_128_gcm_encrypt0(text, text_len, key, (unsigned char**)&_cipher, out_len);
	memcpy(mac, _cipher, 16);
	memcpy(*cipher_text, _cipher + 32, text_len);
	free(_cipher);
}

void encrypt(char *text, size_t text_len, char **cipher_text, char *key, char *mac)
{
	RAND_bytes((unsigned char*)key, 4);
	for (int i = 4; i < 16; i++)
		key[i] = 0;
	size_t out_len;
	char* _cipher;
	aes_128_gcm_encrypt0(text, text_len, key, (unsigned char**)&_cipher, out_len);
	memcpy(mac, _cipher, 16);
	memcpy(*cipher_text, _cipher + 32, text_len);
	free(_cipher);
}

int aes_128_gcm_decrypt0(const char* ciphertext, size_t ciphertext_len,
				const char * key,
				unsigned char ** out, size_t & out_len) {

	unsigned char *plaintext = new unsigned char[ciphertext_len - 32];

	int actual_size = 0, final_size = 0;
	EVP_DecryptInit(d_ctx, EVP_aes_128_gcm(), (const unsigned char*)key, (unsigned char*)ciphertext+16);
	EVP_DecryptUpdate(d_ctx, plaintext, &actual_size, (const unsigned char*)&ciphertext[32], ciphertext_len - 32);
	EVP_CIPHER_CTX_ctrl(d_ctx, EVP_CTRL_GCM_SET_TAG, 16, (char*)ciphertext);
	int ret = EVP_DecryptFinal(d_ctx, &plaintext[actual_size], &final_size);

	*out = plaintext;
	out_len = ciphertext_len - 32;
	return ret;
}

int decrypt(char **text, size_t text_len, char *cipher_text, char *key, char *mac)
{
	size_t out_len;
	char* _cipher = (char *)malloc(text_len + 32);
	memcpy(_cipher, mac, 16);
	for (int i = 0; i < 16; i++)
		_cipher[i + 16] = 0;
	memcpy(_cipher + 32, cipher_text, text_len);
	int ret = aes_128_gcm_decrypt0(_cipher, text_len + 32, key, (unsigned char**)text, out_len);
	free(_cipher);
	return ret;
}
