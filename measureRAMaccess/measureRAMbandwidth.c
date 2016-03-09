#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/time.h>
#include <time.h>
#include <string.h>
#include <unistd.h>

/* how many runs to average by default */
#define DEFAULT_NR_LOOPS 10

/* we have 4 tests at the moment */
#define MAX_TESTS 4

//#define DEFAULT_BLOCK_SIZE 262144

/* test types */
#define TEST_MEMCPY 0
#define TEST_DUMB 1
#define TEST_READ 3
#define TEST_WRITE 2

#define MIN_SIZE   16L*1024L*1024L
#define MAX_SIZE   1L*1024L*1024L*1024L //4 GB
FILE *fp;

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

long *make_array(unsigned long long asize)
{
    unsigned long long t;
    unsigned int long_size=sizeof(long);
    long *a;

    a=calloc(asize, long_size);

    if(NULL==a) {
        perror("Error allocating memory");
        exit(1);
    }

    /* make sure both arrays are allocated, fill with pattern */
    for(t=0; t<asize; t++) {
        a[t]=0xaa;
    }
    return a;
}

double worker(unsigned long long asize, long *a, long *b, int type)
{
    unsigned long long t;
    double starttime, endtime;
    double te;
    unsigned int long_size=sizeof(long);
    /* array size in bytes */
    unsigned long long array_bytes=asize*long_size;
    unsigned long long temp;

    unsigned long hop = getpagesize();

    if(type==TEST_MEMCPY) { /* memcpy test */
        /* timer starts */
        //gettimeofday(&starttime, NULL);
        starttime = rdtsc();
        memcpy(b, a, array_bytes);
        /* timer stops */
        //gettimeofday(&endtime, NULL);
        endtime = rdtsc();
    } else if(type==TEST_DUMB) { /* dumb test */
        //gettimeofday(&size_tarttime, NULL);
        starttime = rdtsc();
        for(t=0; t<asize; t+=32) {
            #define	DOIT(t)	b[t]= a[t];
		    DOIT(t + 0)  DOIT(t + 1)  DOIT(t + 2)  DOIT(t + 3) DOIT(t + 4) DOIT(t + 5) DOIT(t + 6)
		    DOIT(t + 7) DOIT(t + 8) DOIT(t + 9) DOIT(t + 10) DOIT(t + 11) DOIT(t + 12) DOIT(t + 13)
		    DOIT(t + 14) DOIT(t + 15) DOIT(t + 16) DOIT(t + 17) DOIT(t + 18) DOIT(t + 19)
		    DOIT(t + 20) DOIT(t + 21) DOIT(t + 22) DOIT(t + 23) DOIT(t + 24) DOIT(t + 25)
		    DOIT(t + 26)DOIT(t + 27)DOIT(t + 28)DOIT(t + 29)DOIT(t + 30) DOIT(t + 31)
        }
        //gettimeofday(&endtime, NULL);
        endtime = rdtsc();
    }
    else if(type==TEST_READ) { /* dumb test */
        //gettimeofday(&size_tarttime, NULL);
        starttime = rdtsc();
        for(t=0; t<asize; t+=32) {
            #define	DOIT(t)	temp = a[t];
		    DOIT(t + 0)  DOIT(t + 1)  DOIT(t + 2)  DOIT(t + 3) DOIT(t + 4) DOIT(t + 5) DOIT(t + 6)
		    DOIT(t + 7) DOIT(t + 8) DOIT(t + 9) DOIT(t + 10) DOIT(t + 11) DOIT(t + 12) DOIT(t + 13)
		    DOIT(t + 14) DOIT(t + 15) DOIT(t + 16) DOIT(t + 17) DOIT(t + 18) DOIT(t + 19)
		    DOIT(t + 20) DOIT(t + 21) DOIT(t + 22) DOIT(t + 23) DOIT(t + 24) DOIT(t + 25)
		    DOIT(t + 26)DOIT(t + 27)DOIT(t + 28)DOIT(t + 29)DOIT(t + 30) DOIT(t + 31)
       }
        //gettimeofday(&endtime, NULL);
        endtime = rdtsc();
    }
    else if(type==TEST_WRITE) { /* dumb test */
        //gettimeofday(&size_tarttime, NULL);
        starttime = rdtsc();
        for(t=0; t<asize; t+=32) {
            #define	DOIT(t)	a[t] = 2;
		    DOIT(t + 0)  DOIT(t + 1)  DOIT(t + 2)  DOIT(t + 3) DOIT(t + 4) DOIT(t + 5) DOIT(t + 6)
		    DOIT(t + 7) DOIT(t + 8) DOIT(t + 9) DOIT(t + 10) DOIT(t + 11) DOIT(t + 12) DOIT(t + 13)
		    DOIT(t + 14) DOIT(t + 15) DOIT(t + 16) DOIT(t + 17) DOIT(t + 18) DOIT(t + 19)
		    DOIT(t + 20) DOIT(t + 21) DOIT(t + 22) DOIT(t + 23) DOIT(t + 24) DOIT(t + 25)
		    DOIT(t + 26)DOIT(t + 27)DOIT(t + 28)DOIT(t + 29)DOIT(t + 30) DOIT(t + 31)
       }
        //gettimeofday(&endtime, NULL);
        endtime = rdtsc();
    }


    te=(double)((endtime - starttime)*0.0005*0.001*0.001);

    return te;
}

