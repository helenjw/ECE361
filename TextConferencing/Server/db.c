#include "db.h"

//Using hard-coded data, populate UserDb
void Populate_UserDb() {
    
    for ( int i = 0; i < MAXUSERS; i++ ) {
        UserDb[i].active = 0; //default not active
        UserDb[i].socketfd = -1; //default no connected socket
        strcpy( UserDb[i].username, UsernameDb[i] );
        strcpy( UserDb[i].password, PasswordDb[i] );
    }
    
    //Print_UserDb();
}


//Process Login request and send back LO_ACK or LO_NAK
//Modify UserDb to reflect login
bool Process_Login( struct Message* m, int socketfd ) {
    
    //check if username exists
    for ( int i = 0; i < MAXUSERS; i++ ) {
        
        //username and password match existing in db
        if ( strcmp(UserDb[i].username, m->source) == 0  && strcmp(UserDb[i].password, m->data) == 0 ) {
            
            //mark user as active in UserDb. Store socket info
            UserDb[i].active = 1;
            UserDb[i].socketfd = socketfd;
            
            //Send LO_ACK
            char message[MAXLINE];
            Format_Message( New_Message(LO_ACK, strlen(""), "Server", ""), message );
            
            write( UserDb[i].socketfd, message, sizeof(message) );
            
            return true;
        }
    }
        
    //couldn't find username, or password was incorrect
    char message[MAXLINE];
    Format_Message( New_Message(LO_NAK, strlen("Incorrect username or password"), "Server", "Incorrect username or password"), message );
            
    write( socketfd, message, sizeof(message) );
    
    return false;
}


//De-list source as active, and close their socketfd
void Process_Exit( struct Message* m ) {
    
    //guaranteed id is positive, since user is logged in
    int id = Get_UserId( m->source );
    
    //User deactivated, and socketfd closed
    UserDb[id].active = 0;
    close( UserDb[id].socketfd );
    UserDb[id].socketfd = -1;
    
    printf( "%s has logged out\n", UserDb[id].username );
}


//Find user in UserDb by socketfd, delist user as active and close connection
void Process_Unexpected_Exit( int socketfd ) {
    
    for ( int i = 0; i < MAXUSERS; i++ ) {
        if ( UserDb[i].socketfd == socketfd ) {
            UserDb[i].active = 0;
            close( UserDb[i].socketfd );
            UserDb[i].socketfd = -1;
    
            printf( "%s has unexpectedly logged out\n", UserDb[i].username );
        }
    }
}


//Lists every active member and sends as a message
void Process_Query( struct Message* m ) {
    
    //If there is data, send members of a particular session
    if ( strlen(m->data) != 0 ) {
        Send_Session_Members( m );
    
    //If no data, send active users and sessions
    } else {
        Send_Active_Users( m );
        Send_Active_Sessions( m );
    }
}


//send active user list
void Send_Active_Users( struct Message* m ){
    
    //active users sent in data as: "<user 1> ... <user n>"
    char active_users[MAXLINE] = "Active Users: ";
    
    for ( int i = 0; i < MAXUSERS; i++ ) {
        if ( UserDb[i].active == 1 ) {
            //printf("%s\n", UserDb[i].username);
            strcat( active_users, UserDb[i].username );
            strcat( active_users, " " );
        }
    }
    
    //Send back active user list to client
    char message[MAXLINE];
    Format_Message( New_Message(QU_ACK, sizeof(active_users), "Server", active_users), message );
            
    write( UserDb[Get_UserId( m->source )].socketfd, message, sizeof(message) );
}


