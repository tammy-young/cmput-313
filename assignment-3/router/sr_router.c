#include "sr_router.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sr_arpcache.h"
#include "sr_if.h"
#include "sr_protocol.h"
#include "sr_rt.h"
#include "sr_utils.h"

/*---------------------------------------------------------------------
 * Method: sr_init(void)
 * Scope:  Global
 *
 * Initialize the routing subsystem
 *
 *---------------------------------------------------------------------*/

void sr_init(struct sr_instance* sr) {
    /* REQUIRES */
    assert(sr);

    /* Initialize cache and cache cleanup thread */
    sr_arpcache_init(&(sr->cache));

    pthread_attr_init(&(sr->attr));
    pthread_attr_setdetachstate(&(sr->attr), PTHREAD_CREATE_JOINABLE);
    pthread_attr_setscope(&(sr->attr), PTHREAD_SCOPE_SYSTEM);
    pthread_attr_setscope(&(sr->attr), PTHREAD_SCOPE_SYSTEM);
    pthread_t thread;

    pthread_create(&thread, &(sr->attr), sr_arpcache_timeout, sr);

    /* TODO: Add initialization code here if needed */

} /* -- sr_init -- */

/* TODO: Complete the implementation of this function that sends an ICMP message
 */
void send_icmp_msg(struct sr_instance* sr, uint8_t* packet, unsigned int len, uint8_t type, uint8_t code) {
    /* Get ethernet header from packet */
    sr_ethernet_hdr_t* eth_hdr = (sr_ethernet_hdr_t*)packet;
    /* Get IP header from packet */
    sr_ip_hdr_t* ip_hdr = (sr_ip_hdr_t*)(packet + sizeof(sr_ethernet_hdr_t));

    printf("ip header\n");
    print_hdr_ip(packet);
    printf("my address\n");
    print_addr_ip_int(ip_hdr->ip_src);
    printf("all interfaces\n");
    sr_print_if_list(sr);

    /* Call longest_prefix_matching() to get source IP */
    /* You should implement longest_prefix_matching() in sr_rt.c */
    struct sr_rt* rt_entry = longest_prefix_matching(sr, ip_hdr->ip_src);

    if (!rt_entry) {
        printf("Error: send_icmp_msg: routing table entry not found.\n");
        return;
    }

    /* Get outgoing interface */
    struct sr_if* interface = sr_get_interface(sr, rt_entry->interface);

    switch (type) {
        case icmp_type_echo_reply: {
            /* Initialize ethernet header source & destination MAC:
             * 00-00-00-00-00-00 */
            /* You should correctly set the source & destination MAC later when
             * checking the ARP cache */
            fprintf(stdout, "echoing reply\n");
            memset(eth_hdr->ether_shost, 0, ETHER_ADDR_LEN);
            memset(eth_hdr->ether_dhost, 0, ETHER_ADDR_LEN);

            /* Set source & destination IP addresses */
            uint32_t temp = ip_hdr->ip_dst;
            ip_hdr->ip_dst = ip_hdr->ip_src;
            ip_hdr->ip_src = temp;

            /* Construct ICMP header */
            sr_icmp_hdr_t* icmp_hdr = (sr_icmp_hdr_t*)(packet + sizeof(sr_ethernet_hdr_t) + (ip_hdr->ip_hl * 4));
            icmp_hdr->icmp_type = type;
            icmp_hdr->icmp_code = code;

            /* Compute ICMP checksum */
            icmp_hdr->icmp_sum = 0;
            icmp_hdr->icmp_sum = cksum(icmp_hdr, ntohs(ip_hdr->ip_len) - (ip_hdr->ip_hl * 4));

            /* TODO: Check ARP cache and send packet or ARP request */
            struct sr_arpentry* arp_entry = sr_arpcache_lookup(&(sr->cache), rt_entry->gw.s_addr);
            if (arp_entry) {
                fprintf(stdout, "\tin arp cache\n");
                memcpy(eth_hdr->ether_dhost, arp_entry->mac, ETHER_ADDR_LEN);
                memcpy(eth_hdr->ether_shost, interface->addr, ETHER_ADDR_LEN);
                sr_send_packet(sr, packet, len, interface->name);
                free(arp_entry);
            } else {
                fprintf(stdout, "\tqueued in arp cache\n");
                struct sr_arpreq* arp_req = sr_arpcache_queuereq(&(sr->cache), rt_entry->gw.s_addr, packet, len, interface->name);
                handle_arpreq(sr, arp_req);
            }

            break;
        }
        case icmp_type_time_exceeded:
        case icmp_type_dest_unreachable: {
            fprintf(stdout, "destination unreachable\n");
            /* Length of the new ICMP packet */
            unsigned int new_len = sizeof(sr_ethernet_hdr_t) + (ip_hdr->ip_hl * 4) + sizeof(sr_icmp_t3_hdr_t);
            /* Construct new ICMP packet */
            uint8_t* new_packet = malloc(new_len);
            assert(new_packet);

            /* Construct ethernet hdr */
            sr_ethernet_hdr_t* new_eth_hdr = (sr_ethernet_hdr_t*)new_packet;
            /* Construct IP hdr */
            sr_ip_hdr_t* new_ip_hdr = (sr_ip_hdr_t*)(new_packet + sizeof(sr_ethernet_hdr_t));
            /* Construct type 3 ICMP hdr */
            sr_icmp_t3_hdr_t* icmp_hdr = (sr_icmp_t3_hdr_t*)(new_packet + sizeof(sr_ethernet_hdr_t) + (ip_hdr->ip_hl * 4));

            /* Initialize ethernet header source & destination MAC:
             * 00-00-00-00-00-00 */
            /* You should correctly set the source & destination MAC later when
             * checking the ARP cache */
            memset(new_eth_hdr->ether_shost, 0, ETHER_ADDR_LEN);
            memset(new_eth_hdr->ether_dhost, 0, ETHER_ADDR_LEN);

            /* Set protocol type to IP */
            new_eth_hdr->ether_type = htons(ethertype_ip);

            /* Set new IP hdr */
            new_ip_hdr->ip_v = 4;
            new_ip_hdr->ip_hl = ip_hdr->ip_hl;
            new_ip_hdr->ip_tos = 0;
            new_ip_hdr->ip_len = htons((ip_hdr->ip_hl * 4) + sizeof(sr_icmp_t3_hdr_t));
            new_ip_hdr->ip_id = htons(0);
            new_ip_hdr->ip_off = htons(IP_DF);
            new_ip_hdr->ip_ttl = 255;
            new_ip_hdr->ip_p = ip_protocol_icmp;
            /* If code == 3, set source IP to received packet's destination IP
             */
            /* Otherwise, set source IP to outgoing interface's IP */
            if (code == icmp_dest_unreachable_port) {
                new_ip_hdr->ip_src = ip_hdr->ip_dst;
            } else {
                new_ip_hdr->ip_src = interface->ip;
            }

            /* Set destination IP to the source IP of the received packet */
            new_ip_hdr->ip_dst = ip_hdr->ip_src;

            /* Recalculate checksum */
            new_ip_hdr->ip_sum = 0;
            new_ip_hdr->ip_sum = cksum(new_ip_hdr, ip_hdr->ip_hl * 4);

            /* Set type 3 ICMP hdr */
            icmp_hdr->icmp_type = type;
            icmp_hdr->icmp_code = code;
            icmp_hdr->unused = 0;
            icmp_hdr->next_mtu = 0;
            memcpy(icmp_hdr->data, ip_hdr, ICMP_DATA_SIZE);
            icmp_hdr->icmp_sum = 0;
            icmp_hdr->icmp_sum = cksum(icmp_hdr, sizeof(sr_icmp_t3_hdr_t));

            /* TODO: Check ARP cache and send packet or ARP request */
            struct sr_arpentry* arp_entry = sr_arpcache_lookup(&(sr->cache), rt_entry->gw.s_addr);
            if (arp_entry) {
                fprintf(stdout, "\tin arp cache\n");
                memcpy(new_eth_hdr->ether_dhost, arp_entry->mac, ETHER_ADDR_LEN);
                memcpy(new_eth_hdr->ether_shost, interface->addr, ETHER_ADDR_LEN);
                sr_send_packet(sr, new_packet, new_len, interface->name);
                free(arp_entry);
                free(new_packet);
            } else {
                fprintf(stdout, "\tqueued in arp cache\n");
                struct sr_arpreq* arp_req = sr_arpcache_queuereq(&(sr->cache), rt_entry->gw.s_addr, new_packet, new_len, interface->name);
                handle_arpreq(sr, arp_req);
            }

            break;
        }
    }
}

