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
Developers: Ashwin Paranjpe, Santosh Vempala, Hrushikesh Mehendale
Georgia Institute of Technology, Atlanta, USA
 */



#ifndef _INJECT_H
#define	_INJECT_H

#ifndef IS_EMB_DEV
#define IS_EMB_DEV 0
#else
#endif

#define DEBUG 0
#define PRINT_PACKET 1  //both DEBUG and PRINT_PACKET should not be 1 at the same time
#define GW_SUPPORT 0
#define DST_ETHER_ADDR	"FF:FF:FF:FF:FF:FF"
#define BUFFER_LEN 1024
#define MANIFOLD_UPDATE_TYPE 0x3434
#define HB_INTERVAL 5
#define MAX_NUM_OF_HOSTS 40
#define MAX_NUM_OF_STAT_ENTRIES 40
#define MAX_NODE_NAME_SIZE 10
#define NUM_OF_REAL_HOSTS 6
#define ACTIVE_TIMEOUT 20
#define SOCKET_SELECT_TIMEOUT 0
#define MAX_DST 255
#define TX_LOG_FILE_NAME "/.LifeNetData/config/tx.log"
#define GW_CHECK_URL "www.google.com"
#define MAX_NUM_DNS_TX 3
#define TX_FILE_NAME "/proc/txstats"
#define RX_FILE_NAME "/proc/rxstats"

#if IS_EMB_DEV

	#define IS_GW_FILE_NAME "/sdcard/is_gw.conf"
        #define WAN_IF_FILE_NAME "/sdcard/eth_iface.conf"
        #define FIREWALL_FWD_ALL_FILE_NAME "/sdcard/fwd_all.sh"
        #define FIREWALL_FLUSH_ALL_FILE_NAME "/sdcard/flush_all.sh"
        #define HOSTS_FILE_PATH "/sdcard/hosts"
        #define STATS_FILE_PATH "/sdcard/statlist"
        #define STATS_REFINED_FILE_PATH "/sdcard/statlist_refined"
        #define GW_LOG_FILE_NAME "/sdcard/gw_log.conf"

#else
	#define IS_GW_FILE_NAME "/sdcard/is_gw.conf"
        #define WAN_IF_FILE_NAME "/sdcard/eth_iface.conf"
        #define FIREWALL_FWD_ALL_FILE_NAME "/sdcard/fwd_all.sh"
        #define FIREWALL_FLUSH_ALL_FILE_NAME "/sdcard/flush_all.sh"
        #define HOSTS_FILE_PATH "/sdcard/hosts"
        #define STATS_FILE_PATH "/sdcard/statlist"
        #define STATS_REFINED_FILE_PATH "/sdcard/statlist_refined"
        #define GW_LOG_FILE_NAME "/sdcard/gw_log.conf"
#endif
#define ALLOWED_GW_VD_DEVIATION 20 /*in percent*/


/*
Socket Functions
*/

int create_raw_socket(int protocol_to_send);
int bind_raw_socket_to_interface(char *device, int raw_sock, int protocol);
int SendRawPacket(int rawsock, unsigned char *pkt, int pkt_len);
/*
Network functions
*/
unsigned char* create_gnsm_packet(uint8_t *src_mac, uint8_t *dst_mac, int protocol, char ip_address[15], char node_name[10], uint8_t is_gw);

/*
Supporting functions
*/

int pack(unsigned char *ptr, uint8_t *src_mac, int num_bytes);
void create_ethernet_header(struct ethhdr *ptr, uint8_t *src_mac, uint8_t *dst_mac, int protocol);
void get_wan_if_name(char * wan_if);
int set_gw_status(char *wan_if);
void write_gw_status_into_file(int is_gw);
int handle_dns_and_iptables(unsigned char *buf, int is_gw);
int pack_curr_node_info(unsigned char *buf, char ip_address[15], uint8_t *src_mac, char node_name[10]);
int string_to_byte_order_etheraddr(const u_char *asc, char *addr);
uint8_t check_manifold_lkm_status();

/*
File handling functions
*/
void write_first_line_into_file(char *buf, char *file_name);
int read_and_pack_rxstats(char *buf);
int read_and_pack_txstats(char *buf);
int read_and_pack_dns(char *buf);
int read_from_first_three_lines_of_file_having_pattern(char *buf1, char *buf2, char *buf3, char *file_name, char *pattern, int token_num);
void read_first_line_from_file(char *buf, char *file_name);
int first_line_exists_in_file(char *file_name);
#endif	/* _INJECT_H */
