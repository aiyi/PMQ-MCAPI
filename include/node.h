//Module includes node spesific data and functions to manipulate them
//Also some general functions.
#ifndef NODE_H
#define NODE_H
#include <mcapi.h>
#include "pmq_layer.h"

//variables assosiated witn one MCAPI node entity
//effectively a singleton, as a node represents a process
struct nodeData
{
    mca_domain_t domain_id; //domain where the node belong to
    mca_node_t node_id; //id of this node
    //each index is a port number, each value is mqdt of endpoint, -1 is null
    struct endPointData endPoints[MCA_MAX_DOMAINS][MCA_MAX_NODES]\
        [MCAPI_MAX_ENDPOINTS];
};

//returns pointer to the nodedata
struct nodeData* getNodeData();

/* checks if the given domain is valid */
mcapi_boolean_t mcapi_trans_valid_domain(mcapi_uint_t domain_num);

//sets keys of all buffers to MCAPI_NULL
void bufClearAll();

/* checks if this node has already called initialize */
extern mcapi_boolean_t mcapi_trans_initialized ();

/* checks if the buffer parameter is valid */
extern mcapi_boolean_t mcapi_trans_valid_buffer_param (MCAPI_IN void* buffer);

/* checks if the request parameter is valid */
extern mcapi_boolean_t mcapi_trans_valid_request_handle
(MCAPI_IN mcapi_request_t* request);

/* checks if the size parameter is valid */
extern mcapi_boolean_t mcapi_trans_valid_size_param (MCAPI_IN size_t*size);

/* checks if the given node is valid */
extern mcapi_boolean_t mcapi_trans_valid_node(mcapi_uint_t node_num);

/* checks to see if the port_num is a valid port_num for this system */
extern mcapi_boolean_t mcapi_trans_valid_port(mcapi_uint_t port_num);

/* checks if the endpoint handle refers to a valid endpoint */
extern mcapi_boolean_t mcapi_trans_valid_endpoint (mcapi_endpoint_t endpoint);
extern mcapi_boolean_t mcapi_trans_valid_endpoints (mcapi_endpoint_t endpoint1, 
                                             mcapi_endpoint_t endpoint2);
#endif
