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
 	MCAPI_IN mqd_t msgq_id, 
 	MCAPI_IN void* buffer, 
 	MCAPI_IN size_t buffer_size, 
 	MCAPI_IN mcapi_priority_t priority,
    MCAPI_IN mcapi_timeout_t timeout );

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
 	MCAPI_IN mqd_t msgq_id, 
 	MCAPI_OUT void* buffer, 
 	MCAPI_IN size_t buffer_size,
 	MCAPI_OUT size_t* received_size, 
 	MCAPI_OUT mcapi_priority_t* priority,
    MCAPI_IN mcapi_timeout_t timeout );

//checks if there is any messages in queue
//mqd_t msgq_id: the identifier of the receiving queue
//mcapi_status_t* mcapi_status: MCAPI_SUCCESS on success,
//else MCAPI_ERR_GENERAL
//RETURNS count of messages, or MCAPI_NULL on failure
inline mcapi_uint_t pmq_avail(
 	MCAPI_IN mqd_t msgq_id,
    MCAPI_OUT mcapi_status_t* mcapi_status );

//creates message queue for the given local endpoint
//RETURNS MCAPI_SUCCESS on success, else error
inline mcapi_status_t pmq_create_epd(
    MCAPI_IN mcapi_endpoint_t endpoint );

//sets messagequeue for the given remote endpoint
//mcapi_timeout_t timeout: the timeout in milliseconds
//if set to MCAPI_TIMEOUT_INFINITE, there is no timeout
//RETURNS MCAPI_SUCCESS on success, else error or timeout
inline mcapi_status_t pmq_open_epd(
    MCAPI_IN mcapi_endpoint_t endpoint, 
	MCAPI_IN mcapi_timeout_t timeout );

//destroys the given local endpoint
inline void pmq_delete_epd(
    MCAPI_IN mcapi_endpoint_t endpoint );

//creates channel message queue for the given local endpoint
//RETURNS MCAPI_TRUE on success or MCAPI_FALSE on failure
inline mcapi_boolean_t pmq_create_chan( mcapi_endpoint_t us );

//sets channel message queue for the given local endpoint
//RETURNS MCAPI_TRUE on success or MCAPI_FALSE on failure
inline mcapi_boolean_t pmq_open_chan(
    MCAPI_IN mcapi_endpoint_t our_endpoint );

//destroyes the channel message queue of the given local endpoint
inline void pmq_delete_chan(
    MCAPI_IN mcapi_endpoint_t endpoint );

#endif
