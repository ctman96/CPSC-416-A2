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

// Should be mostly just a copy of normal setup code from node.c
void setup(struct node_properties* test_properties) {
    // Setup socket
    test_properties->port = 8008;
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

    FILE* fp = fopen("test_group_list.list", "w");
    fprintf(fp, "localhost 8008\nwww.google.com 12345\nstudents.cs.ubc.ca 98765");
    fclose(fp);

    load_group_list("test_group_list.list", &test_properties->group_list, test_properties->port);

    // Our clock is vectorClock[0] for some simplicity
    test_properties->vectorClock[0].nodeId = test_properties->port;
    test_properties->vectorClock[0].time = 1;

    //  Use group list to initialize vector clocks and determine coordinator
    int group_list_cursor = 0;
    test_properties->coordinator = test_properties->port;
    for (int i = 1; i < MAX_NODES; i++) {
        // Skip ourselves, since we're [0]
        if (test_properties->group_list.list[group_list_cursor].port == test_properties->port) {
            group_list_cursor++;
        }

        // Initialize other nodes to time 0
        test_properties->vectorClock[i].nodeId = test_properties->group_list.list[group_list_cursor].port;
        test_properties->vectorClock[i].time = 0;

        // Set coordinator to highest nodeId
        if (test_properties->group_list.list[group_list_cursor].port > test_properties->coordinator) {
            test_properties->coordinator = test_properties->group_list.list[group_list_cursor].port;
        }
        group_list_cursor++;
    }

    test_properties->curElectionId = test_properties->port * 100000; // Attempt at unique electionIds per node
    test_properties->last_AYA = time(NULL);
    test_properties->last_IAA = time(NULL);


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

    struct sockaddr_in client;
    int len;
    char  buff[100];
    memset(&client, 0, sizeof(client));

    // ------
    printf("=== Test sending message ===\n");
    struct msg sndmsg;
    sndmsg.msgID = ELECT;
    sndmsg.electionID = 1001;
    memcpy(sndmsg.vectorClock, test_properties->vectorClock, sizeof(sndmsg.vectorClock));
    int clock_before = test_properties->vectorClock[0].time;
    if (send_message(test_properties, test_properties->port, &sndmsg) < 0) {
        printf("send_message failed unexpectedly!\n");
        printf("=== Test sending message ===\n");
        printf("======= SEND_MESSAGE =======\n");
        return -1;
    }

    // Receive message
    struct msg rcvmessage;
    int size = recvfrom(test_properties->sockfd, &rcvmessage, sizeof(rcvmessage), 0, (struct sockaddr *) &client, &len);
    if (size < 0) {
        printf("Receiving error");
        printf("=== Test sending message ===\n");
        printf("======= SEND_MESSAGE =======\n");
        return -1;
    }

    if (rcvmessage.msgID == INVALID) {
        printf("Error receiving message!\n");
    }

    if (rcvmessage.msgID != sndmsg.msgID || rcvmessage.electionID != sndmsg.electionID) {
        printf("Message contents do not match up!\n");
        status = -1;
    }

    if (test_properties->vectorClock[0].time != clock_before+1) {
        printf("Clock was not incremented!\n");
        status = -1;
    }

    // TODO

    printf("=== Test sending message ===\n");
    // ------

    printf("======= SEND_MESSAGE =======\n");
    return status;
}




int test_receive_message(struct node_properties * test_properties) {
    printf("\n======= RECEIVE_MESSAGE =======\n");
    int status = 0;

    printf("=== Test receiving message ===\n");

    // Setup
    struct msg message;
    message.msgID = AYA;
    message.electionID = test_properties->port;
    memcpy(message.vectorClock, test_properties->vectorClock, sizeof(message.vectorClock));
    test_properties->vectorClock[1].time = 0;
    message.vectorClock[1].time = 10;

    // Send message
    int bytesSent;
    bytesSent = sendto(test_properties->sockfd, (void *)&message, sizeof(message), 0,
                       test_properties->group_list.list[0].nodeaddr->ai_addr, test_properties->group_list.list[0].nodeaddr->ai_addrlen);
    if (bytesSent != sizeof(message)) {
        printf("UDP send failed \n");
        printf("=== Test receiving message ===\n");
        return -1;
    }

    struct received_msg received = receive_message(test_properties);
    if (received.message.msgID != message.msgID || received.message.electionID != message.electionID) {
        printf("Message contents do not match up!\n");
        status = -1;
    }

    if (test_properties->vectorClock[1].time != 10) {
        printf("Vector clock wasn't updated properly!\n");
        status = -1;
    }

    printf("=== Test receiving message ===\n");

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

    // Test group list first, since need it for setup
    int gl = test_group_list();

    // Setup properties
    struct node_properties test_properties;
    setup(&test_properties);

    int snd = test_send_message(&test_properties);
    int rcv = test_receive_message(&test_properties);
    int nrm = test_normal_state(&test_properties);
    int aya = test_aya_state(&test_properties);

    if (gl + snd + rcv + nrm + aya < 0) {
        printf("\nTests failed!\n");
        printf("========= TESTING =========\n");
        return -1;
    }
    printf("\nAll tests passed!\n");
    printf("========= TESTING =========\n");
    return 0;
}