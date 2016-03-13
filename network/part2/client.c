#include "netdb.h"
#include "fcntl.h"
#include "stdlib.h"
#include "stdio.h"
#include "string.h"
#include "netinet/ip.h"
#include "sys/types.h"
#include "sys/socket.h"

#define DGRAM_MAXLENGTH 1 << 15

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

	int fd;
	int bytes;
	int sockfd;
	char datagram[DGRAM_MAXLENGTH];
	const char* host;
	const char* port;
	struct addrinfo hints;
	struct addrinfo* res;
	unsigned long long t_start;
	unsigned long long t_end;
	unsigned long long t_read;

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

    if((connect(sockfd, res->ai_addr, res->ai_addrlen) ) < 0) {
		perror("connect");
		exit(EXIT_FAILURE);
	}
	if((fd = open("512M.file", O_RDONLY) ) < 0 ) {
		perror("open");
		exit(EXIT_FAILURE);
	}

	int i = 0;
	bytes = 0;
	t_start = rdtsc();	
	read(fd, datagram, DGRAM_MAXLENGTH);
	t_read = rdtsc() - t_start;
	t_start = rdtsc();
	do {
		bytes = send(sockfd, datagram, DGRAM_MAXLENGTH,MSG_DONTWAIT);
		read(fd, datagram, DGRAM_MAXLENGTH);
		i++;
	} while(bytes >= 0);
	t_end = rdtsc();
	printf("%lld\r\n", t_end - t_start - t_read*(i-3) );
	
	close(sockfd);
}
