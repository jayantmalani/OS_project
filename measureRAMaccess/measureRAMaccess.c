#include "stdio.h"
#include "stdlib.h"

#define PROC_FREQ_GHZ 2.90
#define SAMPLE_SIZE   32
#define LOAD_COUNT    1000000
#define WARMUPS	      4

#define MIN_STRIDE 16
#define MAX_STRIDE 1024
#define MIN_SIZE   512
#define MAX_SIZE   2L*1024L*1024L*1024L //4 GB

static long* A;

static unsigned long long forloop_oh;
static unsigned long long read_oh;

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

void setupWorkload(int size, int stride) {
	int i;

	A = (long* )malloc(size * sizeof(long) );

	for(i=stride; i<size; i+=stride) {
		A[i] = (long)&A[i-stride];
	}
	A[0] = (long)&A[i-stride];

	long* p = A;
	for(i=0;i<WARMUPS;i++) {
		do {
			p = (long*)*p;
		} while (p != (long*)A);
	}
}

void runWorkload(int size, int stride) {
	if (size <= stride)
		return;

	int i,j;
	long *p;
	double avgCycles;
	long long duration;
	unsigned long long start, end;

	p = A;
	avgCycles = 0;
	for(i=0;i<SAMPLE_SIZE;i++) {
		duration = 0;
		start = rdtsc();
		for(j=0; j<LOAD_COUNT; j++) {
			p = (long*)*p;
		}
		duration = rdtsc() - start - forloop_oh;
		avgCycles += (double)(duration) / (LOAD_COUNT);
	}
	avgCycles /= SAMPLE_SIZE;
	fprintf(fp,"%ld,%ld,%lf,%lf\r\n", stride*sizeof(long),size*sizeof(long),avgCycles,avgCycles/PROC_FREQ_GHZ );
}

void calculateReadOverhead() {
	int i;
	long long unsigned start, end;

	read_oh = 0;
	for(i=0;i<1024;i++) {
		start = rdtsc();
		read_oh += rdtsc() - start;
	}
	read_oh /= 1024;
}

void calculateForLoopOverhead() {
	int i,j;
	long long unsigned start, end, duration;

	forloop_oh = 0;
	for(i=0;i<1024;i++) {
		start = rdtsc();
		for(j=0;j<LOAD_COUNT;j++);
		duration = rdtsc() - start - read_oh;
		forloop_oh += duration;
	}
	forloop_oh /= 1024;
}

int main(int argc, char *argv[]) {
	long i,j;
	unsigned long long start;

	fp = fopen("results.csv", "w");
	fprintf(fp,"stride,array.size,cycles,time(ns)\r\n");
	calculateReadOverhead();
	calculateForLoopOverhead();
	printf("***%llu***\r\n", forloop_oh);

	int strides[32] = {0};
	strides[0] = MIN_STRIDE/sizeof(long);
	for(i=1;strides[i-1]<MAX_STRIDE/sizeof(long);i++) {
		strides[i] = 2 * strides[i-1];
	}

	int sizes[32] = {0};
	sizes[0] = MIN_SIZE/sizeof(long);
	for(i=1;sizes[i-1]<MAX_SIZE/sizeof(long);i++) {
		sizes[i] = 2 * sizes[i-1];
	}

	for(i=0;strides[i]!=0;i++) {
		for(j=0;sizes[j]!=0;j++) {
			printf("[*] Running workload with stride: %ld\tsize: %f MB\r\n",
				sizeof(long)*strides[i],
				(double)(sizeof(long)*sizes[j])/(1024.*1024.) );
			setupWorkload(sizes[j],strides[i]);
			runWorkload(sizes[j], strides[i]);
			free(A);
		}
	}

	fclose(fp);
	fprintf(stdout, "\n[+] Finished. Results in \'results.csv\'\r\n");

	return 0;
}
