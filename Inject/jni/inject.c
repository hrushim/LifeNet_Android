/*
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

Project: MyMANET - A Platform to Build Customized MANET's
Developers: Ashwin Paranjpe, Hrushikesh Mehendale, Santosh Vempala
Georgia Institute of Technology, Atlanta, USA
 */


#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include<stdint.h>	/*for uint32_t*/
#include<sys/socket.h>
#include<features.h>
#include<linux/if_packet.h>
#include<linux/if_ether.h>
#include<errno.h>
#include<ctype.h>
#include<sys/ioctl.h>
#include<net/if.h>
#include<net/ethertypes.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include"inject.h"

int g_additional_len;
int g_is_gw_flag = 0;
int packet_len = 0;
uint8_t g_mac[6];
uint8_t curr_session_id = 0;

// Creates a raw socket
int create_raw_socket(int protocol_to_send) {
    int raw_sock;

    if ((raw_sock = socket(PF_PACKET, SOCK_RAW, htons(protocol_to_send))) == -1) {
        perror("Error creating raw socket: ");
        exit(-1);
    }

    return raw_sock;
}

// Binds the created raw socket to an interface
int bind_raw_socket_to_interface(char *device, int raw_sock, int protocol) {

    struct sockaddr_ll sll;
    struct ifreq ifr;
    unsigned char *mac;

    bzero(&sll, sizeof (sll));
    bzero(&ifr, sizeof (ifr));

    /*Copy device name*/
    strncpy((char *) ifr.ifr_name, device, IFNAMSIZ);

    /*Get hardware address */
    if (ioctl(raw_sock, SIOCGIFHWADDR, &ifr) < 0) {
        printf("ioctl Error: SIOCGIFHWADDR");
        exit(1);
    }

    memset(g_mac, 0, 6);
    memcpy(g_mac, ifr.ifr_hwaddr.sa_data, 6);

#if DEBUG
    printf("\nMy Mac is : %x:%x:%x:%x:%x:%x\n", g_mac[0], g_mac[1], g_mac[2], g_mac[3], g_mac[4], g_mac[5]);
#endif

    /*Get the Interface Index  */
    if ((ioctl(raw_sock, SIOCGIFINDEX, &ifr)) == -1) {
        printf("Error getting Interface index !\n");
        exit(-1);
    }

    /*Bind Raw socket to this interface*/
    sll.sll_family = AF_PACKET;
    sll.sll_ifindex = ifr.ifr_ifindex;
    sll.sll_protocol = htons(protocol);

    if ((bind(raw_sock, (struct sockaddr *) & sll, sizeof (sll))) == -1) {
        perror("Error binding raw socket to interface\n");
        exit(-1);
    }
    return 1;

}

// Sends the packet on the raw socket
int SendRawPacket(int rawsock, unsigned char *pkt, int pkt_len) {

    int sent = 0;
#if DEBUG
    printf("\nPacket len: %d\n\n", pkt_len);
#endif

    if ((sent = write(rawsock, pkt, pkt_len)) != pkt_len) {
        /* Print Error */
        printf("Could only send %d bytes of packet of length %d\n", sent, pkt_len);
        return 0;
    }

    return 1;

}

// Creates the GNSM or heartbeat packet to be injected into the network
unsigned char* create_gnsm_packet(uint8_t *src_mac, uint8_t *dst_mac, int protocol, char ip_address[15], char node_name[10], uint8_t is_gw) {

    char wan_if[10];
    struct ethhdr *eth_header = NULL;
    unsigned char *buf = NULL, *packet = NULL, *start = NULL;

    /*Create ethernet header -> Src MAC, Dest MAC, Protocol type*/
    eth_header = (struct ethhdr *) malloc(sizeof (struct ethhdr) + (sizeof (char) * BUFFER_LEN));
    create_ethernet_header(eth_header, src_mac, dst_mac, protocol);
    buf = (unsigned char *) ((char *) eth_header + sizeof (struct ethhdr));
    start = (char *) eth_header;

    /* Pack SRC_MAC, IP_ADD, Node_name */
    buf += pack_curr_node_info(buf, ip_address, src_mac, node_name);
    /********** TODO: Check for presence of GW here!!! and pack **********/
    is_gw = 0;
    /*Pack the GW flag status*/
    buf += pack(buf, (uint8_t *) & is_gw, 1);
#if PRINT_PACKET
    printf("[%x]", (uint8_t)*(buf - 1));
    fflush(stdout);
#endif
    /*Pack DNS and call iptables scripts */
    buf += handle_dns_and_iptables(buf, is_gw);
    /*Pack txstats information*/
    buf += read_and_pack_txstats(buf);
    /*Pack rxstats information*/
    buf += read_and_pack_rxstats(buf);
    /*Return packet*/
    packet_len = buf - start;
    packet = (char *) eth_header;
    return ((unsigned char *) packet);

}

