//this suite tests connectionless message communications
#include <mcapi.h>
#include "utester.h"
#include <string.h>
#include <stdlib.h>
#include <time.h>

#define MAX_MSG_LEN     30
#define TEST_MESSAGE "THIS THE THE TEST MESSAGE"

static struct endPointData iepd;
static mcapi_info_t info;
static mcapi_status_t status;

//succesful send and receive
test(msg_send_recv)
    mcapi_endpoint_t sender;
    mcapi_endpoint_t receiver;
    mcapi_endpoint_t ureceiver;
    char send_buf[MAX_MSG_LEN];
    char recv_buf[MAX_MSG_LEN];
    size_t received_size;

    strncpy(send_buf, TEST_MESSAGE, MAX_MSG_LEN);

    mcapi_initialize( 1, 2, 0, 0, &info, &status );
    
    sender = mcapi_endpoint_create( 0, &status );
    ureceiver = mcapi_endpoint_create( 1, &status );
    receiver = mcapi_endpoint_get( 1, 2, 1, 1000, &status );
    sassert( MCAPI_SUCCESS, status );
    mcapi_msg_send( sender, receiver, send_buf, MAX_MSG_LEN, 0, &status );
    sassert( MCAPI_SUCCESS, status );
    mcapi_msg_recv( ureceiver, recv_buf, MAX_MSG_LEN, &received_size, &status );

    sassert( MCAPI_SUCCESS, status );
    uassert( received_size == MAX_MSG_LEN );
    uassert2( 0, memcmp( send_buf, recv_buf, MAX_MSG_LEN ) );

    mcapi_endpoint_delete( sender, &status );
    mcapi_endpoint_delete( ureceiver, &status );
    mcapi_finalize( &status );
}

//fails to timeout
test(msg_send_recv_timeout)
    mcapi_endpoint_t sender;
    mcapi_endpoint_t receiver;
    mcapi_endpoint_t ureceiver;
    char send_buf[MAX_MSG_LEN];
    char recv_buf[MAX_MSG_LEN];
    size_t received_size;
    mcapi_timeout_t timeout = 100;
    unsigned int i = 0;
    struct timespec t_start, t_end;

    strncpy(send_buf, TEST_MESSAGE, MAX_MSG_LEN);

    mcapi_initialize( 1, 2, 0, 0, &info, &status );
    
    sender = mcapi_endpoint_create( 0, &status );
    ureceiver = mcapi_endpoint_create( 1, &status );

    mcapi_endpoint_set_attribute( sender, MCAPI_ENDP_ATTR_TIMEOUT, &timeout,
    sizeof(mcapi_timeout_t), &status );
    mcapi_endpoint_set_attribute( ureceiver, MCAPI_ENDP_ATTR_TIMEOUT, &timeout,
    sizeof(mcapi_timeout_t), &status );

    receiver = mcapi_endpoint_get( 1, 2, 1, 1000, &status );
    clock_gettime( CLOCK_MONOTONIC, &t_start );
    mcapi_msg_recv( ureceiver, recv_buf, MAX_MSG_LEN, &received_size, &status );
    clock_gettime( CLOCK_MONOTONIC, &t_end );
    sassert( MCAPI_TIMEOUT, status );

    double diff = (double)( t_end.tv_nsec - t_start.tv_nsec );
    diff += ( t_end.tv_sec - t_start.tv_sec ) * 1000000000;
    
    uassert( diff > ( timeout * 1000000 ) );

    for ( ; i < 11; ++i )
    {
        clock_gettime( CLOCK_MONOTONIC, &t_start );
        mcapi_msg_send( sender, receiver, send_buf, MAX_MSG_LEN, 0, &status );
        clock_gettime( CLOCK_MONOTONIC, &t_end );
    }

    sassert( MCAPI_TIMEOUT, status );

    diff = (double)( t_end.tv_nsec - t_start.tv_nsec );
    diff += ( t_end.tv_sec - t_start.tv_sec ) * 1000000000;
    
    uassert( diff > ( timeout * 1000000 ) );

    mcapi_finalize( &status );
}

