#include "sr_arpcache.h"

#include <netinet/in.h>
#include <pthread.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "sr_if.h"
#include "sr_protocol.h"
#include "sr_router.h"
#include "sr_utils.h"

sr_arp_hdr_t *arp_header_init(uint8_t *arpreq, unsigned short opcode) {
    /* this function initializes an arp header based on the operation required */
    sr_arp_hdr_t *arpreq_arp_hdr = (sr_arp_hdr_t *)(arpreq + sizeof(sr_ethernet_hdr_t));
    arpreq_arp_hdr->ar_hrd = (unsigned short)htons(arp_hrd_ethernet);
    arpreq_arp_hdr->ar_pro = (unsigned short)htons(ethertype_ip);
    arpreq_arp_hdr->ar_hln = (unsigned char)ETHER_ADDR_LEN;
    arpreq_arp_hdr->ar_pln = (unsigned char)sizeof(uint32_t);

    if (opcode == arp_op_request) {
        arpreq_arp_hdr->ar_op = (unsigned short)htons(arp_op_request);
    } else if (opcode == arp_op_reply) {
        arpreq_arp_hdr->ar_op = (unsigned short)htons(arp_op_reply);
    }
    return arpreq_arp_hdr;
}

void handle_arpreq(struct sr_instance *sr, struct sr_arpreq *request) {
    /* TODO: This function must send ARP requests if necessary. Refer to sr_arpcache.h for explanation and pseudocode */

    /*
    pseudocode
    if difftime(now, req->sent) > 1.0
     if req->times_sent >= 5:
         send icmp host unreachable to source addr of all pkts waiting
           on this request
         arpreq_destroy(req)
     else:
         send arp request
         req->sent = now
         req->times_sent++
    */

    time_t now;
    time(&now);

    if (difftime(now, request->sent) > 1.0) {
        if (request->times_sent >= 5) {
            fprintf(stdout, "arp sending host unreachable \n");

            /* send icmp host unreachable to source addr of all pkts waiting */
            struct sr_packet *packet = request->packets;
            while (packet) {
                send_icmp_msg(sr, packet->buf, packet->len, icmp_type_dest_unreachable, icmp_dest_unreachable_host);
                free(packet);
                packet = packet->next;
            }

            sr_arpreq_destroy(&sr->cache, request);
        } else {
            /* send arp request */
            fprintf(stdout, "sending arp req\n");

            struct sr_if *interface = sr_get_interface(sr, request->packets->iface);
            if (!interface) {
                printf("coudln't get interface\n");
                return;
            }

            int len = sizeof(sr_ethernet_hdr_t) + sizeof(sr_arp_hdr_t);
            uint8_t *arpreq = malloc(len);

            sr_ethernet_hdr_t *arpreq_eth_hdr = (sr_ethernet_hdr_t *)arpreq;
            memset(arpreq_eth_hdr->ether_dhost, 0xFF, ETHER_ADDR_LEN);
            memcpy(arpreq_eth_hdr->ether_shost, interface->addr, ETHER_ADDR_LEN);
            arpreq_eth_hdr->ether_type = htons(ethertype_arp);

            sr_arp_hdr_t *arp_req_header = arp_header_init(arpreq, arp_op_request);

            memset(arp_req_header->ar_tha, 0x00, ETHER_ADDR_LEN);
            arp_req_header->ar_tip = request->ip;
            
            memcpy(arp_req_header->ar_sha, interface->addr, ETHER_ADDR_LEN);
            arp_req_header->ar_sip = interface->ip;

            sr_send_packet(sr, arpreq, len, interface->name);
            free(arpreq);

            /* update request info */
            request->sent = now;
            request->times_sent++;

            fprintf(stdout, "done sending arp req\n");
        }
    }
}

