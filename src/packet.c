//This module has functions used in packet MCAPI-communication.
#include <stdio.h>
#include <errno.h>
#include "channel.h"
#include "endpoint.h"

//keyvalue-pair used in buffer handling
struct bufObject
{
    //the endpoint currently using the buffer: MCAPI_NULL if unused
    mcapi_endpoint_t endpoint;
    //the data contained in the buffer
    unsigned char data[MCAPI_MAX_PACKET_SIZE+sizeof(unsigned int)];
};

//the all buffers within the use of this node
static struct bufObject bufferPool[MCAPI_MAX_BUFFERS];

//the place of next unchecked buffer
static unsigned int buf_iter = 0;

#ifdef ALLOW_THREAD_SAFETY
//mutex used to protect the buffer data
static pthread_mutex_t buf_mutex;
#endif

//sets keys of all buffers to MCAPI_NULL
//must be called on init, in other words before buffer use
void bufClearAll()
{
    //iterator
    unsigned int i;
    //error code returned from some system calls
    unsigned int error_code;

    #ifdef ALLOW_THREAD_SAFETY
    //create mutex, may fail because it exists already
    error_code = pthread_mutex_init(&buf_mutex, NULL);

    if ( error_code != EBUSY && error_code != 0 )
    {
        //something unexpected
        perror("When creating mutex for the buffer");
        return;
    }

    //lock mutex for initialization
    if ( pthread_mutex_lock(&buf_mutex) != 0 )
    {
        //something unexpected
        perror("When locking buffer mutex for initialize");
        return;
    }
    #endif

    //iterating through the whole buffer pool
    for ( i = 0; i < MCAPI_MAX_BUFFERS; ++i )
    {
        //take the next buffer
        struct bufObject* bo = &bufferPool[i];
        //and clear
        bo->endpoint = MCAPI_NULL;
        //set "us" at the end of the buffer
        bo->data[MCAPI_MAX_PACKET_SIZE] = i;
    }

    #ifdef ALLOW_THREAD_SAFETY
    //free to use
    if ( pthread_mutex_unlock(&buf_mutex) != 0 )
    {
        //something unexpected
        perror("When unlocking buffer mutex from initialize");
    }
    #endif
}

