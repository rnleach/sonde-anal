/* Ensure assertions of all types are working for tests. */
#ifdef NDEBUG
#undef NDEBUG
#endif

#include <stdio.h>

#include "../src/sonde-anal.c"

/*---------------------------------------------------------------------------------------------------------------------------
 *                                               Test Assertion Tools
 *-------------------------------------------------------------------------------------------------------------------------*/

void 
AssertApproxEq(f64 left, f64 right, f64 abs_eps, f64 prop_eps)
{
    if(isnan(left) || isnan(right)) { HARD_EXIT; }

    f64 abs_diff = fabs(left - right);
    if(abs_diff > abs_eps) { HARD_EXIT; }

    if(abs_diff > 1.0e-12)
    {
        f64 prop_diff = fabs((left - right) / (fabs(left) > fabs(right) ? left : right));
        if(prop_diff > prop_eps) { HARD_EXIT; }
    }
}

/*---------------------------------------------------------------------------------------------------------------------------
 *                                                   Test Modules
 *-------------------------------------------------------------------------------------------------------------------------*/
#include "unit_conversion_tests.c"
#include "thermodynamic_functions_test.c"
#include "bufkit_test.c"

/*---------------------------------------------------------------------------------------------------------------------------
 *                                                    Test Runner 
 *-------------------------------------------------------------------------------------------------------------------------*/
int
main(int argc, char *argv[])
{
    printf("\n\t\tBeginning sonde-anal tests.\n\n");

    all_unit_conversion_tests();
    all_bufkit_tests();
    all_thermodynamic_functions_tests();

    printf("\n\t\tFinished sonde-anal tests.\n\n");
}

