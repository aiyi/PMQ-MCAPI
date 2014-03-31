//this suite focuses on connecting and opening packet channels
#include <mcapi.h>
#include "utester.h"

static struct endPointData iepd;
static mcapi_info_t info;
static mcapi_status_t status;
static mcapi_endpoint_t sender;
static mcapi_endpoint_t receiver;
static size_t size;
static pid_t pid;
static mcapi_request_t request;

//node must be inited first
test(pkt_conn_fail_init)
    mcapi_pktchan_connect_i( sender, receiver, &request, &status );
    sassert( MCAPI_ERR_NODE_NOTINIT, status );
}

//request handle must not be null
test(pkt_conn_fail_req_null)
    struct endPointID us_id = SEND;
    struct endPointID them_id = RECV;
    mcapi_initialize( us_id.domain_id, us_id.node_id, 0, 0, &info, &status );

    mcapi_pktchan_connect_i( sender, receiver, NULL, &status );
    sassert( MCAPI_ERR_PARAMETER, status );

    mcapi_finalize( &status );
}

//endpoints must not be null
test(pkt_conn_fail_endp_null)
    struct endPointID us_id = SEND;
    struct endPointID them_id = RECV;

    mcapi_initialize( us_id.domain_id, us_id.node_id, 0, 0, &info, &status );

    receiver = mcapi_endpoint_create( us_id.port_id, &status );
    sender = mcapi_endpoint_create( them_id.port_id, &status );

    mcapi_pktchan_connect_i( MCAPI_NULL, MCAPI_NULL, &request, &status );
    sassert( MCAPI_ERR_ENDP_INVALID, status );

    mcapi_pktchan_connect_i( MCAPI_NULL, receiver, &request, &status );
    sassert( MCAPI_ERR_ENDP_INVALID, status );

    mcapi_pktchan_connect_i( sender, MCAPI_NULL, &request, &status );
    sassert( MCAPI_ERR_ENDP_INVALID, status );

    mcapi_finalize( &status );
}

//endpoints may not be same
test(pkt_conn_fail_endp_same)
    struct endPointID us_id = SEND;
    struct endPointID them_id = RECV;

    mcapi_initialize( us_id.domain_id, us_id.node_id, 0, 0, &info, &status );

    sender = mcapi_endpoint_create( them_id.port_id, &status );

    mcapi_pktchan_connect_i( sender, sender, &request, &status );
    sassert( MCAPI_ERR_ENDP_INVALID, status );

    mcapi_finalize( &status );
}

//definitions must not be null
test(pkt_conn_fail_def_null)
    mcapi_initialize( 0, 0, 0, 0, &info, &status );

    sender = mcapi_endpoint_create( 0, &status );
    receiver = mcapi_endpoint_create( 1, &status );

    mcapi_pktchan_connect_i( sender, receiver, &request, &status );
    sassert( MCAPI_ERR_ENDP_INVALID, status );

    mcapi_finalize( &status );
}

//definitions must be compatible
test(pkt_conn_fail_def_inva)
    struct endPointID us_id = FOO;
    struct endPointID them_id = RECV;

    mcapi_initialize( us_id.domain_id, us_id.node_id, 0, 0, &info, &status );

    receiver = mcapi_endpoint_create( us_id.port_id, &status );
    sender = mcapi_endpoint_create( them_id.port_id, &status );

    mcapi_pktchan_connect_i( sender, receiver, &request, &status );
    sassert( MCAPI_ERR_ENDP_INVALID, status );

    mcapi_finalize( &status );
}

//must initialize node first
test(pkt_open_fail_init)
    mcapi_pktchan_recv_open_i( NULL, sender, &request, &status );
    sassert( MCAPI_ERR_NODE_NOTINIT, status );
    mcapi_pktchan_send_open_i( NULL, sender, &request, &status );
    sassert( MCAPI_ERR_NODE_NOTINIT, status );
}

//handle must not be null
test(pkt_open_fail_handy)
    mcapi_initialize( 0, 0, 0, 0, &info, &status );

    mcapi_pktchan_recv_open_i( NULL, sender, &request, &status );
    sassert( MCAPI_ERR_PARAMETER, status );
    mcapi_pktchan_send_open_i( NULL, sender, &request, &status );
    sassert( MCAPI_ERR_PARAMETER, status );

    mcapi_finalize( &status );
}

