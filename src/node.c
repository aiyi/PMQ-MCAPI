#include "node.h"
#include <string.h>
#include <errno.h>
#include <stdio.h>

//1 = is initialized, else not
static char nodeInitialized_ = 0;
//pointer to the nodeData stored
static struct nodeData nodeData_;
//all availiable request handles
static struct request_data requestPool[MCAPI_MAX_REQUESTS];
//iterator used to index above array
static unsigned int request_iter = 0;

struct nodeData* getNodeData()
{
    return &nodeData_;
}

void mcapi_initialize(MCAPI_IN mcapi_domain_t domain_id,
    MCAPI_IN mcapi_node_t node_id,
    MCAPI_IN mcapi_node_attributes_t* mcapi_node_attributes,
    MCAPI_IN mcapi_param_t* init_parameters,
    MCAPI_OUT mcapi_info_t* mcapi_info,
    MCAPI_OUT mcapi_status_t* mcapi_status) 
{
    //iterator for endpoint initialization
    unsigned int x;
    //error code returned from some system calls
    unsigned int error_code;

    //must not be null
    if ( mcapi_info == NULL )
    {
        *mcapi_status = MCAPI_ERR_PARAMETER;
        return;
    }

    #ifdef ALLOW_THREAD_SAFETY
    //create mutex, may fail because it exists already
    error_code = pthread_mutex_init(&nodeData_.mutex, NULL);

    if ( error_code != EBUSY && error_code != 0 )
    {
        //something unexpected
        perror("When creating mutex for the node");
        *mcapi_status = MCAPI_ERR_GENERAL;
        return;
    }

    //lock mutex for initialization
    if ( pthread_mutex_lock(&nodeData_.mutex) != 0 )
    {
        //something unexpected
        perror("When locking node mutex for initialize");
        *mcapi_status = MCAPI_ERR_GENERAL;
        return;
    }
    #endif

    //must not be inited already
    if ( nodeInitialized_ == 1 )
    {
        *mcapi_status = MCAPI_ERR_NODE_INITIALIZED;
        
        goto ret;
    }

    //save the ids so that they may be checked later
    nodeData_.domain_id = domain_id;
    nodeData_.node_id = node_id;

    //mark endpoints initally non-initialized, since they are
    for ( x = 0; x < ENDPOINT_COUNT; ++x )
    {
        #ifdef ALLOW_THREAD_SAFETY
        //create mutex to use with the endpoint data
        if ( pthread_mutex_init(&nodeData_.endPoints[x].mutex, NULL) != 0 )
        {
            perror("When creating mutex for an endpoint");
        }

        //lock mutex for initialization
        if ( pthread_mutex_lock(&nodeData_.endPoints[x].mutex) != 0 )
        {
            //something unexpected
            perror("When locking  endpoint mutex for initialize");
            *mcapi_status = MCAPI_ERR_GENERAL;
            return;
        }
        #endif

        //mark non-init, but also non-open and non-pending
        nodeData_.endPoints[x].inited = -1;
        nodeData_.endPoints[x].open = -1;
        nodeData_.endPoints[x].pend_open = -1;
        nodeData_.endPoints[x].pend_close = -1;
        nodeData_.endPoints[x].synced = -1;

        //the default timeout is infinite
        nodeData_.endPoints[x].time_out = MCAPI_TIMEOUT_INFINITE;

        //pair with corresponding endpoint definition
        nodeData_.endPoints[x].defs = findDef( x );

        #ifdef ALLOW_THREAD_SAFETY
        //free to use
        if ( pthread_mutex_unlock(&nodeData_.endPoints[x].mutex) != 0 )
        {
            //something unexpected
            perror("When unlocking endpoint mutex from initialize");
        }
        #endif
    }

    //initialize requests
    for ( x = 0; x < MCAPI_MAX_REQUESTS; ++x )
    {
        mcapi_request_t request = &requestPool[x];

        //mark complete and null function and data
        request->complete = MCAPI_TRUE;
        request->function = MCAPI_NULL;
        request->data = MCAPI_NULL;
        request->reserved = MCAPI_FALSE;

        #ifdef ALLOW_THREAD_SAFETY
        //create mutex to use with the request
        error_code = pthread_mutex_init(&request->mutex, NULL);

        if ( error_code != EBUSY && error_code != 0 )
        {
            perror("When creating mutex for a request");
        }
        #endif
    }

    //clear buffers for packet channels
    bufClearAll();

    //have names constructed
    initMsgqNames();

    //fill in the info
    mcapi_info->mcapi_version = MCAPI_VERSION;
    mcapi_info->organization_id = MCAPI_ORG_ID;
    mcapi_info->implementation_version = MCAPI_IMPL_VERSION;
    mcapi_info->number_of_domains = MCAPI_MAX_DOMAIN;
    mcapi_info->number_of_nodes = MCAPI_MAX_NODE;
    mcapi_info->number_of_ports = MCAPI_MAX_PORT;

    //succée
    *mcapi_status = MCAPI_SUCCESS;
    
    //mark that we are inited!
    nodeInitialized_ = 1;

    ret:
        #ifdef ALLOW_THREAD_SAFETY
        //free to use
        if ( pthread_mutex_unlock(&nodeData_.mutex) != 0 )
        {
            //something unexpected
            perror("When unlocking node mutex from initialize");
        }
        #else
        return;
        #endif
}

