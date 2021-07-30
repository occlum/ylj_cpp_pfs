all: fs test

fs: pfs.cpp fs.cpp fs.h disk.cpp disk.h lsm.cpp lsm.h crypt.cpp crypt.h mhbt.cpp mhbt.h cache.cpp cache.h
		g++ -g -w pfs.cpp fs.cpp disk.cpp lsm.cpp crypt.cpp mhbt.cpp cache.cpp -lcrypto -g -o pfs

test: test.cpp fs.cpp fs.h disk.cpp disk.h lsm.cpp lsm.h crypt.cpp crypt.h mhbt.cpp mhbt.h cache.cpp cache.h
		g++ -g -w test.cpp fs.cpp disk.cpp lsm.cpp crypt.cpp mhbt.cpp cache.cpp -lcrypto -g -o test


clean:
		rm -f data sst_* meta
		rm -f pfs test
