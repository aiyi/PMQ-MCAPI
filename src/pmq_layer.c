#include "pmq_layer.h"
#include <errno.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <stdio.h>

//how many nanos in milli
#define NANO_IN_MILLI 1000000

//how many nanos in one
#define NANO_IN_ONE 1000000000

//how many millis in one
#define MILLI_IN_ONE 1000

//how many micros in milli
#define MICRO_IN_MILLI 1000

//this macro sets current time to given time_spec and then adds given
//milliseconds to it.
#define ADD_MILLIS_TO_NOW( time_limit, timeout ) \
    clock_gettime( CLOCK_REALTIME, &time_limit ); \
    time_limit.tv_nsec += (timeout%MILLI_IN_ONE)*NANO_IN_MILLI; \
    time_limit.tv_sec += timeout/MILLI_IN_ONE + \
    time_limit.tv_nsec/NANO_IN_ONE; \
    time_limit.tv_nsec = time_limit.tv_nsec%NANO_IN_ONE;

inline mcapi_status_t pmq_send(
 	MCAPI_IN mqd_t msgq_id, 
 	MCAPI_IN void* buffer, 
 	MCAPI_IN size_t buffer_size, 
 	MCAPI_IN mcapi_priority_t priority,
    MCAPI_IN mcapi_timeout_t timeout )
{
    //the result of action
    mqd_t result = -1;

    if ( timeout == MCAPI_TIMEOUT_INFINITE )
    {
        //sending the data, priority is ascending in msgq
        result = mq_send( msgq_id, buffer, buffer_size, priority );
    }
    else
    {
        //timeout used by the posix function
        struct timespec time_limit = { 0, 0 };

        //specify timeout for the call
        ADD_MILLIS_TO_NOW( time_limit, timeout );

        //sending the data, priority is ascending in msgq
        result = mq_timedsend( msgq_id, buffer, buffer_size,
            priority, &time_limit );
    }

    //check for error
    if ( result == -1 )
    {
        if ( errno != ETIMEDOUT )
        {
            perror("mq_send");
            return MCAPI_ERR_TRANSMISSION;
        }

        //if it was a timeout, we shall return with timeout
        return MCAPI_TIMEOUT;
    }

    return MCAPI_SUCCESS;
}

inline mcapi_status_t pmq_recv(
 	MCAPI_IN mqd_t msgq_id, 
 	MCAPI_OUT void* buffer, 
 	MCAPI_IN size_t buffer_size,
 	MCAPI_OUT size_t* received_size, 
 	MCAPI_OUT mcapi_priority_t* priority,
    MCAPI_IN mcapi_timeout_t timeout )
{
    if ( timeout == MCAPI_TIMEOUT_INFINITE )
    {
        //receiving the data
        *received_size = mq_receive( msgq_id, buffer, buffer_size, priority );
    }
    else
    {
        //timeout used by posix-function
        struct timespec time_limit = { 0, 0 };

        //specify timeout for the call
        ADD_MILLIS_TO_NOW( time_limit, timeout );

        //receiving the data
        *received_size = mq_timedreceive( msgq_id, buffer,
            buffer_size, priority, &time_limit );
    }

    //check for error
    if ( *received_size == -1 )
    {
        if ( errno != ETIMEDOUT )
        {
            perror("mq_recv");
            return MCAPI_ERR_TRANSMISSION;
        }

        //if it was a timeout, we shall return with timeout
        return MCAPI_TIMEOUT;
    }

    return MCAPI_SUCCESS;
}

