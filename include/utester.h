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
    char names[] = #NAME;\
    print_test( names ); \
    addUTest(); \
    clean();

//macro for running said test
#define dotest(NAME)\
    utest##NAME();

//macro for asserting via expression
#define uassert( ASSERTION ) \
    if ( !(ASSERTION) ) {\
        printf( "UNIT TEST FAILURE: %s: %s\n", names, #ASSERTION ); \
        addError(); }

//macro for asserting via comparing two given values,
//excpt is expected and printed as its variable name
//got is is the result and printed variable value
#define uassert2( excpt, got ) \
    if ( excpt != got ) {\
        printf( "UNIT TEST FAILURE: %s: expected: %s got %d\n", names, #excpt, got ); \
        addError(); }

//macro for asserting MCAPI-error codes: if value does not yield what
//was expected, the error code is printed
#define sassert( excpt, got ) \
    if ( excpt != got ) { \
        char status_msg[MCAPI_MAX_STATUS_MSG_LEN]; \
        mcapi_display_status( got, status_msg, MCAPI_MAX_STATUS_MSG_LEN ); \
        printf( "UNIT TEST FAILURE AT %u: %s got: %s expected: %s\n", __LINE__,\
        names, status_msg, #excpt ); \
        addError(); }

#endif
#endif
