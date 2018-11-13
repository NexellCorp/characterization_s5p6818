/*  Copyright (C) 2011-2013  P.D. Buchan (pdbuchan@yahoo.com)

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

// Send an IPv4 UDP packet via raw socket at the link layer (ethernet frame).
// Need to have destination MAC address.
// Includes some UDP data.

/*
 * prerequisite:
 *   INTERFACE has mac address. If not, need to change sysCmd.
 * 
 * 
 * Sender
 *   1. open socket type of SOCK_RAW
 *   2. make raw packet
 *		SRC_IP_ADDR:PORT => DST_IP_ADDR:PORT
 *   3. send it
 *
 * Receiver
 *	 1. open socket type of SOCK_DGRAM
 *   2. listen on :PORT
 */
#define _GNU_SOURCE				// sched_setaffinity
#ifndef __USE_GNU
#define __USE_GNU
#endif
#include <sched.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>           // close()
#include <string.h>           // strcpy, memset(), and memcpy()

#include <netdb.h>            // struct addrinfo
#include <sys/types.h>        // needed for socket()
#include <sys/socket.h>       // needed for socket()
#include <netinet/in.h>       // IPPROTO_UDP
#include <netinet/ip.h>       // struct ip and IP_MAXPACKET (which is 65535)
#include <netinet/udp.h>      // struct udphdr
#include <arpa/inet.h>        // inet_pton() and inet_ntop()
#include <sys/ioctl.h>        // macro ioctl is defined
#include <bits/ioctls.h>      // defines values for argument "request" of ioctl.
#include <net/if.h>           // struct ifreq
#include <linux/if_ether.h>   // ETH_P_IP = 0x0800, ETH_P_IPV6 = 0x86DD
#include <linux/if_packet.h>  // struct sockaddr_ll (see man 7 packet)
#include <net/ethernet.h>

#include <linux/sockios.h>    // SIOCGMIIREG & SIOCGMIIPHY
#include <linux/ethtool.h>    // ETHTOOL

#include <errno.h>            // errno, perror()

#include <pthread.h>          // pthread create
#include <sys/select.h>       // select
#include <sys/time.h>         // struct timespec

#include "gmac_test.h"
#include "gpio.h"

