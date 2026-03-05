/****************************************************************************************************************************
 *                                                 Test Round Trips
 ***************************************************************************************************************************/
void
thermodynamic_functions_potential_temperature_round_trip_test(void)
{
    f64 const ABS_EPS = 1.0e-12;
    f64 const PROP_EPS = 1.0e-16;

    SondeCelsius const min_t = { .val = -100.0 };
    SondeCelsius const max_t = { .val = 70.0 };

    SondeHectopascal const max_p = { .val = 1100.0 };
    SondeHectopascal const min_p = { .val = 0.2    };

    for(SondeCelsius t = min_t; t.val <= max_t.val; t.val += 0.5)
    {
        for(SondeHectopascal p = max_p; p.val >= min_p.val; p.val -= 1.0)
        {
            SondeKelvin theta = sonde_potential_temperature(p, t);
            SondeCelsius t_back = sonde_temperature_from_pot_temperature(theta, p);

            AssertApproxEq(t.val, t_back.val, ABS_EPS, PROP_EPS);
        }
    }
}

void
thermodynamic_functions_vapor_pressure_dp_round_trip_test(void)
{
    f64 const ABS_EPS = 1.0e-13;
    f64 const PROP_EPS = 1.0e-16;

    for(f64 dp = SONDE_MIN_T_VP_LIQUID_C; dp < SONDE_MAX_T_VP_LIQUID_C; dp += 0.5)
    {
        SondeHectopascal original_vp = sonde_vapor_pressure_water((SondeCelsius){ .val = dp });
        f64 dp_back = sonde_dew_point_from_vapor_pressure(original_vp).val;

        AssertApproxEq(dp, dp_back, ABS_EPS, PROP_EPS);
    }

    for(f64 fp = SONDE_MIN_T_VP_ICE_C; fp < SONDE_MAX_T_VP_ICE_C; fp += 0.5)
    {
        SondeHectopascal original_vp = sonde_vapor_pressure_ice((SondeCelsius){ .val = fp });
        f64 fp_back = sonde_frost_point_from_vapor_pressure_over_ice(original_vp).val;

        AssertApproxEq(fp, fp_back, ABS_EPS, PROP_EPS);
    }
}

void
thermodynamic_functions_relative_humidity_round_trip_test(void)
{
    f64 const ABS_EPS = 1.0e-13;
    f64 const PROP_EPS = 1.0e-9;

    SondeCelsius const min_t = { .val = SONDE_MIN_T_VP_LIQUID_C };
    SondeCelsius const max_t = { .val = SONDE_MAX_T_VP_LIQUID_C };

    for(SondeCelsius t = min_t; t.val < max_t.val; t.val += 0.25)
    {
        for(SondeCelsius dp = min_t; dp.val <= t.val; dp.val += 0.25)
        {
            f64 rh = sonde_relative_humidity_liquid(t, dp);
            SondeCelsius dp_back = sonde_dew_point_from_temperature_and_humidity_liquid(t, rh);

            AssertApproxEq(dp.val, dp_back.val, ABS_EPS, PROP_EPS);
        }
    }
}

void
thermodynamic_functions_mixing_ratio_dew_point_round_trip_test(void)
{
    f64 const ABS_EPS = 1.0e-13;
    f64 const PROP_EPS = 1.0e-30;

    SondeCelsius const min_t = { .val = SONDE_MIN_T_VP_LIQUID_C };
    SondeCelsius const max_t = { .val = SONDE_MAX_T_VP_LIQUID_C };

    SondeHectopascal const max_p = { .val = 1100.0 };
    SondeHectopascal const min_p = { .val = 0.2    };

    for(SondeCelsius dp = min_t; dp.val < max_t.val; dp.val += 0.5)
    {
        for(SondeHectopascal p = max_p; p.val >= min_p.val; p.val -= 1.0)
        {
            f64 mixing_ratio = sonde_mixing_ratio(dp, p);

            if(sonde_error_extract(mixing_ratio) == SONDE_ERROR_NOT_AN_ERROR)
            {
                SondeCelsius dp_back = sonde_dew_point_from_p_and_mw(p, mixing_ratio);
                AssertApproxEq(dp.val, dp_back.val, ABS_EPS, PROP_EPS);
            }
            else
            {
                Assert(sonde_error_extract(mixing_ratio) == SONDE_ERROR_INVALID_INPUT);
            }
        }
    }
}

void
thermodynamic_functions_specific_humidity_dew_point_round_trip_test(void)
{
    f64 const ABS_EPS = 1.0e-13;
    f64 const PROP_EPS = 1.0e-30;

    SondeCelsius const min_t = { .val = SONDE_MIN_T_VP_LIQUID_C };
    SondeCelsius const max_t = { .val = SONDE_MAX_T_VP_LIQUID_C };

    SondeHectopascal const max_p = { .val = 1100.0 };
    SondeHectopascal const min_p = { .val = 0.2    };

    for(SondeCelsius dp = min_t; dp.val < max_t.val; dp.val += 0.5)
    {
        for(SondeHectopascal p = max_p; p.val >= min_p.val; p.val -= 1.0)
        {
            f64 sh = sonde_specific_humidity(dp, p);

            if(sonde_error_extract(sh) == SONDE_ERROR_NOT_AN_ERROR)
            {
                SondeCelsius dp_back = sonde_dew_point_from_p_and_spcecific_humidity(p, sh);
                AssertApproxEq(dp.val, dp_back.val, ABS_EPS, PROP_EPS);
            }
            else
            {
                Assert(sonde_error_extract(sh) == SONDE_ERROR_INVALID_INPUT);
            }
        }
    }
}

