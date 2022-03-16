#ifndef DBSI_ASSERT_H
#define DBSI_ASSERT_H


#include <cassert>


/*
* Dividing assertions into the following groups helps in two
* ways, (i) to document these conditions in the code, and (ii)
* to allow you to enable/disable assertions for different types
* of error independently.
*/


// #define DBSI_DISABLE_ALL_CHECKS


#ifdef DBSI_DISABLE_ALL_CHECKS
#define DBSI_DISABLE_CHECK_PRECOND
#define DBSI_DISABLE_CHECK_POSTCOND
#define DBSI_DISABLE_CHECK_INVARIANT
#endif


#ifndef DBSI_DISABLE_CHECK_PRECOND
#define DBSI_CHECK_PRECOND(expr) assert(expr)
#else
#define DBSI_CHECK_PRECOND(expr) ()
#endif


#ifndef DBSI_DISABLE_CHECK_POSTCOND
#define DBSI_CHECK_POSTCOND(expr) assert(expr)
#else
#define DBSI_CHECK_POSTCOND(expr) ()
#endif


#ifndef DBSI_DISABLE_CHECK_INVARIANT
#define DBSI_CHECK_INVARIANT(expr) assert(expr)
#else
#define DBSI_CHECK_INVARIANT(expr) ()
#endif


#endif  // DBSI_ASSERT_H
