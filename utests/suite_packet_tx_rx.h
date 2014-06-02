//this suite tests packet channel communication
#include <mcapi.h>
#include "utester.h"
#include <string.h>

#define MAX_MSG_LEN     30
#define TEST_MESSAGE "THIS THE THE TEST MESSAGE"

static struct endPointData iepd;
static mcapi_info_t info;
static mcapi_status_t status;
static mcapi_endpoint_t sender;
static mcapi_endpoint_t receiver;
static char send_buf[MAX_MSG_LEN];
static void* recv_buf;
static size_t size;
static pid_t pid;
static mcapi_request_t request;
static mcapi_pktchan_recv_hndl_t nullhand;
static mcapi_timeout_t timeut = 100;

//must be inited
test(pkt_release_fail_init)
    mcapi_pktchan_release( NULL,  &status );
    sassert( MCAPI_ERR_NODE_NOTINIT, status );
}

//must not be null buffer to be released
test(pkt_release_fail_buf)
    mcapi_initialize( 0, 0, 0, 0, &info, &status );
    mcapi_pktchan_release( NULL,  &status );
    sassert( MCAPI_ERR_BUF_INVALID, status );

    mcapi_finalize( &status );
}

//must be inited
test(pkt_send_recv_fail_init)
    mcapi_pktchan_send( nullhand, NULL, MAX_MSG_LEN, &status );
    sassert( MCAPI_ERR_NODE_NOTINIT, status );
    mcapi_pktchan_recv( nullhand, NULL, &size, &status );
    sassert( MCAPI_ERR_NODE_NOTINIT, status );
}

//successfull send & receive
test(pkt_send_recv)

    strncpy(send_buf, TEST_MESSAGE, MAX_MSG_LEN);

    pid = fork();

    if ( pid == 0 )
    {
        mcapi_pktchan_send_hndl_t handy;
        struct endPointID us_id = FOO;
        struct endPointID them_id = BAR;

        mcapi_initialize( us_id.domain_id, us_id.node_id, 0, 0, &info, &status );
        sender = mcapi_endpoint_create( us_id.port_id, &status );

        mcapi_pktchan_send_open_i( &handy, sender, &request, &status );
        sassert( MCAPI_PENDING, status );

        mcapi_wait( &request, &size, 1001, &status );
        sassert( MCAPI_SUCCESS, status );

        mcapi_pktchan_send( handy, send_buf, MAX_MSG_LEN, &status );
        sassert( MCAPI_SUCCESS, status );

        mcapi_finalize( &status );
        exit(0);
    }
    else if ( pid != -1 )
    {
        mcapi_pktchan_recv_hndl_t handy;
        struct endPointID us_id = BAR;
        struct endPointID them_id = FOO;

        mcapi_initialize( us_id.domain_id, us_id.node_id, 0, 0, &info, &status );
        receiver = mcapi_endpoint_create( us_id.port_id, &status );
        sender = mcapi_endpoint_get( them_id.domain_id, them_id.node_id,
        them_id.port_id, 1000, &status );
        
        mcapi_pktchan_connect_i( sender, receiver, &request, &status );
        sassert( MCAPI_PENDING, status );

        mcapi_wait( &request, &size, 1000, &status );
        sassert( MCAPI_SUCCESS, status );

        mcapi_pktchan_recv_open_i( &handy, receiver, &request, &status );
        sassert( MCAPI_PENDING, status );

        mcapi_wait( &request, &size, 1000, &status );
        sassert( MCAPI_SUCCESS, status );

        mcapi_pktchan_recv( handy, &recv_buf, &size, &status );
        sassert( MCAPI_SUCCESS, status );
        uassert( size == MAX_MSG_LEN );
        uassert2( 0, memcmp( send_buf, recv_buf, MAX_MSG_LEN ) );
        mcapi_pktchan_release( recv_buf, &status );
        sassert( MCAPI_SUCCESS, status );

        wait(NULL);

        mcapi_finalize( &status );
    }
    else
    {
        perror("fork");
    }
}

//stuff just stops at middle, must recover
//NOTICE: did not work without timeout in send!
test(pkt_send_recv_fail_trans)

    strncpy(send_buf, TEST_MESSAGE, MAX_MSG_LEN);

    pid = fork();

    if ( pid == 0 )
    {
        mcapi_pktchan_send_hndl_t handy;
        struct endPointID us_id = FOO;
        struct endPointID them_id = BAR;
        unsigned int i = 0;

        mcapi_initialize( us_id.domain_id, us_id.node_id, 0, 0, &info, &status );
        sender = mcapi_endpoint_create( us_id.port_id, &status );

        mcapi_endpoint_set_attribute( sender, MCAPI_ENDP_ATTR_TIMEOUT, &timeut,
        sizeof(mcapi_timeout_t), &status );
        sassert( MCAPI_SUCCESS, status );

        mcapi_pktchan_send_open_i( &handy, sender, &request, &status );
        sassert( MCAPI_PENDING, status );

        mcapi_wait( &request, &size, 1001, &status );
        sassert( MCAPI_SUCCESS, status );

        do
        {
            mcapi_pktchan_send( handy, send_buf, MAX_MSG_LEN, &status );
            ++i; 

            uassert( i < 20 );
        }
        while( status == MCAPI_SUCCESS );

        mcapi_pktchan_send_close_i( handy, &request, &status );
        sassert( MCAPI_PENDING, status );
        mcapi_wait( &request, &size, 1000, &status );
        sassert( MCAPI_SUCCESS, status );

        mcapi_pktchan_send_open_i( &handy, sender, &request, &status );
        sassert( MCAPI_PENDING, status );

        mcapi_wait( &request, &size, 1001, &status );
        sassert( MCAPI_SUCCESS, status );
        
        mcapi_pktchan_send( handy, send_buf, MAX_MSG_LEN, &status );
        sassert( MCAPI_SUCCESS, status );

        mcapi_finalize( &status );
        exit(0);
    }
    else if ( pid != -1 )
    {
        mcapi_pktchan_recv_hndl_t handy;
        struct endPointID us_id = BAR;
        struct endPointID them_id = FOO;

        mcapi_initialize( us_id.domain_id, us_id.node_id, 0, 0, &info, &status );
        receiver = mcapi_endpoint_create( us_id.port_id, &status );
        sender = mcapi_endpoint_get( them_id.domain_id, them_id.node_id,
        them_id.port_id, 1000, &status );
        
        mcapi_pktchan_connect_i( sender, receiver, &request, &status );
        sassert( MCAPI_PENDING, status );

        mcapi_wait( &request, &size, 1000, &status );
        sassert( MCAPI_SUCCESS, status );

        mcapi_pktchan_recv_open_i( &handy, receiver, &request, &status );
        sassert( MCAPI_PENDING, status );

        mcapi_wait( &request, &size, 1000, &status );
        sassert( MCAPI_SUCCESS, status );

        mcapi_pktchan_recv( handy, &recv_buf, &size, &status );
        sassert( MCAPI_SUCCESS, status );
        mcapi_pktchan_release( recv_buf, &status );
        sassert( MCAPI_SUCCESS, status );

        mcapi_pktchan_recv_close_i( handy, &request, &status );
        sassert( MCAPI_PENDING, status );
        mcapi_wait( &request, &size, 1000, &status );
        sassert( MCAPI_SUCCESS, status );

        mcapi_pktchan_connect_i( sender, receiver, &request, &status );
        sassert( MCAPI_PENDING, status );

        mcapi_wait( &request, &size, 1000, &status );
        sassert( MCAPI_SUCCESS, status );

        mcapi_pktchan_recv_open_i( &handy, receiver, &request, &status );
        sassert( MCAPI_PENDING, status );

        mcapi_wait( &request, &size, 1000, &status );
        sassert( MCAPI_SUCCESS, status );

        mcapi_pktchan_recv( handy, &recv_buf, &size, &status );
        sassert( MCAPI_SUCCESS, status );

        wait(NULL);

        mcapi_finalize( &status );
    }
    else
    {
        perror("fork");
    }
}

