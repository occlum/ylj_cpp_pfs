#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>
#include <stdlib.h>
#include <map>
#include <iostream>
#include <chrono>
#include "fs.h"
#include "disk.h"
#include "lsm.h"
#include "crypt.h"
#include "cache.h"

using namespace std;
using namespace chrono;


int main(){
    srand(time(NULL));
    auto begin = std::chrono::high_resolution_clock::now();
    aes_init();
    fs_mount("meta");
    int round = 600000;
	init_cache();
    //test_write(round);
    test_read(round);
    auto end = std::chrono::high_resolution_clock::now();
	double elapsedTime = ((duration<double, std::milli>)(end - begin)).count();
	cout << "\ntotal elapsed time: " << elapsedTime << endl;
    double bd = round / 1000.0 * BLOCK_SIZE / elapsedTime;
    cout << "bandwidth " << bd << endl;
    aes_release();
    fs_umount("meta");
}
