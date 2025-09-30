#include <stdio.h>
#include <stdlib.h>
#include "ipv4.h"
#include "ipv4_config.h"
#include "../eth/eth.h"

#define PROTOCOL 123

int main() {
    char *config_file = "../configs/ipv4_config_server.txt";

    ipv4_layer_t *layer = malloc(sizeof(ipv4_layer_t));
    if (!layer) {
        perror("malloc ipv4_layer_t");
        return 1;
    }

    char ifname[IFACE_NAME_MAX_LENGTH];
    if (ipv4_config_read(config_file, ifname, layer->addr, layer->netmask) != 0) {
        fprintf(stderr, "Error reading IPv4 config file %s\n", config_file);
        free(layer);
        return 1;
    }

    layer->iface = eth_open(ifname);
    if (!layer->iface) {
        fprintf(stderr, "Error opening Ethernet interface %s\n", ifname);
        free(layer);
        return 1;
    }

    unsigned char buffer[1500];
    ipv4_addr_t sender;
    char sender_str[IPv4_STR_MAX_LENGTH];

    printf("Waiting for a packet...\n");
    int len = ipv4_recv(layer, PROTOCOL, buffer, sender, sizeof(buffer), -1);

    if (len > 0) {
        ipv4_addr_str(sender, sender_str);
        printf("Packet received from %s\n", sender_str);
        printf("Payload: %.*s\n", len, buffer);
    }

    eth_close(layer->iface);
    free(layer);

    return 0;
}
