#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/uio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/syscall.h>
#include <pthread.h>
#include <linux/kernel.h>

// only works on pentium+ x86
// access the pentium cycle counter
// this routine lifted from somewhere on the Web...
void access_counter(unsigned int *hi, unsigned int *lo) {
    asm("rdtsc; movl %%edx,%0; movl %%eax,%1"   /* Read cycle counter */
	: "=r" (*hi), "=r" (*lo)                /* and move results to */
	: /* No input */                        /* the two outputs */
	: "%edx", "%eax");
}

// here's the system call we'll use
#define DO_SYSCALL syscall(SYS_getpid)

static unsigned int t9 = 0;
// calculate difference (in microseconds) between two struct timevals
// assumes difference is less than 2^32 seconds, and unsigned int is 32 bits
unsigned int timediff(struct timeval before, struct timeval after) {
  unsigned int diff;

  diff = after.tv_sec - before.tv_sec;
  diff *= 1000000;
  diff += (after.tv_usec - before.tv_usec);

  return diff;
}

// measure the system call using the cycle counter.  measures the
// difference in time between doing two system calls and doing
// one system call, to try to factor out any measurement overhead

void *newUtil(void *t)
{
	unsigned int highs;
        unsigned int *ptr;
	access_counter(&highs,t);
        ptr = (int *)(t);
	return ;
}

void *procedure_call0()
{
	int highs, lows;
//	access_counter(&highs,&lows);
	return ;
}

int procedure_call1(int a)
{
	a =a +1;
	return a ;
}

int procedure_call2(int a, int b)
{
	a = a+b;
	return a;
}

void procedure_call3(int a, int b, int c)
{
	//asm("nop");
	return;
}

void procedure_call4(int a, int b, int c, int d)
{
	return;
}

void procedure_call5(int a, int b, int c, int d, int e)
{
	return;
}

void procedure_call6(int a, int b, int c, int d, int e, int f)
{
	return;
}

void procedure_call7(int a, int b, int c, int d, int e, int f, int g)
{
	return;
}

int newFunc()
{
  unsigned int functionStart, t, functionEnd;
  int a, b;
  access_counter(&t,&functionStart);
  a = a+b;
  b = a+ b;
  access_counter(&t, &functionEnd);
  return functionEnd - functionStart;
}

void measure_cyclecounter2(float mhz) {
  unsigned int high_s, low_s, high_e, low_e;
  unsigned int high_s1, low_s1, high_e1, low_e1;
  unsigned int high_s2, low_s2, high_e2, low_e2;
  unsigned int high_s3, low_s3, high_e3, low_e3;
  unsigned int high_s4, low_s4, high_e4, low_e4;
  unsigned int high_s5, low_s5, high_e5, low_e5;
  unsigned int high_s6, low_s6, high_e6, low_e6;
  unsigned int high_s7, low_s7, high_e7, low_e7;
  unsigned int high_s0, low_s0, high_e0, low_e0;
  unsigned int passed, t, buf;
  int pipefd[2];
  size_t nbytes;
  float latency_with_read, latency_no_read;
  float latency_with_func0;
  float latency_with_func1;
  float latency_with_func2;
  float latency_with_func3;
  float latency_with_func4;
  float latency_with_func5;
  float latency_with_func6;
  float latency_with_func7;
  float latency_with_func0_in, taskCreationTime, taskCreationTime2, taskCreationTime3, contextSwitch1, taskCreationProcess;
  unsigned int beforeFunction, afterFunction, functionTime, t1, t2 = 0, t3, start1, start2, start3, start4, start5, temp;
  float baseFunctionTime;
  // warm up all the caches by exercising the functions
  access_counter(&high_s, &low_s);
  // read(5, buf, 4);
  DO_SYSCALL;
  access_counter(&high_e, &low_e);

  // now do it for real
  access_counter(&high_s, &low_s);
  DO_SYSCALL;
  access_counter(&high_e, &low_e);
  latency_with_read = ((float) (low_e - low_s) / mhz);

  access_counter(&high_s, &low_s);
  access_counter(&high_e, &low_e);
  latency_no_read = ((float) (low_e - low_s) / mhz);

  access_counter(&t1,&start1);
  if (pipe(pipefd) == -1)
    exit(EXIT_FAILURE);

  pid_t pID= fork();
  if (pID == 0)
  {
    access_counter(&t2,&start2);
    write(pipefd[1],&start2,sizeof(start2));
    close(pipefd[1]);
    close(pipefd[0]);
    _exit(EXIT_SUCCESS);
  }
  else
  {
    
    access_counter(&t3,&start3);
    wait(NULL);
    read(pipefd[0],&buf, 4); 
    close(pipefd[0]);
    close(pipefd[1]);
    contextSwitch1 = ((float) (buf - start3) / mhz);
    printf("%f \n", contextSwitch1 - latency_no_read);
  }
}

