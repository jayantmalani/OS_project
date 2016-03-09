#define _GNU_SOURCE
#include "fcntl.h"
#include "stdio.h"
#include "stdlib.h"
#include "sys/mman.h"
#include "unistd.h"
#include "time.h"
#include "string.h"
#include <sys/stat.h>

#define FOUT   "resultsContentionRead.csv"
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
    char* inFile="1.dat";
	FILE* fout;

    int num_process = atoi(argv[1]);
    int seq = atoi(argv[2]);
    int k = 0;
    int index;
    int pid = 0;
    char *filenames[] = {"1.dat","2.dat","3.dat","4.dat","5.dat","6.dat","7.dat","8.dat","9.dat","10.dat"};
    int ITR;

    for(k = 1; k < num_process; k++) {
        if (pid == 0) {
            pid = fork();
            inFile = filenames[k];
            index = k;
        }
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
        if (seq == 0){
            long offset = random_at_most(ITR);
            lseek(fd, offset*blockSize, SEEK_SET);
        }
        start = rdtsc();
        r = read(fd,buf,blockSize);
        end = rdtsc();
        if (r == -1)
        {
             printf("...error while reading..");
             exit(1);
        }
        fprintf(fout,"%d,%s,%llu\r\n",index,inFile,end-start);
        diff += end-start;
    }
    diff = diff/ITR;
    printf("%llu\n",diff);
    close(fd);
    free(buf);
	fclose(fout);
    return 0;
}
