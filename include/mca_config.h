//THIS FILE INCLUDES CONSTANTS DEFINABLE BY APPLICATION
//However each process communicating with each other must have same settings
//and thus both application and the implementation must be recompiled each
//time these values are changed.

//How many buffers there are in total availiable for packet receive.
//As a fist rule, one for each receiving packet endpoint, but only if
//previously obtained buffer is released before receive is called again.
#define MCAPI_MAX_BUFFERS 32

//Maximum lenght of a message in bytes. NOTICE: must not be shorter than
//string defines in channel.h or channel open and close shall fail.
#ifndef MCAPI_MAX_MSG_SIZE
#define MCAPI_MAX_MSG_SIZE 128
#endif

//Maximum length of a packet in bytes
#ifndef MCAPI_MAX_PKT_SIZE
#define MCAPI_MAX_PKT_SIZE 1024
#endif

//How many endpoints each node may have.
#define MCAPI_MAX_ENDPOINTS 32

//How many nodes each domain may have.
#define MCA_MAX_NODES 16

//How many domains there are.
#define MCA_MAX_DOMAINS 2
