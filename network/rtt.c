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

//#define LOOPBACK   1

#define TRIALS     64
#define BUFSIZE    1024
#define IP4_HDRLEN 20
#define TCP_HDRLEN 20
#define ETH_HDRLEN 14
#define DGRAM_SIZE 4096
#define LOG2TRIALS 6

#define PROBE printf("!Probe!\r\n");
#define DUMP(s, i, buf, sz)  {printf(s);                   \
                              for (i = 0; i < (sz);i++)    \
                                  printf("%x ", 0xff & buf[i]); \
                              printf("\n");}

void getOverheads(int sockfd);
void createIpHeader(struct iphdr* iph);
void createTcpHeader(struct tcphdr* tcph, int srcPort, int dstPort, char flags, int seq, int ack_seq);
unsigned short getChecksum(unsigned short* dg, int len); 
static __attribute__((always_inline)) unsigned long long rdtsc(void); 

int ipId;
int dstIp;
int srcIp;
int dstPort;
int srcPort;
uint8_t dstMac[6];
uint8_t srcMac[6];
uint8_t eth_frame[IP_MAXPACKET];
const char* GET_REQ = "GET / HTTP/1.0\r\n\r\n";
unsigned long long sendOverhead;
unsigned long long recvOverhead;

int main(int argc, char* argv[]) {
	int i;
	int sockfd;
	int saddr_size;
	char* iface;
	time_t t;
	struct ifreq ifr;
	struct iphdr *iph;
	struct tcphdr *tcph;
	struct ifaddrs *ifaddr;
	struct sockaddr saddr;
	struct sockaddr_ll dev;
	uint8_t recvBuff[IP_MAXPACKET];

	// Parse command line arguments for target IP, target MAC, and target port
	if(argc < 2) {
		fprintf(stdout, "usage: rtt <target ip> <target MAC> <target port> <interface>\r\n");
		exit(EXIT_FAILURE);
	}

	char tmp[80];
	dstIp = inet_addr(argv[1]);
	i = 0;
	do {
		strcpy(tmp,"0x");
		strcat(tmp,argv[i+2]);
		dstMac[i] = strtol(tmp,NULL,0);
	} while(i++ < 6);
	dstPort = atoi(argv[8]);
	iface = argv[9];

	// Generate a random IP packet ID and source port
	srand((unsigned) time(&t) );
	ipId = rand() % 65336;
	srcPort = 1024 + rand() % (65536 - 1024);

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
	strncpy(srcMac, ifr.ifr_hwaddr.sa_data,6);
	
	// Get our interface from which we'll be sending. This will be used in
	// building the ethernet (layer 2) frame.
	memset(&dev, 0, sizeof(dev) );
	if((dev.sll_ifindex = if_nametoindex(iface) ) == 0) {
		perror("if_nametoindex");
		exit(EXIT_FAILURE);
	}
	dev.sll_family = AF_PACKET;
	memcpy(dev.sll_addr, srcMac, 6);
	dev.sll_halen = 6;
	
	// Get the IP associated with our chosen interface
	getifaddrs(&ifaddr);
	while((ifaddr->ifa_addr->sa_family != AF_INET) || (strcmp(ifaddr->ifa_name,iface) != 0) ) {
		ifaddr = ifaddr->ifa_next;
	}
	srcIp = ((struct sockaddr_in*)ifaddr->ifa_addr)->sin_addr.s_addr;
	
	// Build the ethernet frame.
	memset(eth_frame, 0, IP_MAXPACKET);
	iph  = (struct iphdr*)(eth_frame + ETH_HDRLEN);
	tcph = (struct tcphdr*)(eth_frame + ETH_HDRLEN + IP4_HDRLEN);
	createIpHeader(iph);
	createTcpHeader(tcph, srcPort, dstPort, 0x02, htonl(rand() ), 0);

	int frame_length;
	frame_length = 2*6 + 2 + IP4_HDRLEN + TCP_HDRLEN;
	memcpy(eth_frame,dstMac,6);
	memcpy(eth_frame + 6,srcMac,6);
	eth_frame[12] = ETH_P_IP / 256;
	eth_frame[13] = ETH_P_IP % 256;	
	
	// Get a file dscriptor to our raw packet socket.
	if((sockfd = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL) ) ) < 0) {
		perror("socket");
		exit(EXIT_FAILURE);
	}

	int bytes;
	// Send out out packet over the socket, using the interface we chose.	
	if((bytes = sendto(sockfd, eth_frame, frame_length, 0, (struct sockaddr*) &dev, sizeof(dev) ) ) <= 0) {
		perror("sendto:");
		exit(EXIT_FAILURE);
	}

	// Start receiving packets
	saddr_size = sizeof(saddr);
	memset(recvBuff,0,IP_MAXPACKET);
	struct iphdr* in_iph;
	do {
		if(recvfrom(sockfd, recvBuff, IP_MAXPACKET, 0, &saddr, &saddr_size ) == -1) {
			perror("recvfrom");
			exit(EXIT_FAILURE);
		}

		in_iph = (struct iphdr*)(recvBuff + ETH_HDRLEN);
	} while(in_iph->saddr != dstIp);

	// Get the TCP header from the inbound packet
	struct tcphdr *in_tcph = (struct tcphdr*)(recvBuff + ETH_HDRLEN + IP4_HDRLEN);
		
	if(in_iph->saddr == inet_addr("127.0.0.1") ) {
		createTcpHeader(tcph, dstPort, srcPort, 0x12, ntohl(rand()), ntohl(in_tcph->seq)+1);
		if((bytes = sendto(sockfd, eth_frame, frame_length, 0, (struct sockaddr*)&dev, sizeof(dev) ) ) <= 0) {
			perror("sendto:");
			exit(EXIT_FAILURE);
		}
		do {
			if(recvfrom(sockfd, recvBuff, IP_MAXPACKET, 0, &saddr, &saddr_size ) == -1) {
				perror("recvfrom");
				exit(EXIT_FAILURE);
			}

			in_iph = (struct iphdr*)(recvBuff + ETH_HDRLEN);
		} while(in_iph->saddr != dstIp);
	}
	
	in_tcph = (struct tcphdr*)(recvBuff + ETH_HDRLEN + IP4_HDRLEN);
	createTcpHeader(tcph, srcPort, dstPort, 0x10, ntohl(in_tcph->ack_seq), ntohl(in_tcph->seq)+1);
	// Is this the SYN-ACK we're looking for?
	printf("%x %x %x %x\r\n", in_tcph->dest, tcph->source, in_tcph->ack, in_tcph->syn);
	if(in_tcph->dest == tcph->source && (in_tcph->ack && (in_tcph->fin || in_tcph->syn) ) ) {

		// Reply to the SYN-ACK with an ACK
		createTcpHeader(tcph, srcPort, dstPort, 0x10, ntohl(in_tcph->ack_seq), ntohl(in_tcph->seq)+1);
		if((bytes = sendto(sockfd, eth_frame, frame_length, 0, (struct sockaddr*)&dev, sizeof(dev) ) ) <= 0) {
			perror("sendto:");
			exit(EXIT_FAILURE);
		}			
	}
	
	// Reset the connection so it's not left open on the remote host (just good manners)
	createTcpHeader(tcph, srcPort, dstPort, 0x04, ntohl(tcph->seq), ntohl(tcph->ack_seq));
	if((bytes = sendto(sockfd, eth_frame, frame_length, 0, (struct sockaddr*)&dev, sizeof(dev) ) ) <= 0) {
		perror("sendto:");
		exit(EXIT_FAILURE);
	}
	close(sockfd);
	return 0;
}

