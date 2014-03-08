#include "channel.h"
#include <errno.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/fcntl.h>

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
    //Blocking, maximum number of msgs, their max size and current number
    struct mq_attr attr = { 0, MAX_QUEUE_ELEMENTS, 0, 0 };
    //the retrieved attributes are obtained here for the check
    struct mq_attr uattr;
    //the queue created for channel
    mqd_t msgq_id;

    //the provided data is supposed to be castable to our endpoint
    us = (mcapi_endpoint_t)data;

    //check for valid endpoint
    if ( !mcapi_trans_valid_endpoint(us) )
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

    //delete the existing queue. does not work if someone has opened, though
    //NOTICE: errors not checked since possible errors do not concern us.
    mq_unlink( us->defs->chan_name );

    //yes: we may switch the size depending on type
    if ( us->defs->type == MCAPI_PKT_CHAN )
    {
        attr.mq_msgsize = MCAPI_MAX_PKT_SIZE;
    }
    else if ( us->defs->type == MCAPI_NO_CHAN )
    {
        attr.mq_msgsize = MCAPI_MAX_MSG_SIZE;
    }
    else if ( us->defs->type == MCAPI_SCL_CHAN )
    {
        //in case of scalar channel, the channel defined size is used
        attr.mq_msgsize = us->defs->scalar_size;
    }
    else
    {
        fprintf(stderr, "Channel connect provided with null chan type!\n");
        return MCAPI_FALSE;
    }
    
    //try to open the message queue to be used as channel
    msgq_id = mq_open(us->defs->chan_name, O_RDWR | O_CREAT | O_EXCL,
    (S_IRUSR | S_IWUSR), &attr);

    //if did not work, then its an error
    if ( msgq_id == -1 )
    {
        perror("When opening msq from connect");
        return MCAPI_FALSE;
    }

    //try to open the attributes...
    if (mq_getattr(msgq_id, &uattr) == -1)
    {
        perror("When obtaining channel msq attributes for check");
        return MCAPI_FALSE;
    }

    //Close this end of the queue so that no residual is left in any case.
    //In the other hand, if this node needs it later, it shall reopen it
    //at open-calls.
    mq_close( msgq_id );
    
    //...and check if match
    if ( attr.mq_flags != uattr.mq_flags || attr.mq_maxmsg != uattr.mq_maxmsg
        || attr.mq_msgsize != uattr.mq_msgsize )
    {
        fprintf(stderr, "Set channel sq attributes do not match!\n");
        return MCAPI_FALSE;
    }

    //done
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

    //if it already exists, it means we are connected already!
    if ( mq_open( send_endpoint->defs->chan_name, O_WRONLY ) != -1 )
    {
        *mcapi_status = MCAPI_ERR_CHAN_CONNECTED;
        return;
    }

    //fill in the request and give pend.
    request->function = mcapi_chan_wait_connect;
    request->data = (void*)send_endpoint;
    
    *mcapi_status = MCAPI_PENDING;
}