/*---------------------------------------------------------------------
 * Method: sr_forward_ip_packet(struct sr_instance* sr, uint8_t* packet,
 *                        unsigned int len, const char* iface)
 * Scope:  Global
 *
 * This method is called each time the router receives a packet that requires
 * forwarding (packet is not for the current router).
 *
 *---------------------------------------------------------------------*/

void sr_forward_ip_packet(struct sr_instance* sr, uint8_t* packet, unsigned int len, const char* iface) {
    assert(sr);
    assert(packet);
    assert(iface);

    fprintf(stdout, "\nforwarding packettttt\n");

    struct sr_ip_hdr* ip_header = (struct sr_ip_hdr*)(packet + sizeof(struct sr_ethernet_hdr));

    printf("destination\n");
    print_addr_ip_int(ip_header->ip_dst);

    /* check valid checksum */
    uint16_t received_checksum = ip_header->ip_sum;
    ip_header->ip_sum = 0;
    uint16_t computed_checksum = cksum(ip_header, sizeof(sr_ip_hdr_t));
    if (received_checksum != computed_checksum) {
        fprintf(stderr, "bad checksum got %x but computed %x\n", received_checksum, computed_checksum);
        return;
    }

    if (ip_header->ip_ttl <= 1) {
        send_icmp_msg(sr, packet, len, icmp_type_time_exceeded, 0);
        return;
    }

    ip_header->ip_ttl--;
    ip_header->ip_sum = 0;
    ip_header->ip_sum = cksum(ip_header, sizeof(sr_ip_hdr_t));

    /* check routing table */
    struct sr_rt* routing_table_entry = longest_prefix_matching(sr, ip_header->ip_dst);
    if (!routing_table_entry) {
        fprintf(stderr, "ip not in routing table\n");
        send_icmp_msg(sr, packet, len, icmp_type_dest_unreachable, icmp_dest_unreachable_net);
        return;
    }

    struct sr_if* rt_out_interface = sr_get_interface(sr, routing_table_entry->interface);

    uint32_t next_hop_ip = routing_table_entry->gw.s_addr;
    
    struct sr_arpentry* arp_cache_entry = sr_arpcache_lookup(&sr->cache, next_hop_ip);
    if (arp_cache_entry) {
        printf("sending packet now\n");
        sr_ethernet_hdr_t* eth_header = (sr_ethernet_hdr_t*)packet;
        memcpy(eth_header->ether_dhost, arp_cache_entry->mac, ETHER_ADDR_LEN);
        memcpy(eth_header->ether_shost, rt_out_interface->addr, ETHER_ADDR_LEN);
        sr_send_packet(sr, packet, len, rt_out_interface->name);
        free(arp_cache_entry);
    } else {
        printf("sending packet later, queued rn\n");
        struct sr_arpreq* arp_req = sr_arpcache_queuereq(&sr->cache, next_hop_ip, packet, len, rt_out_interface->name);
        handle_arpreq(sr, arp_req);
    }

    fprintf(stdout, "\tdone forwarding the packet\n");
}

