#define _GNU_SOURCE     /* Obtain O_DIRECT definition from <fcntl.h> */
#include "stdio.h"
#include "stdlib.h"
#include "sys/mman.h"
#include <unistd.h>
#include "time.h"
#include "string.h"
#include <sys/stat.h>
#include "fcntl.h"

#define FOUT   "resultsSeqRead.csv"
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
	FILE* fout;

    int ITR;
    if (argc){
        inFile = argv[1];
        printf("Opening file %s \n",inFile);
    }
    else {
        printf("Please enter correct filename.\n");
        exit(1);
    }

    int fd = open(inFile,O_RDONLY | O_DIRECT);
    if (fd < 0) {
        printf("File error!\n");
        exit(1);
    }

	fout = fopen(FOUT,"a+");
    struct stat b;
	fstat(fd, &b);
    size_t size = b.st_size;
    blksize_t blockSize = b.st_blksize;
    ITR = size/blockSize;
    int check = posix_fadvise(fd, 0, size, POSIX_FADV_RANDOM| POSIX_FADV_DONTNEED);
    if (check)
    {
        printf("error in posix_fadvise");
        exit(1);
    }

    int pagesize=getpagesize();
    void *buf=malloc(size+pagesize);
    posix_memalign(&buf,pagesize,size);
    if (buf == NULL)
    {
        printf("....memory not allocated.....");
        exit(1);
    }
    unsigned long long start,end,diff;
    int i,r;
    diff = 0;

    printf("...Block Size %llu... size of buf %lu\n", blockSize,sizeof(buf) );

    for(i = 0; i < ITR; i++) {
        start = rdtsc();
        r = read(fd,buf,blockSize);
        end = rdtsc();
        if (r == -1)
        {
             printf("...error while reading..");
             exit(1);
        }
        fprintf(fout,"%s,%llu\r\n",inFile,end-start);
        diff += end-start;
    }
    diff = diff/ITR;
    printf("%llu\n",diff);
    printf("...Block Size %llu... size of buf %lu \n", blockSize,sizeof(buf) );
    close(fd);
    free(buf);
	fclose(fout);
    return 0;
}
