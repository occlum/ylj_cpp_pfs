int transform(lsm_kv *lv, int size, lsm_kv **mhbtv);
int read_mhbt(int num, int pos, int lba, lsm_kv *value);
void mhbt_write(lsm_kv *lv, int size, vector<char> &output);