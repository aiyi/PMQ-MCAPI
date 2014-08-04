//This module has functions used in scalar MCAPI-communication.
#include "channel.h"
#include "endpoint.h"

void mcapi_sclchan_connect_i(
    MCAPI_IN mcapi_endpoint_t  send_endpoint, 
    MCAPI_IN mcapi_endpoint_t  receive_endpoint, 
    MCAPI_OUT mcapi_request_t* request, 
    MCAPI_OUT mcapi_status_t* mcapi_status)
{
    mcapi_chan_connect( send_endpoint, receive_endpoint, request, mcapi_status);
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
    //check for initialization
    if ( mcapi_trans_initialized() == MCAPI_FALSE )
    {
        //no init means failure
        *mcapi_status = MCAPI_ERR_NODE_NOTINIT;
        return;
    }

    //critical section for endpoint begins here
    LOCK_CHANNEL( send_handle );

    //handle must be valid send handle
    if ( !mcapi_trans_valid_sclchan_send_handle(send_handle) )
    {
        *mcapi_status = MCAPI_ERR_CHAN_INVALID;
    }
    else
    {
        //POSIX will handle the rest, timeout of sending endpoint is used
        //priority is fixed to that expected of channel traffic
        *mcapi_status = pmq_send( send_handle.us->chan_msgq_id,
        (void*)&dataword, bytes, MCAPI_MAX_PRIORITY+1,
        send_handle.us->time_out );
    }

    //critical section ends
    UNLOCK_CHANNEL( send_handle );
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
    //the priority obtained
    unsigned msg_prio;
    //the received value
    mcapi_uint64_t value = -1;
    //how long message we got in bytes
    size_t mslen;

    //check for initialization
    if ( !mcapi_trans_initialized() )
    {
        //no init means failure
        *mcapi_status = MCAPI_ERR_NODE_NOTINIT;
        return -1;
    }

    LOCK_CHANNEL( receive_handle );

    //handle must be valid
    if ( !mcapi_trans_valid_sclchan_recv_handle(receive_handle) )
    {
        *mcapi_status = MCAPI_ERR_CHAN_INVALID;
    }
    else
    {
        //POSIX will handle the recv, timeout of receiving endpoint is used
        *mcapi_status = pmq_recv( receive_handle.us->chan_msgq_id,
        (void*)&value, bytes, &mslen, &msg_prio, receive_handle.us->time_out );
    }

    UNLOCK_CHANNEL( receive_handle );

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
    //return value
    mcapi_uint_t to_ret = MCAPI_NULL;

    //check for initialization
    if ( mcapi_trans_initialized() == MCAPI_FALSE )
    {
        //no init means failure
        *mcapi_status = MCAPI_ERR_NODE_NOTINIT;
        return MCAPI_NULL;
    }

    LOCK_CHANNEL( receive_handle );

    //handle must be valid
    if ( !mcapi_trans_valid_sclchan_recv_handle( receive_handle ) )
    {
        *mcapi_status = MCAPI_ERR_CHAN_INVALID;
    }
    else
    {
        to_ret = pmq_avail( receive_handle.us->chan_msgq_id, mcapi_status );
    }

    UNLOCK_CHANNEL( receive_handle );

    return to_ret;
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
