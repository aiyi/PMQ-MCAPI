#include "channel.h"
#include "endpoint.h"
#include "node.h"
#include <stdio.h>
#include <errno.h>

mcapi_boolean_t mcapi_chan_wait_connect( void* data )
{
    return MCAPI_TRUE;
}

void mcapi_chan_connect(
    MCAPI_IN mcapi_endpoint_t  send_endpoint, 
    MCAPI_IN mcapi_endpoint_t  receive_endpoint, 
    MCAPI_OUT mcapi_request_t* request, 
    MCAPI_OUT mcapi_status_t* mcapi_status)
{
    //check for initialization
    if ( !mcapi_trans_initialized() )
    {
        //no init means failure
        *mcapi_status = MCAPI_ERR_NODE_NOTINIT;
        return;
    }

    //request must be valid
    if ( !mcapi_trans_valid_request_handle( request ) )
    {
        *mcapi_status = MCAPI_ERR_PARAMETER;
        return;
    }

    //endpoints musnt be same
    if ( send_endpoint == receive_endpoint )
    {
        *mcapi_status = MCAPI_ERR_ENDP_INVALID;
        return;
    }

    //lock sender
    LOCK_ENPOINT( send_endpoint );

    //check for valid endpoint
    if ( !mcapi_trans_valid_endpoint(send_endpoint) )
    {
        *mcapi_status = MCAPI_ERR_ENDP_INVALID;
        goto ret;
    }

    //must not be pending close
    if ( send_endpoint->pend_close == 1 )
    {
        *mcapi_status = MCAPI_ERR_CHAN_CLOSEPENDING;
        goto ret;
    }

    //unlock sender, lock receiver
    UNLOCK_ENPOINT( send_endpoint );
    LOCK_ENPOINT( receive_endpoint );

    //check for valid endpoint
    if ( !mcapi_trans_valid_endpoint(receive_endpoint) )
    {
        *mcapi_status = MCAPI_ERR_ENDP_INVALID;
        goto ret;
    }

    //must not be pending close
    if ( receive_endpoint->pend_close == 1 )
    {
        *mcapi_status = MCAPI_ERR_CHAN_CLOSEPENDING;
        goto ret;
    }

    //unlock receiver
    UNLOCK_ENPOINT( receive_endpoint );

    //fill in the request
    *request = reserve_request( mcapi_chan_wait_connect, NULL );

    //no request means there was none left
    if ( *request == MCAPI_NULL )
    {
        *mcapi_status = MCAPI_ERR_REQUEST_LIMIT;
        return;
    }

    //give pending
    *mcapi_status = MCAPI_PENDING;

    return;

    //mutexes must be unlocked even if error occurs
    ret:
        UNLOCK_ENPOINT( send_endpoint );
        UNLOCK_ENPOINT( receive_endpoint );
}

