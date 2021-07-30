enum cache_type{data_cache, mhbt_cache};

char *put_to_cache(int lba, char *block, int &pop_k, cache_type typ);
void put_to_cache(int k, char *block, cache_type typ);
char *find_in_cache(int k, cache_type typ);
int init_cache();
char *pop(int &lba, cache_type type);