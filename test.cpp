#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>
#include <stdlib.h>
#include <map>
#include <time.h>
#include "fs.h"
#include "disk.h"
#include "lsm.h"
#include "crypt.h"
#include "cache.h"
#include <iostream>
#include <chrono>

using namespace std;
using namespace chrono;


int test_cache()
{
    init_cache();
    char x[512], *y;
    for (int i = 0; i < 512; i++) {
        x[i] = 'a';
    }
    put_to_cache(0, x, mhbt_cache);
    int k = in_cache(0, &y, mhbt_cache);
    printf("%s\n", y);
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
    delete [] plain_text;
	return 0;
}

int test_write(int round) {
    for (int i = 0; i < round; i++){
        int lba = rand() % (1 << 25);
        
        //printf("%d\n", lba);
        write_test(lba);
    }
}

int main(){
    srand(time(NULL));
    auto begin = std::chrono::high_resolution_clock::now();
    aes_init();
    fs_mount("meta");
    //printf("fff\n");
    int round = 20000;
	init_cache();
    test_write(round);
    auto end = std::chrono::high_resolution_clock::now();
	double elapsedTime = ((duration<double, std::milli>)(end - begin)).count();
	cout << "\ntotal elapsed time: " << elapsedTime << endl;
    double bd = round * BLOCK_SIZE / elapsedTime;
    cout << "bandwidth " << round * BLOCK_SIZE / elapsedTime << endl;
    aes_release();
    C0_to_C1();
    fs_umount("meta");
}