//receiver cannot be sender and vice-versa
test(pkt_open_fail_dir)

    pid = fork();

    if ( pid == 0 )
    {
        mcapi_pktchan_send_hndl_t handy;
        struct endPointID us_id = FOO;
        struct endPointID them_id = BAR;

        mcapi_initialize( us_id.domain_id, us_id.node_id, 0, 0, &info, &status );
        sender = mcapi_endpoint_create( us_id.port_id, &status );

        mcapi_pktchan_recv_open_i( &handy, sender, &request, &status );
        sassert( MCAPI_ERR_CHAN_DIRECTION, status );

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

        mcapi_pktchan_send_open_i( &handy, receiver, &request, &status );
        sassert( MCAPI_ERR_CHAN_DIRECTION, status );

        wait(NULL);

        mcapi_finalize( &status );
    }
    else
    {
        perror("fork");
    }
}

//cannot open, if there is already another one pending
test(pkt_open_fail_pend)

    pid = fork();

    if ( pid == 0 )
    {
        mcapi_pktchan_send_hndl_t handy;
        struct endPointID us_id = BAR;
        struct endPointID them_id = FOO;

        mcapi_initialize( us_id.domain_id, us_id.node_id, 0, 0, &info, &status );
        sender = mcapi_endpoint_create( us_id.port_id, &status );

        mcapi_pktchan_recv_open_i( &handy, sender, &request, &status );
        sassert( MCAPI_PENDING, status );
        mcapi_pktchan_recv_open_i( &handy, sender, &request, &status );
        sassert( MCAPI_ERR_CHAN_OPENPENDING, status );

        mcapi_finalize( &status );
        exit(0);
    }
    else if ( pid != -1 )
    {
        mcapi_pktchan_recv_hndl_t handy;
        struct endPointID us_id = FOO;
        struct endPointID them_id = BAR;

        mcapi_initialize( us_id.domain_id, us_id.node_id, 0, 0, &info, &status );
        receiver = mcapi_endpoint_create( us_id.port_id, &status );

        mcapi_pktchan_send_open_i( &handy, receiver, &request, &status );
        sassert( MCAPI_PENDING, status );
        mcapi_pktchan_send_open_i( &handy, receiver, &request, &status );
        sassert( MCAPI_ERR_CHAN_OPENPENDING, status );

        wait(NULL);

        mcapi_finalize( &status );
    }
    else
    {
        perror("fork");
    }
}

//end points must be valid
test(pkt_open_fail_endp)

    pid = fork();

    if ( pid == 0 )
    {
        mcapi_pktchan_send_hndl_t handy;
        struct endPointID us_id = FOO;
        struct endPointID them_id = BAR;

        mcapi_initialize( us_id.domain_id, us_id.node_id+1, 0, 0, &info, &status );
        sender = mcapi_endpoint_create( us_id.port_id, &status );

        mcapi_pktchan_send_open_i( &handy, NULL, &request, &status );
        sassert( MCAPI_ERR_ENDP_INVALID, status );
        mcapi_pktchan_send_open_i( &handy, sender, &request, &status );
        sassert( MCAPI_ERR_ENDP_INVALID, status );

        mcapi_finalize( &status );
        exit(0);
    }
    else if ( pid != -1 )
    {
        mcapi_pktchan_recv_hndl_t handy;
        struct endPointID us_id = BAR;
        struct endPointID them_id = FOO;

        mcapi_initialize( us_id.domain_id, us_id.node_id+1, 0, 0, &info, &status );
        receiver = mcapi_endpoint_create( us_id.port_id, &status );

        mcapi_pktchan_recv_open_i( &handy, NULL, &request, &status );
        sassert( MCAPI_ERR_ENDP_INVALID, status );
        mcapi_pktchan_recv_open_i( &handy, receiver, &request, &status );
        sassert( MCAPI_ERR_ENDP_INVALID, status );

        wait(NULL);

        mcapi_finalize( &status );
    }
    else
    {
        perror("fork");
    }
}

//request-parameter must be valid
test(pkt_open_fail_req_null)
    mcapi_pktchan_send_hndl_t handy;
    struct endPointID us_id = SEND;
    struct endPointID them_id = RECV;

    mcapi_initialize( us_id.domain_id, us_id.node_id, 0, 0, &info, &status );

    sender = mcapi_endpoint_create( us_id.port_id, &status );

    mcapi_pktchan_send_open_i( &handy, sender, NULL, &status );
    sassert( MCAPI_ERR_PARAMETER, status );

    mcapi_finalize( &status );
}

