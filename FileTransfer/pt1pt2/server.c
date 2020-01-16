/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   main.c
 * Author: Helen Jiang
 *
 * Created on September 29, 2019, 1:06 AM
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdbool.h>
#include "packet.h"

#define ARGUMENT_COUNT 3
#define PORT_POSITION 2

/*
 * 
 */
int main(int argc, char** argv) {
    
    //check user argument
    if ( argc != ARGUMENT_COUNT ) {
        perror("Invalid argument format");
        exit(EXIT_FAILURE);
    }
    
    
    //Create socket
    int socketfd;
    
    if ( (socketfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0 ) {
        perror("Socket creation was unsuccessful");
        exit(EXIT_FAILURE);
    }
    
    
    //setting server information
    struct sockaddr_in server_address;
    
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = htonl(INADDR_ANY);
    server_address.sin_port = htons( atoi(argv[PORT_POSITION]) ); //port number indicated as second argument in command line
    memset( &server_address.sin_zero, 0, sizeof(server_address.sin_zero) ); //set to 0 by programmer
    
    
    //bind socket on server to an address
    if ( bind( socketfd, (const struct sockaddr *) &server_address, sizeof(server_address) ) < 0 ) {
        perror("Unable to bind socket to address");
        exit(EXIT_FAILURE);
    }
    
    
    //listen for messages
    int msg_len;
    char buffer[MAX_PACKET_SIZE];
    struct sockaddr_in client_address;
    socklen_t client_len = sizeof(client_address); //must initialize or else client address won't store
    
    if ( (msg_len = recvfrom( socketfd, (char*) buffer, MAX_PACKET_SIZE, MSG_WAITALL, (struct sockaddr*) &client_address, &client_len) ) < 0 ){
        perror("Client messages cannot be received");
        exit(EXIT_FAILURE);
    }
    
    buffer[msg_len] = '\0'; //terminate the message
    
    printf("client says : %s\n", &buffer);
    
    
    //sending response to client
    if ( strcmp("ftp", buffer) != 0 ) {
        if ( sendto( socketfd, "no", strlen("no"), MSG_CONFIRM, (struct sockaddr *) &client_address, client_len ) != -1 ) {
            printf("Reply to client : no\n");
            return (EXIT_FAILURE);
        }
        
    } else if ( strcmp("ftp", buffer) == 0 ) {
        if ( sendto(socketfd, "yes", strlen("yes"), MSG_CONFIRM, (struct sockaddr *) &client_address, client_len ) != -1 ) {
            printf("Reply to client : yes\n");
        }
    }
    
    
    
    //File transfer can start
    FILE* fp;
    
    while ( true ) {
        
        //listen for client data
        recvfrom( socketfd, (char*) buffer, MAX_PACKET_SIZE, MSG_WAITALL, (struct sockaddr*) &client_address, &client_len );
        
        //create Packet using information given by client
        struct Packet* p = New_Packet( buffer );
        
        //send back ack to client
        sendto( socketfd, "ack" , strlen( "ack" ), MSG_CONFIRM, (struct sockaddr *) &client_address, client_len );
        
        //first file... initiate the stream writer
        if ( p->frag_no == 1 ) {
            fp = fopen( p->filename, "w" ); //write to file
        }
        
        fwrite( p->filedata, sizeof(char), p->size, fp );
        
        //last file... must exit
        if ( p->frag_no == p->total_frag ) {
            fclose (fp);
            break;
        }
    }
    
    
    printf("File successfully received\n");
    close(socketfd);
    return(EXIT_SUCCESS);
}