//timeout send & receive
test(pkt_send_recv_timeout)
    unsigned int i = 0;
    struct timespec t_start, t_end;

    strncpy(send_buf, TEST_MESSAGE, MAX_MSG_LEN);

    pid = fork();

    if ( pid == 0 )
    {
        mcapi_pktchan_send_hndl_t handy;
        struct endPointID us_id = FOO;
        struct endPointID them_id = BAR;

        mcapi_initialize( us_id.domain_id, us_id.node_id, 0, 0, &info, &status );
        sender = mcapi_endpoint_create( us_id.port_id, &status );

        mcapi_endpoint_set_attribute( sender, MCAPI_ENDP_ATTR_TIMEOUT, &timeut,
        sizeof(mcapi_timeout_t), &status );
        sassert( MCAPI_SUCCESS, status );

        mcapi_pktchan_send_open_i( &handy, sender, &request, &status );
        sassert( MCAPI_PENDING, status );

        mcapi_wait( &request, &size, 1001, &status );
        sassert( MCAPI_SUCCESS, status );

        for ( ; i < 10; ++i )
        {
            mcapi_pktchan_send( handy, send_buf, MAX_MSG_LEN, &status );
            sassert( MCAPI_SUCCESS, status );
        }

        clock_gettime( CLOCK_MONOTONIC, &t_start );
        mcapi_pktchan_send( handy, send_buf, MAX_MSG_LEN, &status );
        clock_gettime( CLOCK_MONOTONIC, &t_end );
        sassert( MCAPI_TIMEOUT, status );

        double diff = (double)( t_end.tv_nsec - t_start.tv_nsec );
        diff += ( t_end.tv_sec - t_start.tv_sec ) * 1000000000;
        
        uassert( diff > ( timeut * 1000000 ) );

        mcapi_finalize( &status );
        exit(0);
    }
    else if ( pid != -1 )
    {
        mcapi_pktchan_recv_hndl_t handy;
        struct endPointID us_id = BAR;
        struct endPointID them_id = FOO;

        mcapi_initialize( us_id.domain_id, us_id.node_id, 0, 0, &info, &status );
        receiver = mcapi_endpoint_create( us_id.port_id, &status );
        sender = mcapi_endpoint_get( them_id.domain_id, them_id.node_id,
        them_id.port_id, 1000, &status );

        mcapi_endpoint_set_attribute( receiver, MCAPI_ENDP_ATTR_TIMEOUT, &timeut,
        sizeof(mcapi_timeout_t), &status );
        sassert( MCAPI_SUCCESS, status );
        
        mcapi_pktchan_connect_i( sender, receiver, &request, &status );
        sassert( MCAPI_PENDING, status );

        mcapi_wait( &request, &size, 1000, &status );
        sassert( MCAPI_SUCCESS, status );

        mcapi_pktchan_recv_open_i( &handy, receiver, &request, &status );
        sassert( MCAPI_PENDING, status );

        mcapi_wait( &request, &size, 1000, &status );
        sassert( MCAPI_SUCCESS, status );

        wait(NULL);

        for ( ; i < 10; ++i )
        {
            mcapi_pktchan_recv( handy, &recv_buf, &size, &status );
            sassert( MCAPI_SUCCESS, status );
        }

        clock_gettime( CLOCK_MONOTONIC, &t_start );
        mcapi_pktchan_recv( handy, &recv_buf, &size, &status );
        clock_gettime( CLOCK_MONOTONIC, &t_end );
        sassert( MCAPI_TIMEOUT, status );

        double diff = (double)( t_end.tv_nsec - t_start.tv_nsec );
        diff += ( t_end.tv_sec - t_start.tv_sec ) * 1000000000;
        
        uassert( diff > ( timeut * 1000000 ) );

        mcapi_finalize( &status );
    }
    else
    {
        perror("fork");
    }
}

//cannot delete what is connected
test(dele_con_fail)

    strncpy(send_buf, TEST_MESSAGE, MAX_MSG_LEN);

    pid = fork();

    if ( pid == 0 )
    {
        mcapi_pktchan_send_hndl_t handy;
        struct endPointID us_id = FOO;
        struct endPointID them_id = BAR;

        mcapi_initialize( us_id.domain_id, us_id.node_id, 0, 0, &info, &status );
        sender = mcapi_endpoint_create( us_id.port_id, &status );

        mcapi_pktchan_send_open_i( &handy, sender, &request, &status );
        sassert( MCAPI_PENDING, status );

        mcapi_wait( &request, &size, 1001, &status );
        sassert( MCAPI_SUCCESS, status );

        mcapi_endpoint_delete( sender, &status );
        sassert( MCAPI_ERR_CHAN_CONNECTED, status );

        mcapi_finalize( &status );
        exit(0);
    }
    else if ( pid != -1 )
    {
        mcapi_pktchan_recv_hndl_t handy;
        struct endPointID us_id = BAR;
        struct endPointID them_id = FOO;

        mcapi_initialize( us_id.domain_id, us_id.node_id, 0, 0, &info, &status );
        receiver = mcapi_endpoint_create( us_id.port_id, &status );
        sender = mcapi_endpoint_get( them_id.domain_id, them_id.node_id,
        them_id.port_id, 1000, &status );
        
        mcapi_pktchan_connect_i( sender, receiver, &request, &status );
        sassert( MCAPI_PENDING, status );

        mcapi_wait( &request, &size, 1000, &status );
        sassert( MCAPI_SUCCESS, status );

        mcapi_pktchan_recv_open_i( &handy, receiver, &request, &status );
        sassert( MCAPI_PENDING, status );

        mcapi_wait( &request, &size, 1000, &status );
        sassert( MCAPI_SUCCESS, status );

        mcapi_endpoint_delete( receiver, &status );
        sassert( MCAPI_ERR_CHAN_CONNECTED, status );

        wait(NULL);

        mcapi_finalize( &status );
    }
    else
    {
        perror("fork");
    }
}

