#ifndef DB_H
#define DB_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <netdb.h>
#include "packet.h"

#define MAXNAME 100
#define MAXUSERS 10
#define MAXSESSIONS 10
#define MEMBER_DEL -1
#define SESSION_DEL "!f~df9))"
#define ERROR -1

struct User {
    char username[MAXNAME];
    char password[MAXNAME];
    int active;
    int socketfd;
};

struct Session {
    char name[MAXNAME];
    int num_members; //number of active users
    int members[MAXUSERS];
};

//hard coded values for username and password
//Expandable to 100 users... because no one will use this anyways
static char UsernameDb[MAXUSERS][MAXNAME] = {
    "sophie",
    "helen",
    "justin"
};

static char PasswordDb[MAXUSERS][MAXNAME] = {
    "jiang",
    "jiang",
    "ko"
};

//db to store Users and active Sessions
static struct User UserDb[MAXUSERS] = {};
static struct Session SessionDb[MAXSESSIONS] = {};


//Using hard-coded data, populate UserDb
void Populate_UserDb();


//Process Login request and send back LO_ACK or LO_NAK
//True of LO_ACK, false if LO_NAK
//Modify UserDb to reflect login
bool Process_Login( struct Message* m, int socketfd );


//De-list source as active, and close their socketfd
void Process_Exit( struct Message* m );


//Find user in UserDb by socketfd, delist user as active and close connection
void Process_Unexpected_Exit( int socketfd );


//Lists every active member and sends as a message
void Process_Query( struct Message* m );


//send active user list
void Send_Active_Users( struct Message* m );


//send active sessions list
void Send_Active_Sessions( struct Message* m );


//send members of a specific session
void Send_Session_Members( struct Message* m );


//Insert newly created session in SessionDb
//Session creator is automatically joined to session
void Process_NewSess( struct Message* m );


//Delete requesting user from conference session
//Decrement num_members, and delete session if num_members = 0
void Process_LeaveSess( struct Message* m );


//Add requesting user to conference session and Increment num_members if successful
//Send back JN_ACK and JN_NAK
void Process_JoinSess( struct Message* m );


//Find chatroom, and send to message to everyone in the chat room
void Process_Message( struct Message* m );


//check chatroom and user exist
//Add user to chatroom if they are not already in the chatroom, and are online
void Process_Invite( struct Message* m );


//return the Session Id (Index in SessionDB) if found
//ERROR if not found
int Find_Session( unsigned char name[] );


//Get index where username is stored in UserDb
int Get_UserId( char username[] );


//Get index where User is stored in SessionDb.members
int Get_MemberId( char username[], int session_Id );


//Return index of first slot in session member array with 0 or MEMBER_DEL flag
//Return ERROR if none available
int Get_Available_Member_Id( struct Session session );


//for debugging
void Print_UserDb();
void Print_SessionDb();

#endif /* DB_H */