//must not succeed if not inited
test(msg_send_recv_fail_init)
    mcapi_endpoint_t sender;
    mcapi_endpoint_t receiver;
    mcapi_endpoint_t ureceiver;
    char send_buf[MAX_MSG_LEN];
    char recv_buf[MAX_MSG_LEN];
    size_t received_size;

    mcapi_msg_send( sender, receiver, send_buf, MAX_MSG_LEN, 0, &status );
    sassert( MCAPI_ERR_NODE_NOTINIT, status );
    mcapi_msg_recv( ureceiver, recv_buf, MAX_MSG_LEN, &received_size, &status );
    sassert( MCAPI_ERR_NODE_NOTINIT, status );
}

//must not succeed, if endpoints are null
test(msg_send_recv_fail_endp_null)
    mcapi_endpoint_t sender = MCAPI_NULL;
    mcapi_endpoint_t receiver = MCAPI_NULL;
    mcapi_endpoint_t ureceiver = MCAPI_NULL;
    char send_buf[MAX_MSG_LEN];
    char recv_buf[MAX_MSG_LEN];
    size_t received_size;

    mcapi_initialize( 1, 2, 0, 0, &info, &status );

    mcapi_msg_send( sender, receiver, send_buf, MAX_MSG_LEN, 0, &status );
    sassert( MCAPI_ERR_ENDP_INVALID, status );
    mcapi_msg_recv( receiver, recv_buf, MAX_MSG_LEN, &received_size, &status );
    sassert( MCAPI_ERR_ENDP_INVALID, status );

    mcapi_finalize( &status );
}

//must not succeed, if endpoints are not inited
test(msg_send_recv_fail_endp_inva)
    mcapi_endpoint_t sender;
    mcapi_endpoint_t receiver;
    char send_buf[MAX_MSG_LEN];
    char recv_buf[MAX_MSG_LEN];
    size_t received_size;

    mcapi_initialize( 1, 2, 0, 0, &info, &status );

    sender = mcapi_endpoint_create( 0, &status );
    sassert( MCAPI_SUCCESS, status );
    receiver = mcapi_endpoint_create( 1, &status );
    sassert( MCAPI_SUCCESS, status );
    sender->inited = -1;
    receiver->inited = -1;

    mcapi_msg_send( sender, receiver, send_buf, MAX_MSG_LEN, 0, &status );
    sassert( MCAPI_ERR_ENDP_INVALID, status );
    mcapi_msg_recv( receiver, recv_buf, MAX_MSG_LEN, &received_size, &status );
    sassert( MCAPI_ERR_ENDP_INVALID, status );

    mcapi_finalize( &status );
}

//must not accept null buffer
test(msg_send_recv_fail_buff)
    mcapi_endpoint_t sender;
    mcapi_endpoint_t receiver;
    mcapi_endpoint_t ureceiver;
    size_t received_size;

    mcapi_initialize( 1, 2, 0, 0, &info, &status );

    sender = mcapi_endpoint_create( 0, &status );
    ureceiver = mcapi_endpoint_create( 1, &status );
    receiver = mcapi_endpoint_get( 1, 2, 1, 1000, &status );

    mcapi_msg_send( sender, receiver, NULL, MAX_MSG_LEN, 0, &status );
    sassert( MCAPI_ERR_PARAMETER, status );
    mcapi_msg_recv( ureceiver, NULL, MAX_MSG_LEN, &received_size, &status );
    sassert( MCAPI_ERR_PARAMETER, status );

    mcapi_endpoint_delete( sender, &status );
    mcapi_endpoint_delete( ureceiver, &status );
    mcapi_finalize( &status );
}

