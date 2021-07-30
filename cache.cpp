#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <hash_map>
#include <string>
#include <list>
#include "cache.h"
#include "disk.h"

using namespace std;
using namespace __gnu_cxx;

class LRUCache {  
public:
    LRUCache()
    {  
        m_capacity = 0;
    }

    LRUCache(int capacity)
    {  
        m_capacity = capacity;  
    }

    void clear()
    {
        cache_Map.clear();
        cache.clear();
    }  
  
    char *get(int key) 
    {  
        char *retValue = nullptr;
        hash_map<int, list<pair<int, char[BLOCK_SIZE]>> :: iterator> ::iterator it = cache_Map.find(key);
        if (it != cache_Map.end()) {
            retValue = it->second->second;
            list<pair<int, char[BLOCK_SIZE]>> :: iterator ptrPair = it -> second;
            pair<int, char[BLOCK_SIZE]> tmpPair = *ptrPair;
            cache.erase(ptrPair);
            cache.push_front(tmpPair);
            cache_Map[key] = cache.begin();
        }
        return retValue;          
    }
  
    char *set(int key, char *value, int &pop_key) 
    {  
        char *retValue = nullptr;
        hash_map<int, list<pair<int, char[BLOCK_SIZE]>> :: iterator> :: iterator it = cache_Map.find(key);  
        if (it != cache_Map.end()) {  
            list<pair<int, char[BLOCK_SIZE]>> :: iterator ptrPait = it ->second;
            memcpy(ptrPait->second, value, BLOCK_SIZE);
            pair<int , char[BLOCK_SIZE]> tmpPair = *ptrPait;
            cache.erase(ptrPait);
            cache.push_front(tmpPair);
            cache_Map[key] = cache.begin();  
        }
        else {
            pair<int, char[BLOCK_SIZE]> tmpPair;
            tmpPair.first = key;
            memcpy(tmpPair.second, value, BLOCK_SIZE);
  
            if (m_capacity == cache.size()) {
                int delKey = cache.back().first;
                retValue = new char[BLOCK_SIZE];
                memcpy(retValue, cache.back().second, BLOCK_SIZE);
                pop_key = delKey;
                cache.pop_back();
                hash_map<int, list<pair<int, char[BLOCK_SIZE]>> :: iterator> :: iterator delIt = cache_Map.find(delKey);
                cache_Map.erase(delIt);
            }
            cache.push_front(tmpPair);
            cache_Map[key] = cache.begin();
        }
        return retValue;
    }

    char *pop(int &lba)
    {  
        char *retValue = nullptr;
        if (cache.size() != 0) {
            lba = cache.back().first;
            retValue = new char[BLOCK_SIZE];
            memcpy(retValue, cache.back().second, BLOCK_SIZE);
            cache.pop_back();
            hash_map<int, list<pair<int, char[BLOCK_SIZE]>> :: iterator> :: iterator delIt = cache_Map.find(lba);
            cache_Map.erase(delIt);
        }
        return retValue;
    }
  
    int m_capacity;
    list<pair<int, char[BLOCK_SIZE]>> cache;
    hash_map<int, list<pair<int, char[BLOCK_SIZE]>> :: iterator> cache_Map;
};
LRUCache s[2];

int init_cache(){
    s[0].m_capacity = 100000;
    s[1].m_capacity = 100000;
    s[0].clear();
    s[1].clear();
}

char *put_to_cache(int k, char *block, int &pop_k, cache_type typ) {
    return s[typ].set(k, block, pop_k);
}

void put_to_cache(int k, char *block, cache_type typ){
    int pop_k;
    s[typ].set(k, block, pop_k);
}

char *find_in_cache(int k, cache_type typ) {
	return s[typ].get(k);
}

char *pop(int &lba, cache_type typ){
    return s[typ].pop(lba);
}