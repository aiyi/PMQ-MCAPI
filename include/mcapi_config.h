//THIS FILE INCLUDES CONSTANTS DEFINABLE BY APPLICATION
//However each process communicating with each other must have same settings
//and thus both application and the implementation must be recompiled each
//time these values are changed.

#ifndef MCAPI_CONFIG
#define MCAPI_CONFIG

//How many buffers there are in total availiable for packet receive.
//As a fist rule, one for each receiving packet endpoint, but only if
//previously obtained buffer is released before receive is called again.
#ifndef MCAPI_MAX_BUFFERS
#define MCAPI_MAX_BUFFERS 32
#endif

//Maximum lenght of a message in bytes. NOTICE: must not be shorter than
//string defines in channel.h or channel open and close shall fail.
#ifndef MCAPI_MAX_MESSAGE_SIZE
#define MCAPI_MAX_MESSAGE_SIZE 128
#endif

//Maximum length of a packet in bytes
#ifndef MCAPI_MAX_PACKET_SIZE
#define MCAPI_MAX_PACKET_SIZE 1024
#endif

//Below constraints are ineffective, but must be provided for compatibility
#define MCAPI_MAX_PORT 1
#define MCAPI_MAX_NODE 1
#define MCAPI_MAX_DOMAIN 1

#endif
