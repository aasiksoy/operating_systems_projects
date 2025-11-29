/**
 * \author {AUTHOR}
 */

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include "config.h"
#include "lib/tcpsock.h"
#include <time.h>
#include <pthread.h>


pthread_mutex_t conn_mutex;
int conn_counter = 0;
typedef struct {
    tcpsock_t *client;
} thread_args_t;
/**
 * Implements a sequential test server (only one connection at the same time)
 */

void* client_thread(void *arg){

	thread_args_t *args = (thread_args_t *) arg;//cast arg to thread_args_t*
	tcpsock_t *client = args -> client;//extract client socket

	sensor_data_t  data;
	int bytes;
	int result;

	do{
		bytes=sizeof(data.id);
		result = tcp_receive(client, (void*)&data.id, &bytes);
		if (result != TCP_NO_ERROR) break;
		bytes = sizeof(data.value);
		result = tcp_receive(client,(void*)&data.value,&bytes);
		if (result != TCP_NO_ERROR) break;
		bytes = sizeof(data.ts);
		result = tcp_receive(client,(void*)&data.ts,&bytes);
		if (result != TCP_NO_ERROR) break;

	    printf("sensor id = %" PRIu16 " - temperature = %g - timestamp = %ld\n", data.id, data.value, (long int)data.ts);

	}while(result  == TCP_NO_ERROR);

	if(result == TCP_CONNECTION_CLOSED){
		printf("peer has closed connection\n");
	}
	else{
		printf("error occurred on connection to peer\n");
	}
	pthread_mutex_lock(&conn_mutex);
	conn_counter++;
	printf("%d clients have  been served\n", conn_counter);
	pthread_mutex_unlock(&conn_mutex);

	tcp_close(&client);
	free(args);
	return NULL;

    
}



int main(int argc, char *argv[]) {
    tcpsock_t *server, *client;
    sensor_data_t data;
    int bytes, result;

//The server first checks whether the user provided the required two arguments
//(port and max number of clients). Then it converts the arguments from text to
//integers using atoi and stores them in PORT and MAX_CONN. After printing a start message,
//the server calls tcp_passive_open, which creates a listening TCP socket on the given port.
//If this fails, the server immediately exits; otherwise, the program continues.

    if(argc < 3) {
    	printf("Please provide the right arguments: first the port, then the max nb of clients");
    	return -1;
    }
    
    int MAX_CONN = atoi(argv[2]);
    int PORT = atoi(argv[1]);

    printf("Test server is started\n");
    if (tcp_passive_open(&server, PORT) != TCP_NO_ERROR){
        printf("error opening the passive socket.\n");
        exit(EXIT_FAILURE);
    }

	pthread_mutex_init(&conn_mutex, NULL);


	while (conn_counter < MAX_CONN){

	printf("waiting for connection\n");
        if (tcp_wait_for_connection(server, &client) != TCP_NO_ERROR){
			printf("error while waiting for a connection\n");
			continue;
		}
        printf("Incoming client connection\n");
        //allocate memory for the thread and fill threat argument struct for this client
        thread_args_t *args = malloc(sizeof(thread_args_t));
        
        if(args ==NULL){
            printf("error while calling malloc for thread arguments\n");
            continue;
        }
        args -> client = client;
        
        pthread_t tid;
        
        if(pthread_create(&tid,NULL,client_thread,args) != 0){
            printf("error: could not create thread\n");
            tcp_close(&client);
            free(args);
            continue;
        }
        pthread_detach(tid);//detaching the thread


    } 
    pthread_mutex_destroy(&conn_mutex);
    if (tcp_close(&server) != TCP_NO_ERROR){
      printf("error closing the server socket\n");
      exit(EXIT_FAILURE);
    }
    
    printf("Test server is shutting down\n");
    return 0;
}