void
thermodynamic_functions_specific_humidity_mixing_ratio_round_trip_test(void)
{
    f64 const ABS_EPS = 1.0e-13;
    f64 const PROP_EPS = 1.0e-30;

    for(f64 sh = 0.0; sh <= 1.0; sh += 0.0005)
    {
        f64 mw = sonde_mixing_ratio_from_specific_humidity(sh);
        f64 sh_back = sonde_specific_humidity_from_mixing_ratio(mw);

        AssertApproxEq(sh, sh_back, ABS_EPS, PROP_EPS);
    }
}

void
thermodynamic_functions_equivalent_potential_temperature_at_lcl_round_trip_test(void)
{
    f64 const ABS_EPS = 1.0e-13;
    f64 const PROP_EPS = 1.0e-30;

    SondeCelsius const min_t = { .val = SONDE_MIN_T_VP_LIQUID_C };
    SondeCelsius const max_t = { .val = SONDE_MAX_T_VP_LIQUID_C };
    f64 const delta_t = 0.5;

    SondeHectopascal const max_p = { .val = 1100.0 };
    SondeHectopascal const min_p = { .val = 0.2    };
    f64 const delta_p = -1.0;

    for(SondeCelsius t = (SondeCelsius){ .val = min_t.val + delta_t }; t.val < max_t.val; t.val += delta_t)
    {
        for(SondeHectopascal p = max_p; p.val >= min_p.val; p.val += delta_p)
        {
            SondeKelvin theta_e = sonde_equivalent_potential_temperature(t, t, p);
            SondeCelsius t_back = sonde_temperature_from_equiv_pot_temp_saturated_pressure(p, theta_e);

            SondeError theta_e_error = sonde_error_extract(theta_e.val);
            SondeError t_back_error = sonde_error_extract(t_back.val);

            if(theta_e_error == SONDE_ERROR_NOT_AN_ERROR && t_back_error == SONDE_ERROR_NOT_AN_ERROR)
            {
                AssertApproxEq(t.val, t_back.val, ABS_EPS, PROP_EPS);
            }
            else
            {
                Assert(theta_e_error == SONDE_ERROR_INVALID_INPUT
                        || t_back_error == SONDE_ERROR_INVALID_INPUT 
                        || t_back_error == SONDE_ERROR_OUT_OF_RANGE);
            }
        }
    }
}

void
thermodynamic_functions_round_trips(void)
{
    thermodynamic_functions_potential_temperature_round_trip_test();
    thermodynamic_functions_vapor_pressure_dp_round_trip_test();
    thermodynamic_functions_relative_humidity_round_trip_test();
    thermodynamic_functions_mixing_ratio_dew_point_round_trip_test();
    thermodynamic_functions_specific_humidity_dew_point_round_trip_test();
    thermodynamic_functions_specific_humidity_mixing_ratio_round_trip_test();
    thermodynamic_functions_equivalent_potential_temperature_at_lcl_round_trip_test();
}

void
thermodynamic_functions_wet_bulb_and_virtual_temperature_test(void)
{
    SondeCelsius const min_t = { .val = SONDE_MIN_T_VP_LIQUID_C };
    SondeCelsius const max_t = { .val = SONDE_MAX_T_VP_LIQUID_C };
    f64 const delta_t = 1.0;

    SondeHectopascal const max_p = { .val = 1100.0 };
    SondeHectopascal const min_p = { .val = 0.2    };
    f64 const delta_p = -2.0;

    for(SondeCelsius t = (SondeCelsius){ .val = min_t.val + delta_t }; t.val < max_t.val; t.val += delta_t)
    {
        for(SondeCelsius dp = (SondeCelsius){ .val = min_t.val + delta_t }; dp.val <= t.val; dp.val += delta_t)
        {
            for(SondeHectopascal p = max_p; p.val >= min_p.val; p.val += delta_p)
            {
                SondeCelsius wb = sonde_wet_bulb(t, dp, p);
                if(sonde_error_extract(wb.val) == SONDE_ERROR_NOT_AN_ERROR)
                {
                    Assert(dp.val <= wb.val && wb.val <= t.val);
                }

                SondeCelsius vt = sonde_virtual_temperature(t, dp, p);
                if(sonde_error_extract(vt.val) == SONDE_ERROR_NOT_AN_ERROR)
                {
                    Assert(vt.val >= t.val);
                }
            }
        }
    }
}

/****************************************************************************************************************************
 *                                        All Thermodynamic Functions Tests
 ***************************************************************************************************************************/
void
all_thermodynamic_functions_tests(void)
{
    printf("\t\t\tThermodynamic functions tests...");

    thermodynamic_functions_round_trips();
    thermodynamic_functions_wet_bulb_and_virtual_temperature_test();

    printf("completed.\n");
}

