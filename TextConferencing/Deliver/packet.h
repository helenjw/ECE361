#ifndef PACKET_H
#define PACKET_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//message types
#define LOGIN 0
#define LO_ACK 1
#define LO_NAK 2
#define EXIT 3
#define JOIN 4
#define JN_ACK 5
#define JN_NAK 6
#define LEAVE_SESS 7
#define NEW_SESS 8
#define NS_ACK 9
#define MESSAGE 10 
#define QUERY 11
#define QU_ACK 12
#define INVITE 13

#define ERROR -1
#define HANGUP 0

#define MAXLINE 1000
#define MAXNAME 100

struct Message {
    unsigned int type;
    unsigned int size;
    unsigned char source[MAXNAME];
    unsigned char data[MAXLINE];
};


//Creates new Message
struct Message* New_Message( unsigned int type, unsigned int size, unsigned char source[], unsigned char data[] );

//Decodes message sent from client in the format <type>:<size>:<source>:<data>
struct Message* Decode_Message( char message_buff[] );

//Format message to send to server using format: <type> : <size> : <source> : <data>
char* Format_Message( struct Message* m, char buffer[] );

//Find index of kth character starting from left.
int Index_of_Kth( char str[], char delimiter, int k );

//For debugging
void Print_Message( struct Message *m );

#endif /* PACKET_H */

