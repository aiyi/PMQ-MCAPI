#include "utester.h"

#ifdef UTEST
#include <stdio.h>
#include "suite_node.h"
#include "suite_endpoint.h"
#include "suite_msg.h"
#include "suite_packet_con_open.h"
#include "suite_packet_tx_rx.h"
#include "suite_scalar_con_open.h"
#include "suite_scalar_tx_rx.h"

static int uTestCount = 0;
static int errorCount = 0;

int main()
{
    mcapi_status_t status;

    printf("Running the unit tests.\n" );

    //each suite must be called from here
    suite_node();
    suite_endpoint();
    suite_msg();
    suite_packet_con_open();
    suite_packet_tx_rx();
    suite_scalar_con_open();
    suite_scalar_tx_rx();

    //unitializing the thing. just in case.
    mcapi_finalize( &status );

    printf("Ran %d unit tests with %d errors found.\n", uTestCount, errorCount );

    return EXIT_SUCCESS;
}

void addUTest()
{
    ++uTestCount;
}

void addError()
{
    ++errorCount;
}

void print_test(char* name)
{
    if ( PRINT_TEST_NAMES == 1 )
        printf( "Running test %s\n", name );
}
#endif
