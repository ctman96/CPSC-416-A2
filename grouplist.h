//
// Created by Cody on 2/29/2020.
//

#ifndef A2_GROUPLIST_H
#define A2_GROUPLIST_H

#include "msg.h"

struct node_info {
    char hostname[256];
    unsigned long port;
    struct addrinfo *nodeaddr;
};

struct group_list {
    int node_count;
    struct node_info list[MAX_NODES];
};

int load_group_list(char* groupListFile, struct group_list* group_list, unsigned long our_port);


#endif //A2_GROUPLIST_H
