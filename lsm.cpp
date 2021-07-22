#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <map>
#include <iostream>
#include <algorithm>
#include "disk.h"
#include "fs.h"
#include "lsm.h"
#include "crypt.h"
#include "mhbt.h"

using namespace std;

map<int, lsm_kv> lsm_C0;
int *sst_file_count;
int *sst_file_size;

void lsm_init(int *_sst_file_count, int *_sst_file_size)
{
	sst_file_count = _sst_file_count;
	sst_file_size = _sst_file_size;
	//printf("sst_count %d %d\n", *sst_file_count, sst_file_count);
	lsm_C0.clear();
}

int sst_find(int num, int lba, lsm_kv *value)
{
	char sst_file_name[20];
	snprintf(sst_file_name, sizeof(sst_file_name), "sst_%d", num);

	int head = 0, tail = sst_file_size[num] - 1;
	while (head < tail) {
		int mid = (head + tail) / 2;
		sst_read_index(sst_file_name, mid, (char *)value);
		if (value->lba < lba)
			head = mid + 1;
		else
			tail = mid;
	}
	sst_read_index(sst_file_name, head, (char *)value);
	if (value->lba == lba) {
		printf("sst find\n");
		return 1;
	}
	else 
		return 0;

}


int small_sst_find(int num, int lba, lsm_kv *value)
{
	char sst_file_name[20];
	snprintf(sst_file_name, sizeof(sst_file_name), "sst_%d", num);

	char buf[5120];
//	int ret = file_read(sst_file_name, 0, BLOCK_SIZE, buf);
	int ret = small_sst_read(sst_file_name, sst_file_size[num], buf);
	if (ret < 0)
		return -1;

	sst sst_file;
	sst_file.kv_array = (lsm_kv*)buf;
	sst_file.sst_size = sst_file_size[num];

	int head = 0, tail = sst_file.sst_size - 1;	
	while (head < tail) {
		int mid = (head + tail) / 2;
		if (sst_file.kv_array[mid].lba < lba)
			head = mid + 1;
		else
			tail = mid;
	}
	if (sst_file.kv_array[head].lba == lba) {
		*value = sst_file.kv_array[head];
		printf("sst find\n");
		ret = 1;
	}
	else
		ret = 0;
	return ret;
}

lsm_kv lsm_find(int lba)
{
	map<int, lsm_kv>::iterator iter;
	iter = lsm_C0.find(lba);
	if (iter != lsm_C0.end()) 
		return iter->second;
	else {
		lsm_kv kv(-1, 0, 0, 0);
		for (int i = *sst_file_count - 1; i >= 0; i--) {
//			if (sst_find(i, lba, &kv)) {
			if (read_mhbt(i, sst_file_size[i], lba, &kv)) {
				return kv;
			}
		}
		return kv;
	}
}

int C0_to_C1()
{
	sst sst_file;
	sst_file.sst_size = lsm_C0.size();
	if (sst_file.sst_size <= 0)
		return 0;
	sst_file.kv_array = new lsm_kv[sst_file.sst_size];
	map<int, lsm_kv>::iterator iter;

	int i = 0;
	for (iter = lsm_C0.begin(); iter != lsm_C0.end(); iter++) {
		sst_file.kv_array[i] = iter->second;
		i++;
	}
	lsm_C0.clear();

	char sst_name[20];
        snprintf(sst_name, sizeof(sst_name), "sst_%d", *sst_file_count);
	sst_file_size[*sst_file_count] = sst_file.sst_size;
	lsm_kv *mhbt_kv;
	int mhbt_size = transform(sst_file.kv_array, sst_file.sst_size, &mhbt_kv);
        //return file_write(sst_name, 0, sst_file.sst_size * sizeof(lsm_kv), (char *)(sst_file.kv_array));
//	return small_sst_write(sst_name, sst_file.sst_size, (char *)(sst_file.kv_array));
	sst_file_size[*sst_file_count] = mhbt_size / 16;
	printf("%d\n", mhbt_size / 16);
	(*sst_file_count)++;
	small_sst_write(sst_name, mhbt_size / 16, (char *)mhbt_kv);
	free(mhbt_kv);
}

int lsm_insert(lsm_kv kv){
	lsm_C0[kv.lba] = kv;
/*	if (lsm_C0.size() > 100)
		return C0_to_C1();*/
}

/*

int data_write(int lba, char *buf, int pba){
	int ret;
	char *data_file_name = "data";
	string plaintext = buf;
	string key = "0";

 	size_t enc_msg_len = -1;
	unsigned char *enc_msg = nullptr;
        aes_128_gcm_encrypt0(buf, 128, key.c_str(), &enc_msg, enc_msg_len);
	//printf("%d  %d\n", strlen(buf), strlen((char *)enc_msg));
	ret = data_disk_write(data_file_name, pba, (char *)enc_msg);
	lsm_kv kv(lba, -1, -1, pba);
	lsm_insert(kv);
	printf("%s\n", buf);
//	printf("xbuf %s\n", enc_msg);
	free(enc_msg);
	return ret;
}

int data_read(int lba, char *buf, int super_sst_count){
	char *data_file_name = "data";
	sst_count = super_sst_count;
	lsm_kv kv = lsm_find(lba);
	printf("pba  %d\n", kv.pba);
	data_disk_read(data_file_name, kv.pba, buf);
	for (int i = 0; i < 160; i++)
		printf("%d ", buf[i]);
	printf("\n");
	char xbuf[160];
	for (int i = 0; i < 160; i++)
		xbuf[i] = buf[i + kv.pba];

	string key = "0";
 	unsigned char * dec_msg = nullptr;
        size_t len_dec_msg = -1;
        aes_128_gcm_decrypt0(xbuf, 160, key.c_str(), &dec_msg, len_dec_msg);

	printf("len %d\n", len_dec_msg);
	vector<unsigned char> ciphertext_final;
        ciphertext_final.resize(len_dec_msg, '\0');
        std::copy(dec_msg, dec_msg + len_dec_msg, ciphertext_final.begin());

        for (int i = 0; i < len_dec_msg; i++) {
                cout << dec_msg[i];
        }
        cout << endl;

//	printf("%s\n", dec_msg);
	free(dec_msg);
	return 0;
}
*/

int data_write(int lba, char *buf, int pba){
        int ret;
        char *data_file_name = "data";
        ret = data_disk_write(data_file_name, pba, buf);
	printf("%s\n", buf);
        lsm_kv kv(lba, -1, -1, pba);
        lsm_insert(kv);
        return ret;
}

int data_read(int lba, char *buf){
        char *data_file_name = "data";
	//lsm_kv kv;
        lsm_kv kv = lsm_find(lba);
	if (kv.lba == -1) {
		printf("Block %d not found!\n", lba);
		return -1;
	}
	else {
        	return data_disk_read(data_file_name, kv.pba, buf);	
	}
}



int compaction(){          
	//to do
}
