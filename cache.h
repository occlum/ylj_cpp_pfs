enum policy{lru, naive};
enum cache_type{data_cache, mhbt_cache};

void put_to_cache(int k, char *block, cache_type typ);
int in_cache(int k, char **block, cache_type typ);
int init_cache();