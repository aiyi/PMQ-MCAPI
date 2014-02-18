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

#define COLOR "\e[96mcyan\e[0m: "

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
//buffer of data sent in messages
unsigned char* send_buf;
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
//is one if traffic was ok
char ok = 1;

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

    printf(COLOR "receiver thread launched\n");

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

        //retrieve stuff  
        mcapi_pktchan_recv( handy1, (void*)&recv_buf, &size, &status );
        check( MCAPI_SUCCESS, status );
        
        //check they are consistent
        for ( j = 0; j < MCAPI_MAX_PKT_SIZE; ++j )
        {
            //each byte is supposed to be incremented by one from original.
            unsigned char comp = use_buf[j] + 1;

            if ( comp != recv_buf[j] )
            {
                ok = 0;
            }
        }

        //release
        mcapi_pktchan_release( recv_buf, &status );
        check( MCAPI_SUCCESS, status );

        //free the usebuf as well, as it was dynamically reserved
        free( use_buf );
    }

    printf(COLOR "receiving done, closing this thread\n");

    return 0;
}

int main()
{
    //an iterator used in loops
    unsigned int i = 0;

    //the identifiers of endpoints mentioned earlier in module
    struct endPointID id_mag_cyan_send = MAG_CYAN_SEND;
    struct endPointID id_mag_cyan_recv = MAG_CYAN_RECV;
    struct endPointID id_cyan_mag_send = CYAN_MAG_SEND;
    struct endPointID id_cyan_mag_recv = CYAN_MAG_RECV;

    printf(COLOR "here\n");

    //we are the cyan
    mcapi_initialize( id_mag_cyan_recv.domain_id, id_mag_cyan_recv.node_id, 0,
    0, &info, &status );
    check( MCAPI_SUCCESS, status );

    //create our receiving channel endpoint
    mag_cyan_recv = mcapi_endpoint_create( id_mag_cyan_recv.port_id, &status );
    check( MCAPI_SUCCESS, status );
    //get their sending channel endpoint
    mag_cyan_send = mcapi_endpoint_get( id_mag_cyan_send.domain_id,
    id_mag_cyan_send.node_id, id_mag_cyan_send.port_id, TIMEOUT, &status );
    check( MCAPI_SUCCESS, status );

    //create our sending channel endpoint
    cyan_mag_send = mcapi_endpoint_create( id_cyan_mag_send.port_id, &status );
    check( MCAPI_SUCCESS, status );
    //get their receiving channel endpoint
    cyan_mag_recv = mcapi_endpoint_get( id_cyan_mag_recv.domain_id,
    id_cyan_mag_recv.node_id, id_cyan_mag_recv.port_id, TIMEOUT, &status );
    check( MCAPI_SUCCESS, status );

    //form the sending channel
    mcapi_pktchan_connect_i( cyan_mag_send, cyan_mag_recv, &request2, &status );
    check( MCAPI_PENDING, status );
    //wait for them it happen
    mcapi_wait( &request2, &size, TIMEOUT, &status );
    check( MCAPI_SUCCESS, status );

    //open our ends
    mcapi_pktchan_recv_open_i( &handy1, mag_cyan_recv, &request1, &status );
    check( MCAPI_PENDING, status );
    mcapi_pktchan_send_open_i( &handy2, cyan_mag_send, &request2, &status );
    check( MCAPI_PENDING, status );

    //wait for them to happen, IN RIGHT ORDER: receiver before send
    //sides of communication must do these in reverse order or deadlock
    mcapi_wait( &request1, &size, TIMEOUT, &status );
    check( MCAPI_SUCCESS, status );
    mcapi_wait( &request2, &size, TIMEOUT, &status );
    check( MCAPI_SUCCESS, status );

    printf(COLOR "launching receiver thread\n");

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
    //initialize the random number generation
    srand(i);

    //this is to be done multiple times
    for ( i = 0; i < ITERATIONS; ++i )
    {
        //an iterator used in loops
        unsigned int j = 0;
        //reserve buf in malloc for easier use
        send_buf = (unsigned char*)malloc(MCAPI_MAX_PKT_SIZE);

        if ( send_buf == NULL )
        {
            perror( COLOR "malloc failed; exiting!" );
            exit(0);
        }

        //populate packet with random numbers
        for ( j = 0; j < MCAPI_MAX_PKT_SIZE; ++j )
        {
            send_buf[j] = rand();
        }

        //send the packet
        mcapi_pktchan_send( handy2, send_buf, MCAPI_MAX_PKT_SIZE, &status );

        //obtain lock
        pthread_mutex_lock(&mHandle);

        //pass the buffer on
        mid_buf = send_buf;

        //done: release lock and sync
        pthread_mutex_unlock(&mHandle);
        pthread_mutex_unlock(&sHandle1);

        //but obtain the other sync
        pthread_mutex_lock(&sHandle2);
    }

    printf(COLOR "sending done, waiting the other thread\n");

    //join the other thread so that we dont kill anything prematurely
    pthread_join(thread, NULL);

    //tell about ok
    printf(COLOR "traffic okness: %d\n", ok);

    //close our ends
    mcapi_pktchan_send_close_i( handy2, &request2, &status );
    check( MCAPI_PENDING, status );
    mcapi_pktchan_recv_close_i( handy1, &request1, &status );
    check( MCAPI_PENDING, status );
    //wait for them to happen, IN RIGHT ORDER: send before receive
    //sides of communication must do these in reverse order or deadlock
    mcapi_wait( &request1, &size, TIMEOUT, &status );
    check( MCAPI_SUCCESS, status );
    mcapi_wait( &request2, &size, TIMEOUT, &status );
    check( MCAPI_SUCCESS, status );

    printf(COLOR "closed, shutdown\n");

    //finalize at the end, regardless of which process we are
    mcapi_finalize( &status );
    check( MCAPI_SUCCESS, status );

    //time to delete the mutexes
    pthread_mutex_destroy(&mHandle);
    pthread_mutex_destroy(&sHandle1);
    pthread_mutex_destroy(&sHandle2);

    return EXIT_SUCCESS;
}
