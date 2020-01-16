/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   packet.h
 * Author: jiang256
 *
 * Created on October 14, 2019, 1:18 PM
 */

#ifndef PACKET_H
#define PACKET_H

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


//Reconstructs string from client into Packet struct
struct Packet* New_Packet( char str[] );


//For debugging purposes
void Print_Packet ( struct Packet* p );


//Find index of Kth delimiter. Used to determine where data in the packet starts
int Index_of_Kth ( char str[], char delimiter, int k );

#endif /* PACKET_H */


