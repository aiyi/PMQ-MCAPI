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

//must not work if node is noty initialized
test(scl_conn_fail_init)
    mcapi_sclchan_connect_i( sender, receiver, &request, &status );
    sassert( MCAPI_ERR_NODE_NOTINIT, status );
}

//a valid request parameter must be provided
test(scl_conn_fail_req_null)
    struct endPointID us_id = SSCL;
    struct endPointID them_id = RSCL;
    mcapi_initialize( us_id.domain_id, us_id.node_id, 0, 0, &info, &status );

    mcapi_sclchan_connect_i( sender, receiver, NULL, &status );
    sassert( MCAPI_ERR_PARAMETER, status );

    mcapi_finalize( &status );
}

//endpoints must not be null
test(scl_conn_fail_endp_null)
    struct endPointID us_id = SSCL;
    struct endPointID them_id = RSCL;

    mcapi_initialize( us_id.domain_id, us_id.node_id, 0, 0, &info, &status );

    receiver = mcapi_endpoint_create( us_id.port_id, &status );
    sender = mcapi_endpoint_create( them_id.port_id, &status );

    mcapi_sclchan_connect_i( MCAPI_NULL, MCAPI_NULL, &request, &status );
    sassert( MCAPI_ERR_ENDP_INVALID, status );

    mcapi_sclchan_connect_i( MCAPI_NULL, receiver, &request, &status );
    sassert( MCAPI_ERR_ENDP_INVALID, status );

    mcapi_sclchan_connect_i( sender, MCAPI_NULL, &request, &status );
    sassert( MCAPI_ERR_ENDP_INVALID, status );

    mcapi_finalize( &status );
}

//endpoints msut not be same
test(scl_conn_fail_endp_same)
    struct endPointID us_id = SSCL;
    struct endPointID them_id = RSCL;

    mcapi_initialize( us_id.domain_id, us_id.node_id, 0, 0, &info, &status );

    sender = mcapi_endpoint_create( them_id.port_id, &status );

    mcapi_sclchan_connect_i( sender, sender, &request, &status );
    sassert( MCAPI_ERR_ENDP_INVALID, status );

    mcapi_finalize( &status );
}

//endpoint definitions must exist
test(scl_conn_fail_def_null)
    mcapi_initialize( 0, 0, 0, 0, &info, &status );

    sender = mcapi_endpoint_create( 0, &status );
    receiver = mcapi_endpoint_create( 1, &status );

    mcapi_sclchan_connect_i( sender, receiver, &request, &status );
    sassert( MCAPI_ERR_ENDP_INVALID, status );

    mcapi_finalize( &status );
}

//the endpoint definions must match
test(scl_conn_fail_def_inva)
    struct endPointID us_id = SSCL;
    struct endPointID them_id = RSCL;

    mcapi_initialize( us_id.domain_id, us_id.node_id, 0, 0, &info, &status );

    receiver = mcapi_endpoint_create( us_id.port_id, &status );
    sender = mcapi_endpoint_create( them_id.port_id, &status );

    mcapi_sclchan_connect_i( sender, receiver, &request, &status );
    sassert( MCAPI_ERR_ENDP_INVALID, status );

    mcapi_finalize( &status );
}

//must not work if node is not initialized
test(scl_open_fail_init)
    mcapi_sclchan_recv_open_i( NULL, sender, &request, &status );
    sassert( MCAPI_ERR_NODE_NOTINIT, status );
    mcapi_sclchan_send_open_i( NULL, sender, &request, &status );
    sassert( MCAPI_ERR_NODE_NOTINIT, status );
}

//handles must not be null
test(scl_open_fail_handy)
    mcapi_initialize( 0, 0, 0, 0, &info, &status );

    mcapi_sclchan_recv_open_i( NULL, sender, &request, &status );
    sassert( MCAPI_ERR_PARAMETER, status );
    mcapi_sclchan_send_open_i( NULL, sender, &request, &status );
    sassert( MCAPI_ERR_PARAMETER, status );

    mcapi_finalize( &status );
}

//directions must match definitions
test(scl_open_fail_dir)

    pid = fork();

    if ( pid == 0 )
    {
        mcapi_sclchan_send_hndl_t handy;
        struct endPointID us_id = SSCL;
        struct endPointID them_id = RSCL;

        mcapi_initialize( us_id.domain_id, us_id.node_id, 0, 0, &info, &status );
        sender = mcapi_endpoint_create( us_id.port_id, &status );

        mcapi_sclchan_recv_open_i( &handy, sender, &request, &status );
        sassert( MCAPI_ERR_CHAN_DIRECTION, status );

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

        mcapi_sclchan_send_open_i( &handy, receiver, &request, &status );
        sassert( MCAPI_ERR_CHAN_DIRECTION, status );

        wait(NULL);

        mcapi_finalize( &status );
    }
    else
    {
        perror("fork");
    }
}

