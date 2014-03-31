//this suite tests scalar-channel communication
#include <mcapi.h>
#include "utester.h"

static mcapi_info_t info;
static mcapi_status_t status;
static mcapi_endpoint_t sender;
static mcapi_endpoint_t receiver;
static mcapi_uint64_t send;
static mcapi_uint64_t recv;
static size_t size;
static pid_t pid;
static mcapi_request_t request;
static mcapi_timeout_t timeout = 100;
static mcapi_sclchan_recv_hndl_t nullhand;

//must be inited
test(scl_send_recv_fail_init)
    mcapi_sclchan_send_uint64( nullhand, 0, &status );
    sassert( MCAPI_ERR_NODE_NOTINIT, status );
    mcapi_sclchan_recv_uint64( nullhand, &status );
    sassert( MCAPI_ERR_NODE_NOTINIT, status );
}

//see that 64-bit send and receive works
test(scl_send_recv)

    pid = fork();

    if ( pid == 0 )
    {
        mcapi_sclchan_send_hndl_t handy;
        struct endPointID us_id = SSCL;
        struct endPointID them_id = RSCL;

        mcapi_initialize( us_id.domain_id, us_id.node_id, 0, 0, &info, &status );
        sender = mcapi_endpoint_create( us_id.port_id, &status );

        mcapi_sclchan_send_open_i( &handy, sender, &request, &status );
        sassert( MCAPI_PENDING, status );

        mcapi_wait( &request, &size, 1001, &status );
        sassert( MCAPI_SUCCESS, status );

        mcapi_sclchan_send_uint64( handy, send, &status );
        sassert( MCAPI_SUCCESS, status );

        mcapi_finalize( &status );
        exit(0);
    }
    else if ( pid != -1 )
    {
        mcapi_sclchan_recv_hndl_t handy;
        struct endPointID us_id = RSCL;
        struct endPointID them_id = SSCL;

        mcapi_initialize( us_id.domain_id, us_id.node_id, 0, 0, &info, &status );
        receiver = mcapi_endpoint_create( us_id.port_id, &status );
        sender = mcapi_endpoint_get( them_id.domain_id, them_id.node_id,
        them_id.port_id, 1000, &status );
        
        mcapi_sclchan_connect_i( sender, receiver, &request, &status );
        sassert( MCAPI_PENDING, status );
        mcapi_wait( &request, &size, 1000, &status );
        sassert( MCAPI_SUCCESS, status );

        mcapi_sclchan_recv_open_i( &handy, receiver, &request, &status );
        sassert( MCAPI_PENDING, status );

        mcapi_wait( &request, &size, 1000, &status );
        sassert( MCAPI_SUCCESS, status );

        recv = mcapi_sclchan_recv_uint64( handy, &status );
        sassert( MCAPI_SUCCESS, status );
        uassert( send == recv );

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
test(scl_send_recv_fail_trans)

    strncpy(send_buf, TEST_MESSAGE, MAX_MSG_LEN);

    pid = fork();

    if ( pid == 0 )
    {
        mcapi_sclchan_send_hndl_t handy;
        struct endPointID us_id = SSCL;
        struct endPointID them_id = RSCL;
        unsigned int i = 0;

        mcapi_initialize( us_id.domain_id, us_id.node_id, 0, 0, &info, &status );
        sender = mcapi_endpoint_create( us_id.port_id, &status );

        mcapi_endpoint_set_attribute( sender, MCAPI_ENDP_ATTR_TIMEOUT, &timeut,
        sizeof(mcapi_timeout_t), &status );
        sassert( MCAPI_SUCCESS, status );

        mcapi_sclchan_send_open_i( &handy, sender, &request, &status );
        sassert( MCAPI_PENDING, status );

        mcapi_wait( &request, &size, 1001, &status );
        sassert( MCAPI_SUCCESS, status );

        do
        {
            mcapi_sclchan_send_uint64( handy, 0xAA, &status );
            ++i; 

            uassert( i < 20 );
        }
        while( status == MCAPI_SUCCESS );

        mcapi_sclchan_send_close_i( handy, &request, &status );
        sassert( MCAPI_PENDING, status );
        mcapi_wait( &request, &size, 1000, &status );
        sassert( MCAPI_SUCCESS, status );

        mcapi_sclchan_send_open_i( &handy, sender, &request, &status );
        sassert( MCAPI_PENDING, status );

        mcapi_wait( &request, &size, 1001, &status );
        sassert( MCAPI_SUCCESS, status );
        
        mcapi_sclchan_send_uint64( handy, 0xAA, &status );
        sassert( MCAPI_SUCCESS, status );

        mcapi_finalize( &status );
        exit(0);
    }
    else if ( pid != -1 )
    {
        mcapi_sclchan_recv_hndl_t handy;
        struct endPointID us_id = RSCL;
        struct endPointID them_id = SSCL;

        mcapi_initialize( us_id.domain_id, us_id.node_id, 0, 0, &info, &status );
        receiver = mcapi_endpoint_create( us_id.port_id, &status );
        sender = mcapi_endpoint_get( them_id.domain_id, them_id.node_id,
        them_id.port_id, 1000, &status );
        
        mcapi_sclchan_connect_i( sender, receiver, &request, &status );
        sassert( MCAPI_PENDING, status );

        mcapi_wait( &request, &size, 1000, &status );
        sassert( MCAPI_SUCCESS, status );

        mcapi_sclchan_recv_open_i( &handy, receiver, &request, &status );
        sassert( MCAPI_PENDING, status );

        mcapi_wait( &request, &size, 1000, &status );
        sassert( MCAPI_SUCCESS, status );

        mcapi_sclchan_recv_uint64( handy, &status );

        mcapi_sclchan_recv_close_i( handy, &request, &status );
        sassert( MCAPI_PENDING, status );
        mcapi_wait( &request, &size, 1000, &status );
        sassert( MCAPI_SUCCESS, status );

        mcapi_sclchan_connect_i( sender, receiver, &request, &status );
        sassert( MCAPI_PENDING, status );

        mcapi_wait( &request, &size, 1000, &status );
        sassert( MCAPI_SUCCESS, status );

        mcapi_sclchan_recv_open_i( &handy, receiver, &request, &status );
        sassert( MCAPI_PENDING, status );

        mcapi_wait( &request, &size, 1000, &status );
        sassert( MCAPI_SUCCESS, status );

        mcapi_sclchan_recv_uint64( handy, &status );
        sassert( MCAPI_SUCCESS, status );

        wait(NULL);

        mcapi_finalize( &status );
    }
    else
    {
        perror("fork");
    }
}

