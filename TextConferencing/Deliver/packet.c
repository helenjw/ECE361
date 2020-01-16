#include "packet.h"

//Creates and returns Message structure created using specified parameters
struct Message* New_Message( unsigned int type, unsigned int size, unsigned char source[], unsigned char data[] ) {
    
    //new Message structure
    struct Message *m = malloc( sizeof(struct Message) );
    
    //Populating structure with specified field values
    m->type = type;
    m->size = size;
    strcpy( m->source, source );
    strcpy( m->data, data );
    
    //Print_Message(m);
    
    return m;
}


//Decodes message sent from client in the format <type>:<size>:<source>:<data>
struct Message* Decode_Message( char message_buff[] ) {
    
    //create new message structure
    struct Message* m = malloc( sizeof(struct Message) );
    
    //populating Message struct fields
    char source_helper[MAXNAME];
    sscanf( message_buff, "%d:%d:%s", &m->type, &m->size, source_helper );
    strcpy( m->source, strtok(source_helper, ":") ) ; //last field is populated until white space detected -> truncate everything after first ':'
    
    //populating 'Data' field
    strcpy( m->data, &message_buff[Index_of_Kth(message_buff, ':', 3) + 1] );
    
    //Print_Message(m);
    
    return m;
}


//Format message to send to server using format: <type> : <size> : <source> : <data>
char* Format_Message( struct Message* m, char buffer[] ) {
    
    sprintf( buffer, "%d:%d:%s:%s", m->type, m->size, &m->source, &m->data );
    
//    printf("%s\n", &buffer);
    
    return buffer;
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


//For debugging
void Print_Message( struct Message *m ) {
    printf("Type : %d\n", m->type);
    printf("size : %d\n", m->size);
    printf("source : %s\n", m->source);
    printf("data : %s\n", m->data);
}