//see that a packet of maximum size goes through intact
test(pkt_send_recv_big)
    char send_buf[MCAPI_MAX_PACKET_SIZE];

    unsigned int i;

    srand(send_buf[0]);

    for ( i = 0; i < MCAPI_MAX_PACKET_SIZE; ++i )
    {
        send_buf[i] = rand();
    }

    pid = fork();

    if ( pid == 0 )
    {
        mcapi_pktchan_send_hndl_t handy;
        struct endPointID us_id = FOO;
        struct endPointID them_id = BAR;

        mcapi_initialize( us_id.domain_id, us_id.node_id, 0, 0, &info, &status );
        sender = mcapi_endpoint_create( us_id.port_id, &status );

        mcapi_pktchan_send_open_i( &handy, sender, &request, &status );
        mcapi_wait( &request, &size, 1001, &status );

        mcapi_pktchan_send( handy, send_buf, MCAPI_MAX_PACKET_SIZE, &status );
        sassert( MCAPI_SUCCESS, status );

        mcapi_finalize( &status );
        exit(0);
    }
    else if ( pid != -1 )
    {
        mcapi_pktchan_recv_hndl_t handy;
        struct endPointID us_id = BAR;
        struct endPointID them_id = FOO;

        mcapi_initialize( us_id.domain_id, us_id.node_id, 0, 0, &info, &status );
        receiver = mcapi_endpoint_create( us_id.port_id, &status );
        sender = mcapi_endpoint_get( them_id.domain_id, them_id.node_id,
        them_id.port_id, 1000, &status );
        
        mcapi_pktchan_connect_i( sender, receiver, &request, &status );
        mcapi_wait( &request, &size, 1001, &status );
        mcapi_pktchan_recv_open_i( &handy, receiver, &request, &status );
        mcapi_wait( &request, &size, 1000, &status );

        mcapi_pktchan_recv( handy, &recv_buf, &size, &status );
        sassert( MCAPI_SUCCESS, status );
        uassert( size == MCAPI_MAX_PACKET_SIZE );
        uassert2( 0, memcmp( send_buf, recv_buf, MCAPI_MAX_PACKET_SIZE ) );
        mcapi_pktchan_release( recv_buf, &status );
        sassert( MCAPI_SUCCESS, status );

        wait(NULL);

        mcapi_finalize( &status );
    }
    else
    {
        perror("fork");
    }
}

//see that even a tiny packet works
test(pkt_send_recv_small)
    char send_buf[1];

    send_buf[0] = 'b';

    pid = fork();

    if ( pid == 0 )
    {
        mcapi_pktchan_send_hndl_t handy;
        struct endPointID us_id = FOO;
        struct endPointID them_id = BAR;

        mcapi_initialize( us_id.domain_id, us_id.node_id, 0, 0, &info, &status );
        sender = mcapi_endpoint_create( us_id.port_id, &status );

        mcapi_pktchan_send_open_i( &handy, sender, &request, &status );
        mcapi_wait( &request, &size, 1001, &status );
        mcapi_pktchan_send( handy, send_buf, 1, &status );
        sassert( MCAPI_SUCCESS, status );

        mcapi_finalize( &status );
        exit(0);
    }
    else if ( pid != -1 )
    {
        mcapi_pktchan_recv_hndl_t handy;
        struct endPointID us_id = BAR;
        struct endPointID them_id = FOO;

        mcapi_initialize( us_id.domain_id, us_id.node_id, 0, 0, &info, &status );
        receiver = mcapi_endpoint_create( us_id.port_id, &status );
        sender = mcapi_endpoint_get( them_id.domain_id, them_id.node_id,
        them_id.port_id, 1000, &status );
        
        mcapi_pktchan_connect_i( sender, receiver, &request, &status );
        mcapi_wait( &request, &size, 1001, &status );
        mcapi_pktchan_recv_open_i( &handy, receiver, &request, &status );
        mcapi_wait( &request, &size, 1000, &status );

        mcapi_pktchan_recv( handy, &recv_buf, &size, &status );
        sassert( MCAPI_SUCCESS, status );
        uassert( size == 1 );
        uassert2( 0, memcmp( send_buf, recv_buf, 1 ) );
        mcapi_pktchan_release( recv_buf, &status );
        sassert( MCAPI_SUCCESS, status );

        wait(NULL);

        mcapi_finalize( &status );
    }
    else
    {
        perror("fork");
    }
}

//see that even zero-length packet works
test(pkt_send_recv_zero)
    pid = fork();

    if ( pid == 0 )
    {
        mcapi_pktchan_send_hndl_t handy;
        struct endPointID us_id = FOO;
        struct endPointID them_id = BAR;

        mcapi_initialize( us_id.domain_id, us_id.node_id, 0, 0, &info, &status );
        sender = mcapi_endpoint_create( us_id.port_id, &status );
        mcapi_pktchan_send_open_i( &handy, sender, &request, &status );
        mcapi_wait( &request, &size, 1001, &status );

        mcapi_pktchan_send( handy, send_buf, 0, &status );
        sassert( MCAPI_SUCCESS, status );

        mcapi_finalize( &status );
        exit(0);
    }
    else if ( pid != -1 )
    {
        mcapi_pktchan_recv_hndl_t handy;
        struct endPointID us_id = BAR;
        struct endPointID them_id = FOO;

        mcapi_initialize( us_id.domain_id, us_id.node_id, 0, 0, &info, &status );
        receiver = mcapi_endpoint_create( us_id.port_id, &status );
        sender = mcapi_endpoint_get( them_id.domain_id, them_id.node_id,
        them_id.port_id, 1000, &status );
        
        mcapi_pktchan_connect_i( sender, receiver, &request, &status );
        mcapi_wait( &request, &size, 1001, &status );

        mcapi_pktchan_recv_open_i( &handy, receiver, &request, &status );
        mcapi_wait( &request, &size, 1000, &status );
        mcapi_pktchan_recv( handy, &recv_buf, &size, &status );
        sassert( MCAPI_SUCCESS, status );
        uassert( size == 0 );
        mcapi_pktchan_release( recv_buf, &status );
        sassert( MCAPI_SUCCESS, status );

        wait(NULL);

        mcapi_finalize( &status );
    }
    else
    {
        perror("fork");
    }
}

//a sub-zero length should not work
test(pkt_send_recv_sub_zero)
    pid = fork();

    if ( pid == 0 )
    {
        mcapi_pktchan_send_hndl_t handy;
        struct endPointID us_id = FOO;
        struct endPointID them_id = BAR;

        mcapi_initialize( us_id.domain_id, us_id.node_id, 0, 0, &info, &status );
        sender = mcapi_endpoint_create( us_id.port_id, &status );
        mcapi_pktchan_send_open_i( &handy, sender, &request, &status );
        mcapi_wait( &request, &size, 1001, &status );

        mcapi_pktchan_send( handy, send_buf, -1, &status );
        sassert( MCAPI_ERR_PKT_SIZE, status );

        mcapi_finalize( &status );
        exit(0);
    }
    else if ( pid != -1 )
    {
        mcapi_pktchan_recv_hndl_t handy;
        struct endPointID us_id = BAR;
        struct endPointID them_id = FOO;

        mcapi_initialize( us_id.domain_id, us_id.node_id, 0, 0, &info, &status );
        receiver = mcapi_endpoint_create( us_id.port_id, &status );
        sender = mcapi_endpoint_get( them_id.domain_id, them_id.node_id,
        them_id.port_id, 1000, &status );
        mcapi_pktchan_connect_i( sender, receiver, &request, &status );
        mcapi_wait( &request, &size, 1001, &status );
        mcapi_pktchan_recv_open_i( &handy, receiver, &request, &status );

        mcapi_wait( &request, &size, 1000, &status );
        sassert( MCAPI_SUCCESS, status );

        wait(NULL);

        mcapi_finalize( &status );
    }
    else
    {
        perror("fork");
    }
}