void mcapi_finalize( MCAPI_OUT mcapi_status_t* mcapi_status)
{
    //iterators for endpoint finalization: domain, node, port
    unsigned int x;

    #ifdef ALLOW_THREAD_SAFETY
    if ( pthread_mutex_lock(&nodeData_.mutex) != 0 )
    {
        //something unexpected
        perror("When locking node mutex for finalize");
        *mcapi_status = MCAPI_ERR_GENERAL;
        return;
    }
    #endif

    //check for initialization
    if ( nodeInitialized_ != 1 )
    {
        //no init means failure
        *mcapi_status = MCAPI_ERR_NODE_NOTINIT;
        goto ret;
    }

    //unlink and close the messagequeues
    for ( x = 0; x < ENDPOINT_COUNT; ++x )
    {
        struct endPointData* epd = &nodeData_.endPoints[x];

        //skip if not inited to begin with
        if ( epd->inited != 1 )
            continue;

        //close, so that we are no longer reserving them
        pmq_delete_epd( epd );
        pmq_delete_chan( epd, MCAPI_TRUE );
    }

    //discard requests
    for ( x = 0; x < MCAPI_MAX_REQUESTS; ++x )
    {
        mcapi_request_t request = &requestPool[x];

        //mark complete and null function and data
        request->complete = MCAPI_TRUE;
        request->function = MCAPI_NULL;
        request->data = MCAPI_NULL;
        request->reserved = MCAPI_FALSE;
    }

    //mark that we are not inited!
    nodeInitialized_ = 0;

    //succée
    *mcapi_status = MCAPI_SUCCESS;

    ret:
        #ifdef ALLOW_THREAD_SAFETY
        if ( pthread_mutex_unlock(&nodeData_.mutex) != 0 )
        {
            //something unexpected
            perror("When unlocking node mutex after finalize");
        }
        #else
        return;
        #endif
}

mcapi_boolean_t mcapi_test(
    MCAPI_IN mcapi_request_t* request,
    MCAPI_OUT size_t* size,
    MCAPI_OUT mcapi_status_t* mcapi_status )
{
    //NOTICE: in princible, should not work correctly, but it does  since
    //non-blocking calls of this implementation are limited anyway.
    mcapi_boolean_t ret_val = mcapi_wait( request, size, 0, mcapi_status );

    //fix the status, as accordingly spedicification, should work that way
    if ( *mcapi_status == MCAPI_TIMEOUT )
        *mcapi_status = MCAPI_PENDING;

    return ret_val;
}