void getOverheads(int sockfd) {
	int i;
	char recvBuff[BUFSIZE];
	const char sendChar = "\0";
	unsigned long long readStart, sendStart, recvStart;
	unsigned long long readDuration, sendDuration, recvDuration;
	unsigned long long readOverhead;

	i = 0;
	readDuration = 0;
	do {
		readStart = rdtsc();
		readDuration += rdtsc() - readStart;
	} while(i++ < TRIALS);

	readOverhead = readDuration >> LOG2TRIALS;

	i = 0;
	sendDuration = 0;
	recvDuration = 0;
	do {
		sendStart = rdtsc();
		if((send(sockfd, GET_REQ, strlen(GET_REQ), 0) ) == -1) {
			perror("send");
			exit(EXIT_FAILURE);
		}
		sendDuration += rdtsc() - sendStart - readOverhead;
		usleep(100000);
		recvStart = rdtsc();
		if((recv(sockfd, &recvBuff, BUFSIZE, 0) ) == -1 ) {
			perror("recv");
			exit(EXIT_FAILURE);
		}
		recvDuration += rdtsc() - recvStart - readOverhead;
	} while(i++ < TRIALS);

	sendOverhead = sendDuration >> LOG2TRIALS;
	recvOverhead = recvDuration >> LOG2TRIALS;
}

void createIpHeader(struct iphdr* iph) {

	iph->version  = 4; // IPv4
	iph->ihl      = 5; // IP header length = ihl*4 (20 bytes)
	iph->tos      = 0; // Type of service
	iph->tot_len  = htons(sizeof(struct ip) + sizeof(struct tcphdr) ); // Total len of IP pkt and TCP header
	iph->id       = htons(ipId++); // Random ID we generated for this packet, incremented on each sned
	iph->frag_off = htons(16384); // Something with packet fragmenation. Just used an example
	iph->ttl      = 64; // Time-to-live (64 hops)
	iph->protocol = IPPROTO_TCP; // TCP protocol
	iph->check    = 0; // Initialize IP checksum to 0 (calculated a few lines later)
	iph->saddr    = srcIp; // Source IP
	iph->daddr    = dstIp; // Destination IP
	iph->check    = getChecksum((unsigned short*)eth_frame, iph->tot_len); // Compute checksum
}

void createTcpHeader(struct tcphdr* tcph, int srcPort, int dstPort, char flags, int seq, int ack_seq) {
	struct psdhdr {
		unsigned int src_addr;
		unsigned int dst_addr;
		unsigned char reserved;
		unsigned char protocol;
		unsigned short tcp_length;

		struct tcphdr tcp;
	};

	struct psdhdr psh;

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
	psh.src_addr   = srcIp; // Source address
	psh.dst_addr   = dstIp; // Destination IP
	psh.reserved   = 0;     // For future use by the protocol, set to zero
	psh.protocol   = IPPROTO_TCP; // TCP protocol
	psh.tcp_length = htons(sizeof(struct tcphdr ) ); //Length of TCP header

	// Copy the whole TCP header into the psuedo-header data structure.
	memcpy(&psh.tcp, tcph, sizeof(struct tcphdr ) );

	// Compute checksum over whole of psuedo-header bytes
	tcph->check = getChecksum((unsigned short*)&psh, sizeof(struct psdhdr) );

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