mcapi_boolean_t mcapi_chan_wait_open( void* data )
{
    //the handle for channel coms
    struct handle_type* handy;
    //the endpoint we are opening for packet communication!
    mcapi_endpoint_t our_endpoint;
    //how long message we got in bytes
    size_t mslen;
    //the buffer used for open code
    char code_buf[MCAPI_MAX_PACKET_SIZE];
    //the status code is needed by some calls
    mcapi_status_t mcapi_status;
    //the value returned
    mcapi_boolean_t ret_val;
    //direction of channel-to-close
    channel_dir dir;

    //check null
    if ( data == MCAPI_NULL )
    {
        fprintf(stderr, "Open wait provided with null data!\n");
        return MCAPI_FALSE;
    }

    //the provided data is supposed to be castable to our handle
    handy = (struct handle_type*)data;

    //we get us from handy
    our_endpoint = handy->us;

    //critical section begins here
    #ifdef ALLOW_THREAD_SAFETY
    if ( pthread_mutex_lock(&our_endpoint->mutex) != 0 )
    {
        perror("When locking endpoint mutex for open");
        return MCAPI_FALSE;
    }
    #endif

    //check for valid endpoint
    if ( !mcapi_trans_valid_endpoint(our_endpoint) )
    {
        fprintf(stderr, "Open wait provided with invalid endpoint!\n");
        ret_val = MCAPI_FALSE;
        goto ret;
    }

    //will branch based on direction, if none, it is an error
    dir = our_endpoint->defs->dir;

    if ( dir != CHAN_DIR_SEND && dir != CHAN_DIR_RECV )
    {
        fprintf(stderr, "Open wait provided with invalid directory!\n");
        ret_val = MCAPI_FALSE;
        goto ret;
    }

    //already open -> return immediately
    if ( our_endpoint->open == 1 )
    {
        ret_val = MCAPI_TRUE;
        our_endpoint->pend_open = 0;
        goto ret;
    }

    if ( dir == CHAN_DIR_SEND )
    {
        //open the channel. failure means retry
        if ( our_endpoint->chan_msgq_id == -1  &&
        pmq_open_chan_send( our_endpoint) != MCAPI_TRUE )
        {
            ret_val = MCAPI_FALSE;
            goto ret;
        }

        //and now what remains to be done is send the open code to them
        mcapi_status = pmq_send( our_endpoint->chan_msgq_id,
        code_buf, 1, 0, 0 );
    }
    else
    {
        //open the channel. failure means retry
        if ( our_endpoint->chan_msgq_id == -1  &&
        pmq_open_chan_recv( our_endpoint) != MCAPI_TRUE )
        {
            ret_val = MCAPI_FALSE;
            goto ret;
        }

        //and now what remains to be done is receiving the open code to us
        mcapi_status = pmq_recv( our_endpoint->chan_msgq_id, code_buf,
        MCAPI_MAX_PACKET_SIZE, &mslen, NULL, 0 );
    }

    //an error means failure within this iteration.
    if ( mcapi_status != MCAPI_SUCCESS )
    {
        ret_val = MCAPI_FALSE;
        goto ret;
    }

    //and mark us open
    our_endpoint->open = 1;
    our_endpoint->pend_open = 0;

    ret:
        #ifdef ALLOW_THREAD_SAFETY
        if ( pthread_mutex_unlock(&our_endpoint->mutex) != 0 )
        {
            perror("When unlocking endpoint mutex from open");
        }
        #endif

        return ret_val;
}

void mcapi_chan_open(
    MCAPI_OUT struct handle_type* handle, 
    MCAPI_IN mcapi_endpoint_t  endpoint, 
    MCAPI_OUT mcapi_request_t* request, 
    MCAPI_OUT mcapi_status_t* mcapi_status,
    MCAPI_IN channel_type expected_type,
    MCAPI_IN channel_dir expected_dir )
{
    //check for initialization
    if ( mcapi_trans_initialized() == MCAPI_FALSE )
    {
        //no init means failure
        *mcapi_status = MCAPI_ERR_NODE_NOTINIT;
        return;
    }

    //must not be null
    if ( !handle )
    {
        *mcapi_status = MCAPI_ERR_PARAMETER;
        return;
    }

    //critical section for endpoint begins here
    LOCK_ENPOINT( endpoint );

    //request must be valid
    if ( !mcapi_trans_valid_request_handle( request ) )
    {
        *mcapi_status = MCAPI_ERR_PARAMETER;
        goto ret;
    }

    //check for valid LOCAL endpoint
    if ( !mcapi_trans_valid_endpoint(endpoint) || 
    mcapi_trans_endpoint_isowner( endpoint ) == MCAPI_FALSE )
    {
        *mcapi_status = MCAPI_ERR_ENDP_INVALID;
        goto ret;
    }

    //must not be pending, one way or another!
    if ( endpoint->pend_close == 1 )
    {
        *mcapi_status = MCAPI_ERR_CHAN_CLOSEPENDING;
        goto ret;
    }

    if ( endpoint->pend_open == 1 )
    {
        *mcapi_status = MCAPI_ERR_CHAN_OPENPENDING;
        goto ret;
    }

    //must not be open already
    if ( endpoint->open == 1 )
    {
        *mcapi_status = MCAPI_ERR_CHAN_OPEN;
        goto ret;
    }

    //check for correct chan type
    if ( endpoint->defs->type != expected_type )
    {
        *mcapi_status = MCAPI_ERR_CHAN_TYPE;
        goto ret;
    }

    //check for correct chan dir
    if ( endpoint->defs->dir != expected_dir )
    {
        *mcapi_status = MCAPI_ERR_CHAN_DIRECTION;
        goto ret;
    }

    //fill in the request
    *request = reserve_request( mcapi_chan_wait_open, (void*)handle );

    //no request means there was none left
    if ( *request == MCAPI_NULL )
    {
        *mcapi_status = MCAPI_ERR_REQUEST_LIMIT;
        goto ret;
    }

    //assign handle
    handle->us = endpoint;
    //mark sync
    handle->us->synced = -1;

    //mark pending
    handle->us->pend_open = 1;
    *mcapi_status = MCAPI_PENDING;

    //mutex must be unlocked even if error occurs
    ret:
        UNLOCK_ENPOINT( endpoint );
}

