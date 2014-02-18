#include "node.h"
#include <linux/limits.h>
#include <string.h>

//1 = is initialized, else not
static char nodeInitialized_ = 0;
//pointer to the nodeData stored
static struct nodeData nodeData_;

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
    //iterators for endpoint initialization: domain, node, port
    unsigned int x;
    unsigned int y;
    unsigned int z;

    //check for init
    if ( mcapi_trans_initialized() == MCAPI_TRUE )
    {
        *mcapi_status = MCAPI_ERR_NODE_INITIALIZED;
        return;
    }

    //musnt be null
    if ( mcapi_info == NULL )
    {
        *mcapi_status = MCAPI_ERR_PARAMETER;
        return;
    }

    //check for valid node
    if ( mcapi_trans_valid_node( node_id ) == MCAPI_FALSE ) 
    {
        *mcapi_status = MCAPI_ERR_NODE_INVALID;
        return;
    }

    //check for valid domain
    if ( mcapi_trans_valid_domain( domain_id ) == MCAPI_FALSE )
    {
        *mcapi_status = MCAPI_ERR_DOMAIN_INVALID;
        return;
    }

    //save the ids so that they may be checked later
    nodeData_.domain_id = domain_id;
    nodeData_.node_id = node_id;

    //mark endpoints initally non-initialized, since they are
    for ( x = 0; x < MCA_MAX_DOMAINS; ++x )
    {
        for ( y = 0; y < MCA_MAX_NODES; ++y )
        {
            for ( z = 0; z < MCAPI_MAX_ENDPOINTS; ++z )
            {
                //mark non-init, but also non-open and non-pending
                nodeData_.endPoints[x][y][z].inited = -1;
                nodeData_.endPoints[x][y][z].open = -1;
                nodeData_.endPoints[x][y][z].pend_open = -1;
                nodeData_.endPoints[x][y][z].pend_close = -1;

                //initially, also messagequeues must be non-inited
                nodeData_.endPoints[x][y][z].msgq_id = -1;
                nodeData_.endPoints[x][y][z].chan_msgq_id = -1;

                //the default timeout is infinite
                nodeData_.endPoints[x][y][z].time_out = MCAPI_TIMEOUT_INFINITE;

                //find the endpoint definition for this one
                nodeData_.endPoints[x][y][z].defs =
                findDef( x, y, z );
            }
        }
    }

    //clear buffers for packet channels
    bufClearAll();

    //have names constructed
    initMsgqNames();

    //fill in the info
    mcapi_info->mcapi_version = MCAPI_VERSION;
    mcapi_info->organization_id = MCAPI_ORG_ID;
    mcapi_info->implementation_version = MCAPI_IMPL_VERSION;
    mcapi_info->number_of_domains = MCA_MAX_DOMAINS;
    mcapi_info->number_of_nodes = MCA_MAX_NODES;
    mcapi_info->number_of_ports = MCAPI_MAX_ENDPOINTS;
    
    //mark that we are inited!
    nodeInitialized_ = 1;

    //succée
    *mcapi_status = MCAPI_SUCCESS;
}

void mcapi_finalize( MCAPI_OUT mcapi_status_t* mcapi_status)
{
    //iterators for endpoint finalization: domain, node, port
    unsigned int x;
    unsigned int y;
    unsigned int z;

    //check for initialization
    if ( mcapi_trans_initialized() == MCAPI_FALSE )
    {
        //no init means failure
        *mcapi_status = MCAPI_ERR_NODE_NOTINIT;
        return;
    }

    //unlink and close the messagequeues
    for ( x = 0; x < MCA_MAX_DOMAINS; ++x )
    {
        for ( y = 0; y < MCA_MAX_NODES; ++y )
        {
            for ( z = 0; z < MCAPI_MAX_ENDPOINTS; ++z )
            {
                struct endPointData* epd =
                &nodeData_.endPoints[x][y][z];

                //skip if not inited to begin with
                if ( epd->inited != 1 )
                    continue;

                //close, so that we are no longer reserving them
                mq_close( epd->msgq_id );
            }
        }
    }

    //mark that we are not inited!
    nodeInitialized_ = 0;

    //succée
    *mcapi_status = MCAPI_SUCCESS;
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
    //becomes true, when compeleted
    mcapi_boolean_t complete;
    //how close we are to timeout
    uint32_t ticks = 0;

    //check for initialization
    if ( mcapi_trans_initialized() == MCAPI_FALSE )
    {
        //no init means failure
        *mcapi_status = MCAPI_ERR_NODE_NOTINIT;
        return MCAPI_FALSE;
    }

    //request must be ok
    if ( !mcapi_trans_valid_request_handle(request) ||
    request->function == MCAPI_NULL )
    {
        *mcapi_status = MCAPI_ERR_REQUEST_INVALID;
        return MCAPI_FALSE;
    }

    //yeah, must be valid size as well, though no supported
    //waitable uses it :-)
    if ( !mcapi_trans_valid_size_param(size) )
    {
        *mcapi_status = MCAPI_ERR_PARAMETER;
        return MCAPI_FALSE;
    }

    //loop until timeout, if applicable
    do
    {
        //sleep a millisecond between iterations
        usleep(1000);
        //the waited-for function
        mcapi_boolean_t (*function) (void*) = request->function;
        //call the function
        complete = function( request->data );
        //closer to time out!
        ++ticks;
    }
    while ( complete != MCAPI_TRUE && ( timeout == MCAPI_TIMEOUT_INFINITE ||
    ticks < timeout ) );

    //if it was a timeout, we shall return with timeout
    if ( complete != MCAPI_TRUE )
    {
        *mcapi_status = MCAPI_TIMEOUT;
        return MCAPI_FALSE;
    }

    //else the operation is complete and ready to return with success
    *mcapi_status = MCAPI_SUCCESS; 

    return MCAPI_TRUE;
}

//macro used to simplify the below code-to-text function
#define STATUS_CASE( status ) \
    case( status ): message = strcpy(status_message, #status); break;

char* mcapi_display_status( mcapi_status_t status, char* status_message,
size_t size)
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
    message[size-1] = '\0';

    //return pointer to the buffer
    return message;
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
    if ( nodeInitialized_ == 1 )
    {
        return MCAPI_TRUE;
    }

    return MCAPI_FALSE;
}

/* checks if the given node is valid */
mcapi_boolean_t mcapi_trans_valid_node(mcapi_uint_t node_num)
{
    if ( node_num >= 0 && node_num < MCA_MAX_NODES )
    {
        return MCAPI_TRUE;
    }

    return MCAPI_FALSE;
}

/* checks if the given domain is valid */
mcapi_boolean_t mcapi_trans_valid_domain(mcapi_uint_t domain_num)
{
    if ( domain_num >= 0 && domain_num < MCA_MAX_DOMAINS )
    {
        return MCAPI_TRUE;
    }

    return MCAPI_FALSE;
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
