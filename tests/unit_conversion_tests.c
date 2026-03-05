/****************************************************************************************************************************
 *                                          Test Obvious Conversion Values
 ***************************************************************************************************************************/
#define ABS_EPS 1.0e-15
#define PROP_EPS 1.0e-16
void
unit_conversions_test_obvious_conversion_values(void)
{
    SondeFahrenheit freezing = sonde_celsius_to_fahrenheit((SondeCelsius){ .val = 0.0 });
    AssertApproxEq(freezing.val, 32.0, ABS_EPS, PROP_EPS);

    SondeCelsius absolute_zero = sonde_kelvin_to_celsius((SondeKelvin){ .val = 0.0 });
    AssertApproxEq(absolute_zero.val, -273.15, ABS_EPS, PROP_EPS);
}
#undef ABS_EPS
#undef PROP_EPS

/****************************************************************************************************************************
 *                                                 Test Round Trips
 ***************************************************************************************************************************/
#define ABS_EPS 2.5e-10
#define PROP_EPS 5.75e-13
void
unit_conversions_test_round_trips(void)
{
    size const N_ITER = 10000;

    f64 const MINIMUM_C = -250.0;
    f64 const MAXIMUM_C = +250.0;
    f64 const RANGE_C = MAXIMUM_C - MINIMUM_C;
    for(size i = 0; i <= N_ITER; ++i)
    {
        SondeCelsius original = { .val = MINIMUM_C + (f64)i / (f64)N_ITER * RANGE_C };
        SondeKelvin kelvin_conversion = sonde_celsius_to_kelvin(original);
        SondeFahrenheit fahrenheit_conversion = sonde_celsius_to_fahrenheit(original);

        SondeCelsius back_from_kelvin = sonde_kelvin_to_celsius(kelvin_conversion);
        SondeCelsius back_from_fahrenheit = sonde_fahrenheit_to_celsius(fahrenheit_conversion);

        AssertApproxEq(original.val, back_from_kelvin.val, ABS_EPS, PROP_EPS);
        AssertApproxEq(original.val, back_from_fahrenheit.val, ABS_EPS, PROP_EPS);
    }

    f64 const MINIMUM_M = 0.0;
    f64 const MAXIMUM_M = 1000000.0;
    f64 const RANGE_M = MAXIMUM_M - MINIMUM_M;
    for(size i = 0; i <= N_ITER; ++i)
    {
        SondeMeter m_original = { .val = MINIMUM_M + (f64)i / (f64)N_ITER * RANGE_M };
        SondeKilometer km_m_conversion = sonde_meters_to_kilometers(m_original);
        SondeStatuteMile sm_m_conversion = sonde_meters_to_miles(m_original);
        SondeFeet ft_m_conversion = sonde_meters_to_feet(m_original);

        SondeMeter back_from_km = sonde_kilometers_to_meters(km_m_conversion);
        SondeMeter back_from_sm = sonde_miles_to_meters(sm_m_conversion);
        SondeMeter back_from_ft = sonde_feet_to_meters(ft_m_conversion);

        AssertApproxEq(m_original.val, back_from_km.val, ABS_EPS, PROP_EPS);
        AssertApproxEq(m_original.val, back_from_sm.val, ABS_EPS, PROP_EPS);
        AssertApproxEq(m_original.val, back_from_ft.val, ABS_EPS, PROP_EPS);

        SondeKilometer km_original = { .val = MINIMUM_M + (f64)i / (f64)N_ITER * RANGE_M };
        SondeStatuteMile sm_km_conversion = sonde_kilometers_to_miles(km_original);
        SondeFeet ft_km_conversion = sonde_kilometers_to_feet(km_original);

        SondeKilometer km_back_from_sm = sonde_miles_to_kilometers(sm_km_conversion);
        SondeKilometer km_back_from_ft = sonde_feet_to_kilometers(ft_km_conversion);

        AssertApproxEq(km_original.val, km_back_from_sm.val, ABS_EPS, PROP_EPS);
        AssertApproxEq(km_original.val, km_back_from_ft.val, ABS_EPS, PROP_EPS);

        SondeStatuteMile sm_original = { .val = MINIMUM_M + (f64)i / (f64)N_ITER * RANGE_M };
        SondeFeet ft_sm_conversion = sonde_miles_to_feet(sm_original);

        SondeStatuteMile sm_back_from_ft = sonde_feet_to_miles(ft_sm_conversion);

        AssertApproxEq(sm_original.val, sm_back_from_ft.val, ABS_EPS, PROP_EPS);
    }

}
#undef ABS_EPS
#undef PROP_EPS

/****************************************************************************************************************************
 *                                            All Unit Converstion Tests
 ***************************************************************************************************************************/
void
all_unit_conversion_tests(void)
{
    printf("\t\t\tUnit conversion tests...");

    unit_conversions_test_obvious_conversion_values();
    unit_conversions_test_round_trips();

    printf("completed.\n");
}