mcapi_boolean_t mcapi_chan_wait_close( void* data )
{
    //the endpoint which associated channel we are closing
    mcapi_endpoint_t our_endpoint;
    //the value returned
    mcapi_boolean_t ret_val = MCAPI_FALSE;
    //true if sending, else false
    mcapi_boolean_t sending;
    //direction of channel-to-close
    channel_dir dir;

    //check null
    if ( data == MCAPI_NULL )
    {
        fprintf(stderr, "Close wait provided with null data!\n");
        return MCAPI_FALSE;
    }

    //the provided data is supposed to be castable to our endpoint
    our_endpoint = (mcapi_endpoint_t)data;

    //critical section begins here
    #ifdef ALLOW_THREAD_SAFETY
    if ( pthread_mutex_lock(&our_endpoint->mutex) != 0 )
    {
        perror("When locking endpoint mutex for close");
        return MCAPI_FALSE;
    }
    #endif

    //will branch based on direction, if none, it is an error
    dir = our_endpoint->defs->dir;

    //check for valid endpoint and direction
    if ( !mcapi_trans_valid_endpoint(our_endpoint) ||
        ( dir != CHAN_DIR_SEND && dir != CHAN_DIR_RECV ) )
    {
        fprintf(stderr, "Chan close wait provided with invalid endpoint!\n");
    }
    else
    {
        //PMQ-layer does its thing
        pmq_delete_chan( our_endpoint, dir == CHAN_DIR_RECV );

        //no longer pending
        our_endpoint->pend_close = 0;

        ret_val = MCAPI_TRUE;
    }

    #ifdef ALLOW_THREAD_SAFETY
    if ( pthread_mutex_unlock(&our_endpoint->mutex) != 0 )
    {
        perror("When unlocking endpoint mutex from close");
    }
    #endif

    return ret_val;
}

void mcapi_chan_close(
    MCAPI_IN struct handle_type handle, 
    MCAPI_OUT mcapi_request_t* request, 
    MCAPI_OUT mcapi_status_t* mcapi_status,
    MCAPI_IN channel_type expected_type,
    MCAPI_IN channel_dir expected_dir)
{
    //check for initialization
    if ( !mcapi_trans_initialized() )
    {
        //no init means failure
        *mcapi_status = MCAPI_ERR_NODE_NOTINIT;
        return;
    }

    //request must be valid
    if ( !mcapi_trans_valid_request_handle( request ) )
    {
        *mcapi_status = MCAPI_ERR_PARAMETER;
        return;
    }

    //critical section for channel
    LOCK_CHANNEL( handle );

    //handle must be valid
    if ( !mcapi_trans_valid_endpoint( handle.us ) )
    {
        *mcapi_status = MCAPI_ERR_CHAN_INVALID;
        goto ret;
    }

    //cant close if not open
    if ( handle.us->open != 1 )
    {
        *mcapi_status = MCAPI_ERR_CHAN_NOTOPEN;
        goto ret;
    }

    //check for correct chan type
    if ( handle.us->defs->type != expected_type )
    {
        *mcapi_status = MCAPI_ERR_CHAN_TYPE;
        goto ret;
    }

    //check for correct chan dir
    if ( handle.us->defs->dir != expected_dir )
    {
        *mcapi_status = MCAPI_ERR_CHAN_DIRECTION;
        goto ret;
    }

    //fill in the request
    *request = reserve_request( mcapi_chan_wait_close, (void*)handle.us );

    //no request means there was none left
    if ( *request == MCAPI_NULL )
    {
        *mcapi_status = MCAPI_ERR_REQUEST_LIMIT;
        goto ret;
    }

    //closed
    handle.us->open = 0;
    handle.us->pend_close = 1;
    *mcapi_status = MCAPI_PENDING;

    //mutex must be unlocked even if error occurs
    ret:
        UNLOCK_CHANNEL( handle );
}
