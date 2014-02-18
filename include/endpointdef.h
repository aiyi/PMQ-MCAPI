//Includes data types used to define endpoints statically. Features also
//some support functions, mostly used in initalizations.
//ALSO contains static defines needed in unit tests. Otherwise will
//refer to file endpointlist.h for endpoint definitions.
#ifndef ENDPOINTDEFS_H
#define ENDPOINTDEFS_H
#include <stdlib.h>
#include "mca.h"

#ifdef UTEST
    //These defines are to be used in tests. Put here to avoid overwrite.
    #define MSG_PAUL {1, 2, 0}
    #define MSG_PAT {1, 2, 1}

    #define FOO {0, 5, 1}
    #define BAR {0, 6, 2}
    #define SEND {0, 8, 4}
    #define RECV {0, 8, 6}

    #define SSCL {0, 13, 2}
    #define RSCL {0, 14, 3}
    #define SSCL8 {0, 13, 4}
    #define RSCL8 {0, 14, 5}
    #define SSCL16 {0, 13, 6}
    #define RSCL16 {0, 14, 7}
    #define SSCL32 {0, 13, 8}
    #define RSCL32 {0, 14, 9}

    #define MSG_PAUL_DEF { MSG_PAUL, MCAPI_NO_CHAN, CHAN_NO_DIR, MSG_PAT, 0 }
    #define MSG_PAT_DEF { MSG_PAT, MCAPI_NO_CHAN, CHAN_NO_DIR, MSG_PAUL, 0 }

    #define FOO_DEF { FOO, MCAPI_PKT_CHAN, CHAN_DIR_SEND, BAR, 0 }
    #define BAR_DEF { BAR, MCAPI_PKT_CHAN, CHAN_DIR_RECV, FOO, 0 }
    #define SEND_DEF { SEND, MCAPI_PKT_CHAN, CHAN_DIR_SEND, RECV, 0 }
    #define RECV_DEF { RECV, MCAPI_PKT_CHAN, CHAN_DIR_RECV, SEND, 0 }

    #define SSCL_DEF { SSCL, MCAPI_SCL_CHAN, CHAN_DIR_SEND, RSCL, 8 }
    #define RSCL_DEF { RSCL, MCAPI_SCL_CHAN, CHAN_DIR_RECV, SSCL, 8 }
    #define SSCL_DEF8 { SSCL8, MCAPI_SCL_CHAN, CHAN_DIR_SEND, RSCL8, 1 }
    #define RSCL_DEF8 { RSCL8, MCAPI_SCL_CHAN, CHAN_DIR_RECV, SSCL8, 1 }
    #define SSCL_DEF16 { SSCL16, MCAPI_SCL_CHAN, CHAN_DIR_SEND, RSCL16, 2 }
    #define RSCL_DEF16 { RSCL16, MCAPI_SCL_CHAN, CHAN_DIR_RECV, SSCL16, 2 }
    #define SSCL_DEF32 { SSCL32, MCAPI_SCL_CHAN, CHAN_DIR_SEND, RSCL32, 4 }
    #define RSCL_DEF32 { RSCL32, MCAPI_SCL_CHAN, CHAN_DIR_RECV, SSCL32, 4 }

    #define DEF_LIST { MSG_PAUL_DEF, MSG_PAT_DEF, FOO_DEF, BAR_DEF, SEND_DEF, \
    RECV_DEF, SSCL_DEF, RSCL_DEF, SSCL_DEF8, RSCL_DEF8, SSCL_DEF16, \
    RSCL_DEF16, SSCL_DEF32, RSCL_DEF32 }
#else
    //In normal operation endpoints and channels are supposed to be there
    #include "endpointlist.h"
#endif

struct endPointID
{
    mca_domain_t domain_id; //domain where the node belong to
    mca_node_t node_id; //node where the end point belong to
    unsigned int port_id; //the port associated with the endpoint
};

/* enum for channel direction */
typedef enum {
    CHAN_NO_DIR = 0,
    CHAN_DIR_SEND,
    CHAN_DIR_RECV,
} channel_dir;

/* enum for channel types */
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
