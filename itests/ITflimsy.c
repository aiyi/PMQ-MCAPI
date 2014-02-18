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

#define COLOR "\e[2mflimsy\e[0m: "

#define TIMEOUT 1000

#define ITERATIONS 500

int main(int argc, char *argv[])
{
    //status message received in almost all MCAPI-calls
    mcapi_status_t status;
    //info-struct received in initialization
    mcapi_info_t info;
    //buffer for incoming messages
    char send_buf[MAX_MSG_LEN];
    //the status code converted to string
    char status_msg[MCAPI_MAX_STATUS_MSG_LEN];
    //size parameter required in some calls
    size_t size;
    //an iterator used in loops
    unsigned int i = 0;
    //request handle is used to operate wait-calls
    mcapi_request_t request;
    //sandles used in channel-messaging
    mcapi_pktchan_send_hndl_t handy;
    //the endpoints used in channel-oriented communication
    mcapi_endpoint_t flimsy_point;
    //the identifiers of above endpoints
    struct endPointID flimsy_id = FLIMSY;
    //the timeout of our endpoint
    mcapi_timeout_t timeut = 10;
    //how many we sent successfully
    unsigned int success = 0;

    printf(COLOR "here\n");

    //We are yellow! initialize accordingly
    mcapi_initialize( flimsy_id.domain_id, flimsy_id.node_id, 0, 0, &info, &status );
    check( MCAPI_SUCCESS, status );

    //open our channel endpoint
    flimsy_point = mcapi_endpoint_create( flimsy_id.port_id, &status );
    check( MCAPI_SUCCESS, status );
    //set timeout for it
    mcapi_endpoint_set_attribute( flimsy_point, MCAPI_ENDP_ATTR_TIMEOUT, &timeut,
    sizeof(mcapi_timeout_t), &status );
    check( MCAPI_SUCCESS, status );
    //open our end, let receiver form connection
    mcapi_pktchan_send_open_i( &handy, flimsy_point, &request, &status );
    check( MCAPI_PENDING, status );
    //wait for it to happen
    mcapi_wait( &request, &size, TIMEOUT, &status );
    check( MCAPI_SUCCESS, status );
    
    //initialize the random number generation
    srand(i);
    
    printf(COLOR "starting sends\n");

    usleep(10000);

    //this is to be done multiple times
    for ( i = 0; i < ITERATIONS; ++i )
    {
        //make a random
        unsigned int joku = rand() % 100;

        //now send
        mcapi_pktchan_send( handy, send_buf, MAX_MSG_LEN, &status );

        if ( status != MCAPI_SUCCESS )
        {
            mcapi_display_status( status, status_msg, MCAPI_MAX_STATUS_MSG_LEN );

            //we shall finish our close
            mcapi_wait( &request, &size, TIMEOUT, &status );
            check( MCAPI_SUCCESS, status );
            //and are forced to wait for reopen
            mcapi_pktchan_send_open_i( &handy, flimsy_point, &request, &status );
            mcapi_wait( &request, &size, TIMEOUT, &status );
            check( MCAPI_SUCCESS, status );

            //printf(COLOR "reconnection success\n" );
        }
        else
        {
            //success deservess to be marked
            ++success;

            if ( joku > 98 )
            {
                //disconnect if it feels like that!
                printf(COLOR "CLOSING! %u\n", joku);
                mcapi_pktchan_send_close_i( handy, &request, &status );
            }
        }
    }

    //and now close
    printf(COLOR "closing with %u successful\n", success);
    mcapi_pktchan_send_close_i( handy, &request, &status );
    mcapi_wait( &request, &size, TIMEOUT, &status );
    check( MCAPI_SUCCESS, status );

    printf(COLOR "closed, shutdown\n");

    //shut-down
    mcapi_finalize( &status );
    check( MCAPI_SUCCESS, status );

    return EXIT_SUCCESS;
}
