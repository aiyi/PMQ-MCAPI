//This module has functions used in creating, getting and checking endpoints.
#include "node.h"

/* checks if the endpoint handle refers to a valid endpoint */
mcapi_boolean_t mcapi_trans_valid_endpoint (mcapi_endpoint_t endpoint)
{
    if ( endpoint == MCAPI_NULL || endpoint->inited != 1 ||
    endpoint->defs == MCAPI_NULL )
    {
        return MCAPI_FALSE;
    }

    return MCAPI_TRUE;
}

mcapi_boolean_t mcapi_trans_valid_endpoints (mcapi_endpoint_t endpoint1, 
                                               mcapi_endpoint_t endpoint2)
{
    if ( endpoint1 == MCAPI_NULL || endpoint2 == MCAPI_NULL ||
    endpoint1->inited != 1 || endpoint2->inited != 1 ||
    endpoint1->defs == MCAPI_NULL || endpoint2->defs == MCAPI_NULL )
    {
        return MCAPI_FALSE;
    }

    return MCAPI_TRUE;
}

/* checks to see if the port_num is a valid port_num for this system */
mcapi_boolean_t mcapi_trans_valid_port(mcapi_uint_t port_num)
{
    if ( port_num >= 0 && port_num < MCAPI_MAX_PORT )
    {
        return MCAPI_TRUE;
    }

    return MCAPI_FALSE;
}

mcapi_boolean_t mcapi_trans_endpoint_exists (mcapi_domain_t domain_id, 
    uint32_t port_num)
{
    struct nodeData* nd = getNodeData();

    if ( nd->endPoints[domain_id][nd->node_id][port_num].inited != 1 )
    {
        return MCAPI_FALSE;
    }

    return MCAPI_TRUE;
}

/* checks if the given endpoint is owned by the given node */
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

/* create endpoint <node_num,port_num> and return it's handle */
mcapi_endpoint_t mcapi_endpoint_create(
 	MCAPI_IN mcapi_port_t port_id, 
 	MCAPI_OUT mcapi_status_t* mcapi_status)
{
    //pointer to local node spesific data
    struct nodeData* nd = getNodeData();
    //the associated data of the would-be-endpoint
    struct endPointData* epd;

    //check for initialization
    if ( mcapi_trans_initialized() == MCAPI_FALSE )
    {
        //no init means failure
        *mcapi_status = MCAPI_ERR_NODE_NOTINIT;
        return MCAPI_NULL;
    }

    //check for valid
    if ( mcapi_trans_valid_port( port_id ) == MCAPI_FALSE)
    {
        *mcapi_status = MCAPI_ERR_PORT_INVALID;
        return MCAPI_NULL;
    }

    //must not already reserved
    if ( mcapi_trans_endpoint_exists( nd->domain_id, port_id ) )
    {
        *mcapi_status = MCAPI_ERR_ENDP_EXISTS;
        return MCAPI_NULL;
    }

    //obtain pointer to the proper slot of the table
    epd = &nd->endPoints[nd->domain_id][nd->node_id][port_id];

    //lack of defines will also mean invalid port
    if ( epd->defs == MCAPI_NULL )
    {
        *mcapi_status = MCAPI_ERR_PORT_INVALID;
        return MCAPI_NULL;
    }

    //if it already exist, reuse it!
    if ( epd->inited == 1 )
    {
        //mark it as success
        *mcapi_status = MCAPI_SUCCESS;

        return epd;
    }

    //let PMQ-layer handle rest
    pmq_create_epd( epd );

    //failure with POSIX -> return
    if ( *mcapi_status != MCAPI_SUCCESS )
    {
        return MCAPI_NULL;
    }

    //mark it inited as well
    epd->inited = 1;
    //name was already assigned before, in the construction

    //mark it as success
    *mcapi_status = MCAPI_SUCCESS;

    //return the pointer, as it is the endpointtype
    return epd;
}