//see that send and receive timeouts
test(scl_send_recv_timeout)
    unsigned int i = 0;

    pid = fork();

    if ( pid == 0 )
    {
        mcapi_sclchan_send_hndl_t handy;
        struct endPointID us_id = SSCL;
        struct endPointID them_id = RSCL;

        mcapi_initialize( us_id.domain_id, us_id.node_id, 0, 0, &info, &status );
        sender = mcapi_endpoint_create( us_id.port_id, &status );

        mcapi_endpoint_set_attribute( sender, MCAPI_ENDP_ATTR_TIMEOUT, 
        &timeout, sizeof(mcapi_timeout_t), &status );
        sassert( MCAPI_SUCCESS, status );

        mcapi_sclchan_send_open_i( &handy, sender, &request, &status );
        sassert( MCAPI_PENDING, status );

        mcapi_wait( &request, &size, 1001, &status );
        sassert( MCAPI_SUCCESS, status );

        for ( i = 0; i < 10; ++i )
        {
            mcapi_sclchan_send_uint64( handy, send, &status );
            sassert( MCAPI_SUCCESS, status );
        }

        mcapi_sclchan_send_uint64( handy, send, &status );
        sassert( MCAPI_TIMEOUT, status );

        mcapi_finalize( &status );
        exit(0);
    }
    else if ( pid != -1 )
    {
        mcapi_sclchan_recv_hndl_t handy;
        struct endPointID us_id = RSCL;
        struct endPointID them_id = SSCL;

        mcapi_initialize( us_id.domain_id, us_id.node_id, 0, 0, &info, &status );
        receiver = mcapi_endpoint_create( us_id.port_id, &status );
        sender = mcapi_endpoint_get( them_id.domain_id, them_id.node_id,
        them_id.port_id, 1000, &status );

        mcapi_endpoint_set_attribute( receiver, MCAPI_ENDP_ATTR_TIMEOUT, 
        &timeout, sizeof(mcapi_timeout_t), &status );
        sassert( MCAPI_SUCCESS, status );
        
        mcapi_sclchan_connect_i( sender, receiver, &request, &status );
        sassert( MCAPI_PENDING, status );
        mcapi_wait( &request, &size, 1000, &status );
        sassert( MCAPI_SUCCESS, status );

        mcapi_sclchan_recv_open_i( &handy, receiver, &request, &status );
        sassert( MCAPI_PENDING, status );

        mcapi_wait( &request, &size, 1000, &status );
        sassert( MCAPI_SUCCESS, status );

        wait(NULL);

        for ( i = 0; i < 10; ++i )
        {
            recv = mcapi_sclchan_recv_uint64( handy, &status );
            sassert( MCAPI_SUCCESS, status );
        }

        recv = mcapi_sclchan_recv_uint64( handy, &status );
        sassert( MCAPI_TIMEOUT, status );

        mcapi_finalize( &status );
    }
    else
    {
        perror("fork");
    }
}

