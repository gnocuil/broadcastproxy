#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/socket.h>
#include <linux/if_ether.h>
#include <linux/if_packet.h>
#include <netinet/ip.h>
#include <netinet/udp.h> 
#include <net/if.h>
#include <net/if_arp.h>

uint16_t port;
char iface[100];
char oface[100];
int raw_fd;

struct sockaddr_ll recvDev;
struct sockaddr_ll sendDev;

void usage()
{
    puts("Usage: bp --port <UDP_port> -i <recv_interface_name> -o <send_interface_name>");
    puts("Example: bp --port 6000 -i eth0 -o eth1");
    exit(0);
}

void socket_init()
{
    bzero(&sendDev,sizeof(sendDev));    
    sendDev.sll_family = AF_PACKET;
    sendDev.sll_halen = htons (ETH_ALEN);
    sendDev.sll_hatype = ARPHRD_ETHER;
    sendDev.sll_pkttype = PACKET_HOST;
    sendDev.sll_protocol = htons(ETH_P_ALL);
    bzero(&recvDev,sizeof(recvDev));    
    recvDev.sll_family = AF_PACKET;
    recvDev.sll_halen = htons (ETH_ALEN);
    recvDev.sll_hatype = ARPHRD_ETHER;
    recvDev.sll_pkttype = PACKET_HOST;
    recvDev.sll_protocol = htons(ETH_P_ALL);

    recvDev.sll_ifindex = if_nametoindex(iface);    
    sendDev.sll_ifindex = if_nametoindex(oface);

    raw_fd = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if (raw_fd <= 0) {
        printf("Error creating packet socket\n");
        exit(0);
    }
    if (bind(raw_fd, (struct sockaddr*)&recvDev, sizeof(recvDev)) < 0) {
        printf("Error binding to interface %s\n", iface);
        exit(0);
    }
}

uint8_t buf[65536];

int socket_recv() {
    int count = recv(raw_fd, buf, 65536, 0);
    return  count;
}

void socket_send() {
    sendto(raw_fd, buf, 65536, 0, (struct sockaddr *) &sendDev, sizeof(sendDev));
}

int isbcast_mac()
{
    return buf[0] == 0xff && buf[1] == 0xff && buf[2] == 0xff && buf[3] == 0xff && buf[4] == 0xff && buf[5] == 0xff;
}

int isIPv4()
{
    return buf[12] == 0x08 && buf[13] == 0x00;
}

int main(int argc, char **argv)
{
    for (int i = 1; i + 1< argc; ++i) {
        if (strcmp(argv[i], "--port")==0) {
            int tmp;
            sscanf(argv[i+1], "%d", &tmp);
            port = (uint16_t)tmp;
            ++i;
        } else if (strcmp(argv[i], "-i")==0) {
            strcpy(iface, argv[i+1]);
            ++i;
        } else if (strcmp(argv[i], "-o")==0) {
            strcpy(oface, argv[i+1]);
            ++i;
        } else {puts("#1");
            usage();
        }
    }puts("#2");
    if (strlen(iface) == 0 || strlen(oface) == 0)
        usage();
    printf("listen on %s, UDP port %d, to %s\n", iface, port, oface);
    
    socket_init();
    do {
        int cnt = socket_recv();
        if (!isbcast_mac()) 
            continue;
for (int i = 0; i < 20; ++i) printf("%x ", buf[i] & 0xff);printf("\n");
        if (!isIPv4())
            continue;
        puts("ipv4!");
        iphdr *ip = (iphdr*)(buf + 14);
        if (ip->protocol == IPPROTO_UDP) {
            udphdr* udp = (udphdr*)(buf + 14 + 20);
            printf("UDP: from %d to %d\n", udp->source, udp->dest); 
            if (ntohs(udp->source) == port || ntohs(udp->dest) == port) {
                socket_send();
            }
        }
    } while (true);
}
