#ifndef sr_INTERFACE_H
#define sr_INTERFACE_H

#ifdef _LINUX_
#include <stdint.h>
#endif /* _LINUX_ */

#ifdef _SOLARIS_
#include </usr/include/sys/int_types.h>
#endif /* SOLARIS */

#ifdef _DARWIN_
#include <inttypes.h>
#endif

#include "sr_protocol.h"

struct sr_instance;

/* ----------------------------------------------------------------------------
 * struct sr_if
 *
 * Node in the interface list for each router
 *
 * -------------------------------------------------------------------------- */

struct sr_if
{
  char name[sr_IFACE_NAMELEN];
  unsigned char addr[ETHER_ADDR_LEN];
  uint32_t ip;
  uint32_t speed;
  struct sr_if* next;
};

struct sr_if* sr_get_interface(struct sr_instance* sr, const char* name);
struct sr_if* sr_get_interface_by_ip(struct sr_instance* sr, uint32_t ip);
struct sr_if* sr_get_interface_by_mac(struct sr_instance* sr, unsigned char* addr);
void sr_add_interface(struct sr_instance*, const char*);
void sr_set_ether_addr(struct sr_instance*, const unsigned char*);
void sr_set_ether_ip(struct sr_instance*, uint32_t ip_nbo);
void sr_print_if_list(struct sr_instance*);
void sr_print_if(struct sr_if*);


#endif /* --  sr_INTERFACE_H -- */