mcapi_boolean_t mcapi_wait(
    MCAPI_IN mcapi_request_t* request, 
    MCAPI_OUT size_t* size,  
    MCAPI_IN mcapi_timeout_t timeout,
    MCAPI_OUT mcapi_status_t* mcapi_status)
{
    //becomes true, if compeleted
    mcapi_boolean_t complete;
    //how close we are to timeout
    mcapi_timeout_t ticks = 0;

    //check for initialization
    if ( mcapi_trans_initialized() == MCAPI_FALSE )
    {
        //no init means failure
        *mcapi_status = MCAPI_ERR_NODE_NOTINIT;
        return MCAPI_FALSE;
    }

    //request must be ok, this includes existing function to call
    if ( !mcapi_trans_valid_request_handle(request) ||
    (*request) == MCAPI_NULL || (*request)->function == MCAPI_NULL )
    {
        *mcapi_status = MCAPI_ERR_REQUEST_INVALID;
        return MCAPI_FALSE;
    }

    //must be valid size as well
    if ( !mcapi_trans_valid_size_param(size) )
    {
        *mcapi_status = MCAPI_ERR_PARAMETER;
        return MCAPI_FALSE;
    }

    #ifdef ALLOW_THREAD_SAFETY
    {
        int error_code;

        //no two threads may wait the same request!
        error_code = pthread_mutex_trylock( &(*request)->mutex );

        if ( error_code == EBUSY )
        {
            //simultaenous access
            *mcapi_status = MCAPI_ERR_WAIT_PENDING;
            return MCAPI_FALSE;
        }
        else if ( error_code != 0 )
        {
            //something unexpected
            perror("When checking mutex for simultaenous wait");
            *mcapi_status = MCAPI_ERR_GENERAL;
            return MCAPI_FALSE;
        }
    }
    #endif

    //inital value of completeness is taken from request handle
    complete = (*request)->complete;

    while ( complete != MCAPI_TRUE && ( timeout == MCAPI_TIMEOUT_INFINITE ||
    ticks <= timeout ) )
    {
        //sleep a millisecond between iterations
        usleep(1000);
        //call the waited-for function
        complete = (*request)->function( (*request)->data );
        //closer to time out!
        ++ticks;
    }

    if ( complete != MCAPI_TRUE )
    {
        //not complete already -> timeout
        *mcapi_status = MCAPI_TIMEOUT;
    }
    else
    {
        //the operation is complete -> ready to return with success
        *mcapi_status = MCAPI_SUCCESS;
        (*request)->complete = MCAPI_TRUE;
    }

    #ifdef ALLOW_THREAD_SAFETY
    //we no longer need the mutex either way
    if ( pthread_mutex_unlock(&(*request)->mutex) != 0 )
    {
        perror("When unlocking request mutex");
    }
    #endif

    return complete;
}

mcapi_request_t reserve_request(
    mcapi_boolean_t (*function) (void*),
    void* data )
{
    unsigned int i;
    mcapi_request_t to_ret = MCAPI_NULL;

    #ifdef ALLOW_THREAD_SAFETY
    //needs node here
    if ( pthread_mutex_lock(&nodeData_.mutex) != 0 )
    {
        perror("When locking node mutex for request free");
    }
    #endif

    //terminated after the whole request pool is gone-through
    for ( i = 0; i < MCAPI_MAX_REQUESTS; ++i )
    {
        //take the next request
        mcapi_request_t request = &requestPool[request_iter];

        //time to increase iterator
        ++request_iter;

        //if through, zero it
        if ( request_iter >= MCAPI_MAX_REQUESTS )
            request_iter = 0;

        //if free, return
        if ( request->reserved == MCAPI_FALSE )
        {
            to_ret = request;
            //fill up details
            to_ret->function = function;
            to_ret->data = data;
            to_ret->complete = MCAPI_FALSE;
            //and mark it reserved
            to_ret->reserved = MCAPI_TRUE;

            //no need for further iteration
            break;
        }
    }

    #ifdef ALLOW_THREAD_SAFETY
    //free node mutex
    if ( pthread_mutex_unlock(&nodeData_.mutex) != 0 )
    {
        perror("When unlocking node mutex from request free");
    }
    #endif

    return to_ret;
}