//endpoints must exist
test(scl_open_fail_endp)

    pid = fork();

    if ( pid == 0 )
    {
        mcapi_sclchan_send_hndl_t handy;
        struct endPointID us_id = SSCL;
        struct endPointID them_id = RSCL;

        mcapi_initialize( us_id.domain_id, us_id.node_id+1, 0, 0, &info, &status );
        sassert( MCAPI_SUCCESS, status );
        sender = mcapi_endpoint_create( us_id.port_id, &status );

        mcapi_sclchan_send_open_i( &handy, NULL, &request, &status );
        sassert( MCAPI_ERR_ENDP_INVALID, status );
        mcapi_sclchan_send_open_i( &handy, sender, &request, &status );
        sassert( MCAPI_ERR_ENDP_INVALID, status );

        mcapi_finalize( &status );
        exit(0);
    }
    else if ( pid != -1 )
    {
        mcapi_sclchan_recv_hndl_t handy;
        struct endPointID us_id = RSCL;
        struct endPointID them_id = SSCL;

        mcapi_initialize( us_id.domain_id, us_id.node_id+1, 0, 0, &info, &status );
        sassert( MCAPI_SUCCESS, status );
        receiver = mcapi_endpoint_create( us_id.port_id, &status );

        mcapi_sclchan_recv_open_i( &handy, NULL, &request, &status );
        sassert( MCAPI_ERR_ENDP_INVALID, status );
        mcapi_sclchan_recv_open_i( &handy, receiver, &request, &status );
        sassert( MCAPI_ERR_ENDP_INVALID, status );

        wait(NULL);

        mcapi_finalize( &status );
    }
    else
    {
        perror("fork");
    }
}

//request parameter must be valid
test(scl_open_fail_req_null)
    mcapi_sclchan_send_hndl_t handy;
    struct endPointID us_id = SSCL;
    struct endPointID them_id = RSCL;

    mcapi_initialize( us_id.domain_id, us_id.node_id, 0, 0, &info, &status );

    sender = mcapi_endpoint_create( us_id.port_id, &status );

    mcapi_sclchan_send_open_i( &handy, sender, NULL, &status );
    sassert( MCAPI_ERR_PARAMETER, status );

    mcapi_finalize( &status );
}

//channel type must be right
test(scl_open_fail_chan_type)
    mcapi_sclchan_send_hndl_t handy;
    struct endPointID us_id = SSCL;
    struct endPointID them_id = RSCL;

    mcapi_initialize( us_id.domain_id, us_id.node_id, 0, 0, &info, &status );

    sender = mcapi_endpoint_create( us_id.port_id, &status );
    sender->defs->type = MCAPI_PKT_CHAN;

    mcapi_sclchan_send_open_i( &handy, sender, &request, &status );
    sassert( MCAPI_ERR_CHAN_TYPE, status );
    sender->defs->type = MCAPI_SCL_CHAN;

    mcapi_finalize( &status );
}

//cannot open channel if already open
test(scl_open_fail_open)

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
        sassert( MCAPI_SUCCESS, status );
        mcapi_sclchan_send_open_i( &handy, sender, &request, &status );
        sassert( MCAPI_ERR_CHAN_OPEN, status );

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
        mcapi_sclchan_recv_open_i( &handy, receiver, &request, &status );
        mcapi_wait( &request, &size, 1000, &status );
        sassert( MCAPI_SUCCESS, status );
        mcapi_sclchan_recv_open_i( &handy, receiver, &request, &status );
        sassert( MCAPI_ERR_CHAN_OPEN, status );

        wait(NULL);

        mcapi_finalize( &status );
    }
}

//must timeout if the other end does not produce itself
test(scl_open_fail_timeout_endp)
    mcapi_sclchan_recv_hndl_t handy;
    struct endPointID us_id = RSCL;
    struct endPointID them_id = SSCL;

    mcapi_initialize( us_id.domain_id, us_id.node_id, 0, 0, &info, &status );
    receiver = mcapi_endpoint_create( us_id.port_id, &status );
    sender = mcapi_endpoint_get( them_id.domain_id, them_id.node_id,
    them_id.port_id, 100, &status );
    sassert( MCAPI_TIMEOUT, status );
    
    mcapi_sclchan_connect_i( sender, receiver, &request, &status );
    mcapi_sclchan_recv_open_i( &handy, receiver, &request, &status );
    mcapi_wait( &request, &size, 100, &status );
    sassert( MCAPI_TIMEOUT, status );

    mcapi_finalize( &status );
}

void suite_scalar_con_open()
{
    iepd.inited = -1;

    dotest(scl_conn_fail_init)
    dotest(scl_conn_fail_req_null)
    dotest(scl_conn_fail_endp_null)
    dotest(scl_conn_fail_endp_same)
    dotest(scl_conn_fail_def_null)
    dotest(scl_conn_fail_def_inva)
    dotest(scl_open_fail_init)
    dotest(scl_open_fail_handy)
    dotest(scl_open_fail_init)
    dotest(scl_open_fail_dir)
    dotest(scl_open_fail_endp)
    dotest(scl_open_fail_req_null)
    dotest(scl_open_fail_chan_type)
    dotest(scl_open_fail_open)
    dotest(scl_open_fail_timeout_endp)
}