//must not work if node is not inited
test(scl_send_recv_init)
    mcapi_sclchan_send_uint64( nullhand, send, &status );
    sassert( MCAPI_ERR_NODE_NOTINIT, status );
    mcapi_sclchan_recv_uint64( nullhand, &status );
    sassert( MCAPI_ERR_NODE_NOTINIT, status );
}

//channel handlers must be valid
test(scl_send_recv_inva_chan)
    struct endPointID us_id = SSCL;
    mcapi_initialize( us_id.domain_id, us_id.node_id, 0, 0, &info, &status );
    mcapi_sclchan_send_hndl_t handy1;
    mcapi_sclchan_recv_hndl_t handy2;

    handy1.us = MCAPI_NULL;
    handy2.us = MCAPI_NULL;

    mcapi_sclchan_send_uint64( handy1, send, &status );
    sassert( MCAPI_ERR_CHAN_INVALID, status );
    mcapi_sclchan_recv_uint64( handy2, &status );
    sassert( MCAPI_ERR_CHAN_INVALID, status );

    mcapi_finalize( &status );
}

//see that close works
test(scl_close)

    pid = fork();

    if ( pid == 0 )
    {
        mcapi_sclchan_send_hndl_t handy;
        struct endPointID us_id = SSCL;
        struct endPointID them_id = RSCL;

        mcapi_initialize( us_id.domain_id, us_id.node_id, 0, 0, &info, &status );
        sender = mcapi_endpoint_create( us_id.port_id, &status );

        mcapi_sclchan_send_open_i( &handy, sender, &request, &status );
        mcapi_wait( &request, &size, 1001, &status );
        mcapi_sclchan_send_uint64( handy, send, &status );

        mcapi_sclchan_send_close_i( handy, &request, &status );
        sassert( MCAPI_PENDING, status );
        mcapi_wait( &request, &size, 1001, &status );
        sassert( MCAPI_SUCCESS, status );
        mcapi_sclchan_send_close_i( handy, &request, &status );
        sassert( MCAPI_ERR_CHAN_NOTOPEN, status );

        mcapi_finalize( &status );

        exit(0);
    }
    else if ( pid != -1 )
    {
        mcapi_sclchan_recv_hndl_t handy;
        struct endPointID us_id = RSCL;
        struct endPointID them_id = SSCL;

        mcapi_initialize( us_id.domain_id, us_id.node_id, 0, 0, &info, &status );
        receiver = mcapi_endpoint_create( us_id.port_id, &status );
        sender = mcapi_endpoint_get( them_id.domain_id, them_id.node_id,
        them_id.port_id, 1000, &status );
        
        mcapi_sclchan_connect_i( sender, receiver, &request, &status );
        mcapi_wait( &request, &size, 1001, &status );
        mcapi_sclchan_recv_open_i( &handy, receiver, &request, &status );
        mcapi_wait( &request, &size, 1000, &status );
        mcapi_sclchan_recv_uint64( handy, &status );
        
        mcapi_sclchan_recv_close_i( handy, &request, &status );
        sassert( MCAPI_PENDING, status );

        mcapi_wait( &request, &size, 1000, &status );
        sassert( MCAPI_SUCCESS, status );
        mcapi_sclchan_recv_close_i( handy, &request, &status );
        sassert( MCAPI_ERR_CHAN_NOTOPEN, status );
        
        wait(NULL);

        mcapi_finalize( &status );
    }
    else
    {
        perror("fork");
    }
}

