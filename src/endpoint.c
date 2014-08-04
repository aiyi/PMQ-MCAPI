#include "endpoint.h"
#include "node.h"

//checks if the endpoint handle refers to a valid endpoint
mcapi_boolean_t mcapi_trans_valid_endpoint (mcapi_endpoint_t endpoint)
{
    if ( endpoint == MCAPI_NULL || endpoint->inited != 1 ||
    endpoint->defs == MCAPI_NULL )
    {
        return MCAPI_FALSE;
    }

    return MCAPI_TRUE;
}

//checks if given endpoint handles refer to a valid endpoints
mcapi_boolean_t mcapi_trans_valid_endpoints
(mcapi_endpoint_t endpoint1, mcapi_endpoint_t endpoint2)
{
    if ( endpoint1 == MCAPI_NULL || endpoint2 == MCAPI_NULL ||
    endpoint1->inited != 1 || endpoint2->inited != 1 ||
    endpoint1->defs == MCAPI_NULL || endpoint2->defs == MCAPI_NULL )
    {
        return MCAPI_FALSE;
    }

    return MCAPI_TRUE;
}

//checks if local endpoint of given port exists
mcapi_boolean_t mcapi_trans_endpoint_exists ( uint32_t port_num )
{
    struct nodeData* nd = getNodeData();
    struct endPointData* epd = findEpd( nd->domain_id, nd->node_id, port_num );

    if ( epd == MCAPI_NULL || epd->inited != 1 )
    {
        return MCAPI_FALSE;
    }

    return MCAPI_TRUE;
}

//checks if the given endpoint is owned by the given node
mcapi_boolean_t mcapi_trans_endpoint_isowner (mcapi_endpoint_t endpoint)
{
    struct nodeData* nd = getNodeData();

    if ( nd->node_id != endpoint->defs->us.node_id ||
    nd->domain_id != endpoint->defs->us.domain_id )
    {
        return MCAPI_FALSE;
    }

    return MCAPI_TRUE;
}

mcapi_endpoint_t mcapi_endpoint_create(
 	MCAPI_IN mcapi_port_t port_id, 
 	MCAPI_OUT mcapi_status_t* mcapi_status)
{
    //pointer to local node specific data
    struct nodeData* nd = getNodeData();
    //the associated data of the would-be-endpoint
    struct endPointData* epd = MCAPI_NULL;

    //check for initialization
    if ( mcapi_trans_initialized() == MCAPI_FALSE )
    {
        //no init means failure
        *mcapi_status = MCAPI_ERR_NODE_NOTINIT;
        return MCAPI_NULL;
    }

    //critical section for node begins here
    LOCK_NODE( nd );

    //must not be already reserved
    if ( mcapi_trans_endpoint_exists( port_id ) )
    {
        *mcapi_status = MCAPI_ERR_ENDP_EXISTS;
        goto ret;
    }

    //obtain pointer to the proper slot of the table
    epd = findEpd( nd->domain_id, nd->node_id, port_id );

    //lack of defines will mean invalid port
    if ( epd == MCAPI_NULL || epd->defs == MCAPI_NULL )
    {
        *mcapi_status = MCAPI_ERR_PORT_INVALID;
        goto ret;
    }

    //critical section for endpoint begins here
    LOCK_ENPOINT( epd );

    //let PMQ-layer handle rest
    *mcapi_status = pmq_create_epd( epd );

    //failure with POSIX -> return
    if ( *mcapi_status != MCAPI_SUCCESS )
    {
        epd = MCAPI_NULL;
        goto ret;
    }

    //mark it inited as well
    epd->inited = 1;
    //name was already assigned before, in the construction

    //mark it as success
    *mcapi_status = MCAPI_SUCCESS;

    //mutex must be unlocked even if error occurs
    ret:
        UNLOCK_NODE( nd );
        UNLOCK_ENPOINT( epd );

        //return the pointer, as it is the endpointtype
        return epd;
}

mcapi_endpoint_t mcapi_endpoint_get(
    MCAPI_IN mcapi_domain_t domain_id,
 	MCAPI_IN mcapi_node_t node_id, 
 	MCAPI_IN mcapi_port_t port_id, 
	MCAPI_IN mcapi_timeout_t timeout,
 	MCAPI_OUT mcapi_status_t* mcapi_status )
{ 
    //the associated data of the would-be-endpoint
    struct endPointData* epd = MCAPI_NULL;
    //pointer to local node spesific data
    struct nodeData* nd = getNodeData();

    //check for initialization
    if ( mcapi_trans_initialized() == MCAPI_FALSE )
    {
        //no init means failure
        *mcapi_status = MCAPI_ERR_NODE_NOTINIT;
        return MCAPI_NULL;
    }

    //critical section for node begins here
    LOCK_NODE( nd );

    //obtain pointer to the proper slot of the table
    epd = findEpd( domain_id, node_id, port_id );

    //lack of defines will mean invalid port
    if ( epd == MCAPI_NULL || epd->defs == MCAPI_NULL )
    {
        *mcapi_status = MCAPI_ERR_PORT_INVALID;
        goto ret;
    }

    //critical section for endpoint begins here
    LOCK_ENPOINT( epd );

    //if it already exists, return it then
    if ( epd->inited == 1 )
    {
        *mcapi_status = MCAPI_SUCCESS;
        goto ret;
    }

    //The POSIX layer handles the rest
    *mcapi_status = pmq_open_epd( epd, timeout );

    //failure with POSIX -> return
    if ( *mcapi_status != MCAPI_SUCCESS )
    {
        epd = MCAPI_NULL;
        goto ret;
    }

    //mark it inited
    epd->inited = 1;
    //name was already assigned before, in the construction

    //upon the successful completion, return the endpoint data, as it the type
    *mcapi_status = MCAPI_SUCCESS;

    //mutex must be unlocked even if error occurs
    ret:
        UNLOCK_NODE( nd );
        UNLOCK_ENPOINT( epd );

        //return the pointer, as it is the endpoint type
        return epd;
}

