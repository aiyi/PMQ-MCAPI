//Module has a function to unlink all posix message queues used by defined 
//endpoints. Such defines are looked up from list in endpointdef.h. If
//MAKE_EXECUTABLE is defined, this module has a main, which sole purpose
//is to call the clean-function.
#include "endpointdef.h"
#include <stdio.h>
#include <mqueue.h>

//constructs all possible message queue names and unlinks them
void clean()
{
    //iterator and size derived from the size of the array and its elements
    unsigned int i;
    size_t total_size = sizeof(endPointDefs_)/sizeof(struct endPointDef);
    //the name of the end point message queue
    char msg_name[sizeof(MSGQ_NAME_FORMAT)];
    //the name of msgq of channel
    char chan_name[sizeof(CHAN_NAME_FORMAT)]; 

    for ( i = 0; i < total_size; ++i )
    {
        struct endPointDef* e = &endPointDefs_[i];
        //the received is needed to construct name of channel to be closed
        struct endPointID receiver;
        //the "us" is needed to close the message queue used in configuration
        //and connectionless messages
        struct endPointID us = e->us;

        //construct name for the messagequeue of the endpoint
        if ( sprintf( msg_name, MSGQ_NAME_FORMAT, us.domain_id, us.node_id,
        us.port_id ) < 0 )
        {
            perror("Could not construct message msq name!\n");
        }

        //unlink it so that it is no longer used
        mq_unlink( msg_name );

        //pick receiver based on channel direction
        if ( e->dir == CHAN_DIR_RECV )
        {
            receiver = e->us;
        }
        else
        {
            //else there is nothing to do here
            continue;
        }

        //construct the channel name from receiving identifier
        if ( sprintf( chan_name, CHAN_NAME_FORMAT, receiver.domain_id,
        receiver.node_id, receiver.port_id ) < 0 )
        {
            perror("Could not construct channel msq name!\n");
        }

        //unlink so that it is no longer reserved
        mq_unlink( chan_name );
    }
}

//create main only if this is to become an executable
#ifdef MAKE_EXECUTABLE
int main()
{
    clean();

    return EXIT_SUCCESS;
}
#endif
