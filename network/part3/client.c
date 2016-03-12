#include <netdb.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>

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

	int sockfd;
	const char* host;
	const char* port;
	struct addrinfo hints;
	struct addrinfo* res;
	unsigned long long t_start;
	unsigned long long t_end;

	if(argc < 2) {
		fprintf(stderr, "usage: client <host ip> <port>\r\n");
		exit(EXIT_FAILURE);
	}
	host = argv[1];
	port = argv[2];

	memset(&hints, 0, sizeof(hints) );
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	getaddrinfo(host,port,&hints,&res);
    if((sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol) ) <= 0) {
		perror("socket");
		exit(EXIT_FAILURE);
	}

	t_start = rdtsc();
    if((connect(sockfd, res->ai_addr, res->ai_addrlen) ) < 0) {
		perror("connect");
		exit(EXIT_FAILURE);
	}
	t_end = rdtsc();
	printf("%llu,", t_end - t_start);
	
	t_start = rdtsc();
	close(sockfd);
	t_end = rdtsc();
	printf("%llu\r\n", t_end - t_start);
}