//returns and reserves next unused buffer or MCAPI_NULL, if none found
//reserves for the given endpoint.
//Within threaded build, will reiterate until timeout
struct bufObject* bufFindEmpty( mcapi_endpoint_t endpoint,
    mcapi_timeout_t* timeout )
{
    unsigned int i; //iterator
    struct bufObject* to_ret = MCAPI_NULL; //return value

    #ifdef ALLOW_THREAD_SAFETY
    do 
    {
        //lock mutex
        if ( pthread_mutex_lock(&buf_mutex) != 0 )
        {
            //something unexpected
            perror("When locking buffer mutex for reservation");
            return;
        }
    #endif

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

            //if free, mark and break break
            if ( bo->endpoint == MCAPI_NULL )
            {
                bo->endpoint = endpoint;
                to_ret = bo;
                break;
            }
        }

    #ifdef ALLOW_THREAD_SAFETY
        //closer to timeout!
        if ( *timeout != MCAPI_TIMEOUT_INFINITE )
        {
            *timeout = *timeout - 1;
        }

        //free to use
        if ( pthread_mutex_unlock(&buf_mutex) != 0 )
        {
            //something unexpected
            perror("When unlocking buffer mutex from reservation");
        }

        if ( to_ret == MCAPI_NULL )
        {
            //no luck: sleep mllisecond
            usleep(1000);
        }
        else
        {
            //success: break the loop
            break;
        }
    }
    while ( timeout > 0 );
    #endif

    return to_ret;
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
    if ( size > MCAPI_MAX_PACKET_SIZE )
    {
        *mcapi_status = MCAPI_ERR_PKT_SIZE; 
        return;
    }

    //buffer must be usable
    if ( !mcapi_trans_valid_buffer_param(buffer) || size < 0 )
    {
        *mcapi_status = MCAPI_ERR_PARAMETER;
        return;
    }

    //critical section for channel begins here
    LOCK_CHANNEL( send_handle );

    //handle must be valid send handle
    if ( !mcapi_trans_valid_pktchan_send_handle(send_handle) )
    {
        *mcapi_status = MCAPI_ERR_CHAN_INVALID;
        goto ret;
    }

    //must be open
    if ( send_handle.us->open != 1 )
    {
        *mcapi_status = MCAPI_ERR_CHAN_INVALID;
        goto ret;
    }

    //POSIX will handle the rest, timeout of sending endpoint is used
    //priority is fixed to that expected of channel traffic
    *mcapi_status = pmq_send( send_handle.us->chan_msgq_id, buffer,
    size, MCAPI_MAX_PRIORITY+1, send_handle.us->time_out );

    //mutex must be unlocked even if error occurs
    ret:
        UNLOCK_CHANNEL( send_handle );
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
    //the timeout used
    mcapi_timeout_t timeout;

    //check for initialization
    if ( !mcapi_trans_initialized() )
    {
        //no init means failure
        *mcapi_status = MCAPI_ERR_NODE_NOTINIT;
        return;
    }

    //critical section for channel begins here
    LOCK_CHANNEL( receive_handle );

    //handle must be valid
    if ( !mcapi_trans_valid_pktchan_recv_handle(receive_handle) )
    {
        *mcapi_status = MCAPI_ERR_CHAN_INVALID;
        goto ret;
    }
  
    //buffer must be usable
    if ( !mcapi_trans_valid_buffer_param(buffer))
    {
        *mcapi_status = MCAPI_ERR_PARAMETER;
        goto ret;
    }

    //must be open
    if ( receive_handle.us->open != 1 )
    {
        *mcapi_status = MCAPI_ERR_CHAN_INVALID;
        goto ret;
    }

    //fill in timeout
    timeout = receive_handle.us->time_out;

    //reserve buffer AFTER the initial pit holes
    bo = bufFindEmpty( receive_handle.us, &timeout );

    //Within threaded implementation, reaching here will mean timeout
    //With no threads, no one can free buffer for us, so works either way
    if ( bo == MCAPI_NULL )
    {
        *mcapi_status = MCAPI_TIMEOUT;
        goto ret;
    }

    //POSIX will handle the recv, timeout of receiving endpoint is used
    *mcapi_status = pmq_recv( receive_handle.us->chan_msgq_id, bo->data,
    MCAPI_MAX_PACKET_SIZE, received_size, &msg_prio, timeout );

    if ( *mcapi_status == MCAPI_SUCCESS )
    {
        //succée: pass the buffer to caller
        *buffer = &bo->data;
    }
    else 
    {
        //failure: release the buffer
        #ifdef ALLOW_THREAD_SAFETY
        //lock mutex
        if ( pthread_mutex_lock(&buf_mutex) != 0 )
        {
            //something unexpected
            perror("When locking buffer mutex for un-reserve");
            return;
        }
        #endif

        bo->endpoint = MCAPI_NULL;

        #ifdef ALLOW_THREAD_SAFETY
        //free to use
        if ( pthread_mutex_unlock(&buf_mutex) != 0 )
        {
            //something unexpected
            perror("When unlocking from un-reserve");
        }
        #endif
    }

    //mutex must be unlocked even if error occurs
    ret:
        UNLOCK_CHANNEL( receive_handle );
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

    #ifdef ALLOW_THREAD_SAFETY
    //lock mutex
    if ( pthread_mutex_lock(&buf_mutex) != 0 )
    {
        //something unexpected
        perror("When locking buffer mutex for free");
        return;
    }
    #endif

    //magic: we get the struct index from the end of buffer
    loc = buffer + MCAPI_MAX_PACKET_SIZE;
    i = (unsigned int*)loc;

    //out of bound means an error
    if ( *i > MCAPI_MAX_BUFFERS )
    {
        *mcapi_status = MCAPI_ERR_BUF_INVALID;
    }
    else
    {
        //obtain the struct proper based on the index
        bo = &bufferPool[*i];

        //Free the buffer by nullifying holder and its done
        bo->endpoint = MCAPI_NULL;

        *mcapi_status = MCAPI_SUCCESS;
    }

    #ifdef ALLOW_THREAD_SAFETY
    //free to use
    if ( pthread_mutex_unlock(&buf_mutex) != 0 )
    {
        //something unexpected
        perror("When unlocking from free");
    }
    #endif
}

mcapi_uint_t mcapi_pktchan_available(
    MCAPI_IN mcapi_pktchan_recv_hndl_t receive_handle,
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

    //must be locked for operation
    LOCK_CHANNEL( receive_handle );

    //handle must be valid
    if ( !mcapi_trans_valid_pktchan_recv_handle( receive_handle ) )
    {
        *mcapi_status = MCAPI_ERR_CHAN_INVALID;
    }
    else
    {
        to_ret = pmq_avail( receive_handle.us->chan_msgq_id, mcapi_status );
    }

    //release
    UNLOCK_CHANNEL( receive_handle );

    return to_ret;
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
