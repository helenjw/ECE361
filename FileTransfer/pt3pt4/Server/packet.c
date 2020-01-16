/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "packet.h"

//Reconstructs string from client into Packet struct
struct Packet* New_Packet( char str[] ) {
    
    //create new Packet structure
    struct Packet* p = malloc( sizeof(struct Packet) );
    
    //populating Packet struct except for data field
    char filename_helper[MAX_PACKET_SIZE];
    sscanf( str, "%d:%d:%d:%s:", &(p->total_frag), &(p->frag_no), &(p->size), filename_helper );
    p->filename = strtok( filename_helper, ":" );  //last field is populated until white space detected -> truncate until first ':' from right
    
    
    //populating filedata sent by client
    memcpy( p->filedata, &str[ Index_of_Kth(str, ':', 4) + 1 ], p->size );  //data starts after 4th ':' from the left

    return p;
}

//For debugging purposes
void Print_Packet ( struct Packet* p ) {
    printf("Total Fragments : %d\n", p->total_frag);
    printf("Fragment Number : %d\n", p->frag_no);
    printf("Filename : %s\n", p->filename);   
    printf("Size : %d\n", p->size);
    printf("Data : %s\n", p->filedata);
}


//Find index of kth character starting from left.
int Index_of_Kth( char str[], char delimiter, int k ) {
    
    for ( int i = 0; i < strlen(str) ; i++ ) {
        if ( str[i] == delimiter ) {
            
            //counts down to find kth repeating character
            if ( k == 1 ) {
                return i;
            }
            
            k--;
        }
    }
    
    return -1;
}
