#include <openssl/aes.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
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

void aes_128_gcm_encrypt0(const char* plaintext, size_t plaintext_len,
				const char * key,
				unsigned char ** out, size_t & out_len) {

	size_t output_length = AES_BLOCK_SIZE + AES_BLOCK_SIZE + plaintext_len;
//	unsigned char * output = (unsigned char*)malloc(output_length);
	unsigned char *output = new unsigned char[output_length];

	RAND_bytes(output+16, 16);

	int actual_size = 0, final_size = 0;
	
	EVP_EncryptInit(e_ctx, EVP_aes_128_gcm(), (const unsigned char*)key, output + 16);
	EVP_EncryptUpdate(e_ctx, &output[32], &actual_size, (const unsigned char*)plaintext, plaintext_len);
	EVP_EncryptFinal(e_ctx, &output[32 + actual_size], &final_size);
	EVP_CIPHER_CTX_ctrl(e_ctx, EVP_CTRL_GCM_GET_TAG, 16, output);
	
	*out = output;
	out_len = output_length;
}

std::vector<unsigned char> aes_128_gcm_encrypt(std::string plaintext, std::string key){
	size_t enc_length = plaintext.length() * 3;
	std::vector<unsigned char> output;
	output.resize(enc_length, '\0');

	unsigned char tag[AES_BLOCK_SIZE];
	unsigned char iv[AES_BLOCK_SIZE];
	RAND_bytes(iv, sizeof(iv));
	std::copy(iv, iv + 16, output.begin() + 16);

	int actual_size = 0, final_size = 0;
	EVP_CIPHER_CTX* e_ctx = EVP_CIPHER_CTX_new();
	//EVP_CIPHER_CTX_ctrl(e_ctx, EVP_CTRL_GCM_SET_IVLEN, 16, NULL);
	EVP_EncryptInit(e_ctx, EVP_aes_128_gcm(), (const unsigned char*)key.c_str(), iv);
	EVP_EncryptUpdate(e_ctx, &output[32], &actual_size, (const unsigned char*)plaintext.data(), plaintext.length());
	EVP_EncryptFinal(e_ctx, &output[32 + actual_size], &final_size);
	EVP_CIPHER_CTX_ctrl(e_ctx, EVP_CTRL_GCM_GET_TAG, 16, tag);
	std::copy(tag, tag + 16, output.begin());
	std::copy(iv, iv + 16, output.begin() + 16);
	output.resize(32 + actual_size + final_size);
	EVP_CIPHER_CTX_free(e_ctx);
	return output;
}

void aes_128_gcm_decrypt0(const char* ciphertext, size_t ciphertext_len,
				const char * key,
				unsigned char ** out, size_t & out_len) {

//	unsigned char *plaintext = (unsigned char*)malloc(ciphertext_len - 32);
	unsigned char *plaintext = new unsigned char[ciphertext_len - 32];

	int actual_size = 0, final_size = 0;
	EVP_DecryptInit(d_ctx, EVP_aes_128_gcm(), (const unsigned char*)key, (unsigned char*)ciphertext+16);
	EVP_DecryptUpdate(d_ctx, plaintext, &actual_size, (const unsigned char*)&ciphertext[32], ciphertext_len - 32);
	EVP_CIPHER_CTX_ctrl(d_ctx, EVP_CTRL_GCM_SET_TAG, 16, (char*)ciphertext);
	EVP_DecryptFinal(d_ctx, &plaintext[actual_size], &final_size);

	*out = plaintext;
	out_len = ciphertext_len - 32;
}

std::string aes_128_gcm_decrypt(std::vector<unsigned char> ciphertext, std::string key)
{

	unsigned char tag[AES_BLOCK_SIZE];
	unsigned char iv[AES_BLOCK_SIZE];
	std::copy(ciphertext.begin(), ciphertext.begin() + 16, tag);
	std::copy(ciphertext.begin() + 16, ciphertext.begin() + 32, iv);
	std::vector<unsigned char> plaintext; 
	plaintext.resize(ciphertext.size(), '\0');

	int actual_size = 0, final_size = 0;
	EVP_CIPHER_CTX *d_ctx = EVP_CIPHER_CTX_new();
	EVP_DecryptInit(d_ctx, EVP_aes_128_gcm(), (const unsigned char*)key.c_str(), iv);
	EVP_DecryptUpdate(d_ctx, &plaintext[0], &actual_size, &ciphertext[32], ciphertext.size() - 32);
	EVP_CIPHER_CTX_ctrl(d_ctx, EVP_CTRL_GCM_SET_TAG, 16, tag);
	EVP_DecryptFinal(d_ctx, &plaintext[actual_size], &final_size);
	EVP_CIPHER_CTX_free(d_ctx);
	plaintext.resize(actual_size + final_size, '\0');

	return string(plaintext.begin(), plaintext.end());
}