//must not accept too big size in send and null size on receive
test(msg_send_recv_fail_size)
    mcapi_endpoint_t sender;
    mcapi_endpoint_t receiver;
    mcapi_endpoint_t ureceiver;
    char send_buf[MAX_MSG_LEN];
    char recv_buf[MAX_MSG_LEN];
    size_t received_size;

    mcapi_initialize( 1, 2, 0, 0, &info, &status );

    sender = mcapi_endpoint_create( 0, &status );
    ureceiver = mcapi_endpoint_create( 1, &status );
    receiver = mcapi_endpoint_get( 1, 2, 1, 1000, &status );

    mcapi_msg_send( sender, receiver, send_buf, MCAPI_MAX_MESSAGE_SIZE+1, 0, 
    &status );
    sassert( MCAPI_ERR_MSG_SIZE, status );
    mcapi_msg_recv( ureceiver, recv_buf, MAX_MSG_LEN, NULL, &status );
    sassert( MCAPI_ERR_PARAMETER, status );

    mcapi_endpoint_delete( sender, &status );
    mcapi_endpoint_delete( ureceiver, &status );
    mcapi_finalize( &status );
}

//could send message to non-connected endpoint, albeit definition
test(msg_send_recv_fail_chan)
    mcapi_endpoint_t sender;
    mcapi_endpoint_t receiver;
    mcapi_endpoint_t ureceiver;
    char send_buf[MAX_MSG_LEN];
    char recv_buf[MAX_MSG_LEN];
    size_t received_size;
    struct endPointID us_id = SEND;
    struct endPointID them_id = RECV;

    strncpy(send_buf, TEST_MESSAGE, MAX_MSG_LEN);

    mcapi_initialize( us_id.domain_id, us_id.node_id, 0, 0, &info, &status );
    
    sender = mcapi_endpoint_create( us_id.port_id, &status );
    ureceiver = mcapi_endpoint_create( them_id.port_id, &status );
    receiver = mcapi_endpoint_get( them_id.domain_id, them_id.node_id,
    them_id.port_id, 1000, &status );
    mcapi_msg_send( sender, receiver, send_buf, MAX_MSG_LEN, 0, &status );
    sassert( MCAPI_SUCCESS, status );
    mcapi_msg_recv( ureceiver, recv_buf, MAX_MSG_LEN, &received_size, &status );
    sassert( MCAPI_SUCCESS, status );

    mcapi_finalize( &status );
}

//message priority must lay within limits
test(msg_send_fail_prio)
    mcapi_endpoint_t sender;
    mcapi_endpoint_t receiver;
    mcapi_endpoint_t ureceiver;
    char send_buf[MAX_MSG_LEN];

    mcapi_initialize( 1, 2, 0, 0, &info, &status );

    sender = mcapi_endpoint_create( 0, &status );
    ureceiver = mcapi_endpoint_create( 1, &status );
    receiver = mcapi_endpoint_get( 1, 2, 1, 1000, &status );

    mcapi_msg_send( sender, receiver, send_buf, MAX_MSG_LEN,
    MCAPI_MAX_PRIORITY+1, &status );
    sassert( MCAPI_ERR_PRIORITY, status );

    mcapi_endpoint_delete( sender, &status );
    mcapi_endpoint_delete( ureceiver, &status );
    mcapi_finalize( &status );
}

//truncation must be informed, yet we still expect fitting
//part to be found
test(msg_recv_fail_trunc)
    mcapi_endpoint_t sender;
    mcapi_endpoint_t receiver;
    mcapi_endpoint_t ureceiver;
    char send_buf[2];
    char recv_buf[1];
    size_t received_size;

    send_buf[0] = 'a';
    send_buf[1] = 'b';

    mcapi_initialize( 1, 2, 0, 0, &info, &status );
    
    sender = mcapi_endpoint_create( 0, &status );
    ureceiver = mcapi_endpoint_create( 1, &status );
    receiver = mcapi_endpoint_get( 1, 2, 1, 1000, &status );
    mcapi_msg_send( sender, receiver, &send_buf, 2, 0, &status );
    sassert( MCAPI_SUCCESS, status );
    mcapi_msg_recv( ureceiver, recv_buf, 1, &received_size, &status );

    sassert( MCAPI_ERR_MSG_TRUNCATED, status );
    uassert( received_size == 1 );
    uassert2( 0, memcmp( send_buf, recv_buf, 1 ) );

    mcapi_endpoint_delete( sender, &status );
    mcapi_endpoint_delete( ureceiver, &status );
    mcapi_finalize( &status );
}

