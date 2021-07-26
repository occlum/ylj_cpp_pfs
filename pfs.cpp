#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>
#include <stdlib.h>
#include <map>
#include "fs.h"
#include "disk.h"
#include "lsm.h"
#include "crypt.h"
#include "cache.h"

using namespace std;

int main(int argc, char **argv)
{
	char input[64 + 16 + 16 + 16 + LARGE_FILE];
	char comm[64], arg1[16], arg2[16], arg3[16], arg4[LARGE_FILE];
		
	srand(time(NULL));
	fs_mount("meta");
	printf("%% ");
	aes_init();
	init_cache();
	char *c = new char(BLOCK_SIZE);
	while(fgets(input, 256, stdin)) {
		bzero(comm,64); 
		bzero(arg1,16); 
		bzero(arg2,16); 
		bzero(arg3,16); 
		bzero(arg4, LARGE_FILE);
		int numArg = sscanf(input, "%s %s %s %s %s", comm, arg1, arg2, arg3, arg4);
		if(command(comm, "fquit")) {
			execute_command("C0_to_C1", arg1, arg2, arg3, arg4, numArg - 1);
			break;
		}
		else {
			execute_command(comm, arg1, arg2, arg3, arg4, numArg - 1);
		}
		printf("%% ");
	}
		
	fs_umount("meta");
	return 0;
}