//null buffer should not be accepted by functions
test(pkt_send_inva_buffer)
    pid = fork();

    if ( pid == 0 )
    {
        mcapi_pktchan_send_hndl_t handy;
        struct endPointID us_id = FOO;
        struct endPointID them_id = BAR;

        mcapi_initialize( us_id.domain_id, us_id.node_id, 0, 0, &info, &status );
        sender = mcapi_endpoint_create( us_id.port_id, &status );
        mcapi_pktchan_send_open_i( &handy, sender, &request, &status );
        mcapi_wait( &request, &size, 1001, &status );

        mcapi_pktchan_send( handy, MCAPI_NULL, MAX_MSG_LEN, &status );
        sassert( MCAPI_ERR_PARAMETER, status );

        mcapi_finalize( &status );
        exit(0);
    }
    else if ( pid != -1 )
    {
        mcapi_pktchan_recv_hndl_t handy;
        struct endPointID us_id = BAR;
        struct endPointID them_id = FOO;

        mcapi_initialize( us_id.domain_id, us_id.node_id, 0, 0, &info, &status );
        receiver = mcapi_endpoint_create( us_id.port_id, &status );
        sender = mcapi_endpoint_get( them_id.domain_id, them_id.node_id,
        them_id.port_id, 1000, &status );
        mcapi_pktchan_connect_i( sender, receiver, &request, &status );
        mcapi_wait( &request, &size, 1001, &status );
        mcapi_pktchan_recv_open_i( &handy, receiver, &request, &status );
        mcapi_wait( &request, &size, 1001, &status );

        mcapi_pktchan_recv( handy, NULL, &size, &status );
        sassert( MCAPI_ERR_PARAMETER, status );

        wait(NULL);

        mcapi_finalize( &status );
    }
    else
    {
        perror("fork");
    }
}

//node must be inited before use
test(pkt_send_recv_init)
    mcapi_pktchan_send( nullhand, &send_buf, 0, &status );
    sassert( MCAPI_ERR_NODE_NOTINIT, status );
    mcapi_pktchan_recv( nullhand, &recv_buf, &size, &status );
    sassert( MCAPI_ERR_NODE_NOTINIT, status );
}

//too large packet should be rejected
test(pkt_send_too_big)
    mcapi_pktchan_recv_hndl_t handy;
    struct endPointID us_id = BAR;
    struct endPointID them_id = FOO;

    mcapi_initialize( us_id.domain_id, us_id.node_id, 0, 0, &info, &status );

    mcapi_pktchan_send( handy, &send_buf, MCAPI_MAX_PACKET_SIZE+1, &status );
    sassert( MCAPI_ERR_PKT_SIZE, status );

    mcapi_finalize( &status );
}

//channel must accessed via valid handle
test(pkt_send_recv_inva_chan)

    strncpy(send_buf, TEST_MESSAGE, MAX_MSG_LEN);

    pid = fork();

    if ( pid == 0 )
    {
        mcapi_pktchan_send_hndl_t handy;
        struct endPointID us_id = FOO;
        struct endPointID them_id = BAR;

        mcapi_initialize( us_id.domain_id, us_id.node_id, 0, 0, &info, &status );
        sender = mcapi_endpoint_create( us_id.port_id, &status );
        receiver = mcapi_endpoint_get( them_id.domain_id, them_id.node_id,
        them_id.port_id, 1000, &status );

        mcapi_pktchan_send_open_i( &handy, sender, &request, &status );
        sassert( MCAPI_PENDING, status );

        mcapi_wait( &request, &size, 1001, &status );
        sassert( MCAPI_SUCCESS, status );

        handy.us = receiver;
        mcapi_pktchan_send( handy, send_buf, MAX_MSG_LEN, &status );
        sassert( MCAPI_ERR_CHAN_INVALID, status );

        mcapi_finalize( &status );
        exit(0);
    }
    else if ( pid != -1 )
    {
        mcapi_pktchan_recv_hndl_t handy;
        struct endPointID us_id = BAR;
        struct endPointID them_id = FOO;

        mcapi_initialize( us_id.domain_id, us_id.node_id, 0, 0, &info, &status );
        receiver = mcapi_endpoint_create( us_id.port_id, &status );
        sender = mcapi_endpoint_get( them_id.domain_id, them_id.node_id,
        them_id.port_id, 1000, &status );
        
        mcapi_pktchan_connect_i( sender, receiver, &request, &status );
        sassert( MCAPI_PENDING, status );
        mcapi_wait( &request, &size, 1000, &status );
        sassert( MCAPI_SUCCESS, status );

        mcapi_pktchan_recv_open_i( &handy, receiver, &request, &status );
        sassert( MCAPI_PENDING, status );

        mcapi_wait( &request, &size, 1000, &status );
        sassert( MCAPI_SUCCESS, status );

        handy.us = sender;
        mcapi_pktchan_recv( handy, &recv_buf, &size, &status );
        sassert( MCAPI_ERR_CHAN_INVALID, status );

        wait(NULL);

        mcapi_finalize( &status );
    }
    else
    {
        perror("fork");
    }
}

//test if every buffer possible may be obtained, then released and then obtained
//again. Will not release second time, as next initialization will free everthing.
test(pkt_buffer_stress)

    strncpy(send_buf, TEST_MESSAGE, MAX_MSG_LEN);

    pid = fork();

    unsigned int i;

    if ( pid == 0 )
    {
        mcapi_pktchan_send_hndl_t handy;
        struct endPointID us_id = FOO;
        struct endPointID them_id = BAR;

        mcapi_initialize( us_id.domain_id, us_id.node_id, 0, 0, &info, &status );
        sender = mcapi_endpoint_create( us_id.port_id, &status );

        mcapi_pktchan_send_open_i( &handy, sender, &request, &status );
        sassert( MCAPI_PENDING, status );

        mcapi_wait( &request, &size, 1001, &status );
        sassert( MCAPI_SUCCESS, status );

        for ( i = 0; i < MCAPI_MAX_BUFFERS*2; ++i )
        {
            mcapi_pktchan_send( handy, send_buf, MAX_MSG_LEN, &status );
            sassert( MCAPI_SUCCESS, status );
        }

        mcapi_finalize( &status );
        exit(0);
    }
    else if ( pid != -1 )
    {
        mcapi_pktchan_recv_hndl_t handy;
        struct endPointID us_id = BAR;
        struct endPointID them_id = FOO;
        void* recv_buf[MCAPI_MAX_BUFFERS];

        mcapi_initialize( us_id.domain_id, us_id.node_id, 0, 0, &info, &status );
        receiver = mcapi_endpoint_create( us_id.port_id, &status );
        sender = mcapi_endpoint_get( them_id.domain_id, them_id.node_id,
        them_id.port_id, 1000, &status );
        
        mcapi_pktchan_connect_i( sender, receiver, &request, &status );
        sassert( MCAPI_PENDING, status );
        mcapi_wait( &request, &size, 1001, &status );
        sassert( MCAPI_SUCCESS, status );

        mcapi_pktchan_recv_open_i( &handy, receiver, &request, &status );
        sassert( MCAPI_PENDING, status );

        mcapi_wait( &request, &size, 1000, &status );
        sassert( MCAPI_SUCCESS, status );

        for ( i = 0; i < MCAPI_MAX_BUFFERS; ++i )
        {
            mcapi_pktchan_recv( handy, &recv_buf[i], &size, &status );
            sassert( MCAPI_SUCCESS, status );
            uassert( size == MAX_MSG_LEN );
            uassert2( 0, memcmp( send_buf, recv_buf[i], MAX_MSG_LEN ) );
        }

        for ( i = 0; i < MCAPI_MAX_BUFFERS; ++i )
        {
            mcapi_pktchan_release( recv_buf[i], &status );
            sassert( MCAPI_SUCCESS, status );
        }

        for ( i = 0; i < MCAPI_MAX_BUFFERS; ++i )
        {
            mcapi_pktchan_recv( handy, &recv_buf[i], &size, &status );
            sassert( MCAPI_SUCCESS, status );
            uassert( size == MAX_MSG_LEN );
            uassert2( 0, memcmp( send_buf, recv_buf[i], MAX_MSG_LEN ) );
        }

        wait(NULL);

        mcapi_finalize( &status );
    }
    else
    {
        perror("fork");
    }
}

