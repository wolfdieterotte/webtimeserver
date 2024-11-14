#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <syslog.h>
#include <signal.h>
#include <time.h>

#define DBG
#include "../debug/dbg.h"

#include "../threadpool/threadpool.h"
#include "../properties/properties.h"

#define BUFFER_SIZE 1024    // size of the lookup repository

/* Function prototypes */

// function talking to a client
void handle_client(void* arg);

// get time string
void get_time(char* buffer);

/* Preprocessor directives */
#define NUM_CONNECTIONS 100       // number of pending connections in the connection queue

