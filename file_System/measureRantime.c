#define _GNU_SOURCE
#include "fcntl.h"
#include "stdio.h"
#include "stdlib.h"
#include "sys/mman.h"
#include "unistd.h"
#include "time.h"
#include "string.h"
#include <sys/stat.h>

#define FOUT   "resultsRanRead.csv"
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


unsigned long long getReadTime(char* filename, void* buf, size_t size) {
	unsigned long long start, end, diff;

	int fd = open(filename, O_RDONLY);

	start = rdtsc();
	read(fd, buf, size);
	end = rdtsc();

	close(fd);

	diff = end - start;
	return diff;
}

/*
	Source: http://stackoverflow.com/questions/2509679/how-to-generate-a-random-number-from-within-a-range
*/
long random_at_most(long max) {
  unsigned long
    // max <= RAND_MAX < ULONG_MAX, so this is okay.
    num_bins = (unsigned long) max + 1,
    num_rand = (unsigned long) RAND_MAX + 1,
    bin_size = num_rand / num_bins,
    defect   = num_rand % num_bins;

  long x;
  // This is carefully written not to overflow
  while (num_rand - defect <= (unsigned long)(x = random()));

  // Truncated division is intentional
  return x/bin_size;
}

int main(int argc, char *argv[]) {
    char* inFile;
	FILE* fout;

    int ITR;
    if (argc)
        inFile = argv[1];
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
    void *buf = malloc(size+pagesize);
    posix_memalign(&buf,pagesize,size);
    if (buf == NULL)
    {
        printf("....memory not allocated.....");
        exit(1);
    }

    unsigned long long start,end,diff;
    int i,r;
    diff = 0;

    for(i = 0; i < ITR; i++) {
        //long offset = random_at_most(size - blockSize);
        long offset = random_at_most(ITR);
        lseek(fd, offset*blockSize, SEEK_SET);
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
    close(fd);
    free(buf);
	fclose(fout);
    return 0;
}
