#include <stdlib.h>
#include <mcapi.h>
#include <stdio.h>

//Wait about five seconds before timeout
#define TIMEOUT 5000
//NOTICE: other defines, like THE_DOMAIN are defines in file ../include/endpointlist.h!

//The name of the node
#define NAME "RECV: "

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

    printf( NAME "Receiver here!\n");
    //sleep to better illustrate the communication
    usleep( 1500000 );

    //We are receiver! Initialize with our domain and node id
    mcapi_initialize( THE_DOMAIN, YELLOW_NODE, 0, 0, &info, &status );

    //THIS IS HOW YOU CONVERT STATUS CODE TO STRING:
    mcapi_display_status( status, status_msg, MCAPI_MAX_STATUS_MSG_LEN );
    //print it:
    printf( NAME "Result of initialization: %s\n", status_msg );
    usleep( 1500000 );

    //create our end point with our port id
    printf( NAME "creating receiving endpoint\n" );
    usleep( 1500000 );
    recv_point = mcapi_endpoint_create( YELLOW_SIN, &status );

    usleep( 3000000 );

    //open our end of channel, let sender form connection
    printf( NAME "opening the receiving end of the channel\n" );
    mcapi_sclchan_recv_open_i( &handy, recv_point, &request, &status );
    //wait for it to happen
    mcapi_wait( &request, &size, TIMEOUT, &status );

    printf( NAME "receiving\n" );

    usleep( 3000000 );

    //receive a scalar value from channel
    value = mcapi_sclchan_recv_uint16( handy, &status );

    printf( NAME "received 0x%hX\n", value );

    usleep( 3000000 );

    printf( NAME "closing!\n" );

    //close our end
    mcapi_sclchan_recv_close_i( handy, &request, &status );
    //wait for it to happen
    mcapi_wait( &request, &size, TIMEOUT, &status );

    usleep( 1500000 );
    printf( NAME "closed!\n" );

    //shut-down
    mcapi_finalize( &status );

    return EXIT_SUCCESS;
}
