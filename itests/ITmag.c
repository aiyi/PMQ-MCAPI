#include <stdlib.h>
#include <string.h>
#include <mcapi.h>
#include <stdio.h>

#define check( excpt, got ) \
    if ( excpt != got ) { \
        char status_msg[MCAPI_MAX_STATUS_MSG_LEN]; \
        mcapi_display_status( got, status_msg, MCAPI_MAX_STATUS_MSG_LEN ); \
        printf( "FAILURE AT %s, %u: got: %s expected: %s\n", __FILE__, \
        __LINE__, status_msg, #excpt ); \
        exit(0); }

#define COLOR "\e[95mmagenta\e[0m: "

#define ITERATIONS 15000
#define TIMEOUT 5000

//status message received in almost all MCAPI-calls
mcapi_status_t status;
//info-struct received in initialization
mcapi_info_t info;
//the status code converted to string
char status_msg[MCAPI_MAX_STATUS_MSG_LEN];
//request handles used to operate wait-calls
mcapi_request_t request1;
mcapi_request_t request2;
//size parameter required in some calls
size_t size = 1;
//recv-handle used in channel-messaging
mcapi_pktchan_recv_hndl_t handy1;
//send-handle used in channel-messaging
mcapi_pktchan_send_hndl_t handy2;
//pid is used to identify process
pid_t pid;
//get our stuff here
unsigned char* recv_buf;
//a previously received buffer is passed on with this
//A SHARED VARIABLE BETWEEN THREADS
unsigned char* mid_buf;
//the secondary thread
pthread_t thread;
//used for mutualy exclusion between threads
pthread_mutex_t mHandle;
//used for syn between threads
pthread_mutex_t sHandle1;
//used for syn between threads
pthread_mutex_t sHandle2;
//the endpoints used in channel-oriented communication
mcapi_endpoint_t mag_cyan_send;
mcapi_endpoint_t mag_cyan_recv;
mcapi_endpoint_t cyan_mag_send;
mcapi_endpoint_t cyan_mag_recv;

static void * _thrd_wrapper_function(void * aArg)
{
    //an iterator used in loops
    unsigned int i = 0;
    //our pointer to buf. exist here so we can
    //give up the shared memory earlier
    unsigned char* use_buf;

    printf(COLOR "sender thread launched\n");

    //this is to be done multiple times
    for ( i = 0; i < ITERATIONS; ++i )
    {
        //an iterator used in loops
        unsigned int j = 0;
        //obtain sync, that way we know there is something to do
        pthread_mutex_lock(&sHandle1);

        //obtain lock
        pthread_mutex_lock(&mHandle);

        //pass the buffer on
        use_buf = mid_buf;

        //done: release lock
        pthread_mutex_unlock(&mHandle);

        //and then free the other sync
        pthread_mutex_unlock(&sHandle2);

        //increment each value by one
        for ( j = 0; j < MCAPI_MAX_PACKET_SIZE; ++j )
        {
            use_buf[j] = use_buf[j] + 1;
        }

        //send the packet
        mcapi_pktchan_send( handy2, use_buf, MCAPI_MAX_PACKET_SIZE, &status );
        
        //release
        mcapi_pktchan_release( use_buf, &status );
        check( MCAPI_SUCCESS, status );
    }

    printf(COLOR "sending done, closing this thread\n");

    return 0;
}

int main()
{
    //an iterator used in loops
    unsigned int i = 0;

    printf(COLOR "here\n");

    //we are the magenta
    mcapi_initialize( THE_DOMAIN, MAG_NODE, 0, 0, &info, &status );
    check( MCAPI_SUCCESS, status );

    //create our sending channel endpoint
    mag_cyan_send = mcapi_endpoint_create( MAG_SEND, &status );
    check( MCAPI_SUCCESS, status );
    //get their receiving channel endpoint
    mag_cyan_recv = mcapi_endpoint_get( THE_DOMAIN,
    CYAN_NODE, CYAN_RECV, TIMEOUT, &status );
    check( MCAPI_SUCCESS, status );

    //create our receiving channel endpoint
    cyan_mag_recv = mcapi_endpoint_create( MAG_RECV, &status );
    check( MCAPI_SUCCESS, status );
    //get their sending channel endpoint
    cyan_mag_send = mcapi_endpoint_get( THE_DOMAIN,
    CYAN_NODE, CYAN_SEND, TIMEOUT, &status );
    check( MCAPI_SUCCESS, status );

    //form the sending channel
    mcapi_pktchan_connect_i( mag_cyan_send, mag_cyan_recv, &request2, &status );
    check( MCAPI_PENDING, status );
    //wait for it to happen
    mcapi_wait( &request2, &size, TIMEOUT, &status );
    check( MCAPI_SUCCESS, status );

    //open our ends
    mcapi_pktchan_send_open_i( &handy2, mag_cyan_send, &request2, &status );
    check( MCAPI_PENDING, status );
    mcapi_pktchan_recv_open_i( &handy1, cyan_mag_recv, &request1, &status );
    check( MCAPI_PENDING, status );

    //wait for them to happen, IN RIGHT ORDER: send before receive
    //sides of communication must do these in reverse order or deadlock
    mcapi_wait( &request2, &size, TIMEOUT, &status );
    check( MCAPI_SUCCESS, status );
    mcapi_wait( &request1, &size, TIMEOUT, &status );
    check( MCAPI_SUCCESS, status );

    printf(COLOR "launching sender thread\n");

    //initialize mutexex
    pthread_mutex_init(&mHandle, NULL);
    pthread_mutex_init(&sHandle1, NULL);
    pthread_mutex_init(&sHandle2, NULL);
    //lock sync, as initially there is nothing to see
    pthread_mutex_lock(&sHandle1);
    pthread_mutex_lock(&sHandle2);

    //time to split
    if( pthread_create( &thread, NULL, _thrd_wrapper_function, (void*)&i ) != 0 )
    {
        perror( COLOR "thread creation failed; exiting!" );
        exit(0);
    }

    printf(COLOR "beginning the traffic\n");

    //this is to be done multiple times
    for ( i = 0; i < ITERATIONS; ++i )
    {
        //retrieve stuff    
        mcapi_pktchan_recv( handy1, (void*)&recv_buf, &size, &status );
        check( MCAPI_SUCCESS, status );

        //obtain lock
        pthread_mutex_lock(&mHandle);

        //pass the buffer on
        mid_buf = recv_buf;

        //done: release lock and sync
        pthread_mutex_unlock(&mHandle);
        pthread_mutex_unlock(&sHandle1);

        //but obtain the other sync
        pthread_mutex_lock(&sHandle2);
    }

    printf(COLOR "receiving done, waiting the other thread\n");

    //join the other thread so that we dont kill anything prematurely
    pthread_join(thread, NULL);

    printf(COLOR "traffic done, closing\n");

    //close our ends
    mcapi_pktchan_recv_close_i( handy1, &request1, &status );
    check( MCAPI_PENDING, status );
    mcapi_pktchan_send_close_i( handy2, &request2, &status );
    check( MCAPI_PENDING, status );
    //wait for them to happen, IN RIGHT ORDER: send before receive
    //sides of communication must do these in reverse order or deadlock
    mcapi_wait( &request2, &size, TIMEOUT, &status );
    check( MCAPI_SUCCESS, status );
    mcapi_wait( &request1, &size, TIMEOUT, &status );
    check( MCAPI_SUCCESS, status );

    printf(COLOR "closed, shutdown\n");

    //finalize at the end
    mcapi_finalize( &status );
    check( MCAPI_SUCCESS, status );

    //time to delete the mutexes
    pthread_mutex_destroy(&mHandle);
    pthread_mutex_destroy(&sHandle1);
    pthread_mutex_destroy(&sHandle2);

    return EXIT_SUCCESS;
}
