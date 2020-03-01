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


int reply_IAA(struct node_properties* properties, struct received_msg* received) {
    struct msg IAA_msg;
    IAA_msg.msgID = AYA;
    IAA_msg.electionID = received->message.electionID; // the electionID is to be set to the port number of the node sending the AYA
    memcpy(IAA_msg.vectorClock, properties->vectorClock, sizeof(IAA_msg.vectorClock));
    // TODO: verify port is in group list and same address info?
    send_message(properties, received->message.electionID, &IAA_msg);
}

int send_AYA(struct node_properties* properties) {
    struct msg AYA_msg;
    AYA_msg.msgID = AYA;
    AYA_msg.electionID = (int)properties->port; // the electionID is to be set to the port number of the node sending the AYA
    memcpy(AYA_msg.vectorClock, properties->vectorClock, sizeof(AYA_msg.vectorClock));
    int n = send_message(properties, properties->coordinator, &AYA_msg);
    if (n < 0) {
        return  -1; // don't update last_AYA on error
    } else {
        properties->last_AYA = time(NULL);
        return 0;
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

    // Check for message
    struct received_msg received = receive_message(properties);

    switch(received.message.msgID) {
        case ELECT:
            reply_answer(properties, &received);
            properties->curElectionId++;
            properties->state = ELECT_STATE;
            return 0;
        case COORD:
            register_coordinator(properties, &received);
            break;
        case AYA:
            // If coordinator, reply to AYA messages
            if (properties->coordinator == properties->port) {
                reply_IAA(properties, &received);
            }
            break;
        default:
            break;
    }

    // If we're not the coordinator,send AYA messages to coordinator
    if (properties->coordinator != properties->port) {
        // If AYATime seconds have passed since last IAA received, send a new AYA
        // TODO: is this supposed to be random like in the example code
        if (time(NULL) - properties->last_IAA > properties->AYATime) {
            int n = send_AYA(properties);
            if (n < 0) {
                printf("send_aya failed\n");
                return -1; // Don't move to AYA_STATE on an error
            } else {
                properties->state = AYA_STATE;
                return 0;
            }
        }
    }

    // Debug / test TODO remove
    struct msg sndmsg;
    sndmsg.msgID = ELECT;
    sndmsg.electionID = properties->curElectionId++;
    memcpy(sndmsg.vectorClock, properties->vectorClock, sizeof(sndmsg.vectorClock));
    send_message(properties, properties->port, &sndmsg);
    struct received_msg receive = receive_message(properties);
    properties->state = STOPPED;
}





/*
    Receive IAA -> Normal state
    reach timeout = Coordinator failure detected -> Election State
    ELECT message received- > Reply ANSWER message  -> Election State
    Receive COORD message -> register new coordinator
 */
int aya_state(struct node_properties* properties) {

    // Check for message
    struct received_msg received = receive_message(properties);

    switch(received.message.msgID) {
        case ELECT:
            reply_answer(properties, &received);
            properties->curElectionId++;
            properties->state = ELECT_STATE;
            return 0;
        case COORD:
            register_coordinator(properties, &received);
            properties->state = NORMAL_STATE;
            return 0;
        case IAA:
            properties->last_IAA = time(NULL);
            properties->state = NORMAL_STATE;
            return 0;
        default:
            break;
    }

    // If timeout, coordinator failure detected
    if (time(NULL) - properties->last_AYA > properties->timeoutValue) {
        properties->curElectionId++;
        properties->state = ELECT_STATE;
        return 0;
    }

    return 0;
}





/*
    ELECT message received -> Reply ANSWER message
        Send ELECT to higher nodes,  -> AwaitAnswer state
            Elect message needs unique electionId
                Maybe of the format "%d%d", nodeId, electionCounter++?
        Receive COORD message -> register new coordinator -> Normal state

 */
int elect_state(struct node_properties* properties) {
    properties->state = STOPPED;
    // TODO
    return 0;
}





/*
    ELECT message received -> Reply ANSWER message
    Receive ANSWER -> AwaitCoordinatorState
    If no answer is received, send COORD to all nodes with lower numbers
    Receive COORD message -> register new coordinator -> Normal state
 */
int await_answer_state(struct node_properties* properties) {
    properties->state = STOPPED;
    // TODO
    return 0;
}





/*
    Receive COORD message -> register new coordinator -> Normal state
    Timeout if a COORD message isn't received within ((MAX_NODES + 1) timeout value
 */
int await_coord_state(struct node_properties* properties) {
    properties->state = STOPPED;
    // TODO
    return 0;
}





// Will attempt to receive a message. Should not be blocking (socket currently created in node.c is set to be non-blocking)
// If there is no message to receive, returned msg should have msgId of INVALID
struct received_msg receive_message(struct node_properties* properties) {
    struct received_msg received_message;
    received_message.message.msgID = INVALID;

    int len;
    char  buff[100];
    memset(&received_message.client, 0, sizeof(received_message.client));

    int size = recvfrom(properties->sockfd, &received_message.message, sizeof(received_message.message), 0, (struct sockaddr *) &received_message.client, &len);

    if (size == -1 ) {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            printf("Error receiving: %s! \n", strerror(errno));
            // TODO ?
        } else {
            return received_message;
        }
    } else {
        // Update vector clock
        merge_clocks(properties->vectorClock, received_message.message.vectorClock);

        // Log receipt
        char lg_msg[128];
        sprintf(lg_msg, "Receive %s Message: (electionId: %d):", msgTypeToStr(received_message.message.msgID), received_message.message.electionID);
        log_event(lg_msg, properties->port, properties->vectorClock, MAX_NODES);
    }

    return received_message;
}


