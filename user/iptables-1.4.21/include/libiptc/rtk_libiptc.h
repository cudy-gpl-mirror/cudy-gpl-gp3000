#ifndef _RTKLIBIPTC_H
#define _RTKLIBIPTC_H

#include <libiptc/xtcshared.h>
#include <linux/netfilter_ipv4/ip_tables.h>
#include <linux/netfilter_ipv6/ip6_tables.h>

/* Does this chain exist? */
int iptc_is_chain(const char *chain, struct xtc_handle *const handle);

/* Take a snapshot of the rules.  Returns NULL on error. */
struct xtc_handle *iptc_init(const char *tablename);

/* Cleanup after iptc_init(). */
void iptc_free(struct xtc_handle *h);

/* Iterator functions to run through the chains.  Returns NULL at end. */
const char *iptc_first_chain(struct xtc_handle *handle);
const char *iptc_next_chain(struct xtc_handle *handle);

/* Get first rule in the given chain: NULL for empty chain. */
const struct ipt_entry *iptc_first_rule(const char *chain,
					struct xtc_handle *handle);

/* Returns NULL when rules run out. */
const struct ipt_entry *iptc_next_rule(const struct ipt_entry *prev,
				       struct xtc_handle *handle);

//=========================IPV6============================

/* Does this chain exist? */
int ip6tc_is_chain(const char *chain, struct xtc_handle *const handle);

/* Take a snapshot of the rules. Returns NULL on error. */
struct xtc_handle *ip6tc_init(const char *tablename);

/* Cleanup after ip6tc_init(). */
void ip6tc_free(struct xtc_handle *h);

/* Iterator functions to run through the chains.  Returns NULL at end. */
const char *ip6tc_first_chain(struct xtc_handle *handle);
const char *ip6tc_next_chain(struct xtc_handle *handle);

/* Get first rule in the given chain: NULL for empty chain. */
const struct ip6t_entry *ip6tc_first_rule(const char *chain,
					  struct xtc_handle *handle);

/* Returns NULL when rules run out. */
const struct ip6t_entry *ip6tc_next_rule(const struct ip6t_entry *prev,
					 struct xtc_handle *handle);

#endif /* _RTKLIBIPTC_H */