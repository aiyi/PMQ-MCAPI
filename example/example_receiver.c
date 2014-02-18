#include <stdlib.h>
#include <mcapi.h>
#include <stdio.h>

//Wait about five seconds before timeout
#define TIMEOUT 5000
//NOTICE: other defines, like THE_DOMAIN are defines in file ../include/endpointlist.h!

int main()
{
    //status message received in almost all MCAPI-calls
    mcapi_status_t status;
    //info-struct received in initialization
    mcapi_info_t info;
    //the status code converted to string
    char status_msg[MCAPI_MAX_STATUS_MSG_LEN];
    //size parameter required in some calls
    size_t size;
    //request handle is used to operate wait-calls
    mcapi_request_t request;
    //value received in communication
    short value = 0;
    //the receiving end point of the communication
    mcapi_endpoint_t recv_point;
    //handle used in channel communication
    mcapi_sclchan_recv_hndl_t handy;

    printf( "Receiver here!\n");

    //We are receiver! Initialize with our domain and node id
    mcapi_initialize( THE_DOMAIN, YELLOW_NODE, 0, 0, &info, &status );

    //THIS IS HOW YOU CONVERT STATUS CODE TO STRING:
    mcapi_display_status( status, status_msg, MCAPI_MAX_STATUS_MSG_LEN );
    //print it:
    printf( "Result of receiver initialization: %s\n", status_msg );

    //create our end point with our port id
    recv_point = mcapi_endpoint_create( YELLOW_SCL_IN, &status );
    //open our end of channel, let sender form connection
    mcapi_sclchan_recv_open_i( &handy, recv_point, &request, &status );
    //wait for it to happen
    mcapi_wait( &request, &size, TIMEOUT, &status );

    printf( "Receiver: receiving\n" );

    //receive a scalar value from channel
    value = mcapi_sclchan_recv_uint16( handy, &status );

    printf( "Receiver: received %hX\n", value );

    //close our end
    mcapi_sclchan_recv_close_i( handy, &request, &status );
    //wait for it to happen
    mcapi_wait( &request, &size, TIMEOUT, &status );

    //shut-down
    mcapi_finalize( &status );

    return EXIT_SUCCESS;
}
