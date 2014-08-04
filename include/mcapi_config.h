//THIS FILE INCLUDES CONSTANTS DEFINABLE BY APPLICATION
//However each process communicating with each other must have same settings
//and thus both application and the implementation must be recompiled each
//time these values are changed.

#ifndef MCAPI_CONFIG
#define MCAPI_CONFIG

//Defines how many messages there may be in POSIX message queue at time.
//Sender will block if full.
#ifndef MAX_QUEUE_ELEMENTS
#define MAX_QUEUE_ELEMENTS 10
#endif

//How many buffers there are in total available for packet receive.
//As a fist rule, one for each receiving packet endpoint, but only if
//previously obtained buffer is released before receive is called again.
#ifndef MCAPI_MAX_BUFFERS
#define MCAPI_MAX_BUFFERS 32
#endif

//Maximum number of simultaneous non-blocking requests
#ifndef MCAPI_MAX_REQUESTS
#define MCAPI_MAX_REQUESTS 32
#endif

//Maximum lenght of a message in bytes.
#ifndef MCAPI_MAX_MESSAGE_SIZE
#define MCAPI_MAX_MESSAGE_SIZE 128
#endif

//Maximum length of a packet in bytes. NOTICE: must not be shorter than
//eight (8) bytes!
#ifndef MCAPI_MAX_PACKET_SIZE
#define MCAPI_MAX_PACKET_SIZE 1024
#endif

//Uncomment to allow thread safety. WILL LIKELY AFFECT PERFORMANCE
//#define ALLOW_THREAD_SAFETY

#endif