// Copies the data into the buffer
int pack(unsigned char *ptr, uint8_t *src_mac, int num_bytes) {

    memcpy(ptr, src_mac, num_bytes);
    return (num_bytes);
}

// Creates ethernet header
void create_ethernet_header(struct ethhdr *ptr, uint8_t *src_mac, uint8_t *dst_mac, int protocol) {

    char dst_mac_temp[6], mac_temp[6];

    memset(dst_mac_temp, '\0', 6);
    memset(mac_temp, '\0', 6);

    if (ptr == NULL) {
        printf("\nCould not allocate memory to create ethernet header\n");
        exit(1);
    }
    memset(ptr, 0, sizeof (struct ethhdr) + (sizeof (char) * BUFFER_LEN));
    memcpy(ptr->h_source, g_mac, 6);
    if (string_to_byte_order_etheraddr(dst_mac, dst_mac_temp) != 0) {
        printf("\nError in converting ethernet address string to byte order\n"); fflush(stdout);
        exit(1);
    }
    memcpy(ptr->h_dest, (char *) dst_mac_temp, 6);
    ptr->h_proto = htons(protocol);

#if PRINT_PACKET
    uint8_t *tmp = (uint8_t *) ptr;
    printf("[%x %x %x %x %x %x]", *tmp, *(tmp + 1), *(tmp + 2), *(tmp + 3), *(tmp + 4), *(tmp + 5));
    tmp += 6;
    printf("[%x %x %x %x %x %x]", *tmp, *(tmp + 1), *(tmp + 2), *(tmp + 3), *(tmp + 4), *(tmp + 5));
    tmp += 6;
    printf("[%x %x]", *tmp, *(tmp + 1));
    fflush(stdout);
#endif
}

// If current node is a gateway packs the DNS information as well
int handle_dns_and_iptables(unsigned char *buf, int is_gw) {

    char filename[100];
    int offset = 0;
    if (is_gw == 1) {

        /*If GW present pack DNS addresses into packet*/
        offset += read_and_pack_dns(buf);
        buf += offset;

        /*This system command configures the iptables as this node is now the gateway*/
        if (g_is_gw_flag == 0) {
            memset(filename, '\0', 100);
#if IS_EMB_DEV
            sprintf(filename, "%s", FIREWALL_FWD_ALL_FILE_NAME);
#else
            sprintf(filename, "bash %s%s", getenv("HOME"), FIREWALL_FWD_ALL_FILE_NAME);
#endif
            system(filename);
            g_is_gw_flag = 1;
        }

        return offset;

    } else {
        g_is_gw_flag = 0;

        return offset;
    }
}

