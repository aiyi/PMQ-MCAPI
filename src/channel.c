#include "channel.h"
#include <stdio.h>

mcapi_boolean_t mcapi_chan_wait_connect( void* data )
{
    //our original.
    //NOTICE: "us" is misleading, as it is not nescessarily from our node.
    mcapi_endpoint_t us;
    //both endpoints must be accessible before we may consider them connected
    mcapi_endpoint_t our_clone;
    mcapi_endpoint_t their_clone;
    struct endPointID usID;
    struct endPointID themID;
    //while not really observed, status must be provided as parameter
    mcapi_status_t status;

    //the provided data is supposed to be castable to our endpoint
    us = (mcapi_endpoint_t)data;

    //check for valid endpoint
    if ( !mcapi_trans_valid_endpoint( us ) )
    {
        fprintf(stderr, "Chan connect wait provided with invalid endpoint!\n");
        return MCAPI_FALSE;
    }

    //obtain identifiers of both ends from defs.
    usID = us->defs->us;
    themID = us->defs->them;
    
    //first us
    our_clone = mcapi_endpoint_get( usID.domain_id, usID.node_id,
    usID.port_id, 0, &status );

    if ( our_clone == MCAPI_NULL )
    {
        return MCAPI_FALSE;
    }
    
    //then them
    their_clone = mcapi_endpoint_get( themID.domain_id, themID.node_id,
    themID.port_id, 0, &status );

    if ( their_clone == MCAPI_NULL )
    {
        return MCAPI_FALSE;
    }

    //pmq-layer handles the rest
    return pmq_create_chan( us );
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

    //must not be null
    if ( !mcapi_trans_valid_request_handle( request ) )
    {
        *mcapi_status = MCAPI_ERR_PARAMETER;
        return;
    }

    //check for valid endpoints
    if ( !mcapi_trans_valid_endpoints(send_endpoint,receive_endpoint) )
    {
        *mcapi_status = MCAPI_ERR_ENDP_INVALID;
        return;
    }

    //endpoints musnt be same
    if ( send_endpoint == receive_endpoint )
    {
        *mcapi_status = MCAPI_ERR_ENDP_INVALID;
        return;
    }

    //must not be pending close
    if ( send_endpoint->pend_close == 1 || receive_endpoint->pend_close == 1 )
    {
        *mcapi_status = MCAPI_ERR_CHAN_CLOSEPENDING;
        return;
    }

    //check they are defined to same channel
    if ( !sameID( send_endpoint->defs->us, receive_endpoint->defs->them ) ||
    !sameID( send_endpoint->defs->them, receive_endpoint->defs->us ) )
    {
        *mcapi_status = MCAPI_ERR_ENDP_INVALID;
        return;
    }

    //fill in the request and give pend.
    request->function = mcapi_chan_wait_connect;
    request->data = (void*)send_endpoint;
    
    *mcapi_status = MCAPI_PENDING;
}

