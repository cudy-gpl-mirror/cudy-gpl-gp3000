#ifndef _SCRIPT_H
#define _SCRIPT_H

void run_script(struct dhcpMessage *packet, const char *name);

#ifdef CONFIG_USER_DHCP_OPT_33_249
void dealStaticRTOption(unsigned char *data, int code);
#endif

#ifdef _CONFIG_DHCPC_OPTION43_ACSURL_
int dealOption43(unsigned char *data);
#endif
int dealOption15(unsigned char *data);
int dealOption20(unsigned char *data);
int dealOption66(unsigned char *data);
int dealOption67(unsigned char *data);
int dealOption100(unsigned char *data);
#ifdef CONFIG_IPV6_SIT_6RD
int dealOption212(unsigned char *data);
#endif
#ifdef TIME_ZONE
int dealOption42(unsigned char *data);
#endif
#ifdef CONFIG_USER_RTK_VOIP
int dealOption120(unsigned char *data);
#endif
#ifdef RECEIVE_DHCP_OPTION_125
int dealOption125(unsigned char *data);
#endif
#ifdef _PRMT_X_CT_COM_DHCP_
int appendOption60(unsigned char *value);
#endif
#if defined(CONFIG_USER_DNSMASQ_DNSMASQ) || defined(CONFIG_USER_DNSMASQ_DNSMASQ245)
int dealOption6(unsigned char *data);
#endif

#endif