// Packs the primary information like ip address, mac address and nodename
int pack_curr_node_info(unsigned char *buf, char ip_address[15], uint8_t *src_mac, char node_name[10]) {

    uint32_t ip_long = 0, ip_long_nbyte_order = 0, offset = 0;

    ip_long = inet_addr(ip_address);
    ip_long_nbyte_order = htonl(ip_long);

    buf += pack(buf, g_mac, 6);
    offset += 6;
#if DEBUG
    printf("\nCurrent node info ->");
    printf("\n\tSource MAC: %x %x %x %x %x %x", *(buf - 6), *(buf - 5), *(buf - 4), *(buf - 3), *(buf - 2), *(buf - 1));
#endif
#if PRINT_PACKET
    printf("[%x %x %x %x %x %x]", (uint8_t)*(buf - 6), (uint8_t)*(buf - 5), (uint8_t)*(buf - 4), (uint8_t)*(buf - 3), (uint8_t)*(buf - 2), (uint8_t)*(buf - 1));
#endif

    buf += pack(buf, (uint8_t *) & ip_long_nbyte_order, 4);
    offset += 4;
#if DEBUG
    struct in_addr temp_ip_in_addr;
    temp_ip_in_addr.s_addr = ntohl(ip_long_nbyte_order);
    printf("\n\tIP Address -> %s (%x %x %x %x)", (char *) inet_ntoa(temp_ip_in_addr), *(buf - 4), *(buf - 3), *(buf - 2), *(buf - 1));
    fflush(stdout);
#endif
#if PRINT_PACKET
    printf("[%x %x %x %x]", (uint8_t)*(buf - 4), (uint8_t)*(buf - 3), (uint8_t)*(buf - 2), (uint8_t)*(buf - 1));
#endif

    buf += pack(buf, (uint8_t *) node_name, 10);
    offset += 10;
#if DEBUG
    printf("\n\tNode name -> %s (%x %x %x %x %x %x %x %x %x %x)", node_name, *(buf - 10), *(buf - 9), *(buf - 8), *(buf - 7), *(buf - 6), *(buf - 5), *(buf - 4), *(buf - 3), *(buf - 2), *(buf - 1));
    fflush(stdout);
#endif
#if PRINT_PACKET
    printf("[%x %x %x %x %x %x %x %x %x %x]", (uint8_t)*(buf - 10), (uint8_t)*(buf - 9), (uint8_t)*(buf - 8), (uint8_t)*(buf - 7), (uint8_t)*(buf - 6), (uint8_t)*(buf - 5), (uint8_t)*(buf - 4), (uint8_t)*(buf - 3), (uint8_t)*(buf - 2), (uint8_t)*(buf - 1));
    fflush(stdout);
#endif


#if DEBUG
    printf("\nReturning %d", offset);
#endif
    return offset;
}

int string_to_byte_order_etheraddr(const u_char *asc, char *addr) {
    int cnt;

    for (cnt = 0; cnt < 6; ++cnt) {
        unsigned int number;
        char ch;

        ch = tolower(*asc++);
        if ((ch < '0' || ch > '9') && (ch < 'a' || ch > 'f'))
            return 1;
        number = isdigit(ch) ? (ch - '0') : (ch - 'a' + 10);

        ch = tolower(*asc);
        if ((cnt < 5 && ch != ':') || (cnt == 5 && ch != '\0' && !isspace(ch))) {
            ++asc;
            if ((ch < '0' || ch > '9') && (ch < 'a' || ch > 'f'))
                return 1;
            number <<= 4;
            number += isdigit(ch) ? (ch - '0') : (ch - 'a' + 10);
            ch = *asc;
            if (cnt < 5 && ch != ':')
                return 1;
        }

        /* Store result.  */
        addr[cnt] = (unsigned char) number;

        /* Skip ':'.  */
        ++asc;
    }

    return 0;
}