void sr_arpcache_sweepreqs(struct sr_instance *sr) {
    /* TODO: This function is called once every second.
     * For each request sent out, keep checking whether to resend the request or
     * destroy the arp request. Refer to sr_arpcache.h for explanation and
     * pseudocode. */

    /*
    psuedocode
    void sr_arpcache_sweepreqs(struct sr_instance *sr) {
        for each request on sr->cache.requests:
            handle_arpreq(request)
    }
    */

    struct sr_arpreq *cache_request = sr->cache.requests;
    struct sr_arpreq *next;

    while (cache_request) {
        next = cache_request->next;
        handle_arpreq(sr, cache_request);
        cache_request = next;
    }
}

/* Do not modify the rest of this code. */

/* Check if IP->MAC mapping is in the cache. Note IP is in network byte order.
 * The returned structure must be freed by the caller if it is not NULL. */
struct sr_arpentry *sr_arpcache_lookup(struct sr_arpcache *cache, uint32_t ip) {
    pthread_mutex_lock(&(cache->lock));
    
    struct sr_arpentry *entry = NULL, *copy = NULL;
    
    int i;
    for (i = 0; i < SR_ARPCACHE_SZ; i++) {
        if ((cache->entries[i].valid) && (cache->entries[i].ip == ip)) {
            entry = &(cache->entries[i]);
        }
    }
    
    /* Must return a copy b/c another thread could jump in and modify
       table after we return. */
    if (entry) {
        copy = (struct sr_arpentry *) malloc(sizeof(struct sr_arpentry));
        memcpy(copy, entry, sizeof(struct sr_arpentry));
    }
        
    pthread_mutex_unlock(&(cache->lock));
    
    return copy;
}

/* Add an ARP request to the ARP request queue. If the request is already on
 * the queue, adds the packet to the linked list of packets for this sr_arpreq
 * that corresponds to this ARP request. You should free the passed *packet.
 *
 * A pointer to the ARP request is returned; it should not be freed. The caller
 * can remove the ARP request from the queue by calling sr_arpreq_destroy. */
struct sr_arpreq *sr_arpcache_queuereq(struct sr_arpcache *cache, uint32_t ip, uint8_t *packet, /* borrowed */
                                       unsigned int packet_len, char *iface) {
    pthread_mutex_lock(&(cache->lock));

    struct sr_arpreq *req;
    for (req = cache->requests; req != NULL; req = req->next) {
        if (req->ip == ip) {
            break;
        }
    }

    /* If the IP wasn't found, add it */
    if (!req) {
        req = (struct sr_arpreq *)calloc(1, sizeof(struct sr_arpreq));
        req->ip = ip;
        req->next = cache->requests;
        cache->requests = req;
    }

    /* Add the packet to the list of packets for this request */
    if (packet && packet_len && iface) {
        struct sr_packet *new_pkt = (struct sr_packet *)malloc(sizeof(struct sr_packet));

        new_pkt->buf = (uint8_t *)malloc(packet_len);
        memcpy(new_pkt->buf, packet, packet_len);
        new_pkt->len = packet_len;
        new_pkt->iface = (char *)malloc(sr_IFACE_NAMELEN);
        strncpy(new_pkt->iface, iface, sr_IFACE_NAMELEN);
        new_pkt->next = req->packets;
        req->packets = new_pkt;
    }

    pthread_mutex_unlock(&(cache->lock));

    return req;
}

/* Do these two things:
 * 1) Look up this IP in the request queue. If it is found, return a pointer
 * to the sr_arpreq with this IP. Otherwise, return NULL.
 * 2) Insert this IP->MAC mapping into the cache and mark it valid. */
struct sr_arpreq *sr_arpcache_insert(struct sr_arpcache *cache, unsigned char *mac, uint32_t ip) {
    pthread_mutex_lock(&(cache->lock));
    
    struct sr_arpreq *req, *prev = NULL, *next = NULL; 
    for (req = cache->requests; req != NULL; req = req->next) {
        if (req->ip == ip) {            
            if (prev) {
                next = req->next;
                prev->next = next;
            } 
            else {
                next = req->next;
                cache->requests = next;
            }
            
            break;
        }
        prev = req;
    }
    