void printout(double te, double mt, int type)
{
    switch(type) {
        case TEST_MEMCPY:
            printf("Method: MEMCPY\t");
            break;
        case TEST_READ:
            printf("Method: READ\t");
            break;
        case TEST_DUMB:
            printf("Method: DUMB\t");
            break;
        case TEST_WRITE:
            printf("Method: WRITE\t");
            break;
    }
    printf("Elapsed: %.5f\t", te);
    printf("MiB: %.5f\t", mt);
    switch(type) {
        case TEST_MEMCPY:
            printf("MEMCPY: %.3f MiB/s\n", mt/te);
            fprintf(fp,"MEMCPY,%d,%.3f\r\n", (int)mt,mt/te);
            break;
        case TEST_READ:
            printf("READ: %.3f MiB/s\n", mt/te);
            fprintf(fp,"READ,%d,%.3f\r\n", (int)mt,mt/te);
            break;
        case TEST_DUMB:
            printf("Copy: %.3f MiB/s\n", mt/te);
            fprintf(fp,"Copy,%d,%.3f\r\n", (int)mt,mt/te);
            break;
        case TEST_WRITE:
            printf("WRITE: %.3f MiB/s\n", mt/te);
            fprintf(fp,"WRITE,%d,%.3f\r\n", (int)mt,mt/te);
            break;
    }

    return;
}

/* ------------------------------------------------------ */

//void do_copy(int nr_loops, int tests[], unsigned long long asize, long *a,long *b, unsigned long long block_size, double mt)
//{
//    int testno;
//    double te, te_sum; /* time elapsed */
//    int showavg=1;
//    int i;
//    /* run all tests requested, the proper number of times */
//    for(testno=0; testno<MAX_TESTS; testno++) {
//        te_sum=0;
//        if(tests[testno]) {
//            for (i=0; nr_loops==0 || i<nr_loops; i++) {
//                te=worker(asize, a, b, testno, block_size);
//                te_sum+=te;
//                printf("%d\t", i);
//                printout(te, mt, testno);
//            }
//            if(showavg) {
//                printf("AVG\t");
//                printout(te_sum/nr_loops, mt, testno);
//            }
//        }
//    }
//}
//
//void do_read(int nr_loops, int tests[], unsigned long long asize, long *a,long *b, unsigned long long block_size, double mt)
//{
//    int testno;
//    double te, te_sum; /* time elapsed */
//    int showavg=1;
//    int i;
//    long temp;
//    /* run all tests requested, the proper number of times */
//    for(testno=0; testno<MAX_TESTS; testno++) {
//        te_sum=0;
//        if(tests[testno]) {
//            for (i=0; nr_loops==0 || i<nr_loops; i++) {
//                te=worker(asize, a, b, testno, block_size);
//                te_sum+=te;
//                printf("%d\t", i);
//                printout(te, mt, testno);
//            }
//            if(showavg) {
//                printf("AVG\t");
//                printout(te_sum/nr_loops, mt, testno);
//            }
//        }
//    }
//}

int main(int argc, char **argv)
{
    unsigned int long_size=0;
    double te, te_sum; /* time elapsed */
    unsigned long long asize=0; /* array size (elements in array) */
    int i;
    long *a, *b; /* the two arrays to be copied from/to */
    int o; /* getopt options */
    unsigned long testno, sizeno;

	fp = fopen("results_mem_bw.csv", "w");
	fprintf(fp,"type,size,MiBytes\r\n");

	int sizes[7] = {0};
	sizes[0] = MIN_SIZE/sizeof(long);
	for(i=1;sizes[i-1]<MAX_SIZE/sizeof(long);i++) {
		sizes[i] = 2 * sizes[i-1];
	}

    /* how many runs to average? */
    int nr_loops=DEFAULT_NR_LOOPS;
    /* fixed memcpy block size for -t2 */
    /* show average, -a */
    int showavg=1;
    /* what tests to run (-t x) */
    int tests[MAX_TESTS];
    double mt=2; /* MiBytes transferred == array size in MiB */
    int quiet=0; /* suppress extra messages */

    tests[0]=0;
    tests[1]=0;
    tests[2]=0;
    tests[3]=0;

    /* default is to run all tests if no specific tests were requested */
    if( (tests[0]+tests[1]+tests[2]+tests[3]) == 0) {
        tests[0]=1;
        tests[1]=1;
        tests[2]=1;
        tests[3]=1;
    }

    long_size=sizeof(long); /* the size of long on this platform */

    if(!quiet) {
        printf("Long uses %d bytes. ", long_size);
    }


    /* ------------------------------------------------------ */
    if(!quiet) {
        printf("Getting down to business... Doing %d runs per test.\n", nr_loops);
        printf("Page Size %d .\n", getpagesize());
    }

    for(testno=0; testno<MAX_TESTS; testno++) {
        for(sizeno=0; sizeno<7;sizeno++){
            te_sum=0;
            asize = sizes[sizeno];
            mt = asize/1024/1024*long_size;
            if(tests[testno]) {
                for (i=0; nr_loops==0 || i<nr_loops; i++) {
                    a=make_array(asize);
                    b=make_array(asize);
                    te=worker(asize, a, b, testno);
                    free(a);
                    free(b);
                    te = te/2;
                    te_sum+=te;
                    printf("%d\t", i);
                    printout(te, mt, testno);
                }
                if(showavg) {
                    printf("AVG\t");
                    printout(te_sum/nr_loops, mt, testno);
                }
            }
        }
    }

	fclose(fp);
    return 0;
}
