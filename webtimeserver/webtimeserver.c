//
//  server.c
//
//  Created by Wolf-Dieter Otte on 8/19/24.
//

#include "webtimeserver.h"

// mutexes for protecting critical sections
pthread_mutex_t mutex_cache = PTHREAD_MUTEX_INITIALIZER;


/* ******************************************************* */
/* main()                                                  */
/* ******************************************************* */
int main(int argc, char** argv)
{
    Properties* properties;

    int server_socket;                 // file descriptor of server socket
    int client_socket;                 // file descriptor of client socket
    struct sockaddr_in server_address; // for naming the server's listening socket
    int yes = 1;

    threadpool pool;                   // threadpool

    
    // ----------------------------------------------------------
    // read properties
    // ----------------------------------------------------------
    char* properties_file = "default.properties"; // default file name
    
    if(argc == 2)
    {
        // if properties file provided at startup, use that
        properties_file = argv[1];
    }
    
    properties = property_read_properties(properties_file);
 
    
    // ----------------------------------------------------------
    // create the threadpool
    // ----------------------------------------------------------
    pool = threadpool_create();

    
    // ----------------------------------------------------------
    // ignore SIGPIPE, sent when client disconnected
    // ----------------------------------------------------------
    signal(SIGPIPE, SIG_IGN);
 
    
    // ----------------------------------------------------------
    // create unnamed network socket for server to listen on
    // ----------------------------------------------------------
    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("[main] failed to create socket");
        exit(EXIT_FAILURE);
    }
 
    
    // ----------------------------------------------------------
    // bind the socket
    // ----------------------------------------------------------
    // lose the pesky "Address already in use" error message
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1) 
    {
	perror("setsockopt");
	exit(EXIT_FAILURE);
    }

    server_address.sin_family      = AF_INET;           // accept IP addresses
    server_address.sin_addr.s_addr = htonl(INADDR_ANY); // accept clients on any interface
    server_address.sin_port        = htons(atoi(property_get_property(properties, "SERVER_PORT"))); // port to listen on
    if (bind(server_socket, (struct sockaddr *)&server_address, sizeof(server_address)) != 0)     // binding unnamed socket to a particular port
    {
        perror("[main] failed to bind port");
        exit(EXIT_FAILURE);
    }
   
    
    // ----------------------------------------------------------
    // listen on the socket
    // ----------------------------------------------------------
    if (listen(server_socket, NUM_CONNECTIONS) != 0)
    {
        perror("[main] failed to listen");
        exit(EXIT_FAILURE);
    }
    

    // ----------------------------------------------------------
    // server loop
    // ----------------------------------------------------------
    debug("Server with PID %d initialized", getpid());
    
    while (true)
    {
        // accept connection to client
        client_socket = accept(server_socket, NULL, NULL);
        debug("[main] client accepted");

        // add task handle_client with client socket as argument
        threadpool_add_task(pool, task_copy_arguments, handle_client, (void*)&client_socket);
    }
    
    // we never reach here...
    //exit(EXIT_SUCCESS); // everything went ok
}


/* ******************************************************* */
/* handle_request()                                        */
/* ******************************************************* */
void handle_client(void* arg)
{
    int client_socket = *((int*)arg);   // the socket connected to the client
    char date_string[80];
    char client_request[BUFFER_SIZE];
    char http_response[256];
    long bytes_read;
    
    const char *http_string =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html\r\n"
        "Connection: close\r\n"
        "\r\n"
        "<html><body><h1>The current time is:</h1>"
        "%s"
        "</body></html>";
    
    debug("[handle_client] client socket: %d", client_socket);
    
    // read client request and print it out (optional)
    bytes_read = read(client_socket, client_request, (long)BUFFER_SIZE-1);
    if (bytes_read < 0)
    {
        perror("[handle_client] failed to read from socket");
        close(client_socket);
        return;
    }
    client_request[bytes_read] = '\0'; // Null-terminate the buffer
    // debug("Request:\n%s\n", client_request);

    // get time information and send to client
    get_time(date_string);
    sprintf(http_response, http_string, date_string);
    write(client_socket, http_response, strlen(http_response));

    // cleanup
    if (close(client_socket) == -1)
    {
        //perror("Error closing socket");
        fprintf(stderr, "[handle_request] thread ID %p error closing socket, bad file descriptor: %d", (void*)pthread_self(), client_socket);
        //exit(EXIT_FAILURE);
    } else {
        debug("[handle_request] thread ID %p socket closed: %d", (void*)pthread_self(), client_socket);
    }    
}


/* ******************************************************* */
/* prepare arguments for thread function                   */
/* ******************************************************* */
void *task_copy_arguments(void *args_in)
{
    void *args_out;
    
    args_out = malloc(sizeof(int));
    *((int*)args_out) = *((int*)args_in);
    
    return args_out;
}


/* ******************************************************* */
/* get a string of the current time                        */
/* ******************************************************* */
void get_time(char* buffer)
{
    
    time_t now;
    struct tm *timeinfo;

    time(&now); // Get the current time
    //timeinfo = localtime(&now); // Convert to local time
    timeinfo = localtime(&now); // Convert to local time

    strftime(buffer, 80, "%Y-%m-%d %H:%M:%S", timeinfo); // Format the time
}
