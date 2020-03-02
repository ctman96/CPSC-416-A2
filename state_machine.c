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
    // TODO: verify port is in group list and same address info?
    return send_message(properties, received->message.electionID, &IAA_msg);
}

int send_AYA(struct node_properties* properties) {
    struct msg AYA_msg;
    AYA_msg.msgID = AYA;
    AYA_msg.electionID = (int)properties->port; // the electionID is to be set to the port number of the node sending the AYA
    int n = send_message(properties, properties->coordinator, &AYA_msg);
    if (n < 0) {
        return  -1; // don't update last_AYA on error
    } else {
        properties->last_AYA = time(NULL);
        return 0;
    }
}

// sends ELECT message to all nodes in group list with higher port
int send_ELECTS(struct node_properties* properties) {
    for (int i = 1; i < properties->group_list.node_count; i++) {
        if (properties->group_list.list[i].port > properties->port) {
            struct msg ELECT_msg;
            ELECT_msg.msgID = ELECT;
            ELECT_msg.electionID = properties->curElectionId;
            if (send_message(properties, properties->group_list.list[i].port, &ELECT_msg) < 0) {
                printf("Send elects error\n");
                return -1;
            }
        }
    }
    return 0;
}

// sends COORDs to all nodes with lower port
int send_COORDS(struct node_properties* properties) {
    // start from one, as self is index zero. Send coords to all lower ports
    for (int i = 1; i <= properties->group_list.node_count; i++) {
        if (properties->group_list.list[i].port < properties->port) {
            struct msg COORD_msg;
            COORD_msg.msgID = COORD;
            COORD_msg.electionID = properties->curElectionId;
            if (send_message(properties, properties->group_list.list[i].port, &COORD_msg) < 0) {
                printf("Send coords error\n");
                return -1;
            }
        }
    }
    return 0;
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
            if (reply_answer(properties, &received) < 0) return -1;
            printf("Switching from NORMAL to ELECT state\n");
            properties->curElectionId++;
            properties->state = ELECT_STATE;
            return 0;
        case COORD:
            register_coordinator(properties, &received);
            break;
        case AYA:
            // If coordinator, reply to AYA messages
            if (properties->coordinator == properties->port) {
                if (reply_IAA(properties, &received) < 0) return -1;
            }
            break;
        default:
            break;
    }

    // If we're not the coordinator,send AYA messages to coordinator
    if (properties->coordinator != properties->port) {
        // If AYATime seconds have passed since last IAA received, send a new AYA
        if (time(NULL) - properties->last_IAA > properties->rand_aya_time) {
            int n = send_AYA(properties);
            if (n < 0) {
                printf("send_aya failed\n");
                return -1; // Don't move to AYA_STATE on an error
            } else {
                printf("Switching from NORMAL to AYA state\n");
                properties->state = AYA_STATE;
                return 0;
            }
        }
    }
}


void set_rand_aya(struct node_properties* properties) {
    unsigned long rn = random();
    properties->rand_aya_time = rn % (2*properties->AYATime);
}

// helper to set values when switching to normal
void to_normal(struct node_properties* properties) {
    properties->last_IAA = time(NULL);
    set_rand_aya(properties);
    properties->state = NORMAL_STATE;
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
            if (reply_answer(properties, &received) < 0) return -1;
            printf("Switching from AYA to ELECT state\n");
            properties->curElectionId++;
            properties->state = ELECT_STATE;
            return 0;
        case COORD:
            register_coordinator(properties, &received);
            printf("Switching from AYA to NORMAL state\n");
            to_normal(properties);
            return 0;
        case IAA:
            to_normal(properties);
            return 0;
        default:
            break;
    }

    // If timeout, coordinator failure detected
    if (time(NULL) - properties->last_AYA > properties->timeoutValue) {
        printf("Switching from AYA to ELECT state\n");
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

    // first respond to recieved messages
    struct received_msg received = receive_message(properties);
    switch(received.message.msgID) {
        case COORD:
            register_coordinator(properties, &received);
            printf("Switching from ELECT to NORMAL state\n");
            to_normal(properties);
            return 0;
        case ELECT:
            if (reply_answer(properties, &received) < 0) return -1;
            break;
        default:
            break;
    }
    
    // send out election
    send_ELECTS(properties);
    // set time of election to check for timeout later
    properties->ELECT_time = time(NULL);
    printf("Switching from ELECT to NORMAL state\n");
    properties->state = AWAIT_ANSWER_STATE;
    return 0;
}


/*
    ELECT message received -> Reply ANSWER message
    Receive ANSWER -> AwaitCoordinatorState
    If no answer is received, send COORD to all nodes with lower numbers
    Receive COORD message -> register new coordinator -> Normal state
 */
int await_answer_state(struct node_properties* properties) {
    struct received_msg received = receive_message(properties);
    switch(received.message.msgID) {
        case COORD:
            register_coordinator(properties, &received);
            printf("Switching from AWAIT_ANSWER_STATE to NORMAL_STATE\n");
            to_normal(properties);
            return 0;
        case ELECT:
            if (reply_answer(properties, &received) < 0) return -1;
            break;
        case ANSWER:
            printf("Switching from AWAIT_ANSWER_STATE to AWAIT_COORD_STATE\n");
            properties->AWAIT_COORD_time = time(NULL);
            properties->state = AWAIT_COORD_STATE;
            return 0;
        default:
            break;
    }

    // if waited for more than timeout without an answer, then node is new coordinator
    if (time(NULL) - properties->ELECT_time > properties->timeoutValue) {
        send_COORDS(properties);
        properties->coordinator = properties->port;
        printf("Switching from AWAIT_ANSWER_STATE to NORMAL_STATE\n");
        properties->state = NORMAL_STATE;
    }

    return 0;
}





/*
    Receive COORD message -> register new coordinator -> Normal state
    Timeout if a COORD message isn't received within ((MAX_NODES + 1) timeout value
 */
int await_coord_state(struct node_properties* properties) {
    struct received_msg received = receive_message(properties);
    switch(received.message.msgID) {
        case COORD:
            register_coordinator(properties, &received);
            printf("Switching from AWAIT_COORD_STATE to NORMAL_STATE\n");
            to_normal(properties);
            return 0;
        case ELECT:
            if (reply_answer(properties, &received) < 0) return -1;
            break;
        default:
            break;
    }

    // If times out, call new election
    if (time(NULL) - properties->AWAIT_COORD_time > (properties->timeoutValue * (MAX_NODES + 1))) {
        properties->curElectionId++;
        properties->state = ELECT_STATE;
    }

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

    // Increment our clock and add to msg
    properties->vectorClock[0].time++;
    memcpy(message->vectorClock, properties->vectorClock, sizeof(message->vectorClock));

    // Log Send
    char lg_msg[128];
    sprintf(lg_msg, "Send %s Message to N%d: (electionId: %d):", msgTypeToStr(message->msgID), node_id_port, message->electionID);
    log_event(lg_msg, properties->port, properties->vectorClock, MAX_NODES);

    // Check if packet should be "dropped"
    int rn = random();
    int sc = rn % 99;
    if (sc < properties->sendFailureProbability) {
        log_debug("Debug: Message dropped");
        return 0;
    }

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