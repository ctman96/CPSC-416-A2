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

int load_group_list(char* groupListFile, struct group_list* group_list) {
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
        fclose(fp);
        return -1;
    }

    char *line_buffer = NULL;
    size_t buffer_size = 0;

    // Read all lines
    while( getline(&line_buffer, &buffer_size, fp) >= 0 ) {
        struct node_info* node_info = &group_list->list[group_list->node_count];
        group_list->node_count++;

        // Split by delimiter (whitespace)
        node_info->hostname = strtok(line_buffer, " ");
        char *port = strtok(line_buffer, " ");
        // Check for invalid data / failed parsing
        if (node_info->hostname == NULL || port == NULL) {
            printf("Error parsing groupListFile, line %d: %s \n", group_list->node_count, line_buffer);
            free(line_buffer);
            fclose(fp);
            return -1;
        }
        // Attempt to convert port to number
        char * end;
        node_info->port = strtoul(port, &end, 10);
        if (port == end) {
            printf("Port conversion error: %s\n", port);
            free(line_buffer);
            fclose(fp);
            return -1;
        }

        // Load address info
        struct addrinfo hints;
        node_info->nodeaddr = NULL;

        memset(&hints, 0, sizeof(hints));
        hints.ai_socktype = SOCK_DGRAM;
        hints.ai_family = AF_INET;
        hints.ai_protocol = IPPROTO_UDP;

        if (getaddrinfo(node_info->hostname, port, &hints, &node_info->nodeaddr)) {
            printf("Couldn't lookup hostname\n");
            free(line_buffer);
            fclose(fp);
            return -1;
        }
    }

    free(line_buffer);
    fclose(fp);
    return 0;
}