#define nx_debug(fmt, ...)			\
		if (g_debug == 1)			\
			printf(fmt, ##__VA_ARGS__)



int g_repeat = DEF_REPEAT;
int g_phyid = DEF_PHYID;					/* phy interface id. h/w config */

int g_debug = 0;					/* print debug message. hidden */


pthread_mutex_t sync_mutex;
pthread_cond_t  sync_cond;
int sync_state = 0;

char recvBuffer[1600];

// Function prototypes
unsigned short int checksum (unsigned short int *, int);
unsigned short int udp4_checksum (struct ip, struct udphdr, unsigned char *, int);

struct ifreq ifr;



/**
 * struct mii_ioctl_data {
 *		__u16		phy_id;
 *		__u16		reg_num;
 *		__u16		val_in;
 *		__u16		val_out;
 * };
 **/

int mdio_read(int skfd, int phy_id, int location)
{
	unsigned short int *data = (unsigned short int *)(&ifr.ifr_data);

	data[0] = phy_id;
	data[1] = location;

	if (ioctl(skfd, SIOCGMIIREG, &ifr) < 0) {
		fprintf(stderr, "SIOCGMIIREG on %s failed: %s\n", ifr.ifr_name,
				strerror(errno));
		return -1;
	}
	return data[3];
}

int mdio_write(int skfd, int phy_id, int location, int val)
{
	unsigned short int *data = (unsigned short int *)(&ifr.ifr_data);

	data[0] = (uint16_t)phy_id;
	data[1] = (uint16_t)location;
	data[2] = (uint16_t)val;

	if (ioctl(skfd, SIOCSMIIREG, &ifr) < 0) {
		fprintf(stderr, "SIOCSMIIREG on %s failed: %s\n", ifr.ifr_name,
				strerror(errno));
		return -1;
	}
	return 0;
}


// realtek phy
//   10M PCS Loopback
int enable_loopback(int skfd, int phy_id)
{
	mdio_write(skfd, phy_id, 31, 0x0000);
	mdio_write(skfd, phy_id,  0, 0x8000);
	usleep(50000);
	mdio_write(skfd, phy_id,  0, 0x4100);
	usleep(50000);

	return 0;
}

int disable_loopback(int skfd, int phy_id)
{
	mdio_write(skfd, phy_id, 31, 0x0000);
	mdio_write(skfd, phy_id,  0, 0x1140);

	return 0;
}



int basic_mii(void)
{
	int skfd;
	int phy_id = g_phyid;

	int mii_reg;
	unsigned short int mii_val[32];

	const char *ifname = INTERFACE_NAME;


	printf(" ==== Basic mii setting... =============================================\n");

	if ((skfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("socket");
		exit(-1);
	}


	/* Get the vitals from the interface. */
	strncpy(ifr.ifr_name, ifname, IFNAMSIZ);
	if (ioctl(skfd, SIOCGMIIPHY, &ifr) < 0) {
		fprintf(stderr, "SIOCGMIIPHY on %s failed: %s\n", ifname,
				strerror(errno));
		(void) close(skfd);
		return 1;
	}


	nx_debug(" ==== Loopback Enable  =====================================================\n");
	enable_loopback(skfd, phy_id);
	usleep(50000);


	nx_debug(" ==== MII Regs. ============================================================\n");
	for (mii_reg = 0; mii_reg < 32; mii_reg++) {
		mii_val[mii_reg] = mdio_read(skfd, phy_id, mii_reg);
		nx_debug("%s %4.4x", (mii_reg % 8) == 0 ? "\n  " : "",
				mii_val[mii_reg]);
	}
	nx_debug("\n");

	close(skfd);

	return 0;
}


void get_src_mac_addr( char *interface, char *src_mac )
{
	int sd;
	struct ifreq _ifr;
	// Submit request for a socket descriptor to look up interface.
	//if ((sd = socket (PF_PACKET, SOCK_RAW, htons (ETH_P_ALL))) < 0) {
	if ((sd = socket (PF_INET, SOCK_DGRAM, 0)) < 0) {
		printf("socket() failed to get socket descriptor for using ioctl() %d\n", __LINE__);
		exit (EXIT_FAILURE);
	}

	// Use ioctl() to look up interface name and get its MAC address.
	memset (&_ifr, 0, sizeof (_ifr));
	snprintf (_ifr.ifr_name, sizeof (_ifr.ifr_name), "%s", interface);
	if (ioctl (sd, SIOCGIFHWADDR, &_ifr) < 0) {
		perror ("ioctl() failed to get source MAC address ");
		exit (EXIT_FAILURE);
	}
	close (sd);

	memcpy (src_mac, _ifr.ifr_hwaddr.sa_data, 6);
}

int is_link_up( const char *interface )
{
	int sd;
	struct ifreq _ifr;

//#define __LINKSTAT_FROM_MII_
#ifdef __LINKSTAT_FROM_MII_
	uint16_t *data, mii_val;
	uint32_t phy_id;
	// Submit request for a socket descriptor to look up interface.

	/* Get the vitals from the interface */
	strncpy (_ifr.ifr_name, interface, IFNAMSIZ);
	if ((sd = socket (PF_PACKET, SOCK_RAW, htons (ETH_P_ALL))) < 0) {
		printf("socket() failed to get socket descriptor for using ioctl() %d\n", __LINE__);
		exit (EXIT_FAILURE);
	}

	if (ioctl (sd, SIOCGMIIPHY, &_ifr ) <0 )
	{
		strerror (errno);
		close (sd);
		return -1;
	}

	data = (uint16_t *) (& _ifr.ifr_data);
	phy_id = data [0];
	data [1] = 1;

	if( ioctl (sd, SIOCGMIIREG, &_ifr) <0 )
	{
		strerror (errno);
		return 2;
	}

	(void)close (sd);

	mii_val = data [3];
	//	printf( "mii_val = 0x%04x\n", mii_val );
	return (((mii_val & 0x0016) == 0x0004)? 1 : 0);
#else
	struct ethtool_value eth;

	if ((sd = socket (PF_PACKET, SOCK_RAW, htons (ETH_P_ALL))) < 0) {
	//if ((sd = socket (PF_INET, SOCK_DGRAM, htons (ETH_P_ALL))) < 0) {
		printf("socket() failed to get socket descriptor for using ioctl() %d\n", __LINE__);
		exit (EXIT_FAILURE);
	}

	memset(&_ifr, 0, sizeof(_ifr));
	strncpy (_ifr.ifr_name, interface, IFNAMSIZ);
	_ifr.ifr_data = (caddr_t) &eth;
	eth.cmd = ETHTOOL_GLINK;

	if( ioctl (sd, SIOCETHTOOL, &_ifr) < 0 )
	{
		strerror (errno);
		return 2;
	}

	(void)close (sd);

	return (eth.data) ? 1 : 0;
#endif
}

int send_packet( void )
{
	int status, datalen, frame_length, sd, bytes, ip_flags[4];
	char interface[40], target[40], src_ip[16], dst_ip[16];
	struct ip iphdr;
	struct udphdr udphdr;
	unsigned char *data;
	char src_mac[6], dst_mac[6], *ether_frame;
	struct sockaddr_ll device;
	void *tmp;
	int on = 1;
	int i;

	// Allocate memory for various arrays.
	memset (src_mac, 0, 6 * sizeof (unsigned char));
	memset (dst_mac, 0, 6 * sizeof (unsigned char));

	// Maximum UDP payload size = 65535 - IPv4 header (20 bytes) - UDP header (8 bytes)
	tmp = (unsigned char *) malloc ((IP_MAXPACKET - IP4_HDRLEN - UDP_HDRLEN) * sizeof (unsigned char));
	if (tmp != NULL) {
		data = tmp;
	} else {
		fprintf (stderr, "ERROR: Cannot allocate memory for array 'data'.\n");
		exit (EXIT_FAILURE);
	}
	memset (data, 0, (IP_MAXPACKET - IP4_HDRLEN - UDP_HDRLEN) * sizeof (unsigned char));

	tmp = (unsigned char *) malloc (IP_MAXPACKET * sizeof (unsigned char));
	if (tmp != NULL) {
		ether_frame = tmp;
	} else {
		fprintf (stderr, "ERROR: Cannot allocate memory for array 'ether_frame'.\n");
		exit (EXIT_FAILURE);
	}
	memset (ether_frame, 0, IP_MAXPACKET * sizeof (unsigned char));

	memset (interface, 0, 40 * sizeof (char));
	memset (target, 0, 40 * sizeof (char));
	memset (src_ip, 0, 16 * sizeof (char));
	memset (dst_ip, 0, 16 * sizeof (char));
	memset (ip_flags, 0, 4 * sizeof (int));

	// Interface to send packet through.
	strcpy (interface, INTERFACE_NAME);

	// Find Source Mac Address
	get_src_mac_addr( interface, src_mac );

	/* Report source MAC address to stdout. */
	src_mac[5] = (src_mac[5] + 1) & 0xff;
	nx_debug("  Source MAC of interface %s is ", interface);
	for (i=0; i<5; i++) {
		nx_debug("%02x:", (unsigned char)src_mac[i]);
	}
	nx_debug("%02x\n", src_mac[5]);

	// Find interface index from interface name and store index in
	// struct sockaddr_ll device, which will be used as an argument of sendto().
	if ((device.sll_ifindex = if_nametoindex (interface)) == 0) {
		perror ("if_nametoindex() failed to obtain interface index ");
		exit (EXIT_FAILURE);
	}
	//printf ("Index for interface %s is %i\n", interface, device.sll_ifindex);

	// Set destination MAC address: you need to fill these out
	dst_mac[0] = 0xff;
	dst_mac[1] = 0xff;
	dst_mac[2] = 0xff;
	dst_mac[3] = 0xff;
	dst_mac[4] = 0xff;
	dst_mac[5] = 0xff;

	// Source IPv4 address: you need to fill this out
	strcpy (src_ip, SRC_IP_ADDR);

	// Destination URL or IPv4 address: you need to fill this out
	strcpy (dst_ip, DST_IP_ADDR);

	// Fill out sockaddr_ll.
	device.sll_family = AF_PACKET;
	memcpy (device.sll_addr, src_mac, 6);
	device.sll_halen = htons (6);

	// UDP data
	datalen = DATA_SIZE;
	snprintf((char *)data, DATA_SIZE+1, "%s", DATA_STRING);

	// IPv4 header

	// IPv4 header length (4 bits): Number of 32-bit words in header = 5
	iphdr.ip_hl = IP4_HDRLEN / sizeof (unsigned int);

	// Internet Protocol version (4 bits): IPv4
	iphdr.ip_v = 4;

	// Type of service (8 bits)
	iphdr.ip_tos = 0;

	// Total length of datagram (16 bits): IP header + UDP header + datalen
	iphdr.ip_len = htons (IP4_HDRLEN + UDP_HDRLEN + datalen);

	// ID sequence number (16 bits): unused, since single datagram
	iphdr.ip_id = htons (0);

	// Flags, and Fragmentation offset (3, 13 bits): 0 since single datagram

	// Zero (1 bit)
	ip_flags[0] = 0;

	// Do not fragment flag (1 bit)
	ip_flags[1] = 0;

	// More fragments following flag (1 bit)
	ip_flags[2] = 0;

	// Fragmentation offset (13 bits)
	ip_flags[3] = 0;

	iphdr.ip_off = htons ((ip_flags[0] << 15)
	    			+ (ip_flags[1] << 14)
	      			+ (ip_flags[2] << 13)
	      			+  ip_flags[3]);

	// Time-to-Live (8 bits): default to maximum value
	iphdr.ip_ttl = 255;

	// Transport layer protocol (8 bits): 17 for UDP
	iphdr.ip_p = IPPROTO_UDP;

	// Source IPv4 address (32 bits)
	if ((status = inet_pton (AF_INET, src_ip, &(iphdr.ip_src))) != 1) {
		fprintf (stderr, "inet_pton() failed.\nError message: %s", strerror (status));
		exit (EXIT_FAILURE);
	}

	// Destination IPv4 address (32 bits)
	if ((status = inet_pton (AF_INET, dst_ip, &(iphdr.ip_dst))) != 1) {
		fprintf (stderr, "inet_pton() failed.\nError message: %s", strerror (status));
		exit (EXIT_FAILURE);
	}

	// IPv4 header checksum (16 bits): set to 0 when calculating checksum
	iphdr.ip_sum = 0;
	iphdr.ip_sum = checksum ((unsigned short int *) &iphdr, IP4_HDRLEN);

	// UDP header

	// Source port number (16 bits): pick a number
	udphdr.source = htons (PORT);

	// Destination port number (16 bits): pick a number
	udphdr.dest = htons (PORT);

	// Length of UDP datagram (16 bits): UDP header + UDP data
	udphdr.len = htons (UDP_HDRLEN + datalen);

	// UDP checksum (16 bits)
	udphdr.check = udp4_checksum (iphdr, udphdr, data, datalen);

	// Fill out ethernet frame header.

	// Ethernet frame length = ethernet header (MAC + MAC + ethernet type) + ethernet data (IP header + UDP header + UDP data)
	frame_length = 6 + 6 + 2 + IP4_HDRLEN + UDP_HDRLEN + datalen;

	// Destination and Source MAC addresses
	memcpy (ether_frame, dst_mac, 6);
	memcpy (ether_frame + 6, src_mac, 6);

	// Next is ethernet type code (ETH_P_IP for IPv4).
	// http://www.iana.org/assignments/ethernet-numbers
	ether_frame[12] = ETH_P_IP / 256;
	ether_frame[13] = ETH_P_IP % 256;

	// Next is ethernet frame data (IPv4 header + UDP header + UDP data).

	// IPv4 header
	memcpy (ether_frame + 14, &iphdr, IP4_HDRLEN);

	// UDP header
	memcpy (ether_frame + 14 + IP4_HDRLEN, &udphdr, UDP_HDRLEN);

	// UDP data
	memcpy (ether_frame + 14 + IP4_HDRLEN + UDP_HDRLEN, data, datalen);

	// Submit request for a raw socket descriptor.
	if ((sd = socket (PF_PACKET, SOCK_RAW, htons (ETH_P_ALL))) < 0) {
		perror ("socket() failed ");
		exit (EXIT_FAILURE);
	}

	//setsockopt() failed: Protocol not available *
    if (setsockopt(sd, IPPROTO_IP, IP_HDRINCL, &on, sizeof(on)) == 0) {
        perror("setsockopt() failed");
		exit (EXIT_FAILURE);
	}

	nx_debug(" ================= sendto frame ============================================ \n");
	for (i = 0; i < frame_length; i++)
	{
		nx_debug("%02x ", (unsigned char)ether_frame[i]);
		if ((i + 1) % 40 == 0)
			nx_debug("\n");
	}
	nx_debug("\n");


	// Send ethernet frame to socket.
	if ((bytes = sendto (sd, ether_frame, frame_length, 0, (struct sockaddr *) &device, sizeof (device))) <= 0) {
		perror ("sendto() failed");
		exit (EXIT_FAILURE);
	}

	// Close socket descriptor.
	close (sd);

	// Free allocated memory.
	free (data);
	free (ether_frame);

	return (0);
}

// Checksum function
unsigned short int
checksum (unsigned short int *addr, int len)
{
	int nleft = len;
	int sum = 0;
	unsigned short int *w = addr;
	unsigned short int answer = 0;

	while (nleft > 1) {
		sum += *w++;
		nleft -= sizeof (unsigned short int);
	}

	if (nleft == 1) {
		*(unsigned char *) (&answer) = *(unsigned char *) w;
		sum += answer;
	}

	sum = (sum >> 16) + (sum & 0xFFFF);
	sum += (sum >> 16);
	answer = ~sum;
	return (answer);
}

// Build IPv4 UDP pseudo-header and call checksum function.
unsigned short int
udp4_checksum (struct ip iphdr, struct udphdr udphdr, unsigned char *payload, int payloadlen)
{
	char buf[IP_MAXPACKET];
	char *ptr;
	int chksumlen = 0;
	int i;

	ptr = &buf[0];  // ptr points to beginning of buffer buf

	// Copy source IP address into buf (32 bits)
	memcpy (ptr, &iphdr.ip_src.s_addr, sizeof (iphdr.ip_src.s_addr));
	ptr += sizeof (iphdr.ip_src.s_addr);
	chksumlen += sizeof (iphdr.ip_src.s_addr);

	// Copy destination IP address into buf (32 bits)
	memcpy (ptr, &iphdr.ip_dst.s_addr, sizeof (iphdr.ip_dst.s_addr));
	ptr += sizeof (iphdr.ip_dst.s_addr);
	chksumlen += sizeof (iphdr.ip_dst.s_addr);

	// Copy zero field to buf (8 bits)
	*ptr = 0; ptr++;
	chksumlen += 1;

	// Copy transport layer protocol to buf (8 bits)
	memcpy (ptr, &iphdr.ip_p, sizeof (iphdr.ip_p));
	ptr += sizeof (iphdr.ip_p);
	chksumlen += sizeof (iphdr.ip_p);

	// Copy UDP length to buf (16 bits)
	memcpy (ptr, &udphdr.len, sizeof (udphdr.len));
	ptr += sizeof (udphdr.len);
	chksumlen += sizeof (udphdr.len);

	// Copy UDP source port to buf (16 bits)
	memcpy (ptr, &udphdr.source, sizeof (udphdr.source));
	ptr += sizeof (udphdr.source);
	chksumlen += sizeof (udphdr.source);

	// Copy UDP destination port to buf (16 bits)
	memcpy (ptr, &udphdr.dest, sizeof (udphdr.dest));
	ptr += sizeof (udphdr.dest);
	chksumlen += sizeof (udphdr.dest);

	// Copy UDP length again to buf (16 bits)
	memcpy (ptr, &udphdr.len, sizeof (udphdr.len));
	ptr += sizeof (udphdr.len);
	chksumlen += sizeof (udphdr.len);

	// Copy UDP checksum to buf (16 bits)
	// Zero, since we don't know it yet
	*ptr = 0; ptr++;
	*ptr = 0; ptr++;
	chksumlen += 2;

	// Copy payload to buf
	memcpy (ptr, payload, payloadlen);
	ptr += payloadlen;
	chksumlen += payloadlen;

	// Pad to the next 16-bit boundary
	for (i=0; i<payloadlen%2; i++, ptr++) {
		*ptr = 0;
		ptr++;
		chksumlen++;
	}

	return checksum ((unsigned short int *) buf, chksumlen);
}



void *send_func(void *arg)
{
#ifdef __NEED_TIMEDWAIT__
    struct timeval now;
    struct timespec ts;

    gettimeofday(&now, NULL);
    ts.tv_sec = now.tv_sec + 2;
    ts.tv_nsec = now.tv_usec * 1000;

	pthread_mutex_lock( &sync_mutex );
	while (sync_state == 0)
		pthread_cond_timedwait( &sync_cond, &sync_mutex, &ts );
	pthread_mutex_unlock( &sync_mutex );
#else
	pthread_mutex_lock( &sync_mutex );
	while (sync_state == 0)
		pthread_cond_wait( &sync_cond, &sync_mutex );
	pthread_mutex_unlock( &sync_mutex );
#endif

	if( 0 != send_packet() )
	{
		printf("Send Packet Error!!\n");
		return (void *)(-1);
	}

	return 0;
}

void *recv_func(void *arg)
{
	int sd;
	ssize_t recvSize;
	fd_set rfds;
	struct timeval tv;
	int retval;

	#ifdef USING_PROMISC
	struct ifreq ifr;
	#endif

	#ifdef RECV_UDP
	struct sockaddr_in   server_addr;

	sd = socket(PF_INET, SOCK_DGRAM, 0);
	#else
	sd = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
	#endif
	if( sd < 0 )
	{
		printf("Socket Open Failed\n");
		return (void *)(-1);
	}

	#ifdef RECV_UDP
	memset( &server_addr, 0, sizeof( server_addr));
	server_addr.sin_family      = AF_INET;
	server_addr.sin_port        = htons(PORT);
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	if(-1 == bind( sd, (struct sockaddr*)&server_addr, sizeof(server_addr) ))
	{
		printf( "Socket bind() failed\n");
		goto errout;
	}
	#endif


	#ifdef USING_PROMISC
	// enable 'promiscuous mode' for the selected socket interface
	strncpy( ifr.ifr_name, INTERFACE_NAME, IFNAMSIZ );
	if ( ioctl( sd, SIOCGIFFLAGS, &ifr ) < 0 ) {
		printf( "ioctl: get ifflags\n" );
		goto errout;
	}
	ifr.ifr_flags |= IFF_PROMISC;  // enable 'promiscuous' mode
	if ( ioctl( sd, SIOCSIFFLAGS, &ifr ) < 0 ) {
		printf( "ioctl: set ifflags\n" );
		goto errout;
	}
	#endif


	FD_ZERO( &rfds );
	FD_SET( sd, &rfds );

	tv.tv_sec = 2;
	tv.tv_usec = 0;

	pthread_mutex_lock( &sync_mutex );
	sync_state = 1;
	pthread_cond_signal( &sync_cond );
	pthread_mutex_unlock( &sync_mutex );

	retval = select( sd+1, &rfds, NULL, NULL, &tv );
	if( retval > 0 )
	{
	}
	else if( retval == 0 )
	{
		printf("timeout!!!\n");
		goto errout;
	}
	else
	{
		printf("select error!!!\n");
		goto errout;
	}

	memset(recvBuffer, 0x0, sizeof(recvBuffer));
	recvSize = recvfrom(sd, recvBuffer, sizeof(recvBuffer), 0, NULL, NULL );
	printf("        recv size =================> %zd\n", recvSize);

errout:
	#ifdef USING_PROMISC
	// turn off the interface's 'promiscuous' mode
	ifr.ifr_flags &= ~IFF_PROMISC;  
	if ( ioctl( sd, SIOCSIFFLAGS, &ifr) < 0 ) {
		printf( "ioctl: set ifflags" );
		return (void *)(-1);
	}
	#endif

	close( sd );

	return (void *)0;
}


void print_usage(void)
{
	fprintf(stderr, "Loopback Test Program (ver. %s)\n", VERSION_STRING);
	fprintf(stderr, "Usage: %s\n", PROG_NAME);
	fprintf(stderr, "                [-C test_count]      (default %d)\n", DEF_REPEAT);
	fprintf(stderr, "                [-e phy-id]          (default %d)\n", DEF_PHYID);
	fprintf(stderr, "                [-T not support]\n");
}

int parse_opt(int argc, char *argv[])
{
	int opt;
	int _repeat = 1;

	while ((opt = getopt(argc, argv, "e:C:Tdh")) != -1) {
		switch (opt) {
			case 'C':
				_repeat		= atoi(optarg);
				break;
			case 'T':
				print_usage();
				break;
			case 'd':
				g_debug		= 1;
				break;
			case 'e':
				g_phyid		= atoi(optarg);
				break;
			case 'h':
			default: /* '?' */
				print_usage();
				exit(EINVAL);
		}
	}


	if (_repeat < 0) {
		goto _exit;
	}

	g_repeat = _repeat;

	return 0;

_exit:
	print_usage();
	exit(EINVAL);
}


int dump_packet(void)
{
	int i;
	struct ip *iphdr;
	char *tmp;

	//	Check src, dest mac address
	printf("dest mac : %02x:%02x:%02x:%02x:%02x:%02x src mac : %02x:%02x:%02x:%02x:%02x:%02x\n", 
		recvBuffer[0],recvBuffer[1],recvBuffer[2],recvBuffer[3],recvBuffer[4],recvBuffer[5],
		recvBuffer[6],recvBuffer[7],recvBuffer[8],recvBuffer[9],recvBuffer[10],recvBuffer[11]);

	//	ip address
	printf("ethernet type : 0x%02x%02x\n", recvBuffer[13], recvBuffer[12]);

	//	Check IP Adress
	iphdr = (struct ip*)(recvBuffer + 14);

	tmp = (char*)&iphdr->ip_src;
	printf("dst ip : %d.%d.%d.%d  ", tmp[0],tmp[1],tmp[2],tmp[3] );

	tmp = (char*)&iphdr->ip_dst;
	printf("src ip : %d.%d.%d.%d\n", tmp[0],tmp[1],tmp[2],tmp[3] );

	//	Check DATA
	tmp = recvBuffer + 14 + IP4_HDRLEN + UDP_HDRLEN;
	printf("Data : ");
	for( i=0 ; i<DATA_SIZE ; i++ )
	{
		printf("%c", tmp[i]);
	}
	printf("\n");

	return 0;
}


int gmac_test (void)
{
	pthread_t th_sender;
	pthread_t th_receiver; 
	int ret_snd;
	int ret_rcv;
#ifdef RECV_UDP
#else
	struct ip *iphdr;
	unsigned char *tmp;
	unsigned char src_mac[6];
#endif

	int err = 0;


	if( 0 != pthread_create( &th_sender, NULL, send_func, NULL ) )
	{
		printf("Send thread create failed!!!\n");
		err = 1;
	}
	if( 0 != pthread_create( &th_receiver, NULL, recv_func, NULL ) )
	{
		printf("Receive thread create failed!!!\n");
		err = 1;
	}


	pthread_join(th_sender, (void *)&ret_snd);
	pthread_join(th_receiver, (void *)&ret_rcv);

	if (err || ret_snd || ret_rcv)
		goto errout;

	//	Check src mac address
#ifdef RECV_UDP
	nx_debug("Data Compare : ");
	if (0 == strncmp(recvBuffer, DATA_STRING, DATA_SIZE)) {
		nx_debug("OK\n");
	}
	else {
		nx_debug(" compare Failed\n");
		goto errout;
	}
#else
	get_src_mac_addr( INTERFACE_NAME, src_mac );
	if( 0 != memcmp( recvBuffer+6, src_mac, 6 ) )
	{
		printf("Source mac address miss-match(recv:%02x:%02x:%02x:%02x:%02x:%02x, send:%02x:%02x:%02x:%02x:%02x:%02x)\n",
		recvBuffer[0],recvBuffer[1],recvBuffer[2],recvBuffer[3],recvBuffer[4],recvBuffer[5],
		src_mac[0],src_mac[1],src_mac[2],src_mac[3],src_mac[4],src_mac[5] );
		goto errout;
	}

	//	Check IP Address
	iphdr = (struct ip*)(recvBuffer + 14);
	if( 0 != strncmp( inet_ntoa(iphdr->ip_src), SRC_IP_ADDR, strlen(inet_ntoa(iphdr->ip_src)) ) )
	{
		printf("Source IP Address miss-match\n");
		goto errout;
	}

	if( 0 != strncmp( inet_ntoa(iphdr->ip_dst), SRC_IP_ADDR, strlen(inet_ntoa(iphdr->ip_src)) ) )
	{
		printf("Destination IP Address miss-match\n");
		printf("\trcv: %s\n", inet_ntoa(iphdr->ip_dst));
		printf("\tsrc: %s\n", SRC_IP_ADDR);
		goto errout;
	}

	//	Check Data String
	tmp = recvBuffer + 14 + IP4_HDRLEN + UDP_HDRLEN;
	if( 0 != strncmp(tmp, DATA_STRING, DATA_SIZE) )
	{
		printf("Data String Error\n");
		goto errout;
	}

	dump_packet();
#endif	/* RECV_UDP */

	printf("Loopback Test OK!!!\n");

	return 0;
errout:
	dump_packet();

	return (-1);
}

int waiting_link_up(void)
{
	int retries = 10;

	while (retries--) {
		if( 1 == is_link_up(INTERFACE_NAME) )
			break;

		usleep(100000);		/* sleep 100 ms */
	}

	if (retries == 0) {
		printf("%s interface link down!(Check cable connection !!!)\n", INTERFACE_NAME);
		return (-1);
	}

	return 0;
}


int test(void)
{
	int test_retries = TEST_RESTRIES;


	pthread_mutex_init( &sync_mutex, NULL );
	pthread_cond_init( &sync_cond, NULL );

	while (test_retries > 0) {
		if (gmac_test() == 0)
			break;
		test_retries--;
	}

	pthread_mutex_destroy( &sync_mutex ); 
	pthread_cond_destroy( &sync_cond );

	if (test_retries == 0) {
		printf("Test failed!\n");
		return -1;
	}
	return 0;
}




#ifdef TARGET_ASB

/* ****************************************************************
 *  Muxer init
 * ****************************************************************/

#define MMC_MUX_SEL		PAD_GPIO_C + 4 
#define MMC_MUX_EN		PAD_GPIO_E + 13

#define MMC_SEL_OFF		0
#define MMC_SEL_CH1		1
#define MMC_SEL_CH2		2

static void mux_en(int on)
{
	gpio_set_value(MMC_MUX_EN , on);
}

static void mmc_sel(int ch)
{
	if(ch == MMC_SEL_CH1)
	{
		gpio_set_value(MMC_MUX_SEL , 0);
		//gpio_set_value(MMC2_DET_CON, 1);
		//gpio_set_value(MMC1_DET_CON, 0);
	}
	else if(ch == MMC_SEL_CH2 )
	{
		gpio_set_value(MMC_MUX_SEL , 1);
		//gpio_set_value(MMC1_DET_CON, 1);
		//gpio_set_value(MMC2_DET_CON, 0);
	}
	else if(ch == MMC_SEL_OFF)
	{
		gpio_set_value(MMC_MUX_SEL , 0);
		//gpio_set_value(MMC1_DET_CON, 1);
		//gpio_set_value(MMC2_DET_CON, 1);
	}
}

static int det_init(void)
{	
#if 0
	if(gpio_export(MMC1_DET_CON))
	{	
		printf("init CD Det pin err\n");
		return -1;
	}	
	if(gpio_dir_out(MMC1_DET_CON))
	{
		printf("init CD Det pin err\n");
		return -1;
	}
	if(gpio_export(MMC2_DET_CON))
	{	
		printf("init CD Det pin err\n");
		return -1;
	}	
	if(gpio_dir_out(MMC2_DET_CON))
	{
		printf("init CD Det pin err\n");
		return -1;
	}
#endif
	if(gpio_export(MMC_MUX_SEL))
	{	
		printf("init MUX_SEL CON pin err\n");
		return -1;
	}	
	if(gpio_dir_out(MMC_MUX_SEL))
	{
		printf("init MUX_SEL CON pin err\n");
		return -1;
	}
	if(gpio_export(MMC_MUX_EN))
	{
		printf("init MUX_EN CON pin err\n");
		return -1;
	}
	if(gpio_dir_out(MMC_MUX_EN))
	{
		printf("init MUX_EN CON pin err\n");
		return -1;
	}
	mmc_sel(MMC_SEL_OFF);
	return 0;
}

static int det_deinit(void)
{
	//gpio_unexport(MMC1_DET_CON);
	//gpio_unexport(MMC2_DET_CON);
	return 0;
}

int mux_init(void)
{
	nx_debug(" ==== ASB muxer set =============================================\n");

	mux_en(1);

	det_init();

	mux_en(0);
	return 0;
}

int mux_deinit(void)
{
	nx_debug(" ==== ASB muxer unset =============================================\n");

	det_deinit();

	return 0;
}

#endif








/* ================== DEBUGGING START ===================== */

void print_running_cpu(void)
{
	int cpu;
	cpu = sched_getcpu();

	printf(" > currently %s running on CPU #%d.\n", PROG_NAME, cpu);
}

void set_cpu_affinity(void)
{
	int cpu = 1;					/* run cpu 1 : 0 test ok */

	cpu_set_t cpu_set;
	CPU_ZERO(&cpu_set);
	CPU_SET(cpu, &cpu_set);
	sched_setaffinity(0, sizeof(cpu_set_t), &cpu_set);

	printf(" * setting cpu affinity : [%d]\n", cpu);
}

void print_cpu_affinity(void)
{
	int i, count;

	cpu_set_t cpu_set;
	CPU_ZERO(&cpu_set);
	sched_getaffinity(0, sizeof(cpu_set_t), &cpu_set);

	count = CPU_COUNT(&cpu_set);

	printf(" * affinity count : %d(", count);
	for (i = 0; i < count; i++) {
		if (CPU_ISSET(i, &cpu_set))
			printf(" %d", i);
	}
	printf(")\n");
}

void pin_cpu(void)
{
	print_running_cpu();

	print_cpu_affinity();
	set_cpu_affinity();
	print_cpu_affinity();

	print_running_cpu();
}

/* ================== DEBUGGING END ======================= */







/* ****************************************************************
 *  Main
 * ****************************************************************/

int main (int argc, char **argv)
{
	char sysCmd[64];

	int r;


	pin_cpu();	// w/a translation fault issue

	parse_opt(argc, argv);

	fprintf(stdout, "Loopback Test Program (ver. %s) start ...\n", VERSION_STRING);

	//	Check Linux Interface & IP Settings
	usleep(1000);
	sprintf(sysCmd, "ifconfig %s down", INTERFACE_NAME);
	if (-1 == system(sysCmd))
	{
		printf("# interface down failed.\n");
		exit(-1);
	}
	usleep(500);
	sprintf( sysCmd, "ifconfig %s hw ether %s", INTERFACE_NAME, MAC_ADDR);
	if( -1 == system(sysCmd) )
	{
		printf("# interface hw addr setup failed.\n");
		exit(-1);
	}
	usleep(500);
	sprintf( sysCmd, "ifconfig %s %s up", INTERFACE_NAME, DST_IP_ADDR );
	if( -1 == system(sysCmd) )
	{
		printf("# ifconfig failed check device driver or physical connection\n");
		exit(-1);
	}

	if (waiting_link_up() < 0) {
		return -1;
	}


#ifdef TARGET_ASB
	mux_init();
#endif

	basic_mii();			/* including loopback settings */

	for (r = 1; r <= g_repeat; r++) {
		printf("[TRY #%d] ....\n", r);
		if (test() < 0) {
			exit(EBADMSG);
		}
	}

	sprintf(sysCmd, "ifconfig %s down", INTERFACE_NAME);
	if (-1 == system(sysCmd))
	{
		printf("# interface down failed.\n");
	}

#ifdef TARGET_ASB
	mux_deinit();
#endif

	exit(EXIT_SUCCESS);
}
