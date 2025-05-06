# - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Name : Tammy Young
# SID : 1706229
# CCID : tqyoung
# - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

# Design Decisions
One design decision I made was trying to make functions modular and for specific tasks, similar to the source code. For example, I designed sr_handlepacket to contain entirely code for handling packets on the current router. When a packet needs to be forwarded, in other words, requires handling by another router, it will call sr_forward_ip_packet to forward the packet. Another example is the function I created in sr_arpcache.c called arp_header_init, which creates a new arp header with all the fields filled in in an initial state. It also takes an operation's opcode as an argument to be more modular and abstract.

Another design decision I made was to check for correctness. For example, ARP and IP packets were checked for correct formatting before processing. This includes checking the protocol type, header lengths, checksum, and TTL.

I also didn't implement validate_ip or validate_icmp because at first I didn't know we had to implement functions in the sr_utils.c file and it's too late to implement them right now (it's 4:54 pm on April 10).

Besides the points listed above, I didn't really have to make any design decisions because we were assigned to code a protocol.

# Testing
1. I tested pinging server1 and server2 with the client to see if packets could reach it or not.
2. I pinged sw0-eth1, sw0-eth2, and sw0-eth3 to test those connections as well.
3. I pinged the other server, client, sw0-eth1, sw0-eth2, and sw0-eth3 with one of the servers.
4. I tested pinging from both servers an IP address that doesn't exist (in the routing table).
5. I tested tracepath from client to the servers and servers to the client.
6. I tested wget from client to the servers.
7. I tested traceroute between client and the servers and servers to client
8. I tested pinging between the servers and client with TTL 1 and 2 (1 fails with TTL exceeded and 2 lets the packet go through)

# Sources
* https://tetcos.com/help/v13.2/Experiments-Manual/Working-of-ARP-and-IP-Forwarding-in-LAN-and-across-a-router.html
* https://www.pynetlabs.com/how-does-packet-flow-in-network/
* https://www.techtarget.com/searchnetworking/definition/Address-Resolution-Protocol-ARP
* https://www.fortinet.com/resources/cyberglossary/what-is-arp
