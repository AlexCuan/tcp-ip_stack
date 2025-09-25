//
// Created by student on 9/22/25.
//

#include "ipv4_client.h"
#include "ipv4.h"
#include "ipv4_config.h"
#include "ipv4_route_table.h"
#include "../eth/eth.h"
#include "../arp/arp.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <arpa/inet.h>

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


// Initialize the IPv4 client with ipv4_open()



// Main function

int main(int argc, char* argv[]) {
}

int ipv4_send (ipv4_layer_t * layer, ipv4_addr_t dst, uint8_t protocol,  unsigned char * payload, int payload_len){
    ipv4_route_t *route = ipv4_route_table_lookup(layer->routing_table, dst);
    if (!route) {
        return -1;
    }

    mac_addr_t next_hop_mac;
    ipv4_addr_t next_hop_ip;

    if (memcmp(route->gateway_addr, IPv4_ZERO_ADDR, IPv4_ADDR_SIZE) == 0) {
        memcpy(next_hop_ip, dst, IPv4_ADDR_SIZE);
    } else {
        memcpy(next_hop_ip, route->gateway_addr, IPv4_ADDR_SIZE);
    }

    if (arp_resolve(layer->iface, next_hop_ip, next_hop_mac) != 0) {
        return -1;
    }

    int header_len = sizeof(ipv4_header_t);
    int total_len = header_len + payload_len;
    unsigned char* buffer = malloc(total_len);

    ipv4_header_t* ip_header = (ipv4_header_t*) buffer;
    ip_header->version_ihl = (4 << 4) | 5;
    ip_header->type_of_service = 0;
    ip_header->total_length = htons(total_len);
    ip_header->identification = 0;
    ip_header->flags_fragment_offset = 0;
    ip_header->time_to_live = 64;
    ip_header->protocol = protocol;
    ip_header->header_checksum = 0;
    memcpy(ip_header->src_addr, layer->addr, IPv4_ADDR_SIZE);
    memcpy(ip_header->dest_addr, dst, IPv4_ADDR_SIZE);

    uint16_t checksum = ipv4_checksum((unsigned char*)ip_header, header_len);
    ip_header->header_checksum = htons(checksum);

    memcpy(buffer + header_len, payload, payload_len);

    int bytes_sent = eth_send(layer->iface, next_hop_mac, ETH_TYPE_IPV4, buffer, total_len);

    free(buffer);

    return bytes_sent;
}
