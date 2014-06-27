//This module is needed to execute unit tests. Otherwise it is not needed
//And should not be linked. Functions only, if UTEST is defined.
#ifndef UTESTER_H
#define UTESTER_H

#ifdef UTEST

//used by macros to increase count of ran unit tests by one
void addUTest();
//used by macros to increase count of errors by one
void addError();
//used by macros to print test name when ran, if defined so 
void print_test(char* name);
//the cleaning function found in cleaner module. called at start of test
void clean();

//1 if print name of the unit test when ran, else not
#define PRINT_TEST_NAMES 1

//macro used to define test
#define test(NAME) \
void utest##NAME() \
{ \
    char names[] = #NAME; \
    print_test( names ); \
    addUTest(); \
    clean();

//macro for running said test
#define dotest(NAME)\
    utest##NAME();

//macro for asserting via expression
#define uassert( ASSERTION ) \
    if ( !(ASSERTION) ) {\
        printf( "UNIT TEST FAILURE AT %u: %s: %s\n", __LINE__, names, \
        #ASSERTION ); \
        addError(); }

//macro for asserting via comparing two given values,
//excpt is expected and printed as its variable name
//got is is the result and printed variable value
#define uassert2( excpt, got ) \
    if ( excpt != got ) {\
        printf( "UNIT TEST FAILURE AT %u: %s: expected: %s got %d\n", \
        __LINE__, names, #excpt, got ); \
        addError(); }

//macro for asserting MCAPI-error codes: if value does not yield what
//was expected, the error code is printed
#define sassert( excpt, got ) \
    if ( excpt != got ) { \
        char status_msg[MCAPI_MAX_STATUS_MSG_LEN]; \
        mcapi_display_status( got, status_msg, MCAPI_MAX_STATUS_MSG_LEN ); \
        printf( "UNIT TEST FAILURE AT %u: %s got: %s expected: %s\n", \
        __LINE__, names, status_msg, #excpt ); \
        addError(); }

//These endpoint defines are to be used in tests. Put here to avoid overwrite.
#define MSG_PAUL {1, 2, 0}
#define MSG_PAT {1, 2, 1}

#define FOO {0, 5, 1}
#define BAR {0, 6, 2}
#define SEND {0, 8, 4}
#define RECV {0, 8, 6}

#define SSCL {0, 13, 2}
#define RSCL {0, 14, 3}
#define SSCL8 {0, 13, 4}
#define RSCL8 {0, 14, 5}
#define SSCL16 {0, 13, 6}
#define RSCL16 {0, 14, 7}
#define SSCL32 {0, 13, 8}
#define RSCL32 {0, 14, 9}

#define MSG_PAUL_DEF { MSG_PAUL, MCAPI_NO_CHAN, CHAN_NO_DIR, MSG_PAT, 0 }
#define MSG_PAT_DEF { MSG_PAT, MCAPI_NO_CHAN, CHAN_NO_DIR, MSG_PAUL, 0 }

#define FOO_DEF { FOO, MCAPI_PKT_CHAN, CHAN_DIR_SEND, BAR, 0 }
#define BAR_DEF { BAR, MCAPI_PKT_CHAN, CHAN_DIR_RECV, FOO, 0 }
#define SEND_DEF { SEND, MCAPI_PKT_CHAN, CHAN_DIR_SEND, RECV, 0 }
#define RECV_DEF { RECV, MCAPI_PKT_CHAN, CHAN_DIR_RECV, SEND, 0 }

#define SSCL_DEF { SSCL, MCAPI_SCL_CHAN, CHAN_DIR_SEND, RSCL, 8 }
#define RSCL_DEF { RSCL, MCAPI_SCL_CHAN, CHAN_DIR_RECV, SSCL, 8 }
#define SSCL_DEF8 { SSCL8, MCAPI_SCL_CHAN, CHAN_DIR_SEND, RSCL8, 1 }
#define RSCL_DEF8 { RSCL8, MCAPI_SCL_CHAN, CHAN_DIR_RECV, SSCL8, 1 }
#define SSCL_DEF16 { SSCL16, MCAPI_SCL_CHAN, CHAN_DIR_SEND, RSCL16, 2 }
#define RSCL_DEF16 { RSCL16, MCAPI_SCL_CHAN, CHAN_DIR_RECV, SSCL16, 2 }
#define SSCL_DEF32 { SSCL32, MCAPI_SCL_CHAN, CHAN_DIR_SEND, RSCL32, 4 }
#define RSCL_DEF32 { RSCL32, MCAPI_SCL_CHAN, CHAN_DIR_RECV, SSCL32, 4 }

#define DEF_LIST { MSG_PAUL_DEF, MSG_PAT_DEF, FOO_DEF, BAR_DEF, SEND_DEF, \
RECV_DEF, SSCL_DEF, RSCL_DEF, SSCL_DEF8, RSCL_DEF8, SSCL_DEF16, \
RSCL_DEF16, SSCL_DEF32, RSCL_DEF32 }

#endif
#endif