void measure_cyclecounter3(float mhz) {
  unsigned int high_s, low_s, high_e, low_e;
  unsigned int high_s1, low_s1, high_e1, low_e1;
  unsigned int high_s2, low_s2, high_e2, low_e2;
  unsigned int high_s3, low_s3, high_e3, low_e3;
  unsigned int high_s4, low_s4, high_e4, low_e4;
  unsigned int high_s5, low_s5, high_e5, low_e5;
  unsigned int high_s6, low_s6, high_e6, low_e6;
  unsigned int high_s7, low_s7, high_e7, low_e7;
  unsigned int high_s0, low_s0, high_e0, low_e0;
  unsigned int passed, t, buf;
  int pipefd[2];
  size_t nbytes;
  float latency_with_read, latency_no_read;
  float latency_with_func0;
  float latency_with_func1;
  float latency_with_func2;
  float latency_with_func3;
  float latency_with_func4;
  float latency_with_func5;
  float latency_with_func6;
  float latency_with_func7;
  float latency_with_func0_in, taskCreationTime, taskCreationTime2, taskCreationTime3, contextSwitch1;
  unsigned int beforeFunction, afterFunction, functionTime, t1, t2 = 0, t3, start1, start2, start3, start4, start5, temp;
  float baseFunctionTime;
  // warm up all the caches by exercising the functions
  access_counter(&high_s, &low_s);
  // read(5, buf, 4);
  DO_SYSCALL;
  access_counter(&high_e, &low_e);

  // now do it for real
  access_counter(&high_s, &low_s);
  DO_SYSCALL;
  access_counter(&high_e, &low_e);
  latency_with_read = ((float) (low_e - low_s) / mhz);

  access_counter(&high_s, &low_s);
  access_counter(&high_e, &low_e);
  latency_no_read = ((float) (low_e - low_s) / mhz);

     pthread_t thread;
     int rc = pthread_create(&thread,NULL,newUtil,(void *) &start5);
     access_counter(&t2, &start4);
     pthread_join(thread,NULL);
     taskCreationTime3 = ((float) (start5 - start4) / mhz);
     pthread_exit(NULL);
}



