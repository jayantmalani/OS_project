#include "netdb.h"
#include "linux/if_packet.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "unistd.h"
#include "sys/types.h"
#include "netinet/ip.h"
#include "sys/socket.h"

#define BUFSIZE    1024

int main(int argc, char* argv[]) {
	int connfd;
	int sockfd;
	int addr_size;
	unsigned char recvBuff[BUFSIZE];
	struct sockaddr_in saddr;
	struct sockaddr_storage conn_addr;
	struct iphdr *iph;

	if((sockfd = socket(AF_INET, SOCK_STREAM, PF_UNSPEC) ) < 0) {
		perror("socket");
		exit(EXIT_FAILURE);
	}

	memset(&saddr,0,sizeof(struct sockaddr_in) );
	saddr.sin_family = AF_INET;
	saddr.sin_port = htons(atoi(argv[1]) );
	saddr.sin_addr.s_addr = inet_addr("127.0.0.1");
	saddr.sin_addr.s_addr = INADDR_ANY;

	addr_size = sizeof(saddr);
	if((bind(sockfd, (struct sockaddr*)&saddr, addr_size) ) < 0) {
		perror("bind");
		exit(EXIT_FAILURE);
	}
	if((listen(sockfd, 8) ) == -1) {
		perror("listen");
		exit(EXIT_FAILURE);
	}
	
	int bytesRecv = 0;
	addr_size = sizeof(connfd);
	connfd = accept(sockfd, (struct sockaddr*)&connfd, &addr_size);
	printf("connected\r\n");
	close(connfd);
	close(sockfd);
	return 0;
}
