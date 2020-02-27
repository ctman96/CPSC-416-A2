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
    fp = fopen(LOG_FILE,"w"); // Open new file

    if (!fp) {
        fprintf(stderr, "Unable to open log file!");
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
        fprintf(stderr, "Logger not initialized!");
        return -1;
    }

    // Log event description
    fprintf(fp, "%s\n", text);

    // Log vector clock
    fprintf(fp, "N%d {", node);
    bool first = true;
    for (int i = 0; i < clock_length; i++) {
        // Ignore nodes we don't know the time of
        if (clock[i].time <= 0) continue;

        if (!first) fprintf(fp, " , ");
        fprintf(fp, "\"N%d\" : %d", clock[i].nodeId, clock[i].time);
        first = false;
    }
    fprintf(fp, "}\n");

    return 0;
}

// Log debug lines to log
int log_debug(char* text) {
    if (!running || !fp) {
        fprintf(stderr, "Logger not initialized!");
        return -1;
    }

    fprintf(fp, "%s\n", text);
}