void test() {
	aes_init();

	//create a sample key
	unsigned char key_bytes[16];
	RAND_bytes(key_bytes, sizeof(key_bytes));
	string key = string((char *)key_bytes, sizeof(key_bytes));

	//text to encrypt
	string plaintext = "elephants in space dsfsdfgdgdliangyihuaifgf";
	cout << plaintext << endl;

	//encrypt
	//vector<unsigned char> ciphertext = aes_128_gcm_encrypt(plaintext, key);
	unsigned char * enc_msg = nullptr;
	size_t enc_msg_len = -1;
	aes_128_gcm_encrypt0(plaintext.c_str(), plaintext.length(), key.c_str(), &enc_msg, enc_msg_len);

	vector<unsigned char> ciphertext;
	ciphertext.resize(enc_msg_len, '\0');
	std::copy(enc_msg, enc_msg + enc_msg_len, ciphertext.begin());

	//output
	static const char *chars = "0123456789ABCDEF";
	for (int i = 0; i < ciphertext.size(); i++)
	{
		cout << chars[ciphertext[i] / 16];
		cout << chars[ciphertext[i] % 16];
	}
	cout << endl;

	//decrypt
	//string out = aes_128_gcm_decrypt(ciphertext, key);
	//cout << out << endl;
	unsigned char * dec_msg = nullptr;
	size_t len_dec_msg = -1;
	aes_128_gcm_decrypt0((char*)enc_msg, enc_msg_len, key.c_str(), &dec_msg, len_dec_msg);

	vector<unsigned char> ciphertext_final;
	ciphertext_final.resize(len_dec_msg, '\0');
	std::copy(dec_msg, dec_msg + len_dec_msg, ciphertext_final.begin());

	for (int i = 0; i < len_dec_msg; i++) {
		cout << dec_msg[i];
	}
	cout << endl;


	free(enc_msg);
	free(dec_msg);
}

void performanceTest() {
	aes_init();

	//create a sample key
	unsigned char key_bytes[16];
	RAND_bytes(key_bytes, sizeof(key_bytes));
	string key = string((char *)key_bytes, sizeof(key_bytes));

	//text to encrypt
	string plaintext = "elephants in space dsfsdfgdgdliangyihuaifgf";
	cout << plaintext << endl;

	const int round = 10000;
	auto begin = std::chrono::high_resolution_clock::now();
	for (int i = 0; i < round; i++) {
		//encrypt
		unsigned char * enc_msg = nullptr;
		size_t enc_msg_len = -1;
		aes_128_gcm_encrypt0(plaintext.c_str(), plaintext.length(), key.c_str(), &enc_msg, enc_msg_len);

		/*vector<unsigned char> ciphertext;
		ciphertext.resize(enc_msg_len, '\0');
		std::copy(enc_msg, enc_msg + enc_msg_len, ciphertext.begin());*/

		//output
		/*static const char *chars = "0123456789ABCDEF";
		for (int i = 0; i < ciphertext.size(); i++)
		{
			cout << chars[ciphertext[i] / 16];
			cout << chars[ciphertext[i] % 16];
		}
		cout << endl;*/

		//decrypt
		unsigned char * dec_msg = nullptr;
		size_t len_dec_msg = -1;
		aes_128_gcm_decrypt0((char*)enc_msg, enc_msg_len, key.c_str(), &dec_msg, len_dec_msg);

		/*vector<unsigned char> ciphertext_final;
		ciphertext_final.resize(len_dec_msg, '\0');
		std::copy(dec_msg, dec_msg + len_dec_msg, ciphertext_final.begin());

		for (int i = 0; i < len_dec_msg; i++) {
			cout << dec_msg[i];
		}
		cout << endl;*/


		free(enc_msg);
		free(dec_msg);
	}
	
	auto end = std::chrono::high_resolution_clock::now();
	double elapsedTime = ((duration<double, std::milli>)(end - begin)).count();
	cout << "\ntotal elapsed time: " << elapsedTime << ", ave = " << (elapsedTime / round) << endl;

	
}
/*
int main(int argc, char **argv)
{
	test();
	performanceTest();


	EVP_CIPHER_CTX_free(e_ctx);
	EVP_CIPHER_CTX_free(d_ctx);
}*/

