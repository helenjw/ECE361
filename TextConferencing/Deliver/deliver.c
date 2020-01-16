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
#include <stdbool.h>
#include <pthread.h>
#include "packet.h"

//returns a socketfd to communicate with the server
int Connect_to_Server( char server_ip[], char server_port[] );

//Listen to incoming messages from server
void *Listen_to_Server( void* socket );

//Returns socketfd of connection with server, ERROR if unsuccessful
int Validate_Login( char input[], char username[] );

//Send QUIT request to server, and close socket
void LOGOUT_Request( int socketfd, char username[] );

//Send LIST request to server to get active users and active sessions
void LIST_Request( int socketfd, char username[] );

//Send LIST request to server to get members in a session
void MEMBER_Request( char input[], int socketfd, char username[] );

//Send NEW_SESS request to server, and display results
void CREATE_Request( char input[], int socketfd, char username[] );

//send LEAVE_SESS request to server
//Server side ignores request if session does not exist
void LEAVE_Request( char input[], int socketfd, char username[] );

//send JOIN request to server
void JOIN_Request( char input[], int socketfd, char username[] );

//list out operations and arguments for program
void HELP_Request();

//Send messages to specified chatroom
//User must specify in the form of "/message <session id> <message to session>"
void MESSAGE_Request( char input[], int socketfd, char username[] );

//Add user to a chatroom
void INVITE_Request( char input[], int socketfd, char username[] );


/*
 * 
 */
int main(int argc, char** argv) {

    int socketfd;
    char input[MAXLINE], username[MAXNAME], command[MAXLINE];
    
    while (true){
        //only proceed once a successful login has occured
        while (true) {
            //storing user input and user command
            fgets( input, MAXLINE, stdin );
            sscanf( input, "%s", &command );

            if ( strcmp(command, "/quit") == 0 ) {
                return (EXIT_SUCCESS);

            } else if ( strcmp(command, "/help") == 0 ) {
                HELP_Request();

            } else if ( strcmp(command, "/login") == 0 ) {
                if ( (socketfd = Validate_Login(input, username)) != ERROR ) {
                    break;
                }

            } else {
                printf("Please log in first\n");
            }
        }

        //Authentication success
        printf("     Logged in as: %s\n\n", username);


        //Create new thread to listen to incoming data from server
        pthread_t recvThread;
        int* socketPtr = &socketfd;
        pthread_create( &recvThread, NULL, Listen_to_Server, socketPtr );


        //Service client
        while (true) {

            //Get client input
            //char command[MAXLINE];
            fgets( input, MAXLINE, stdin );
            sscanf( input, "%s", &command );

            //Process client commands
            if ( strcmp(command, "/quit") == 0 ){
                return (EXIT_SUCCESS);
                
            } else if ( strcmp(command, "/logout") == 0 ) {
                LOGOUT_Request(socketfd, username);
                break;
                
            } else if ( strcmp(command, "/login") == 0 ) {
                printf("Already logged in as %s\n", username);
                
            } else if ( strcmp(command, "/list") == 0 ) {
                LIST_Request(socketfd, username);
                
            } else if ( strcmp(command, "/createsession") == 0 ) {
                CREATE_Request(input, socketfd, username);
                
            } else if ( strcmp(command, "/leavesession") == 0 ) {
                LEAVE_Request(input, socketfd, username);
                
            } else if ( strcmp(command, "/joinsession") == 0 ) {
                JOIN_Request( input, socketfd, username );
                
            } else if ( strcmp(command, "/help") == 0 ) {
                HELP_Request();
                
            } else if ( strcmp(command, "/message") == 0 ) {
                MESSAGE_Request( input, socketfd, username );
                
            } else if ( strcmp(command, "/members") == 0 ) {
                MEMBER_Request( input, socketfd, username );
                
            } else if ( strcmp(command, "/invite") == 0 ) {
                INVITE_Request( input, socketfd, username );
                
            } else {
                printf("     Unrecognized request\n");
            }
        }
    }

    return (EXIT_SUCCESS);
}

