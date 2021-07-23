#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <map>
#include <string>

using namespace std;

map<int, char*> cache;

int init_cache(){
    cache.clear();
}

void put_cache(int num, int pos, char *block){
    int k = pos * 100 + num;
    cache[k] = new char[512];
    memcpy(cache[k], block, 512);
}

int in_cache(int num, int pos, char *block) {
    map<int, char*>::iterator iter;
    int k = pos * 100 + num;
	iter = cache.find(k);
	if (iter != cache.end()) {
        memcpy(block, iter->second, 512);
        return true;
    }
	return false;
}