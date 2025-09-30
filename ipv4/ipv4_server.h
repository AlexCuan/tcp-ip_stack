#ifndef IPV4_SERVER_H
#define IPV4_SERVER_H

#include "ipv4.h"

int ipv4_recv(ipv4_layer_t * layer, uint8_t protocol, unsigned char buffer [], ipv4_addr_t sender, int buf_len, long int timeout);

#endif //IPV4_SERVER_H