//Returns socketfd of connection with server, ERROR if unsuccessful
int Validate_Login( char input[], char username[] ) {
    
    //storing parameters needed for login
    char command [MAXLINE], username_help[MAXNAME], password[MAXNAME], server_ip[MAXLINE], server_port[MAXLINE];
    sscanf( input, "%s %s %s %s %s", &command, &username_help, &password, &server_ip, &server_port );
    strcpy( username, username_help );

    //Make connection to server
    int socketfd;
    if ( ( socketfd = Connect_to_Server(server_ip, server_port) ) == ERROR ) {
        return ERROR;
    }
    
    
    //Send login credentials to server
    //format data as: <password> to client to get login verified
    char message[MAXLINE];
    Format_Message( New_Message(LOGIN, strlen(password), username, password), message );
    
    if ( ( write(socketfd, message, sizeof(message)) ) == ERROR ) {
        printf("     Error: Unable to send message to server\n");
        return ERROR;
    }

    
    //listen for LO_ACK or LO_NAK
    char message_buff[MAXLINE];
    recv( socketfd, (char*)message_buff, sizeof(message_buff), 0 );
    
    struct Message* m = Decode_Message( message_buff );
    
    //On Unsuccessful login, return ERROR
    if ( m->type == LO_NAK ) {
        printf( "     Error: %s\n", m->data );
        return ERROR;
    }
    
    free(m);
    return socketfd;
}


//returns a socketfd to communicate with the server
int Connect_to_Server( char server_ip[], char server_port[] ) {
    
    //setting up socket information
    struct addrinfo hints, *res;
    
    memset( &hints, 0, sizeof(hints) );
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    
    getaddrinfo( server_ip, server_port, &hints, &res );
    
    
    //make socket
    int socketfd;
    if ( (socketfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) < 0 ) {
        printf("     Error: Unable to make socket\n");
        return ERROR;
    }
    
    
    //connect
    if ( (connect( socketfd, res->ai_addr, res->ai_addrlen )) < 0 ) {
        printf("     Error: Unable to connect socket\n");
        return ERROR;
    }
    
    return socketfd;
}


//Send QUIT request to server, and close socket
void LOGOUT_Request( int socketfd, char username[] ) {
    
    //send quit request
    char message[MAXLINE];
    Format_Message( New_Message(EXIT, strlen(""), username, ""), message );
    write(socketfd, message, sizeof(message));
    
    //close socket
    close( socketfd );
    
    printf("     %s has logged out\n", username);
}


//Send LIST request to server, and display results
void LIST_Request( int socketfd, char username[] ) {
    
    //send LIST request
    char message[MAXLINE];
    Format_Message( New_Message(QUERY, strlen(""), username, ""), message );
    write(socketfd, message, sizeof(message));
    
}


//Send NEW_SESS request to server, and display results
void CREATE_Request( char input[], int socketfd, char username[] ) {
    
    //storing parameters needed to create session
    char command [MAXLINE], session_name[MAXNAME];
    sscanf( input, "%s %s", &command, &session_name );
    
    //send CREATE request
    char message[MAXLINE];
    Format_Message( New_Message(NEW_SESS, strlen(session_name), username, session_name), message );
    write(socketfd, message, sizeof(message));

}


//send LEAVE_SESS request to server
//Server side ignores request if session does not exist
void LEAVE_Request( char input[], int socketfd, char username[] ) {
    
    //storing parameters needed to create session
    char command [MAXLINE], session_name[MAXNAME];
    sscanf( input, "%s %s", &command, &session_name );
    
    //send LEAVE request
    char message[MAXLINE];
    Format_Message( New_Message(LEAVE_SESS, strlen(session_name), username, session_name), message );
    write(socketfd, message, sizeof(message));
    
    printf("     You have left session %s\n", session_name);
}


