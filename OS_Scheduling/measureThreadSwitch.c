#include "pthread.h"
#include "stdio.h"
#include "stdlib.h"
#include "unistd.h"
#include "sys/types.h"
#include "sys/wait.h"
#include "signal.h"

#define T_COUNT 10

#if defined(__i386__)
static __inline__ unsigned long long rdtsc(void) {
	unsigned long long int x;
	__asm__ volatile(".byte 0x0f, 0x31" : "=A" (x))
	return x;
}
#elif defined(__x86_64__)

static __inline__ unsigned long long rdtsc(void) {
	unsigned hi, lo;
	__asm__ __volatile__("rdtsc" : "=a"(lo), "=d"(hi));
	return ( (unsigned long long)lo)|( ( (unsigned long long)hi)<<32);
}

unsigned long long measureOverhead(void) {
	unsigned long long start, end;
	unsigned long long totalCycles = 0;

	int i = 0;
	while(i < 1000) {
		start = rdtsc();
		totalCycles += (rdtsc() - start);
		i++;
	}
	return totalCycles / 1000;

}

unsigned long long f0(void) {	
	unsigned long long end = rdtsc();
	return end;	
}

unsigned long long f1(int a) {	
	unsigned long long end = rdtsc();
	return end + a;	
}

unsigned long long f2(int a, int b) {	
	unsigned long long end = rdtsc();
	return end + a + b;	
}

void *printMessage(void *tid) {
	pthread_exit(NULL);
}

int main(int argc, char *argv[]) {

	unsigned long long start, end, overhead;


	pthread_t warmUpT[T_COUNT];
	pthread_t measureT;
	void *v;
	
	int i = 0;
	while(i < 10) {
		pthread_create(&warmUpT[i], NULL, printMessage, v);
		i++;
	}
		
	i = 0;
	while(i < 10) {
		pthread_join(warmUpT[i], NULL);
		i++;
	}
		
	start = rdtsc();
	pthread_create(&measureT, NULL, printMessage, v);
	overhead = rdtsc() - start;
	printf("%llu\r\n", overhead);
	pthread_exit(NULL);

	usleep(1000);
	return 0;
}

#endif
