#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "ipv4.h"
#include "ipv4_config.h"
#include "../eth/eth.h"

#define PROTOCOL 123

int main() {
    char *config_file = "../configs/ipv4_config_client.txt";
    char *server_ip_str = "192.100.101.101";
    char *message = "Hello from the client!";

    printf("DEBUG: Opening IPv4 layer with config: '%s'\n", 
           config_file);

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

    printf("DEBUG: IPv4 layer opened successfully.\n");

    ipv4_addr_t dest_addr;
    printf("DEBUG: Parsing destination IP string: '%s'\n", server_ip_str);
    if (ipv4_str_addr(server_ip_str, dest_addr) != 0) {
        printf("Invalid destination IP address\n");
        eth_close(layer->iface);
        free(layer);
        return 1;
    }
    printf("DEBUG: Destination IP parsed to: %d.%d.%d.%d\n",
           dest_addr[0], dest_addr[1], dest_addr[2], dest_addr[3]);

    printf("Sending packet to %s...\n", server_ip_str);

    printf("DEBUG: Calling ipv4_send with:\n");
    printf("  - dest_addr: %d.%d.%d.%d\n", dest_addr[0], dest_addr[1], dest_addr[2], dest_addr[3]);
    printf("  - protocol: %d\n", PROTOCOL);
    printf("  - payload: '%s'\n", message);
    printf("  - payload_len: %zu\n", strlen(message));

    int bytes_sent = ipv4_send(layer, dest_addr, PROTOCOL, (unsigned char *)message, strlen(message));

    if (bytes_sent > 0) {
        printf("Packet sent successfully! (%d bytes)\n", bytes_sent);
    } else {
        printf("Error sending packet. ipv4_send returned: %d\n", bytes_sent);
    }

    eth_close(layer->iface);
    free(layer);
    return 0;
}
