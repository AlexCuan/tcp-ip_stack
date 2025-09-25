#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include "ipv4_server.h"
#include "ipv4.h"
#include "ipv4_config.h"
#include "ipv4_route_table.h"
#include "../eth/eth.h"

ipv4_layer_t * ipv4_open(char * file_conf, char * file_conf_route) {
    ipv4_layer_t * layer = malloc(sizeof(ipv4_layer_t));
    if (!layer) {
        perror("malloc ipv4_layer_t");
        return NULL;
    }

    /* 1. Crear layer->routing_table */
    layer->routing_table = ipv4_route_table_create();
    if (!layer->routing_table) {
        free(layer);
        return NULL;
    }

    /* 2. Leer direcciones y subred de file_conf */
    char ifname[IFACE_NAME_MAX_LENGTH];
    if (ipv4_config_read(file_conf, ifname, layer->addr, layer->netmask) != 0) {
        fprintf(stderr, "Error reading IPv4 config file %s\n", file_conf);
        ipv4_route_table_free(layer->routing_table);
        free(layer);
        return NULL;
    }

    /* 3. Leer tabla de reenvío IP de file_conf_route */
    if (ipv4_route_table_read(file_conf_route, layer->routing_table) < 0) {
        fprintf(stderr, "Error reading IPv4 route table file %s\n", file_conf_route);
        ipv4_route_table_free(layer->routing_table);
        free(layer);
        return NULL;
    }

    /* 4. Inicializar capa Ethernet con eth_open() */
    layer->iface = eth_open(ifname);
    if (!layer->iface) {
        fprintf(stderr, "Error opening Ethernet interface %s\n", ifname);
        ipv4_route_table_free(layer->routing_table);
        free(layer);
        return NULL;
    }

    return layer;
}

// Helper to print hex data
void print_hex(unsigned char *data, int len) {
    for (int i = 0; i < len; i++) {
        printf("%02x ", data[i]);
        if ((i + 1) % 16 == 0) {
            printf("\n");
        }
    }
    printf("\n");
}

int ipv4_recv(ipv4_layer_t * layer, uint8_t protocol,
              unsigned char buffer[], ipv4_addr_t sender, int buf_len,
              long int timeout) {

    mac_addr_t src_mac;
    unsigned char eth_buffer[ETH_MTU];
    int payload_len;

    while (1) {
        payload_len = eth_recv(layer->iface, src_mac, ETH_TYPE_IPV4, eth_buffer, sizeof(eth_buffer), timeout);
        if (payload_len <= 0) {
            return -1; // Timeout or error
        }

        if (payload_len < sizeof(ipv4_header_t)) {
            // Packet too small to be a valid IPv4 packet
            continue;
        }

        ipv4_header_t *ip_header = (ipv4_header_t *)eth_buffer;

        // 1. Validate Version and Header Length
        if ((ip_header->version_ihl >> 4) != 4) {
            fprintf(stderr, "IPv4 Recv: Incorrect IP version\n");
            continue;
        }
        unsigned int header_len = (ip_header->version_ihl & 0x0F) * 4;
        if (header_len < sizeof(ipv4_header_t)) {
            fprintf(stderr, "IPv4 Recv: Invalid header length\n");
            continue;
        }

        // 2. Validate Checksum
        uint16_t received_checksum = ip_header->header_checksum;
        ip_header->header_checksum = 0;
        uint16_t calculated_checksum = ipv4_checksum((unsigned char *)ip_header, header_len);
        ip_header->header_checksum = received_checksum;

        if (received_checksum != htons(calculated_checksum)) {
            fprintf(stderr, "IPv4 Recv: Invalid checksum\n");
            continue;
        }

        // 3. Check destination address
        if (memcmp(ip_header->dest_addr, layer->addr, IPv4_ADDR_SIZE) != 0) {
            // Not for us
            continue;
        }

        // 4. Check protocol
        if (ip_header->protocol != protocol) {
            continue;
        }

        // 5. Get payload
        int ip_total_len = ntohs(ip_header->total_length);
        int ip_payload_len = ip_total_len - header_len;

        if (ip_payload_len <= 0) {
            continue;
        }

        // 6. Copy sender IP
        memcpy(sender, ip_header->src_addr, IPv4_ADDR_SIZE);

        // 7. Copy payload to user buffer
        int len_to_copy = (ip_payload_len > buf_len) ? buf_len : ip_payload_len;
        unsigned char *payload = eth_buffer + header_len;
        memcpy(buffer, payload, len_to_copy);

        // 8. Print payload to screen
        printf("Received IPv4 packet with protocol %d from ", protocol);
        char sender_str[IPv4_STR_MAX_LENGTH];
        ipv4_addr_str(sender, sender_str);
        printf("%s\n", sender_str);
        printf("Payload (%d bytes):\n", len_to_copy);
        print_hex(buffer, len_to_copy);

        return len_to_copy;
    }
}