//must work with zero-length messages
test(msg_send_recv_zero)
    mcapi_endpoint_t sender;
    mcapi_endpoint_t receiver;
    mcapi_endpoint_t ureceiver;
    char send_buf[0];
    char recv_buf[0];
    size_t received_size;

    mcapi_initialize( 1, 2, 0, 0, &info, &status );
    
    sender = mcapi_endpoint_create( 0, &status );
    ureceiver = mcapi_endpoint_create( 1, &status );
    receiver = mcapi_endpoint_get( 1, 2, 1, 1000, &status );
    mcapi_msg_send( sender, receiver, send_buf, 0, 0, &status );
    sassert( MCAPI_SUCCESS, status );
    mcapi_msg_recv( ureceiver, recv_buf, 0, &received_size, &status );

    sassert( MCAPI_SUCCESS, status );
    uassert( received_size == 0 );

    mcapi_endpoint_delete( sender, &status );
    mcapi_endpoint_delete( ureceiver, &status );
    mcapi_finalize( &status );
}

//size of message cant be below zero
test(msg_send_recv_sub_zero)
    mcapi_endpoint_t sender;
    mcapi_endpoint_t receiver;
    mcapi_endpoint_t ureceiver;
    char send_buf[1];
    char recv_buf[1];
    size_t received_size;

    send_buf[0] = 'a';

    mcapi_initialize( 1, 2, 0, 0, &info, &status );
    
    sender = mcapi_endpoint_create( 0, &status );
    ureceiver = mcapi_endpoint_create( 1, &status );
    receiver = mcapi_endpoint_get( 1, 2, 1, 1000, &status );
    mcapi_msg_send( sender, receiver, &send_buf, -1, 0, &status );
    sassert( MCAPI_ERR_MSG_SIZE, status );

    mcapi_endpoint_delete( sender, &status );
    mcapi_endpoint_delete( ureceiver, &status );
    mcapi_finalize( &status );
}

//even extremely small message must work
test(msg_send_recv_small)
    mcapi_endpoint_t sender;
    mcapi_endpoint_t receiver;
    mcapi_endpoint_t ureceiver;
    char send_buf[1];
    char recv_buf[1];
    size_t received_size;

    send_buf[0] = 'a';

    mcapi_initialize( 1, 2, 0, 0, &info, &status );
    
    sender = mcapi_endpoint_create( 0, &status );
    ureceiver = mcapi_endpoint_create( 1, &status );
    receiver = mcapi_endpoint_get( 1, 2, 1, 1000, &status );
    mcapi_msg_send( sender, receiver, &send_buf, 1, 0, &status );
    sassert( MCAPI_SUCCESS, status );
    mcapi_msg_recv( ureceiver, recv_buf, 1, &received_size, &status );

    sassert( MCAPI_SUCCESS, status );
    uassert( received_size == 1 );
    uassert2( 0, memcmp( send_buf, recv_buf, 1 ) );

    mcapi_endpoint_delete( sender, &status );
    mcapi_endpoint_delete( ureceiver, &status );
    mcapi_finalize( &status );
}

//tests if message reaching the upper lenght limit still works
test(msg_send_recv_big)
    mcapi_endpoint_t sender;
    mcapi_endpoint_t receiver;
    mcapi_endpoint_t ureceiver;
    char send_buf[MCAPI_MAX_MESSAGE_SIZE];
    char recv_buf[MCAPI_MAX_MESSAGE_SIZE];
    size_t received_size;
    unsigned int i;

    srand(send_buf[0]);

    for ( i = 0; i < MCAPI_MAX_MESSAGE_SIZE; ++i )
    {
        send_buf[i] = rand();
    }

    mcapi_initialize( 1, 2, 0, 0, &info, &status );
    
    sender = mcapi_endpoint_create( 0, &status );
    ureceiver = mcapi_endpoint_create( 1, &status );
    receiver = mcapi_endpoint_get( 1, 2, 1, 1000, &status );
    mcapi_msg_send( sender, receiver, send_buf, MCAPI_MAX_MESSAGE_SIZE, 0, 
    &status );
    sassert( MCAPI_SUCCESS, status );
    mcapi_msg_recv( ureceiver, recv_buf, MCAPI_MAX_MESSAGE_SIZE, &received_size, 
    &status );

    sassert( MCAPI_SUCCESS, status );
    uassert( received_size == MCAPI_MAX_MESSAGE_SIZE );
    uassert2( 0, memcmp( send_buf, recv_buf, MCAPI_MAX_MESSAGE_SIZE ) );

    mcapi_endpoint_delete( sender, &status );
    mcapi_endpoint_delete( ureceiver, &status );
    mcapi_finalize( &status );
}

