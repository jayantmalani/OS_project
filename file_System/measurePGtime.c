#define _GNU_SOURCE     /* Obtain O_DIRECT definition from <fcntl.h> */
#include "fcntl.h"
#include "stdio.h"
#include "stdlib.h"
#include "sys/mman.h"
#include "unistd.h"
#include "time.h"
#include "string.h"
#include <sys/stat.h>

#define FOUT   "resultsPageBuffer.csv"
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


int main(int argc, char *argv[]) {
    char* inFile;
	FILE* fout;

    if (argc)
        inFile = argv[1];
    else {
        printf("Please enter correct filename.\n");
        exit(1);
    }

    int fd = open(inFile,O_RDONLY);
    if (fd < 0) {
        printf("File error!\n");
        exit(1);
    }

	fout = fopen(FOUT,"a+");

    struct stat b;
	fstat(fd, &b);
    size_t size = b.st_size;
    int check = posix_fadvise(fd, 0, size, POSIX_FADV_SEQUENTIAL);
    if (check)
    {
        printf("error in posix_fadvise");
        exit(1);
    }

    void *buf = malloc(size);
    if (buf == NULL)
    {
        printf("....memory not allocated.....");
        exit(1);
    }

    int i,r;
	unsigned long long start, end, diff = 0,t;

    // Warming the cache
    for(i = 0; i < WARMUP; i++)
    {
        lseek(fd, 0, SEEK_SET);
     	start = rdtsc();
	    r = read(fd, buf, size);
	    end = rdtsc();
        if (r == -1)
        {
             printf("...error while reading..");
             exit(1);
        }

    }

    lseek(fd, 0, SEEK_SET);
	start = rdtsc();
    r = read(fd, buf, size);
    end = rdtsc();
    if (r == -1)
    {
        printf("...error while reading..");
        exit(1);
    }

    t = end - start;
    printf("%llu,%llu,%llu\r\n",size,t,t*4096/size);
    fprintf(fout,"%llu,%llu,%llu\r\n",size,t,t*4096/size);

//    t = getReadTime(inFile,buf,size);
//    // reading from file possibly in cache
//    for(i = 0; i < ITR; i++) {
//        t = getReadTime(inFile,buf,size);
//        printf("%llu,%llu,%llu\r\n",size,t,t*4096/size);
//        fprintf(fout,"%llu,%llu,%llu\r\n",size,t,t*4096/size);
//    }
	fclose(fout);
    close(fd);
    free(buf);
    return 0;
}
