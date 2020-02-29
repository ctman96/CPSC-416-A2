//
// Created by Cody on 2/27/2020.
//

#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

#include "state_machine.h"
#include "logger.h"

int state_main(struct node_properties* properties) {
    switch(properties->state) {
        case NORMAL_STATE:
            return normal_state(properties);
        case AYA_STATE:
            return aya_state(properties);
        case ELECT_STATE:
            return elect_state(properties);
        case AWAIT_ANSWER_STATE:
            return await_answer_state(properties);
        case AWAIT_COORD_STATE:
            return await_coord_state(properties);
        default:
            break;
    }
}

/*
    Normal state:
        If
            not coordinator:
                Send AYA message to Coordinator -> AYA state
            else
                AYA message received -> send IAA message
        ELECT message received -> Reply ANSWER message -> Election State
        Receive COORD message -> register new coordinator

 */
int normal_state(struct node_properties* properties) {
    // TODO
    struct msg sndmsg;
    sndmsg.msgID = ELECT;
    sndmsg.electionID = properties->curElectionId++;
    memcpy(sndmsg.vectorClock, properties->vectorClock, sizeof(sndmsg.vectorClock));

    // Debug / test TODO remove
    send_message(properties, "localhost", properties->port, &sndmsg);
    struct msg message = receive_message(properties);

    properties->state = STOPPED;
}

/*
    Receive IAA -> Normal state
    reach AYA time = Coordinator failure detected -> Election State
    ELECT message received- > Reply ANSWER message  -> Election State
    Receive COORD message -> register new coordinator
 */
int aya_state(struct node_properties* properties) {
    // TODO
}

/*
    ELECT message received -> Reply ANSWER message
        Send ELECT to higher nodes,  -> AwaitAnswer state
            Elect message needs unique electionId
                Maybe of the format "%d%d", nodeId, electionCounter++?
        Receive COORD message -> register new coordinator -> Normal state

 */
int elect_state(struct node_properties* properties) {
    // TODO
}

/*
    ELECT message received -> Reply ANSWER message
    Receive ANSWER -> AwaitCoordinatorState
    If no answer is received, send COORD to all nodes with lower numbers
    Receive COORD message -> register new coordinator -> Normal state
 */
int await_answer_state(struct node_properties* properties) {
    // TODO
}

/*
    Receive COORD message -> register new coordinator -> Normal state
    Timeout if a COORD message isn't received within ((MAX_NODES + 1) timeout value
 */
int await_coord_state(struct node_properties* properties) {
    // TODO
}

// Will attempt to receive a message. Should not be blocking (socket currently created in node.c is set to be non-blocking)
// If there is no message to receive, returned msg should have msgId of INVALID
struct msg receive_message(struct node_properties* properties) {
    struct msg message;
    message.msgID = INVALID;

    struct sockaddr_in client;
    int len;
    char  buff[100];
    memset(&client, 0, sizeof(client));

    int size = recvfrom(properties->sockfd, &message, sizeof(message), 0, (struct sockaddr *) &client, &len);

    if (size == -1 ) {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            fprintf(stderr, "Error receiving!");
            // TODO ?
        } else {
            return message;
        }
    } else {
        // Debug log TODO remove
        /*char debug_msg[128];
        sprintf(debug_msg, "Receive_message: (id: %d , election: %d):", message.msgID, message.electionID);
        log_event(debug_msg, message.msgID, message.vectorClock, MAX_NODES);*/

        // Update vector clock
        // Ignore our clock [0]
        for (int i = 1;  i < MAX_NODES; i++) {
            struct clock* current_clock = &properties->vectorClock[i];

            for (int j = 0; j < MAX_NODES; j++) {
                struct clock* received_clock = &properties->vectorClock[i];

                if (received_clock->nodeId != current_clock->nodeId)
                    continue;

                if (received_clock->time > current_clock->time)
                    current_clock->time = received_clock->time;

                break;
            }
        }

        // Log receipt
        char lg_msg[128];
        sprintf(lg_msg, "Receive %s Message: (electionId: %d):", msgTypeToStr(message.msgID), message.electionID);
        log_event(lg_msg, properties->port, properties->vectorClock, MAX_NODES);
    }

    // TODO: also will want to pass back client address info, so maybe change return type to a wrapper struct
    return message;
}

int send_message(struct node_properties* properties, char *hostname, unsigned int port, struct msg* message) {

    // Setup recipient information
    struct addrinfo hints, *nodeAddr;
    nodeAddr = NULL;

    memset(&hints, 0, sizeof(hints));
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_family = AF_INET;
    hints.ai_protocol = IPPROTO_UDP;

    char port_str[16];
    sprintf(port_str, "%d", port);
    if (getaddrinfo(hostname, port_str, &hints, &nodeAddr)) {
        printf("Couldn't lookup hostname\n");
        return -1;
    }

    // Increment our clock
    properties->vectorClock[0].time++;

    // Log Send
    char lg_msg[128];
    sprintf(lg_msg, "Send %s Message: (electionId: %d):", msgTypeToStr(message->msgID), message->electionID);
    log_event(lg_msg, properties->port, properties->vectorClock, MAX_NODES);

    // Send message
    int bytesSent;
    bytesSent = sendto(properties->sockfd, (void *)message, sizeof(*message), 0, // TODO: MSG_CONFIRM if reply??
                       nodeAddr->ai_addr, nodeAddr->ai_addrlen);
    if (bytesSent != sizeof(*message)) {
        perror("UDP send failed: ");
        return -1;
    }
}

int reply_answer(struct node_properties* properties, struct msg* msg) {
    // TODO
}

int register_coordinator(struct node_properties* properties, struct msg* msg) {
    // TODO
}