void mcapi_endpoint_delete(
 	MCAPI_IN mcapi_endpoint_t endpoint, 
 	MCAPI_OUT mcapi_status_t* mcapi_status)
{
    //iterator
    unsigned int i;
    //data of the node from witch endpoint shall be unassociated
    struct nodeData* nd = getNodeData();
    //we need receive buffer to empty the message queue somewhere
    char recv_buf[MCAPI_MAX_MESSAGE_SIZE];
    //timeout used by posix-function: actually no time at all
    struct timespec time_limit = { 0, 0 };
    //used to observe, when buffer is 
    size_t mslen;

    //check for initialization
    if ( mcapi_trans_initialized() == MCAPI_FALSE )
    {
        //no init means failure
        *mcapi_status = MCAPI_ERR_NODE_NOTINIT;
        return;
    }

    //critical section for node begins here
    LOCK_NODE( nd );

    //critical section for endpoint begins here
    LOCK_ENPOINT( endpoint );

    //check for valid
    if ( mcapi_trans_valid_endpoint( endpoint ) == MCAPI_FALSE )
    {
        *mcapi_status = MCAPI_ERR_ENDP_INVALID;
        goto ret;
    }

    //check for connected
    if ( endpoint->open == 1 )
    {
        *mcapi_status = MCAPI_ERR_CHAN_CONNECTED;
        goto ret;
    }

    //check for owner
    if ( mcapi_trans_endpoint_isowner( endpoint ) == MCAPI_FALSE )
    {
        *mcapi_status = MCAPI_ERR_ENDP_NOTOWNER;
        goto ret;
    }

    //Use the layer to handle the practical deletion
    pmq_delete_epd( endpoint );

    //mark the endpoint uninited
    endpoint->inited = -1;
    //discard open pending
    endpoint->pend_open = -1;
    endpoint->synced = -1;
    //assume attributes are discarded as well
    endpoint->time_out = MCAPI_TIMEOUT_INFINITE;

    //so far so good
    *mcapi_status = MCAPI_SUCCESS;

    //mutex must be unlocked even if error occurs
    ret:
        UNLOCK_ENPOINT( endpoint );
        UNLOCK_NODE( nd );
}

void mcapi_endpoint_set_attribute(
    MCAPI_IN mcapi_endpoint_t endpoint,
    MCAPI_IN mcapi_uint_t attribute_num,
    MCAPI_IN const void* attribute,
    MCAPI_IN size_t attribute_size,
    MCAPI_OUT mcapi_status_t* mcapi_status )
{
    //data of the node from witch endpoint shall be unassociated
    struct nodeData* nd = getNodeData();

    //check for initialization
    if ( mcapi_trans_initialized() == MCAPI_FALSE )
    {
        //no init means failure
        *mcapi_status = MCAPI_ERR_NODE_NOTINIT;
        return;
    }

    //critical section for node begins here
    LOCK_NODE( nd );

    //critical section for endpoint begins here
    LOCK_ENPOINT( endpoint );
    
    //check for valid
    if ( mcapi_trans_valid_endpoint( endpoint ) == MCAPI_FALSE )
    {
        *mcapi_status = MCAPI_ERR_ENDP_INVALID;
        goto ret;
    }

    //check for connected
    if ( endpoint->open == 1 )
    {
        *mcapi_status = MCAPI_ERR_CHAN_CONNECTED;
        goto ret;
    }

    //check for owner, so that it will local
    if ( mcapi_trans_endpoint_isowner( endpoint ) == MCAPI_FALSE )
    {
        *mcapi_status = MCAPI_ERR_ENDP_REMOTE;
        goto ret;
    }

    //attribute supplied must not be null
    if ( attribute == MCAPI_NULL )
    {
        *mcapi_status = MCAPI_ERR_PARAMETER;
        goto ret;
    }

    //beyond boundaries means unknown
    if ( attribute_num < 0 || attribute_num > MCAPI_ENDP_ATTR_END )
    {
        *mcapi_status = MCAPI_ERR_ATTR_NUM;
        goto ret;
    }

    //only one is supported
    if ( attribute_num != MCAPI_ENDP_ATTR_TIMEOUT )
    {
        *mcapi_status = MCAPI_ERR_ATTR_NOTSUPPORTED;
        goto ret;
    }

    //size must correspond
    if ( attribute_size != sizeof(mcapi_timeout_t) )
    {
        *mcapi_status = MCAPI_ERR_ATTR_SIZE;
        goto ret;
    }
    
    //finally, we may set it
    endpoint->time_out = *(mcapi_timeout_t*)attribute;
    *mcapi_status = MCAPI_SUCCESS;

    //mutex must be unlocked even if error occurs
    ret:
        UNLOCK_ENPOINT( endpoint );
        UNLOCK_NODE( nd );
}
