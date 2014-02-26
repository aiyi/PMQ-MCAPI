#include <stdlib.h>
#include <string.h>
#include <mcapi.h>
#include <stdio.h>

#define MAX_MSG_LEN     30

#define check( excpt, got ) \
    if ( excpt != got ) { \
        char status_msg[MCAPI_MAX_STATUS_MSG_LEN]; \
        mcapi_display_status( got, status_msg, MCAPI_MAX_STATUS_MSG_LEN ); \
        printf( "FAILURE AT %s, %u: got: %s expected: %s\n", __FILE__, \
        __LINE__, status_msg, #excpt ); \
        exit(0); }

#define COLOR "\e[1msturdy\e[0m: "

#define TIMEOUT 1000

#define ITERATIONS 500

int main()
{
    //status message received in almost all MCAPI-calls
    mcapi_status_t status;
    //info-struct received in initialization
    mcapi_info_t info;
    //the status code converted to string
    char status_msg[MCAPI_MAX_STATUS_MSG_LEN];
    //an iterator used in loops
    unsigned int i = 0;
    //request handle is used to operate wait-calls
    mcapi_request_t request;
    //size parameter required in some calls
    size_t size = 1;
    //send-handle used in channel-messaging
    mcapi_pktchan_send_hndl_t handy;
    //pid is used to identify process
    pid_t pid;
    //get our stuff here. tip: if signed were used, it bastardized the values
    unsigned char* recv_buf;
    //the timeout of our receive endpoint
    mcapi_timeout_t timeut = 10;
    //how many we sent successfully
    unsigned int success = 0;

    //the endpoints used in channel-oriented communication
    mcapi_endpoint_t sturdy_point;
    mcapi_endpoint_t flimsy_point;
    //the identifiers of above endpoints
    struct endPointID sturdy_id = STURDY;
    struct endPointID flimsy_id = FLIMSY;

    printf( COLOR "here\n");

    //we are the green
    mcapi_initialize( sturdy_id.domain_id, sturdy_id.node_id, 0, 0, &info, &status );
    check( MCAPI_SUCCESS, status );

    //create our channel message endpoint
    sturdy_point = mcapi_endpoint_create( sturdy_id.port_id, &status );
    check( MCAPI_SUCCESS, status );
    //set timeout for it
    mcapi_endpoint_set_attribute( sturdy_point, MCAPI_ENDP_ATTR_TIMEOUT, &timeut,
    sizeof(mcapi_timeout_t), &status );
    check( MCAPI_SUCCESS, status );
    //get their channel message endpoint
    flimsy_point = mcapi_endpoint_get( flimsy_id.domain_id,
    flimsy_id.node_id, flimsy_id.port_id, TIMEOUT, &status );
    check( MCAPI_SUCCESS, status );

    //form the channel
    mcapi_pktchan_connect_i( flimsy_point, sturdy_point, &request, &status );
    check( MCAPI_PENDING, status );
    //wait for it to happen
    mcapi_wait( &request, &size, TIMEOUT, &status );
    check( MCAPI_SUCCESS, status );
    //open our end
    mcapi_pktchan_recv_open_i( &handy, sturdy_point, &request, &status );
    check( MCAPI_PENDING, status );
    //wait for it to happen
    mcapi_wait( &request, &size, TIMEOUT, &status );
    check( MCAPI_SUCCESS, status );

    printf(COLOR "beginning the receivals\n");

    //this is to be done multiple times
    for ( i = 0; i < ITERATIONS; ++i )
    {
        //start to receive our stuff
        mcapi_pktchan_recv( handy, (void*)&recv_buf, &size, &status );
        
        if ( status != MCAPI_SUCCESS )
        {
            //must close & reconnect propably
            mcapi_pktchan_recv_close_i( handy, &request, &status );
            mcapi_wait( &request, &size, TIMEOUT, &status );
            mcapi_pktchan_connect_i( flimsy_point, sturdy_point, &request, &status );
            mcapi_wait( &request, &size, TIMEOUT, &status );
            check( MCAPI_SUCCESS, status );
            //and reopen
            mcapi_pktchan_recv_open_i( &handy, sturdy_point, &request, &status );
            mcapi_wait( &request, &size, TIMEOUT, &status );

            //timeout AGAIN -> go home
            if ( status == MCAPI_TIMEOUT )
            {
                break;
            }

            //printf(COLOR "reconnection success\n" );
        }
        else
        {
            //mark success
            ++success;

            //release
            mcapi_pktchan_release( recv_buf, &status );
            check( MCAPI_SUCCESS, status );
        }
    }
    
    printf(COLOR " done, closing with %u successful\n", success);

    //close our end
    mcapi_pktchan_recv_close_i( handy, &request, &status );
    //wait for it to happen
    mcapi_wait( &request, &size, TIMEOUT, &status );

    printf(COLOR "closed, release and shut down\n");

    //release
    mcapi_pktchan_release( recv_buf, &status );

    //finalize at the end, regardless of which process we are
    mcapi_finalize( &status );

    return EXIT_SUCCESS;
}