inline mcapi_uint_t pmq_avail(
 	MCAPI_IN mqd_t msgq_id,
    MCAPI_OUT mcapi_status_t* mcapi_status )
{
    //the retrieved attributes are obtained here for the check
    struct mq_attr uattr;

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

inline mcapi_status_t pmq_create_epd(
    MCAPI_IN mcapi_endpoint_t endpoint )
{
    //the queue created for endpoint
    mqd_t msgq_id;
    //the attributes to be set for queue
    //Blocking, maximum number of msgs, their max size and current number
    struct mq_attr attr = { 0, MAX_QUEUE_ELEMENTS, MCAPI_MAX_MESSAGE_SIZE, 0 };
    //the retrieved attributes are obtained here for the check
    struct mq_attr uattr;

    //open the queue for reception, but only create
    msgq_id = mq_open(endpoint->defs->msg_name, O_RDWR | O_CREAT | O_EXCL,
    (S_IRUSR | S_IWUSR), &attr);

    //if did not work, then its an error
    if (msgq_id == (mqd_t)-1) {
        perror("When opening msq from create");
        return MCAPI_ERR_GENERAL;
    }

    //try to open the attributes...
    if (mq_getattr(msgq_id, &uattr) == -1)
    {
        perror("When obtaining msq attributes for check");
        mq_close( msgq_id );
        mq_unlink( endpoint->defs->msg_name );
        return MCAPI_ERR_GENERAL;
    }
    
    //...and check if match
    if ( attr.mq_flags != uattr.mq_flags || attr.mq_maxmsg != uattr.mq_maxmsg
        || attr.mq_msgsize != uattr.mq_msgsize )
    {
        fprintf(stderr, "Set msq attributes do not match!\n");
        return MCAPI_ERR_GENERAL;
    }

    //now, set its messagequeue
    endpoint->msgq_id = msgq_id;

    return MCAPI_SUCCESS;
}

inline mcapi_status_t pmq_open_epd(
    MCAPI_IN mcapi_endpoint_t endpoint, 
	MCAPI_IN mcapi_timeout_t timeout )
{
    //the queue to be obtained
    mqd_t msgq_id = -1;
    //how close we are to timeout
    uint32_t ticks = 0;

    //obtaining the receiving queue, until they have created it!
    do
    {
        //sleep a millisecond between iterations
        usleep( MICRO_IN_MILLI );
        //try to open for receive only, but do not create it
        msgq_id = mq_open(endpoint->defs->msg_name, O_WRONLY );
        //closer for time out!
        ++ticks;
    }
    while ( msgq_id == -1 && ( timeout == MCAPI_TIMEOUT_INFINITE ||
    ticks < timeout ) );

    //if it was a timeout, we shall return with timeout
    if ( msgq_id == -1 )
    {
        if ( errno != ENOENT )
        {
            perror("When opening msq from get");
            return MCAPI_ERR_GENERAL;
        }

        return MCAPI_TIMEOUT;
    }

    //now, set the messagequeue
    endpoint->msgq_id = msgq_id;

    return MCAPI_SUCCESS;
}

inline void pmq_delete_epd(
    MCAPI_IN mcapi_endpoint_t endpoint )
{
    //close and unlink the queue
    mq_close( endpoint->msgq_id );
    mq_unlink( endpoint->defs->msg_name );
    //nullify the mgsq_id
    endpoint->msgq_id = -1;
}

inline mcapi_boolean_t pmq_create_chan( mcapi_endpoint_t us )
{
    //Blocking, maximum number of msgs, their max size and current number
    struct mq_attr attr = { 0, MAX_QUEUE_ELEMENTS, 0, 0 };
    //the retrieved attributes are obtained here for the check
    struct mq_attr uattr;
    //the queue created for channel
    mqd_t msgq_id;

    //yes: we may switch the size depending on type
    if ( us->defs->type == MCAPI_PKT_CHAN )
    {
        attr.mq_msgsize = MCAPI_MAX_PACKET_SIZE;
    }
    else if ( us->defs->type == MCAPI_NO_CHAN )
    {
        attr.mq_msgsize = MCAPI_MAX_MESSAGE_SIZE;
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
        mq_close( msgq_id );
        mq_unlink( us->defs->chan_name );
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

inline mcapi_boolean_t pmq_open_chan(
    MCAPI_IN mcapi_endpoint_t our_endpoint )
{
    //the queue to be obtained
    mqd_t msgq_id;

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

    return MCAPI_TRUE;
}

inline void pmq_delete_chan(
    MCAPI_IN mcapi_endpoint_t endpoint )
{
    //close and unlink the queue
    mq_close( endpoint->chan_msgq_id );
    mq_unlink( endpoint->defs->chan_name );
    //nullify the mgsq_id
    endpoint->chan_msgq_id = -1;
}
