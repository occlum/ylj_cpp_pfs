#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "disk.h"

char disk[MAX_BLOCK][BLOCK_SIZE];
char data_disk[BLOCK_SIZE];


int disk_read(int block, char *buf)
{
	if(block < 0 || block >= MAX_BLOCK /*|| strlen(buf) < strlen(disk[block])*/) {
		printf("disk_read error\n");
		return -1;
	}
	memcpy(buf, disk[block], BLOCK_SIZE);

	return 0;
}

int disk_write(int block, char *buf)
{
	if(block < 0 || block >= MAX_BLOCK /*|| strlen(disk[block]) < strlen(buf)*/) {
		printf("disk_write error\n");
		return -1;
	}
	memcpy(disk[block], buf, BLOCK_SIZE);

	return 0;
}

int disk_mount(char *name)
{
	FILE *fp = fopen(name, "r");
	if(fp != NULL) {
		fread(disk, BLOCK_SIZE, MAX_BLOCK, fp);
		fclose(fp);
		return 1;
	}
	return 0;
}

int disk_umount(char *name)
{
	FILE *fp = fopen(name, "w");
	if(fp == NULL) {
		fprintf(stderr, "disk_umount: file open error! %s\n", name);
		return -1;
	}

	fwrite(disk, BLOCK_SIZE, MAX_BLOCK, fp);
	fclose(fp);
	return 1;
}

int data_disk_read(char *name, int block, char *buf)
{
	FILE *fp = fopen(name, "r");
	if (fp != NULL) {
		fseek(fp, block * BLOCK_SIZE, 0);
		fread(data_disk, BLOCK_SIZE, 1, fp);
		memcpy(buf, data_disk, BLOCK_SIZE);
		for (int i = 0; i < 512; i++)
			printf("%c", data_disk[i]);
		printf("\n");
		fclose(fp);
		return 1;
	}
	return 0;
}

int data_disk_write(char *name, int block, char *buf)
{
	FILE *fp = fopen(name, "a");
	if (fp != NULL) {
		memcpy(data_disk, buf, BLOCK_SIZE);
		fwrite(data_disk, BLOCK_SIZE, 1, fp);
		fclose(fp);
		return 1;
	}
	return 0;
}

int small_sst_read(char *name, int sst_size, char *buf)
{
        FILE *fp = fopen(name, "r");
        int buf_len = sst_size * 16;
        if (fp != NULL) {
                fread(buf, buf_len, 1, fp);
                fclose(fp);
                return 1;
        }
        return 0;
}

int small_sst_write(char *name, int sst_size, char *buf)
{
        FILE *fp = fopen(name, "w");
        int buf_len = sst_size * 16;
        if (fp != NULL) {
                fwrite(buf, buf_len, 1, fp);
                fclose(fp);
                return 1;
        }
        return 0;
}


int sst_read(char *name, int sst_size, char *buf)
{
	FILE *fp = fopen(name, "r");
	int buf_len = sst_size * 16;
	if (fp != NULL) {
		fread(buf, BLOCK_SIZE, buf_len / BLOCK_SIZE, fp);
		fclose(fp);
		return 1;
	}
	return 0;
}

int sst_write(char *name, int sst_size, char *buf)
{
	FILE *fp = fopen(name, "w");
	int buf_len = sst_size * 16;
	if (fp != NULL) {
		fwrite(buf, BLOCK_SIZE, buf_len / BLOCK_SIZE, fp);
		fclose(fp);
		return 1;
	}
	return 0;
}

int sst_read_index(char *name, int index, char *buf)
{
	FILE *fp = fopen(name, "r");
	if (fp != NULL) {
		fseek(fp, index * 16, 0);
		fread(buf, 16, 1, fp);
		fclose(fp);
		return 1;
	}
	return 0;
}

int sst_read_pos(int num, int pos, char *buf)
{
	char name[20];
        snprintf(name, sizeof(name), "sst_%d", num);
        FILE *fp = fopen(name, "r");
        if (fp != NULL) {
                fseek(fp, pos * 512, 0);
                fread(buf, 512, 1, fp);
                fclose(fp);
                return 1;
        }
        return 0;
}