//see that channel may be reused after close
test(scl_re_open)

    strncpy(send_buf, TEST_MESSAGE, MAX_MSG_LEN);

    pid = fork();

    if ( pid == 0 )
    {
        mcapi_sclchan_send_hndl_t handy;
        struct endPointID us_id = SSCL;
        struct endPointID them_id = RSCL;

        mcapi_initialize( us_id.domain_id, us_id.node_id, 0, 0, &info, &status );
        sender = mcapi_endpoint_create( us_id.port_id, &status );

        mcapi_sclchan_send_open_i( &handy, sender, &request, &status );
        mcapi_wait( &request, &size, 1001, &status );

        mcapi_sclchan_send_uint64( handy, send, &status );
        sassert( MCAPI_SUCCESS, status );
        mcapi_sclchan_send_close_i( handy, &request, &status );
        sassert( MCAPI_PENDING, status );
        mcapi_wait( &request, &size, 1001, &status );
        sassert( MCAPI_SUCCESS, status );

        mcapi_sclchan_send_open_i( &handy, sender, &request, &status );
        sassert( MCAPI_PENDING, status );
        mcapi_wait( &request, &size, 1001, &status );
        sassert( MCAPI_SUCCESS, status );
        mcapi_sclchan_send_uint64( handy, send, &status );
        sassert( MCAPI_SUCCESS, status );

        mcapi_finalize( &status );

        exit(0);
    }
    else if ( pid != -1 )
    {
        mcapi_sclchan_recv_hndl_t handy;
        struct endPointID us_id = RSCL;
        struct endPointID them_id = SSCL;

        mcapi_initialize( us_id.domain_id, us_id.node_id, 0, 0, &info, &status );
        receiver = mcapi_endpoint_create( us_id.port_id, &status );
        sender = mcapi_endpoint_get( them_id.domain_id, them_id.node_id,
        them_id.port_id, 1000, &status );
        
        mcapi_sclchan_connect_i( sender, receiver, &request, &status );
        mcapi_wait( &request, &size, 1001, &status );
        mcapi_sclchan_recv_open_i( &handy, receiver, &request, &status );
        mcapi_wait( &request, &size, 1000, &status );
        recv = mcapi_sclchan_recv_uint64( handy, &status );
        
        mcapi_sclchan_recv_close_i( handy, &request, &status );
        mcapi_wait( &request, &size, 1000, &status );
        mcapi_sclchan_recv_close_i( handy, &request, &status );
        sassert( MCAPI_ERR_CHAN_NOTOPEN, status );

        mcapi_sclchan_connect_i( sender, receiver, &request, &status );
        mcapi_wait( &request, &size, 1001, &status );

        mcapi_sclchan_recv_open_i( &handy, receiver, &request, &status );
        sassert( MCAPI_PENDING, status );
        mcapi_wait( &request, &size, 1000, &status );
        sassert( MCAPI_SUCCESS, status );
        recv = mcapi_sclchan_recv_uint64( handy, &status );
        uassert( recv == send );
        
        wait(NULL);

        mcapi_finalize( &status );
    }
    else
    {
        perror("fork");
    }
}

