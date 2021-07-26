#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "fs.h"
#include "disk.h"
#include <string>
#include "lsm.h"

using namespace std;

char inodeMap[MAX_INODE / 8];
char blockMap[MAX_BLOCK / 8];
Inode inode[MAX_INODE];
SuperBlock superBlock;
Dentry curDir;
int curDirBlock;
int x = 0;


int rand_string(char *str, size_t size)
{
		if(size < 1) return 0;
		int n, key;
		const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
		for (n = 0; n < size; n++) {
				key = rand() % (int) (sizeof charset - 1);
				str[n] = '?';
		}
		str[size] = '\0';
		return size+1;
}

void toggle_bit(char *array, int index)
{
		array[index/8] ^= 1 << (index % 8);
}

char get_bit(char *array, int index)
{
		return 1 & (array[index/8] >> (index % 8));
}

void set_bit(char *array, int index, char value)
{
		if(value != 0 && value != 1) return;
		array[index/8] ^= 1 << (index % 8);
		if(get_bit(array, index) == value) return;
		toggle_bit(array, index);
}

int get_free_inode()
{
		int i = 0;
		for(i = 0; i < MAX_INODE; i++)
		{
				if(get_bit(inodeMap, i) == 0) {
						set_bit(inodeMap, i, 1);
						superBlock.freeInodeCount--;
						return i;
				}
		}

		return -1;
}

int get_free_block()
{
		int i = 0;
		for(i = 0; i < MAX_BLOCK; i++)
		{
				if(get_bit(blockMap, i) == 0) {
						set_bit(blockMap, i, 1);
						superBlock.freeBlockCount--;
						return i;
				}
		}

		return -1;
}

int format_timeval(struct timeval *tv, char *buf, size_t sz)
{
		ssize_t written = -1;
		struct tm *gm;
		gm = gmtime(&tv->tv_sec);

		if (gm)
		{
				written = (ssize_t)strftime(buf, sz, "%Y-%m-%d %H:%M:%S", gm);
				if ((written > 0) && ((size_t)written < sz))
				{
						int w = snprintf(buf+written, sz-(size_t)written, ".%06dZ", tv->tv_usec);
						written = (w > 0) ? written + w : -1;
				}
		}
		return written;
}

bool command(char *comm, char *comm2)
{
	if(strlen(comm) == strlen(comm2) && strncmp(comm, comm2, strlen(comm)) == 0) 
		return true;
	return false;
}

int fs_mount(char *name)
{
	int numInodeBlock =  (sizeof(Inode) * MAX_INODE) / BLOCK_SIZE;
	int i, index, inode_index = 0;

	// load superblock, inodeMap, blockMap and inodes into the memory
	if(disk_mount(name) == 1) {
		disk_read(0, (char*)&superBlock);
		disk_read(1, inodeMap);
		disk_read(2, blockMap);
		for(i = 0; i < numInodeBlock; i++){
			index = i + 3;
			disk_read(index, (char*) (inode + inode_index));
			inode_index += (BLOCK_SIZE / sizeof(Inode));
		}
		// root directory
		curDirBlock = inode[0].directBlock[0];
		disk_read(curDirBlock, (char*)&curDir);
	} else {
		// Init file system superblock, inodeMap and blockMap
		superBlock.freeBlockCount = MAX_BLOCK - (1 + 1 + 1 + numInodeBlock);
		superBlock.freeInodeCount = MAX_INODE;
		superBlock.sst_file_count = 0;
		superBlock.data_size = 0;
		//Init inodeMap
		for(i = 0; i < MAX_INODE / 8; i++){
			set_bit(inodeMap, i, 0);
		}
		//Init blockMap
		for(i = 0; i < MAX_BLOCK / 8; i++){
			if(i < (1 + 1 + 1 + numInodeBlock)) 
				set_bit(blockMap, i, 1);
			else 
				set_bit(blockMap, i, 0);
		}
		//Init root dir
		int rootInode = get_free_inode();
		curDirBlock = get_free_block();

		inode[rootInode].type =directory;
		inode[rootInode].owner = 0;
		inode[rootInode].group = 0;
		gettimeofday(&(inode[rootInode].created), NULL);
		gettimeofday(&(inode[rootInode].lastAccess), NULL);
		inode[rootInode].size = 1;
		inode[rootInode].blockCount = 1;
		inode[rootInode].directBlock[0] = curDirBlock;

		curDir.numEntry = 1;
		strncpy(curDir.dentry[0].name, ".", 1);
		curDir.dentry[0].name[1] = '\0';
		curDir.dentry[0].inode = rootInode;
		disk_write(curDirBlock, (char*)&curDir);
		if(disk_mount(name) == 1) {
			printf("hh");
		}
	}
	lsm_init(&superBlock.sst_file_count, superBlock.sst_file_size);
	return 0;
}

