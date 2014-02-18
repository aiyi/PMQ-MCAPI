//this suite tests generic endpoint functions
#include <mcapi.h>
#include "mca_config.h"
#include "mcapi_impl_spec.h"
#include "utester.h"

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

//must fail, if port is below valid region
test(create_inva_lil_port_fail)
    mcapi_initialize( 1, 2, 0, 0, &info, &status );
    int testvalue = -1;

    if ( MCAPI_PORT_ANY == testvalue )
        testvalue = -2;

    mcapi_endpoint_t sender;
    sender = mcapi_endpoint_create( testvalue, &status );

    uassert( status == MCAPI_ERR_PORT_INVALID );
    uassert( sender == MCAPI_NULL );

    mcapi_finalize( &status );
}

//must fail, if port is above valid region
test(create_inva_big_port_fail)
    mcapi_initialize( 1, 2, 0, 0, &info, &status );

    mcapi_endpoint_t sender;
    sender = mcapi_endpoint_create( MCAPI_MAX_ENDPOINTS, &status );

    uassert( status == MCAPI_ERR_PORT_INVALID );
    uassert( sender == MCAPI_NULL );

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

//must fail if too many ports already
test(create_no_port_fail)
    mcapi_initialize( 1, 2, 0, 0, &info, &status );

    mcapi_endpoint_t sender;

    unsigned int i;

    for ( i  = 0; i < MCAPI_MAX_ENDPOINTS + 1; ++i )
    {
        sender = mcapi_endpoint_create( MCAPI_PORT_ANY, &status );
    }

    uassert( status == MCAPI_ERR_PORT_INVALID );
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

//get must fail if port is below acceptable region
test(get_lil_port_inva_fail)
    mcapi_endpoint_t receiver;

    mcapi_initialize( 1, 2, 0, 0, &info, &status );

    receiver = mcapi_endpoint_get( 1, 2, -1, 1000, &status );
    uassert( status == MCAPI_ERR_PORT_INVALID );
    uassert( receiver == MCAPI_NULL );
    mcapi_finalize( &status );
}

//get must fail if node is below acceptable region
test(get_lil_node_inva_fail)
    mcapi_endpoint_t receiver;

    mcapi_initialize( 1, 2, 0, 0, &info, &status );

    receiver = mcapi_endpoint_get( 1, -1, 1, 1000, &status );
    uassert( status == MCAPI_ERR_NODE_INVALID );
    uassert( receiver == MCAPI_NULL );
    mcapi_finalize( &status );
}

//get must fail if domain is below acceptable region
test(get_lil_domain_inva_fail)
    mcapi_endpoint_t receiver;

    mcapi_initialize( 1, 2, 0, 0, &info, &status );

    receiver = mcapi_endpoint_get( -1, 2, 1, 1000, &status );
    uassert( status == MCAPI_ERR_DOMAIN_INVALID );
    uassert( receiver == MCAPI_NULL );
    mcapi_finalize( &status );
}

//get must fail if port is above acceptable region
test(get_big_port_inva_fail)
    mcapi_endpoint_t receiver;

    mcapi_initialize( 1, 2, 0, 0, &info, &status );

    receiver = mcapi_endpoint_get( 1, 2, MCAPI_MAX_ENDPOINTS, 1000, &status );
    uassert( status == MCAPI_ERR_PORT_INVALID );
    uassert( receiver == MCAPI_NULL );
    mcapi_finalize( &status );
}

//get must fail if node is above acceptable region
test(get_big_node_inva_fail)
    mcapi_endpoint_t receiver;

    mcapi_initialize( 1, 2, 0, 0, &info, &status );

    receiver = mcapi_endpoint_get( 1, MCA_MAX_NODES, 1, 1000, &status );
    uassert( status == MCAPI_ERR_NODE_INVALID );
    uassert( receiver == MCAPI_NULL );
    mcapi_finalize( &status );
}

//get must fail if domain is above acceptable region
test(get_big_domain_inva_fail)
    mcapi_endpoint_t receiver;

    mcapi_initialize( 1, 2, 0, 0, &info, &status );

    receiver = mcapi_endpoint_get( MCA_MAX_DOMAINS, 2, 1, 1000, &status );
    uassert( status == MCAPI_ERR_DOMAIN_INVALID );
    uassert( receiver == MCAPI_NULL );
    mcapi_finalize( &status );
}

//get must fail if the timeout passes
test(get_timeout)
    mcapi_endpoint_t receiver;

    mcapi_initialize( 1, 2, 0, 0, &info, &status );

    receiver = mcapi_endpoint_get( 1, 2, 0, 500, &status );
    uassert( status == MCAPI_TIMEOUT );
    uassert( receiver == MCAPI_NULL );
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
    dotest(create_inva_lil_port_fail)
    dotest(create_inva_big_port_fail)
    dotest(create_no_port_fail)
    dotest(get_end)
    dotest(get_init_fail)
    dotest(get_lil_port_inva_fail)
    dotest(get_lil_node_inva_fail)
    dotest(get_lil_domain_inva_fail)
    dotest(get_big_port_inva_fail)
    dotest(get_big_node_inva_fail)
    dotest(get_big_domain_inva_fail)
    dotest(get_timeout)
}
