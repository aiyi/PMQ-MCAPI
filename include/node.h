//Module includes node spesific data and functions to manipulate them
//Also some general functions.
#ifndef NODE_H
#define NODE_H
#include <mcapi.h>
#include "pmq_layer.h"

//This macro will lock the node mutex, but only if
//thread safety is enabled. Will return with error if lock fails.
#ifdef ALLOW_THREAD_SAFETY
    #define LOCK_NODE( node ) \
        if ( pthread_mutex_lock(&node->mutex) != 0 ) \
        { \
            perror("When locking node mutex"); \
            *mcapi_status = MCAPI_ERR_GENERAL; \
            return; \
        }
#else
    #define LOCK_NODE( node ) ;
#endif

//This macro will unlock the node mutex, but only if
//thread safety is enabled. Will return with error if lock fails.
#ifdef ALLOW_THREAD_SAFETY
    #define UNLOCK_NODE( node ) \
        if ( pthread_mutex_unlock(&node->mutex) != 0 ) \
        { \
            perror("When unlocking node mutex"); \
            *mcapi_status = MCAPI_ERR_GENERAL; \
            return; \
        }
#else
    #define UNLOCK_NODE( node ) ;
#endif

//variables assosiated with one MCAPI node entity
//effectively a singleton, as a node represents a process
struct nodeData
{
    mca_domain_t domain_id; //domain where the node belong to
    mca_node_t node_id; //id of this node
    //data of ALL recognized endpoints, not just the ones in this node!
    struct endPointData endPoints[ENDPOINT_COUNT];
    #ifdef ALLOW_THREAD_SAFETY
    //mutex used to protect the node data
    pthread_mutex_t mutex;
    #endif
};

//returns pointer to the nodedata
struct nodeData* getNodeData();

//tries to reserve and return a request. It will be filled with given
//function and data. Returns MCAPI_NULL on failure
mcapi_request_t reserve_request(
    mcapi_boolean_t (*function) (void*),
    void* data );

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
#endif