int fs_umount(char *name)
{
	int numInodeBlock =  (sizeof(Inode)*MAX_INODE )/ BLOCK_SIZE;
	int i, index, inode_index = 0;
	disk_write(0, (char*) &superBlock);
	disk_write(1, inodeMap);
	disk_write(2, blockMap);
	for(i = 0; i < numInodeBlock; i++) {
		index = i+3;
		disk_write(index, (char*) (inode+inode_index));
		inode_index += (BLOCK_SIZE / sizeof(Inode));
	}
	// current directory
	disk_write(curDirBlock, (char*)&curDir);

	disk_umount(name);	
}

int search_cur_dir(char *name)
{
		int i;

		for(i = 0; i < curDir.numEntry; i++)
		{
				if(command(name, curDir.dentry[i].name)) return curDir.dentry[i].inode;
		}
		return -1;
}

int file_create(char *name, int size)
{
		int i;

		if(size >= LARGE_FILE) {
				printf("Do not support files larger than %d bytes yet.\n", LARGE_FILE);
				return -1;
		}

		int inodeNum = search_cur_dir(name); 
		if(inodeNum >= 0) {
				printf("File create failed:  %s exist.\n", name);
				return -1;
		}

		if(curDir.numEntry + 1 >= (BLOCK_SIZE / sizeof(DirectoryEntry))) {
				printf("File create failed: directory is full!\n");
				return -1;
		}

		int numBlock = size / BLOCK_SIZE;
		if(size % BLOCK_SIZE > 0) numBlock++;

		if(numBlock > superBlock.freeBlockCount) {
				printf("File create failed: not enough space\n");
				return -1;
		}

		if(superBlock.freeInodeCount < 1) {
				printf("File create failed: not enough inode\n");
				return -1;
		}

		char *tmp = (char*) malloc(sizeof(int) * size);

		rand_string(tmp, size);
//		printf("rand_string =\n%s\n", tmp);
		
		// get available inode and fill it
		inodeNum = get_free_inode();
		if(inodeNum < 0) {
				printf("File_create error: not enough inode.\n");
				return -1;
		}
		
		inode[inodeNum].type = file;
		inode[inodeNum].owner = 1;
		inode[inodeNum].group = 2;
		gettimeofday(&(inode[inodeNum].created), NULL);
		gettimeofday(&(inode[inodeNum].lastAccess), NULL);
		inode[inodeNum].size = size;
		inode[inodeNum].blockCount = numBlock;
		inode[inodeNum].link_count = 1;
		
		// add a new file into the current directory entry
		strncpy(curDir.dentry[curDir.numEntry].name, name, strlen(name));
		curDir.dentry[curDir.numEntry].name[strlen(name)] = '\0';
		curDir.dentry[curDir.numEntry].inode = inodeNum;
		curDir.numEntry++;
		disk_write(curDirBlock, (char*)&curDir); // newly added by kingthousu

		// get data blocks
		if(numBlock <= 10){  
			// for small files
			for(i = 0; i < numBlock; i++)
			{
					int block = get_free_block();
					if(block == -1) {
							printf("File_create error: get_free_block failed\n");
							return -1;
					}
					inode[inodeNum].directBlock[i] = block;
					disk_write(block, tmp+(i*BLOCK_SIZE));
			}
		} else{	
			// for large files
			int block = get_free_block();
			int pos;
			inode[inodeNum].directBlock[9] = block;
			inode[inodeNum].indirectBlock = 1;
			for(i = 0; i < numBlock; i++)
			{		
					if(i < 9){
						block = get_free_block();
						if(block == -1) {
								printf("File_create error: get_free_block failed\n");
								return -1;
						}
						inode[inodeNum].directBlock[i] = block;
						disk_write(block, tmp+(i*BLOCK_SIZE));
					}else{

						block = get_free_block();
						if(block == -1) {
								printf("File_create error: get_free_block failed\n");
								return -1;
						}
						pos = (i-9)*4;
						//m_write(inode[inodeNum].directBlock[9], &block , pos);
						disk_write(block, tmp+(i*BLOCK_SIZE));
					}
					
			}
		}
//		printf("file created: %s, inode %d, size %d\n", name, inodeNum, size);
		free(tmp);
		return 0;
}
int m_file_create(char *name, char * tmp)
{
		int i;
		int inodeNum = search_cur_dir(name); 
		int size =strlen(tmp);
		if(inodeNum >= 0) {
				printf("File create failed:  %s exist.\n", name);
				return -1;
		}
		if(curDir.numEntry + 1 >= (BLOCK_SIZE / sizeof(DirectoryEntry))) {
				printf("File create failed: directory is full!\n");
				return -1;
		}
        if(size >= LARGE_FILE) {
				printf("Do not support files larger than %d bytes yet.\n", LARGE_FILE);
				return -1;
		}
		int numBlock = strlen(tmp) / BLOCK_SIZE;
		if(size % BLOCK_SIZE > 0) numBlock++;
		if(numBlock > superBlock.freeBlockCount) {
				printf("File create failed: not enough space\n");
				return -1;
		}
		if(superBlock.freeInodeCount < 1) {
				printf("File create failed: not enough inode\n");
				return -1;
		}
		// get available inode and fill it
		inodeNum = get_free_inode();
		if(inodeNum < 0) {
				printf("File_create error: not enough inode.\n");
				return -1;
		}	
		inode[inodeNum].type = file;
		inode[inodeNum].owner = 1;
		inode[inodeNum].group = 2;
		gettimeofday(&(inode[inodeNum].created), NULL);
		gettimeofday(&(inode[inodeNum].lastAccess), NULL);
		inode[inodeNum].size = size;
		inode[inodeNum].blockCount = numBlock;
		inode[inodeNum].link_count = 1;
		
		// add a new file into the current directory entry
		strncpy(curDir.dentry[curDir.numEntry].name, name, strlen(name));
		curDir.dentry[curDir.numEntry].name[strlen(name)] = '\0';
		curDir.dentry[curDir.numEntry].inode = inodeNum;
		curDir.numEntry++;
		disk_write(curDirBlock, (char*)&curDir); // newly added by kingthousu

		// get data blocks
		if(numBlock <= 10){
			// for small files
			for(i = 0; i < numBlock; i++)
			{
					int block = get_free_block();
					if(block == -1) {
							printf("File_create error: get_free_block failed\n");
							return -1;
					}
					inode[inodeNum].directBlock[i] = block;
					disk_write(block, tmp+(i*BLOCK_SIZE));
			}
		}else{	
			// for large files
			int block = get_free_block();
			int pos;
			inode[inodeNum].directBlock[9] = block;
			inode[inodeNum].indirectBlock = 1;
			for(i = 0; i < numBlock; i++)
			{		
					if(i < 9){
						block = get_free_block();
						if(block == -1) {
								printf("File_create error: get_free_block failed\n");
								return -1;
						}
						inode[inodeNum].directBlock[i] = block;
						disk_write(block, tmp+(i*BLOCK_SIZE));
					}else{

						block = get_free_block();
						if(block == -1) {
								printf("File_create error: get_free_block failed\n");
								return -1;
						}
						pos = (i-9)*4;
						//m_write(inode[inodeNum].directBlock[9], &block , pos);
						disk_write(block, tmp+(i*BLOCK_SIZE));
					}
					
			}
		}

		printf("file created: %s, inode %d, size %d\n", name, inodeNum, size);

		return 0;
}

