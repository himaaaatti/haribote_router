#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <linux/if.h>
#include <net/ethernet.h>
#include <netpacket/packet.h>
#include <netinet/if_ether.h>

int init_raw_socket(const char* device, bool promisc_flag, bool ip_only)
{
    struct ifreq ifreq;
    struct sockaddr_ll sa;
    int sock, result;

    char* error_msg[] = {"socket", "ioctl", "bind"};
    int msg_index;
    enum {
        SOCKET,
        IOCTL,
        BIND,
    };

    if (ip_only) {
        sock = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_IP));
        if (sock < 0) {
            msg_index = SOCKET;
            goto error;
        }
    } else {
        sock = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
        if (sock < 0) {
            msg_index = SOCKET;
            goto error;
        }
    }

    memset(&ifreq, 0, sizeof(struct ifreq));
    strncpy(ifreq.ifr_name, device, sizeof(ifreq.ifr_name) - 1);
    result = ioctl(sock, SIOCGIFINDEX, &ifreq);
    if (result < 0) {
        msg_index = IOCTL;
        goto error;
    }

    sa.sll_family = PF_PACKET;
    if (ip_only) {
        sa.sll_protocol = htons(ETH_P_IP);
    } else {
        sa.sll_protocol = htons(ETH_P_ALL);
    }

    sa.sll_ifindex = ifreq.ifr_ifindex;

    result = bind(sock, (struct sockaddr*)&sa, sizeof(sa));
    if (result < 0) {
        msg_index = BIND;
        goto error;
    }

    if (promisc_flag) {
        result = ioctl(sock, SIOCGIFFLAGS, &ifreq);
        if (result < 0) {
            msg_index = IOCTL;
            goto error;
        }
        ifreq.ifr_flags = ifreq.ifr_flags | IFF_PROMISC;

        result = ioctl(sock, SIOCSIFFLAGS, &ifreq);
        if (result < 0) {
            msg_index = IOCTL;
            goto error;
        }
    }

    return sock;

error:
    perror(error_msg[msg_index]);
    if (sock >= 0) {
        close(sock);
    }
    return -1;
}

// transrate MAC addr into string.
char* my_ether_ntoa_r(uint8_t* hwaddr, char* buf, socklen_t size)
{
    snprintf(buf,
             size,
             "%02x:%02x:%02x:%02x:%02x:%02x:",
             hwaddr[0],
             hwaddr[1],
             hwaddr[2],
             hwaddr[3],
             hwaddr[4],
             hwaddr[5]);

    return buf;
}

void print_ether_header(struct ether_header* eth, FILE* fp)
{
    char buf[80];
    fprintf(fp, "ether_header-------------------------\n");
    fprintf(fp,
            "ether_dhost=%s\n",
            my_ether_ntoa_r(eth->ether_dhost, buf, sizeof(buf)));
    fprintf(fp,
            "ether_shost=%s\n",
            my_ether_ntoa_r(eth->ether_shost, buf, sizeof(buf)));

    fprintf(fp, "ether_type=%02X", ntohs(eth->ether_type));
    switch (ntohs(eth->ether_type)) {
        case ETH_P_IP:
            fprintf(fp, "(IP)\n");
            break;

        case ETH_P_IPV6:
            fprintf(fp, "(IPv6)\n");
            break;
        case ETH_P_ARP:
            fprintf(fp, "(ARP)\n");
            break;
        default:
            fprintf(fp, "(unknown)\n");
    }
}

int main(int argc, char const* argv[])
{
    int sock, size;
    uint8_t buf[2048];

    if (argc <= 1) {
        fprintf(stderr, "ltest device-name\n");
        return 1;
    }

    sock = init_raw_socket(argv[1], 0, 0);
    if(sock == -1){
        fprintf(stderr, "init_raw_socket: error: %s\n", argv[1]);
        return -1;
    }

    while(1){
/*         struct sockaddr_ll from; */
/*         socklen_t fromlen; */
/*         memset(&from, 0, sizeof(from)); */

/*         fromlen = sizeof(from); */
/*         size = recvfrom(sock, buf, sizeof(buf), 0, (struct sockaddr *)&from, &fromlen); */

        size = read(sock, buf, sizeof(buf));
        if(size <= 0){
            perror("read");
        }
        else{
            if(size >= sizeof(struct ether_header)){
                print_ether_header((struct ether_header *)buf, stdout);
            }
            else{
                fprintf(stderr, "read size(%d) < %lu\n", size, sizeof(struct ether_header));
            }
        }
    }

    close(sock);
    return 0;
}
