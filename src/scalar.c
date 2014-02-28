//This module has functions used in scalar MCAPI-communication.
#include "channel.h"
#include <stdio.h>
#include <errno.h>
#include <time.h>

void mcapi_sclchan_connect_i(
 	MCAPI_IN mcapi_endpoint_t  send_endpoint, 
 	MCAPI_IN mcapi_endpoint_t  receive_endpoint, 
 	MCAPI_OUT mcapi_request_t* request, 
 	MCAPI_OUT mcapi_status_t* mcapi_status)
{
    mcapi_chan_connect( send_endpoint, receive_endpoint, request, mcapi_status);

    if ( *mcapi_status != MCAPI_PENDING )
        return;

    //check for incompatible sizes
    if ( send_endpoint->defs->scalar_size !=
    receive_endpoint->defs->scalar_size )
    {
        *mcapi_status = MCAPI_ERR_ATTR_INCOMPATIBLE;
    }
}

void mcapi_sclchan_recv_open_i(
 	MCAPI_OUT mcapi_sclchan_recv_hndl_t* receive_handle, 
 	MCAPI_IN mcapi_endpoint_t receive_endpoint, 
 	MCAPI_OUT mcapi_request_t* request, 
 	MCAPI_OUT mcapi_status_t* mcapi_status) 
{
    mcapi_chan_open(receive_handle, receive_endpoint, request, mcapi_status,
    MCAPI_SCL_CHAN, CHAN_DIR_RECV);
}

void mcapi_sclchan_send_open_i(
 	MCAPI_OUT mcapi_sclchan_send_hndl_t* send_handle, 
 	MCAPI_IN mcapi_endpoint_t  send_endpoint, 
 	MCAPI_OUT mcapi_request_t* request, 
 	MCAPI_OUT mcapi_status_t* mcapi_status)
{
    mcapi_chan_open(send_handle, send_endpoint, request, mcapi_status,
    MCAPI_SCL_CHAN, CHAN_DIR_SEND);
}

void mcapi_sclchan_recv_close_i(
 	MCAPI_IN mcapi_sclchan_recv_hndl_t receive_handle, 
 	MCAPI_OUT mcapi_request_t* request, 
 	MCAPI_OUT mcapi_status_t* mcapi_status)
{
    mcapi_chan_close( receive_handle, request, mcapi_status,
    MCAPI_SCL_CHAN, CHAN_DIR_RECV);
}

void mcapi_sclchan_send_close_i(
 	MCAPI_IN mcapi_sclchan_send_hndl_t send_handle, 
 	MCAPI_OUT mcapi_request_t* request, 
 	MCAPI_OUT mcapi_status_t* mcapi_status)
{
    mcapi_chan_close( send_handle, request, mcapi_status,
    MCAPI_SCL_CHAN, CHAN_DIR_SEND );
}

inline void mcapi_sclchan_send(
 	MCAPI_IN mcapi_sclchan_send_hndl_t send_handle, 
 	MCAPI_IN mcapi_uint64_t dataword,
 	MCAPI_OUT mcapi_status_t* mcapi_status,
    size_t bytes)
{
    //timeout used by the posix function: actually no time at all
    struct timespec time_limit = { 0, 0 };
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

    //handle must be valid send handle
    if ( !mcapi_trans_valid_sclchan_send_handle(send_handle) )
    {
        *mcapi_status = MCAPI_ERR_CHAN_INVALID;
        return;
    }

    //timeout of sending endpoint is used
    timeout = send_handle.us->time_out;

    if ( timeout == MCAPI_TIMEOUT_INFINITE )
    {
        //sending the message, priority is inversed, as it works that way in msgq
        result = mq_send(send_handle.us->chan_msgq_id, (void*)&dataword, 
        bytes, MCAPI_MAX_PRIORITY+1 );
    }
    else
    {
        //specify timeout for the call: first take the current time
        clock_gettime( CLOCK_REALTIME, &time_limit );
        //and then add the needed seconds
        time_t seconds = timeout/1000;
        time_limit.tv_sec += seconds;
        //and needed millis
        long millis = (timeout%1000)*1000;
        time_limit.tv_nsec += millis;

        //sending the message, priority is inversed, as it works that way in msgq
        result = mq_timedsend(send_handle.us->chan_msgq_id, (void*)&dataword, 
        bytes, MCAPI_MAX_PRIORITY+1, &time_limit );
    }

    //if it was a timeout, we shall return with timeout
    if ( result == -1 )
    {
        if ( errno != ETIMEDOUT )
        {
            perror("mq send scalar");
            *mcapi_status = MCAPI_ERR_GENERAL;
        }
        else
            *mcapi_status = MCAPI_ERR_GENERAL;

        return;
    }
    *mcapi_status = MCAPI_SUCCESS; 
}

void mcapi_sclchan_send_uint64(
 	MCAPI_IN mcapi_sclchan_send_hndl_t send_handle, 
 	MCAPI_IN mcapi_uint64_t dataword,
 	MCAPI_OUT mcapi_status_t* mcapi_status)
{
    mcapi_sclchan_send( send_handle, dataword, mcapi_status, 8 );
}

void mcapi_sclchan_send_uint32(
 	MCAPI_IN mcapi_sclchan_send_hndl_t send_handle, 
 	MCAPI_IN mcapi_uint32_t dataword,
 	MCAPI_OUT mcapi_status_t* mcapi_status)
{
    mcapi_sclchan_send( send_handle, dataword, mcapi_status, 4 );
}

