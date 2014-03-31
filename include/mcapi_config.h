//THIS FILE INCLUDES CONSTANTS DEFINABLE BY APPLICATION
//However each process communicating with each other must have same settings
//and thus both application and the implementation must be recompiled each
//time these values are changed.

#ifndef MCAPI_CONFIG
#define MCAPI_CONFIG

//How many buffers there are in total availiable for packet receive.
//As a fist rule, one for each receiving packet endpoint, but only if
//previously obtained buffer is released before receive is called again.
#define MCAPI_MAX_BUFFERS 32

//Maximum lenght of a message in bytes. NOTICE: must not be shorter than
//string defines in channel.h or channel open and close shall fail.
#ifndef MCAPI_MAX_MSG_SIZE
#define MCAPI_MAX_MSG_SIZE 128
#endif

#define MCAPI_MAX_MESSAGE_SIZE MCAPI_MAX_MSG_SIZE

//Maximum length of a packet in bytes
#ifndef MCAPI_MAX_PKT_SIZE
#define MCAPI_MAX_PKT_SIZE 1024
#endif

#define MCAPI_MAX_PACKET_SIZE MCAPI_MAX_PKT_SIZE

//How many endpoints each node may have.
#define MCAPI_MAX_PORT 32

//How many nodes each domain may have.
#define MCAPI_MAX_NODE 16

//How many domains there are.
#define MCAPI_MAX_DOMAIN 2

#endif
