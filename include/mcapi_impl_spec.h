/*
Copyright (c) 2010, The Multicore Association
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

(1) Redistributions of source code must retain the above copyright
notice, this list of conditions and the following disclaimer.
 
(2) Redistributions in binary form must reproduce the above copyright
notice, this list of conditions and the following disclaimer in the
documentation and/or other materials provided with the distribution. 

(3) Neither the name of the Multicore Association nor the names of its
contributors may be used to endorse or promote products derived from
this software without specific prior written permission. 

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#ifndef MCAPI_DATATYPES_H
#define MCAPI_DATATYPES_H

#include <stdint.h>

#include <mca.h>
#include <mqueue.h>
#include "mcapi_config.h"
#include "endpointdef.h"

/******************************************************************
           definitions and constants 
 ******************************************************************/
//Defines how many priority levels there may be when sending messages
#define MCAPI_MAX_PRIORITY 10

//Defines how many messages there may be in receiving buffer before
//sender will block.
#define MAX_QUEUE_ELEMENTS 10 

//The ID of implementing organization
#define MCAPI_ORG_ID 33720

//The current version of the implementation
#define MCAPI_IMPL_VERSION 0101

//Used on some unimplemented features
#define MAX_NUM_ATTRIBUTES 4

/******************************************************************
           datatypes
******************************************************************/ 

//The runtime data associated with an endpoint.
struct endPointData
{
    mqd_t msgq_id; //the messagequeue used for communication
    struct endPointDef* defs; //predefined constants of the end point
    char open; //1 if channel open, else not 1
    char synced; //1 if sync message is sent during operation, else not 1
    char pend_open; //1 if penging open, else not 1
    char pend_close; //1 if penging close, else not 1
    char inited; //1 if initalized, else not 1
    mqd_t chan_msgq_id; //the messagequeue used for channel communication
    mca_timeout_t time_out; //timeout for operations without other timeout
};

//the endpoint type must be defined as something usable.
//and we shall deem it as pointer to implementation spedific endpoint data.
//NOTICE: as it is pointer, its size shall be 32 bits in 32 bit processes and
//64 bits in 64 bit processes, but it should not matter.
typedef struct endPointData* mcapi_endpoint_t;

//the request handle provided to wait function. includes the function called
//by wait to perform the actual operation pluss the data to be supplied for it
struct request_data
{
    mca_boolean_t (*function) (void*);
    void* data;
};

//define mcapi handle as our implementation spesific handle
typedef struct request_data mcapi_request_t;

//a generic handletype. is a struct so that may be expanded
struct handle_type
{
    mcapi_endpoint_t us; //the endpoint, which is "us"
};

//handles used to access channels. in our implementation we use above struct
typedef struct handle_type mcapi_pktchan_recv_hndl_t;
typedef struct handle_type mcapi_pktchan_send_hndl_t;
typedef struct handle_type mcapi_sclchan_send_hndl_t;
typedef struct handle_type mcapi_sclchan_recv_hndl_t;

//BELOW STUFF ARE NOT USED IN IMPLEMENTATION AND THUS NOT EXPLAINED

typedef int mcapi_param_t;

typedef struct {
  mca_boolean_t valid;
  uint16_t attribute_num;
  uint32_t bytes;
  void* attribute_d;  
} attribute_entry_t;

typedef struct {
  attribute_entry_t entries[MAX_NUM_ATTRIBUTES];
} attributes_t;
  
typedef attributes_t mcapi_node_attributes_t;
typedef attributes_t mcapi_endpt_attributes_t;

#endif

#ifdef __cplusplus
extern } 
#endif /* __cplusplus */
