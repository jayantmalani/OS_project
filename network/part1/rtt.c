#include "stdio.h"
#include "net/if.h"
#include "stdlib.h"
#include "string.h"
#include "unistd.h"
#include "ifaddrs.h"
#include "sys/ioctl.h"
#include "netinet/ip.h"
#include "netinet/tcp.h"
#include "linux/if_ether.h"
#include "linux/if_packet.h"

#define IP4_HDRLEN 20
#define TCP_HDRLEN 20
#define ETH_HDRLEN 14
#define LOG2TRIALS 5
#define TRIALS     1 << LOG2TRIALS

#define PROBE printf("!Probe!\r\n");
#define DUMP(s, i, buf, sz)  {printf(s);                   \
                              for (i = 0; i < (sz);i++)    \
                                  printf("%02x ", 0xff & buf[i]); \
                              printf("\n");}

void createEthFrame(uint8_t*);
void createIpHeader(uint8_t*);
void sendRST(int, uint8_t*, struct sockaddr_ll*);
void getOverheads(int, uint8_t*, struct sockaddr_ll*);
void tcpHandshake(int, uint8_t*, struct sockaddr_ll*);
void createTcpHeader(uint8_t*, int, int, char, int, int);
unsigned short getChecksum(unsigned short*, int); 
static __attribute__((always_inline)) unsigned long long rdtsc(void); 

struct ticks {
	unsigned long long read;
	unsigned long long recvfrom;
	unsigned long long tcpRtt;
};

struct host {
	int ip;
	int port;
	uint8_t hwaddr[6];
};

struct host *src;
struct host *dest;
struct ticks cycles;

const int FRAME_LENGTH = 2*6 + 2 + IP4_HDRLEN + TCP_HDRLEN;

int main(int argc, char* argv[]) {
	int i;
	int sockfd;
	int FRAME_LENGTH;
	char* iface;
	time_t t;
	struct ifreq ifr;
	struct ifaddrs *ifaddr;
	struct sockaddr_ll dev;
	uint8_t eth_frame[IP_MAXPACKET];

	// Parse command line arguments for target IP, target MAC, and target port
	if(argc < 2) {
		fprintf(stdout, "usage: rtt <target ip> <target MAC> <target port> <interface>\r\n");
		exit(EXIT_FAILURE);
	}

	src = (struct host*)malloc(sizeof(struct host) );
	dest = (struct host*)malloc(sizeof(struct host) );

	char tmp[80];
	dest->ip = inet_addr(argv[1]);
	i = 0;
	do {
		strcpy(tmp,"0x");
		strcat(tmp,argv[i+2]);
		dest->hwaddr[i] = strtol(tmp,NULL,0);
	} while(i++ < 6);
	dest->port = atoi(argv[8]);
	iface = argv[9];

	// Generate a random IP packet ID and source port
	srand((unsigned) time(&t) );

	// Open a socket to get the HW address of the local machine. We're not using
	// this socket for the TCP connection. It's just to get the device hw address
	if((sockfd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL) ) ) < 0 ) {
		perror("socket");
		exit(EXIT_FAILURE);
	}

	memset(&ifr, 0, sizeof(struct ifreq) );	
	snprintf(ifr.ifr_name, sizeof(ifr.ifr_name), iface);
	if(ioctl(sockfd, SIOCGIFHWADDR, &ifr) == -1) {
		perror("ioctl");
		exit(EXIT_FAILURE);
	}
	close(sockfd);
	strncpy(src->hwaddr, ifr.ifr_hwaddr.sa_data,6);
	
	// Get our interface from which we'll be sending. This will be used in
	// building the ethernet (layer 2) frame.
	memset(&dev, 0, sizeof(dev) );
	if((dev.sll_ifindex = if_nametoindex(iface) ) == 0) {
		perror("if_nametoindex");
		exit(EXIT_FAILURE);
	}
	dev.sll_family = AF_PACKET;
	memcpy(dev.sll_addr, src->hwaddr, 6);
	dev.sll_halen = 6;
	
	// Get the IP associated with our chosen interface
	getifaddrs(&ifaddr);
	while((ifaddr->ifa_addr->sa_family != AF_INET) || (strcmp(ifaddr->ifa_name,iface) != 0) ) {
		ifaddr = ifaddr->ifa_next;
	}
	src->ip = ((struct sockaddr_in*)ifaddr->ifa_addr)->sin_addr.s_addr;
	
	// Get a file dscriptor to our raw packet socket.
	if((sockfd = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL) ) ) < 0) {
		perror("socket");
		exit(EXIT_FAILURE);
	}

	i = 0;
	memset(&cycles,0,sizeof(struct ticks) );
	fprintf(stderr, "  [+] Calculating overheads...\r\n");
	while(TRIALS > i++) {
		getOverheads(sockfd, eth_frame, &dev);
	}
	cycles.read >>= LOG2TRIALS;
	cycles.recvfrom >>= LOG2TRIALS;

	i = 0;
	fprintf(stderr, "  [+] Sending handshakes...\r\n");
	while(TRIALS > i++) {
		tcpHandshake(sockfd, eth_frame, &dev);
	}
	cycles.tcpRtt >>= LOG2TRIALS;
	printf("%llu\r\n", cycles.tcpRtt - cycles.read - cycles.recvfrom);

	close(sockfd);
	return 0;
}