// Pack data from /proc/rxstats i.e. receive statistics
int read_and_pack_rxstats(char *buf) {

    char line[80];
    char mac_temp[6];
    FILE *fptr;
    int offset = 0;
    uint8_t line_count = 0;
    /*Open file for reading */

    fptr = fopen(RX_FILE_NAME, "r");
    if (fptr == NULL) {
        printf("\n%s file could not be read\n", RX_FILE_NAME);
        exit(1);
    }

    while ((fgets(line, 80, fptr) != NULL)) {
        char *mac = strtok(line, " ");
        if (string_to_byte_order_etheraddr(mac, mac_temp) != 0) {
            printf("\n2: Error in string_etheraddr\n");
            exit(1);
        }
        if (memcmp(g_mac, mac_temp, 6) != 0) {
            line_count++;
        }
    }

    buf += pack(buf, (uint8_t *) (int) & line_count, 1);
    offset += 1;
#if PRINT_PACKET
    printf("[%x]", (uint8_t)*(buf - 1));
    fflush(stdout);
#endif
#if DEBUG
    printf("\nHere %d", line_count);
    fflush(stdout);
#endif

    if (fptr)
        fclose(fptr);

    memset(&line, '\0', sizeof (line));
    memset(&mac_temp, '\0', sizeof (mac_temp));

    fptr = fopen(RX_FILE_NAME, "r");
    if (fptr == NULL) {
        printf("\n%s file could not be read\n", RX_FILE_NAME);
        exit(1);
    }

    /*Read file line by line and Pack MAC, #rx_packets and #session_id*/
    while ((fgets(line, 80, fptr) != NULL)) {

#if DEBUG
        printf("%s\n", line);
#endif

        char *mac = strtok(line, " ");
        char *num_of_pkts_string = strtok(NULL, " ");
        char *session_id_string = strtok(NULL, " ");
        char *num_of_bcast_pkts_string = strtok(NULL, " ");

        uint32_t num_of_pkts = atoi(num_of_pkts_string);
        uint32_t num_of_bcast_pkts = atoi(num_of_bcast_pkts_string);
        uint32_t session_id = atoi(session_id_string);
        uint32_t num = htonl(num_of_pkts);
        uint32_t num_bcast = htonl(num_of_bcast_pkts);
        uint32_t session = htonl(session_id);

#if DEBUG
        printf("Reading and packing Rxstats--> \n\tmac : %s, \n\tnum_of_pkts : %d, \n\tsession : %d\n",
                mac, num_of_pkts, session_id);
#endif
        if (string_to_byte_order_etheraddr(mac, mac_temp) != 0) {
            printf("\n2: Error in string_etheraddr\n");
            exit(1);
        }

        if (memcmp(g_mac, mac_temp, 6) != 0) {

            buf += pack(buf, (uint8_t *) (int) & mac_temp, 6);
#if PRINT_PACKET
            printf("[%x %x %x %x %x %x]", (uint8_t)*(buf - 6), (uint8_t)*(buf - 5), (uint8_t)*(buf - 4), (uint8_t)*(buf - 3), (uint8_t)*(buf - 2), (uint8_t)*(buf - 1));
            fflush(stdout);
#endif
            buf += pack(buf, (uint8_t *) (int) & num, sizeof (num));
#if PRINT_PACKET
            printf("[%x %x %x %x]", (uint8_t)*(buf - 4), (uint8_t)*(buf - 3), (uint8_t)*(buf - 2), (uint8_t)*(buf - 1));
            fflush(stdout);
#endif
            buf += pack(buf, (uint8_t *) (int) & session, sizeof (session));
#if PRINT_PACKET
            printf("[%x %x %x %x]", (uint8_t)*(buf - 4), (uint8_t)*(buf - 3), (uint8_t)*(buf - 2), (uint8_t)*(buf - 1));
            fflush(stdout);
#endif
            buf += pack(buf, (uint8_t *) (int) & num_bcast, sizeof (num_bcast));
#if PRINT_PACKET
            printf("[%x %x %x %x]", (uint8_t)*(buf - 4), (uint8_t)*(buf - 3), (uint8_t)*(buf - 2), (uint8_t)*(buf - 1));
            fflush(stdout);
#endif
            offset = offset + 6 + 2 * sizeof (num) + sizeof (session);

        }

        memset(line, 0, 80);
    }
    if (fptr)
        fclose(fptr);

    return offset;

}