void measure_cyclecounter(float mhz) {
  unsigned int high_s, low_s, high_e, low_e;
  unsigned int high_s1, low_s1, high_e1, low_e1;
  unsigned int high_s2, low_s2, high_e2, low_e2;
  unsigned int high_s3, low_s3, high_e3, low_e3;
  unsigned int high_s4, low_s4, high_e4, low_e4;
  unsigned int high_s5, low_s5, high_e5, low_e5;
  unsigned int high_s6, low_s6, high_e6, low_e6;
  unsigned int high_s7, low_s7, high_e7, low_e7;
  unsigned int high_s0, low_s0, high_e0, low_e0;
  unsigned int passed, t;
  size_t nbytes;
  float latency_with_read, latency_no_read;
  float latency_with_func0;
  float latency_with_func1;
  float latency_with_func2;
  float latency_with_func3;
  float latency_with_func4;
  float latency_with_func5;
  float latency_with_func6;
  float latency_with_func7;
  float latency_with_func0_in, taskCreationTime, taskCreationTime2, taskCreationTime3, taskCreationProcess;
  unsigned int beforeFunction, afterFunction, functionTime, t1, t2, t3, start1, start2, start3, start4, start5, temp;
  float baseFunctionTime;
  // warm up all the caches by exercising the functions
  access_counter(&high_s, &low_s);
  // read(5, buf, 4);
  DO_SYSCALL;
  access_counter(&high_e, &low_e);

  // now do it for real
  access_counter(&high_s, &low_s);
  DO_SYSCALL;
  access_counter(&high_e, &low_e);
  latency_with_read = ((float) (low_e - low_s) / mhz);

  access_counter(&high_s, &low_s);
  access_counter(&high_e, &low_e);
  latency_no_read = ((float) (low_e - low_s) / mhz);

  // now do it for real
//  access_counter(&high_s0, &low_s0);
//  passed = procedure_call0();
//  access_counter(&high_e0, &low_e0);
//  latency_with_func0 = ((float) (low_e0 - low_s0) / mhz);
//  latency_with_func0_in = ((float) (passed - low_s0) / mhz);

  // now do it for real
  access_counter(&high_s1, &low_s1);
  procedure_call1(1);
  access_counter(&high_e1, &low_e1);
  latency_with_func1 = ((float) (low_e1 - low_s1) / mhz);

  // now do it for real
  access_counter(&high_s2, &low_s2);
  procedure_call2(1,1);
  access_counter(&high_e2, &low_e2);
  latency_with_func2 = ((float) (low_e2 - low_s2) / mhz);

  // now do it for real
  access_counter(&high_s3, &low_s3);
  procedure_call3(1,1,1);
  access_counter(&high_e3, &low_e3);
  latency_with_func3 = ((float) (low_e3 - low_s3) / mhz);

  // now do it for real
  access_counter(&high_s4, &low_s4);
  procedure_call4(1,1,1,1);
  access_counter(&high_e4, &low_e4);
  latency_with_func4 = ((float) (low_e4 - low_s4) / mhz);

  // now do it for real
  access_counter(&high_s5, &low_s5);
  procedure_call5(1,1,1,1,1);
  access_counter(&high_e5, &low_e5);
  latency_with_func5 = ((float) (low_e5 - low_s5) / mhz);

  // now do it for real
  access_counter(&high_s6, &low_s6);
  procedure_call6(1,1,1,1,1,1);
  access_counter(&high_e6, &low_e6);
  latency_with_func6 = ((float) (low_e6 - low_s6) / mhz);

  // now do it for real
  access_counter(&high_s7, &low_s7);
  procedure_call7(1,1,1,1,1,1,1);
  access_counter(&high_e7, &low_e7);
  latency_with_func7 = ((float) (low_e7 - low_s7) / mhz);

  access_counter(&t, &beforeFunction);
  functionTime = newFunc();
  access_counter(&t,  &afterFunction);
  baseFunctionTime = ((afterFunction - beforeFunction) - functionTime) / mhz;
  
  access_counter(&t1,&start1);
  pid_t pID= fork();
  if (pID == 0)
  {
    access_counter(&t2,&start2);
    // access_counter(&t3,&start3);
    taskCreationTime = ((float) (start2 - start1) / mhz);
    taskCreationProcess = taskCreationTime - latency_no_read;
  }
  else
  {
    access_counter(&t3,&start3);
    // access_counter(&t3,&start3);
    taskCreationTime2 = ((float) (start3 - start1) / mhz);
    taskCreationProcess = taskCreationTime2 - latency_no_read;
  }


  if (pID != 0)
  {
   
   pthread_t thread;
   access_counter(&t1,&start4);
   int rc = pthread_create(&thread,NULL,procedure_call0,NULL);
   access_counter(&t2,&start5);
   taskCreationTime3 = ((float) (start5 - start4) / mhz);
   printf("%f %f \n", taskCreationProcess, taskCreationTime3 - latency_no_read - (latency_with_func4 - latency_no_read));
   pthread_exit(NULL);
  }
}

// measure the system call using the cycle counter.  measures the
// difference in time between doing two*NLOOPS system calls and doing
// one*NLOOPS system calls, to try to factor out any measurement overhead

#define NUMLOOPS 10000
void measure_gettimeofday() {
  struct timeval beforeone, afterone;
  struct timeval beforetwo, aftertwo;
  int loopcount;
  unsigned int diffone, difftwo, result;

  // warm up all caches
  gettimeofday(&beforeone, NULL);
  gettimeofday(&beforetwo, NULL);
  for (loopcount = 0; loopcount < NUMLOOPS; loopcount++) {
    DO_SYSCALL;
  }
  gettimeofday(&afterone, NULL);
  gettimeofday(&aftertwo, NULL);
  
  // measure loop of one syscall
  gettimeofday(&beforeone, NULL);
  for (loopcount = 0; loopcount < NUMLOOPS; loopcount++) {
    DO_SYSCALL;
  }
  gettimeofday(&afterone, NULL);

  // measure loop of two syscalls
  gettimeofday(&beforetwo, NULL);
  for (loopcount = 0; loopcount < NUMLOOPS; loopcount++) {
    DO_SYSCALL;
    DO_SYSCALL;
  }
  gettimeofday(&aftertwo, NULL);

  diffone = timediff(beforeone, afterone);
  difftwo = timediff(beforetwo, aftertwo);

  result = difftwo - diffone;
}

void usage(void) {
  fprintf(stderr, "usage: measure_syscall cpu_mhz\n");
  fprintf(stderr, "  e.g.,  measure_syscall 2791.375\n");
  exit(-1);
}



int main(int argc, char **argv) {

  float mhz;

  if (argc < 2)
    usage();
  
  if (sscanf(argv[1], "%f", &mhz) != 1)
    usage();

  if ((mhz < 100.0)  || (mhz > 100000.0))
    usage();

  // measure usiing the cycle counter
 measure_cyclecounter(mhz);
 //measure_cyclecounter3(mhz);
 //measure_cyclecounter2(mhz);

  // measure using "gettimeofday"
 //  measure_gettimeofday();
  pthread_exit(NULL);
  return 0;
}


