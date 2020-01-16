#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <ctype.h>
#include <time.h>
#include <stdbool.h>
#include "packet.h"

#define BILLION 1000000000.0
#define ARGUMENT_COUNT 4
#define PORT_POSITION 3
#define ADDRESS_POSITION 2

/*
 * 
 */
int main(int argc, char** argv) {
    
    //check user input
    if ( argc != ARGUMENT_COUNT ) {
        perror("Invalid argument format\n");
        exit(EXIT_FAILURE);
    }
    
    
    //Create socket
    int socketfd;
    
    if ( (socketfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0 ) {
        perror("Socket creation was unsuccessful\n");
        exit(EXIT_FAILURE);
    }
    
    
    //setting server informations
    struct sockaddr_in server_address;
    
    memset( &server_address, 0, sizeof(server_address) );
    
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons( atoi(argv[PORT_POSITION]) ); //port number indicated in command line
    server_address.sin_addr.s_addr = inet_addr( argv[ADDRESS_POSITION] ); //server address from command line
    
    
    //Prompting user for message to server
    char client_message[MAX_PACKET_SIZE], file[MAX_PACKET_SIZE];
    
    printf( "Enter name of file to be transferred in the format 'ftp <filename>' :" );
    scanf( "%s %s", client_message, file );
    
    //Check for existence of file
    if ( access(file, F_OK) != 0 ) {
        printf("File %s cannot be found\n", &file);
        exit(EXIT_FAILURE);
    }

    //send message to server and start RTT clock
    struct timespec start;
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    sendto( socketfd, client_message, strlen(client_message), MSG_CONFIRM, (const struct sockaddr *) &server_address, sizeof(server_address) );
    
    
    //receive server message
    int msg_len, incoming_len;
    char buffer[MAX_PACKET_SIZE];
    struct sockaddr_in incoming_address;
    
    if ( (msg_len = recvfrom( socketfd, (char*) buffer, MAX_PACKET_SIZE, 0, (struct sockaddr*) &incoming_address, &incoming_len) ) < 0 ){
        perror("Client messages unable be received\n");
        exit(EXIT_FAILURE);
    }
    
    buffer[msg_len] = '\0'; //terminate the message
    
    
    //Stopping RTT clock.
    struct timespec end;
    clock_gettime(CLOCK_MONOTONIC, &end);
    
    
    //Let client know if file transfer can start
    if ( strcmp(buffer, "no") == 0 ) {
        return(EXIT_FAILURE);
    }
    
    float rtt = ( (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / BILLION ) ;
    printf("Round trip time (RTT) : %f seconds\n", rtt );
    printf("A file transfer can start\n");
    
    
    
    
    //start file transfer
    FILE *fp = fopen( file, "r" );
    int total_fragments = Get_Total_Frag( fp );
    
    for ( int i = 1; i <= total_fragments; i++ ) {
        
        //format file to deliver to server
        char packet_buffer[MAX_PACKET_SIZE];
        struct Packet *p = New_Packet( total_fragments, i, file, fp );
        
        Format_Packet( p, packet_buffer );
        sendto( socketfd, packet_buffer, sizeof(packet_buffer), MSG_CONFIRM, (const struct sockaddr *) &server_address, sizeof(server_address) );
        printf("Sending packet %d\n", i);
        
        //initiate ACK timer
        //struct timespec end;
        clock_gettime( CLOCK_MONOTONIC, &start );
        
        //listen for ack
        while( true ) {

            char ack[MAX_PACKET_SIZE];
            recvfrom( socketfd, (char*) ack, MAX_PACKET_SIZE, 0, (struct sockaddr*) &incoming_address, &incoming_len);
            
            //recording time waiting so far
            //sleep(1);
            clock_gettime( CLOCK_MONOTONIC, &end );
            
            //check for timeout            
            if ( ((end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / BILLION)>= 2*rtt ) {
                //resend current packet
                printf("Timeout! Resending packet %d\n", i);
                i--;
                break;
            }
            
            //once we are sure there is no timeout, check for ack
            if ( strcmp(ack, "ack") == 0 ) {
                printf( "Packet %d acknowledged\n", i );
                break;
            }
        }
    }
    
    
    printf("File successfully transferred\n");
    fclose(fp);
    close(socketfd);
    return (EXIT_SUCCESS);
}

