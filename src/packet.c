//This module has functions used in packet MCAPI-communication.
#include <stdio.h>
#include "channel.h"

//keyvalue-pair used in buffer handling
struct bufObject
{
    //the endpoint currently using the buffer: MCAPI_NULL if unused
    mcapi_endpoint_t endpoint;
    //the data contained in the buffer
    unsigned char data[MCAPI_MAX_PKT_SIZE+sizeof(unsigned int)];
};

//the all buffers within the use of this node
static struct bufObject bufferPool[MCAPI_MAX_BUFFERS];

//the place of next unchecked buffer
unsigned int buf_iter = 0;

//sets keys of all buffers to MCAPI_NULL
//must be called on init, in other words before buffer use
void bufClearAll()
{
    unsigned int i;

    //iterating through the whole buffer pool
    for ( i = 0; i < MCAPI_MAX_BUFFERS; ++i )
    {
        //take the next buffer
        struct bufObject* bo = &bufferPool[i];
        //and clear
        bo->endpoint = MCAPI_NULL;
        //set "us" at the end of the buffer
        bo->data[MCAPI_MAX_PKT_SIZE] = i;
    }
}

//returns next unused buffer or MCAPI_NULL, if none found
struct bufObject* bufFindEmpty()
{
    unsigned int i;

    //terminated after the whole buffer pool is gone-through
    for ( i = 0; i < MCAPI_MAX_BUFFERS; ++i )
    {
        //take the next buffer
        struct bufObject* bo = &bufferPool[buf_iter];

        //time to increase iterator
        ++buf_iter;

        //if through, zero it
        if ( buf_iter >= MCAPI_MAX_BUFFERS )
            buf_iter = 0;

        //if free, return
        if ( bo->endpoint == MCAPI_NULL )
            return bo;
    }

    return MCAPI_NULL;
}

void mcapi_pktchan_connect_i(
 	MCAPI_IN mcapi_endpoint_t  send_endpoint, 
 	MCAPI_IN mcapi_endpoint_t  receive_endpoint, 
 	MCAPI_OUT mcapi_request_t* request, 
 	MCAPI_OUT mcapi_status_t* mcapi_status)
{
    mcapi_chan_connect( send_endpoint, receive_endpoint, request,
    mcapi_status);
}

void mcapi_pktchan_recv_open_i(
 	MCAPI_OUT mcapi_pktchan_recv_hndl_t* receive_handle, 
 	MCAPI_IN mcapi_endpoint_t receive_endpoint, 
 	MCAPI_OUT mcapi_request_t* request, 
 	MCAPI_OUT mcapi_status_t* mcapi_status) 
{
    mcapi_chan_open( receive_handle, receive_endpoint, request,
    mcapi_status, MCAPI_PKT_CHAN, CHAN_DIR_RECV );
}

void mcapi_pktchan_send_open_i(
 	MCAPI_OUT mcapi_pktchan_send_hndl_t* send_handle, 
 	MCAPI_IN mcapi_endpoint_t  send_endpoint, 
 	MCAPI_OUT mcapi_request_t* request, 
 	MCAPI_OUT mcapi_status_t* mcapi_status)
{
    mcapi_chan_open( send_handle, send_endpoint, request,
    mcapi_status, MCAPI_PKT_CHAN, CHAN_DIR_SEND );
}

void mcapi_pktchan_recv_close_i(
 	MCAPI_IN mcapi_pktchan_recv_hndl_t receive_handle, 
 	MCAPI_OUT mcapi_request_t* request, 
 	MCAPI_OUT mcapi_status_t* mcapi_status)
{
    mcapi_chan_close( receive_handle, request, mcapi_status, MCAPI_PKT_CHAN,
    CHAN_DIR_RECV );
}

void mcapi_pktchan_send_close_i(
 	MCAPI_IN mcapi_pktchan_send_hndl_t send_handle, 
 	MCAPI_OUT mcapi_request_t* request, 
 	MCAPI_OUT mcapi_status_t* mcapi_status)
{
    mcapi_chan_close( send_handle, request, mcapi_status, MCAPI_PKT_CHAN,
    CHAN_DIR_SEND );
}

void mcapi_pktchan_send(
 	MCAPI_IN mcapi_pktchan_send_hndl_t send_handle, 
 	MCAPI_IN void* buffer, 
 	MCAPI_IN size_t size, 
 	MCAPI_OUT mcapi_status_t* mcapi_status)
{
    //check for initialization
    if ( mcapi_trans_initialized() == MCAPI_FALSE )
    {
        //no init means failure
        *mcapi_status = MCAPI_ERR_NODE_NOTINIT;
        return;
    }

    //size constrains apply
    if ( size > MCAPI_MAX_PKT_SIZE )
    {
        *mcapi_status = MCAPI_ERR_PKT_SIZE; 
        return;
    }

    //handle must be valid drmf handle
    if ( !mcapi_trans_valid_pktchan_send_handle(send_handle) )
    {
        *mcapi_status = MCAPI_ERR_CHAN_INVALID;
        return;
    }

    //buffer must be usable
    if ( !mcapi_trans_valid_buffer_param(buffer) || size < 0 )
    {
        *mcapi_status = MCAPI_ERR_PARAMETER;
        return;
    }

    //must be open
    if ( send_handle.us->open != 1 )
    {
        *mcapi_status = MCAPI_ERR_CHAN_INVALID;
        return;
    }

    //POSIX will handle the rest, timeout of sending endpoint is used
    //priority is fixed to that expected of channel traffic
    *mcapi_status = pmq_send( send_handle.us->chan_msgq_id, buffer,
    size, MCAPI_MAX_PRIORITY+1, send_handle.us->time_out );
}

