//This module acts as a layer between the actual POSIX-calls and other modules
#ifndef PMQ_LAYER_H
#define PMQ_LAYER_H

#include <mcapi.h>
#include <mqueue.h>

//sends a message via posix message queue
//mqd_t msgq_id: the identifier of the receiving queue
//void* buffer: the buffer containing the message
//size_t buffer_size: the size of the message within buffer
//mcapi_priority_t priority: the priority, ascending
//mcapi_timeout_t timeout: the timeout in milliseconds
//if set to MCAPI_TIMEOUT_INFINITE, there is no timeout
//RETURNS MCAPI_SUCCESS on success, else error or timeout
inline mcapi_status_t pmq_send(
    mqd_t msgq_id, 
    void* buffer, 
    size_t buffer_size, 
    mcapi_priority_t priority,
    mcapi_timeout_t timeout );

//receives a message via posix message queue
//mqd_t msgq_id: the identifier of the receiving queue
//void* buffer: the buffer containing the message
//size_t buffer_size: the size of the buffer for message
//size_t* received_size: the actual received size
//mcapi_priority_t priority*: the priority, ascending
//mcapi_timeout_t timeout: the timeout in milliseconds
//if set to MCAPI_TIMEOUT_INFINITE, there is no timeout
//RETURNS MCAPI_SUCCESS on success, else error or timeout
inline mcapi_status_t pmq_recv(
    mqd_t msgq_id, 
    void* buffer, 
    size_t buffer_size,
    size_t* received_size, 
    mcapi_priority_t* priority,
    mcapi_timeout_t timeout );

//checks if there is any messages in queue
//mqd_t msgq_id: the identifier of the receiving queue
//mcapi_status_t* mcapi_status: MCAPI_SUCCESS on success,
//else MCAPI_ERR_GENERAL
//RETURNS count of messages, or MCAPI_NULL on failure
inline mcapi_uint_t pmq_avail(
    mqd_t msgq_id,
    mcapi_status_t* mcapi_status );

//creates message queue for the given local endpoint
//RETURNS MCAPI_SUCCESS on success, else error
inline mcapi_status_t pmq_create_epd(
    mcapi_endpoint_t endpoint );

//sets messagequeue for the given remote endpoint
//mcapi_timeout_t timeout: the timeout in milliseconds
//if set to MCAPI_TIMEOUT_INFINITE, there is no timeout
//RETURNS MCAPI_SUCCESS on success, else error or timeout
inline mcapi_status_t pmq_open_epd(
    mcapi_endpoint_t endpoint, 
	mcapi_timeout_t timeout );

//destroys the given local endpoint
inline void pmq_delete_epd(
    mcapi_endpoint_t endpoint );

//opens channel message queue for the given local endpoint
//sending and reseiving ends have their own functions for this
//RETURNS MCAPI_TRUE on success or MCAPI_FALSE on failure
inline mcapi_boolean_t pmq_open_chan_recv( mcapi_endpoint_t us );
inline mcapi_boolean_t pmq_open_chan_send( mcapi_endpoint_t us );

//destroys the channel message queue of the given local endpoint
//unlinks only if parameter unlink is MCAPI_TRUE
inline void pmq_delete_chan(
    mcapi_endpoint_t endpoint,
    mcapi_boolean_t unlink );

#endif
