//This module has functions used in creating, getting and checking endpoints.
#ifndef ENDPOINT_H
#define ENDPOINT_H
#include <mcapi.h>

//finds endpoint data matching the tuple. return MCAPI_NULL if not.
struct endPointData* findEpd( mcapi_domain_t domain_id, mcapi_node_t node_id,
unsigned int port_id);

/* checks if the endpoint handle refers to a valid endpoint */
extern mcapi_boolean_t mcapi_trans_valid_endpoint (mcapi_endpoint_t endpoint);
extern mcapi_boolean_t mcapi_trans_valid_endpoints (mcapi_endpoint_t endpoint1, 
                                             mcapi_endpoint_t endpoint2);

//This macro will lock the endpoint specific mutex, but only if
//thread safety is enabled. Will return with error if lock fails.
#ifdef ALLOW_THREAD_SAFETY
    #define LOCK_ENPOINT( endpoint ) \
        if ( endpoint == MCAPI_NULL ) \
        { \
            *mcapi_status = MCAPI_ERR_ENDP_INVALID; \
            goto ret; \
        } \
        if ( pthread_mutex_lock(&endpoint->mutex) != 0 ) \
        { \
            perror("When locking endpoint mutex"); \
            *mcapi_status = MCAPI_ERR_GENERAL; \
            goto ret; \
        }
#else
    #define LOCK_ENPOINT( endpoint ) ;
#endif

//This macro will unlock the endpoint specific mutex, but only if
//thread safety is enabled. Will ONLY print error if fails.
#ifdef ALLOW_THREAD_SAFETY
    #define UNLOCK_ENPOINT( endpoint ) \
        if ( endpoint != MCAPI_NULL && \
        pthread_mutex_unlock(&endpoint->mutex) != 0 ) \
        { \
            perror("When unlocking endpoint mutex"); \
        }
#else
    #define UNLOCK_ENPOINT( endpoint ) ;
#endif

//This macro will lock the channel handle specific mutex, but only if
//thread safety is enabled. Will return with error if lock fails.
#ifdef ALLOW_THREAD_SAFETY
    #define LOCK_CHANNEL( handle ) \
        if ( handle.us == MCAPI_NULL ) \
        { \
            *mcapi_status = MCAPI_ERR_CHAN_INVALID; \
            return; \
        } \
        if ( pthread_mutex_lock(&handle.us->mutex) != 0 ) \
        { \
            perror("When locking channel handle mutex"); \
            *mcapi_status = MCAPI_ERR_GENERAL; \
            return; \
        }
#else
    #define LOCK_CHANNEL( endpoint ) ;
#endif

//This macro will unlock the channel handle specific mutex, but only if
//thread safety is enabled. Will ONLY print error if fails.
#ifdef ALLOW_THREAD_SAFETY
    #define UNLOCK_CHANNEL( handle ) \
        if ( handle.us != MCAPI_NULL && \
        pthread_mutex_unlock(&handle.us->mutex) != 0 ) \
        { \
            perror("When unlocking channel handle mutex"); \
        }
#else
    #define UNLOCK_CHANNEL( endpoint ) ;
#endif

#endif
