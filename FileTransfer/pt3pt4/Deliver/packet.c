#include "packet.h"

//Returns the total number of UDP packets needed to transfer file
int Get_Total_Frag ( FILE *stream ) {
    
    //go to the end of file, get stream position for size, then return stream to beginning
    fseek( stream, 0L, SEEK_END );
    long int size = ftell( stream );
    rewind( stream );
    
    //return the ceiling... except netbeans isn't recongizing ceiling
    return ( ((double) size + MAX_FILEDATA_SIZE -1) / MAX_FILEDATA_SIZE );
}


//Makes a new packet and populates struct Packet
struct Packet* New_Packet( int total_frag, int frag_no, char* filename, FILE *stream ) {
    
    //new packet structure
    struct Packet *p = malloc( sizeof(struct Packet) );
    
    //allocating given parameters
    p->total_frag = total_frag;
    p->frag_no = frag_no;
    p->filename = filename;
    
    //getting file size and file data
    p->size = fread( p->filedata, sizeof(char), MAX_FILEDATA_SIZE, stream );

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


//stores string to be delivered to server in the format:
//<total_frag> : <frag_no> : <size> : <filename> : <filedata>
char* Format_Packet( struct Packet* p, char buffer[] ) {
    
    int offset = sprintf( buffer, "%d:%d:%d:%s:", p->total_frag, p->frag_no, p->size, p->filename );
    memcpy( buffer + offset, p->filedata, p->size );
    
    return buffer;
}