//send JOIN request to server
void JOIN_Request( char input[], int socketfd, char username[] ) {
    //storing parameters needed to create session
    char command [MAXLINE], session_name[MAXNAME];
    sscanf( input, "%s %s", &command, &session_name );
    
    //send JOIN request
    char message[MAXLINE];
    Format_Message( New_Message(JOIN, strlen(session_name), username, session_name), message );
    write(socketfd, message, sizeof(message));

}

//Listen to incoming messages from server
void *Listen_to_Server( void* socket ) {
    
    //setup
    int* socket_p = (int *)socket;
    char message_buff[MAXLINE];
    
    while (true) {
        
        //Clear message buffer before writing data
        memset( message_buff, 0, sizeof(message_buff) );
        
        //Listen to message
        int status = recv( *socket_p, (char*)message_buff, sizeof(message_buff), 0 );
        
        if ( status == ERROR || status == HANGUP ) {
            return NULL;
        }
        
        //Decode message by type
        struct Message* m = Decode_Message( message_buff );
        
        if ( m->type == MESSAGE ) {
            
            //store chat name and message
            char chatname[MAXNAME], dialogue[MAXLINE];
            sscanf( m->data, "%s", chatname );
            
            // Display message like this:
            // -----<chatname>-----
            // <source> : <dialogue>
            printf("\n");
            printf("     --------%s--------\n", chatname);
            printf("     %s: %s\n", m->source, &m->data[ strlen(chatname) + 1]);
            
        } else {
            printf("     %s\n", m->data);
        }

        free( m );
    }
}

//list out operations and arguments for program
void HELP_Request() {
    //login
    printf("    /login <username> <password> <server IP> <server port>\n");
    
    //logout
    printf("    /logout\n");
    
    //createsession
    printf("    /createsession <session name>\n");
    printf("         Create a new chatroom that you will be a part of\n");
    
    //Joinsession
    printf("    /joinsession <session name>\n");
    printf("         Join an already-existing chatroom and see its messages\n");
    
    //Leavesession
    printf("    /leavesession <session name>\n");
    printf("         Leave a chatroom. You will no longer be able to see its messages\n");
    
    //list
    printf("    /list\n");
    printf("         Get a list of active users and chatrooms\n");

    //listmember
    printf("    /members <session name>\n");
    printf("         Get a list of the members of <session name>\n");
    
    //message
    printf("    /message <session name> <message>\n");
    printf("         Send a message to members of the chatroom with <session name>\n");
    
    //invite
    printf("    /invite <session name> <user name>\n");
    printf("         Add user with <username> to chatroom with <session name>\n");
    
    //quit
    printf("    /quit\n");
    printf("         Exit the program\n");
}

//Send messages to specified chatroom
//User must specify in the form of "/message <session id> <message to session>"
void MESSAGE_Request( char input[], int socketfd, char username[] ) {
    
    //grabbing everything after command as data to send to server
    char data[MAXLINE];
    strcpy( data, &input[Index_of_Kth(input, ' ', 1)] );
    
    //send MESSAGE request
    char message[MAXLINE];
    Format_Message( New_Message(MESSAGE, strlen(data), username, data), message );
    write(socketfd, message, sizeof(message));
}

//Send LIST request to server to get members in a session
void MEMBER_Request( char input[], int socketfd, char username[] ) {
    
    //storing parameters needed to make member request
    char command [MAXLINE], session_name[MAXNAME];
    sscanf( input, "%s %s", &command, &session_name );
    
    //send QUERY request
    char message[MAXLINE];
    Format_Message( New_Message(QUERY, strlen(session_name), username, session_name), message );
    write(socketfd, message, sizeof(message));
}

//Add user to a chatroom
//User will give args in the form <session name> <user name>
void INVITE_Request( char input[], int socketfd, char username[] ) {
    
    //grabbing everything after command and send to server
    char data[MAXLINE];
    strcpy( data, &input[Index_of_Kth(input, ' ', 1)] );
    
    //send INVITE request
    char message[MAXLINE];
    Format_Message( New_Message(INVITE, strlen(data), username, data), message );
    write(socketfd, message, sizeof(message));
}