void createEthFrame(uint8_t* eth_frame) {
	memcpy(eth_frame,dest->hwaddr,6);
	memcpy(eth_frame + 6,src->hwaddr,6);
	eth_frame[12] = ETH_P_IP / 256;
	eth_frame[13] = ETH_P_IP % 256;	
}

void createIpHeader(uint8_t* eth_frame) {

	struct iphdr* iph = (struct iphdr*)(eth_frame + ETH_HDRLEN);

	iph->version  = 4; // IPv4
	iph->ihl      = 5; // IP header length = ihl*4 (20 bytes)
	iph->tos      = 0; // Type of service
	iph->tot_len  = htons(sizeof(struct ip) + sizeof(struct tcphdr) ); // Total len of IP pkt and TCP header
	iph->id       = htons(rand() % 65336); // Random ID we generated for this packet, incremented on each sned
	iph->frag_off = htons(16384); // Something with packet fragmenation. Just used an example
	iph->ttl      = 64; // Time-to-live (64 hops)
	iph->protocol = IPPROTO_TCP; // TCP protocol
	iph->check    = 0; // Initialize IP checksum to 0 (calculated a few lines later)
	iph->saddr    = src->ip; // Source IP
	iph->daddr    = dest->ip; // Destination IP
	iph->check    = getChecksum((unsigned short*)iph, iph->tot_len); // Compute checksum
}

void getOverheads(int sockfd, uint8_t* eth_frame, struct sockaddr_ll* dev) {

	int i;
	int saddr_size;
	struct sockaddr saddr;
	struct tcphdr* in_tcph;
	uint8_t recvBuff[IP_MAXPACKET];
	unsigned long long readStart;
	unsigned long long recvfromStart;
	unsigned long long tmpRecvfromEnd;
	unsigned long long startTcpRtt;
	unsigned long long tmpEndTcpRtt;
	
	src->port = 1024 + rand() % (65536 - 1024);
	memset(eth_frame, 0, IP_MAXPACKET);
	createEthFrame(eth_frame);
	createIpHeader(eth_frame);
	createTcpHeader(eth_frame, src->port, dest->port, 0x02, htonl(rand() ), 0);

	readStart = rdtsc();
	cycles.read += rdtsc() - readStart;
	// Send out out packet over the socket, using the interface we chose.	
	saddr_size = sizeof(saddr);
	struct iphdr* in_iph;

	if(sendto(sockfd, eth_frame, FRAME_LENGTH, 0, (struct sockaddr*) dev, sizeof(*dev) ) <= 0) {
		perror("sendto:");
		exit(EXIT_FAILURE);
	}
	startTcpRtt = rdtsc();

	in_iph = (struct iphdr*)(recvBuff + ETH_HDRLEN);
	in_tcph = (struct tcphdr*)(recvBuff + ETH_HDRLEN + IP4_HDRLEN);
	if(dest->ip == inet_addr("127.0.0.1") ) {
		uint8_t synack[IP_MAXPACKET];
		while(in_iph->saddr != src->ip || in_tcph->rst == 1) {
			if(recvfrom(sockfd, recvBuff, IP_MAXPACKET, 0, &saddr, &saddr_size ) < 0) {
				perror("recvfrom");
				exit(EXIT_FAILURE);
			}
			in_iph = (struct iphdr*)(recvBuff + ETH_HDRLEN);
			in_tcph = (struct tcphdr*)(recvBuff + ETH_HDRLEN + IP4_HDRLEN);
		}

		memcpy(synack, eth_frame, IP_MAXPACKET);
		in_iph = (struct iphdr*)(synack + ETH_HDRLEN);
		in_iph->id = 0;
		createTcpHeader(synack, dest->port, src->port, 0x12, ntohl(rand() ), ntohl(in_tcph->seq)+1);
		if(sendto(sockfd, synack, FRAME_LENGTH, 0, (struct sockaddr*) dev, sizeof(*dev) ) <= 0) {
			perror("sendto:");
			exit(EXIT_FAILURE);
		}
	} else {
		usleep(250000);
	}

	// Start receiving packets
	do {
		recvfromStart = rdtsc();
		if(recvfrom(sockfd, recvBuff, IP_MAXPACKET, 0, &saddr, &saddr_size ) < 0) {
			perror("recvfrom");
			exit(EXIT_FAILURE);
		}
		tmpRecvfromEnd = rdtsc();
		in_tcph = (struct tcphdr*)(recvBuff + ETH_HDRLEN + IP4_HDRLEN);
	} while(ntohs(in_tcph->source) != dest->port);
	cycles.recvfrom += tmpRecvfromEnd - recvfromStart;

	// Reset the connection so it's not left open on the remote host (just good manners)
	createTcpHeader(eth_frame, src->port, dest->port, 0x04, ntohl(in_tcph->ack_seq), ntohl(in_tcph->seq)+1);
	if(sendto(sockfd, eth_frame, FRAME_LENGTH, 0, (struct sockaddr*)dev, sizeof(*dev) ) <= 0) {
		perror("sendto:");
		exit(EXIT_FAILURE);
	}
	while(recvfrom(sockfd, recvBuff, IP_MAXPACKET, MSG_DONTWAIT, &saddr, &saddr_size) > 0);
}


