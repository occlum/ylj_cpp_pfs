all: fs

fs: pfs.cpp fs.cpp fs.h fs_util.cpp disk.cpp disk.h lsm.cpp lsm.h crypt.cpp crypt.h mhbt.cpp mhbt.h
		g++ -g -w pfs.cpp fs.cpp disk.cpp fs_util.cpp lsm.cpp crypt.cpp mhbt.cpp -lcrypto -g -o pfs

clean:
		rm -f data sst_* meta
		rm -f pfs
