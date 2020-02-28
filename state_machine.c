//
// Created by Cody on 2/27/2020.
//

#include "state_machine.h"

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

int normal_state(struct node_properties* properties) {
    // TODO
    properties->state = STOPPED;
}

int aya_state(struct node_properties* properties) {
    // TODO
}

int elect_state(struct node_properties* properties) {
    // TODO
}

int await_answer_state(struct node_properties* properties) {
    // TODO
}

int await_coord_state(struct node_properties* properties) {
    // TODO
}