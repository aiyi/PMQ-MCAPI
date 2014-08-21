#include <stdlib.h>
#include <mcapi.h>
#include <stdio.h>

//Wait about five seconds before timeout
#define TIMEOUT 5000
//NOTICE: other defines, like THE_DOMAIN are defines in file ../include/endpointlist.h!

//The name of the node
#define NAME "SEND: "

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

    printf( NAME "Sender here!\n");
    //sleep to better illustrate the communication
    usleep( 3000000 );

    //We are sender! Initialize with our domain id and node id
    mcapi_initialize( THE_DOMAIN, RED_NODE, 0, 0, &info,
    &status );

    //THIS IS HOW YOU CONVERT STATUS CODE TO STRING:
    mcapi_display_status( status, status_msg, MCAPI_MAX_STATUS_MSG_LEN );
    //print it:
    printf( NAME "Result of initialization: %s\n", status_msg );
    usleep( 1500000 );

    printf( NAME "creating sending endpoint\n" );
    usleep( 1500000 );

    //create our channel endpoint with our port id
    send_point = mcapi_endpoint_create( RED_SIN, &status );
    //get their channel message endpoint, with their domain, node and port id
    printf( NAME "obtaining receiving endpoint\n" );
    recv_point = mcapi_endpoint_get( THE_DOMAIN, YELLOW_NODE, YELLOW_SIN,
    TIMEOUT, &status );
    usleep( 1500000 );

    //form the scalar channel
    printf( NAME "connecting the channel\n" );
    mcapi_sclchan_connect_i( send_point, recv_point, &request, &status );
    //wait for it to happen
    mcapi_wait( &request, &size, TIMEOUT, &status );
    //open our end of it
    usleep( 1500000 );
    printf( NAME "opening the sending end of the channel\n" );
    mcapi_sclchan_send_open_i( &handy, send_point, &request, &status );
    //wait for it to happen
    mcapi_wait( &request, &size, TIMEOUT, &status );

    printf( NAME "sending 0x%hX\n", value );
    usleep( 1500000 );
    
    //send the scalar value via channel
    mcapi_sclchan_send_uint16( handy, value, &status );

    printf( NAME "sent\n" );

    usleep( 3000000 );

    printf( NAME "closing!\n" );

    //close our end
    mcapi_sclchan_send_close_i( handy, &request, &status );
    //wait for it to happen
    mcapi_wait( &request, &size, TIMEOUT, &status );

    usleep( 1000000 );
    printf( NAME "closed!\n" );

    //finalize at the end
    mcapi_finalize( &status );

    return EXIT_SUCCESS;
}
