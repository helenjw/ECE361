#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdbool.h>

#include "db.h"
#include "packet.h"

#define PORT_POSITION 1  // port we're listening on
#define BACKLOG 10

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa);

// Set up and return socketfd for welcome port
int Welcome_Port_Setup( char welcome_port[] );


int main(int argc, char** argv) {
    
    //program setup
    Populate_UserDb();
    
    fd_set master;    // master file descriptor list
    fd_set read_fds;  // temp file descriptor list for select()
    int fdmax;        // maximum file descriptor number
    
    FD_ZERO(&master);    // clear the master and temp sets
    FD_ZERO(&read_fds);
    
    
    // set up welcome port to listen to for incoming connections
    int listener;
    if ( (listener = Welcome_Port_Setup(argv[PORT_POSITION])) == ERROR ) {
        printf("Unable to set up welcome socket\n");
        return (EXIT_FAILURE);
    }

    
    // listen for connections
    if ( listen(listener, BACKLOG) == ERROR ) {
        printf("Unable to listen for connections\n");
        return (EXIT_FAILURE);
    }

    //update fdmax and add listener socket to fd_list
    FD_SET(listener, &master);
    fdmax = listener;
    
   
    //To be used in main loop to gather client messages
    char message_buff[MAXLINE];
    
    // main loop
    while (true) {
        
        read_fds = master;
        
        //Handle error from select()
        if ( select(fdmax+1, &read_fds, NULL, NULL, NULL) == ERROR ) {
            printf( "Socket selection error\n" );
            return(EXIT_FAILURE);
        }

        
        // run through the existing connections looking for data to read
        for( int i = 0; i <= fdmax; i++) {
            
            //There exists new data to read
            if ( FD_ISSET(i, &read_fds) ) {
                
                //Handle new connections. Users must successfully login
                if (i == listener) {

                    struct sockaddr_storage client_address; // client address
                    socklen_t addrlen = sizeof (client_address);
                    int newfd = accept(listener, (struct sockaddr *)&client_address, &addrlen);
                    
                    if ( newfd != ERROR ) {
                                                    
                        char remoteIP[INET6_ADDRSTRLEN];
                        printf("selectserver: new connection from %s on socket %d\n", inet_ntop(client_address.ss_family, get_in_addr((struct sockaddr*)&client_address), remoteIP, INET6_ADDRSTRLEN), newfd);
                        FD_SET(newfd, &master); //add to master
                        
                        //listen for login information from client
                        recv( newfd, (char*)message_buff, sizeof(message_buff), 0 );
                        struct Message* m = Decode_Message( message_buff );
                        
                        //If login is successful
                        if ( Process_Login(m, newfd) ) {
                            FD_SET(newfd, &master); //add to master list
                            
                            if ( newfd > fdmax ) {
                                fdmax = newfd;
                            }
                        }
                        
                        free(m);
                    }
                    

                //Handle available data from known connections
                } else {
                    
                    int n; //size of client message
                    
                    // Connection with client unexpectedly closed/broken
                    if ( (n = recv(i, message_buff, sizeof(message_buff), 0)) <= 0 ) {
                        
                        Process_Unexpected_Exit( i );
                        close( i );
                        FD_CLR( i, &master ); // remove from master set 
                        
                    //User requests
                    } else {

                        struct Message* m = Decode_Message( message_buff );
                        
                        if ( m->type == EXIT ) {
                            Process_Exit( m );
                            close( i );
                            FD_CLR( i, &master );
                        }
                        
                        if ( m->type == QUERY ) {
                            Process_Query( m );
                        }
                        
                        if ( m->type == NEW_SESS ) {
                            Process_NewSess( m );
                        }
                        
                        if ( m->type == LEAVE_SESS ) {
                            Process_LeaveSess( m );
                        }
                        
                        if ( m->type == JOIN ) {
                            Process_JoinSess( m );
                        }
                        
                        if ( m->type == MESSAGE ) {
                            Process_Message( m );
                        }
                        
                        if ( m->type == INVITE ) {
                            Process_Invite( m );
                        }
                        
                        free(m);
                    }
                    
                } // END handle data from client
            } // END got new incoming connection
        } // END looping through file descriptors
    } // END of while(true) loop
    
    return 0;
}


// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}


// Set up and return socketfd for welcome port
int Welcome_Port_Setup( char welcome_port[] ) {
    
    // get socket
    struct addrinfo hints;
    
    memset( &hints, 0, sizeof(hints) );
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;    
        
    struct addrinfo *ai;
    int rv;
    if ( (rv = getaddrinfo(NULL, welcome_port, &hints, &ai)) != 0 ) {
        return ERROR;
    }
	
    
    // Bind socket
    struct addrinfo *p;
    int yes = 1;
    int listener;  // listening socket descriptor
    
    for(p = ai; p != NULL; p = p->ai_next) {
        
        listener = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        
        if (listener < 0) { 
            continue;
        }

        // lose the pesky "address already in use" error message
        setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

        if ( bind(listener, p->ai_addr, p->ai_addrlen) < 0 ) {
            close(listener);
            continue;
        }
        
        break;
    }

    
    // if p is NULL, then socket wasn't bound
    if (p == NULL) {
        return ERROR;
    }

    freeaddrinfo(ai); // all done with this
    return listener;
}