    int i;
    for (i = 0; i < SR_ARPCACHE_SZ; i++) {
        if (!(cache->entries[i].valid))
            break;
    }
    
    if (i != SR_ARPCACHE_SZ) {
        memcpy(cache->entries[i].mac, mac, 6);
        cache->entries[i].ip = ip;
        cache->entries[i].added = time(NULL);
        cache->entries[i].valid = 1;
    }
    
    pthread_mutex_unlock(&(cache->lock));
    
    return req;
}

/* Free all memory associated with the passed arp request entry. If this arp
 * request entry is on the arp request queue, remove it from the queue. */
void sr_arpreq_destroy(struct sr_arpcache *cache, struct sr_arpreq *entry) {
    pthread_mutex_lock(&(cache->lock));

    if (entry) {
        struct sr_arpreq *req, *prev = NULL, *next = NULL;
        for (req = cache->requests; req != NULL; req = req->next) {
            if (req == entry) {
                if (prev) {
                    next = req->next;
                    prev->next = next;
                } else {
                    next = req->next;
                    cache->requests = next;
                }

                break;
            }
            prev = req;
        }

        struct sr_packet *pkt, *nxt;

        for (pkt = entry->packets; pkt; pkt = nxt) {
            nxt = pkt->next;
            if (pkt->buf) free(pkt->buf);
            if (pkt->iface) free(pkt->iface);
            free(pkt);
        }

        free(entry);
    }

    pthread_mutex_unlock(&(cache->lock));
}

/* Print out the ARP table. */
void sr_arpcache_dump(struct sr_arpcache *cache) {
    fprintf(stderr, "\nMAC            IP         ADDED                      VALID\n");
    fprintf(stderr, "-----------------------------------------------------------\n");

    int i;
    for (i = 0; i < SR_ARPCACHE_SZ; i++) {
        struct sr_arpentry *cur = &(cache->entries[i]);
        unsigned char *mac = cur->mac;
        fprintf(stderr, "%.1x%.1x%.1x%.1x%.1x%.1x   %.8x   %.24s   %d\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5], ntohl(cur->ip), ctime(&(cur->added)), cur->valid);
    }

    fprintf(stderr, "\n");
}

/* Initialize table and table lock. Return 0 on success. */
int sr_arpcache_init(struct sr_arpcache *cache) {
    /* Seed RNG to kick out a random entry if all entries full. */
    srand(time(NULL));

    /* Invalidate all entries */
    memset(cache->entries, 0, sizeof(cache->entries));
    cache->requests = NULL;

    /* Acquire mutex lock */
    pthread_mutexattr_init(&(cache->attr));
    pthread_mutexattr_settype(&(cache->attr), PTHREAD_MUTEX_RECURSIVE);
    int success = pthread_mutex_init(&(cache->lock), &(cache->attr));

    return success;
}

/* Destroy table and table lock. Return 0 on success. */
int sr_arpcache_destroy(struct sr_arpcache *cache) { return pthread_mutex_destroy(&(cache->lock)) && pthread_mutexattr_destroy(&(cache->attr)); }

/* Sweep through the cache invalidating entries that were added more than
 * SR_ARPCACHE_TO seconds ago. This is a thread start-up routine. */
void *sr_arpcache_timeout(void *sr_ptr) {
    struct sr_instance *sr = sr_ptr;
    struct sr_arpcache *cache = &(sr->cache);

    while (1) {
        sleep(1.0);

        pthread_mutex_lock(&(cache->lock));

        time_t curtime = time(NULL);

        int i;
        for (i = 0; i < SR_ARPCACHE_SZ; i++) {
            if ((cache->entries[i].valid) && (difftime(curtime, cache->entries[i].added) > SR_ARPCACHE_TO)) {
                cache->entries[i].valid = 0;
            }
        }

        sr_arpcache_sweepreqs(sr);

        pthread_mutex_unlock(&(cache->lock));
    }

    return NULL;
}