//see that 8-bit traffic works
test(scl_send_recv_8bits)

    pid = fork();

    if ( pid == 0 )
    {
        mcapi_sclchan_send_hndl_t handy;
        struct endPointID us_id = SSCL8;
        struct endPointID them_id = RSCL8;

        mcapi_initialize( us_id.domain_id, us_id.node_id, 0, 0, &info, &status );
        sender = mcapi_endpoint_create( us_id.port_id, &status );

        mcapi_sclchan_send_open_i( &handy, sender, &request, &status );
        sassert( MCAPI_PENDING, status );

        mcapi_wait( &request, &size, 1001, &status );
        sassert( MCAPI_SUCCESS, status );

        mcapi_sclchan_send_uint8( handy, send, &status );
        sassert( MCAPI_SUCCESS, status );

        mcapi_finalize( &status );
        exit(0);
    }
    else if ( pid != -1 )
    {
        mcapi_sclchan_recv_hndl_t handy;
        struct endPointID us_id = RSCL8;
        struct endPointID them_id = SSCL8;

        mcapi_initialize( us_id.domain_id, us_id.node_id, 0, 0, &info, &status );
        receiver = mcapi_endpoint_create( us_id.port_id, &status );
        sender = mcapi_endpoint_get( them_id.domain_id, them_id.node_id,
        them_id.port_id, 1000, &status );
        
        mcapi_sclchan_connect_i( sender, receiver, &request, &status );
        mcapi_wait( &request, &size, 1001, &status );
        sassert( MCAPI_SUCCESS, status );

        mcapi_sclchan_recv_open_i( &handy, receiver, &request, &status );
        sassert( MCAPI_PENDING, status );

        mcapi_wait( &request, &size, 1000, &status );
        sassert( MCAPI_SUCCESS, status );

        recv = mcapi_sclchan_recv_uint8( handy, &status );
        sassert( MCAPI_SUCCESS, status );
        uassert( send == recv );

        wait(NULL);

        mcapi_finalize( &status );
    }
    else
    {
        perror("fork");
    }
}

//see that 16-bit traffic works
test(scl_send_recv_16bits)

    pid = fork();

    if ( pid == 0 )
    {
        mcapi_sclchan_send_hndl_t handy;
        struct endPointID us_id = SSCL16;
        struct endPointID them_id = RSCL16;

        mcapi_initialize( us_id.domain_id, us_id.node_id, 0, 0, &info, &status );
        sender = mcapi_endpoint_create( us_id.port_id, &status );

        mcapi_sclchan_send_open_i( &handy, sender, &request, &status );
        sassert( MCAPI_PENDING, status );

        mcapi_wait( &request, &size, 1001, &status );
        sassert( MCAPI_SUCCESS, status );

        mcapi_sclchan_send_uint16( handy, send, &status );
        sassert( MCAPI_SUCCESS, status );

        mcapi_finalize( &status );
        exit(0);
    }
    else if ( pid != -1 )
    {
        mcapi_sclchan_recv_hndl_t handy;
        struct endPointID us_id = RSCL16;
        struct endPointID them_id = SSCL16;

        mcapi_initialize( us_id.domain_id, us_id.node_id, 0, 0, &info, &status );
        receiver = mcapi_endpoint_create( us_id.port_id, &status );
        sender = mcapi_endpoint_get( them_id.domain_id, them_id.node_id,
        them_id.port_id, 1000, &status );
        
        mcapi_sclchan_connect_i( sender, receiver, &request, &status );
        mcapi_wait( &request, &size, 1001, &status );
        sassert( MCAPI_SUCCESS, status );

        mcapi_sclchan_recv_open_i( &handy, receiver, &request, &status );
        sassert( MCAPI_PENDING, status );

        mcapi_wait( &request, &size, 1000, &status );
        sassert( MCAPI_SUCCESS, status );

        recv = mcapi_sclchan_recv_uint16( handy, &status );
        sassert( MCAPI_SUCCESS, status );
        uassert( send == recv );

        wait(NULL);

        mcapi_finalize( &status );
    }
    else
    {
        perror("fork");
    }
}

