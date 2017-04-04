/*
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 */

#include <arpa/inet.h>
#include <linux/if_packet.h>
#include <linux/ip.h>
#include <linux/udp.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/ether.h>

#define MY_DEST_MAC0 0xff
#define MY_DEST_MAC1 0xff
#define MY_DEST_MAC2 0xff
#define MY_DEST_MAC3 0xff
#define MY_DEST_MAC4 0xff
#define MY_DEST_MAC5 0xff

//ROUTER <--> sendRawEth.c <--> DEST
#define ROUTER_IP0  10
#define ROUTER_IP1  32
#define ROUTER_IP2  162
#define ROUTER_IP3  136

#define DEST_IP0  10
#define DEST_IP1  32
#define DEST_IP2  162
#define DEST_IP3  1

#define ETHER_TYPE	0x0800

#define DEFAULT_IF	"enp0s25"
#define BUF_SIZ		1518

struct arp_packet {
        uint16_t hw_type;
        uint16_t prot_type;
        uint8_t  hlen;
        uint8_t  dlen;
        uint16_t operation;
        uint8_t  sender_hwaddr[6];
        uint8_t  sender_ip[4];
        uint8_t  target_hwaddr[6];
        uint8_t  target_ip[4];
};

union arp_packet_u {
        struct arp_packet arp;
        uint8_t raw_data[sizeof(struct arp_packet)];
};

int main(int argc, char *argv[])
{
	int sockfd;
	struct ifreq if_idx;
	struct ifreq if_mac;
	int tx_len = 0;
	uint8_t sendbuf[BUF_SIZ];
	struct ether_header *eh = (struct ether_header *) sendbuf;
	struct iphdr *iph = (struct iphdr *) (sendbuf + sizeof(struct ether_header));
  struct arp_packet *arp_payload = (struct arp_packet *) (sendbuf + sizeof(struct ether_header) + sizeof(struct iphdr));
	struct sockaddr_ll socket_address;
	char ifName[IFNAMSIZ];

	/* Get interface name */
	if (argc > 1)
		strcpy(ifName, argv[1]);
	else
		strcpy(ifName, DEFAULT_IF);

	/* Open RAW socket to send on */
	if ((sockfd = socket(AF_PACKET, SOCK_RAW, htons(ETHER_TYPE))) == -1) {
	    perror("socket");
	}

	/* Get the index of the interface to send on */
	memset(&if_idx, 0, sizeof(struct ifreq));
	strncpy(if_idx.ifr_name, ifName, IFNAMSIZ-1);
	if (ioctl(sockfd, SIOCGIFINDEX, &if_idx) < 0)
	    perror("SIOCGIFINDEX");
	/* Get the MAC address of the interface to send on */
	memset(&if_mac, 0, sizeof(struct ifreq));
	strncpy(if_mac.ifr_name, ifName, IFNAMSIZ-1);
	if (ioctl(sockfd, SIOCGIFHWADDR, &if_mac) < 0)
	    perror("SIOCGIFHWADDR");

	/* Construct the Ethernet header */
	memset(sendbuf, 0, BUF_SIZ);
	/* Ethernet header */
	eh->ether_shost[0] = ((uint8_t *)&if_mac.ifr_hwaddr.sa_data)[0];
	eh->ether_shost[1] = ((uint8_t *)&if_mac.ifr_hwaddr.sa_data)[1];
	eh->ether_shost[2] = ((uint8_t *)&if_mac.ifr_hwaddr.sa_data)[2];
	eh->ether_shost[3] = ((uint8_t *)&if_mac.ifr_hwaddr.sa_data)[3];
	eh->ether_shost[4] = ((uint8_t *)&if_mac.ifr_hwaddr.sa_data)[4];
	eh->ether_shost[5] = ((uint8_t *)&if_mac.ifr_hwaddr.sa_data)[5];
	eh->ether_dhost[0] = MY_DEST_MAC0;
	eh->ether_dhost[1] = MY_DEST_MAC1;
	eh->ether_dhost[2] = MY_DEST_MAC2;
	eh->ether_dhost[3] = MY_DEST_MAC3;
	eh->ether_dhost[4] = MY_DEST_MAC4;
	eh->ether_dhost[5] = MY_DEST_MAC5;
	/* Ethertype field */
  eh->ether_type = htons(ETH_P_ARP);
	tx_len += sizeof(struct ether_header);

	/* Fill ARP header */
	arp_payload->hw_type = htons(ARPHRD_ETHER);    //hardware type = Ethernet
	arp_payload->prot_type = htons(ETH_P_IP);      //protocol type = IPv4
	arp_payload->hlen = ETH_ALEN;                  //Ethernet addresses are 8 octets
	arp_payload->dlen = 4;                         //IPv4 addresses are 4 octets
	arp_payload->operation = htons(2);             //1 for REQUEST, 2 for REPLY

	arp_payload->sender_hwaddr[0] = ((uint8_t *)&if_mac.ifr_hwaddr.sa_data)[0];
	arp_payload->sender_hwaddr[1] = ((uint8_t *)&if_mac.ifr_hwaddr.sa_data)[1];
	arp_payload->sender_hwaddr[2] = ((uint8_t *)&if_mac.ifr_hwaddr.sa_data)[2];
	arp_payload->sender_hwaddr[3] = ((uint8_t *)&if_mac.ifr_hwaddr.sa_data)[3];
	arp_payload->sender_hwaddr[4] = ((uint8_t *)&if_mac.ifr_hwaddr.sa_data)[4];
	arp_payload->sender_hwaddr[5] = ((uint8_t *)&if_mac.ifr_hwaddr.sa_data)[5];

	arp_payload->sender_ip[0] = 10;  //ROUTER_IP0;
	arp_payload->sender_ip[1] = 32;  //ROUTER_IP1;
	arp_payload->sender_ip[2] = 162; //ROUTER_IP2;
	arp_payload->sender_ip[3] = 136; //ROUTER_IP3;

	arp_payload->target_hwaddr[0] = MY_DEST_MAC0;
	arp_payload->target_hwaddr[1] = MY_DEST_MAC1;
	arp_payload->target_hwaddr[2] = MY_DEST_MAC2;
	arp_payload->target_hwaddr[3] = MY_DEST_MAC3;
	arp_payload->target_hwaddr[4] = MY_DEST_MAC4;
	arp_payload->target_hwaddr[5] = MY_DEST_MAC5;

	arp_payload->target_ip[0] = 0; //DEST_IP0;
	arp_payload->target_ip[1] = 0; //DEST_IP1;
	arp_payload->target_ip[2] = 0; //DEST_IP2;
	arp_payload->target_ip[3] = 0; //DEST_IP3;

  printf("ethernet payload size = %ld\n", sizeof(struct arp_packet));
  tx_len += sizeof(struct arp_packet);

  /* Index of the network device */
	socket_address.sll_ifindex = if_idx.ifr_ifindex;
	/* Address length*/
	socket_address.sll_halen = ETH_ALEN;
	/* Destination MAC */
	socket_address.sll_addr[0] = MY_DEST_MAC0;
	socket_address.sll_addr[1] = MY_DEST_MAC1;
	socket_address.sll_addr[2] = MY_DEST_MAC2;
	socket_address.sll_addr[3] = MY_DEST_MAC3;
	socket_address.sll_addr[4] = MY_DEST_MAC4;
	socket_address.sll_addr[5] = MY_DEST_MAC5;

	/* Send packet */
  if (sendto(sockfd, sendbuf, tx_len, 0, (struct sockaddr*)&socket_address, sizeof(struct sockaddr_ll)) < 0)
	    printf("Send failed\n");

	return 0;
}
