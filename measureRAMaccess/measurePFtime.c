#include "fcntl.h"
#include "stdio.h"
#include "stdlib.h"
#include "sys/mman.h"
#include "unistd.h"
#include "time.h"
#include "string.h"

#define FOUT   "resultsPFt.csv"
#define WARMUP 3
#define ITER   10

#if defined(__i386__)
static __attribute__((always_inline)) unsigned long long rdtsc(void) {
	unsigned long long int x;
	__asm__ volatile(".byte 0x0f, 0x31" : "=A" (x))
	return x;
}

#elif defined(__x86_64__)
static __attribute__((always_inline)) unsigned long long rdtsc(void) {
	unsigned hi, lo;
	__asm__ __volatile__("rdtsc" : "=a"(lo), "=d"(hi));
	return ( (unsigned long long)lo)|( ( (unsigned long long)hi)<<32);
}
#endif

int main(int argc, char *argv[]) {

	char* inFile;

	int i,j,r,npages,pageSize,fin;
	char c;
	char* p;
	FILE* fout;
	void* addr;
	struct stat st;
	long long duration;
	unsigned long long start;

	inFile = argv[1];
	if (stat(inFile,&st) == -1) {
		perror("stat");
		exit(0);
	}
	fin = open(inFile,O_RDONLY);
	fout = fopen(FOUT,"a+");

	pageSize = getpagesize();
	npages = st.st_size / pageSize;

	// Warm up the memory
	addr = mmap(0, st.st_size, PROT_READ, MAP_SHARED, fin, 0);
	for(i=0;i<WARMUP;i++) {
		for(j=0;j<npages;j++) {
			srand(time(NULL)+i+j );
			r = rand() % st.st_size;
			p = (long*)addr;
			c = p[r];
			munmap(addr, st.st_size);
			p = NULL;
			addr = mmap(0, st.st_size, PROT_READ, MAP_SHARED, fin, 0);
			msync(addr, st.st_size, MS_INVALIDATE);
		}
	}

	// Teh main event
	for(i=0;i<ITER;i++) {
		for(j=0;j<npages;j++) {

			srand(time(NULL)+i+j );
			r = rand() % st.st_size;

			p = (long*)addr;
			start = rdtsc();
			c = p[r];
			duration = rdtsc() - start;
			fprintf(fout,"%s,%d,%d,%lld\r\n",argv[1],i,r/pageSize,duration);
			munmap(addr, st.st_size);
			p = NULL;
			addr = mmap(0, st.st_size, PROT_READ, MAP_SHARED, fin, 0);
			msync(addr, st.st_size, MS_INVALIDATE);
		}
	}
	munmap(addr, st.st_size);
	close(fin);
	fclose(fout);
}
