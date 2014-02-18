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
    //request handle is used to operate wait-calls
    mcapi_request_t request;
    //size parameter required in some calls
    size_t size;
    //send-handle used in channel-messaging
    mcapi_sclchan_send_hndl_t handy;
    //number to send in communication
    short value = 0xAACC;
    //the endpoints used in communication: Both ends are needed when connecting
    mcapi_endpoint_t send_point;
    mcapi_endpoint_t recv_point;

    printf( "Node 0: Sender here!\n");
    //sleep to better illustrate the communication
    usleep( 3000000 );

    //We are sender! Onitialize with our domain id and node id
    mcapi_initialize( THE_DOMAIN, RED_NODE, 0, 0, &info,
    &status );

    //THIS IS HOW YOU CONVERT STATUS CODE TO STRING:
    mcapi_display_status( status, status_msg, MCAPI_MAX_STATUS_MSG_LEN );
    //print it:
    printf( "Node 0: Result of initialization: %s\n", status_msg );

    printf( "Node 0: creating sending endpoint\n" );

    //create our channel endpoint with our port id
    send_point = mcapi_endpoint_create( RED_SCL_OUT, &status );
    //get their channel message endpoint, with their domain, node and port id
    printf( "Node 0: obtaining receiving endpoint\n" );
    recv_point = mcapi_endpoint_get( THE_DOMAIN, YELLOW_NODE, YELLOW_SCL_IN,
    TIMEOUT, &status );

    //form the scalar channel
    printf( "Node 0: connecting the channel\n" );
    mcapi_sclchan_connect_i( send_point, recv_point, &request, &status );
    //wait for it to happen
    mcapi_wait( &request, &size, TIMEOUT, &status );
    //open our end of it
    printf( "Node 0: opening the sending end of the channel\n" );
    mcapi_sclchan_send_open_i( &handy, send_point, &request, &status );
    //wait for it to happen
    mcapi_wait( &request, &size, TIMEOUT, &status );

    printf( "Node 0: sending %hX\n", value );
    
    //send the scalar value via channel
    mcapi_sclchan_send_uint16( handy, value, &status );

    printf( "Node 0: sent\n" );

    usleep( 3000000 );

    printf( "Node 0: closing!\n" );

    //close our end
    mcapi_sclchan_send_close_i( handy, &request, &status );
    //wait for it to happen
    mcapi_wait( &request, &size, TIMEOUT, &status );

    printf( "Node 0: closed!\n" );

    //finalize at the end
    mcapi_finalize( &status );

    return EXIT_SUCCESS;
}
