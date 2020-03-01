//
// Created by Cody on 2/29/2020.
//

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <limits.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>

#include "grouplist.h"

int load_group_list(char* groupListFile, struct group_list* group_list, unsigned long our_port) {
    group_list->node_count = 0;

    FILE* fp;
    // On "-" read from stdin
    if (groupListFile == "-") {
        fp = stdin;
    } else {
        fp = fopen(groupListFile, "r");
    }

    // If error loading, return error
    if (!fp) {
        printf("Unable to open groupListFile: %s \n", groupListFile);
        return -1;
    }

    char *line_buffer = NULL;
    size_t buffer_size = 0;
    ssize_t line_size;
    int found_self = -1;

    // Read all lines
    line_size =  getline(&line_buffer, &buffer_size, fp);
    while( line_size >= 0 ) {
        struct node_info* node_info = &group_list->list[group_list->node_count];
        group_list->node_count++;

        char line_cpy[line_size];
        strcpy(line_cpy, line_buffer);

        // Split by delimiter (whitespace)
        node_info->hostname = strtok(line_cpy, " ");
        char *port = strtok(NULL, " ");
        // Check for invalid data / failed parsing
        if (node_info->hostname == NULL || port == NULL) {
            printf("Error parsing group list, line %d: %s \n", group_list->node_count, line_buffer);
            free(line_buffer);
            fclose(fp);
            return -1;
        }
        // Attempt to convert port to number
        char * end;
        node_info->port = strtoul(port, &end, 10);
        if (port == end) {
            printf("group list port conversion error: %s\n", port);
            free(line_buffer);
            fclose(fp);
            return -1;
        }

        // Check for self
        if (node_info->port == our_port) {
            found_self = 1;
        }

        // Load address info
        struct addrinfo hints;
        node_info->nodeaddr = NULL;

        memset(&hints, 0, sizeof(hints));
        hints.ai_socktype = SOCK_DGRAM;
        hints.ai_family = AF_INET;
        hints.ai_protocol = IPPROTO_UDP;

        char port_str[16];
        sprintf(port_str, "%d", node_info->port);
        if (getaddrinfo(node_info->hostname, port_str, &hints, &node_info->nodeaddr)) {
            printf("Couldn't lookup hostname: %s %s\n", node_info->hostname, port);
            free(line_buffer);
            fclose(fp);
            return -1;
        }

        struct sockaddr_in *addr;
        addr = (struct sockaddr_in *)node_info->nodeaddr;
        char ip[INET_ADDRSTRLEN];
        printf("Group List - Loaded node: %s %d %s \n", node_info->hostname, node_info->port, inet_ntoa((struct in_addr)addr->sin_addr));

        line_size = getline(&line_buffer, &buffer_size, fp);
    }

    free(line_buffer);
    fclose(fp);

    if (found_self == -1) {
        printf("Error: didn't find own port in goup list file!\n");
        return -1;
    }

    return 0;
}