int file_cat(char *name)
{
		int i;
		char buf[512];
		int inodeNum = search_cur_dir(name);
		if(inodeNum < 0) {
				printf("file cat error: file is not exist.\n");
				return -1;
		}
		if(inode[inodeNum].type != file){
				printf("file cat error: %s is not a file.\n",name);
				return -1;
		}
		if (inode[inodeNum].indirectBlock != 1){
			// for small files
			int k = 0;
			for(i = 0 ; i < inode[inodeNum].blockCount ; i++){
				disk_read(inode[inodeNum].directBlock[i], buf);
				buf[BLOCK_SIZE] ='\0';
				for (int j = 0; j < strlen(buf); j++) {
					k++;
					printf("%c", buf[j]);
					if (k == superBlock.data_size)
						break;
					if (k % 160 == 0)
						printf("\n");
				}
				if (k == superBlock.data_size)
					break;
				//printf("%s", buf);
			}
			printf("\n");
		}
		else{
			// for large files
			int block,pos;
			inode[inodeNum].indirectBlock = 1;
			int k = 0;
			for(i = 0; i < inode[inodeNum].blockCount; i++)
			{		
					if(i < 9){
						disk_read(inode[inodeNum].directBlock[i], buf);
						buf[BLOCK_SIZE] ='\0';
					//	printf("%d\n", strlen(buf));
						for (int j = 0; j < strlen(buf); j++){
							printf("%c", buf[j]);
							if (buf[j]=='\0'){
								k = 1;
								break;
							}
							if ((j + 1) % 160 == 0)
								printf("\n");
						}
						if (k > 0)
							break;
						//printf("%s\n", buf);
						
					}else{
						pos = (i-9)*4;
						//m_read(&block ,inode[inodeNum].directBlock[9], pos);
						disk_read(block, buf);
						buf[BLOCK_SIZE] ='\0';
						printf("%s", buf);
					}
			}
			printf("\n");
		}
			
}