mcapi_boolean_t mcapi_chan_wait_open( void* data )
{
    //the queue to be obtained
    mqd_t msgq_id;
    //the handle for channel coms
    struct handle_type* handy;
    //the endpoint we are opening for packet communication!
    mcapi_endpoint_t our_endpoint;
    //how long message we got in bytes
    size_t mslen;
    //the buffer used to RECEIVE the open code
    char recv_buf[MCAPI_MAX_PKT_SIZE];
    //timeout used by operations: actually no time at all
    struct timespec time_limit = { 0, 0 };

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

    //no -1 means we already has an messagequeue
    if ( our_endpoint->chan_msgq_id == -1 )
    {
        //try to open, but do not create it
        msgq_id = mq_open(our_endpoint->defs->chan_name, O_RDWR );

        //failure means retry
        if ( msgq_id == -1 )
        {
            //print only if not non-existing, as we expect it.
            if ( errno != ENOENT )
                perror( "when obtaining channel queue");

            return MCAPI_FALSE;
        }

        //success! assign channel to handle
        our_endpoint->chan_msgq_id = msgq_id;
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
        //the status message for endpoint retrieval
        mcapi_status_t get_status;
        //the buffer used to SEND the open code
        char send_buf[] = CODE_OPEN_CONNECTED;

        //wait only for fixed amount of time so that this is "non-blocking"
        their_endpoint = mcapi_endpoint_get( domain_id, node_id, port_id,
        0, &get_status );

        //failure means ERROR as it should exist already!
        if ( get_status != MCAPI_SUCCESS )
        {
            fprintf(stderr, "The other end point did not exist after \
            connect!\n");
            return MCAPI_FALSE;
        }

        //now we can send the open code!
        //the priority is max+2, so that we are sure it is ahead
        //any messages, packets and scalars
        //NOTICE: the messagequeue is now used as it serves in configures!
        mslen = mq_timedsend( their_endpoint->msgq_id, send_buf,
        sizeof(CODE_OPEN_CONNECTED), MCAPI_MAX_PRIORITY+2, &time_limit );

        //an error? Time out also counts as it should not happen!
        if ( mslen == -1 )
        {
            perror("mq_send opening channel");

            return MCAPI_FALSE;
        }

        //mark sync
        our_endpoint->synced = 1;
    }

    //and now what remains to be done is receiving the open code to us
    //NOTICE: the messagequeue is now used as it serves in configures!
    mslen = mq_timedreceive(our_endpoint->msgq_id, recv_buf,
        MCAPI_MAX_PKT_SIZE, NULL, &time_limit );

    //an error?
    if ( mslen == -1 )
    {
        //print error only if not timeout, as timeout is expected
        if ( errno != ETIMEDOUT )
            perror("mq_receive chan open");

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

    //mustn be pending, one way or another!
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
    //nullify the mgsq_id
    handle->us->chan_msgq_id = -1;
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
    //timeout used by operations: actually no time at all
    struct timespec time_limit = { 0, 0 };
    //the endpoint which associated channel we are closing
    mcapi_endpoint_t our_endpoint;
    //how long message we got in bytes: significanse is in error messages
    size_t mslen;
    //the buffer used to RECEIVE the open code
    char recv_buf[MCAPI_MAX_PKT_SIZE];

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
        //the status message for endpoint retrieval
        mcapi_status_t get_status;
        //the buffer used to SEND the close code
        char send_buf[] = CODE_CLOSE_CONNECTED;

        //wait only for fixed amount of time so that this is "non-blocking"
        their_endpoint = mcapi_endpoint_get( domain_id, node_id, port_id,
        0, &get_status );

        //failure means ERROR as it should exist already!
        if ( get_status != MCAPI_SUCCESS )
        {
            fprintf(stderr, "The other end point did not exist after \
            connect!\n");
            return MCAPI_FALSE;
        }

        //now we can send the open code!
        //the priority is max+2, so that we are sure it is ahead
        //any messages, packets and scalars
        //NOTICE: the messagequeue is now used as it serves in configures!
        mslen = mq_timedsend( their_endpoint->msgq_id, send_buf,
        sizeof(CODE_CLOSE_CONNECTED), MCAPI_MAX_PRIORITY+2, &time_limit );

        //an error? Time out also counts as it should not happen!
        if ( mslen == -1 )
        {
            perror("mq_send closing channel");

            return MCAPI_FALSE;
        }

        //mark sync
        our_endpoint->synced = 1;
    }

    //and now what remains to be done is receiving the close code to us
    //NOTICE: the messagequeue is now used as it serves in configures!
    mslen = mq_timedreceive(our_endpoint->msgq_id, recv_buf,
        MCAPI_MAX_PKT_SIZE, NULL, &time_limit );

    //an error?
    if ( mslen == -1 )
    {
        //print error only if not timeout, as timeout is expected
        if ( errno != ETIMEDOUT )
            perror("mq_receive receive open");

        return MCAPI_FALSE;
    }

    //check for the code
    if ( strcmp( CODE_CLOSE_CONNECTED, recv_buf ) != 0 )
    {
        //some random turf: we shall discard it
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

    //close and unlink channel messagequeue
    mq_close( handle.us->chan_msgq_id );
    mq_unlink( handle.us->defs->chan_name );
    //nullify the mgsq_id
    handle.us->chan_msgq_id = -1;
    //mark sync
    handle.us->synced = -1;

    //yeah, pending close
    handle.us->pend_close = 1;
    *mcapi_status = MCAPI_PENDING;

    //fill in the request
    request->function = mcapi_chan_wait_close;
    request->data = (void*)handle.us;
}

mcapi_uint_t mcapi_chan_available(
    MCAPI_IN mcapi_pktchan_recv_hndl_t receive_handle,
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

    //take id from endpoint
    msgq_id = receive_handle.us->chan_msgq_id;

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