//channel type must be valid
test(pkt_open_fail_chan)
    mcapi_pktchan_send_hndl_t handy;
    struct endPointID us_id = SEND;
    struct endPointID them_id = RECV;

    mcapi_initialize( us_id.domain_id, us_id.node_id, 0, 0, &info, &status );

    sender = mcapi_endpoint_create( us_id.port_id, &status );
    sender->defs->type = MCAPI_SCL_CHAN;

    mcapi_pktchan_send_open_i( &handy, sender, &request, &status );
    sassert( MCAPI_ERR_CHAN_TYPE, status );
    sender->defs->type = MCAPI_PKT_CHAN;

    mcapi_finalize( &status );
}

//cannot open, if it is open already
test(pkt_open_fail_open)

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
        sassert( MCAPI_SUCCESS, status );
        mcapi_pktchan_send_open_i( &handy, sender, &request, &status );
        sassert( MCAPI_ERR_CHAN_OPEN, status );

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
        mcapi_pktchan_recv_open_i( &handy, receiver, &request, &status );
        mcapi_wait( &request, &size, 1000, &status );
        sassert( MCAPI_SUCCESS, status );
        mcapi_pktchan_recv_open_i( &handy, receiver, &request, &status );
        sassert( MCAPI_ERR_CHAN_OPEN, status );

        wait(NULL);

        mcapi_finalize( &status );
    }
}

//testing timeout when the other endpoint is absent
test(pkt_open_fail_timeout_endp)
    mcapi_pktchan_recv_hndl_t handy;
    struct endPointID us_id = BAR;
    struct endPointID them_id = FOO;

    mcapi_initialize( us_id.domain_id, us_id.node_id, 0, 0, &info, &status );
    receiver = mcapi_endpoint_create( us_id.port_id, &status );
    sender = mcapi_endpoint_get( them_id.domain_id, them_id.node_id,
    them_id.port_id, 100, &status );
    sassert( MCAPI_TIMEOUT, status );
    
    mcapi_pktchan_connect_i( sender, receiver, &request, &status );
    mcapi_pktchan_recv_open_i( &handy, receiver, &request, &status );
    mcapi_wait( &request, &size, 100, &status );
    sassert( MCAPI_TIMEOUT, status );

    mcapi_finalize( &status );
}

//cant send messages to connected
test(pkt_chan_msg_ban)

    pid = fork();

    if ( pid == 0 )
    {
        mcapi_pktchan_send_hndl_t handy;
        struct endPointID us_id = FOO;
        struct endPointID them_id = BAR;
        char send_buf[MAX_MSG_LEN];

        mcapi_initialize( us_id.domain_id, us_id.node_id, 0, 0, &info, &status );
        sender = mcapi_endpoint_create( us_id.port_id, &status );

        mcapi_pktchan_send_open_i( &handy, sender, &request, &status );
        sassert( MCAPI_PENDING, status );

        mcapi_wait( &request, &size, 1001, &status );
        sassert( MCAPI_SUCCESS, status );

        mcapi_msg_send( sender, receiver, send_buf, MAX_MSG_LEN, 0, &status );
        sassert( MCAPI_ERR_GENERAL, status );
        
        mcapi_finalize( &status );
        exit(0);
    }
    else if ( pid != -1 )
    {
        mcapi_pktchan_recv_hndl_t handy;
        struct endPointID us_id = BAR;
        struct endPointID them_id = FOO;
        char recv_buf[MAX_MSG_LEN];

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

        mcapi_msg_recv( receiver, recv_buf, MAX_MSG_LEN, &size, &status );
        sassert( MCAPI_ERR_GENERAL, status );

        wait(NULL);

        mcapi_finalize( &status );
    }
    else
    {
        perror("fork");
    }
}

void suite_packet_con_open()
{
    iepd.inited = -1;

    dotest(pkt_conn_fail_init)
    dotest(pkt_conn_fail_req_null)
    dotest(pkt_conn_fail_endp_null)
    dotest(pkt_conn_fail_endp_same)
    dotest(pkt_conn_fail_def_null)
    dotest(pkt_conn_fail_def_inva)
    dotest(pkt_open_fail_init)
    dotest(pkt_open_fail_handy)
    dotest(pkt_open_fail_init)
    dotest(pkt_open_fail_dir)
    dotest(pkt_open_fail_pend)
    dotest(pkt_open_fail_endp)
    dotest(pkt_open_fail_req_null)
    dotest(pkt_open_fail_chan)
    dotest(pkt_open_fail_timeout_endp)
    dotest(pkt_chan_msg_ban)
}