void tcpHandshake(int sockfd, uint8_t* eth_frame, struct sockaddr_ll* dev) {

	int i;
	int saddr_size;
	int packetsIn;
	struct sockaddr saddr;
	uint8_t recvBuff[IP_MAXPACKET];
	struct tcphdr* in_tcph;
	unsigned long long startTcpRtt;
	unsigned long long tmpEndTcpRtt;
	
	src->port = 1024 + rand() % (65536 - 1024);
	memset(eth_frame, 0, IP_MAXPACKET);
	createEthFrame(eth_frame);
	createIpHeader(eth_frame);
	createTcpHeader(eth_frame, src->port, dest->port, 0x02, htonl(rand() ), 0);

	// Send out out packet over the socket, using the interface we chose.	
	saddr_size = sizeof(saddr);
	struct iphdr* in_iph;

	packetsIn = 0;
	if(sendto(sockfd, eth_frame, FRAME_LENGTH, 0, (struct sockaddr*) dev, sizeof(*dev) ) <= 0) {
		perror("sendto:");
		exit(EXIT_FAILURE);
	}
	startTcpRtt = rdtsc();

	in_iph = (struct iphdr*)(recvBuff + ETH_HDRLEN);
	in_tcph = (struct tcphdr*)(recvBuff + ETH_HDRLEN + IP4_HDRLEN);
	unsigned long long tmp;
	tmp = rdtsc();
	if(dest->ip == inet_addr("127.0.0.1") ) {
		uint8_t synack[IP_MAXPACKET];
		while(in_iph->saddr != src->ip || in_tcph->rst == 1) {
			if(recvfrom(sockfd, recvBuff, IP_MAXPACKET, 0, &saddr, &saddr_size ) < 0) {
				perror("recvfrom");
				exit(EXIT_FAILURE);
			}
			in_iph = (struct iphdr*)(recvBuff + ETH_HDRLEN);
			in_tcph = (struct tcphdr*)(recvBuff + ETH_HDRLEN + IP4_HDRLEN);
		}

		memcpy(synack, eth_frame, IP_MAXPACKET);
		in_iph = (struct iphdr*)(synack + ETH_HDRLEN);
		in_iph->id = 0;
		createTcpHeader(synack, dest->port, src->port, 0x12, ntohl(rand() ), ntohl(in_tcph->seq)+1);
		if(sendto(sockfd, synack, FRAME_LENGTH, 0, (struct sockaddr*) dev, sizeof(*dev) ) <= 0) {
			perror("sendto:");
			exit(EXIT_FAILURE);
		}
	};
	// Start receiving packets
	do {
		if(recvfrom(sockfd, recvBuff, IP_MAXPACKET, 0, &saddr, &saddr_size ) < 0) {
			perror("recvfrom");
			exit(EXIT_FAILURE);
		}
		in_tcph = (struct tcphdr*)(recvBuff + ETH_HDRLEN + IP4_HDRLEN);
		tmpEndTcpRtt = rdtsc();
		packetsIn++;
	} while(ntohs(in_tcph->source) != dest->port);
	cycles.tcpRtt += tmpEndTcpRtt - startTcpRtt;// - cycles.sendto - packetsIn*(cycles.recvfrom);

	// Reply to the SYN-ACK with an ACK
	createTcpHeader(eth_frame, src->port, dest->port, 0x10, ntohl(in_tcph->ack_seq), ntohl(in_tcph->seq)+1);
	if(sendto(sockfd, eth_frame, FRAME_LENGTH, 0, (struct sockaddr*)dev, sizeof(*dev) ) <= 0) {
		perror("sendto:");
		exit(EXIT_FAILURE);
	}			
	
	// Reset the connection so it's not left open on the remote host (just good manners)
	createTcpHeader(eth_frame, src->port, dest->port, 0x04, ntohl(in_tcph->ack_seq), ntohl(in_tcph->seq)+1);
	if(sendto(sockfd, eth_frame, FRAME_LENGTH, 0, (struct sockaddr*)dev, sizeof(*dev) ) <= 0) {
		perror("sendto:");
		exit(EXIT_FAILURE);
	}
	while(recvfrom(sockfd, recvBuff, IP_MAXPACKET, MSG_DONTWAIT, &saddr, &saddr_size) > 0);
}