int file_read(char *name, int offset, int size, char *readbuf)
{		
       	int i;
	int inodeNum ;
//		char readbuf[LARGE_FILE];
	printf("read\n");
	if((offset+size) >= LARGE_FILE) {
		return -1;
	}
        inodeNum = search_cur_dir(name);
	if(inodeNum < 0) {
		return -1;
	}
	if (inode[inodeNum].indirectBlock != 1){
			// for small files
		for(i = 0 ; i < inode[inodeNum].blockCount ; i++){
				
			disk_read(inode[inodeNum].directBlock[i], &readbuf[BLOCK_SIZE * i]);
				
		}
	}
	else{
			// for large files
		int block,pos;
		for(i = 0; i < inode[inodeNum].blockCount; i++) {		
			if(i < 9){
						
				disk_read(inode[inodeNum].directBlock[i], &readbuf[BLOCK_SIZE * i]);
			}
			else{
				pos = (i-9)*4;
				//m_read(&block ,inode[inodeNum].directBlock[9], pos);
				disk_read(block, &readbuf[BLOCK_SIZE * i]);
			}
		}
	}
        return 0;
}

int file_write(char *name, int offset, int size, char *buf)
{	
	x += size;	
	int i;
	char readbuf[LARGE_FILE] ;
	int inodeNum ;
	printf("file write %d %d\n", strlen(buf), size);
//	if (strlen(buf) < size ){
//		printf("Write error : input string length is less than the size arg \n");
//			return -1;
//	}
	if((offset+size) >= LARGE_FILE) {
		printf("Do not support files larger than %d bytes yet.\n", LARGE_FILE);
		return -1;
	}
		
        inodeNum = search_cur_dir(name);
	if(inodeNum < 0) {
		//printf("file write error: file is not exist.\n");
		//return -1;
		file_create(name, 4096);
       		inodeNum = search_cur_dir(name);
	}
	if (inode[inodeNum].indirectBlock != 1){
		// for small files
		for(i = 0 ; i < inode[inodeNum].blockCount ; i++){
	
			disk_read(inode[inodeNum].directBlock[i], &readbuf[BLOCK_SIZE * i]);
				
		}
	}
	else {
		// for large files
		int block, pos;
			
		for(i = 0; i < inode[inodeNum].blockCount; i++) {		
			if(i < 9){
					
				disk_read(inode[inodeNum].directBlock[i], &readbuf[BLOCK_SIZE * i]);	
			}
			else{
				pos = (i-9)*4;
				//m_read(&block ,inode[inodeNum].directBlock[9], pos);
				disk_read(block, &readbuf[BLOCK_SIZE * i]);
				
			}
		}
	}
		
	for(i = 0; i < size; i++ ) {
		 readbuf[offset + i] = buf[i];
	}
		
	if (inode[inodeNum].indirectBlock != 1){
		// for small files
		for(i = 0 ; i < inode[inodeNum].blockCount; i++) {
			disk_write(inode[inodeNum].directBlock[i], &readbuf[BLOCK_SIZE * i]);
		}
	}
	else{
		// for large files
		int block, pos;		
		for(i = 0; i < inode[inodeNum].blockCount; i++) {		
			if(i < 9){

				disk_write(inode[inodeNum].directBlock[i], &readbuf[BLOCK_SIZE * i]);
			         
			}
			else{
				pos = (i-9)*4;
				//m_read(&block ,inode[inodeNum].directBlock[9], pos);
				disk_write(block, &readbuf[BLOCK_SIZE * i]);
				
			}
		}
	}
	//alternate way
	/*file_remove(name);  
	m_file_create(name  , readbuf);*/
	
        return 0;
}