//see if close works
test(pkt_close)

    strncpy(send_buf, TEST_MESSAGE, MAX_MSG_LEN);

    pid = fork();

    if ( pid == 0 )
    {
        mcapi_pktchan_send_hndl_t handy;
        struct endPointID us_id = FOO;
        struct endPointID them_id = BAR;

        mcapi_initialize( us_id.domain_id, us_id.node_id, 0, 0, &info, &status );
        sender = mcapi_endpoint_create( us_id.port_id, &status );

        mcapi_pktchan_send_open_i( &handy, sender, &request, &status );
        mcapi_wait( &request, &size, 1001, &status );
        mcapi_pktchan_send( handy, send_buf, MAX_MSG_LEN, &status );

        mcapi_pktchan_send_close_i( handy, &request, &status );
        sassert( MCAPI_PENDING, status );
        mcapi_wait( &request, &size, 60000, &status );
        sassert( MCAPI_SUCCESS, status );
        mcapi_pktchan_send_close_i( handy, &request, &status );
        sassert( MCAPI_ERR_CHAN_NOTOPEN, status );

        mcapi_finalize( &status );

        exit(0);
    }
    else if ( pid != -1 )
    {
        mcapi_pktchan_recv_hndl_t handy;
        struct endPointID us_id = BAR;
        struct endPointID them_id = FOO;

        mcapi_initialize( us_id.domain_id, us_id.node_id, 0, 0, &info, &status );
        receiver = mcapi_endpoint_create( us_id.port_id, &status );
        sender = mcapi_endpoint_get( them_id.domain_id, them_id.node_id,
        them_id.port_id, 1000, &status );
        
        mcapi_pktchan_connect_i( sender, receiver, &request, &status );
        mcapi_wait( &request, &size, 1001, &status );
        mcapi_pktchan_recv_open_i( &handy, receiver, &request, &status );
        mcapi_wait( &request, &size, 1000, &status );
        mcapi_pktchan_recv( handy, &recv_buf, &size, &status );
        mcapi_pktchan_release( recv_buf, &status );
        
        mcapi_pktchan_recv_close_i( handy, &request, &status );
        sassert( MCAPI_PENDING, status );
        mcapi_wait( &request, &size, 60000, &status );
        sassert( MCAPI_SUCCESS, status );
        mcapi_pktchan_recv_close_i( handy, &request, &status );
        sassert( MCAPI_ERR_CHAN_NOTOPEN, status );
        
        wait(NULL);

        mcapi_finalize( &status );
    }
    else
    {
        perror("fork");
    }
}

//see if channel may be properly reused after the close!
test(pkt_re_open)

    strncpy(send_buf, TEST_MESSAGE, MAX_MSG_LEN);

    pid = fork();

    if ( pid == 0 )
    {
        mcapi_pktchan_send_hndl_t handy;
        struct endPointID us_id = FOO;
        struct endPointID them_id = BAR;

        mcapi_initialize( us_id.domain_id, us_id.node_id, 0, 0, &info, &status );
        sender = mcapi_endpoint_create( us_id.port_id, &status );

        mcapi_pktchan_send_open_i( &handy, sender, &request, &status );
        mcapi_wait( &request, &size, 1001, &status );
        mcapi_pktchan_send( handy, send_buf, MAX_MSG_LEN, &status );

        mcapi_pktchan_send_close_i( handy, &request, &status );
        sassert( MCAPI_PENDING, status );
        mcapi_wait( &request, &size, 1001, &status );
        sassert( MCAPI_SUCCESS, status );
        mcapi_pktchan_send_close_i( handy, &request, &status );
        sassert( MCAPI_ERR_CHAN_NOTOPEN, status );

        mcapi_pktchan_send_open_i( &handy, sender, &request, &status );
        sassert( MCAPI_PENDING, status );
        mcapi_wait( &request, &size, 1001, &status );
        sassert( MCAPI_SUCCESS, status );
        mcapi_pktchan_send( handy, send_buf, MAX_MSG_LEN, &status );
        sassert( MCAPI_SUCCESS, status );

        mcapi_finalize( &status );

        exit(0);
    }
    else if ( pid != -1 )
    {
        mcapi_pktchan_recv_hndl_t handy;
        struct endPointID us_id = BAR;
        struct endPointID them_id = FOO;

        mcapi_initialize( us_id.domain_id, us_id.node_id, 0, 0, &info, &status );
        receiver = mcapi_endpoint_create( us_id.port_id, &status );
        sender = mcapi_endpoint_get( them_id.domain_id, them_id.node_id,
        them_id.port_id, 1000, &status );
        
        mcapi_pktchan_connect_i( sender, receiver, &request, &status );
        mcapi_wait( &request, &size, 1001, &status );
        mcapi_pktchan_recv_open_i( &handy, receiver, &request, &status );
        mcapi_wait( &request, &size, 1000, &status );
        mcapi_pktchan_recv( handy, &recv_buf, &size, &status );
        mcapi_pktchan_release( recv_buf, &status );
        
        mcapi_pktchan_recv_close_i( handy, &request, &status );
        mcapi_wait( &request, &size, 1000, &status );
        mcapi_pktchan_recv_close_i( handy, &request, &status );
        sassert( MCAPI_ERR_CHAN_NOTOPEN, status );

        mcapi_pktchan_connect_i( sender, receiver, &request, &status );
        mcapi_wait( &request, &size, 1001, &status );
        mcapi_pktchan_recv_open_i( &handy, receiver, &request, &status );
        sassert( MCAPI_PENDING, status );
        mcapi_wait( &request, &size, 1000, &status );
        sassert( MCAPI_SUCCESS, status );
        mcapi_pktchan_recv( handy, &recv_buf, &size, &status );
        sassert( MCAPI_SUCCESS, status );
        uassert( size == MAX_MSG_LEN );
        uassert2( 0, memcmp( send_buf, recv_buf, MAX_MSG_LEN ) );
        mcapi_pktchan_release( recv_buf, &status );
        sassert( MCAPI_SUCCESS, status );
        
        wait(NULL);

        mcapi_finalize( &status );
    }
    else
    {
        perror("fork");
    }
}

