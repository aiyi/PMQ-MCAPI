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

#define COLOR "\e[32mgreen\e[0m: "

#define TIMEOUT 5000

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
    //a separate receive buffer used for messages
    char recv_msg[MAX_MSG_LEN];
    //get our stuff here. tip: if signed were used, it bastardized the values
    unsigned char* recv_buf;

    //the endpoints used in channel-oriented communication
    mcapi_endpoint_t green_chan;
    mcapi_endpoint_t yellow_chan;
    mcapi_endpoint_t green_msg_point;

    printf( COLOR "here\n");

    //we are the green
    mcapi_initialize( THE_DOMAIN, GREEN_NODE, 0, 0, &info, &status );
    check( MCAPI_SUCCESS, status );

    //create our message endpoint
    green_msg_point = mcapi_endpoint_create( GREEN_MSG, &status );
    check( MCAPI_SUCCESS, status );
    //wait for the start signal
    mcapi_msg_recv(green_msg_point, recv_msg, MAX_MSG_LEN, &size,
    &status );
    check( MCAPI_SUCCESS, status );
    printf(COLOR "signal received, starting to connect & open\n");

    //create our channel message endpoint
    green_chan = mcapi_endpoint_create( GREEN_PKT, &status );
    check( MCAPI_SUCCESS, status );
    //get their channel message endpoint
    yellow_chan = mcapi_endpoint_get( THE_DOMAIN,
    YELLOW_NODE, YELLOW_PKT, TIMEOUT, &status );
    check( MCAPI_SUCCESS, status );
    //form the channel
    mcapi_pktchan_connect_i( yellow_chan, green_chan, &request, &status );
    check( MCAPI_PENDING, status );
    //wait for it to happen
    mcapi_wait( &request, &size, TIMEOUT, &status );
    check( MCAPI_SUCCESS, status );
    //open our end
    mcapi_pktchan_recv_open_i( &handy, green_chan, &request, &status );
    check( MCAPI_PENDING, status );
    //wait for it to happen
    mcapi_wait( &request, &size, TIMEOUT, &status );
    check( MCAPI_SUCCESS, status );

    printf(COLOR "beginning the receive\n");

    //start to receive our stuff
    mcapi_pktchan_recv( handy, (void*)&recv_buf, &size, &status );
    check( MCAPI_SUCCESS, status );
    
    printf(COLOR "retrieval done, closing\n");

    //close our end
    mcapi_pktchan_recv_close_i( handy, &request, &status );
    check( MCAPI_PENDING, status );
    //wait for it to happen
    mcapi_wait( &request, &size, TIMEOUT, &status );
    check( MCAPI_SUCCESS, status );

    printf(COLOR "closed, go over the values\n");

    //go through that stuff
    //i+=2 because our values are two bytes while buf is one byte
    for ( i = 0; i < size; i+=2 )
    {
        //reconstruct the short
        short sumval = recv_buf[i+1] << 8 | recv_buf[i];
        //printf( COLOR "%hX %hhX %hhX\n", sumval, recv_buf[i], recv_buf[i+1] );
    }

    printf(COLOR "buffer release and shut down\n");

    //release
    mcapi_pktchan_release( recv_buf, &status );
    check( MCAPI_SUCCESS, status );

    //finalize at the end, regardless of which process we are
    mcapi_finalize( &status );
    check( MCAPI_SUCCESS, status );

    return EXIT_SUCCESS;
}