//send active sessions list
void Send_Active_Sessions( struct Message* m ){
    
    //active sessions sent back as: "<session1> ... <session n>"
    char active_sessions[MAXLINE] = "Active Sessions: ";
    
    for ( int i = 0; i < MAXSESSIONS; i++ ) {
        
        //end of list
        if ( strcmp(SessionDb[i].name, "") == 0 ) {
            break;     
        } 
        
        if ( strcmp(SessionDb[i].name, SESSION_DEL) != 0 ) {
            strcat( active_sessions, SessionDb[i].name );
            strcat( active_sessions, " " );
        }
    }
    
    //Send back active user list to client
    char message[MAXLINE];
    Format_Message( New_Message(QU_ACK, sizeof(active_sessions), "Server", active_sessions), message );
            
    write( UserDb[Get_UserId( m->source )].socketfd, message, sizeof(message) );
}


//send members of a specific session
void Send_Session_Members( struct Message* m ) {
    
    char members[MAXLINE];
    
    int session_id = Find_Session( m->data );
    
    //If session is found
    if ( session_id != ERROR ) {
        
        //Write data as: "<session name> : <member> <member>"
        sprintf( members, "%s: ", m->data );
        
        //loop through members list and send members
        for ( int i = 0; i < MAXUSERS; i++ ) {
            if ( SessionDb[session_id].members[i] != MEMBER_DEL ) {
                strcat( members, UserDb[ SessionDb[session_id].members[i] ].username );
                strcat( members, " " );
            }
        }
        
    } else {
        //write data as: "Couldn't find session <session name>"
        sprintf( members, "Couldn't find session %s", m->data );
    }
    
    //Send back list
    char message[MAXLINE];
    Format_Message( New_Message(QU_ACK, sizeof(members), "Server", members), message );
            
    write( UserDb[Get_UserId( m->source )].socketfd, message, sizeof(message) );
}


//Insert newly created session in SessionDb
//Session creator is automatically joined to session
void Process_NewSess( struct Message* m ) {
    
    char data[MAXLINE];
    
    //Session already exists
    if ( Find_Session(m->data) != ERROR ) {
        sprintf( data, "Session %s already created", m->data );
    
    //create new session
    } else {
        
        //Find place to insert new session in SessionDb. First session with name "" or DEL
        for ( int i = 0; i < MAXSESSIONS; i++ ) {

            if ( strcmp( SessionDb[i].name, "" ) == 0 || strcmp( SessionDb[i].name, SESSION_DEL ) == 0 ) {
                
                //set name of chat, num_members = 1 (for creator), and add creator to session
                strcpy( SessionDb[i].name, m->data );
                SessionDb[i].num_members = 1;

                //Initialize members array to {-1, -1, -1, ... }
                //Since some UserIds can be 0, and some remaining values may be present from previous session
                for ( int j = 0; j < MAXUSERS; j++ ) {
                    SessionDb[i].members[j] = MEMBER_DEL;
                }
                
                //Find ID of user (index of User in User Db), and insert into session members
                SessionDb[i].members[ Get_Available_Member_Id(SessionDb[i]) ] = Get_UserId( m->source );
                
                sprintf( data, "Session %s created", SessionDb[i].name );
                
                break;
            }
        }
    }
    
    
    //Send back NS_ACK to client
    char message[MAXLINE];
    Format_Message( New_Message(NS_ACK, sizeof(data), "Server", data), message );
    write( UserDb[Get_UserId( m->source )].socketfd, message, sizeof(message) );
    
//    Print_SessionDb();
}


//Delete requesting user from conference session
//Decrement num_members, and delete session if num_members = 0
void Process_LeaveSess( struct Message* m ) {
    
    int Id;
    //Do something if such a session exists
    if ( (Id = Find_Session(m->data)) != ERROR ) {

        int memberId = Get_MemberId( m->source, Id );
        
        //Member does not exist in chat
        if ( memberId == ERROR ) {
            return;
        }
        
        SessionDb[Id].num_members --;
        SessionDb[Id].members[memberId] = MEMBER_DEL; //delete member from list
        
        
        //check if there are still members in the room. If not, delete the whole chat
        if ( SessionDb[Id].num_members == 0 ) {
            strcpy( SessionDb[Id].name, SESSION_DEL );
        }
    }
    
//    Print_SessionDb();
}


