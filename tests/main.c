#include <stdio.h>

#include "../src/sonde-anal.c"

/*---------------------------------------------------------------------------------------------------------------------------
 *                                               Test Assertion Tools
 *-------------------------------------------------------------------------------------------------------------------------*/

/* Ensure assertions of all types are working for tests. */
#ifdef NDEBUG
#undef NDEBUG
#endif

/* Crash immediately, useful with a debugger! */
#ifndef HARD_EXIT
  #define HARD_EXIT (*(int volatile*)0) 
#endif

#ifndef PanicIf
  #define PanicIf(assertion) StopIf((assertion), HARD_EXIT)
#endif

#ifndef Panic
  #define Panic() HARD_EXIT
#endif

#ifndef StopIf
  #define StopIf(assertion, error_action) if (assertion) { error_action; }
#endif

#ifndef Assert
  #ifndef NDEBUG
    #define Assert(assertion) if(!(assertion)) { HARD_EXIT; }
  #else
    #define Assert(assertion) (void)(assertion)
  #endif
#endif

void 
AssertApproxEq(f64 left, f64 right, f64 abs_eps, f64 prop_eps)
{
    f64 abs_diff = fabs(left - right);
    f64 prop_diff = fabs((left - right) / (fabs(left) > fabs(right) ? left : right));
    if(abs_diff > abs_eps || prop_diff > prop_eps) { HARD_EXIT; }
}

/*---------------------------------------------------------------------------------------------------------------------------
 *                                                   Test Modules
 *-------------------------------------------------------------------------------------------------------------------------*/
#include "unit_conversion_tests.c"

/*---------------------------------------------------------------------------------------------------------------------------
 *                                                    Test Runner 
 *-------------------------------------------------------------------------------------------------------------------------*/
int
main(int argc, char *argv[])
{
    printf("\n\t\tBeginning sonde-anal tests.\n\n");

    all_unit_conversion_tests();

    printf("\n\t\tFinished sonde-anal tests.\n\n");
}

