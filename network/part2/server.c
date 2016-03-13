#include "netdb.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "netinet/ip.h"
#include "time.h"

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

int main(int argc, char* argv[]) {
	int sockfd;
	int connfd;
	int addr_size;
	struct addrinfo hints;
	struct addrinfo* res;
	struct sockaddr_storage conn_addr;
	char recv_buff[IP_MAXPACKET];
	const char* port;
	
	if(argc < 2) {
		fprintf(stderr, "usage: server <port>");
		exit(EXIT_FAILURE);
	}
	port = argv[1];

	memset(&hints, 0, sizeof(hints) );
	hints.ai_family   = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags    = AI_PASSIVE;

	getaddrinfo(NULL, port, &hints, &res);
	if((sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol) ) <= 0) {
		perror("sockfd");
		exit(EXIT_FAILURE);
	}
	if(bind(sockfd, res->ai_addr, res->ai_addrlen) < 0) {
		perror("bind");
		exit(EXIT_FAILURE);
	}
	if(listen(sockfd,8) < 0) {
		perror("listen");
		exit(EXIT_FAILURE);
	}

	addr_size = sizeof(conn_addr);
	while(1) {
		if((connfd = accept(sockfd,(struct sockaddr *)&conn_addr, &addr_size) ) < 0) {
			perror("connfd");
			exit(EXIT_FAILURE);
		}
		printf("Conenction received\r\n");
		int bytes;
		while((bytes = recv(connfd, recv_buff, (struct sockaddr *)&conn_addr, &addr_size)) > 0);
	}

	exit(EXIT_SUCCESS);
}
