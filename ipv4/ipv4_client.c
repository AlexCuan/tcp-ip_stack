#include <stdio.h>
#include <string.h>
#include "ipv4.h"

#define PROTOCOL 123

int main() {
    char *config_file = "../configs/ipv4_config_client.txt";
    char *route_table_file = "../configs/ipv4_route_table_client.txt";
    char *server_ip_str = "192.100.101.101";
    char *message = "Hello from the client!";

    ipv4_layer_t *layer = ipv4_open(config_file, route_table_file);
    if (!layer) {
        printf("Error opening IPv4 layer\n");
        return 1;
    }

    ipv4_addr_t dest_addr;
    if (ipv4_str_addr(server_ip_str, dest_addr) != 0) {
        printf("Invalid destination IP address\n");
        return 1;
    }

    printf("Sending packet to %s...\n", server_ip_str);
    int bytes_sent = ipv4_send(layer, dest_addr, PROTOCOL, (unsigned char *)message, strlen(message));

    if (bytes_sent > 0) {
        printf("Packet sent successfully!\n");
    } else {
        printf("Error sending packet\n");
    }

    return 0;
}