//was avaibliable
test(msg_send_avail)
    mcapi_endpoint_t sender;
    mcapi_endpoint_t receiver;
    mcapi_endpoint_t ureceiver;
    char send_buf[MAX_MSG_LEN];
    char count;

    strncpy(send_buf, TEST_MESSAGE, MAX_MSG_LEN);

    mcapi_initialize( 1, 2, 0, 0, &info, &status );
    
    sender = mcapi_endpoint_create( 0, &status );
    ureceiver = mcapi_endpoint_create( 1, &status );
    receiver = mcapi_endpoint_get( 1, 2, 1, 1000, &status );
    mcapi_msg_send( sender, receiver, send_buf, MAX_MSG_LEN, 0, &status );
    sassert( MCAPI_SUCCESS, status );
    count = mcapi_msg_available( ureceiver, &status );

    sassert( MCAPI_SUCCESS, status );
    uassert( count == 1 );
    mcapi_finalize( &status );
}

//was not avaibliable
test(msg_send_not_avail)
    mcapi_endpoint_t ureceiver;
    char count;

    mcapi_initialize( 1, 2, 0, 0, &info, &status );
    
    ureceiver = mcapi_endpoint_create( 1, &status );
    count = mcapi_msg_available( ureceiver, &status );

    sassert( MCAPI_SUCCESS, status );
    uassert( count == 0 );
    mcapi_finalize( &status );
}

//was no longer avaibliable after recreation
test(msg_recreate_send_not_avail)
    mcapi_endpoint_t sender;
    mcapi_endpoint_t receiver;
    mcapi_endpoint_t ureceiver;
    char send_buf[MAX_MSG_LEN];
    char count;

    strncpy(send_buf, TEST_MESSAGE, MAX_MSG_LEN);

    mcapi_initialize( 1, 2, 0, 0, &info, &status );
    
    sender = mcapi_endpoint_create( 0, &status );
    ureceiver = mcapi_endpoint_create( 1, &status );
    receiver = mcapi_endpoint_get( 1, 2, 1, 1000, &status );
    mcapi_msg_send( sender, receiver, send_buf, MAX_MSG_LEN, 0, &status );
    mcapi_msg_send( sender, receiver, send_buf, MAX_MSG_LEN, 0, &status );
    mcapi_msg_send( sender, receiver, send_buf, MAX_MSG_LEN, 0, &status );

    mcapi_endpoint_delete( ureceiver, &status );
    sassert( MCAPI_SUCCESS, status );
    ureceiver = mcapi_endpoint_create( 1, &status );
    sassert( MCAPI_SUCCESS, status );

    count = mcapi_msg_available( ureceiver, &status );
    sassert( MCAPI_SUCCESS, status );
    uassert2( 0, count );
    mcapi_finalize( &status );
}