//Add requesting user to conference session and Increment num_members if successful
//Send back JN_ACK and JN_NAK
void Process_JoinSess( struct Message* m ) {
    
    //JN_ACK or JN_NAK 
    char data[MAXLINE], message[MAXLINE];
    
    int Id;
    //Session found
    if ( (Id = Find_Session(m->data)) != ERROR ) {
        
        //Check if user is already a member of the chat
        //If already a part of the chat, send back JN_NAK
        if ( Get_MemberId( m->source, Id) != ERROR ) {
            
            sprintf( data, "Already in session %s", m->data );
            Format_Message( New_Message(JN_NAK, sizeof(data), "Server", data), message );
            
            write( UserDb[Get_UserId( m->source )].socketfd, message, sizeof(message) );
            return;
        }
        
        
        //Proceed to add member to conference space permitting
        int avail_id = Get_Available_Member_Id(SessionDb[Id]);
        SessionDb[Id].num_members ++;
        
        //Space exists for member to join
        if ( avail_id != ERROR ) {
            SessionDb[Id].members[avail_id] = Get_UserId( m->source ); //add user to member list
            
            //Send JN_ACK
            sprintf( data, "Joined session %s", m->data );
            Format_Message( New_Message(JN_ACK, sizeof(data), "Server", data), message );
            
        } else {
            //No more room in session. Send JN_NAK to client
            strcpy( data, "Error: Session is already at maximum occupancy" );
            Format_Message( New_Message(JN_NAK, sizeof(data), "Server", data), message );
        }

    } else {
        //Couldn't find session. Send JN_NAK to client
        sprintf( data, "Error: Couldn't find session %s", m->data );
        Format_Message( New_Message(JN_NAK, sizeof(data), "Server", data), message );
    }
    
    write( UserDb[Get_UserId( m->source )].socketfd, message, sizeof(message) );
//    Print_SessionDb();
}


//Find chatroom, and send to message to everyone in the chat room
void Process_Message( struct Message* m ) {
    
    //Parse message as m->data comes in form: "<session name> <message to session>"
    char session_name[MAXNAME];
    sscanf( m->data, "%s", session_name );
    
    //Look for session. Do something only if it exists and sender is a part of the chat
    int session_id = Find_Session( session_name );
    
    if ( session_id != ERROR  && Get_MemberId(m->source, session_id) != ERROR ) {
        
        //Format message to send to members
        char message_buf[MAXLINE];
        Format_Message( New_Message(MESSAGE, sizeof(m->data), m->source, m->data), message_buf );
        
        //Send to all members
        for ( int i = 0; i < MAXUSERS; i++ ) {
            
            int member_id = SessionDb[session_id].members[i];
            
            //Send to all members of chat except source and non-active members
            if ( member_id != MEMBER_DEL && member_id != Get_UserId(m->source) && UserDb[member_id].active == 1 ) {
                write( UserDb[ member_id ].socketfd, message_buf, sizeof(message_buf) );
            }
        }
    }
}