/* blocking get endpoint for the given <node_num,port_num> and return it's handle */
mcapi_endpoint_t mcapi_endpoint_get(
    MCAPI_IN mcapi_domain_t domain_id,
 	MCAPI_IN mcapi_node_t node_id, 
 	MCAPI_IN mcapi_port_t port_id, 
	MCAPI_IN mcapi_timeout_t timeout,
 	MCAPI_OUT mcapi_status_t* mcapi_status )
{ 
    //the associated data of the would-be-endpoint
    struct endPointData* epd;
    //pointer to local node spesific data
    struct nodeData* nd = getNodeData();

    //check for initialization
    if ( mcapi_trans_initialized() == MCAPI_FALSE )
    {
        //no init means failure
        *mcapi_status = MCAPI_ERR_NODE_NOTINIT;
        return MCAPI_NULL;
    }

    //check for valid port
    if ( mcapi_trans_valid_port( port_id ) == MCAPI_FALSE )
    {
        *mcapi_status = MCAPI_ERR_PORT_INVALID;
        return MCAPI_NULL;
    }

    //check for valid node
    if ( mcapi_trans_valid_node( node_id ) == MCAPI_FALSE )
    {
        *mcapi_status = MCAPI_ERR_NODE_INVALID;
        return MCAPI_NULL;
    }

    //check for valid domain
    if ( mcapi_trans_valid_domain( domain_id ) == MCAPI_FALSE )
    {
        *mcapi_status = MCAPI_ERR_DOMAIN_INVALID;
        return MCAPI_NULL;
    }

    //obtain pointer to the proper slot of the table
    epd = &nd->endPoints[domain_id][node_id][port_id];

    //lack of defines will also mean invalid port
    if ( epd->defs == MCAPI_NULL )
    {
        *mcapi_status = MCAPI_ERR_PORT_INVALID;
        return MCAPI_NULL;
    }

    //if it already exists, return it then
    if ( epd->inited == 1 )
    {
        *mcapi_status = MCAPI_SUCCESS;
        return epd;
    }

    //The POSIX layer handles the rest
    *mcapi_status = pmq_open_epd( epd, timeout );

    //failure with POSIX -> return
    if ( *mcapi_status != MCAPI_SUCCESS )
    {
        return MCAPI_NULL;
    }

    //mark it inited
    epd->inited = 1;
    //name was already assigned before, in the construction

    //upon the succesfull completion, return the endpoint data, as it the type
    *mcapi_status = MCAPI_SUCCESS;
    return epd;
}

/* delete the given endpoint */
void mcapi_endpoint_delete(
 	MCAPI_IN mcapi_endpoint_t endpoint, 
 	MCAPI_OUT mcapi_status_t* mcapi_status)
{
    //iterator
    unsigned int i;
    //data of the node from witch endpoint shall be unassosiated
    struct nodeData* nd = getNodeData();
    //we need receive buffer to empty the message queue somewhere
    char recv_buf[MCAPI_MAX_MSG_SIZE];
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

    //check for valid
    if ( mcapi_trans_valid_endpoint( endpoint ) == MCAPI_FALSE )
    {
        *mcapi_status = MCAPI_ERR_ENDP_INVALID;
        return;
    }

    //check for connected
    if ( endpoint->open == 1 )
    {
        *mcapi_status = MCAPI_ERR_CHAN_CONNECTED;
        return;
    }

    //check for owner
    if ( mcapi_trans_endpoint_isowner( endpoint ) == MCAPI_FALSE )
    {
        *mcapi_status = MCAPI_ERR_ENDP_NOTOWNER;
        return;
    }

    //Use the layer to handle the practical deletion
    pmq_delete_epd( endpoint );

    //mark the endpoint uninited
    endpoint->inited = -1;
    *mcapi_status = MCAPI_SUCCESS;
}

void mcapi_endpoint_set_attribute(
    MCAPI_IN mcapi_endpoint_t endpoint,
    MCAPI_IN mcapi_uint_t attribute_num,
    MCAPI_IN const void* attribute,
    MCAPI_IN size_t attribute_size,
    MCAPI_OUT mcapi_status_t* mcapi_status )
{
    //check for initialization
    if ( mcapi_trans_initialized() == MCAPI_FALSE )
    {
        //no init means failure
        *mcapi_status = MCAPI_ERR_NODE_NOTINIT;
        return;
    }
    
    //check for valid
    if ( mcapi_trans_valid_endpoint( endpoint ) == MCAPI_FALSE )
    {
        *mcapi_status = MCAPI_ERR_ENDP_INVALID;
        return;
    }

    //check for connected
    if ( endpoint->open == 1 )
    {
        *mcapi_status = MCAPI_ERR_CHAN_CONNECTED;
        return;
    }

    //check for owner, so that it will local
    if ( mcapi_trans_endpoint_isowner( endpoint ) == MCAPI_FALSE )
    {
        *mcapi_status = MCAPI_ERR_ENDP_REMOTE;
        return;
    }

    //attribute supplied must not be null
    if ( attribute == MCAPI_NULL )
    {
        *mcapi_status = MCAPI_ERR_PARAMETER;
        return;
    }

    //beyond boundaries means unknown
    if ( attribute_num < 0 || attribute_num > MCAPI_ENDP_ATTR_END )
    {
        *mcapi_status = MCAPI_ERR_ATTR_NUM;
        return;
    }

    //only one is supported
    if ( attribute_num != MCAPI_ENDP_ATTR_TIMEOUT )
    {
        *mcapi_status = MCAPI_ERR_ATTR_NOTSUPPORTED;
        return;
    }

    //size must correspond
    if ( attribute_size != sizeof(mcapi_timeout_t) )
    {
        *mcapi_status = MCAPI_ERR_ATTR_SIZE;
        return;
    }
    
    //finally, we may set it
    endpoint->time_out = *(mcapi_timeout_t*)attribute;
    *mcapi_status = MCAPI_SUCCESS;
}