//see that when close is pending, certain functions will fail
test(pkt_close_fail_pend)

    strncpy(send_buf, TEST_MESSAGE, MAX_MSG_LEN);

    pid = fork();

    if ( pid == 0 )
    {
        mcapi_pktchan_send_hndl_t handy;
        struct endPointID us_id = FOO;
        struct endPointID them_id = BAR;

        mcapi_initialize( us_id.domain_id, us_id.node_id, 0, 0, &info, &status );
        sender = mcapi_endpoint_create( us_id.port_id, &status );

        mcapi_pktchan_send_open_i( &handy, sender, &request, &status );
        mcapi_wait( &request, &size, 1001, &status );
        mcapi_pktchan_send( handy, send_buf, MAX_MSG_LEN, &status );

        mcapi_pktchan_send_close_i( handy, &request, &status );
        sassert( MCAPI_PENDING, status );
        mcapi_pktchan_send_close_i( handy, &request, &status );
        sassert( MCAPI_ERR_CHAN_CLOSEPENDING, status );
        mcapi_pktchan_send_open_i( &handy, sender, &request, &status );
        sassert( MCAPI_ERR_CHAN_CLOSEPENDING, status );

        mcapi_finalize( &status );

        exit(0);
    }
    else if ( pid != -1 )
    {
        mcapi_pktchan_recv_hndl_t handy;
        struct endPointID us_id = BAR;
        struct endPointID them_id = FOO;

        mcapi_initialize( us_id.domain_id, us_id.node_id, 0, 0, &info, &status );
        receiver = mcapi_endpoint_create( us_id.port_id, &status );
        sender = mcapi_endpoint_get( them_id.domain_id, them_id.node_id,
        them_id.port_id, 1000, &status );
        
        mcapi_pktchan_connect_i( sender, receiver, &request, &status );
        mcapi_wait( &request, &size, 1001, &status );
        mcapi_pktchan_recv_open_i( &handy, receiver, &request, &status );
        mcapi_wait( &request, &size, 1000, &status );
        mcapi_pktchan_recv( handy, &recv_buf, &size, &status );
        mcapi_pktchan_release( recv_buf, &status );
        
        mcapi_pktchan_recv_close_i( handy, &request, &status );
        sassert( MCAPI_PENDING, status );
        mcapi_pktchan_recv_close_i( handy, &request, &status );
        sassert( MCAPI_ERR_CHAN_CLOSEPENDING, status );
        mcapi_pktchan_recv_open_i( &handy, receiver, &request, &status );
        sassert( MCAPI_ERR_CHAN_CLOSEPENDING, status );
        mcapi_pktchan_connect_i( sender, receiver, &request, &status );
        sassert( MCAPI_ERR_CHAN_CLOSEPENDING, status );
        
        wait(NULL);

        mcapi_finalize( &status );
    }
    else
    {
        perror("fork");
    }
}

//see that connection works even if called by third party
test(pkt_third_party_con)

    strncpy(send_buf, TEST_MESSAGE, MAX_MSG_LEN);

    pid = fork();

    if ( pid == 0 )
    {
        mcapi_pktchan_send_hndl_t handy;
        struct endPointID us_id = FOO;
        struct endPointID them_id = BAR;

        mcapi_initialize( us_id.domain_id, us_id.node_id, 0, 0, &info, &status );
        sender = mcapi_endpoint_create( us_id.port_id, &status );

        mcapi_pktchan_send_open_i( &handy, sender, &request, &status );
        sassert( MCAPI_PENDING, status );

        mcapi_wait( &request, &size, 1001, &status );
        sassert( MCAPI_SUCCESS, status );

        mcapi_pktchan_send( handy, send_buf, MAX_MSG_LEN, &status );
        sassert( MCAPI_SUCCESS, status );

        mcapi_finalize( &status );
        exit(0);
    }
    else if ( pid != -1 )
    {
        pid = fork();

        if ( pid == 0 )
        {
            mcapi_pktchan_recv_hndl_t handy;
            struct endPointID us_id = BAR;
            struct endPointID them_id = FOO;

            mcapi_initialize( us_id.domain_id, us_id.node_id, 0, 0, &info, 
            &status );
            receiver = mcapi_endpoint_create( us_id.port_id, &status );

            mcapi_pktchan_recv_open_i( &handy, receiver, &request, &status );
            sassert( MCAPI_PENDING, status );

            mcapi_wait( &request, &size, 1000, &status );
            sassert( MCAPI_SUCCESS, status );

            mcapi_pktchan_recv( handy, &recv_buf, &size, &status );
            sassert( MCAPI_SUCCESS, status );
            uassert( size == MAX_MSG_LEN );
            uassert2( 0, memcmp( send_buf, recv_buf, MAX_MSG_LEN ) );
            mcapi_pktchan_release( recv_buf, &status );
            sassert( MCAPI_SUCCESS, status );

            mcapi_finalize( &status );
            exit(0);
        }
        else if ( pid != -1 )
        {
            mcapi_pktchan_send_hndl_t handy;
            struct endPointID us_id = BAR;
            struct endPointID them_id = FOO;

            mcapi_initialize( us_id.domain_id, us_id.node_id, 0, 0, &info, 
            &status );
            receiver = mcapi_endpoint_get( us_id.domain_id, us_id.node_id,
            us_id.port_id, 1000, &status );
            sender = mcapi_endpoint_get( them_id.domain_id, them_id.node_id,
            them_id.port_id, 1000, &status );
            
            mcapi_pktchan_connect_i( sender, receiver, &request, &status );
            sassert( MCAPI_PENDING, status );

            mcapi_wait( &request, &size, 1000, &status );
            sassert( MCAPI_SUCCESS, status );

            wait(NULL);

            mcapi_finalize( &status );
        }
        else
        {
            perror("fork");
        }

        wait(NULL);
    }
    else
    {
        perror("fork");
    }
}

//packet availiable
test(pkt_avail)

    strncpy(send_buf, TEST_MESSAGE, MAX_MSG_LEN);

    pid = fork();

    if ( pid == 0 )
    {
        mcapi_pktchan_send_hndl_t handy;
        struct endPointID us_id = FOO;
        struct endPointID them_id = BAR;

        mcapi_initialize( us_id.domain_id, us_id.node_id, 0, 0, &info, &status );
        sender = mcapi_endpoint_create( us_id.port_id, &status );

        mcapi_pktchan_send_open_i( &handy, sender, &request, &status );
        sassert( MCAPI_PENDING, status );

        mcapi_wait( &request, &size, 1001, &status );
        sassert( MCAPI_SUCCESS, status );

        mcapi_pktchan_send( handy, send_buf, MAX_MSG_LEN, &status );
        sassert( MCAPI_SUCCESS, status );
        mcapi_pktchan_send( handy, send_buf, MAX_MSG_LEN, &status );
        sassert( MCAPI_SUCCESS, status );

        mcapi_finalize( &status );
        exit(0);
    }
    else if ( pid != -1 )
    {
        mcapi_pktchan_recv_hndl_t handy;
        struct endPointID us_id = BAR;
        struct endPointID them_id = FOO;
        char count;

        mcapi_initialize( us_id.domain_id, us_id.node_id, 0, 0, &info, &status );
        receiver = mcapi_endpoint_create( us_id.port_id, &status );
        sender = mcapi_endpoint_get( them_id.domain_id, them_id.node_id,
        them_id.port_id, 1000, &status );
        
        mcapi_pktchan_connect_i( sender, receiver, &request, &status );
        sassert( MCAPI_PENDING, status );

        mcapi_wait( &request, &size, 1000, &status );
        sassert( MCAPI_SUCCESS, status );

        mcapi_pktchan_recv_open_i( &handy, receiver, &request, &status );
        sassert( MCAPI_PENDING, status );

        mcapi_wait( &request, &size, 1000, &status );
        sassert( MCAPI_SUCCESS, status );

        wait(NULL);

        count = mcapi_pktchan_available( handy, &status );
        uassert( count == 2 );
        sassert( MCAPI_SUCCESS, status );

        mcapi_pktchan_recv( handy, &recv_buf, &size, &status );
        sassert( MCAPI_SUCCESS, status );
        mcapi_pktchan_recv( handy, &recv_buf, &size, &status );
        sassert( MCAPI_SUCCESS, status );

        mcapi_finalize( &status );
    }
    else
    {
        perror("fork");
    }
}

