//functions in this module are used internally by both channel types
//channel type spesific functions are on their respective modules
#ifndef CHANNEL_H
#define CHANNEL_H

#include <mcapi.h>

//define code for opening channel
#define CODE_OPEN_CONNECTED "MCAPI_CHAN_OPEN"

//define code for closing channel
#define CODE_CLOSE_CONNECTED "MCAPI_CHAN_CLOSE"

//used to connect channel, so that they can be opened
void mcapi_chan_connect(
 	MCAPI_IN mcapi_endpoint_t  send_endpoint, 
 	MCAPI_IN mcapi_endpoint_t  receive_endpoint, 
 	MCAPI_OUT mcapi_request_t* request, 
 	MCAPI_OUT mcapi_status_t* mcapi_status);

//used to open channel
void mcapi_chan_open(
 	MCAPI_OUT struct handle_type* handle, 
 	MCAPI_IN mcapi_endpoint_t  endpoint, 
 	MCAPI_OUT mcapi_request_t* request, 
 	MCAPI_OUT mcapi_status_t* mcapi_status,
    MCAPI_IN channel_type expected_type,
    MCAPI_IN channel_dir expected_dir );

//used to close channel
void mcapi_chan_close(
 	MCAPI_IN struct handle_type handle, 
 	MCAPI_OUT mcapi_request_t* request, 
 	MCAPI_OUT mcapi_status_t* mcapi_status,
    MCAPI_IN channel_type expected_type,
    MCAPI_IN channel_dir expected_dir);

//used to check how many items the channel has ready for retrieval
mcapi_uint_t mcapi_chan_available(
    MCAPI_IN mcapi_pktchan_recv_hndl_t receive_handle,
    MCAPI_OUT mcapi_status_t* mcapi_status );

/* checks if the given channel handle is valid */
extern mcapi_boolean_t mcapi_trans_valid_pktchan_send_handle( mcapi_pktchan_send_hndl_t handle);
extern mcapi_boolean_t mcapi_trans_valid_pktchan_recv_handle( mcapi_pktchan_recv_hndl_t handle);
extern mcapi_boolean_t mcapi_trans_valid_sclchan_send_handle( mcapi_sclchan_send_hndl_t handle);
extern mcapi_boolean_t mcapi_trans_valid_sclchan_recv_handle( mcapi_sclchan_recv_hndl_t handle);
#endif