mcapi_boolean_t mcapi_chan_wait_open( void* data )
{
    //the handle for channel coms
    struct handle_type* handy;
    //the endpoint we are opening for packet communication!
    mcapi_endpoint_t our_endpoint;
    //how long message we got in bytes
    size_t mslen;
    //the buffer used to RECEIVE the open code
    char recv_buf[MCAPI_MAX_MSG_SIZE];
    //the status code is needed by some calls
    mcapi_status_t status;

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

    //check for valid endpoint
    if ( !mcapi_trans_valid_endpoint(our_endpoint) )
    {
        fprintf(stderr, "Open wait provided with invalid endpoint!\n");
        return MCAPI_FALSE;
    }

    //already open -> return immediately
    if ( our_endpoint->open == 1 )
        return MCAPI_TRUE;

    //open the channel. failure means retry
    if ( pmq_open_chan( our_endpoint) != MCAPI_TRUE )
    {
        return MCAPI_FALSE;
    }

    //skip if already sent
    if ( our_endpoint->synced != 1 )
    {
        //the other endpoint of channel open
        mcapi_endpoint_t their_endpoint;
        //the indentifier of the oppoposing side
        mcapi_domain_t domain_id = our_endpoint->defs->them.domain_id;
     	mcapi_node_t node_id = our_endpoint->defs->them.node_id;
     	mcapi_port_t port_id = our_endpoint->defs->them.port_id;
        //the buffer used to SEND the open code
        char send_buf[] = CODE_OPEN_CONNECTED;

        //wait only for fixed amount of time so that this is "non-blocking"
        their_endpoint = mcapi_endpoint_get( domain_id, node_id, port_id,
        0, &status );

        //failure means ERROR as it should exist already!
        if ( status != MCAPI_SUCCESS )
        {
            fprintf(stderr, "The other end point did not exist after \
            connect!\n");
            return MCAPI_FALSE;
        }

        //now we can send the open code!
        //the priority is max+2, so that we are sure it is ahead
        //any messages, packets and scalars
        //NOTICE: the messages are now used as it serves in configures!
        status = pmq_send( their_endpoint->msgq_id, send_buf,
        sizeof(send_buf), MCAPI_MAX_PRIORITY+2, 0 );

        //failure means once again error. should never happen.
        if ( status != MCAPI_SUCCESS )
        {
            perror("mq_send opening channel");

            return MCAPI_FALSE;
        }

        //mark sync
        our_endpoint->synced = 1;
    }

    //and now what remains to be done is receiving the open code to us
    //NOTICE: the messagequeue is now used as it serves in configures!
    status = pmq_recv( our_endpoint->msgq_id, recv_buf, MCAPI_MAX_MSG_SIZE, 
    &mslen, NULL, 0 );

    //an error means failure within this iteration.
    if ( status != MCAPI_SUCCESS )
    {
        return MCAPI_FALSE;
    }

    //check for the code
    if ( strcmp( CODE_OPEN_CONNECTED, recv_buf ) != 0 )
    {
        //some random turf: we shall discard it
        fprintf(stderr, "The channel connect received invalid message!\n");
        return MCAPI_FALSE;
    }

    //and mark us open
    our_endpoint->open = 1;
    our_endpoint->pend_open = 0;

    return MCAPI_TRUE;
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

    //must not be null
    if ( !request )
    {
        *mcapi_status = MCAPI_ERR_PARAMETER;
        return;
    }

    //check for valid endpoint
    if ( !mcapi_trans_valid_endpoint(endpoint) )
    {
        *mcapi_status = MCAPI_ERR_ENDP_INVALID;
        return;
    }

    //must not be pending, one way or another!
    if ( endpoint->pend_close == 1 )
    {
        *mcapi_status = MCAPI_ERR_CHAN_CLOSEPENDING;
        return;
    }

    if ( endpoint->pend_open == 1 )
    {
        *mcapi_status = MCAPI_ERR_CHAN_OPENPENDING;
        return;
    }

    //must not be open already
    if ( endpoint->open == 1 )
    {
        *mcapi_status = MCAPI_ERR_CHAN_OPEN;
        return;
    }

    //check for correct chan type
    if ( endpoint->defs->type != expected_type )
    {
        *mcapi_status = MCAPI_ERR_CHAN_TYPE;
        return;
    }

    //check for correct chan dir
    if ( endpoint->defs->dir != expected_dir )
    {
        *mcapi_status = MCAPI_ERR_CHAN_DIRECTION;
        return;
    }

    //assign handle
    handle->us = endpoint;
    //mark sync
    handle->us->synced = -1;

    //fill in the request
    request->function = mcapi_chan_wait_open;
    request->data = (void*)handle;

    *mcapi_status = MCAPI_PENDING;
    //mark pending
    handle->us->pend_open = 1;
}

