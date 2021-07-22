#include <stdio.h>
#include <map>

using namespace std;

struct lsm_kv{
        int lba;
        int key;
        int mac;
        int pba;

	explicit lsm_kv(){
	}

	explicit lsm_kv(int _lba, int _key, int _mac, int _pba) {
		lba = _lba;
		key = _key;
		mac = _mac;
		pba = _pba;
	}

	bool operator<(const lsm_kv& s){
		return lba < s.lba;
	}
};

struct sst{
        int sst_size;
        lsm_kv *kv_array;
};


int sst_find(char *sst_file_name, int lba, lsm_kv *value);
int C0_to_C1();
int data_read(int lba, char *buf);
int data_write(int lba, char *buf, int pba);
void lsm_init(int *_sst_file_count, int *_sst_file_size);
