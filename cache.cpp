#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <map>
#include <string>
#include <list>
#include "cache.h"
#include "disk.h"

using namespace std;

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
  
    char *get(int key, bool &flag) 
    {  
        char *retValue;
        map<int, list<pair<int, char[BLOCK_SIZE]>> :: iterator> ::iterator it = cache_Map.find(key);
        if (it != cache_Map.end()) {
            retValue = it->second->second;
            list<pair<int, char[BLOCK_SIZE]>> :: iterator ptrPair = it -> second;
            pair<int, char[BLOCK_SIZE]> tmpPair = *ptrPair;
            cache.erase(ptrPair);
            cache.push_front(tmpPair);
            cache_Map[key] = cache.begin();
            flag = true;
        }
        else 
            flag = false;
        return retValue;          
    }
  
    void set(int key, char *value) 
    {  
        map<int, list<pair<int, char[BLOCK_SIZE]>> :: iterator> :: iterator it = cache_Map.find(key);  
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
                cache.pop_back();
                map<int, list<pair<int, char[BLOCK_SIZE]>> :: iterator> :: iterator delIt = cache_Map.find(delKey);
                cache_Map.erase(delIt);
            }
            cache.push_front(tmpPair);
            cache_Map[key] = cache.begin();
        }
    }
  
    int m_capacity;
    list<pair<int, char[BLOCK_SIZE]>> cache;
    map<int, list<pair<int, char[BLOCK_SIZE]>> :: iterator> cache_Map;
};
LRUCache s[2];

int init_cache(){
    s[0].m_capacity = 25000;
    s[1].m_capacity = 25000;
    s[0].clear();
    s[1].clear();
}

void put_to_cache(int k, char *block, cache_type typ) {
    s[typ].set(k, block);
}

int in_cache(int k, char **block, cache_type typ) {
    bool flag;
	*block = s[typ].get(k, flag);
    return flag;
}