//succesful after recreation
test(msg_recreate_send_recv)
    mcapi_endpoint_t sender;
    mcapi_endpoint_t receiver;
    mcapi_endpoint_t ureceiver;
    char send_buf[MAX_MSG_LEN];
    char recv_buf[MAX_MSG_LEN];
    size_t received_size;
    char count;

    strncpy(send_buf, TEST_MESSAGE, MAX_MSG_LEN);

    mcapi_initialize( 1, 2, 0, 0, &info, &status );
    
    sender = mcapi_endpoint_create( 0, &status );
    ureceiver = mcapi_endpoint_create( 1, &status );
    receiver = mcapi_endpoint_get( 1, 2, 1, 1000, &status );
    mcapi_msg_send( sender, receiver, send_buf, MAX_MSG_LEN, 0, &status );
    sassert( MCAPI_SUCCESS, status );
    mcapi_msg_recv( ureceiver, recv_buf, MAX_MSG_LEN, &received_size, &status );

    mcapi_endpoint_delete( sender, &status );
    mcapi_endpoint_delete( ureceiver, &status );

    sender = mcapi_endpoint_create( 0, &status );
    ureceiver = mcapi_endpoint_create( 1, &status );
    receiver = mcapi_endpoint_get( 1, 2, 1, 1000, &status );
    mcapi_msg_send( sender, receiver, send_buf, MAX_MSG_LEN, 0, &status );
    sassert( MCAPI_SUCCESS, status );
    count = mcapi_msg_available( ureceiver, &status );

    sassert( MCAPI_SUCCESS, status );
    uassert( count == 1 );
    mcapi_finalize( &status );

    mcapi_finalize( &status );
}

//higher priority comes first
test(msg_send_recv_prio)
    mcapi_endpoint_t sender;
    mcapi_endpoint_t receiver;
    mcapi_endpoint_t ureceiver;
    char send_buf[MAX_MSG_LEN];
    char recv_buf[MAX_MSG_LEN];
    size_t received_size;

    strncpy(send_buf, TEST_MESSAGE, MAX_MSG_LEN);

    mcapi_initialize( 1, 2, 0, 0, &info, &status );
    
    sender = mcapi_endpoint_create( 0, &status );
    ureceiver = mcapi_endpoint_create( 1, &status );
    receiver = mcapi_endpoint_get( 1, 2, 1, 1000, &status );
    mcapi_msg_send( sender, receiver, send_buf, MAX_MSG_LEN, 1, &status );
    sassert( MCAPI_SUCCESS, status );
    strncpy(send_buf, "YKKONEN", MAX_MSG_LEN);
    mcapi_msg_send( sender, receiver, send_buf, MAX_MSG_LEN, 0, &status );

    mcapi_msg_recv( ureceiver, recv_buf, MAX_MSG_LEN, &received_size, &status );

    sassert( MCAPI_SUCCESS, status );
    uassert( received_size == MAX_MSG_LEN );
    uassert2( 0, memcmp( send_buf, recv_buf, MAX_MSG_LEN ) );

    strncpy(send_buf, TEST_MESSAGE, MAX_MSG_LEN);
    mcapi_msg_recv( ureceiver, recv_buf, MAX_MSG_LEN, &received_size, &status );

    sassert( MCAPI_SUCCESS, status );
    uassert( received_size == MAX_MSG_LEN );
    uassert2( 0, memcmp( send_buf, recv_buf, MAX_MSG_LEN ) );

    mcapi_endpoint_delete( sender, &status );
    mcapi_endpoint_delete( ureceiver, &status );
    mcapi_finalize( &status );
}

void suite_msg()
{
    iepd.inited = -1;

    dotest(msg_send_recv)
    dotest(msg_send_recv_timeout)
    dotest(msg_send_recv_fail_init)
    dotest(msg_send_recv_fail_endp_null)
    dotest(msg_send_recv_fail_endp_inva)
    dotest(msg_send_recv_fail_buff)
    dotest(msg_send_recv_fail_size)
    dotest(msg_send_recv_fail_chan)
    dotest(msg_send_fail_prio)
    dotest(msg_recv_fail_trunc)
    dotest(msg_send_recv_zero)
    dotest(msg_send_recv_sub_zero)
    dotest(msg_send_recv_small)
    dotest(msg_send_recv_big)
    dotest(msg_send_avail)
    dotest(msg_send_not_avail)
    dotest(msg_recreate_send_not_avail)
    dotest(msg_recreate_send_recv)
    dotest(msg_send_recv_prio)
}
