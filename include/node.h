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
    //data of ALL recognized endpoints, not just the ones in this node!
    struct endPointData endPoints[ENDPOINT_COUNT];
};

//returns pointer to the nodedata
struct nodeData* getNodeData();

//finds endpoint data matching the tuple. return MCAPI_NULL if not.
struct endPointData* findEpd( mcapi_domain_t domain_id, mcapi_node_t node_id,
unsigned int port_id);

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

/* checks if the endpoint handle refers to a valid endpoint */
extern mcapi_boolean_t mcapi_trans_valid_endpoint (mcapi_endpoint_t endpoint);
extern mcapi_boolean_t mcapi_trans_valid_endpoints (mcapi_endpoint_t endpoint1, 
                                             mcapi_endpoint_t endpoint2);
#endif
