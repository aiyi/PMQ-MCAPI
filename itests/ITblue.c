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

#define COLOR "\e[94mblue\e[0m: "

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
    //size parameter required in somecalls
    size_t size = 1;
    //send-handle used in channel-messaging
    mcapi_sclchan_send_hndl_t handy;
    //how many scalars we are goint to send
    char count = 17;
    //pid is used to identify process
    pid_t pid;

    //the endpoints used in channel-oriented communication
    mcapi_endpoint_t blue_chan;
    mcapi_endpoint_t yellow_chan;
    //the identifiers of above endpoints
    struct endPointID blue_cos = BLUE_COS;
    struct endPointID yellow_cos = YELLOW_COS;

    //take the count from arguments, if none given use the default
    if ( argc > 1 )
    {
        count = strtol( argv[1], NULL, 10 );
    }

    printf(COLOR "here\n");

    //we are the blue
    mcapi_initialize( blue_cos.domain_id, blue_cos.node_id, 0, 0, &info, &status );
    check( MCAPI_SUCCESS, status );
    //create our channel message endpoint
    blue_chan = mcapi_endpoint_create( blue_cos.port_id, &status );
    check( MCAPI_SUCCESS, status );
    //get their channel message endpoint
    yellow_chan = mcapi_endpoint_get( yellow_cos.domain_id,
    yellow_cos.node_id, yellow_cos.port_id, TIMEOUT, &status );
    check( MCAPI_SUCCESS, status );
    //form the channel
    mcapi_sclchan_connect_i( blue_chan, yellow_chan, &request, &status );
    check( MCAPI_PENDING, status );
    //wait for it to happen
    mcapi_wait( &request, &size, TIMEOUT, &status );
    check( MCAPI_SUCCESS, status );
    //open our end
    mcapi_sclchan_send_open_i( &handy, blue_chan, &request, &status );
    check( MCAPI_PENDING, status );
    //wait for it to happen
    mcapi_wait( &request, &size, TIMEOUT, &status );
    check( MCAPI_SUCCESS, status );

    printf(COLOR "beginning the transfer\n");

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
            double cosval = cos( param );
            //convert to 1/100 degrees
            double dval = (cosval*180*100)/3.14159265;
            //cast to short
            short sval = (short)dval;
            //printf( "cos producer: %f %d\n", cosval, sval );

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