// Pack data from /proc/txstats i.e. transmit statistics
int read_and_pack_txstats(char *buf) {
    char line[80];
    FILE *fptr;
    int offset = 0;
    uint8_t line_count = 0;
    uint8_t mac_temp[6];


    fptr = fopen(TX_FILE_NAME, "r");
    if (fptr == NULL) {
        printf("\n %s could not be read", TX_FILE_NAME);
        exit(1);
    }

    memset(&line, '\0', sizeof (line));

    while ((fgets(line, 80, fptr) != NULL)) {
        char *mac = strtok(line, " ");
        if (string_to_byte_order_etheraddr(mac, mac_temp) != 0) {
            printf("\n2: Error in string_etheraddr\n");
            exit(1);
        }
        if (memcmp(g_mac, mac_temp, 6) != 0) {
            line_count++;
        }
    }

    buf += pack(buf, (uint8_t *) (int) & line_count, 1);
    offset += 1;
#if PRINT_PACKET
    printf("[%x]", (uint8_t)*(buf - 1));
    fflush(stdout);
#endif

    if (fptr)
        fclose(fptr);

    fptr = fopen(TX_FILE_NAME, "r");
    if (fptr == NULL) {
        printf("\n %s could not be read", TX_FILE_NAME);
        exit(1);
    }

    memset(&line, '\0', sizeof (line));

    while ((fgets(line, 80, fptr) != NULL)) {

        memset(&mac_temp, '\0', 6);
        char *mac = strtok(line, " ");

        if (string_to_byte_order_etheraddr(mac, mac_temp) != 0) {
            printf("\n2: Error in string_etheraddr\n");
            exit(1);
        }
        if (memcmp(g_mac, mac_temp, 6) != 0) {
            buf += pack(buf, (uint8_t *) (int) & mac_temp, 6);
            offset += 6;
#if PRINT_PACKET
            printf("[%x %x %x %x %x %x]", (uint8_t)*(buf - 6), (uint8_t)*(buf - 5), (uint8_t)*(buf - 4), (uint8_t)*(buf - 3), (uint8_t)*(buf - 2), (uint8_t)*(buf - 1));
            fflush(stdout);
#endif
        }

        char *num_of_pkts_string = strtok(NULL, " ");
        char *session_id_string = strtok(NULL, " ");

        uint32_t num_of_pkts = atoi(num_of_pkts_string);
        uint32_t session_id = atoi(session_id_string);
        uint32_t num_of_pkts_nbyte_order = htonl(num_of_pkts);
        uint32_t session_id_nbyte_order = htonl(session_id);

        if (memcmp(g_mac, mac_temp, 6) != 0) {
            buf += pack(buf, (uint8_t *) (int) & num_of_pkts_nbyte_order, 4);
#if PRINT_PACKET
            printf("[%x %x %x %x]", (uint8_t)*(buf - 4), (uint8_t)*(buf - 3), (uint8_t)*(buf - 2), (uint8_t)*(buf - 1));
            fflush(stdout);
#endif
            buf += pack(buf, (uint8_t *) (int) & session_id_nbyte_order, 4);
#if PRINT_PACKET
            printf("[%x %x %x %x]", (uint8_t)*(buf - 4), (uint8_t)*(buf - 3), (uint8_t)*(buf - 2), (uint8_t)*(buf - 1));
            fflush(stdout);
#endif
            offset += 8;
        }

#if DEBUG
        printf("\nReading and packing txstats for %s ->\n\tNumber of packets -> %d\n\tSession id -> %d", mac, num_of_pkts, session_id);
        fflush(stdout);
#endif

        num_of_pkts_string = strtok(NULL, " ");
        num_of_pkts_string = strtok(NULL, " ");
        num_of_pkts_string = strtok(NULL, " ");
        session_id_string = strtok(NULL, " ");
        num_of_pkts = atoi(num_of_pkts_string);
        session_id = atoi(session_id_string);
        num_of_pkts_nbyte_order = htonl(num_of_pkts);
        session_id_nbyte_order = htonl(session_id);

        if (memcmp(g_mac, mac_temp, 6) != 0) {

            buf += pack(buf, (uint8_t *) (int) & num_of_pkts_nbyte_order, 4);
#if PRINT_PACKET
            printf("[%x %x %x %x]", (uint8_t)*(buf - 4), (uint8_t)*(buf - 3), (uint8_t)*(buf - 2), (uint8_t)*(buf - 1));
            fflush(stdout);
#endif
            buf += pack(buf, (uint8_t *) (int) & session_id_nbyte_order, 4);
#if PRINT_PACKET
            printf("[%x %x %x %x]", (uint8_t)*(buf - 4), (uint8_t)*(buf - 3), (uint8_t)*(buf - 2), (uint8_t)*(buf - 1));
            fflush(stdout);
#endif
            offset += 8;

#if DEBUG
            printf("\n\n\tNumber of fwds -> %d\n\tFWD Session id -> %d\n", num_of_pkts, session_id);
            fflush(stdout);
#endif
            memset(line, 0, 80);
        }
    }

    if (fptr)
        fclose(fptr);

    return offset;
}