int file_stat(char *name)
{
		char timebuf[28];
		int inodeNum = search_cur_dir(name);
		if(inodeNum < 0) {
				printf("file cat error: file is not exist.\n");
				return -1;
		}

		printf("Inode = %d\n", inodeNum);
		if(inode[inodeNum].type == file) printf("type = file\n");
		else printf("type = directory\n");
		printf("owner = %d\n", inode[inodeNum].owner);
		printf("group = %d\n", inode[inodeNum].group);
		printf("size = %d\n", inode[inodeNum].size);
		printf("link_count = %d\n", inode[inodeNum].link_count);
		printf("num of block = %d\n", inode[inodeNum].blockCount);
		format_timeval(&(inode[inodeNum].created), timebuf, 28);
		printf("Created time = %s\n", timebuf);
		format_timeval(&(inode[inodeNum].lastAccess), timebuf, 28);
		printf("Last accessed time = %s\n", timebuf);
}

int file_remove(char *name)
{
		int i,fileBlock,numEntry;
                int inodeNum = search_cur_dir(name); 
		if(inodeNum >= 0) {
		
				if(inode[inodeNum].type != file){
					printf("Not a file");
					return -1;
				}
		    
		if( inode[inodeNum].link_count == 1){
	        	if(inode[inodeNum].indirectBlock != 1){
		        	fileBlock = inode[inodeNum].directBlock[0];
				for(i = 0 ; i <inode[inodeNum].blockCount   ;i++){
				    set_bit(blockMap,  fileBlock + i , 0);
				    superBlock.freeBlockCount++;
				}
				
			}else{
				fileBlock = inode[inodeNum].directBlock[0];
				for(i = 0 ; i <inode[inodeNum].blockCount   ;i++){
				    
					if(i < 9){
						set_bit(blockMap,  fileBlock + i , 0);
				    		superBlock.freeBlockCount++;
					}else{

						int pos = (i-9)*4;
						//m_read(&fileBlock ,inode[inodeNum].directBlock[9], pos);
						set_bit(blockMap,  fileBlock , 0);
				    		superBlock.freeBlockCount++;
						
					}				
				}
				
			}
			set_bit(inodeMap,inodeNum, 0);
			superBlock.freeInodeCount++;
		}else {
		      inode[inodeNum].link_count--;
		}
		
		for(i = 0 ; i <curDir.numEntry ; i++ ){ 
		        
		        if(strcmp(curDir.dentry[i].name, name )== 0){
		            numEntry = i;
		            break;	            
		        }
		         
		     }
		     for(i = numEntry ; i < curDir.numEntry-1 ; i++ ){
		            curDir.dentry[i].inode = curDir.dentry[i+1].inode;
		            strcpy(curDir.dentry[i].name, curDir.dentry[i+1].name);
		     }
		        curDir.numEntry--;
		        disk_write(curDirBlock, (char*)&curDir);
		     	
//				printf("%s file deleted sucessfully",name);		
				return 0;
		}
		else {
//		    printf(" File not exist");
			return -1;
		}	
}

int dir_make(char* name)

