//
// Created by Cody on 2/27/2020.
//

#ifndef A2_STATE_MACHINE_H
#define A2_STATE_MACHINE_H

#include "msg.h"

typedef enum {
    NORMAL_STATE = 30,
    AYA_STATE = 31,
    ELECT_STATE = 32,
    AWAIT_ANSWER_STATE = 33,
    AWAIT_COORD_STATE = 34,
    STOPPED = 35,
} nodeState;

struct node_properties {
    nodeState      state;
    unsigned long  port;
    char *         groupListFileName;
    char *         logFileName;
    unsigned long  timeoutValue;
    unsigned long  AYATime;
    unsigned long  sendFailureProbability;

    unsigned long coordinator; // Current coordinator's id
    unsigned int curElectionId; // counter to use for choosing electionId for msg (See msg.h for details, is for debugging only)

    struct clock vectorClock[MAX_NODES]; // Note: node_properties->vectorClock[0] is to be node's own clock.

    int sockfd; // The node's socket descriptor
};

int state_main(struct node_properties* properties);

int normal_state(struct node_properties* properties);
int aya_state(struct node_properties* properties);
int elect_state(struct node_properties* properties);
int await_answer_state(struct node_properties* properties);
int await_coord_state(struct node_properties* properties);
struct msg receive_message(struct node_properties* properties);
int send_message(struct node_properties* properties, char *hostname, unsigned int port, struct msg* msg);
int reply_answer(struct node_properties* properties, struct msg* msg);
int register_coordinator(struct node_properties* properties, struct msg* msg);



#endif //A2_STATE_MACHINE_H
