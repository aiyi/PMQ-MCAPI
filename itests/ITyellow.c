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

#define COLOR "\e[93myellow\e[0m: "

#define TIMEOUT 5000

int main(int argc, char *argv[])
{
    //the endpoints used in message-oriented communication
    mcapi_endpoint_t red_msg_point;
    mcapi_endpoint_t yellow_msg_point;
    mcapi_endpoint_t green_msg_point;
    //the endpoints used in channel-oriented communication
    mcapi_endpoint_t yellow_sin_chan;
    mcapi_endpoint_t yellow_cos_chan;
    mcapi_endpoint_t yellow_pkt_chan;
    //the identifiers of above endpoints
    struct endPointID yellow_msg = YELLOW_MSG;
    struct endPointID yellow_sin = YELLOW_SIN;
    struct endPointID yellow_cos = YELLOW_COS;
    struct endPointID red_msg = RED_MSG;
    struct endPointID green_msg = GREEN_MSG;
    struct endPointID yellow_pkt = YELLOW_PKT;
    //status message received in almost all MCAPI-calls
    mcapi_status_t status;
    //info-struct received in initialization
    mcapi_info_t info;
    //buffer for incoming messages
    char recv_buf[MAX_MSG_LEN];
    //the status code converted to string
    char status_msg[MCAPI_MAX_STATUS_MSG_LEN];
    //size parameter required in some calls
    size_t size;
    //an iterator used in loops
    unsigned int i = 0;
    //request handle is used to operate wait-calls
    mcapi_request_t request;
    //a second request handle! just so that we see it works :)
    mcapi_request_t request2;
    //sandles used in channel-messaging
    mcapi_sclchan_recv_hndl_t sin_handle;
    mcapi_sclchan_recv_hndl_t cos_handle;
    mcapi_pktchan_send_hndl_t pkt_handle;
    //how many scalars we are expecting
    char count = 0;
    //buffer of data sent in messages
    unsigned char* send_buf;

    printf(COLOR "here\n");

    //We are yellow! initialize accordingly
    mcapi_initialize( yellow_msg.domain_id, yellow_msg.node_id, 0, 0, &info, &status );
    check( MCAPI_SUCCESS, status );
    //create our side of messaging
    yellow_msg_point = mcapi_endpoint_create( yellow_msg.port_id, &status );
    check( MCAPI_SUCCESS, status );
    //obtain the red message point
    red_msg_point = mcapi_endpoint_get( red_msg.domain_id,
    red_msg.node_id, red_msg.port_id, TIMEOUT, &status );
    check( MCAPI_SUCCESS, status );

    printf(COLOR "start-up messaging\n");

    //wait for the amount to come
    mcapi_msg_recv( yellow_msg_point, recv_buf, MAX_MSG_LEN, &size,
    &status );
    check( MCAPI_SUCCESS, status );
    //read the count from first byte
    count = recv_buf[0];
    //surprise! this process reserves the buffer with malloc
    send_buf = (char*)malloc(count*2);

    //send ack
    send_buf[0] = 'a';
    mcapi_msg_send( yellow_msg_point, red_msg_point, send_buf, 1,
    0, &status );
    check( MCAPI_SUCCESS, status );

    printf(COLOR "start-up messaged with %u bytes. expecting %u scalars\n",
    size, count );

    //open our channel endpoint to sin
    yellow_sin_chan = mcapi_endpoint_create( yellow_sin.port_id, &status );
    check( MCAPI_SUCCESS, status );
    //open our channel endpoint to cos
    yellow_cos_chan = mcapi_endpoint_create( yellow_cos.port_id, &status );
    check( MCAPI_SUCCESS, status );
    //open our ends, let senders form connection
    mcapi_sclchan_recv_open_i( &sin_handle, yellow_sin_chan, &request, &status );
    check( MCAPI_PENDING, status );
    mcapi_sclchan_recv_open_i( &cos_handle, yellow_cos_chan, &request2, &status );
    check( MCAPI_PENDING, status );
    //wait for it to happen
    mcapi_wait( &request, &size, TIMEOUT, &status );
    check( MCAPI_SUCCESS, status );
    mcapi_wait( &request2, &size, TIMEOUT, &status );
    check( MCAPI_SUCCESS, status );

    printf(COLOR "beginning the receive value\n");

    for ( i = 0; i < 15000; ++i )
    {
        //an iterator used in loops
        unsigned int j = 0;

        //start to receveive stuff
        //i+=2 because our values are two bytes while buf is one byte
        for ( j = 0; j < count; ++j )
        {
            //receive cos scalar
            short cval = mcapi_sclchan_recv_uint16( cos_handle, &status );
            check( MCAPI_SUCCESS, status );
            //receive sin scalar
            short sval = mcapi_sclchan_recv_uint16( sin_handle, &status );
            check( MCAPI_SUCCESS, status );
            //addition
            short sumval = sval + cval;
            //put to buf
            send_buf[j*2] = sumval;
            send_buf[j*2+1] = sumval >> 8;
            //printf( COLOR "%hX %hhX %hhX\n", sumval, send_buf[j*2], send_buf[j*2+1] );
        }
    }

    printf(COLOR "receiving done, closing scalar channels\n");

    //close our ends
    mcapi_sclchan_recv_close_i( sin_handle, &request, &status );
    check( MCAPI_PENDING, status );
    mcapi_sclchan_recv_close_i( cos_handle, &request2, &status );
    check( MCAPI_PENDING, status );
    //wait for it to happen
    mcapi_wait( &request, &size, TIMEOUT, &status );
    check( MCAPI_SUCCESS, status );
    mcapi_wait( &request2, &size, TIMEOUT, &status );
    check( MCAPI_SUCCESS, status );

    printf(COLOR "closed, informing green\n");

    //obtain their endpoint
    green_msg_point = mcapi_endpoint_get( green_msg.domain_id,
    green_msg.node_id, green_msg.port_id, TIMEOUT, &status );
    check( MCAPI_SUCCESS, status );
    //send the message
    mcapi_msg_send( yellow_msg_point, green_msg_point, send_buf, 1,
    0, &status );
    check( MCAPI_SUCCESS, status );

    printf(COLOR "informed, opening packet channel\n");

    //open our channel endpoint to green
    yellow_pkt_chan = mcapi_endpoint_create( yellow_pkt.port_id, &status );
    check( MCAPI_SUCCESS, status );
    //open our end, let receiver form connection
    mcapi_pktchan_send_open_i( &pkt_handle, yellow_pkt_chan, &request, &status );
    check( MCAPI_PENDING, status );
    //wait for it to happen
    mcapi_wait( &request, &size, TIMEOUT, &status );
    check( MCAPI_SUCCESS, status );
    
    //now send
    printf(COLOR "sending the packet\n");
    mcapi_pktchan_send( pkt_handle, send_buf, count, &status );

    //and now close
    printf(COLOR "sent the packet, closing\n");
    mcapi_pktchan_send_close_i( pkt_handle, &request, &status );
    check( MCAPI_PENDING, status );
    mcapi_wait( &request, &size, TIMEOUT, &status );
    check( MCAPI_SUCCESS, status );

    printf(COLOR "closed, shutdown\n");

    //free buf
    free( send_buf );

    //shut-down
    mcapi_finalize( &status );
    check( MCAPI_SUCCESS, status );

    return EXIT_SUCCESS;
}
