//Includes data types used to define endpoints statically. Features also
//some support functions, mostly used in initalizations.
#ifndef ENDPOINTDEFS_H
#define ENDPOINTDEFS_H

#include <stdlib.h>
#include "mca.h"

//Unit tests use their own endpoint defines
#ifdef UTEST
    #include "utester.h"
#else
    #include "endpointlist.h"
#endif

//struct used to tuple the endpoint indentifier
struct endPointID
{
    mca_domain_t domain_id; //domain where the node belong to
    mca_node_t node_id; //node where the end point belong to
    unsigned int port_id; //the port associated with the endpoint
};

//enum for channel direction
typedef enum {
    CHAN_NO_DIR = 0,
    CHAN_DIR_SEND,
    CHAN_DIR_RECV,
} channel_dir;

//enum for channel types 
typedef enum {
    MCAPI_NO_CHAN = 0,
    MCAPI_PKT_CHAN,
    MCAPI_SCL_CHAN,
} channel_type;

//the format of message queue name used by configuration and connectionless
//message. meant to filled with sprintf in order: domain, node and port number
#define MSGQ_NAME_FORMAT "/MCAPI_MSG_D%X_N%X_P%X"

//the format of message queue name used by channels. meant to filled with
//receiving endpoint identifier in order: domain, node and port number
#define CHAN_NAME_FORMAT "/MCAPI_CHAN_D%X_N%X_P%X"

//The predefined values of an endpoint. These are supposed to stay constant
//in runtime, except names are initialized when the node is initialized.
struct endPointDef
{
    struct endPointID us; //own identifier
    channel_type type; //the channel type, or no channel
    channel_dir dir; //the direction, or nol direction
    struct endPointID them; //other side of channel if applicable
    size_t scalar_size; //number of bytes in single scalar, if applicable.
    char msg_name[sizeof(MSGQ_NAME_FORMAT)]; //the name of message/config queue
    char chan_name[sizeof(CHAN_NAME_FORMAT)]; //the name of msgq of channel
};

//finds endpoint definition matching the tuple. return MCAPI_NULL if not.
struct endPointDef* findDef( mca_domain_t domain_id, mca_node_t node_id,
unsigned int port_id);

//returns true, if the identifiers have same values, else false
mca_boolean_t same( struct endPointID first, struct endPointID scnd );

//constructs both messagequeue names for each def
void initMsgqNames();

//Array containing all definitions of predefined endpoints
static struct endPointDef endPointDefs_[] = DEF_LIST;
#endif
