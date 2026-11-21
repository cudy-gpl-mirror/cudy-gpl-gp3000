#ifndef _SERVERPACKET_H
#define _SERVERPACKET_H

int sendOffer(struct dhcpMessage *oldpacket, struct server_config_t *server_config_p);
int sendNAK(struct dhcpMessage *oldpacket, struct server_config_t *server_config_p);
int sendACK(struct dhcpMessage *oldpacket, u_int32_t yiaddr, struct server_config_t *server_config_p);
int send_inform(struct dhcpMessage *oldpacket, struct server_config_t *server_config_p);

unsigned char getDeviceType(void);
void addDhcpdOption(struct dhcpMessage *packet, struct server_config_t *server_config_p);
void find_match_serving_pool(struct dhcpMessage *dhcppacket, struct server_config_t **server_config_p);
#ifdef IP_BASED_CLIENT_TYPE
u_int32_t find_address(int check_expired, struct client_category_t *deviceCategory, u_int8_t *chaddr, struct server_config_t *server_config_p );
int parse_CTC_Vendor_Class(unsigned char* option60, struct dhcp_ctc_client_info *pstClientInfo);
int check_type(u_int32_t addr_n, struct client_category_t *deviceCategory, struct server_config_t *server_config_p);
#else
u_int32_t find_address(int check_expired, u_int8_t *chaddr, struct server_config_t *server_config_p);
#endif

#ifdef _CWMP_TR111_
void clear_all_deviceId(void);
int del_deviceId(u_int32_t iaddr);
int dump_deviceId(void);
#endif
unsigned long find_IP_by_Mac(unsigned char * packet_chaddr);
int find_Mac_by_IP(u_int32_t ipaddr);
#ifdef SUPPORT_DHCP_RESERVED_IPADDR
int isReservedIPAddress(u_int32_t ipaddr, struct server_config_t *server_config_p);
#endif
#endif