int send_message(struct node_properties* properties, unsigned long node_id_port, struct msg* message) {

    struct addrinfo* nodeAddr = NULL;

    // Get recipient info from group_list
    for (int i = 0; i < properties->group_list.node_count; i++) {
        if (properties->group_list.list[i].port == node_id_port) {
            nodeAddr = properties->group_list.list[i].nodeaddr;
        }
    }

    if (nodeAddr == NULL) {
        printf("Cannot find id %d in group list, or address information was not loaded correctly!\n", node_id_port);
        return -1;
    }

    // Increment our clock
    properties->vectorClock[0].time++;

    // Log Send
    char lg_msg[128];
    sprintf(lg_msg, "Send %s Message to N%d: (electionId: %d):", msgTypeToStr(message->msgID), node_id_port, message->electionID);
    log_event(lg_msg, properties->port, properties->vectorClock, MAX_NODES);

    // Send message
    int bytesSent;
    bytesSent = sendto(properties->sockfd, (void *)message, sizeof(*message), 0, // TODO: MSG_CONFIRM if reply??
                       nodeAddr->ai_addr, nodeAddr->ai_addrlen);
    if (bytesSent != sizeof(*message)) {
        printf("UDP send failed \n");
        return -1;
    }

    return 0;
}


int reply_answer(struct node_properties* properties, struct received_msg* received) {
    struct msg ANSWER_msg;
    ANSWER_msg.msgID = ANSWER;
    ANSWER_msg.electionID = received->message.electionID;
    memcpy(ANSWER_msg.vectorClock, properties->vectorClock, sizeof(ANSWER_msg.vectorClock));
    // TODO: verify port is in group list and same address info??
    return send_message(properties, ntohs(received->client.sin_port), &ANSWER_msg);
}


int register_coordinator(struct node_properties* properties, struct received_msg* received) {
    properties->coordinator = ntohs(received->client.sin_port);
    return 0;
}


// Helper to update our clock from a received clock
void merge_clocks(struct clock our_vector_clock[MAX_NODES],  struct clock received_vector_clock[MAX_NODES]) {
    // Ignore our clock [0]
    for (int i = 1;  i < MAX_NODES; i++) {
        struct clock* current_clock = &our_vector_clock[i];

        for (int j = 0; j < MAX_NODES; j++) {
            struct clock* received_clock = &received_vector_clock[i];

            if (received_clock->nodeId != current_clock->nodeId)
                continue;

            if (received_clock->time > current_clock->time)
                current_clock->time = received_clock->time;

            break;
        }
    }
}