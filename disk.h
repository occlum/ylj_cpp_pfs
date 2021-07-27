#define BLOCK_SIZE 4096
#define MAX_BLOCK 4096

extern char disk[MAX_BLOCK][BLOCK_SIZE];

//struct key_mac{
//	int key;
//	int mac;
//};

int disk_read(int block, char *buf);
int disk_write(int block, char *buf);

int disk_mount(char *name);
int disk_umount(char *name);

int data_disk_read(char *name, int block, char *buf);
int data_disk_write(char *name, int block, char *buf);

int small_sst_read(char *name, int sst_size, char *buf);
int small_sst_write(char *name, int sst_size, char *buf);
int sst_read(char *name, int sst_size, char *buf);
int sst_write(char *name, int sst_size, char *buf);
int sst_read_index(char *name, int index, char *buf);
int sst_read_pos(int num, int pos, char *buf);