//see that 32-bit traffic works
test(scl_send_recv_32bits)

    pid = fork();

    if ( pid == 0 )
    {
        mcapi_sclchan_send_hndl_t handy;
        struct endPointID us_id = SSCL32;
        struct endPointID them_id = RSCL32;

        mcapi_initialize( us_id.domain_id, us_id.node_id, 0, 0, &info, &status );
        sender = mcapi_endpoint_create( us_id.port_id, &status );

        mcapi_sclchan_send_open_i( &handy, sender, &request, &status );
        sassert( MCAPI_PENDING, status );

        mcapi_wait( &request, &size, 1001, &status );
        sassert( MCAPI_SUCCESS, status );

        mcapi_sclchan_send_uint32( handy, send, &status );
        sassert( MCAPI_SUCCESS, status );

        mcapi_finalize( &status );
        exit(0);
    }
    else if ( pid != -1 )
    {
        mcapi_sclchan_recv_hndl_t handy;
        struct endPointID us_id = RSCL32;
        struct endPointID them_id = SSCL32;

        mcapi_initialize( us_id.domain_id, us_id.node_id, 0, 0, &info, &status );
        receiver = mcapi_endpoint_create( us_id.port_id, &status );
        sender = mcapi_endpoint_get( them_id.domain_id, them_id.node_id,
        them_id.port_id, 1000, &status );
        
        mcapi_sclchan_connect_i( sender, receiver, &request, &status );
        mcapi_wait( &request, &size, 1001, &status );
        sassert( MCAPI_SUCCESS, status );

        mcapi_sclchan_recv_open_i( &handy, receiver, &request, &status );
        sassert( MCAPI_PENDING, status );

        mcapi_wait( &request, &size, 1000, &status );
        sassert( MCAPI_SUCCESS, status );

        recv = mcapi_sclchan_recv_uint32( handy, &status );
        sassert( MCAPI_SUCCESS, status );
        uassert( send == recv );

        wait(NULL);

        mcapi_finalize( &status );
    }
    else
    {
        perror("fork");
    }
}

//a scalar availiable
test(scl_avail)

    pid = fork();

    if ( pid == 0 )
    {
        mcapi_sclchan_send_hndl_t handy;
        struct endPointID us_id = SSCL;
        struct endPointID them_id = RSCL;

        mcapi_initialize( us_id.domain_id, us_id.node_id, 0, 0, &info, &status );
        sender = mcapi_endpoint_create( us_id.port_id, &status );

        mcapi_sclchan_send_open_i( &handy, sender, &request, &status );
        sassert( MCAPI_PENDING, status );

        mcapi_wait( &request, &size, 1001, &status );
        sassert( MCAPI_SUCCESS, status );

        mcapi_sclchan_send_uint64( handy, send, &status );
        sassert( MCAPI_SUCCESS, status );
        mcapi_sclchan_send_uint64( handy, send, &status );
        sassert( MCAPI_SUCCESS, status );
        mcapi_sclchan_send_uint64( handy, send, &status );
        sassert( MCAPI_SUCCESS, status );

        mcapi_finalize( &status );
        exit(0);
    }
    else if ( pid != -1 )
    {
        mcapi_sclchan_recv_hndl_t handy;
        struct endPointID us_id = RSCL;
        struct endPointID them_id = SSCL;

        mcapi_initialize( us_id.domain_id, us_id.node_id, 0, 0, &info, &status );
        receiver = mcapi_endpoint_create( us_id.port_id, &status );
        sender = mcapi_endpoint_get( them_id.domain_id, them_id.node_id,
        them_id.port_id, 1000, &status );
        
        mcapi_sclchan_connect_i( sender, receiver, &request, &status );
        sassert( MCAPI_PENDING, status );
        mcapi_wait( &request, &size, 1000, &status );
        sassert( MCAPI_SUCCESS, status );

        mcapi_sclchan_recv_open_i( &handy, receiver, &request, &status );
        sassert( MCAPI_PENDING, status );

        mcapi_wait( &request, &size, 1000, &status );
        sassert( MCAPI_SUCCESS, status );

        wait(NULL);

        recv = mcapi_sclchan_available( handy, &status );
        sassert( MCAPI_SUCCESS, status );
        uassert( 3 == recv );

        recv = mcapi_sclchan_recv_uint64( handy, &status );
        sassert( MCAPI_SUCCESS, status );
        uassert( send == recv );
        recv = mcapi_sclchan_recv_uint64( handy, &status );
        sassert( MCAPI_SUCCESS, status );
        uassert( send == recv );
        recv = mcapi_sclchan_recv_uint64( handy, &status );
        sassert( MCAPI_SUCCESS, status );
        uassert( send == recv );

        mcapi_finalize( &status );
    }
    else
    {
        perror("fork");
    }
}

