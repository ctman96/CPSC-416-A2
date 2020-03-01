//
// Created by Cody on 3/1/2020.
//

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <errno.h>
#include <time.h>
#include <netdb.h>
#include <fcntl.h>

#include "test.h"
#include "grouplist.h"
#include "logger.h"
#include "state_machine.h"

void setup(struct node_properties* test_properties) {
    // Setup socket
    test_properties->port = 12345;
    struct sockaddr_in servAddr;
    if ( (test_properties->sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }
    int flags = fcntl(test_properties->sockfd, F_GETFL);
    flags |= O_NONBLOCK;
    fcntl(test_properties->sockfd, F_SETFL, flags);
    memset(&servAddr, 0, sizeof(servAddr));
    servAddr.sin_family = AF_INET;
    servAddr.sin_port = htons(test_properties->port);
    servAddr.sin_addr.s_addr = INADDR_ANY;
    if ( bind(test_properties->sockfd, (const struct sockaddr *)&servAddr,
              sizeof(servAddr)) < 0 )  {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    // TODO
}

int test_group_list() {
    printf("\n======= GROUP_LIST =======\n");
    int status = 0;

    // -------------
    // Test reading file:
    printf("=== Test reading from file ===\n");
    FILE* fp = fopen("test_group_list.list", "w");
    fprintf(fp, "localhost 8008\nwww.google.com 12345\nstudents.cs.ubc.ca 98765");
    fclose(fp);
    struct group_list group_list1;
    if (load_group_list("test_group_list.list", &group_list1, 8008)) {
        printf("load_group_list failed unexpectedly! \n");
        printf("=== Test reading from file ===\n");
        printf("======= GROUP_LIST =======\n");
        return -1;
    }

    if (group_list1.node_count != 3) {
        status = -1;
        printf("Group list loading loaded incorrect number of nodes!");
    }

    int cmp1, cmp2, cmp3;
    cmp1 = strcmp(group_list1.list[0].hostname, "localhost");
    cmp2 = strcmp(group_list1.list[1].hostname, "www.google.com");
    cmp3 = strcmp(group_list1.list[2].hostname, "students.cs.ubc.ca");


    if (cmp1 != 0 || group_list1.list[0].port != 8008
        || cmp2 != 0 || group_list1.list[1].port != 12345
        || cmp3 != 0 || group_list1.list[2].port != 98765) {

        printf("%s %d %d , %s %d %d , %s %d %d\n",
               group_list1.list[0].hostname, cmp1, group_list1.list[0].port,
               group_list1.list[1].hostname, cmp2, group_list1.list[1].port,
               group_list1.list[2].hostname, cmp3, group_list1.list[2].port);

        status = -1;
        printf("Group list incorrectly parsed data!\n");
    }
    printf("=== Test reading from file ===\n");
    // -------------

    // -------------
    // Test reading stdin
    printf("=== Test reading from stdin ===\n");
    freopen("test_group_list.list", "r", stdin);
    struct group_list group_list;
    if (load_group_list("-", &group_list, 8008) < 0) {
        printf("load_group_list failed unexpectedly!\n");
        printf("=== Test reading from stdin ===\n");
        printf("======= GROUP_LIST =======\n");
        return -1;
    };

    if (group_list.node_count != 3) {
        status = -1;
        printf("Group list loading loaded incorrect number of nodes!");
    }

    cmp1 = strcmp(group_list.list[0].hostname, "localhost");
    cmp2 = strcmp(group_list.list[1].hostname, "www.google.com");
    cmp3 = strcmp(group_list.list[2].hostname, "students.cs.ubc.ca");

    if (cmp1 != 0 || group_list.list[0].port != 8008
        || cmp2 != 0 || group_list.list[1].port != 12345
        || cmp3 != 0 || group_list.list[2].port != 98765) {
        status = -1;
        printf("Group list incorrectly parsed data!\n");
    }

    //  compare resolved address info?
    printf("=== Test reading from stdin ===\n");
    // -------------


    printf("======= GROUP_LIST =======\n");
    return status;
}

int test_send_message(struct node_properties * test_properties) {
    printf("\n======= SEND_MESSAGE =======\n");
    int status = 0;

    printf("======= SEND_MESSAGE =======\n");
    return status;
}

int test_receive_message(struct node_properties * test_properties) {
    printf("\n======= RECEIVE_MESSAGE =======\n");
    int status = 0;
    // TODO
    printf("======= RECEIVE_MESSAGE =======\n");
    return status;
}

int test_normal_state(struct node_properties * test_properties) {
    printf("\n======= NORMAL_STATE =======\n");
    int status = 0;
    // TODO
    printf("======= NORMAL_STATE =======\n");
    return status;
}

int test_aya_state(struct node_properties * test_properties) {
    printf("\n======= AYA_STATE =======\n");
    int status = 0;
    // TODO
    printf("======= AYA_STATE =======\n");
    return status;
}

int test() {
    printf("\n=========\n TESTING \n=========\n");

    int gl = test_group_list();

    // Setup properties
    struct node_properties test_properties;
    setup(&test_properties);

    int snd = test_send_message(&test_properties);
    int rcv = test_receive_message(&test_properties);
    int nrm = test_normal_state(&test_properties);
    int aya = test_aya_state(&test_properties);

    if (gl + snd + rcv + nrm + aya < 0) {
        printf("Tests failed!\n");
        printf("========= TESTING =========\n");
        return -1;
    }
    printf("\nAll tests passed!\n");
    printf("========= TESTING =========\n");
    return 0;
}