/*---------------------------------------------------------------------
 * Method: sr_handlepacket(uint8_t* p, char* iface)
 * Scope:  Global
 *
 * This method is called each time the router receives a packet on the
 * interface.  The packet buffer, the packet length and the receiving
 * interface are passed in as parameters. The packet is complete with
 * ethernet headers.
 *
 * Note: Both the packet buffer and the character's memory are handled
 * by sr_vns_comm.c that means do NOT delete either.  Make a copy of the
 * packet instead if you intend to keep it around beyond the scope of
 * the method call.
 *
 *---------------------------------------------------------------------*/

void sr_handlepacket(struct sr_instance* sr, uint8_t* buf /* lent */, unsigned int len, char* iface /* lent */) {
    /* REQUIRES */
    assert(sr);
    assert(buf);
    assert(iface);

    printf("\n\n*** -> Received packet of length %d \n", len);

    /* check min length for an ethernet frame */
    if (len < sizeof(sr_ethernet_hdr_t)) {
        fprintf(stderr, "ethernet packet too short\n");
        return;
    }

    sr_ethernet_hdr_t* ethernet_header = (sr_ethernet_hdr_t*)buf;
    uint16_t ethertype = ntohs(ethernet_header->ether_type);

    if (ethertype == ethertype_arp) {
        /* arp = 2054 */

        /* get header */
        sr_arp_hdr_t* arp_header = (sr_arp_hdr_t*)(buf + sizeof(sr_ethernet_hdr_t));
        struct sr_if* out_interface = sr_get_interface_by_ip(sr, arp_header->ar_tip);
        if (!out_interface) {
            printf("current router not destination\n");
            return;
        }

        unsigned short opcode = ntohs(arp_header->ar_op);

        if (opcode == arp_op_request) {
            fprintf(stdout, "new arp request\n");
            struct sr_if* interface = sr_get_interface(sr, iface);

            uint8_t* arp_reply = malloc(len);
            memcpy(arp_reply, buf, len);

            /* ethernet header */
            sr_ethernet_hdr_t* reply_eth_header = (sr_ethernet_hdr_t*)arp_reply;
            memcpy(reply_eth_header->ether_dhost, ethernet_header->ether_shost, ETHER_ADDR_LEN);
            memcpy(reply_eth_header->ether_shost, interface->addr, ETHER_ADDR_LEN);
            reply_eth_header->ether_type = htons(ethertype_arp);

            /* arp header */
            sr_arp_hdr_t* reply_arp_header = (sr_arp_hdr_t*)(arp_reply + sizeof(sr_ethernet_hdr_t));
            reply_arp_header->ar_hrd = htons(arp_hrd_ethernet);
            reply_arp_header->ar_pro = htons(ethertype_ip);
            reply_arp_header->ar_hln = ETHER_ADDR_LEN;
            reply_arp_header->ar_pln = 4;
            reply_arp_header->ar_op = htons(arp_op_reply);
            
            memcpy(reply_arp_header->ar_sha, interface->addr, ETHER_ADDR_LEN);
            reply_arp_header->ar_sip = interface->ip;
            memcpy(reply_arp_header->ar_tha, arp_header->ar_sha, ETHER_ADDR_LEN);
            reply_arp_header->ar_tip = arp_header->ar_sip;

            /* send arp reply */
            fprintf(stdout, "sending arp reply directly\n");
            sr_send_packet(sr, arp_reply, len, interface->name);
            free(arp_reply);
        } else if (opcode == arp_op_reply) {
            fprintf(stdout, "\narp reply\n");
            struct sr_arpreq* arp_entry = sr_arpcache_insert(&sr->cache, arp_header->ar_sha, arp_header->ar_sip);

            if (arp_entry) {
                struct sr_if* interface;
                sr_ethernet_hdr_t* ethernet_header;
                struct sr_packet* arp_packet = arp_entry->packets;

                while (arp_packet) {
                    /* send packet reply to all packets in arp cache */
                    interface = sr_get_interface(sr, arp_packet->iface);
                    if (interface) {
                        ethernet_header = (sr_ethernet_hdr_t*)(arp_packet->buf);
                        memcpy(ethernet_header->ether_dhost, arp_header->ar_sha, ETHER_ADDR_LEN);
                        memcpy(ethernet_header->ether_shost, interface->addr, ETHER_ADDR_LEN);
                        sr_send_packet(sr, arp_packet->buf, arp_packet->len, arp_packet->iface);
                    }
                    arp_packet = arp_packet->next;
                }

                sr_arpreq_destroy(&sr->cache, arp_entry);
            }
            fprintf(stdout, "\tdone reply\n");
        }

    } else if (ethertype == ethertype_ip) {
        /* ip = 2048 */
        fprintf(stderr, "got ip packet\n");

        if (len < sizeof(sr_ethernet_hdr_t) + sizeof(sr_ip_hdr_t)) {
            fprintf(stderr, "ip packet too short\n");
            return;
        }

        sr_ip_hdr_t* ip_header = (sr_ip_hdr_t*)(buf + sizeof(sr_ethernet_hdr_t));

        /* check ip checksum */
        uint16_t original_sum = ip_header->ip_sum;
        ip_header->ip_sum = 0;
        if (cksum(ip_header, ip_header->ip_hl * 4) != original_sum) {
            fprintf(stderr, "invalid checksum\n");
            return;
        }
        ip_header->ip_sum = original_sum;

        if (ip_header->ip_ttl <= 1) {
            send_icmp_msg(sr, buf, len, icmp_type_time_exceeded, 0);
            return;
        }

        /* check if packet is for this router */
        struct sr_if* interface = sr_get_interface_by_ip(sr, ip_header->ip_dst);

        if (interface) {
            fprintf(stdout, "packet is for us\n");
            uint8_t protocol = ip_header->ip_p;
            if (protocol == ip_protocol_icmp) {
                fprintf(stdout, "packet is icmp\n");
                sr_icmp_hdr_t* icmp_header = (sr_icmp_hdr_t*)(buf + sizeof(sr_ethernet_hdr_t) + (ip_header->ip_hl * 4));
                if (icmp_header->icmp_type == icmp_type_echo_request) {
                    /* echo icmp message */
                    fprintf(stdout, "sending icmp message\n");
                    send_icmp_msg(sr, buf, len, icmp_type_echo_reply, 0);
                    return;
                }
            } else if (protocol == ip_protocol_tcp || protocol == ip_protocol_udp) {
                /* send icmp port unreachable to sending host */
                fprintf(stdout, "packet is tcp/udp\n");
                send_icmp_msg(sr, buf, len, icmp_type_dest_unreachable, icmp_dest_unreachable_port);
            }
        } else {
            sr_forward_ip_packet(sr, buf, len, iface);
        }
    }
}