//packet not availiable
test(pkt_no_avail)

    strncpy(send_buf, TEST_MESSAGE, MAX_MSG_LEN);

    pid = fork();

    if ( pid == 0 )
    {
        mcapi_pktchan_send_hndl_t handy;
        struct endPointID us_id = FOO;
        struct endPointID them_id = BAR;

        mcapi_initialize( us_id.domain_id, us_id.node_id, 0, 0, &info, &status );
        sender = mcapi_endpoint_create( us_id.port_id, &status );

        mcapi_pktchan_send_open_i( &handy, sender, &request, &status );
        sassert( MCAPI_PENDING, status );

        mcapi_wait( &request, &size, 1001, &status );
        sassert( MCAPI_SUCCESS, status );

        mcapi_finalize( &status );
        exit(0);
    }
    else if ( pid != -1 )
    {
        mcapi_pktchan_recv_hndl_t handy;
        struct endPointID us_id = BAR;
        struct endPointID them_id = FOO;
        char count;

        mcapi_initialize( us_id.domain_id, us_id.node_id, 0, 0, &info, &status );
        receiver = mcapi_endpoint_create( us_id.port_id, &status );
        sender = mcapi_endpoint_get( them_id.domain_id, them_id.node_id,
        them_id.port_id, 1000, &status );
        
        mcapi_pktchan_connect_i( sender, receiver, &request, &status );
        sassert( MCAPI_PENDING, status );

        mcapi_wait( &request, &size, 1000, &status );
        sassert( MCAPI_SUCCESS, status );

        mcapi_pktchan_recv_open_i( &handy, receiver, &request, &status );
        sassert( MCAPI_PENDING, status );

        mcapi_wait( &request, &size, 1000, &status );
        sassert( MCAPI_SUCCESS, status );

        count = mcapi_pktchan_available( handy, &status );
        uassert( count == 0 );
        sassert( MCAPI_SUCCESS, status );

        wait(NULL);

        mcapi_finalize( &status );
    }
    else
    {
        perror("fork");
    }
}

//see if channel may be properly reused after the finalize!
test(pkt_re_open_fin)

    strncpy(send_buf, TEST_MESSAGE, MAX_MSG_LEN);

    pid = fork();

    if ( pid == 0 )
    {
        mcapi_pktchan_send_hndl_t handy;
        struct endPointID us_id = FOO;
        struct endPointID them_id = BAR;

        mcapi_initialize( us_id.domain_id, us_id.node_id, 0, 0, &info, &status );
        sender = mcapi_endpoint_create( us_id.port_id, &status );

        mcapi_pktchan_send_open_i( &handy, sender, &request, &status );
        mcapi_wait( &request, &size, 1001, &status );
        mcapi_pktchan_send( handy, send_buf, MAX_MSG_LEN, &status );

        mcapi_finalize( &status );
        sassert( MCAPI_SUCCESS, status );

        mcapi_initialize( us_id.domain_id, us_id.node_id, 0, 0, &info, &status );
        sassert( MCAPI_SUCCESS, status );
        sender = mcapi_endpoint_create( us_id.port_id, &status );
        sassert( MCAPI_SUCCESS, status );

        mcapi_pktchan_send_open_i( &handy, sender, &request, &status );
        sassert( MCAPI_PENDING, status );
        mcapi_wait( &request, &size, 1001, &status );
        sassert( MCAPI_SUCCESS, status );
        mcapi_pktchan_send( handy, send_buf, MAX_MSG_LEN, &status );
        sassert( MCAPI_SUCCESS, status );

        mcapi_finalize( &status );

        exit(0);
    }
    else if ( pid != -1 )
    {
        mcapi_pktchan_recv_hndl_t handy;
        struct endPointID us_id = BAR;
        struct endPointID them_id = FOO;

        mcapi_initialize( us_id.domain_id, us_id.node_id, 0, 0, &info, &status );
        receiver = mcapi_endpoint_create( us_id.port_id, &status );
        sender = mcapi_endpoint_get( them_id.domain_id, them_id.node_id,
        them_id.port_id, 1000, &status );
        
        mcapi_pktchan_connect_i( sender, receiver, &request, &status );
        mcapi_wait( &request, &size, 1001, &status );
        mcapi_pktchan_recv_open_i( &handy, receiver, &request, &status );
        mcapi_wait( &request, &size, 1000, &status );

        mcapi_finalize( &status );
        sassert( MCAPI_SUCCESS, status );

        mcapi_initialize( us_id.domain_id, us_id.node_id, 0, 0, &info, &status );
        sassert( MCAPI_SUCCESS, status );
        receiver = mcapi_endpoint_create( us_id.port_id, &status );
        sassert( MCAPI_SUCCESS, status );
        sender = mcapi_endpoint_get( them_id.domain_id, them_id.node_id,
        them_id.port_id, 1000, &status );
        sassert( MCAPI_SUCCESS, status );

        mcapi_pktchan_connect_i( sender, receiver, &request, &status );
        mcapi_wait( &request, &size, 1001, &status );
        mcapi_pktchan_recv_open_i( &handy, receiver, &request, &status );
        sassert( MCAPI_PENDING, status );
        mcapi_wait( &request, &size, 1000, &status );
        sassert( MCAPI_SUCCESS, status );
        mcapi_pktchan_recv( handy, &recv_buf, &size, &status );
        sassert( MCAPI_SUCCESS, status );
        uassert( size == MAX_MSG_LEN );
        uassert2( 0, memcmp( send_buf, recv_buf, MAX_MSG_LEN ) );
        mcapi_pktchan_release( recv_buf, &status );
        sassert( MCAPI_SUCCESS, status );

        int count = mcapi_pktchan_available( handy, &status );
        uassert( count == 0 );
        sassert( MCAPI_SUCCESS, status );
        
        wait(NULL);

        mcapi_finalize( &status );
    }
    else
    {
        perror("fork");
    }
}

