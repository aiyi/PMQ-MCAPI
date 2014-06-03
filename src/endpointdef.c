#include "endpointdef.h"
#include "endpointlist.h"
#include <stdio.h>

struct endPointDef* findDef( unsigned int i )
{
    struct endPointDef* e = &endPointDefs_[i];

    return e;
}

mca_boolean_t sameID( struct endPointID first, struct endPointID scnd )
{
    //has same identifier values -> is same
    if ( first.domain_id != scnd.domain_id || first.node_id != scnd.node_id
        || first.port_id != scnd.port_id )
    {
        return MCA_FALSE;
    }

    return MCA_TRUE;
}

void initMsgqNames()
{
    unsigned int i;

    //assignning the names for all the predefined endpoints
    for ( i = 0; i < ENDPOINT_COUNT; ++i )
    {
        //the endpoint definition whose names are to be constructed
        struct endPointDef* e = &endPointDefs_[i];
        //the endpointidentifier of above
        struct endPointID us = e->us;
        //he receiving end of channel if applicable
        struct endPointID receiver;

        //construct name for the message/config queue
        if ( sprintf( e->msg_name, MSGQ_NAME_FORMAT, us.domain_id,
        us.node_id, us.port_id ) < 0 )
        {
            perror("Could not construct message msq name!\n");
        }

        //pick receiver based on channel direction
        if ( e->dir == CHAN_DIR_RECV )
        {
            receiver = e->us;
        }
        else if ( e->dir == CHAN_DIR_SEND )
        {
            receiver = e->them;
        }
        else
        {
            //else there is no channel here!
            continue;
        }

        //construct the channel name from receiving end identifier
        if ( sprintf( e->chan_name, CHAN_NAME_FORMAT, receiver.domain_id,
        receiver.node_id, receiver.port_id ) < 0 )
        {
            perror("Could not construct channel msq name!\n");
        }
    }
}