// Pack DNS information
int read_and_pack_dns(char *buf) {

#if DEBUG
    printf("\nInside read_and_pack_dns ->");
    fflush(stdout);
#endif


    int offset = 0;
    uint8_t num_dns_ip = 0;
    uint32_t ip_long = 0, ip_long_nbyte_order = 0;
    char dns_ip1[16], dns_ip2[16], dns_ip3[16];

    memset(&dns_ip1, '\0', 16);
    memset(&dns_ip2, '\0', 16);
    memset(&dns_ip3, '\0', 16);

    num_dns_ip = read_from_first_three_lines_of_file_having_pattern(dns_ip1, dns_ip2, dns_ip3, "/etc/resolv.conf", "nameserver", 1);

#if DEBUG
    printf("\n\tNum of DNS ip -> %d %s %s %s", num_dns_ip, dns_ip1, dns_ip2, dns_ip3);
    fflush(stdout);
#endif

    buf += pack(buf, &num_dns_ip, 1);
#if PRINT_PACKET
    printf("[%x]", (uint8_t)*(buf - 1));
#endif
    offset += 1;
#if DEBUG
    printf("\n\tAfter packing num of dns", num_dns_ip);
    fflush(stdout);
#endif
    /*Pack the number of DNS IPs that were successfully extracted, 1 < num_dns_ip <= 3*/
    if (num_dns_ip == 1) {
        /*Pack only a single DNS IP address, which was successfully extracted*/

        ip_long = inet_addr(dns_ip1);
        ip_long_nbyte_order = htonl(ip_long);
        buf += pack(buf, (uint8_t *) & ip_long_nbyte_order, 4);
        offset += 4;
#if PRINT_PACKET
        printf("[%x %x %x %x]", (uint8_t)*(buf - 4), (uint8_t)*(buf - 3), (uint8_t)*(buf - 2), (uint8_t)*(buf - 1));
        fflush(stdout);
#endif

    } else if (num_dns_ip == 2) {
        /*Pack two DNS IP addresses*/

        ip_long = inet_addr(dns_ip1);
        ip_long_nbyte_order = htonl(ip_long);
        buf += pack(buf, (uint8_t *) & ip_long_nbyte_order, 4);
        offset += 4;
#if PRINT_PACKET
        printf("[%x %x %x %x]", (uint8_t)*(buf - 4), (uint8_t)*(buf - 3), (uint8_t)*(buf - 2), (uint8_t)*(buf - 1));
        fflush(stdout);
#endif

        ip_long = inet_addr(dns_ip2);
        ip_long_nbyte_order = htonl(ip_long);
        buf += pack(buf, (uint8_t *) & ip_long_nbyte_order, 4);
        offset += 4;
#if PRINT_PACKET
        printf("[%x %x %x %x]", (uint8_t)*(buf - 4), (uint8_t)*(buf - 3), (uint8_t)*(buf - 2), (uint8_t)*(buf - 1));
        fflush(stdout);
#endif

    } else if (num_dns_ip == 3) {
        /*Pack three DNS IP addresses*/

        ip_long = inet_addr(dns_ip1);
        ip_long_nbyte_order = htonl(ip_long);
        buf += pack(buf, (uint8_t *) & ip_long_nbyte_order, 4);
        offset += 4;
#if PRINT_PACKET
        printf("[%x %x %x %x]", (uint8_t)*(buf - 4), (uint8_t)*(buf - 3), (uint8_t)*(buf - 2), (uint8_t)*(buf - 1));
        fflush(stdout);
#endif

        ip_long = inet_addr(dns_ip2);
        ip_long_nbyte_order = htonl(ip_long);
        buf += pack(buf, (uint8_t *) & ip_long_nbyte_order, 4);
        offset += 4;
#if PRINT_PACKET
        printf("[%x %x %x %x]", (uint8_t)*(buf - 4), (uint8_t)*(buf - 3), (uint8_t)*(buf - 2), (uint8_t)*(buf - 1));
        fflush(stdout);
#endif

        ip_long = inet_addr(dns_ip3);
        ip_long_nbyte_order = htonl(ip_long);
        buf += pack(buf, (uint8_t *) & ip_long_nbyte_order, 4);
        offset += 4;
#if PRINT_PACKET
        printf("[%x %x %x %x]", (uint8_t)*(buf - 4), (uint8_t)*(buf - 3), (uint8_t)*(buf - 2), (uint8_t)*(buf - 1));
        fflush(stdout);
#endif        
    }

    return offset;
}