void createTcpHeader(uint8_t* eth_frame, int srcPort, int dstPort, char flags, int seq, int ack_seq) {
	struct psdhdr {
		unsigned int src_addr;
		unsigned int dst_addr;
		unsigned char reserved;
		unsigned char protocol;
		unsigned short tcp_length;

		struct tcphdr tcp;
	};

	struct psdhdr psh;
	struct tcphdr* tcph = (struct tcphdr*)(eth_frame + ETH_HDRLEN + IP4_HDRLEN);

	tcph->source  = htons(srcPort); // Our (random) port
	tcph->dest    = htons(dstPort); // Destination port
	tcph->seq     = htonl(seq);     // Sequence number (randomly generated
	tcph->ack_seq = htonl(ack_seq); // Ack sequence number, set during handshake
	tcph->doff    = sizeof(struct tcphdr) / 4; // Offset
	tcph->urg     = (flags & 0x20) >> 5; // Flags
	tcph->ack     = (flags & 0x10) >> 4;
	tcph->psh     = (flags & 0x08) >> 3;
	tcph->rst     = (flags & 0x04) >> 2;
	tcph->syn     = (flags & 0x02) >> 1;
	tcph->fin     = flags & 0x01;
	tcph->window  = htons(43690); //Window size, pretty arbitrary for most circumstances
	tcph->check   = 0; // Initialize checksum to 0
	tcph->urg_ptr = 0; // Urgent pointer, don't know what this does. Not important

	// Pseudo-header, not part of the packet itself but used in computing the TCP checksum
	psh.src_addr   = src->ip; // Source address
	psh.dst_addr   = dest->ip; // Destination IP
	psh.reserved   = 0;     // For future use by the protocol, set to zero
	psh.protocol   = IPPROTO_TCP; // TCP protocol
	psh.tcp_length = htons(sizeof(struct tcphdr ) ); //Length of TCP header

	// Copy the whole TCP header into the psuedo-header data structure.
	memcpy(&psh.tcp, tcph, sizeof(struct tcphdr ) );

	// Compute checksum over whole of psuedo-header bytes
	tcph->check = getChecksum((unsigned short*)&psh, sizeof(struct psdhdr) );
}

unsigned short getChecksum(unsigned short* dg, int len) {
	int i, sum;

	sum = 0;
	do {
		sum += *dg++;
		len -= 2;
	} while(len > 0);
	sum = (sum >> 16) + (sum & 0xffff);

	return ~sum & 0xffff;
}

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
