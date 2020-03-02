//
// Created by Cody on 2/26/2020.
//

#ifndef A2_LOGGER_H
#define A2_LOGGER_H

#include "msg.h"

// Initialize logger
int init_logger(char* fileName);

// Log events for shiviz
int log_event(char* text, int node, struct clock* clock, int clock_length);

// Log debug lines
int log_debug(char* text);

void close_logger();

#endif //A2_LOGGER_H
