//This module has functions used in message-oriented MCAPI-communication.
#include <stdio.h>
#include <string.h>
#include <mqueue.h>
#include <mcapi.h>
#include <errno.h>

void mcapi_msg_send(
 	MCAPI_IN mcapi_endpoint_t send_endpoint, 
 	MCAPI_IN mcapi_endpoint_t receive_endpoint, 
 	MCAPI_IN void* buffer, 
 	MCAPI_IN size_t buffer_size, 
 	MCAPI_IN mcapi_priority_t priority, 
 	MCAPI_OUT mcapi_status_t* mcapi_status)
{
    //timeout used by the posix function: actually no time at all
    struct timespec time_limit = { 0, 0 };
    //how close we are to timeout
    uint32_t ticks = 0;
    //the result of action
    mqd_t result = -1;
    //timeout of the operation as whole
    mcapi_timeout_t timeout;

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
    if ( buffer_size > MCAPI_MAX_MSG_SIZE )
    {
        *mcapi_status = MCAPI_ERR_MSG_SIZE;
        return;
    }

    //timeout of sending endpoint is used
    timeout = send_endpoint->time_out;

    //skip the sleep at first iteration
    goto send_action;

    do
    {
        //sleep a millisecond between iterations
        usleep(1000);

        send_action:
        //sending the message, priority is inversed, because it works that way in msgq
        result = mq_timedsend(receive_endpoint->msgq_id, buffer, buffer_size,
            MCAPI_MAX_PRIORITY - priority, &time_limit );

        //closer for time out!
        ++ticks;
    }
    while ( result == -1 && ( timeout == MCAPI_TIMEOUT_INFINITE || 
    ticks < timeout ) );

    //if it was a timeout, we shall return with timeout
    if ( result == -1 )
    {
        if ( errno != ETIMEDOUT )
        {
            perror("mq_send_msg");
            *mcapi_status = MCAPI_ERR_TRANSMISSION;
        }
        else
            *mcapi_status = MCAPI_TIMEOUT;

        return;
    }

    *mcapi_status = MCAPI_SUCCESS;
}

void mcapi_msg_recv(
 	MCAPI_IN mcapi_endpoint_t  receive_endpoint,  
 	MCAPI_OUT void* buffer, 
 	MCAPI_IN size_t buffer_size, 
 	MCAPI_OUT size_t* received_size, 
 	MCAPI_OUT mcapi_status_t* mcapi_status)
{
    //how long message we got in bytes
    size_t mslen;
    //the intermediate buffer for receiving. needed for the receive call
    char recv_buf[MCAPI_MAX_MSG_SIZE];
    //timeout used by posix-function: actually no time at all
    struct timespec time_limit = { 0, 0 };
    //how close we are to timeout
    uint32_t ticks = 0;
    //timeout of the operation as whole
    mcapi_timeout_t timeout;
    //the priority obtained
    unsigned msg_prio;

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

    //timeout of receiving endpoint is used
    timeout = receive_endpoint->time_out;

    //skip the sleep at first iteration
    goto recv_action;

    do
    {
        //sleep a millisecond between iterations
        usleep(1000);

        recv_action:
        //take the message from queue to the intermediate buffer, while rec
        mslen = mq_timedreceive(receive_endpoint->msgq_id, recv_buf,
            MCAPI_MAX_MSG_SIZE, &msg_prio, &time_limit);

        //closer for time out!
        ++ticks;
    }
    while ( mslen == -1 && ( timeout == MCAPI_TIMEOUT_INFINITE || 
    ticks < timeout ) );

    //if it was a timeout, we shall return with timeout
    if ( mslen == -1 )
    {
        if ( errno != ETIMEDOUT )
        {
            perror("mq_recv_msg");
            *mcapi_status = MCAPI_ERR_TRANSMISSION;
        }
        else
            *mcapi_status = MCAPI_TIMEOUT;

        return;
    }

    //pardon? wrong priority indicates wrong sort of traffic
    if( msg_prio < 0 || msg_prio > MCAPI_MAX_PRIORITY )
    {
        fprintf(stderr,"Received message with wrong priority!");
        *mcapi_status = MCAPI_ERR_GENERAL;

        return;
    }

    *received_size = mslen;

    //check for size
    if ( mslen > buffer_size )
    {
        //too many means error and limiting the copy
        *mcapi_status = MCAPI_ERR_MSG_TRUNCATED;
        mslen = buffer_size;
    }
    else
    {
        *mcapi_status = MCAPI_SUCCESS;
    }

    //finally, copy the intermediate buffer to the output buffer
    memcpy( buffer, recv_buf, mslen );
}

mcapi_uint_t mcapi_msg_available(
    MCAPI_IN mcapi_endpoint_t receive_endpoint,
    MCAPI_OUT mcapi_status_t* mcapi_status )
{
    //the queue of endpoint
    mqd_t msgq_id;
    //the retrieved attributes are obtained here for the check
    struct mq_attr uattr;

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

    //take id from endpoint
    msgq_id = receive_endpoint->msgq_id;

    //try to open the attributes...
    if (mq_getattr(msgq_id, &uattr) == -1)
    {
        perror("When obtaining msq attributes to check message count");
        *mcapi_status = MCAPI_ERR_GENERAL;
        return MCAPI_NULL;
    }

    //success
    *mcapi_status = MCAPI_SUCCESS;

    //and return the count
    return uattr.mq_curmsgs;
}
