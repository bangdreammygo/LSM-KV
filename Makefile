
LINK.o = $(LINK.cc)
CXXFLAGS = -std=c++20 -Wall

all: correctness persistence

correctness: kvstore.o correctness.o skiplist.o SSTable.o BloomFilter.o

persistence: kvstore.o persistence.o skiplist.o SSTable.o BloomFilter.o

clean:
	-rm -f correctness persistence skiplist SSTable BloomFilter *.o

reset:
	-rm -rf ./data/level-* ./data/vlog  

