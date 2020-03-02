//
// Created by Cody on 2/26/2020.
//

#include "logger.h"
#include <stdio.h>
#include <stdbool.h>

static bool running;
static char* log_file = "event.log";
static FILE* fp;


int init_logger(char* file_name){
    if (running) return -1;
    log_file = file_name;
    fp = fopen(log_file,"w"); // Open new file

    if (!fp) {
        printf("Unable to open log file!");
        return -1;
    }

    running = true;
    return 0;
}


/*
 * Arguments:
 *      text - Event description text to be logged
 *      node - Logging node's id number
 *      clock - array of clocks (vector clock)
 *      clock_length - length of clock array
 *
 * Returns:
 *      0   - Successful
 *      -1  - Error
 */
int log_event(char* text, int node, struct clock* clock, int clock_length) {
    if (!running || !fp) {
        printf("Logger not initialized!");
        return -1;
    }

    char event_str[256] = "";
    int pos = 0;

    // Log event description
    pos += sprintf(&event_str[pos], "%s\n", text);

    // Log vector clock
    pos += sprintf(&event_str[pos],  "N%d {", node);
    bool first = true;
    for (int i = 0; i < clock_length; i++) {
        // Ignore nodes we don't know the time of
        if (clock[i].time <= 0) continue;

        if (!first) {
            pos += sprintf(&event_str[pos], " , ");
        }
        pos += sprintf(&event_str[pos], "\"N%d\" : %d", clock[i].nodeId, clock[i].time);
        first = false;
    }
    pos += sprintf(&event_str[pos], "}\n");

    fprintf(fp, event_str);

    return 0;
}

// Log debug lines to log
int log_debug(char* text) {
    if (!running || !fp) {
        printf("Logger not initialized!");
        return -1;
    }

    fprintf(fp, "%s\n", text);

    return 0;
}