//macro used to simplify the below code-to-text function
#define STATUS_CASE( status ) \
    case( status ): message = strcpy(status_message, #status); break;

char* mcapi_display_status(
    MCAPI_IN mcapi_status_t status,
    MCAPI_OUT char* status_message,
    MCAPI_IN size_t size)
{
    //pointer to the buffer
    char* message;

    //switch-casing shall be simpliest way to branch between possibilities.
    //in case off unidentified code, a null is returned.
    switch (status) {
        STATUS_CASE (MCAPI_SUCCESS)
        STATUS_CASE (MCAPI_PENDING)
        STATUS_CASE (MCAPI_TIMEOUT)
        STATUS_CASE (MCAPI_ERR_PARAMETER)
        STATUS_CASE (MCAPI_ERR_DOMAIN_INVALID)
        STATUS_CASE (MCAPI_ERR_NODE_INVALID)
        STATUS_CASE (MCAPI_ERR_NODE_INITFAILED)
        STATUS_CASE (MCAPI_ERR_NODE_INITIALIZED)
        STATUS_CASE (MCAPI_ERR_NODE_NOTINIT)
        STATUS_CASE (MCAPI_ERR_NODE_FINALFAILED)
        STATUS_CASE (MCAPI_ERR_PORT_INVALID)
        STATUS_CASE (MCAPI_ERR_ENDP_INVALID)
        STATUS_CASE (MCAPI_ERR_ENDP_EXISTS)
        STATUS_CASE (MCAPI_ERR_ENDP_GET_LIMIT)
        STATUS_CASE (MCAPI_ERR_ENDP_NOTOWNER)
        STATUS_CASE (MCAPI_ERR_ENDP_REMOTE)
        STATUS_CASE (MCAPI_ERR_ATTR_INCOMPATIBLE)
        STATUS_CASE (MCAPI_ERR_ATTR_SIZE)
        STATUS_CASE (MCAPI_ERR_ATTR_NUM)
        STATUS_CASE (MCAPI_ERR_ATTR_VALUE)
        STATUS_CASE (MCAPI_ERR_ATTR_NOTSUPPORTED)
        STATUS_CASE (MCAPI_ERR_ATTR_READONLY)
        STATUS_CASE (MCAPI_ERR_MSG_SIZE)
        STATUS_CASE (MCAPI_ERR_MSG_TRUNCATED)
        STATUS_CASE (MCAPI_ERR_CHAN_OPEN)
        STATUS_CASE (MCAPI_ERR_CHAN_TYPE)
        STATUS_CASE (MCAPI_ERR_CHAN_DIRECTION)
        STATUS_CASE (MCAPI_ERR_CHAN_CONNECTED)
        STATUS_CASE (MCAPI_ERR_CHAN_OPENPENDING)
        STATUS_CASE (MCAPI_ERR_CHAN_CLOSEPENDING)
        STATUS_CASE (MCAPI_ERR_CHAN_NOTOPEN)
        STATUS_CASE (MCAPI_ERR_CHAN_INVALID)
        STATUS_CASE (MCAPI_ERR_PKT_SIZE)
        STATUS_CASE (MCAPI_ERR_TRANSMISSION)
        STATUS_CASE (MCAPI_ERR_PRIORITY)
        STATUS_CASE (MCAPI_ERR_BUF_INVALID)
        STATUS_CASE (MCAPI_ERR_MEM_LIMIT)
        STATUS_CASE (MCAPI_ERR_REQUEST_INVALID)
        STATUS_CASE (MCAPI_ERR_REQUEST_LIMIT)
        STATUS_CASE (MCAPI_ERR_REQUEST_CANCELLED)
        STATUS_CASE (MCAPI_ERR_WAIT_PENDING)
        STATUS_CASE (MCAPI_ERR_GENERAL)
        STATUS_CASE (MCAPI_STATUSCODE_END)
        default : return NULL;
    };

    //enforce policy of null-termination
    if ( size > 0 )
        message[size-1] = '\0';

    //return pointer to the buffer
    return message;
}

struct endPointData* findEpd( mcapi_domain_t domain_id, mcapi_node_t node_id,
unsigned int port_id)
{
    unsigned int i;

    for ( i = 0; i < ENDPOINT_COUNT; ++i )
    {
        struct endPointData* epd = &nodeData_.endPoints[i];
        struct endPointDef* e = epd->defs;

        //provided end point identifier match definition identifier -> is found
        if ( node_id == e->us.node_id && domain_id == e->us.domain_id &&
        port_id == e->us.port_id )
        {
            return epd;
        }
    }

    return MCAPI_NULL;
}

mcapi_domain_t mcapi_domain_id_get(
    MCAPI_OUT mcapi_status_t* mcapi_status )
{
    //check for initialization
    if ( mcapi_trans_initialized() == MCAPI_FALSE )
    {
        //no init means failure
        *mcapi_status = MCAPI_ERR_NODE_NOTINIT;
        return MCAPI_FALSE;
    }

    //return the set domain_id
    return nodeData_.domain_id;
}

mcapi_domain_t mcapi_node_id_get(
    MCAPI_OUT mcapi_status_t* mcapi_status )
{
    //check for initialization
    if ( mcapi_trans_initialized() == MCAPI_FALSE )
    {
        //no init means failure
        *mcapi_status = MCAPI_ERR_NODE_NOTINIT;
        return MCAPI_FALSE;
    }

    //return the set node_id
    return nodeData_.node_id;
}

//checks if size parameter is valid
mcapi_boolean_t mcapi_trans_valid_size_param (MCAPI_IN size_t* size)
{
    if ( size == NULL )
    {
        return MCAPI_FALSE;
    }

    return MCAPI_TRUE;
}

//checks if request parameter is valid
mcapi_boolean_t mcapi_trans_valid_request_handle
(MCAPI_IN mcapi_request_t* request)
{
    if ( request == MCAPI_NULL )
    {
        return MCAPI_FALSE;
    }

    return MCAPI_TRUE;
}

//returns true if the node is initialized, else return false
mcapi_boolean_t mcapi_trans_initialized ()
{
    #ifdef ALLOW_THREAD_SAFETY
    if ( pthread_mutex_lock(&nodeData_.mutex) != 0 )
    {
        //something unexpected
        perror("When locking node mutex to check init");
        return MCAPI_FALSE;
    }
    #endif

    //wacha we return
    mcapi_boolean_t retval = MCAPI_FALSE;

    //is true is inited
    if ( nodeInitialized_ == 1 )
    {
        retval = MCAPI_TRUE;
    }
    
    #ifdef ALLOW_THREAD_SAFETY
    if ( pthread_mutex_unlock(&nodeData_.mutex) != 0 )
    {
        //something unexpected
        perror("When unlocking node mutex from init check");
    }
    #endif

    return retval;
}

//checks if given priority is within the limits
mcapi_boolean_t mcapi_trans_valid_priority(mcapi_priority_t priority)
{
    if ( priority < 0 || priority > MCAPI_MAX_PRIORITY )
    {
        return MCAPI_FALSE;
    }

    return MCAPI_TRUE;
}

//checks if buffer parameter is valid
mcapi_boolean_t mcapi_trans_valid_buffer_param (MCAPI_IN void* buffer)
{
    if ( buffer == MCAPI_NULL )
    {
        return MCAPI_FALSE;
    }

    return MCAPI_TRUE;
}