{
		int i;
		int DirBlock = get_free_block();
		Dentry newDir;
		int inodeNum = search_cur_dir(name); 
		if(inodeNum >= 0) {
		
				printf("Directory create failed:  %s exist.\n", name);
				return -1;
		}

		if(curDir.numEntry + 1 >= (BLOCK_SIZE / sizeof(DirectoryEntry))) {
				printf("Directory create failed: directory is full!\n");
				return -1;
		}

		if(superBlock.freeInodeCount < 1) {
				printf("Directory create failed: not enough inode\n");
				return -1;
		}
		
		inodeNum = get_free_inode();
		if(inodeNum < 0) {
				printf("Directory error: not enough inode.\n");
				return -1;
		}
		        inode[inodeNum].type =directory;
				inode[inodeNum].owner = 0;
				inode[inodeNum].group = 0;
				gettimeofday(&(inode[inodeNum].created), NULL);
				gettimeofday(&(inode[inodeNum].lastAccess), NULL);
				inode[inodeNum].size = 1;
				inode[inodeNum].blockCount = 1;
				inode[inodeNum].link_count = 1;
				inode[inodeNum].directBlock[0] = DirBlock;
				newDir.numEntry = 2;				
				strncpy(newDir.dentry[1].name, "..", 2);
				newDir.dentry[1].name[2] = '\0';
				newDir.dentry[1].inode = curDir.dentry[0].inode;				
				strncpy(newDir.dentry[0].name, ".", 1);
				newDir.dentry[0].name[1] = '\0';
				newDir.dentry[0].inode = inodeNum;
				disk_write(DirBlock, (char*)&newDir);
		
		// add a new directory into the current directory entry
		strncpy(curDir.dentry[curDir.numEntry].name, name, strlen(name));
		curDir.dentry[curDir.numEntry].name[strlen(name)] = '\0';
		curDir.dentry[curDir.numEntry].inode = inodeNum;
		curDir.numEntry++;
		disk_write(curDirBlock, (char*)&curDir);
		return 0;
	
		//printf("Not implemented yet.\n");
}

int dir_remove(char *name)
{
	int i,DirBlock,numEntry;
	if(strcmp(".",name) == 0 | strcmp("..",name)==0){
	    	printf("Cannot delete this directory \n");
	    	return -1;
	}
    int inodeNum = search_cur_dir(name); 
		if(inodeNum >= 0) {
		
				if(inode[inodeNum].type != directory){
					printf("Not a directory");
					return -1;
				}     
		        DirBlock = inode[inodeNum].directBlock[0];
				set_bit(blockMap,  DirBlock, 0);
				superBlock.freeBlockCount++;
				
				set_bit(inodeMap,inodeNum, 0);
				superBlock.freeInodeCount++;
				
		     for(i = 0 ; i <curDir.numEntry ; i++ ){ 
		        
		        if(strcmp(curDir.dentry[i].name, name )== 0){
		            numEntry = i;
		            break;
		            
		        }
		         
		     }
		     for(i = numEntry ; i < curDir.numEntry-1 ; i++ ){
		            curDir.dentry[i].inode = curDir.dentry[i+1].inode;
		            strcpy(curDir.dentry[i].name, curDir.dentry[i+1].name);
		     }
		        curDir.numEntry--;
		        disk_write(curDirBlock, (char*)&curDir);
				printf("%s directory deleted sucessfully",name);
		        			
				return 0;
		}
		else {
		    printf("found not directory");
			return -1;
		}
		
}

int dir_change(char* name)
{   
    int i,DirBlock;
    int inodeNum = search_cur_dir(name); 
		if(inodeNum >= 0) {
				if(inode[inodeNum].type != directory){
					printf("Not a directory \n");
					return -1;
				}  
		        curDirBlock = inode[inodeNum].directBlock[0];
		        disk_read(curDirBlock, (char*)&curDir);
				return 0;
		}else {
		    printf("directory not found ");
			return -1;
		}	

}

int ls()
{       
		int i;
		for(i = 0; i < curDir.numEntry; i++)
		{
				int n = curDir.dentry[i].inode;
				if(inode[n].type == file) printf("type: file, ");
				else printf("type: dir, ");
				printf("name \"%s\", inode %d, size %d byte\n", curDir.dentry[i].name, curDir.dentry[i].inode, inode[n].size);
		}
		return 0;
}

int fs_stat()
{
		printf("File System Status: \n");
		printf("#No of free blocks: %d (%d bytes), #No of free inodes: %d\n", superBlock.freeBlockCount, superBlock.freeBlockCount*512, 			superBlock.freeInodeCount);
}