//check chatroom and user exist
//Add user to chatroom if they are not already in the chatroom, and are online
void Process_Invite( struct Message* m ) {
    
    //Parse message as m->data comes in form: "<session name> <username>"
    char session_name[MAXNAME], username[MAXNAME];
    sscanf( m->data, "%s %s", session_name, username );
    
    //Look for session and username.
    int session_id = Find_Session( session_name );
    int user_id = Get_UserId( username );
    
    //store response back to client
    char data[MAXLINE], message[MAXLINE];
    
    //Do something if session and user exists and is active, 
    //AND that the sending user is already a part of the chat
    if ( session_id != ERROR && user_id != ERROR && UserDb[user_id].active == 1 && Get_MemberId(m->source, session_id) != ERROR ) {
        
        //Look for user in session. Add them if not already in session
        if ( Get_MemberId(username, session_id) == ERROR ) {
            
            //Proceed to add member to conference space permitting
            int avail_id = Get_Available_Member_Id(SessionDb[session_id]);
            SessionDb[session_id].num_members ++;

            //Space exists for member to join
            if ( avail_id != ERROR ) {
                SessionDb[session_id].members[avail_id] = Get_UserId( username ); //add user to member list

                //Format JN_ACK
                sprintf( data, "Added %s to %s", username, session_name );
                Format_Message( New_Message(JN_ACK, sizeof(data), "Server", data), message );
                
                //Send to newly added user to notify them
                char notif[MAXLINE], notif_buff[MAXLINE];
                sprintf( notif, "Added to %s by %s", session_name, m->source );
                Format_Message( New_Message(JN_ACK, sizeof(notif), "Server", notif), notif_buff );
                write( UserDb[Get_UserId( username )].socketfd, notif_buff, sizeof(notif_buff) );

            } else {
                //No more room in session. Send JN_NAK to client
                strcpy( data, "Error: Session is already at maximum occupancy" );
                Format_Message( New_Message(JN_NAK, sizeof(data), "Server", data), message );
            }
            
        } else {
            //member already in chat
            sprintf( data, "Error: %s is already in %s", username, session_name );
            Format_Message( New_Message(JN_NAK, sizeof(data), "Server", data), message );
        }
        
    } else {

        strcpy( data, "Error: Could not fulfill invite request" );
        Format_Message( New_Message(JN_NAK, sizeof(data), "Server", data), message );
    }
    
    //Tell user who initiated the event in all situations
    write( UserDb[Get_UserId( m->source )].socketfd, message, sizeof(message) );
}


//return the Session Id (Index in SessionDB) if found
//ERROR if not found
int Find_Session( unsigned char name[] ) {
    
    for ( int i = 0; i < MAXSESSIONS; i++ ) {
        if ( strcmp(name, SessionDb[i].name) == 0 ) {
            return i;
        }
    }
    
    return ERROR;
}


//Get index where username is stored in UserDb. If it does not exist, return ERROR
int Get_UserId( char username[] ) {

    for ( int i = 0; i < MAXUSERS; i++ ) {
        
        //if username found
        if ( strcmp(UserDb[i].username, username) == 0 ) {
            return i;
        }
    }
    
    return -1;
}


//Get index where User is stored in SessionDb.members/ Return ERROR if not found
int Get_MemberId( char username[], int session_Id ) {
    
    //User Id in UserDb
    int UserId = Get_UserId( username );
    
    for ( int i = 0; i < MAXUSERS; i++ ) {
        if ( (UserId != ERROR) && (SessionDb[session_Id].members[i] == UserId) ) {
            return i;
        } 
    }
    
    return ERROR;
}


//Return index of first slot in session member array with 0 or MEMBER_DEL flag
//Return ERROR if none available
int Get_Available_Member_Id( struct Session session ) {
    
    for ( int i = 0; i < MAXSESSIONS; i++ ) {
        if ( session.members[i] == MEMBER_DEL ) {
            return i;
        }
    }
    
    return ERROR;
}


//Printing functions for debuggin
void Print_UserDb() {
    
    for ( int i = 0; i < MAXUSERS; i++ ) {
        printf( "User %d\n", i );
        printf( "username: %s\n", UserDb[i].username );
        printf( "password: %s\n", UserDb[i].password );
        printf( "active: %d\n", UserDb[i].active );
        printf( "socketfd: %d\n\n", UserDb[i].socketfd );
    }
}

void Print_SessionDb() {
    
    for ( int i = 0; i < MAXSESSIONS; i++ ) { 
        printf( "name: %s\n", SessionDb[i].name );
        printf( "number of members: %d\n", SessionDb[i].num_members );
        printf( "members: " );
        
        for ( int j = 0; j < MAXUSERS; j++ ) {
            printf( "%d  ", SessionDb[i].members[j] );
        }
        
        printf("\n");
    }
    
    printf("\n");
}