//a scalar availiable
test(scl_not_avail)

    pid = fork();

    if ( pid == 0 )
    {
        mcapi_sclchan_send_hndl_t handy;
        struct endPointID us_id = SSCL;
        struct endPointID them_id = RSCL;

        mcapi_initialize( us_id.domain_id, us_id.node_id, 0, 0, &info, &status );
        sender = mcapi_endpoint_create( us_id.port_id, &status );

        mcapi_sclchan_send_open_i( &handy, sender, &request, &status );
        sassert( MCAPI_PENDING, status );

        mcapi_wait( &request, &size, 1001, &status );
        sassert( MCAPI_SUCCESS, status );

        mcapi_finalize( &status );
        exit(0);
    }
    else if ( pid != -1 )
    {
        mcapi_sclchan_recv_hndl_t handy;
        struct endPointID us_id = RSCL;
        struct endPointID them_id = SSCL;

        mcapi_initialize( us_id.domain_id, us_id.node_id, 0, 0, &info, &status );
        receiver = mcapi_endpoint_create( us_id.port_id, &status );
        sender = mcapi_endpoint_get( them_id.domain_id, them_id.node_id,
        them_id.port_id, 1000, &status );
        
        mcapi_sclchan_connect_i( sender, receiver, &request, &status );
        sassert( MCAPI_PENDING, status );
        mcapi_wait( &request, &size, 1000, &status );
        sassert( MCAPI_SUCCESS, status );

        mcapi_sclchan_recv_open_i( &handy, receiver, &request, &status );
        sassert( MCAPI_PENDING, status );

        mcapi_wait( &request, &size, 1000, &status );
        sassert( MCAPI_SUCCESS, status );

        wait(NULL);

        recv = mcapi_sclchan_available( handy, &status );
        sassert( MCAPI_SUCCESS, status );
        uassert( 0 == recv );

        mcapi_finalize( &status );
    }
    else
    {
        perror("fork");
    }
}