int hard_link(char *src, char *dest)
{
		int i;

		int inodeNum = search_cur_dir(dest); 
		if(inodeNum >= 0) {
				printf("Hard link failed:  %s already exist.\n", dest);
				return -1;
		}
		inodeNum = search_cur_dir(src); 
		if(inodeNum < 0) {
				printf("Hard link failed:  %s not exist.\n", src);
				return -1;
		}
		if(curDir.numEntry + 1 >= (BLOCK_SIZE / sizeof(DirectoryEntry))) {
				printf("Hard link failed: directory is full!\n");
				return -1;
		}

		if(superBlock.freeInodeCount < 1) {
				printf("Hard link  failed: not enough inode\n");
				return -1;
		}

			
		// get available inode and fill it
		
		inode[inodeNum].link_count ++;
		
		// add a new file into the current directory entry
		strncpy(curDir.dentry[curDir.numEntry].name, dest, strlen(dest));
		curDir.dentry[curDir.numEntry].name[strlen(dest)] = '\0';
		curDir.dentry[curDir.numEntry].inode = inodeNum;
		printf("curdir %s, name %s\n", curDir.dentry[curDir.numEntry].name, dest);
		curDir.numEntry++;
		disk_write(curDirBlock, (char*)&curDir); // newly added by kingthousu



		printf("Hard link success: %s, inode %d\n", dest, inodeNum);

		return 0;
		//printf("Not implemented yet.\n");
}

int execute_command(char *comm, char *arg1, char *arg2, char *arg3, char *arg4, int numArg)
{
		if(command(comm, "create")) {
				if(numArg < 2) {
						printf("error: create <filename> <size>\n");
						return -1;
				}
				return file_create(arg1, atoi(arg2)); // (filename, size)
		} else if(command(comm, "cat")) {
				if(numArg < 1) {
						printf("error: cat <filename>\n");
						return -1;
				}
				return file_cat(arg1); // file_cat(filename)
		}else if(command(comm, "write")) {
				if(numArg < 4) {
						printf("error: write <filename> <offset> <size> <buf>\n");
						return -1;
				}
				return file_write(arg1, atoi(arg2), atoi(arg3), arg4); // file_write(filename, offset, size, buf);
		}else if(command(comm, "read")) {
				if(numArg < 1) {
						printf("error: read <lba>\n");
						return -1;
				}
				char buf2[5120];
				return data_read(atoi(arg1), buf2); // file_read(filename, offset, size);
		} else if (command(comm, "write_random")){
				if(numArg < 1) {
						printf("error: write_random <lba>\n");
						return -1;
				}
				char buf[5120];
				for (int i = 0; i < 128; i++)
					buf[i] = rand() % 26 + 'a';
				buf[128] = '\0';
				superBlock.data_size += 1;
				return data_write(atoi(arg1), buf, superBlock.data_size - 1);
		
		}
		else if(command(comm, "rm")) {
				if(numArg < 1) {
						printf("error: rm <filename>\n");
						return -1;
				}
				return file_remove(arg1); //(filename)
		} else if(command(comm, "mkdir")) {
				if(numArg < 1) {
						printf("error: mkdir <dirname>\n");
						return -1;
				}
				return dir_make(arg1); // (dirname)
		} else if(command(comm, "rmdir")) {
				if(numArg < 1) {
						printf("error: rmdir <dirname>\n");
						return -1;
				}
				return dir_remove(arg1); // (dirname)
		} else if(command(comm, "cd")) {
				if(numArg < 1) {
						printf("error: cd <dirname>\n");
						return -1;
				}
				return dir_change(arg1); // (dirname)
		} else if(command(comm, "ls"))  {
				return ls();
		} else if(command(comm, "stat")) {
				if(numArg < 1) {
						printf("error: stat <filename>\n");
						return -1;
				}
				return file_stat(arg1); //(filename)
		} else if(command(comm, "df")) {
				return fs_stat();
		} else if(command(comm, "ln")) {
				return hard_link(arg1, arg2); // hard link. arg1: src file or dir, arg2: destination file or dir
		} else if(command(comm, "C0_to_C1")) {
				C0_to_C1();
		} else{
				fprintf(stderr, "%s: command not found.\n", comm);
				return -1;
		}
		return 0;
}