//see if channel may be properly reused after the delete!
test(pkt_re_open_del)

    strncpy(send_buf, TEST_MESSAGE, MAX_MSG_LEN);

    pid = fork();

    if ( pid == 0 )
    {
        mcapi_pktchan_send_hndl_t handy;
        struct endPointID us_id = FOO;
        struct endPointID them_id = BAR;

        mcapi_initialize( us_id.domain_id, us_id.node_id, 0, 0, &info, &status );
        sender = mcapi_endpoint_create( us_id.port_id, &status );

        mcapi_pktchan_send_open_i( &handy, sender, &request, &status );
        mcapi_wait( &request, &size, 1001, &status );
        mcapi_pktchan_send( handy, send_buf, MAX_MSG_LEN, &status );

        mcapi_pktchan_send_close_i( handy, &request, &status );
        sassert( MCAPI_PENDING, status );
        mcapi_wait( &request, &size, 1001, &status );
        sassert( MCAPI_SUCCESS, status );
        mcapi_pktchan_send_close_i( handy, &request, &status );
        sassert( MCAPI_ERR_CHAN_NOTOPEN, status );

        mcapi_endpoint_delete( sender, &status );
        sassert( MCAPI_SUCCESS, status );
        sender = mcapi_endpoint_create( us_id.port_id, &status );
        sassert( MCAPI_SUCCESS, status );

        mcapi_pktchan_send_open_i( &handy, sender, &request, &status );
        sassert( MCAPI_PENDING, status );
        mcapi_wait( &request, &size, 1001, &status );
        sassert( MCAPI_SUCCESS, status );
        mcapi_pktchan_send( handy, send_buf, MAX_MSG_LEN, &status );
        sassert( MCAPI_SUCCESS, status );

        mcapi_finalize( &status );

        exit(0);
    }
    else if ( pid != -1 )
    {
        mcapi_pktchan_recv_hndl_t handy;
        struct endPointID us_id = BAR;
        struct endPointID them_id = FOO;

        mcapi_initialize( us_id.domain_id, us_id.node_id, 0, 0, &info, &status );
        receiver = mcapi_endpoint_create( us_id.port_id, &status );
        sender = mcapi_endpoint_get( them_id.domain_id, them_id.node_id,
        them_id.port_id, 1000, &status );
        
        mcapi_pktchan_connect_i( sender, receiver, &request, &status );
        mcapi_wait( &request, &size, 1001, &status );
        mcapi_pktchan_recv_open_i( &handy, receiver, &request, &status );
        mcapi_wait( &request, &size, 1000, &status );
        mcapi_pktchan_recv( handy, &recv_buf, &size, &status );
        mcapi_pktchan_release( recv_buf, &status );
        
        mcapi_pktchan_recv_close_i( handy, &request, &status );
        mcapi_wait( &request, &size, 1000, &status );

        mcapi_endpoint_delete( receiver, &status );
        sassert( MCAPI_SUCCESS, status );
        receiver = mcapi_endpoint_create( us_id.port_id, &status );
        sassert( MCAPI_SUCCESS, status );

        mcapi_pktchan_connect_i( sender, receiver, &request, &status );
        sassert( MCAPI_PENDING, status );
        mcapi_wait( &request, &size, 1001, &status );
        sassert( MCAPI_SUCCESS, status );
        mcapi_pktchan_recv_open_i( &handy, receiver, &request, &status );
        sassert( MCAPI_PENDING, status );
        mcapi_wait( &request, &size, 1000, &status );
        sassert( MCAPI_SUCCESS, status );
        mcapi_pktchan_recv( handy, &recv_buf, &size, &status );
        sassert( MCAPI_SUCCESS, status );
        uassert( size == MAX_MSG_LEN );
        uassert2( 0, memcmp( send_buf, recv_buf, MAX_MSG_LEN ) );
        mcapi_pktchan_release( recv_buf, &status );
        sassert( MCAPI_SUCCESS, status );
        
        wait(NULL);

        mcapi_finalize( &status );
    }
    else
    {
        perror("fork");
    }
}

//see if channel may be properly reused after delete on open
test(pkt_re_open_del_mid)

    strncpy(send_buf, TEST_MESSAGE, MAX_MSG_LEN);

    pid = fork();

    if ( pid == 0 )
    {
        mcapi_pktchan_send_hndl_t handy;
        struct endPointID us_id = FOO;
        struct endPointID them_id = BAR;

        mcapi_initialize( us_id.domain_id, us_id.node_id, 0, 0, &info, &status );
        sender = mcapi_endpoint_create( us_id.port_id, &status );
        mcapi_endpoint_set_attribute( sender, MCAPI_ENDP_ATTR_TIMEOUT, &timeut,
        sizeof(mcapi_timeout_t), &status );
        sassert( MCAPI_SUCCESS, status );

        mcapi_pktchan_send_open_i( &handy, sender, &request, &status );

        mcapi_endpoint_delete( sender, &status );
        sassert( MCAPI_SUCCESS, status );
        sender = mcapi_endpoint_create( us_id.port_id, &status );
        sassert( MCAPI_SUCCESS, status );

        mcapi_pktchan_send_open_i( &handy, sender, &request, &status );
        sassert( MCAPI_PENDING, status );
        mcapi_wait( &request, &size, 1001, &status );
        sassert( MCAPI_SUCCESS, status );
        mcapi_pktchan_send( handy, send_buf, MAX_MSG_LEN, &status );
        sassert( MCAPI_SUCCESS, status );

        mcapi_finalize( &status );

        exit(0);
    }
    else if ( pid != -1 )
    {
        mcapi_pktchan_recv_hndl_t handy;
        struct endPointID us_id = BAR;
        struct endPointID them_id = FOO;

        mcapi_initialize( us_id.domain_id, us_id.node_id, 0, 0, &info, &status );
        receiver = mcapi_endpoint_create( us_id.port_id, &status );
        sender = mcapi_endpoint_get( them_id.domain_id, them_id.node_id,
        them_id.port_id, 1000, &status );
        mcapi_endpoint_set_attribute( receiver, MCAPI_ENDP_ATTR_TIMEOUT, &timeut,
        sizeof(mcapi_timeout_t), &status );
        sassert( MCAPI_SUCCESS, status );
        
        mcapi_pktchan_connect_i( sender, receiver, &request, &status );
        mcapi_wait( &request, &size, 1001, &status );
        mcapi_pktchan_recv_open_i( &handy, receiver, &request, &status );

        mcapi_endpoint_delete( receiver, &status );
        sassert( MCAPI_SUCCESS, status );
        receiver = mcapi_endpoint_create( us_id.port_id, &status );
        sassert( MCAPI_SUCCESS, status );
        
        mcapi_pktchan_recv_open_i( &handy, receiver, &request, &status );
        sassert( MCAPI_PENDING, status );
        mcapi_wait( &request, &size, 1000, &status );
        sassert( MCAPI_SUCCESS, status );

        mcapi_pktchan_recv( handy, &recv_buf, &size, &status );
        sassert( MCAPI_SUCCESS, status );
        uassert( size == MAX_MSG_LEN );
        uassert2( 0, memcmp( send_buf, recv_buf, MAX_MSG_LEN ) );
        mcapi_pktchan_release( recv_buf, &status );
        sassert( MCAPI_SUCCESS, status );
        
        wait(NULL);

        mcapi_finalize( &status );
    }
    else
    {
        perror("fork");
    }
}

void suite_packet_tx_rx()
{
    nullhand.us = MCAPI_NULL;

    dotest(pkt_release_fail_init)
    dotest(pkt_release_fail_buf)
    dotest(pkt_send_recv_fail_init)
    dotest(pkt_send_recv)
    dotest(pkt_send_recv_fail_trans)
    dotest(pkt_send_recv_timeout)
    dotest(dele_con_fail)
    dotest(pkt_send_recv_big)
    dotest(pkt_send_recv_small)
    dotest(pkt_send_recv_zero)
    dotest(pkt_send_recv_sub_zero)
    dotest(pkt_send_inva_buffer)
    dotest(pkt_send_recv_init)
    dotest(pkt_send_too_big)
    dotest(pkt_send_recv_inva_chan)
    dotest(pkt_close)
    dotest(pkt_buffer_stress)
    dotest(pkt_re_open)
    dotest(pkt_close_fail_pend)
    dotest(pkt_third_party_con)
    dotest(pkt_avail)
    dotest(pkt_no_avail)
    dotest(pkt_re_open_fin)
    dotest(pkt_re_open_del)
    dotest(pkt_re_open_del_mid)
}
