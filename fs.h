#include <sys/time.h>
#include "disk.h"

#define MAX_BLOCK 4096
#define MAX_INODE 512
#define MAX_FILE_NAME 16
#define SMALL_FILE 5120
#define LARGE_FILE 70656
#define MAX_DIR_ENTRY BLOCK_SIZE / sizeof(DirectoryEntry)

typedef enum {file, directory} TYPE;

typedef struct {
	int freeBlockCount;
	int freeInodeCount;
	int sst_file_count;
	int data_size;
	int sst_file_size[124];
} SuperBlock;

typedef struct {
	TYPE type;
	int owner;
	int group;
	struct timeval lastAccess;
	struct timeval created;
	int size;
	int blockCount;
	int directBlock[10];
	int indirectBlock;
	int link_count; // for hardlink
	char padding[24];
} Inode; // 128 byte

typedef struct {
	char name[MAX_FILE_NAME];
	int inode;
} DirectoryEntry;

typedef struct {
	int numEntry;
	DirectoryEntry dentry[MAX_DIR_ENTRY];
	char padding[8];
} Dentry;

extern char inodeMap[MAX_INODE / 8];
extern char blockMap[MAX_BLOCK / 8];
extern SuperBlock superBlock;

int fs_mount(char *name);
int fs_umount(char *name);
int execute_command(char *comm, char *arg1, char *arg2, char *arg3, char *arg4, int numArg);
bool command(char *comm, char *comm2);
int file_read(char *name, int offset, int size, char *readbuf);
int file_write(char *name, int offset, int size, char *buf);
int file_create(char *name ,int size);
int file_remove(char *name);

void write_test(int lba);
int test_write(int round);
int test_read(int round);
