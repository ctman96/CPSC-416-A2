
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

#include "msg.h"
#include "logger.h"
#include "grouplist.h"
#include "state_machine.h"


// The purpose of this file is to provide insight into how to make various library
// calls to perfom some of the key functions required by the application. You are
// free to completely ignore anything in this file and do things whatever way you
// want provided it conforms with the assignment specifications. Note: this file
// compiles and works on the deparment servers running Linux. If you work in
// a different environment your mileage may vary. Remember that whatever you
// do the final program must run on the department Linux machines. 


void usage(char * cmd) {
  printf("usage: %s  portNum groupFileList logFile timeoutValue averageAYATime failureProbability \n",
	 cmd);
}


int main(int argc, char ** argv) {

  // Struct to contain all properties
  struct node_properties properties;
  properties.state = NORMAL_STATE;

  if (argc != 7) {
    usage(argv[0]);
    return -1;
  }

  // Some code illustrating how to parse command line arguments.
  // This cod will probably have to be changed to match how you
  // decide to do things. 
  
  char * end;
  int err = 0;

  properties.port = strtoul(argv[1], &end, 10);
  if (argv[1] == end) {
    printf("Port conversion error\n");
    err++;
  }

  properties.groupListFileName = argv[2];
  properties.logFileName       = argv[3];

  properties.timeoutValue      = strtoul(argv[4], &end, 10);
  if (argv[4] == end) {
    printf("Timeout value conversion error\n");
    err++;
  }

  properties.AYATime  = strtoul(argv[5], &end, 10);
  if (argv[5] == end) {
    printf("AYATime conversion error\n");
    err++;
  }

  properties.sendFailureProbability  = strtoul(argv[6], &end, 10);
  if (argv[5] == end) {
    printf("sendFailureProbability conversion error\n");
    err++;
  }

  if (init_logger(properties.logFileName) == -1) err++;

  if (load_group_list(properties.groupListFileName, &properties.group_list) < 0) {
    printf("Error loading group list!\n");
    err++;
  }

  log_debug("Testing Testing");
  log_event("Started N3", 3, properties.vectorClock, MAX_NODES);
  
  printf("Port number:              %d\n", properties.port);
  printf("Group list file name:     %s\n", properties.groupListFileName);
  printf("Log file name:            %s\n", properties.logFileName);
  printf("Timeout value:            %d\n", properties.timeoutValue);
  printf("AYATime:                  %d\n", properties.AYATime);
  printf("Send failure probability: %d\n", properties.sendFailureProbability);
  printf("Some examples of how to format data for shiviz\n");
  printf("Starting up Node %d\n", properties.port);
  
  printf("N%d {\"N%d\" : %d }\n", properties.port, properties.port, 1);
  printf("Sending to Node 1\n");
  printf("N%d {\"N%d\" : %d }\n", properties.port, properties.port, 2);
  
  if (err) {
    printf("%d conversion error%sencountered, program exiting.\n",
	   err, err>1? "s were ": " was ");
    return -1;
  }

  // Our clock is vectorClock[0] for some simplicity
  properties.vectorClock[0].nodeId = properties.port;
  properties.vectorClock[0].time = 1;

  //  Use group list to initialize vector clocks and determine coordinator
  int group_list_cursor = 0;
  properties.coordinator = properties.port;
  for (int i = 1; i < MAX_NODES; i++) {
    // Skip ourselves, since we're [0]
    if (properties.group_list.list[group_list_cursor].port == properties.port) {
      group_list_cursor++;
    }

    // Initialize other nodes to time 0
    properties.vectorClock[i].nodeId = properties.group_list.list[group_list_cursor].port;
    properties.vectorClock[i].time = 0;

    // Set coordinator to highest nodeId
    if (properties.group_list.list[group_list_cursor].port > properties.coordinator) {
      properties.coordinator = properties.group_list.list[group_list_cursor].port;
    }
    group_list_cursor++;
  }

  properties.curElectionId = properties.port * 100000; // Attempt at unique electionIds per node
  properties.last_AYA = time(NULL);
  properties.last_IAA = time(NULL);


  // If you want to produce a repeatable sequence of "random" numbers
  // replace the call to  time() with an integer.
  srandom(time(0));
  

  int i;
  for (i = 0; i < 10; i++) {
    int rn;
    rn = random(); 
    
    // scale to number between 0 and the 2*AYA time so that 
    // the average value for the timeout is AYA time.
    
    int sc = rn % (2*properties.AYATime);
    printf("Random number %d is: %d\n", i, sc);
  }
  
  
  // This is some sample code to setup a UDP socket for sending and receiving.
  struct sockaddr_in servAddr;

  // Create the socket
  // The following must be one of the parameters don't leave this as it is
  
  if ( (properties.sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
    perror("socket creation failed"); 
    exit(EXIT_FAILURE); 
  }

  // Set socket as non-blocking
  int flags = fcntl(properties.sockfd, F_GETFL);
  flags |= O_NONBLOCK;
  fcntl(properties.sockfd, F_SETFL, flags);

  // Setup my server information 
  memset(&servAddr, 0, sizeof(servAddr)); 
  servAddr.sin_family = AF_INET; 
  servAddr.sin_port = htons(properties.port);
  // Accept on any of the machine's IP addresses.
  servAddr.sin_addr.s_addr = INADDR_ANY;
  
  // Bind the socket to the requested addresses and port 
  if ( bind(properties.sockfd, (const struct sockaddr *)&servAddr,
            sizeof(servAddr)) < 0 )  { 
    perror("bind failed"); 
    exit(EXIT_FAILURE); 
  }

  // At this point the socket is setup and can be used for both
  // sending and receiving
  
  // Now pretend we are A "client, but sent the message to ourselves
  char *msg = "A message to myself!";

  // This would normally be a real hostname or IP address
  // as opposed to localhost
  char *hostname = "localhost";
  struct addrinfo hints, *serverAddr;
  serverAddr = NULL;

  memset(&hints, 0, sizeof(hints));
  hints.ai_socktype = SOCK_DGRAM;
  hints.ai_family = AF_INET;
  hints.ai_protocol = IPPROTO_UDP;

  if (getaddrinfo(hostname, argv[1], &hints, &serverAddr)) {
    printf("Couldn't lookup hostname\n");
    return -1;
  }
  

  // Send the message to ourselves
  int bytesSent;
  bytesSent = sendto(properties.sockfd, (const char *) msg, strlen(msg), MSG_CONFIRM,
		     serverAddr->ai_addr, serverAddr->ai_addrlen);
  if (bytesSent != strlen(msg)) {
    perror("UDP send failed: ");
    return -1;
  }



  struct sockaddr_in client;
    int len;
  char  buff[100];

  memset(&client, 0, sizeof(client));
  //  client.sin_family = AF_INET;
  // client.sin_port = htons(properties.port);
  //client.sin_addr.s_addr = INADDR_ANY;
  
  int n; 
  n = recvfrom(properties.sockfd, buff, 100, MSG_WAITALL,
	       (struct sockaddr *) &client, &len);

  // client will point to the address info of the node
  // that sent this message. The information can be used
  // to send back a response, if needed
  if (n < 0) {
    perror("Receiving error");
    return -3;
  }
  
  buff[n] = (char) 0;
  printf("from %X:%d Size = %d - %s\n",
	 ntohl(client.sin_addr.s_addr), ntohs(client.sin_port),
	 n, buff);

  // Main state loop
  while (properties.state != STOPPED) {
    state_main(&properties);
  }

  // NOTE to avoid memory leaks you must free the struct returned
  // by getaddrinfo. You will probably want to retrieve the information
  // just once and then associate it with the IP address, port pair.
  // Freeing is done with a call to get freeaddrinfo();

  freeaddrinfo(serverAddr);

  return 0;
  
}
