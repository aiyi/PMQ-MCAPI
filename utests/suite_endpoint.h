//this suite tests generic endpoint functions
#include <mcapi.h>
#include "mcapi_impl_spec.h"
#include "utester.h"
#include <time.h>

//an invalid endpoint usable in all tests as such!
static struct endPointData iepd;
static mcapi_info_t info;
static mcapi_status_t status;

//must not succeed if node is not inited
test(delete_init_fail)
    mcapi_endpoint_t sender;
    mcapi_endpoint_delete( sender, &status );

    uassert( status == MCAPI_ERR_NODE_NOTINIT );

    mcapi_finalize( &status );
}

//the endpoint beign deleted must not be null
test(delete_null_fail)
    mcapi_initialize( 1, 2, 0, 0, &info, &status );

    mcapi_endpoint_t sender = MCAPI_NULL;
    mcapi_endpoint_delete( sender, &status );

    uassert( status == MCAPI_ERR_ENDP_INVALID );

    mcapi_finalize( &status );
}

//endpoint beign deleted must be inited
test(delete_invalid_fail)
    mcapi_initialize( 1, 2, 0, 0, &info, &status );

    mcapi_endpoint_t sender;
    mcapi_endpoint_delete( &iepd, &status );

    uassert( status == MCAPI_ERR_ENDP_INVALID );

    mcapi_finalize( &status );
}

//endpoint beign deleted must be in own node
test(delete_own_fail_node)
    mcapi_initialize( 1, 2, 0, 0, &info, &status );

    mcapi_endpoint_t sender;
    sender = mcapi_endpoint_create( 0, &status );
    sassert( MCAPI_SUCCESS, status );
    sender->defs->us.node_id = -1;
    mcapi_endpoint_delete( sender, &status );

    sassert( MCAPI_ERR_ENDP_NOTOWNER, status );
    sender->defs->us.node_id = 2;

    mcapi_finalize( &status );
}

//endpoint beign deleted ought to be in own domain
test(delete_own_fail_domain)
    mcapi_initialize( 1, 2, 0, 0, &info, &status );

    mcapi_endpoint_t sender;
    sender = mcapi_endpoint_create( 0, &status );
    sender->defs->us.domain_id = -1;
    mcapi_endpoint_delete( sender, &status );

    sassert( MCAPI_ERR_ENDP_NOTOWNER, status );
    sender->defs->us.domain_id = 1;

    mcapi_finalize( &status );
}

//successfull deletion of endpoint
test(end_delete)
    mcapi_initialize( 1, 2, 0, 0, &info, &status );

    mcapi_endpoint_t sender;
    sender = mcapi_endpoint_create( 0, &status );
    mcapi_endpoint_delete( sender, &status );

    sassert( MCAPI_SUCCESS, status );

    mcapi_finalize( &status );
}

//successful creation of endpoint
test(end_create)
    mcapi_initialize( 1, 2, 0, 0, &info, &status );

    mcapi_endpoint_t sender;
    sender = mcapi_endpoint_create( 0, &status );

    uassert( status == MCAPI_SUCCESS );
    uassert( sender != MCAPI_NULL );

    mcapi_endpoint_delete( sender, &status );
    mcapi_finalize( &status );
}

//successful creation of endpoint, which was deleted before
test(end_create_delete)
    mcapi_initialize( 1, 2, 0, 0, &info, &status );

    mcapi_endpoint_t sender;
    sender = mcapi_endpoint_create( 0, &status );
    mcapi_endpoint_delete( sender, &status );
    sender = mcapi_endpoint_create( 0, &status );

    sassert( MCAPI_SUCCESS, status );
    uassert( sender != MCAPI_NULL );

    mcapi_endpoint_delete( sender, &status );
    mcapi_finalize( &status );
}

//must initialize node before endpoint creation
test(create_init_fail)

    mcapi_endpoint_t sender;
    sender = mcapi_endpoint_create( 0, &status );

    uassert( status == MCAPI_ERR_NODE_NOTINIT );
    uassert( sender == MCAPI_NULL );

    mcapi_finalize( &status );
}

//cannot create same endpoint twice
test(create_exists_fail)
    mcapi_initialize( 1, 2, 0, 0, &info, &status );

    mcapi_endpoint_t sender;
    mcapi_endpoint_t sender2;
    sender = mcapi_endpoint_create( 1, &status );
    sender2 = mcapi_endpoint_create( 1, &status );

    sassert( MCAPI_ERR_ENDP_EXISTS, status );
    uassert( sender2 == MCAPI_NULL );

    mcapi_endpoint_delete( sender, &status );
    mcapi_finalize( &status );
}

//cannot create undefined endpoint
test(create_no_def_fail)
    mcapi_initialize( 66, 55, 0, 0, &info, &status );

    mcapi_endpoint_t sender;
    sender = mcapi_endpoint_create( 44, &status );

    sassert( MCAPI_ERR_PORT_INVALID, status );
    uassert( sender == MCAPI_NULL );

    mcapi_endpoint_delete( sender, &status );
    mcapi_finalize( &status );
}

//un-succesfull creation of end point of "any port"
test(any_port_create)
    mcapi_initialize( 1, 2, 0, 0, &info, &status );

    mcapi_endpoint_t sender;
    sender = mcapi_endpoint_create( MCAPI_PORT_ANY, &status );

    sassert( MCAPI_ERR_PORT_INVALID, status );
    uassert( sender == MCAPI_NULL );

    mcapi_endpoint_delete( sender, &status );
    mcapi_finalize( &status );
}