int read_from_first_three_lines_of_file_having_pattern(char *buf1, char *buf2, char *buf3, char *file_name, char *pattern, int token_num) {

    int i = 0;
    FILE *fptr;
    char line[80];

    memset(&line, '\0', sizeof (line));
    fptr = fopen(file_name, "r");
    if (fptr == NULL) {
        printf("\nCannot open %s. Terminating...", file_name);
        exit(1);
    }

    while (fgets(line, 80, fptr) != NULL) {
        char *nameserver = NULL, *buf = NULL;

        nameserver = strtok(line, " \n\t");
        buf = strtok(NULL, " \n\t");

        if (strncmp(nameserver, "nameserver", 10) == 0) {
            if (i == 0) {
                strncpy(buf1, buf, 16);
            } else if (i == 1) {
                strncpy(buf2, buf, 16);
            } else if (i == 2) {
                strncpy(buf3, buf, 16);
            }
            i++;
            if (i == MAX_NUM_DNS_TX) {
                break;
            }
        }
        memset(&line, '\0', 80);
    }
    if (fptr) {
        fclose(fptr);
    }

    return i;

}

void read_first_line_from_file(char *buf, char *file_name) {

    FILE *fptr;

    fptr = popen(file_name, "r");
    if (fptr == NULL) {
        printf("\nCould not open %s. Terminating...", file_name);
        exit(1);
    }

    if (fgets(buf, 80, fptr) == NULL) {
        printf("\nCould not read from %s. Terminating...", file_name);
        exit(1);
    }

    pclose(fptr);

}

uint8_t check_manifold_lkm_status() {

    FILE *fp_lkm;
    uint8_t ret_value = 255;

    fp_lkm = fopen("/proc/rxstats", "r");
    if (fp_lkm == NULL) {
        return 0;
    } else {
	fclose(fp_lkm);      
	return 1;
    }

}

int main(int argc, char **argv) {

    int raw_sock; //handler for raw socket
    unsigned char *packet;
    int tmp = 0;

    if (argc != 5) {
        printf("\nUsage: ./inject [INTERFACE] [IP_ADDRESS] [NODE_NAME] [HB_INTERVAL]\n\n");
        exit(0);
    }

    /*Create Raw socket*/
    raw_sock = create_raw_socket(MANIFOLD_UPDATE_TYPE);
    
    /*Bind raw socket to interface*/
    bind_raw_socket_to_interface(argv[1], raw_sock, MANIFOLD_UPDATE_TYPE);

#if DEBUG
    printf("[INTERFACE] = %s\n[IP] = %s\n[NODE_NAME] = %s\n[HB_INTERVAL] = %s", argv[1], argv[2], argv[3], argv[4]);
    printf("\nRaw socket = %d", raw_sock);
    printf("\nRaw socket = %d", raw_sock); fflush(stdout);
#endif    
   
    while (check_manifold_lkm_status()) {
    //while (1) {

        sleep(atoi(argv[4]));

        tmp = 0;

        packet = (unsigned char *) create_gnsm_packet((char *) & g_mac, (char *) DST_ETHER_ADDR, (int) MANIFOLD_UPDATE_TYPE, argv[2], argv[3], 0);

        if (packet == NULL) {
            printf("\nPacket is null");
            fflush(stdout);
            continue;
        }

#if DEBUG
        printf("\nPacket is :\n");
        while (tmp < (packet_len)) {
            printf("%x ", (int) (*(packet + tmp)));
            tmp++;
        }
        fflush(stdout);
#endif
        if (!SendRawPacket(raw_sock, packet, packet_len)) {
            perror("Error sending packet");
        }
        free(packet);
    }

    close(raw_sock);

    return (EXIT_SUCCESS);
}