//does produce errors if too long for posix message queue
test(scl_wrong_size)

    pid = fork();

    if ( pid == 0 )
    {
        mcapi_sclchan_send_hndl_t handy;
        struct endPointID us_id = SSCL16;
        struct endPointID them_id = RSCL16;

        mcapi_initialize( us_id.domain_id, us_id.node_id, 0, 0, &info, &status );
        sender = mcapi_endpoint_create( us_id.port_id, &status );

        mcapi_sclchan_send_open_i( &handy, sender, &request, &status );
        sassert( MCAPI_PENDING, status );

        mcapi_wait( &request, &size, 1001, &status );
        sassert( MCAPI_SUCCESS, status );

        mcapi_sclchan_send_uint32( handy, send, &status );
        sassert( MCAPI_ERR_TRANSMISSION, status );

        mcapi_finalize( &status );
        exit(0);
    }
    else if ( pid != -1 )
    {
        mcapi_sclchan_recv_hndl_t handy;
        struct endPointID us_id = RSCL16;
        struct endPointID them_id = SSCL16;

        mcapi_initialize( us_id.domain_id, us_id.node_id, 0, 0, &info, &status );
        receiver = mcapi_endpoint_create( us_id.port_id, &status );
        sender = mcapi_endpoint_get( them_id.domain_id, them_id.node_id,
        them_id.port_id, 1000, &status );
        
        mcapi_sclchan_connect_i( sender, receiver, &request, &status );
        sassert( MCAPI_PENDING, status );
        mcapi_wait( &request, &size, 1000, &status );
        sassert( MCAPI_SUCCESS, status );

        mcapi_sclchan_recv_open_i( &handy, receiver, &request, &status );
        sassert( MCAPI_PENDING, status );

        mcapi_wait( &request, &size, 1000, &status );
        sassert( MCAPI_SUCCESS, status );

        recv = mcapi_sclchan_recv_uint8( handy, &status );
        sassert( MCAPI_ERR_TRANSMISSION, status );

        wait(NULL);

        mcapi_finalize( &status );
    }
    else
    {
        perror("fork");
    }
}

//does not produce errors, but must not crash either
test(scl_wrong_size_2)

    pid = fork();

    if ( pid == 0 )
    {
        mcapi_sclchan_send_hndl_t handy;
        struct endPointID us_id = SSCL16;
        struct endPointID them_id = RSCL16;

        mcapi_initialize( us_id.domain_id, us_id.node_id, 0, 0, &info, &status );
        sender = mcapi_endpoint_create( us_id.port_id, &status );

        mcapi_sclchan_send_open_i( &handy, sender, &request, &status );
        sassert( MCAPI_PENDING, status );

        mcapi_wait( &request, &size, 1001, &status );
        sassert( MCAPI_SUCCESS, status );

        mcapi_sclchan_send_uint8( handy, send, &status );
        sassert( MCAPI_SUCCESS, status );

        mcapi_finalize( &status );
        exit(0);
    }
    else if ( pid != -1 )
    {
        mcapi_sclchan_recv_hndl_t handy;
        struct endPointID us_id = RSCL16;
        struct endPointID them_id = SSCL16;

        mcapi_initialize( us_id.domain_id, us_id.node_id, 0, 0, &info, &status );
        receiver = mcapi_endpoint_create( us_id.port_id, &status );
        sender = mcapi_endpoint_get( them_id.domain_id, them_id.node_id,
        them_id.port_id, 1000, &status );
        
        mcapi_sclchan_connect_i( sender, receiver, &request, &status );
        sassert( MCAPI_PENDING, status );
        mcapi_wait( &request, &size, 1000, &status );
        sassert( MCAPI_SUCCESS, status );

        mcapi_sclchan_recv_open_i( &handy, receiver, &request, &status );
        sassert( MCAPI_PENDING, status );

        mcapi_wait( &request, &size, 1000, &status );
        sassert( MCAPI_SUCCESS, status );

        recv = mcapi_sclchan_recv_uint32( handy, &status );
        sassert( MCAPI_SUCCESS, status );

        wait(NULL);

        mcapi_finalize( &status );
    }
    else
    {
        perror("fork");
    }
}

void suite_scalar_tx_rx()
{
    nullhand.us = MCAPI_NULL;

    dotest(scl_send_recv_fail_init)
    dotest(scl_send_recv)
    dotest(scl_send_recv_fail_trans)
    dotest(scl_send_recv_timeout)
    dotest(scl_send_recv_init)
    dotest(scl_send_recv_inva_chan)
    dotest(scl_close)
    dotest(scl_re_open)
    dotest(scl_send_recv_8bits)
    dotest(scl_send_recv_16bits)
    dotest(scl_send_recv_32bits)
    dotest(scl_avail)
    dotest(scl_not_avail)
    dotest(scl_wrong_size)
    dotest(scl_wrong_size_2)
}