//successful retrieval of endpoint
test(get_end)
    mcapi_endpoint_t receiver;
    mcapi_endpoint_t ureceiver;

    mcapi_initialize( 1, 2, 0, 0, &info, &status );
    
    ureceiver = mcapi_endpoint_create( 1, &status );
    receiver = mcapi_endpoint_get( 1, 2, 1, 1000, &status );
    uassert( status == MCAPI_SUCCESS );
    uassert( receiver != MCAPI_NULL );

    mcapi_endpoint_delete( ureceiver, &status );
    mcapi_finalize( &status );
}

//get must fail if node not inited
test(get_init_fail)
    mcapi_endpoint_t receiver;

    receiver = mcapi_endpoint_get( 1, 2, 1, 1000, &status );
    uassert( status == MCAPI_ERR_NODE_NOTINIT );
    uassert( receiver == MCAPI_NULL );
}

//get must fail if port is not defined
test(get_epd_fail)
    mcapi_endpoint_t receiver;

    mcapi_initialize( 15, 66, 0, 0, &info, &status );

    receiver = mcapi_endpoint_get( 1, 2, -1, 1000, &status );
    uassert( status == MCAPI_ERR_PORT_INVALID );
    uassert( receiver == MCAPI_NULL );
    mcapi_finalize( &status );
}

//get must fail if the timeout passes
test(get_timeout)
    mcapi_endpoint_t receiver;
    struct timespec t_start, t_end;
    int timeout = 500;

    mcapi_initialize( 1, 2, 0, 0, &info, &status );

    clock_gettime( CLOCK_MONOTONIC, &t_start );
    receiver = mcapi_endpoint_get( 1, 2, 0, timeout, &status );
    clock_gettime( CLOCK_MONOTONIC, &t_end );
    uassert( status == MCAPI_TIMEOUT );
    uassert( receiver == MCAPI_NULL );

    double diff = (double)( t_end.tv_nsec - t_start.tv_nsec );
    diff += ( t_end.tv_sec - t_start.tv_sec ) * 1000000000;
    
    uassert( diff > ( timeout * 1000000 ) );

    mcapi_finalize( &status );
}

//succesfully set endpoint attribute
test(endpoint_set_attr)
    mcapi_endpoint_t sender;
    mcapi_timeout_t timeout = 100;

    mcapi_initialize( 1, 2, 0, 0, &info, &status );
    
    sender = mcapi_endpoint_create( 0, &status );

    mcapi_endpoint_set_attribute( sender, MCAPI_ENDP_ATTR_TIMEOUT, &timeout,
    sizeof(mcapi_timeout_t), &status );
    sassert( MCAPI_SUCCESS, status );

    mcapi_finalize( &status );
}

//invalid endpoint for attribute set
test(endpoint_set_attr_inva)
    mcapi_endpoint_t sender;
    mcapi_timeout_t timeout = 100;

    mcapi_initialize( 1, 2, 0, 0, &info, &status );
    
    sender = MCAPI_NULL;

    mcapi_endpoint_set_attribute( sender, MCAPI_ENDP_ATTR_TIMEOUT, &timeout,
    sizeof(mcapi_timeout_t), &status );
    sassert( MCAPI_ERR_ENDP_INVALID, status );

    mcapi_finalize( &status );
}

//must not set attributes of remote endpoints!
test(endpoint_set_attr_remote)
    mcapi_endpoint_t sender;
    mcapi_timeout_t timeout = 100;
    pid_t pid;

    pid = fork();

    if ( pid == 0 )
    {
        mcapi_initialize( 0, 5, 0, 0, &info, &status );
        
        sender = mcapi_endpoint_create( 1, &status );

        mcapi_finalize( &status );

        exit(0);
    }
    else if ( pid != -1 )
    {
        mcapi_initialize( 1, 2, 0, 0, &info, &status );

        sender = mcapi_endpoint_get( 0, 5, 1, timeout, &status );

        mcapi_endpoint_set_attribute( sender, MCAPI_ENDP_ATTR_TIMEOUT, &timeout,
        sizeof(mcapi_timeout_t), &status );
        sassert(MCAPI_ERR_ENDP_REMOTE, status );

        wait(NULL);

        mcapi_finalize( &status );
    }
    else
    {
        perror("fork");
    }
}

//must be existing attribute
test(endpoint_set_attr_ext)
    mcapi_endpoint_t sender;
    mcapi_timeout_t timeout = 100;

    mcapi_initialize( 1, 2, 0, 0, &info, &status );
    
    sender = mcapi_endpoint_create( 0, &status );

    mcapi_endpoint_set_attribute( sender, MCAPI_ENDP_ATTR_END+1, &timeout,
    sizeof(mcapi_timeout_t), &status );
    sassert( MCAPI_ERR_ATTR_NUM, status );
    mcapi_endpoint_set_attribute( sender, -1, &timeout,
    sizeof(mcapi_timeout_t), &status );
    sassert( MCAPI_ERR_ATTR_NUM, status );


    mcapi_finalize( &status );
}

void suite_endpoint()
{
    iepd.inited = -1;

    dotest(delete_init_fail)
    dotest(delete_null_fail)
    dotest(delete_invalid_fail)
    dotest(delete_own_fail_node)
    dotest(delete_own_fail_domain)
    dotest(end_delete)
    dotest(end_create)
    dotest(end_create_delete)
    dotest(any_port_create)
    dotest(create_init_fail)
    dotest(create_exists_fail)
    dotest(create_no_def_fail)
    dotest(get_end)
    dotest(get_epd_fail)
    dotest(get_init_fail)
    dotest(get_timeout)
    dotest(endpoint_set_attr)
    dotest(endpoint_set_attr_inva)
    dotest(endpoint_set_attr_remote)
    dotest(endpoint_set_attr_ext)
}
