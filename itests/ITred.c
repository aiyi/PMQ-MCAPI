#include <stdlib.h>
#include <string.h>
#include <mcapi.h>
#include <stdio.h>
#include <math.h>

#define MAX_MSG_LEN     30

#define check( excpt, got ) \
    if ( excpt != got ) { \
        char status_msg[MCAPI_MAX_STATUS_MSG_LEN]; \
        mcapi_display_status( got, status_msg, MCAPI_MAX_STATUS_MSG_LEN ); \
        printf( "FAILURE AT %s, %u: got: %s expected: %s\n", __FILE__, \
        __LINE__, status_msg, #excpt ); \
        exit(0); }

#define COLOR "\e[91mred\e[0m: "

#define TIMEOUT 5000

int main(int argc, char *argv[])
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
    mcapi_sclchan_send_hndl_t handy;
    //how many scalars we are going to send
    char count = 17;

    //the endpoints used in message-oriented communication
    mcapi_endpoint_t red_msg_point;
    mcapi_endpoint_t yellow_msg_point;
    //buffer of data sent in messages
    char send_buf[MAX_MSG_LEN];
    //buffer of data received in messages
    char recv_buf[MAX_MSG_LEN];

    //take the count from arguments, if none given use the default
    if ( argc > 1 )
    {
        count = strtol( argv[1], NULL, 10 );
    }

    printf(COLOR "here\n");

    //We are red! initialize accordingly
    mcapi_initialize( THE_DOMAIN, RED_NODE, 0, 0, &info,
    &status );
    check( MCAPI_SUCCESS, status );

    printf(COLOR "start-up messaging\n");
    //...and thus create our message endpoint
    red_msg_point = mcapi_endpoint_create( RED_MSG, &status );
    check( MCAPI_SUCCESS, status );
    //obtain the yellow message point
    yellow_msg_point = mcapi_endpoint_get( THE_DOMAIN,
    YELLOW_NODE, YELLOW_MSG, TIMEOUT, &status );
    check( MCAPI_SUCCESS, status );

    //now we will tell how many scalars we will send
    send_buf[0] = count;
    mcapi_msg_send( red_msg_point, yellow_msg_point, send_buf, 1,
    0, &status );
    check( MCAPI_SUCCESS, status );

    //wait ack
    mcapi_msg_recv( red_msg_point, recv_buf, MAX_MSG_LEN, &size,
    &status );
    check( MCAPI_SUCCESS, status );

    printf(COLOR "start-up messaged, sending %u scalars\n",
    send_buf[0]);

    //the endpoints used in channel-oriented communication
    mcapi_endpoint_t red_chan;
    mcapi_endpoint_t yellow_chan;

    //create our channel endpoint
    red_chan = mcapi_endpoint_create( RED_SIN, &status );
    check( MCAPI_SUCCESS, status );
    //get their channel message endpoint
    yellow_chan = mcapi_endpoint_get( THE_DOMAIN,
    YELLOW_NODE, YELLOW_SIN, TIMEOUT, &status );
    check( MCAPI_SUCCESS, status );
    //form the channel
    mcapi_sclchan_connect_i( red_chan, yellow_chan, &request, &status );
    check( MCAPI_PENDING, status );
    //wait for it to happen
    mcapi_wait( &request, &size, TIMEOUT, &status );
    check( MCAPI_SUCCESS, status );
    //open our end
    mcapi_sclchan_send_open_i( &handy, red_chan, &request, &status );
    check( MCAPI_PENDING, status );
    //wait for it to happen
    mcapi_wait( &request, &size, TIMEOUT, &status );
    check( MCAPI_SUCCESS, status );

    printf(COLOR "beginning the transfer\n");

    //start to send our stuff
    for ( i = 0; i < 15000; ++i )
    {
        //an iterator used in loops
        unsigned int j = 0;

        //start to send our stuff
        for ( j = 0; j < count; ++j )
        {
            //paramater should be cast and transformed to get nicer results
            double param = (double)j * 3.14159265 / 18;
            //the value produced by sin
            double sinval = sin( param );
            //convert to 1/100 degrees
            double dval = (sinval*180*100)/3.14159265;
            //cast to short
            short sval = (short)dval;
            //printf( "sin producer: %f %d\n", sinval, sval );

            //send as a scalar
            mcapi_sclchan_send_uint16( handy, sval, &status );
            check( MCAPI_SUCCESS, status );
        }
    }

    printf(COLOR "transfer done, closing\n");

    //close our end
    mcapi_sclchan_send_close_i( handy, &request, &status );
    check( MCAPI_PENDING, status );
    //wait for it to happen
    mcapi_wait( &request, &size, TIMEOUT, &status );
    check( MCAPI_SUCCESS, status );

    printf(COLOR "closed, shutdown\n");

    //finalize at the end, regardless of which process we are
    mcapi_finalize( &status );
    check( MCAPI_SUCCESS, status );

    return EXIT_SUCCESS;
}