mcapi_boolean_t mcapi_chan_wait_close( void* data )
{
    //the endpoint which associated channel we are closing
    mcapi_endpoint_t our_endpoint;
    //how long message we got in bytes: significanse is in error messages
    size_t mslen;
    //the buffer used to RECEIVE the open code
    char recv_buf[MCAPI_MAX_MSG_SIZE];
    //the status code is needed by some calls
    mcapi_status_t status;

    //the provided data is supposed to be castable to our endpoint
    our_endpoint = (mcapi_endpoint_t)data;

    //check for valid endpoint
    if ( !mcapi_trans_valid_endpoint(our_endpoint) )
    {
        fprintf(stderr, "Chan close wait provided with invalid endpoint!\n");
        return MCAPI_FALSE;
    }

    //already closed -> return immediately
    if ( our_endpoint->open != 1 )
        return MCAPI_TRUE;

    //skip if already sent
    if ( our_endpoint->synced != 1 )
    {
        //and now send the syncronation message to the other endpoint!

        //the other endpoint of channel open
        mcapi_endpoint_t their_endpoint;
        //the indentifier of the oppoposing side
        mcapi_domain_t domain_id = our_endpoint->defs->them.domain_id;
     	mcapi_node_t node_id = our_endpoint->defs->them.node_id;
     	mcapi_port_t port_id = our_endpoint->defs->them.port_id; 
        //the buffer used to SEND the close code
        char send_buf[] = CODE_CLOSE_CONNECTED;

        //wait only for fixed amount of time so that this is "non-blocking"
        their_endpoint = mcapi_endpoint_get( domain_id, node_id, port_id,
        0, &status );

        //failure means ERROR as it should exist already!
        if ( status != MCAPI_SUCCESS )
        {
            fprintf(stderr, "The other end point did not exist after \
            close!\n");
            return MCAPI_FALSE;
        }

        //now we can send the open code!
        //the priority is max+2, so that we are sure it is ahead
        //any messages, packets and scalars
        //NOTICE: the messages are now used as it serves in configures!
        status = pmq_send( their_endpoint->msgq_id, send_buf,
        sizeof(send_buf), MCAPI_MAX_PRIORITY+2, 0 );

        //failure means once again error. should never happen.
        if ( status != MCAPI_SUCCESS )
        {
            perror("mq_send opening channel");

            return MCAPI_FALSE;
        }

        //mark sync
        our_endpoint->synced = 1;
    }

    //and now what remains to be done is receiving the open code to us
    //NOTICE: the messagequeue is now used as it serves in configures!
    status = pmq_recv( our_endpoint->msgq_id, recv_buf, MCAPI_MAX_MSG_SIZE, 
    &mslen, NULL, 0 );

    //an error means failure within this iteration.
    if ( status != MCAPI_SUCCESS )
    {
        return MCAPI_FALSE;
    }

    //check for the code
    if ( strcmp( CODE_CLOSE_CONNECTED, recv_buf ) != 0 )
    {
        //some random turf: we shall discard it
        fprintf(stderr, "The channel close received invalid message!\n" );
        return MCAPI_FALSE;
    }

    //yeah, its done now.
    our_endpoint->pend_close = 0;
    our_endpoint->open = 0;

    return MCAPI_TRUE;
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

    //request musnt be null
    if ( !request )
    {
        *mcapi_status = MCAPI_ERR_PARAMETER;
        return;
    }

    //handle must be valid
    if ( !mcapi_trans_valid_endpoint( handle.us ) )
    {
        *mcapi_status = MCAPI_ERR_CHAN_INVALID;
        return;
    }

    //mustn be pending, one wy or another!
    if ( handle.us->pend_close == 1 )
    {
        *mcapi_status = MCAPI_ERR_CHAN_CLOSEPENDING;
        return;
    }

    //must not be pending open
    if ( handle.us->pend_open == 1 )
    {
        *mcapi_status = MCAPI_ERR_CHAN_OPENPENDING;
        return;
    }

    //cant close what aint open, eh?
    if ( handle.us->open != 1 )
    {
        *mcapi_status = MCAPI_ERR_CHAN_NOTOPEN;
        return;
    }

    //check for correct chan type
    if ( handle.us->defs->type != expected_type )
    {
        *mcapi_status = MCAPI_ERR_CHAN_TYPE;
        return;
    }

    //check for correct chan dir
    if ( handle.us->defs->dir != expected_dir )
    {
        *mcapi_status = MCAPI_ERR_CHAN_DIRECTION;
        return;
    }

    //PMQ-layer does its thing
    pmq_delete_chan( handle.us );
    //mark sync
    handle.us->synced = -1;

    //yeah, pending close
    handle.us->pend_close = 1;
    *mcapi_status = MCAPI_PENDING;

    //fill in the request
    request->function = mcapi_chan_wait_close;
    request->data = (void*)handle.us;
}

inline mcapi_uint_t mcapi_chan_available(
    MCAPI_IN mcapi_pktchan_recv_hndl_t receive_handle,
    MCAPI_OUT mcapi_status_t* mcapi_status )
{
    //check for initialization
    if ( mcapi_trans_initialized() == MCAPI_FALSE )
    {
        //no init means failure
        *mcapi_status = MCAPI_ERR_NODE_NOTINIT;
        return MCAPI_NULL;
    }

    return pmq_avail( receive_handle.us->chan_msgq_id, mcapi_status );
}