void mcapi_sclchan_send_uint16(
 	MCAPI_IN mcapi_sclchan_send_hndl_t send_handle, 
 	MCAPI_IN mcapi_uint16_t dataword,
 	MCAPI_OUT mcapi_status_t* mcapi_status)
{
    mcapi_sclchan_send( send_handle, dataword, mcapi_status, 2 );
}

void mcapi_sclchan_send_uint8(
 	MCAPI_IN mcapi_sclchan_send_hndl_t send_handle, 
 	MCAPI_IN mcapi_uint8_t dataword,
 	MCAPI_OUT mcapi_status_t* mcapi_status)
{
    mcapi_sclchan_send( send_handle, dataword, mcapi_status, 1 );
}

inline mcapi_uint64_t mcapi_sclchan_recv(
 	MCAPI_IN mcapi_sclchan_recv_hndl_t receive_handle,
 	MCAPI_OUT mcapi_status_t* mcapi_status,
    size_t bytes)
{
    //how long message we got in bytes
    size_t mslen;
    //the received value
    mcapi_uint64_t value;
    //the priority obtained
    unsigned msg_prio;
    //timeout used by posix-function: actually no time at all
    struct timespec time_limit = { 0, 0 };
    //how close we are to timeout
    uint32_t ticks = 0;
    //timeout of the operation as whole
    mcapi_timeout_t timeout;

    //check for initialization
    if ( !mcapi_trans_initialized() )
    {
        //no init means failure
        *mcapi_status = MCAPI_ERR_NODE_NOTINIT;
        return -1;
    }

    //handle must be valid
    if ( !mcapi_trans_valid_sclchan_recv_handle(receive_handle) )
    {
        *mcapi_status = MCAPI_ERR_CHAN_INVALID;
        return -1;
    }

    //timeout of sending endpoint is used
    timeout = receive_handle.us->time_out;

    if ( timeout == MCAPI_TIMEOUT_INFINITE )
    {
        //sending the message, priority is inversed, as it works that way in msgq
        mslen = mq_receive(receive_handle.us->chan_msgq_id, (void*)&value,
            bytes, &msg_prio);
    }
    else
    {
        //specify timeout for the call: first take the current time
        clock_gettime( CLOCK_REALTIME, &time_limit );
        //and then add the needed seconds
        time_t seconds = timeout/1000;
        time_limit.tv_sec += seconds;
        //and needed millis
        long millis = (timeout%1000)*1000;
        time_limit.tv_nsec += millis;

        //sending the message, priority is inversed, as it works that way in msgq
        mslen = mq_timedreceive(receive_handle.us->chan_msgq_id, (void*)&value,
            bytes, &msg_prio, &time_limit);
    }

    //if it was a timeout, there is no need for action
    if ( mslen == -1 )
    {
        if ( errno != ETIMEDOUT )
        {
            perror("mq receive scalar");
            *mcapi_status = MCAPI_ERR_GENERAL;
        }
        else
            *mcapi_status = MCAPI_ERR_GENERAL;

        return;
    }

    //success: return the received value
    *mcapi_status = MCAPI_SUCCESS;
    return value;
}

mcapi_uint64_t mcapi_sclchan_recv_uint64(
 	MCAPI_IN mcapi_sclchan_recv_hndl_t receive_handle,
 	MCAPI_OUT mcapi_status_t* mcapi_status)
{
    return mcapi_sclchan_recv( receive_handle, mcapi_status, 8 );
}

mcapi_uint32_t mcapi_sclchan_recv_uint32(
 	MCAPI_IN mcapi_sclchan_recv_hndl_t receive_handle,
 	MCAPI_OUT mcapi_status_t* mcapi_status)
{
    return mcapi_sclchan_recv( receive_handle, mcapi_status, 4 );
}

mcapi_uint16_t mcapi_sclchan_recv_uint16(
 	MCAPI_IN mcapi_sclchan_recv_hndl_t receive_handle,
 	MCAPI_OUT mcapi_status_t* mcapi_status)
{
    return mcapi_sclchan_recv( receive_handle, mcapi_status, 2 );
}

mcapi_uint8_t mcapi_sclchan_recv_uint8(
 	MCAPI_IN mcapi_sclchan_recv_hndl_t receive_handle,
 	MCAPI_OUT mcapi_status_t* mcapi_status)
{
    return mcapi_sclchan_recv( receive_handle, mcapi_status, 1 );
}

mcapi_uint_t mcapi_sclchan_available(
MCAPI_IN mcapi_sclchan_recv_hndl_t receive_handle,
MCAPI_OUT mcapi_status_t* mcapi_status )
{
    //handle must be valid
    if ( !mcapi_trans_valid_sclchan_recv_handle( receive_handle ) )
    {
        *mcapi_status = MCAPI_ERR_CHAN_INVALID;
        return MCAPI_NULL;
    }

    return mcapi_chan_available( receive_handle, mcapi_status );
}

inline mcapi_boolean_t mcapi_trans_valid_sclchan_send_handle( mcapi_sclchan_send_hndl_t handle)
{
    if ( handle.us == MCAPI_NULL )
    {
        return MCAPI_FALSE;
    }

    return MCAPI_TRUE;
}


inline mcapi_boolean_t mcapi_trans_valid_sclchan_recv_handle( mcapi_sclchan_recv_hndl_t handle)
{
    if ( handle.us == MCAPI_NULL )
    {
        return MCAPI_FALSE;
    }

    return MCAPI_TRUE;
}