void mcapi_pktchan_recv(
 	MCAPI_IN mcapi_pktchan_recv_hndl_t receive_handle, 
 	MCAPI_OUT void** buffer, 
 	MCAPI_OUT size_t* received_size, 
 	MCAPI_OUT mcapi_status_t* mcapi_status)
{
    //the buffer for receiving.
    struct bufObject* bo;
    //the priority obtained
    unsigned msg_prio;

    //check for initialization
    if ( !mcapi_trans_initialized() )
    {
        //no init means failure
        *mcapi_status = MCAPI_ERR_NODE_NOTINIT;
        return;
    }

    //handle must be valid
    if ( !mcapi_trans_valid_pktchan_recv_handle(receive_handle) )
    {
        *mcapi_status = MCAPI_ERR_CHAN_INVALID;
        return;
    }
  
    //buffer must be usable
    if ( !mcapi_trans_valid_buffer_param(buffer))
    {
        *mcapi_status = MCAPI_ERR_PARAMETER;
        return;
    }

    //must be open
    if ( receive_handle.us->open != 1 )
    {
        *mcapi_status = MCAPI_ERR_CHAN_INVALID;
        return;
    }

    //reserve buffer AFTER the initial pit holes
    bo = bufFindEmpty();

    //in princible, we ought to wait for an availiable buffer here,
    //but since no one can free one for us, we must return with timeout
    if ( bo == MCAPI_NULL )
    {
        *mcapi_status = MCAPI_TIMEOUT;
        return;
    }

    //POSIX will handle the recv, timeout of receiving endpoint is used
    *mcapi_status = pmq_recv( receive_handle.us->chan_msgq_id, bo->data,
    MCAPI_MAX_PKT_SIZE, received_size, &msg_prio,
    receive_handle.us->time_out );

    if ( *mcapi_status != MCAPI_SUCCESS )
    {
        return;
    }

    //pardon? wrong priority indicates wrong sort of traffic
    if( msg_prio != MCAPI_MAX_PRIORITY+1 )
    {
        fprintf(stderr,"Received packet message with wrong priority!");
        *mcapi_status = MCAPI_ERR_GENERAL;

        return;
    }

    //succée: mark ownership and pass the buffer to caller
    bo->endpoint = receive_handle.us;
    *buffer = &bo->data;

    *mcapi_status = MCAPI_SUCCESS; 
}

void mcapi_pktchan_release(
    MCAPI_IN void* buffer,
    MCAPI_OUT mcapi_status_t* mcapi_status)
{   
    //location variable used to extract metadata
    MCAPI_IN void* loc;
    //the buffer object associated with the actual buffer
    struct bufObject* bo;
    //pointer to the index of buffer object in pool
    unsigned int* i;

    //check for initialization
    if ( !mcapi_trans_initialized() )
    {
        //no init means failure
        *mcapi_status = MCAPI_ERR_NODE_NOTINIT;
        return;
    }

    //buffer must be usable
    if ( !mcapi_trans_valid_buffer_param(buffer) )
    {
        *mcapi_status = MCAPI_ERR_BUF_INVALID;
        return;
    }

    //magic: we get the struct index from the end of buffer
    loc = buffer + MCAPI_MAX_PKT_SIZE;
    i = (unsigned int*)loc;

    //out of bound means an error
    if ( *i > MCAPI_MAX_BUFFERS )
    {
        *mcapi_status = MCAPI_ERR_BUF_INVALID;
        return;
    }

    //obtain the struct proper based on the index
    bo = &bufferPool[*i];

    //Free the buffer by nullifying holder and its done
    bo->endpoint = MCAPI_NULL;

    *mcapi_status = MCAPI_SUCCESS;
}

mcapi_uint_t mcapi_pktchan_available(
    MCAPI_IN mcapi_pktchan_recv_hndl_t receive_handle,
    MCAPI_OUT mcapi_status_t* mcapi_status )
{
    //handle must be valid
    if ( !mcapi_trans_valid_pktchan_recv_handle( receive_handle ) )
    {
        *mcapi_status = MCAPI_ERR_CHAN_INVALID;
        return MCAPI_NULL;
    }

    return mcapi_chan_available( receive_handle, mcapi_status );
}

mcapi_boolean_t mcapi_trans_valid_pktchan_send_handle( mcapi_pktchan_send_hndl_t handle)
{
    if ( handle.us == MCAPI_NULL || handle.us->inited != 1 ||
    handle.us->defs == MCAPI_NULL || handle.us->defs->dir != CHAN_DIR_SEND ||
    handle.us->defs->type != MCAPI_PKT_CHAN )
    {
        return MCAPI_FALSE;
    }

    return MCAPI_TRUE;
}


mcapi_boolean_t mcapi_trans_valid_pktchan_recv_handle( mcapi_pktchan_recv_hndl_t handle)
{
    if ( handle.us == MCAPI_NULL || handle.us->inited != 1 ||
    handle.us->defs == MCAPI_NULL || handle.us->defs->dir != CHAN_DIR_RECV ||
    handle.us->defs->type != MCAPI_PKT_CHAN )
    {
        return MCAPI_FALSE;
    }

    return MCAPI_TRUE;
}