/*

int first_line_exists_in_file(char *file_name) {

    FILE *fptr;
    char buf[80];

    fptr = popen(file_name, "r");
    if (fptr == NULL) {
        printf("\nCould not open %s. Returning 0...", file_name);
        return 0;
    }

    if (fgets(buf, 80, fptr) == NULL) {
#if DEBUG
        printf("\n\tCould not read from %s. Returning 0...", file_name);
        fflush(stdout);
#endif
        pclose(fptr);
        return 0;
    }

    pclose(fptr);
    return 1;

}

void get_wan_if_name(char * wan_if) {

    char line[80], command_str[100];

    memset(&line, 0, sizeof (line));
#if IS_EMB_DEV
    sprintf(command_str, "cat %s", WAN_IF_FILE_NAME);
#else
    memset(command_str, '\0', sizeof (command_str));
    sprintf(command_str, "cat %s%s", getenv("HOME"), WAN_IF_FILE_NAME);
#endif
    read_first_line_from_file(line, command_str);
    strncpy(wan_if, line, 10);
    wan_if[strlen(wan_if) - 1] = '\0';
#if DEBUG
    printf("\nWAN iface name -> %s", wan_if);
    fflush(stdout);
#endif


}

int set_gw_status(char *wan_if) {
    char line[80], command_str[100];

    memset(&line, 0, sizeof (line));
    memset(command_str, '\0', 100);
    sprintf(command_str, "ping -I %s -c 1 www.mymanet.org 2> /tmp/inject_log | grep \"ttl=\"", wan_if);
#if DEBUG
    printf("\n\tSet GW status: %d", first_line_exists_in_file(command_str));
    fflush(stdout);
#endif
    return first_line_exists_in_file(command_str);
}

void write_gw_status_into_file(int is_gw) {
    char filename[100], command_str[100];
    memset(filename, '\0', 100);
    memset(command_str, '\0', 100);
#if IS_EMB_DEV
    sprintf(filename, "%s", IS_GW_FILE_NAME);
#else
    sprintf(filename, "%s%s", getenv("HOME"), IS_GW_FILE_NAME);
#endif
    sprintf(command_str, "%d", is_gw);
#if DEBUG
    printf("\nWriting GW status into file -> \n\tFilename: %s\n\tData: %s", filename, command_str);
    fflush(stdout);
#endif
    write_first_line_into_file(command_str, filename);
}

uint8_t check_manifold_lkm_status() {

    FILE *fp_lkm = NULL;

    uint8_t ret_value = 255;

    if ((fp_lkm = popen("lsmod | grep manifold | wc -l", "r")) == NULL) {
        printf("\nCould open lsmod.. Terminating..."); fflush(stdout);
        exit(1);
    } else {
        char line[10];
        memset(&line, '\0', 10);
        fgets(line, 10, fp_lkm);
        ret_value = (uint8_t)atoi(line);
    }

    pclose(fp_lkm);
    return ret_value;
}

void write_first_line_into_file(char *buf, char *file_name) {

    char command_str[100];

    memset(command_str, '\0', 100);
    sprintf(command_str, "echo %s > %s", buf, file_name);
    system(command_str);
}


*/