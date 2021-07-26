#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <map>
#include <string>

using namespace std;

enum policy{lru, naive};

map<int, char*> cache;

int init_cache(){
    cache.clear();
}

int cache_policy(int num, int pos, int layer, policy _policy){
    if (layer > 3)
        return true;
    return false;
}

void put_to_cache(int num, int pos, int layer, char *block){
    if (cache_policy(num, pos, layer, naive)) {
        int k = pos * 100 + num;
        cache[k] = new char[512];
        memcpy(cache[k], block, 512);
    }
}

void delete_from_cache(int num, int pos){
    int k = pos * 100 + num;
    map<int, char*>::iterator iter;
    iter = cache.find(k);
	if (iter == cache.end())
        return;
    delete[] cache[k];
    cache.erase(k);
}

int in_cache(int num, int pos, char *block) {
    int k = pos * 100 + num;
	map<int, char*>::iterator iter;
    iter = cache.find(k);
	if (iter != cache.end()) {
        memcpy(block, iter->second, 512);
        return true;
    }
	return false;
}