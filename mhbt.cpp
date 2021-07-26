#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include "disk.h"
#include "lsm.h"
#include "crypt.h"
#include "fs.h"
#include "cache.h"

using namespace std;


int transform(lsm_kv *lv, int size, lsm_kv **mhbtv) {
	vector<lsm_kv> q[6];
	vector<lsm_kv> m;
	for (int i = 0; i < 6; i++)
		q[i].clear();
	m.clear();

	int pos = 0;
	int len = BLOCK_SIZE / 16;

	for (int i = 0; i < size; i++) {
		q[0].push_back(lv[i]);

		int j = 0;
		while (q[j].size() == len){
			int key = -1, mac = -1;
			//getkey(&q[j][0], len, key, mac);
			for (int k = 0; k < 32; k++)
				m.push_back(q[j][k]);
			q[j + 1].push_back(lsm_kv(q[j][0].lba, key, mac, pos));
			pos++;
			q[j].clear();
			j++;
		}
	}

	for (int j = 0; j < 6; j++)
		if (q[j].size() != 0 ){
		        int key = -1, mac = -1;
			int padlen = len - q[j].size();
			for (int i = 0; i < padlen; i++)
				q[j].push_back(lsm_kv(-1, -1, -1, -1));
				//getkey(&q[j][0], q[j].size, key, mac);
			if (j != 5){
                q[j + 1].push_back(lsm_kv(q[j][0].lba, key, mac, pos));
			}
			for (int k = 0; k < 32; k++)
				m.push_back(q[j][k]);
                pos++;
                q[j].clear();
		}

	int mhbt_size = sizeof(lsm_kv) * m.size();
	*mhbtv = new lsm_kv[m.size()];
	memcpy(*mhbtv, &m[0], sizeof(lsm_kv) * m.size());
	return mhbt_size;
}

void mhbt_write(lsm_kv *lv, int size, vector<char> &output) {
	vector<lsm_kv> q[6];
	for (int i = 0; i < 6; i++)
		q[i].clear();
	output.clear();
	int pos = 0;
	int len = BLOCK_SIZE / 16;

	for (int i = 0; i < size; i++) {
		q[0].push_back(lv[i]);

		int j = 0;
		while (q[j].size() == len){
			int _key = -1, _mac = -1;

			char plain_text[512];
			memcpy(plain_text, &q[j][0], 512);
			char key[16];
			char mac[16];
			char cipher_text[512];
			encrypt(plain_text, 512, cipher_text, key, mac);
			for (int i = 0; i < 512; i++)
				output.push_back(cipher_text[i]);
			memcpy(&_key, key, 4);

			q[j + 1].push_back(lsm_kv(q[j][0].lba, _key, _mac, pos));
			pos++;
			q[j].clear();
			j++;
		}
	}

	for (int j = 0; j < 6; j++) {
		int _key = -1, _mac = -1;

		int padlen = len - q[j].size();
		for (int i = 0; i < padlen; i++)
			q[j].push_back(lsm_kv(-1, -1, -1, -1));
			
		char plain_text[512];
		memcpy(plain_text, &q[j][0], 512);
		char key[16];
		char mac[16];
		char cipher_text[512];
		if (j != 5)
			encrypt(plain_text, 512, cipher_text, key, mac);
		else {
			encrypt0(plain_text, 512, cipher_text, mac);
		}
		for (int i = 0; i < 512; i++)
			output.push_back(cipher_text[i]);

		memcpy(&_key, key, 4);

		if (j != 5){
            q[j + 1].push_back(lsm_kv(q[j][0].lba, _key, _mac, pos));
		}

        pos++;
        q[j].clear();
	}
}

int read_mhbt(int num, int pos, int lba, lsm_kv *value){
	int j;
	lsm_kv *kv;
	pos = pos / 32 - 1;
	int _key = 0;
	char *plain_text= new char[512];
	for (int layer = 5; layer >= 0; layer--) {
		if (!in_cache(num, pos, (char *)kv)){
			char buf[512];
			sst_read_pos(num, pos, buf);
			char key[16];
			char mac[16];
			memcpy(key, &_key, 4);
			for (int i = 4; i < 16; i++)
				key[i] = 0;
			decrypt(&plain_text, 512, buf, key, mac);
			kv = (lsm_kv*)plain_text;
			put_to_cache(num, pos, layer, (char *)kv);
		}
		else{
			printf("sst block find in cache!\n");
		}
	
		for (j = 0; j < 32; j++)
			if (kv[j].lba > lba || kv[j].lba == -1)
				break;
		if (j == 0) {
			//printf("Not found in mhbt\n");
			return 0;
		}
		pos = kv[j - 1].pba;
		_key = kv[j - 1].key;
		
		if (layer == 0){
			if (kv[j - 1].lba == lba) {
				*value = kv[j - 1];
				//printf("Found in mhbt\n");
				return 1;
			}
			else {
				//printf("Not found in mhbt\n");
				return 0;
			}
		}
	}
	free(plain_text);

}
