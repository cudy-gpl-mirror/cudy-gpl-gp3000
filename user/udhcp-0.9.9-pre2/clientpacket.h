#ifndef _CLIENTPACKET_H
#define _CLIENTPACKET_H

unsigned long random_xid(void);
#ifndef _PRMT_X_TELEFONICA_ES_DHCPOPTION_
int send_discover(unsigned long xid, unsigned long requested);
#else
int send_discover(unsigned long xid);
#endif
int send_selecting(unsigned long xid, unsigned long server, unsigned long requested);
int send_renew(unsigned long xid, unsigned long server, unsigned long ciaddr);
int send_renew(unsigned long xid, unsigned long server, unsigned long ciaddr);
int send_release(unsigned long server, unsigned long ciaddr);
int send_decline(void);
#if defined(SEND_RELEASE_BY_RAWSOCK)
int send_raw_release(unsigned long server, unsigned long ciaddr, unsigned char *dest_arp);
int get_raw_packet(struct dhcpMessage *payload, int fd, unsigned char *src_arp);
#else
int get_raw_packet(struct dhcpMessage *payload, int fd);
#endif

/* Dick Tam, 2003-05-16, Microsoft AUTO IP procedure */
void rt_prepare_microsoft_auto_ip_dhcpack(struct dhcpMessage *packet, u_int32_t addr);

#ifdef _PRMT_X_TELEFONICA_ES_DHCPOPTION_
/* HDCP Options*/
void initialDhcpcOptionTbl(void);
#ifdef CONFIG_00R0
int queryDhcpcOptionTbl(int tag, int *len);
#endif
int updateDhcpcOptionTbl(int tag, unsigned char *value, int len);
void updateReqDhcpcOptionValue(struct dhcpMessage *packet);
#endif
#endif
