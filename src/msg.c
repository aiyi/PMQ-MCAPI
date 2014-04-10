//This module has functions used in message-oriented MCAPI-communication.
#include <stdio.h>
#include <string.h>
#include <mcapi.h>
#include "pmq_layer.h"

void mcapi_msg_send(
    MCAPI_IN mcapi_endpoint_t send_endpoint, 
    MCAPI_IN mcapi_endpoint_t receive_endpoint, 
    MCAPI_IN void* buffer, 
    MCAPI_IN size_t buffer_size, 
    MCAPI_IN mcapi_priority_t priority, 
    MCAPI_OUT mcapi_status_t* mcapi_status)
{
    //check for initialization
    if ( mcapi_trans_initialized() == MCAPI_FALSE )
    {
        //no init means failure
        *mcapi_status = MCAPI_ERR_NODE_NOTINIT;
        return;
    }

    //check for valid endpoint
    if ( mcapi_trans_valid_endpoints( send_endpoint, receive_endpoint )
    == MCAPI_FALSE || !mcapi_trans_endpoint_isowner( send_endpoint ) )
    {
        *mcapi_status = MCAPI_ERR_ENDP_INVALID;
        return;
    }

    //must not be channeled
    if ( receive_endpoint->defs != MCAPI_NULL &&
        receive_endpoint->defs->type != MCAPI_NO_CHAN )
    {
        *mcapi_status = MCAPI_ERR_GENERAL;
        return;
    }

    //check for priority
    if ( mcapi_trans_valid_priority( priority ) == MCAPI_FALSE )
    {
        *mcapi_status = MCAPI_ERR_PRIORITY;
        return;
    }

    //check for buffer
    if ( mcapi_trans_valid_buffer_param(buffer) == MCAPI_FALSE )
    {
        *mcapi_status = MCAPI_ERR_PARAMETER;
        return;
    }

    //check for size
    if ( buffer_size > MCAPI_MAX_MESSAGE_SIZE )
    {
        *mcapi_status = MCAPI_ERR_MSG_SIZE;
        return;
    }

    //POSIX will handle the rest, timeout of sending endpoint is used
    *mcapi_status = pmq_send( receive_endpoint->msgq_id, buffer, buffer_size,
    MCAPI_MAX_PRIORITY - priority, send_endpoint->time_out );
}

void mcapi_msg_recv(
    MCAPI_IN mcapi_endpoint_t  receive_endpoint,  
    MCAPI_OUT void* buffer, 
    MCAPI_IN size_t buffer_size, 
    MCAPI_OUT size_t* received_size, 
    MCAPI_OUT mcapi_status_t* mcapi_status)
{
    //the intermediate buffer for receiving. needed for the receive call
    char recv_buf[MCAPI_MAX_MESSAGE_SIZE];
    //the priority obtained
    unsigned msg_prio;
    //how long message we got in bytes
    size_t mslen;

    //check for initialization
    if ( mcapi_trans_initialized() == MCAPI_FALSE )
    {
        //no init means failure
        *mcapi_status = MCAPI_ERR_NODE_NOTINIT;
        return;
    }

    //check for valid endpoint
    if ( mcapi_trans_valid_endpoint( receive_endpoint ) == MCAPI_FALSE ||
    !mcapi_trans_endpoint_isowner( receive_endpoint ) )
    {
        *mcapi_status = MCAPI_ERR_ENDP_INVALID;
        return;
    }

    //check for buffer
    if ( mcapi_trans_valid_buffer_param(buffer) == MCAPI_FALSE )
    {
        *mcapi_status = MCAPI_ERR_PARAMETER;
        return;
    }

    //check for received size
    if ( received_size == NULL )
    {
        *mcapi_status = MCAPI_ERR_PARAMETER;
        return;
    }

    //must not be channeled
    if ( receive_endpoint->defs != MCAPI_NULL &&
        receive_endpoint->defs->type != MCAPI_NO_CHAN )
    {
        *mcapi_status = MCAPI_ERR_GENERAL;
        return;
    }

    //POSIX will handle the recv, timeout of receiving endpoint is used
    *mcapi_status = pmq_recv( receive_endpoint->msgq_id, recv_buf, 
    MCAPI_MAX_MESSAGE_SIZE, &mslen, &msg_prio, receive_endpoint->time_out );

    if ( *mcapi_status != MCAPI_SUCCESS )
    {
        return;
    }

    //pardon? wrong priority indicates wrong sort of traffic
    if( msg_prio < 0 || msg_prio > MCAPI_MAX_PRIORITY )
    {
        fprintf(stderr,"Received message with wrong priority!");
        *mcapi_status = MCAPI_ERR_GENERAL;

        return;
    }

    //check for size
    if ( mslen > buffer_size )
    {
        //too many means error and limiting the copy
        *mcapi_status = MCAPI_ERR_MSG_TRUNCATED;
        *received_size = buffer_size;
    }
    else
    {
        *mcapi_status = MCAPI_SUCCESS;
        *received_size = mslen;
    }

    //finally, copy the intermediate buffer to the output buffer
    memcpy( buffer, recv_buf, *received_size );
}

mcapi_uint_t mcapi_msg_available(
    MCAPI_IN mcapi_endpoint_t receive_endpoint,
    MCAPI_OUT mcapi_status_t* mcapi_status )
{
    //check for initialization
    if ( mcapi_trans_initialized() == MCAPI_FALSE )
    {
        //no init means failure
        *mcapi_status = MCAPI_ERR_NODE_NOTINIT;
        return MCAPI_NULL;
    }

    //check for valid endpoint
    if ( mcapi_trans_valid_endpoint( receive_endpoint ) == MCAPI_FALSE ||
    !mcapi_trans_endpoint_isowner( receive_endpoint )  )
    {
        *mcapi_status = MCAPI_ERR_ENDP_INVALID;
        return MCAPI_NULL;
    }

    return pmq_avail( receive_endpoint->msgq_id, mcapi_status );
}
