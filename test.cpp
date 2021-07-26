#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>
#include <stdlib.h>
#include <map>
#include "fs.h"
#include "disk.h"
#include "lsm.h"
#include "crypt.h"
#include "cache.h"

using namespace std;

int test_cache()
{
    char x[512], y[512];
    for (int i = 0; i < 512; i++)
        x[i] = i % 64;
    put_to_cache(0, 1, x);
    int k = in_cache(0, 1, y);
    delete_from_cache(0, 1);
}

int test_crypt()
{
    char key[16];
    char mac[16];
    char *text = "hello";
    char cipher_text[5];
    size_t out_len;
    aes_init();
    char *plain_text = new char[5];
    encrypt(text, 5, cipher_text, key, mac);
    decrypt(&plain_text, 5, cipher_text, key, mac);
    for (int i = 0; i < 5; i++)
        printf("%d %d\n", text[i], plain_text[i]);
    printf("%s %s\n", text, plain_text);
    free(plain_text);
	return 0;
}

int main(){
    test_cache();
}