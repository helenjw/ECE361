#ifndef PACKET_H
#define PACKET_H

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_FILEDATA_SIZE 1000
#define MAX_PACKET_SIZE 4096

struct Packet {
        unsigned int total_frag;
        unsigned int frag_no;
        unsigned int size;
        char* filename;
        char filedata[MAX_FILEDATA_SIZE];
};


//Returns the total number of UDP packets needed to transfer file
int Get_Total_Frag ( FILE *stream );


//Makes a new packet and populates Packet struct (like a constructor)
struct Packet* New_Packet ( int total_frag, int frag_no, char* filename, FILE *stream );


//Prints each field in Packet struct for debugging purposes
void Print_Packet ( struct Packet* p );


//stores string to be delivered to server in the format in buffer:
//<total_frag> : <frag_no> : <size> : <filename> : <filedata>
char* Format_Packet( struct Packet* p, char buffer[] );

#endif /* PACKET_H */

