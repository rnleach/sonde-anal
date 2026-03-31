#include <stddef.h>
#include <stdint.h>
#include <math.h>

#include "elk.h"
#include "magpie.h"
#include "coyote.h"
#include "packrat.h"

/***************************************************************************************************************************
 *                                             Error Reporting and Handling
 **************************************************************************************************************************/
typedef enum
{
    SONDE_ERROR_NONE = 0,              /* This is just a NaN with no embedded error information.                      */

    /* Data Errors */
    SONDE_ERROR_MISSING_DATA,          /* This represents missing data.                                               */
    SONDE_ERROR_INVALID_INPUT,         /* An input value was invalid, such as infinity or NaN, or scientific error.   */

    /* Algorithmic Errors */
    SONDE_ERROR_TOO_MANY_ITERATIONS,   /* An iterative algorithm took too many iterations.                            */
    SONDE_ERROR_OUT_OF_RANGE,          /* An input or output was out of the range of allowed values for an algorithm. */
    SONDE_ERROR_NOT_ENOUGH_DATA,       /* Not enough data for the algorithm to work.                                  */

    /* Misuse of the error system. */
    SONDE_ERROR_NOT_AN_ERROR,          /* This isn't a NaN.                                                           */

    SONDE_ERROR_NUMBER_OF_ERRORS       /* Not actually used for any errors, but potentially for compile time checks.  */
} SondeError;

f64 sonde_error_create_nan(SondeError err);
SondeError sonde_error_extract(f64 val);
b32 sonde_is_error(f64 val);
char const *sonde_error_msg_from_code(SondeError err);
char const *sonde_error_msg_from_value(f64 val);

#define SondeErrorWrap(type, code) (type){ .val = sonde_error_create_nan(code) }

/***************************************************************************************************************************
 *                                               Units of Measurement
 **************************************************************************************************************************/
/* Temperatures */
typedef struct { f64 val; } SondeCelsius;
typedef struct { f64 val; } SondeFahrenheit;
typedef struct { f64 val; } SondeKelvin;

/* Pressure */
typedef struct { f64 val; } SondeHectopascal;
typedef SondeHectopascal SondeMillibar;

/* Specific Heat Capacity */
typedef struct { f64 val; } SondeJpKgpK;                /* Joules / (kilogram * Kelvin)      */

/* Distance, Length */
typedef struct { f64 val; } SondeKilometer;
typedef struct { f64 val; } SondeMeter;
typedef struct { f64 val; } SondeCentimeters;
typedef struct { f64 val; } SondeMillimeters;
typedef struct { f64 val; } SondeStatuteMile;
typedef struct { f64 val; } SondeFeet;
typedef struct { f64 val; } SondeInches;

/* Speed, Veclocity*/
typedef struct { f64 val; } SondeMps;                   /* Meters per second                 */
typedef struct { f64 val; } SondeKts;                   /* Knots                             */
typedef struct { f64 val; } SondeMph;                   /* Miles per hour                    */
typedef struct { f64 val; } SondeHectopascalPerSecond;  /* Pressure vertical velocity        */
typedef struct { f64 u; f64 v; } SondeUVMps;            /* U and V wind in meters per second */
typedef struct { f64 spd; f64 dir; } SondeSpdDirKts;    /* Speed and direction in knots      */

/* Power and specific energy */
typedef struct { f64 val; } SondeGw;                    /* Gigawatts                         */
typedef struct { f64 val; } SondeJpKg;                  /* Joules per kilogram               */

/***************************************************************************************************************************
 *                                                      Unit Conversions
 **************************************************************************************************************************/

/* Temperature Conversions */
SondeKelvin     sonde_celsius_to_kelvin(SondeCelsius t)        { return (SondeKelvin)     { .val = t.val + 273.15 };       }
SondeCelsius    sonde_kelvin_to_celsius(SondeKelvin t)         { return (SondeCelsius)    { .val = t.val - 273.15 };       }
SondeCelsius    sonde_fahrenheit_to_celsius(SondeFahrenheit t) { return (SondeCelsius)    { .val = (t.val - 32.0) / 1.8};  }
SondeFahrenheit sonde_celsius_to_fahrenheit(SondeCelsius t)    { return (SondeFahrenheit) { .val =  1.8 * t.val + 32.0};   }

/* Distance Conversions */
SondeKilometer   sonde_miles_to_kilometers(SondeStatuteMile m) { return (SondeKilometer)   { .val = m.val / 1.609344 };    }
SondeKilometer   sonde_meters_to_kilometers(SondeMeter m)      { return (SondeKilometer)   { .val = m.val * 0.001 };       }
SondeKilometer   sonde_feet_to_kilometers(SondeFeet f)         { return (SondeKilometer)   { .val = f.val * 0.0003048 };   }

SondeMeter       sonde_miles_to_meters(SondeStatuteMile m)     { return (SondeMeter)       { .val = m.val / 1609.344 };    }
SondeMeter       sonde_kilometers_to_meters(SondeKilometer km) { return (SondeMeter)       { .val = km.val * 1000.0 };     }
SondeMeter       sonde_feet_to_meters(SondeFeet f)             { return (SondeMeter)       { .val = f.val * 0.3048 };      }

SondeCentimeters sonde_inches_to_centimeters(SondeInches in)   { return (SondeCentimeters) { .val = in.val * 2.54 };       }
SondeCentimeters sonde_millimeters_to_centimeters(SondeMillimeters mm){ return (SondeCentimeters) { .val = mm.val / 10.0}; }

SondeMillimeters sonde_inches_to_millimeters(SondeInches in)   { return (SondeMillimeters) { .val = in.val * 25.4 };       }
SondeMillimeters sonde_centimeters_to_millimeters(SondeCentimeters cm){ return (SondeMillimeters) { .val = cm.val * 10.0}; }

SondeStatuteMile sonde_kilometers_to_miles(SondeKilometer km)  { return (SondeStatuteMile) { .val = km.val * 1.609344 };   }
SondeStatuteMile sonde_meters_to_miles(SondeMeter m)           { return (SondeStatuteMile) { .val = m.val * 1609.344 };    }
SondeStatuteMile sonde_feet_to_miles(SondeFeet f)              { return (SondeStatuteMile) { .val = f.val / 5280.0 };      }

SondeFeet        sonde_miles_to_feet(SondeStatuteMile m)       { return (SondeFeet)        { .val = m.val * 5280.0 };      }
SondeFeet        sonde_kilometers_to_feet(SondeKilometer km)   { return (SondeFeet)        { .val = km.val / 0.0003048 };  }
SondeFeet        sonde_meters_to_feet(SondeMeter m)            { return (SondeFeet)        { .val = m.val / 0.3048 };      }

SondeInches      sonde_centimeters_to_inches(SondeCentimeters cm) { return (SondeInches)   { .val = cm.val / 2.54 };       }
SondeInches      sonde_millimeters_to_inches(SondeMillimeters mm) { return (SondeInches)   { .val = mm.val / 25.4 };       }

/* Vector Conversions */
SondeSpdDirKts   sonde_uv_to_spd_dir(SondeUVMps uv);
SondeUVMps       sonde_spd_dir_to_uv(SondeSpdDirKts sd);

/***************************************************************************************************************************
 *                                Thermodynamic Constants Frequently Used in Meteorology
 **************************************************************************************************************************/

SondeJpKgpK const sonde_const_Rd  = { .val =  287.058 }; /* The gas constant for dry air. (J / (K kg))                    */
SondeJpKgpK const sonde_const_Rv  = { .val =  461.5   }; /* The gas constant for water vapor. (J / (K kg))                */
SondeJpKgpK const sonde_const_cpd = { .val = 1005.7   }; /* Heat capacity of dry air at constant pressure. (J / (K kg))   */
SondeJpKgpK const sonde_const_cpv = { .val = 1870.0   }; /* Heat capacity of water vapor at fixed pressure. (J / (K kg))  */
SondeJpKgpK const sonde_const_cl  = { .val = 4190.0   }; /* Heat capacity of liquid water. (J / (K kg))                   */
SondeJpKgpK const sonde_const_cvd = { .val =  718.0   }; /* Heat capacity of dry air at constant volume. (J / (K kg))     */

f64 const sonde_const_g   = -9.80665;                    /* Acceleration due to gravity at the Earth's surface. (m / s^2) */
f64 const sonde_const_epsilon = 0.6220108342361863;      /* Ratio of Rd and Rv. (no units)                                */
f64 const sonde_const_gamma = 1.4006963788300837;        /* Ratio of cp and cv. (unitless)                                */

/***************************************************************************************************************************
 *                                                  Thermodynamic Formulas
 **************************************************************************************************************************/

typedef struct
{
    SondeHectopascal p;         /* Pressure in hectopascals */
    SondeCelsius t;             /* Temperature in Celsius   */
} SondePressureTemperaturePair;

/* Potential Temperature */
SondeKelvin  sonde_potential_temperature(SondeHectopascal p, SondeCelsius t);
SondeCelsius sonde_temperature_from_pot_temperature(SondeKelvin theta, SondeHectopascal p);

/* Vapor Pressure of Water / Ice, Dew / Frost Point, Relative Humidity */
SondeHectopascal sonde_vapor_pressure_water(SondeCelsius dp);
SondeCelsius sonde_dew_point_from_vapor_pressure(SondeHectopascal vp);
SondeHectopascal sonde_vapor_pressure_ice(SondeCelsius fp);
SondeCelsius sonde_frost_point_from_vapor_pressure_over_ice(SondeHectopascal vp);
f64 sonde_relative_humidity_liquid(SondeCelsius t, SondeCelsius dp);
SondeCelsius sonde_dew_point_from_temperature_and_humidity_liquid(SondeCelsius t, f64 rh);
f64 sonde_relative_humidity_ice(SondeCelsius t, SondeCelsius fp);

/* Mixing Ratio and Specific Humidity */
f64 sonde_mixing_ratio(SondeCelsius dp, SondeHectopascal p);
SondeCelsius sonde_dew_point_from_p_and_mw(SondeHectopascal p, f64 mw);
f64 sonde_specific_humidity(SondeCelsius dp, SondeHectopascal p);
SondeCelsius sonde_dew_point_from_p_and_spcecific_humidity(SondeHectopascal p, f64 specific_humidity);
f64 sonde_mixing_ratio_from_specific_humidity(f64 specific_humidity);
f64 sonde_specific_humidity_from_mixing_ratio(f64 mixing_ratio);

/* Virtual Temperature */
SondeCelsius sonde_virtual_temperature(SondeCelsius t, SondeCelsius dp, SondeHectopascal p);

/* Latent Heat of Condensation / Evaporation. */
SondeJpKgpK sonde_latent_heat_of_condensation_vaporization(SondeCelsius t);

/* Equivalent Potential Temperature */
SondeKelvin sonde_equivalent_potential_temperature(SondeCelsius t, SondeCelsius dp, SondeHectopascal p);
SondeCelsius sonde_temperature_from_equiv_pot_temp_saturated_pressure(SondeHectopascal p, SondeKelvin theta_e);

/* Calculating the LCL */
SondeHectopascal sonde_pressure_at_lcl(SondeCelsius t, SondeCelsius dp, SondeHectopascal p);
SondePressureTemperaturePair sonde_pressure_and_temperature_at_lcl(SondeCelsius t, SondeCelsius dp, SondeHectopascal p);

/* Web Bulb Temperature */
SondeCelsius sonde_wet_bulb(SondeCelsius t, SondeCelsius dp, SondeHectopascal p);

/* Fire Weather */
SondeGw sonde_pft(
    SondeMeter zfc,
    SondeHectopascal pfc,
    SondeMps mean_wind,
    SondeKelvin theta_diff,
    SondeKelvin theta_fc,
    SondeHectopascal p_sfc);

/***************************************************************************************************************************
 *                                                  Sounding Data Structure
 **************************************************************************************************************************/
typedef struct
{
    i64 station_number;   /* 0 is default and meaningless, ie no station number associated with this location. */
    ElkStr id;            /* Some textual station identifier, eg an ICAO identifier. Default NULL.             */
    f64 latitude;         /* NaN values indicate a missing value.                                              */
    f64 longitude;        /* NaN values indicate a missing value.                                              */
    SondeMeter elevation; /* NaN values indicate a missing value.                                              */
} SondeStationInfo;

typedef enum
{
    SONDE_PC_PRESSURE            = (UINT32_C(1) <<  0),
    SONDE_PC_TEMPERATURE         = (UINT32_C(1) <<  1),
    SONDE_PC_DEW_POINT           = (UINT32_C(1) <<  2),
    SONDE_PC_WET_BULB            = (UINT32_C(1) <<  3),
    SONDE_PC_VIRTUAL_TEMPERATURE = (UINT32_C(1) <<  4),
    SONDE_PC_THETA               = (UINT32_C(1) <<  5), /* Potential temperature             */
    SONDE_PC_THETA_E             = (UINT32_C(1) <<  6), /* Equivalent potential temperature  */
    SONDE_PC_PVV                 = (UINT32_C(1) <<  7), /* Pressure vertical velocity        */
    SONDE_PC_GEOPOTENTIAL_HGT    = (UINT32_C(1) <<  8),
    SONDE_PC_CLOUD_FRACTION      = (UINT32_C(1) <<  9),
    // TODO: add RH
    // TODO: add RH ice
    // TODO: add Frost point

    /* These two are mutually exclusive. */
    SONDE_PC_WIND_SPEED_DIR      = (UINT32_C(1) << 10),
    SONDE_PC_WIND_UV             = (UINT32_C(1) << 11)
} SondeProfileCode;

b32 sonde_profile_code_present(u32 profiles, SondeProfileCode code);
u32 sonde_profile_code_set(u32 profiles, SondeProfileCode code);

typedef struct
{
    /* Sounding Metadata */
    SondeStationInfo stn_info;       /* Description of the sounding's location                                            */
    ElkStr source;                   /* Description of where this data came from                                          */
    ElkTime valid_time;              /* Valid time                                                                        */
    i32 lead_time;                   /* Model lead time in hours, if this is a forecast sounding                          */

    u32 profiles;                    /* Keep track of which profiles are present in this sounding, with SondeProfileCode  */

    /* Surface Data */
    SondeHectopascal mslp;           /* Mean sea level pressure                                                           */
    SondeHectopascal station_p;      /* Station pressure                                                                  */
    SondeCelsius surface_t;          /* Station temperature                                                               */
    SondeCelsius surface_dp;         /* Station dew point                                                                 */
    SondeMillimeters precip_1hr;     /* 1-hour precipitation                                                              */

    /* Profile Data */
    PakArrayLedger levels;           /* Ledger for the levels in the sounding                                             */
    SondeHectopascal *p;             /* Pressure                                                                          */
    SondeCelsius *t;                 /* Temperature                                                                       */
    SondeCelsius *dp;                /* Dew Point                                                                         */
    SondeCelsius *wb;                /* Wet Bulb Temperature                                                              */
    SondeCelsius *vt;                /* Virtual Temperature                                                               */
    SondeKelvin *theta;              /* Potential Temperature                                                             */
    SondeKelvin *theta_e;            /* Equivalent Potential Temperature                                                  */
    SondeHectopascalPerSecond *pvv;  /* Pressure Vertical Velocity                                                        */
    SondeMeter *hgt;                 /* Geopotential Height                                                               */
    f64 *cloud_fraction;             /* Cloud fraction                                                                    */
    // TODO: add RH
    // TODO: add RH ice
    // TODO: add Frost point
    union
    {
        SondeSpdDirKts *wind;        /* Wind speed and direction                                                          */
        SondeUVMps *uv_wind;         /* Wind U (west to east) and V (south to north) components                           */
    };
} SondeSounding;

SondeSounding *sonde_sounding_alloc_and_init(MagAllocator *alloc, size capacity);    /* Capacity is max number of levels. */

/* File loading */
typedef struct SondeSoundingList
{
    ElkTime valid_time;
    SondeSounding *snd;
    struct SondeSoundingList *next;
} SondeSoundingList;

SondeSoundingList *sonde_sounding_from_bufkit_str(MagAllocator *alloc, ElkStr txt, ElkStr source_description);
void sonde_sounding_fill_in_profiles(SondeSounding *snd, SondeProfileCode pcodes);            // TODO
void sonde_sounding_list_fill_in_profiles(SondeSoundingList *sndgs, SondeProfileCode pcodes); // TODO

/***************************************************************************************************************************
 *                                                      Implementations
 **************************************************************************************************************************/

_Static_assert(SONDE_ERROR_NUMBER_OF_ERRORS < INT32_MAX, "Too many error types.");
_Static_assert(sizeof(SondeSpdDirKts) == sizeof(SondeUVMps), "Mismatch in type sizes for array punning.");

typedef union { f64 x; u64 i; } SondeErrorPun;

f64 
sonde_error_create_nan(SondeError err)
{
    SondeErrorPun ep = { .x = NAN };
    ep.i |= (u64)(err & 0x7FFFFFFF);

    return ep.x;
}

SondeError 
sonde_error_extract(f64 val)
{
    if(!isnan(val)) { return SONDE_ERROR_NOT_AN_ERROR; }
    SondeErrorPun ep = { .x = val };
    return  (SondeError)(ep.i & 0x7FFFFFFF);
}

b32 
sonde_is_error(f64 val)
{
    return isnan(val);
}

char const *
sonde_error_msg_from_code(SondeError err)
{
    switch(err)
    {
        default:                              return "Unknown error code";
        case SONDE_ERROR_NONE:                return "No error information in NaN";
        case SONDE_ERROR_MISSING_DATA:        return "Missing data value";
        case SONDE_ERROR_INVALID_INPUT:       return "Invalid input value";
        case SONDE_ERROR_TOO_MANY_ITERATIONS: return "Too many iterations";
        case SONDE_ERROR_OUT_OF_RANGE:        return "Value out of range";
        case SONDE_ERROR_NOT_ENOUGH_DATA:     return "Not enough data";
        case SONDE_ERROR_NOT_AN_ERROR:        return "Value was not an error";
    }

    return NULL;
}

char const *
sonde_error_msg_from_value(f64 val)
{
    SondeError err = sonde_error_extract(val);
    return sonde_error_msg_from_code(err);
}

typedef f64(*SondeRootFunc)(f64 x, f64 *params);

SondeSpdDirKts
sonde_uv_to_spd_dir(SondeUVMps uv)
{
    f64 spd = sqrt(uv.u * uv.u + uv.v * uv.v) * 1.9438444924406048;

    f64 direction = 180.0 + 90.0 - atan2(uv.v, uv.u) * 180.0 /  ELK_PI;
    while(direction < 0.0) { direction += 360.0; }
    while(direction >= 360.0) { direction -= 360.0; }

    return (SondeSpdDirKts) { .spd = spd, .dir = direction };
}

SondeUVMps
sonde_spd_dir_to_uv(SondeSpdDirKts sd)
{
    f64 rads = sd.dir * ELK_PI / 180.0;
    f64 spd = sd.spd / 1.9438444924406048;

    f64 u = -spd * sin(rads);
    f64 v = -spd * cos(rads);

    return (SondeUVMps){ .u = u, .v = v };
}


/* Find the root of an equation given values bracketing a root. Used when finding wet bulb
 * temperature among other functions.
 *
 * `f` - The function that you need to find the root for.
 * `a` - The left bracket for the root.
 * `b` - The right bracket for the root.
 * `params` - constants or other values that `f` needs to be evaluated, but that WILL NOT change
 *            during the root finding process. 
 */
f64
sonde_find_root(SondeRootFunc f, f64 a, f64 b, f64 *params)
{
    i32 const MAX_IT = 50;
    f64 const EPS = 1.0e-9;

    if(isnan(a)) { return a; }
    if(isnan(b)) { return b; }

    f64 fa = f(a, params);
    f64 fb = f(b, params);

    if(isnan(fa)) { return fa; }
    if(isnan(fb)) { return fb; }

    /* Check to make sure a root is bracketed by a and b */
    if(fa * fb >= 0.0) { return sonde_error_create_nan(SONDE_ERROR_INVALID_INPUT); }

    /* Check if the left and right brackets are in the correct order and fix them if not. */
    if(fabs(fa) < fabs(fb))
    {
        f64 tmp =  a;  a =  b; b  = tmp;
            tmp = fa; fa = fb; fb = tmp;
    }

    f64 c = a;
    f64 fc = fa;
    f64 d;
    b32 mflag = true;

    for(i32 i = 0; i < MAX_IT; ++i)
    {
        f64 s;
        if( fa != fc && fb != fc)
        {
            /* Try inverse quadratic for next step */
            s = (a * fb * fc) / ((fa - fb) * (fa - fc)) + (b * fa * fc) / ((fb - fa) * (fb - fc)) + (c * fa * fb) / ((fc - fa) * (fc - fb));
        } 
        else
        {
            /* Try secant method for next step */
            s = b - fb * (b - a) / (fb - fa);
        };

        /* Check to see if bisection would be a better idea */
        b32 condition1;
        if(a < b) { condition1 = s > b || s < (3.0 * a + b) / 4.0; }
        else      { condition1 = s < b || s > (3.0 * a + b) / 4.0; }

        b32 condition2 = mflag && fabs(s - b) >= fabs(b - c) / 2.0;
        b32 condition3 = !mflag && fabs(s - b) >= fabs(c - d) / 2.0;
        b32 condition4 = mflag && fabs(b - c) < EPS;
        b32 condition5 = !mflag && fabs(c - d) < EPS;

        if(condition1 || condition2 || condition3 || condition4 || condition5)
        {
            s = (a + b) / 2.0;
            mflag = true;
        }
        else
        {
            mflag = false;
        }

        f64 fs = f(s, params); if(isnan(fs)) { return fs; }
        d = c;
        c = b;
        fc = fb;

        if(fa * fs < 0.0) { b = s; fb = fs; }
        else              { a = s; fa = fs; }

        if(fabs(fa) < fabs(fb))
        {
            f64 tmp =  a;  a =  b; b  = tmp;
                tmp = fa; fa = fb; fb = tmp;
        }

        /* Check for convergence and return */
        if(fb == 0.0 || fabs(b - a) < EPS) { return b; }
    }

    return sonde_error_create_nan(SONDE_ERROR_TOO_MANY_ITERATIONS);
}


SondeKelvin 
sonde_potential_temperature(SondeHectopascal p, SondeCelsius t)
{
    SondeKelvin temp_k = sonde_celsius_to_kelvin(t);
    return (SondeKelvin){ .val = temp_k.val * pow(1000.0 / p.val, sonde_const_Rd.val / sonde_const_cpd.val) };
}

SondeCelsius 
sonde_temperature_from_pot_temperature(SondeKelvin theta, SondeHectopascal p)
{
    SondeKelvin t_k = { .val = theta.val * pow(p.val / 1000.0, sonde_const_Rd.val / sonde_const_cpd.val) };
    return sonde_kelvin_to_celsius(t_k);
}

/* Constants used for the limits of applicability to the empirical relationships used for vapor pressure. */
f64 const SONDE_MIN_T_VP_LIQUID_C = -80.0;
f64 const SONDE_MAX_T_VP_LIQUID_C = 50.0;
f64 const SONDE_MIN_T_VP_ICE_C    = -80.0;
f64 const SONDE_MAX_T_VP_ICE_C    = 0.0;

/* Get the vapor pressure over liquid water.
 *
 * Alduchov, O.A., and Eskridge, R.E. Improved Magnus` form approximation of saturation vapor
 * pressure. United States: N. p., 1997. Web. doi:10.2172/548871.
 *
 * Returns: The vapor pressure of water vapor in hPa.
 */
SondeHectopascal 
sonde_vapor_pressure_water(SondeCelsius dp)
{
    if(dp.val < SONDE_MIN_T_VP_LIQUID_C || dp.val > SONDE_MAX_T_VP_LIQUID_C)
    {
        return SondeErrorWrap(SondeHectopascal, SONDE_ERROR_OUT_OF_RANGE); 
    }

    return (SondeHectopascal){ .val = 6.1037 * exp(17.641 * dp.val / (dp.val + 243.27)) };
}

/* Get the dew point given the vapor pressure of water over liquid water. This function is the
 * inverse of `sonde_vapor_pressure_water`.
 *
 * Returns: The dew point in Celsius.
 */
SondeCelsius 
sonde_dew_point_from_vapor_pressure(SondeHectopascal vp)
{
    f64 a = log(vp.val / 6.1037) / 17.641;
    f64 dp_c = a * 243.27 / (1.0 - a);

    if (dp_c < SONDE_MIN_T_VP_LIQUID_C || dp_c > SONDE_MAX_T_VP_LIQUID_C)
    {
        return SondeErrorWrap(SondeCelsius, SONDE_ERROR_OUT_OF_RANGE);
    }

    return (SondeCelsius){ .val = dp_c };
}

/* Get the vapor pressure over ice.
 *
 * Alduchov, O.A., and Eskridge, R.E. Improved Magnus` form approximation of saturation vapor
 * pressure. United States: N. p., 1997. Web. doi:10.2172/548871.
 *
 * Returns: The vapor pressure of water vapor over ice in hPa.
 */
SondeHectopascal 
sonde_vapor_pressure_ice(SondeCelsius fp)
{
    if(fp.val < SONDE_MIN_T_VP_ICE_C || fp.val > SONDE_MAX_T_VP_ICE_C)
    {
        return SondeErrorWrap(SondeHectopascal, SONDE_ERROR_OUT_OF_RANGE); 
    }
    
    return (SondeHectopascal){ .val = 6.1121 * exp(22.587 * fp.val / (fp.val + 273.86)) };
}

/* Get the frost point given the vapor pressure of water over ice. This function is the inverse of
 * `sonde_vapor_pressure_ice`.
 *
 * Returns: The frost point in Celsius.
 */
SondeCelsius 
sonde_frost_point_from_vapor_pressure_over_ice(SondeHectopascal vp)
{
    f64 a = log(vp.val / 6.1121) / 22.587;
    f64 fp_c = a * 273.86 / (1.0 - a);

    if (fp_c < SONDE_MIN_T_VP_ICE_C || fp_c > SONDE_MAX_T_VP_ICE_C)
    {
        return SondeErrorWrap(SondeCelsius, SONDE_ERROR_OUT_OF_RANGE);
    }

    return (SondeCelsius){ .val = fp_c };
}

/* Calculate the relative humidity with respect to liquid water.
 *
 * Returns: The relative humidity as a decimal, i.e. 0.95 instead of 95%.
 */
f64 
sonde_relative_humidity_liquid(SondeCelsius t, SondeCelsius dp)
{
    SondeHectopascal es = sonde_vapor_pressure_water(t);
    SondeHectopascal e = sonde_vapor_pressure_water(dp);
    return e.val / es.val;
}

/* Calculate the dew point with respect to liquid water.
 *
 * Assumes rh for liquid water and not ice. rh is also in decimal form and not a 
 * percent (i.e. 0.93 and not 93.0).
 *
 * Returns: The dew point in Celsius.
 */
SondeCelsius 
sonde_dew_point_from_temperature_and_humidity_liquid(SondeCelsius t, f64 rh)
{
    SondeHectopascal es = sonde_vapor_pressure_water(t);
    SondeHectopascal e = (SondeHectopascal){ .val = es.val * rh };
    return sonde_dew_point_from_vapor_pressure(e);
}

/* Calculate the relative humidity with respect to ice.
 *
 * Returns: The relative humidity as a decimal, i.e. 0.95 instead of 95%.
 */
f64 
sonde_relative_humidity_ice(SondeCelsius t, SondeCelsius fp)
{
    SondeHectopascal e = sonde_vapor_pressure_ice(t);
    SondeHectopascal es = sonde_vapor_pressure_ice(fp);
    return e.val / es.val;
}

/* Calculate the mixing ratio of water.
 *
 * Eq 5.9 from "Weather Analysis" by Dušan Dujrić
 *
 * Returns: The mixing ratio as a unitless value. Note this is often reported as g/kg, but this
 *          function returns kg/kg or g/g.
 */
f64 
sonde_mixing_ratio(SondeCelsius dp, SondeHectopascal p)
{
    SondeHectopascal vp = sonde_vapor_pressure_water(dp);
    if(vp.val > p.val) { return sonde_error_create_nan(SONDE_ERROR_INVALID_INPUT); }
    return sonde_const_epsilon * vp.val / (p.val - vp.val);
}

/* Given a mixing ratio and pressure, calculate the dew point temperature. If saturation is
 * assumed, this is also the temperature.
 *
 * Returns: The dew point in Celsius.
 */
SondeCelsius 
sonde_dew_point_from_p_and_mw(SondeHectopascal p, f64 mw)
{
    SondeHectopascal vp = { .val = mw * p.val / (mw + sonde_const_epsilon) };
    return sonde_dew_point_from_vapor_pressure(vp);
}

/* Calculate the specific humidity.
 *
 * Eqs 5.11 and 5.12 from from "Weather Analysis" by Dušan Dujrić 
 *
 * `dp` - the dew point, if this is the same as the temperature then this calculates the saturation specific humidity.
 * `p` - the pressure in hPa.
 *
 * Returns the specific humidity. (no units)
 */
f64 
sonde_specific_humidity(SondeCelsius dp, SondeHectopascal p)
{
    SondeHectopascal vp = sonde_vapor_pressure_water(dp);
    if(vp.val > p.val) { return sonde_error_create_nan(SONDE_ERROR_INVALID_INPUT); }
    return vp.val / p.val * sonde_const_epsilon;
}

/* Given a specific humidity and pressure, calculate the dew point temperature. If saturation is
 * assumed, this is also the temperature.
 */
SondeCelsius 
sonde_dew_point_from_p_and_spcecific_humidity(SondeHectopascal p, f64 specific_humidity)
{
    SondeHectopascal vp = { .val = specific_humidity * p.val / sonde_const_epsilon };
    return sonde_dew_point_from_vapor_pressure(vp);
}

/* Convert specific humidity into mixing ratio. */
f64
sonde_mixing_ratio_from_specific_humidity(f64 specific_humidity)
{
    return specific_humidity / (1.0 - specific_humidity);
}

/* Convert mixing ratio into specific humidity. */
f64
sonde_specific_humidity_from_mixing_ratio(f64 mixing_ratio)
{
    return mixing_ratio / (1.0 + mixing_ratio);
}

/* Virtual temperature in Celsius.
 *
 * From the [Glossary of Meteorology].(http://glossary.ametsoc.org/wiki/Virtual_temperature)
 *
 * Returns the virtual temperature in Celsius.
 */
SondeCelsius 
sonde_virtual_temperature(SondeCelsius t, SondeCelsius dp, SondeHectopascal p)
{
    f64 rv = sonde_mixing_ratio(dp, p);
    SondeKelvin t_k = sonde_potential_temperature(p, t);
    SondeKelvin vt = { .val = t_k.val * (1.0 + rv / sonde_const_epsilon) / (1.0 + rv) };
    return sonde_temperature_from_pot_temperature(vt, p);
}

/* Latent heat of condensation / vaporization for water.
 *
 * Polynomial curve fit to Table 2.1. R. R. Rogers; M. K. Yau (1989). A Short Course in Cloud
 * Physics (3rd ed.). Pergamon Press. p. 16. ISBN 0-7506-3215-1.
 *
 * Returns: the latent heat of condensation for water in J / kg.
 */
SondeJpKgpK 
sonde_latent_heat_of_condensation_vaporization(SondeCelsius t)
{
    /* The table has values from -40.0 to 40.0. So from -100.0 to -40.0 is actually an extrapolation.
     * I graphed the values from the extrapolation, and the curve looks good, and is approaching the
     * latent heat of sublimation, but does not exceed it. This seems very reasonable to me,
     * especially considering that a common approximation is to just us a constant value.
     */
    if (t.val < -100.0 || t.val > 60.0)
    {
        return SondeErrorWrap(SondeJpKgpK, SONDE_ERROR_OUT_OF_RANGE);
    }

    /*f64 val = (2500.8 - 2.36 * t.val + 0.0016 * t.val * t.val - 0.00006 * t.val * t.val * t.val) * 1000.0; */
    f64 val = (2500.8 + (-2.36 + (0.0016 - 0.00006 * t.val) * t.val) * t.val) * 1000.0;

    return (SondeJpKgpK){ .val = val };
}

/* Calculate equivalent potential temperature.
 *
 * Equation from ["The Glossary of Meteorology"]
 * (http://glossary.ametsoc.org/wiki/Equivalent_potential_temperature) online where the
 * approximation of ignoring the "total water mixing ratio" is used since most of the time we do
 * not have the necessary information to calculate that.
 *
 * `t` - the initial temperature.
 * `dp` - the initial dew point.
 * `p` - the initial pressure.
 *
 * Returns: The equivalent potential temperature in Kelvin.
 */
SondeKelvin 
sonde_equivalent_potential_temperature(SondeCelsius t, SondeCelsius dp, SondeHectopascal p)
{
    SondeKelvin t_k = sonde_celsius_to_kelvin(t);

    f64 const P0 = 1000.0;  /* Reference pressure for potential temperatures in hPa. */
    f64 rv = sonde_mixing_ratio(dp, p);
    f64 pd = p.val - sonde_vapor_pressure_water(dp).val;

    if(pd < 0.0) { return SondeErrorWrap(SondeKelvin, SONDE_ERROR_INVALID_INPUT); }

    f64 h = sonde_relative_humidity_liquid(t, dp);
    SondeJpKgpK lv = sonde_latent_heat_of_condensation_vaporization(t);

    f64 theta_e =
        t_k.val
        * pow(P0 / pd, sonde_const_Rd.val / sonde_const_cpd.val) 
        * pow(h, -rv * (sonde_const_Rv.val / sonde_const_cpd.val))
        * exp(lv.val * rv / sonde_const_cpd.val / t_k.val);

    if(isinf(theta_e)) { return SondeErrorWrap(SondeKelvin, SONDE_ERROR_OUT_OF_RANGE); }
    return (SondeKelvin){ .val = theta_e };
}

f64
sonde_temperature_c_from_equiv_pot_temp_saturated_pressure_inner_root_(f64 t_c, f64 *params)
{
    SondeCelsius t = { .val = t_c };
    SondeHectopascal p = { .val = params[0] };
    SondeKelvin theta_e = { .val = params[1] };
    SondeKelvin theta_e_calc = sonde_equivalent_potential_temperature(t, t, p);
    return theta_e_calc.val - theta_e.val;
}

/* Given the pressure and equivalent potential temperature, assume saturation and calculate the
 * temperature.
 *
 * This function is useful if you were trying to calculate the temperature for plotting a
 * moist adiabat on a skew-t log-p diagram.
 *
 * Returns: The temperature in Celsius.
 */
SondeCelsius 
sonde_temperature_from_equiv_pot_temp_saturated_pressure(SondeHectopascal p, SondeKelvin theta_e)
{
    f64 params[2] = {p.val, theta_e.val};

    f64 max_t = sonde_dew_point_from_vapor_pressure(p).val;
    if(isnan(max_t)) { return (SondeCelsius){ .val = max_t }; }

    f64 t_c = sonde_find_root(
            sonde_temperature_c_from_equiv_pot_temp_saturated_pressure_inner_root_,
            SONDE_MIN_T_VP_LIQUID_C,
            max_t,
            params);

    return (SondeCelsius){ .val = t_c };
}

f64
sonde_pressure_hpa_at_lcl_inner_root_rough_(f64 press_hpa, f64 *params)
{
    SondeHectopascal p = { .val = press_hpa };
    SondeKelvin theta = { .val = params[0] };
    f64 mw = params[1];

    f64 t_c = sonde_temperature_from_pot_temperature(theta, p).val;
    f64 dp_c = sonde_dew_point_from_p_and_mw(p, mw).val;
    return t_c - dp_c;
}

f64
sonde_pressure_hpa_at_lcl_inner_root_accurate_(f64 press_hpa, f64 *params)
{
    SondeHectopascal p = { .val = press_hpa };
    SondeKelvin theta = { .val = params[0] };
    SondeKelvin theta_e = { .val = params[1] };

    f64 t_c = sonde_temperature_from_pot_temperature(theta, p).val;
    f64 dp_c = sonde_temperature_from_equiv_pot_temp_saturated_pressure(p, theta_e).val;
    return t_c - dp_c;
}

/* Approximate pressure of the Lifting Condensation Level (LCL).
 *
 * `t` - the initial temperature in Celsius.
 * `dp` - the initial dew point in Celsius.
 * `p` - the initial pressure in hectopascals.
 *
 * Returns: The pressure at the LCL in hPa.
 */
SondeHectopascal 
sonde_pressure_at_lcl(SondeCelsius t, SondeCelsius dp, SondeHectopascal p)
{
    if(dp.val >= t.val) { return p; }

    f64 theta_k = sonde_potential_temperature(p, t).val;
    f64 mw = sonde_mixing_ratio(dp, p);
    f64 theta_e_k = sonde_equivalent_potential_temperature(t, dp, p).val;

    /* Search between 1060 and 300 hPa. If it falls outside that range, give up! This is a 
     * quick search using an approximation that is simple and should be very fast to compute.
     */
    f64 params[2] = {theta_k, mw};
    f64 first_guess_hpa = sonde_find_root(sonde_pressure_hpa_at_lcl_inner_root_rough_, 300.0, 1060.0, params);

    /* Now search for the pressure where the equivalent potential temperature and potential
     * temperature of the parcel are equal, assuming saturation. This is more robust, but also
     * more computationally expensive.
     */
    f64 const DELTA = 10.0;
    params[1] = theta_e_k;
    f64 lclp = sonde_find_root(
            sonde_pressure_hpa_at_lcl_inner_root_accurate_,
            first_guess_hpa - DELTA,
            first_guess_hpa + DELTA,
            params);
    lclp = isnan(lclp) ? first_guess_hpa : lclp;

    return (SondeHectopascal){ .val = lclp };
}

/* Calculate the temperature and pressure at the lifting condensation level (LCL).
 *
 * Eqs 5.17 and 5.18 from from "Weather Analysis" by Dušan Dujrić 
 *
 * `t` - the initial temperature in Celsius.
 * `dp` - the initial dew point in Celsius.
 * `p` - the initial pressure of the parcel in hPa.
 *
 * Returns: The pressure in hPa and the temperature in Celsius at the LCL.
 */
SondePressureTemperaturePair 
sonde_pressure_and_temperature_at_lcl(SondeCelsius t, SondeCelsius dp, SondeHectopascal p)
{
    SondeHectopascal plcl = sonde_pressure_at_lcl(t, dp, p);
    SondeCelsius tlcl = sonde_temperature_from_pot_temperature(sonde_potential_temperature(p, t), plcl);

    return (SondePressureTemperaturePair){ .p = plcl, .t = tlcl };
}

f64
sonde_wet_bulb_c_inner_root_(f64 t_c, f64 *parms)
{
    SondeCelsius t = { .val = t_c };
    SondeHectopascal p = { .val = parms[0] };
    SondeKelvin theta_e = { .val = parms[1] };

    SondeKelvin theta_e_calc = sonde_equivalent_potential_temperature(t, t, p);
    return theta_e_calc.val - theta_e.val;
}

/* Calculate the web bulb temperature.
 *
 * Returns: The wet bulb temperature in Celsius.
 */
SondeCelsius 
sonde_wet_bulb(SondeCelsius t, SondeCelsius dp, SondeHectopascal p)
{
    SondePressureTemperaturePair tp = sonde_pressure_and_temperature_at_lcl(t, dp, p);
    SondeHectopascal plcl = tp.p;
    SondeCelsius tlcl = tp.t;

    SondeKelvin theta_e = sonde_equivalent_potential_temperature(tlcl, tlcl, plcl);
    f64 parms[2] = {p.val, theta_e.val};
    f64 wet_bulb_c = sonde_find_root(sonde_wet_bulb_c_inner_root_, dp.val, t.val, parms);
    return (SondeCelsius){ .val = wet_bulb_c };
}

/* Calculate the Pyrocumulonimbus Firepower Threshold (PFT).
 *
 * Output is in Gigawatts. The first reference below (Tory & Kepert, 2021) has most of the details
 * about how to calculate the PFT, the other paper (Tory et. al, 2018) outlines the model in general.
 *
 * # References
 *
 * Tory, K. J., & Kepert, J. D. (2021). Pyrocumulonimbus Firepower Threshold: Assessing the
 *     Atmospheric Potential for pyroCb, Weather and Forecasting, 36(2), 439-456. Retrieved Jun 2,
 *     2021, from https://journals.ametsoc.org/view/journals/wefo/36/2/WAF-D-20-0027.1.xml
 *
 * Tory, K. J., Thurston, W., & Kepert, J. D. (2018). Thermodynamics of Pyrocumulus: A Conceptual
 *     Study, Monthly Weather Review, 146(8), 2579-2598. Retrieved Jun 2, 2021, from
 *     https://journals.ametsoc.org/view/journals/mwre/146/8/mwr-d-17-0377.1.xml
 *
 * # Arguments
 *  - zfc is the height above ground of the level of free convection. Equation 25, Tory & Kepert (2021).
 *  - pfc is the pressure at zfc. Equation 28, Tory & Kepert (2021).
 *  - mean_wind is the magnitude of the mean velocity (vector average). Equation 25, Tory & Kepert (2021).
 *  - theta_diff is the difference between the mixed layer potential temperature and the parcel
 *    potential temperature at the level of free convection. Equation 25, Tory & Kepert (2021).
 *  - theta_fc is the potential temperature at the level of free convection. Equation 26, Tory & Kepert (2021).
 *  - p_sfc is the surface pressure. Equation 28, Tory & Kepert (2021).
 */
SondeGw 
sonde_pft(SondeMeter zfc,
    SondeHectopascal pfc,
    SondeMps mean_wind,
    SondeKelvin theta_diff,
    SondeKelvin theta_fc,
    SondeHectopascal p_sfc)
{
    f64 z_fc_km = sonde_meters_to_kilometers(zfc).val;

    /* Constant component of equation 25, from table 2 in Tory & Kepert (2021).      */
    f64 const PFT_CONST = 397.3;                                  /* J / (kg K)      */

    /* Equation 28                                                                   */
    f64 p_c = p_sfc.val - (p_sfc.val - pfc.val) / (1.0 + 0.32 * 0.4);

    /* Equation 26 - multiply by 100 to convert hPa to Pa, so density is in kg / m^3 */
    f64 density =
        (100.0 * p_c) / (sonde_const_Rd.val * theta_fc.val) * pow(1000.0 / p_c, sonde_const_Rd.val / sonde_const_cpd.val);

    /* Divide by 1000.0 to get Gigawatts. Since the heights z_fc are in km and squared, we've
     * already 'divided' by 10^6.
     */
    return (SondeGw){ .val = PFT_CONST * density * z_fc_km * z_fc_km * mean_wind.val * theta_diff.val / 1000.0 };
}

b32 
sonde_profile_code_present(u32 profiles, SondeProfileCode code)
{
    return !!(profiles & code);
}

u32 
sonde_profile_code_set(u32 profiles, SondeProfileCode code)
{
    switch(code)
    {
        /* For mutually exclusive flags, make sure the other flag is turned off. */
    case SONDE_PC_WIND_SPEED_DIR:  return (profiles & ~SONDE_PC_WIND_UV) | SONDE_PC_WIND_SPEED_DIR;
    case SONDE_PC_WIND_UV:         return (profiles & ~SONDE_PC_WIND_SPEED_DIR) | SONDE_PC_WIND_UV;

    default:                       return profiles | code;
    };
}

SondeSounding *
sonde_sounding_alloc_and_init(MagAllocator *alloc, size capacity)
{
    SondeSounding *snd = mag_allocator_malloc(alloc, SondeSounding);

    snd->profiles = 0;

    snd->mslp = SondeErrorWrap(SondeHectopascal, SONDE_ERROR_MISSING_DATA);
    snd->station_p = SondeErrorWrap(SondeHectopascal, SONDE_ERROR_MISSING_DATA);
    snd->surface_t = SondeErrorWrap(SondeCelsius, SONDE_ERROR_MISSING_DATA);
    snd->surface_dp = SondeErrorWrap(SondeCelsius, SONDE_ERROR_MISSING_DATA);
    snd->precip_1hr = SondeErrorWrap(SondeMillimeters, SONDE_ERROR_MISSING_DATA);

    snd->levels = pak_array_ledger_create(capacity);
    snd->p =              mag_allocator_nmalloc(alloc, capacity, SondeHectopascal);          Assert(snd->p);
    snd->t =              mag_allocator_nmalloc(alloc, capacity, SondeCelsius);              Assert(snd->t);
    snd->dp =             mag_allocator_nmalloc(alloc, capacity, SondeCelsius);              Assert(snd->dp);
    snd->wb =             mag_allocator_nmalloc(alloc, capacity, SondeCelsius);              Assert(snd->wb);
    snd->vt =             mag_allocator_nmalloc(alloc, capacity, SondeCelsius);              Assert(snd->vt);
    snd->theta =          mag_allocator_nmalloc(alloc, capacity, SondeKelvin);               Assert(snd->theta);
    snd->theta_e =        mag_allocator_nmalloc(alloc, capacity, SondeKelvin);               Assert(snd->theta_e);
    snd->pvv =            mag_allocator_nmalloc(alloc, capacity, SondeHectopascalPerSecond); Assert(snd->pvv);
    snd->hgt =            mag_allocator_nmalloc(alloc, capacity, SondeMeter);                Assert(snd->hgt);
    snd->cloud_fraction = mag_allocator_nmalloc(alloc, capacity, f64);                       Assert(snd->cloud_fraction);
    snd->uv_wind =        mag_allocator_nmalloc(alloc, capacity, SondeUVMps);                Assert(snd->uv_wind);

    /* Initialize all the profile data to missing values */
    for(size i = 0; i < capacity; ++i)
    {
        snd->p[i] = SondeErrorWrap(SondeHectopascal, SONDE_ERROR_MISSING_DATA); 
        snd->t[i] = SondeErrorWrap(SondeCelsius, SONDE_ERROR_MISSING_DATA); 
        snd->dp[i] = SondeErrorWrap(SondeCelsius, SONDE_ERROR_MISSING_DATA); 
        snd->wb[i] = SondeErrorWrap(SondeCelsius, SONDE_ERROR_MISSING_DATA); 
        snd->vt[i] = SondeErrorWrap(SondeCelsius, SONDE_ERROR_MISSING_DATA); 
        snd->theta[i] = SondeErrorWrap(SondeKelvin, SONDE_ERROR_MISSING_DATA); 
        snd->theta_e[i] = SondeErrorWrap(SondeKelvin, SONDE_ERROR_MISSING_DATA); 
        snd->pvv[i] = SondeErrorWrap(SondeHectopascalPerSecond, SONDE_ERROR_MISSING_DATA); 
        snd->hgt[i] = SondeErrorWrap(SondeMeter, SONDE_ERROR_MISSING_DATA); 
        snd->cloud_fraction[i] = sonde_error_create_nan(SONDE_ERROR_MISSING_DATA); 
        snd->uv_wind[i] = (SondeUVMps)
            {
                .u = sonde_error_create_nan(SONDE_ERROR_MISSING_DATA),
                .v = sonde_error_create_nan(SONDE_ERROR_MISSING_DATA)
            };
    }

    return snd;
}

typedef enum
{
    SBPC_NONE = 0, /* Column does not exist                                          */
    SBPC_PRES,     /* Pressure hPa                                                   */
    SBPC_T,        /* Temperature, Celsius                                           */
    SBPC_WB,       /* Web bulb temperature, Celsius                                  */
    SBPC_DP,       /* Dew point temperature, Celsius                                 */
    SBPC_THETA_E,  /* Equivalent potential temperature, Kelvin                       */
    SBPC_DRCT,     /* Wind direction degrees                                         */
    SBPC_SPD,      /* Wind speed, knots                                              */
    SBPC_PVV,      /* Pressure vertical velocity                                     */
    SBPC_CLD,      /* Cloud fraction                                                 */
    SBPC_HGT,      /* Geopotential height, meters                                    */
    SBPC_UW,       /* U-wind component, meters per second                            */
    SBPC_VW,       /* V-wind component, meters per second                            */

    // TODO: Add and parse more surface features including mslp and precip_1hr
} InternalSondeBufkitProfileColumns;

b32
internal_sonde_bufkit_parse_datetime(ElkStr str, ElkTime *out)
{
    /* YYMMDD/HHMM format */
    i64 year = INT64_MIN;
    i64 month = INT64_MIN;
    i64 day = INT64_MIN;
    i64 hour = INT64_MIN;
    i64 minutes = INT64_MIN;

    if(
        elk_str_parse_i64(elk_str_substr(str,  0, 2), &year     ) && 
        elk_str_parse_i64(elk_str_substr(str,  2, 2), &month    ) &&
        elk_str_parse_i64(elk_str_substr(str,  4, 2), &day      ) &&
        elk_str_parse_i64(elk_str_substr(str,  7, 2), &hour     ) &&
        elk_str_parse_i64(elk_str_substr(str,  9, 2), &minutes  ))
    {
        year = year > 80 ? year + 1900 : year + 2000;
        *out = elk_time_from_ymd_and_hms((i16)year, (i8)month, (i8)day, (i8)hour, (i8)minutes, 0);
        return true;
    }

    return false;
}

ElkStrSplitPair
internal_sonde_bufkit_next_token(ElkStr input)
{
    ElkStr left = { .start = input.start, .len = 0 };
    ElkStr right = {0};

    /* Skip any leading space. */
    size cnt = 0;
    while( cnt < input.len && (*left.start == ' ' || *left.start == '\n'))
    {
        ++left.start;
        ++cnt;
    }

    /* Find the length */
    while(cnt < input.len && left.start[left.len] != ' ' && left.start[left.len] != '\n')
    {
        ++left.len;
        ++cnt;
    }

    right.start = left.start + left.len;
    right.len = input.len - cnt;

    return (ElkStrSplitPair){ .left = elk_str_strip(left), .right = right };
}

SondeSoundingList *
sonde_sounding_from_bufkit_str(MagAllocator *alloc, ElkStr txt, ElkStr source_description)
{
    SondeSoundingList *sndgs = NULL;
    SondeSoundingList *current_sndg = NULL;

    /* Split the file into the upper air and surface sections. */
    ElkStrSplitPair pair = elk_str_split_at_substr_nt(txt, "STN YYMMDD/HHMM ");
    StopIf(!pair.right.start, goto RETURN);       /* Return early with what data we were able to parse */
    ElkStr upper_air = pair.left;
    ElkStr surface = pair.right;

    /* Get the header row that lists the available columns */
    pair = elk_str_split_at_substr_nt(upper_air, "STID =");
    ElkStr header = elk_str_strip(pair.left);
    ElkStr upper_air_body = pair.right;

    /* Parse the column names for the upper air data ----------------------------------------------------------------------*/
    /* PRES - Pressure (hPa)                       */
    /* TMPC - Temperature (C)                      */
    /* TMWC - Wet bulb temperature (C)             */
    /* DWPC - Dewpoint (C)                         */
    /* THTE - Equivalent potential temperature (K) */
    /* DRCT - Wind direction (degrees)             */
    /* SKNT - Wind speed (knots)                   */
    /* OMEG - Vertical velocity (Pa/s)             */
    /* CFRL - Fractional cloud coverage (percent)  */
    /* HGHT - Height of pressure level (m)         */

    InternalSondeBufkitProfileColumns cols[40] = {0};
    pair = elk_str_split_on_char(header, '\n');   /* Should leave "SNPARM = ..." as pair.left   */
    pair = elk_str_split_on_char(pair.left, '='); /* leaves just the column names in pair.right */
    pair = elk_str_split_on_char(elk_str_strip(pair.right), ';');
    size n_cols = 0;
    u32 profiles = 0;
    while(pair.left.len > 0 && n_cols < ECO_ARRAY_SIZE(cols))
    {
        if(elk_str_eq((ElkStr){ .start = "PRES", .len = 4}, pair.left))
        {
            cols[n_cols++] = SBPC_PRES;
            profiles = sonde_profile_code_set(profiles, SONDE_PC_PRESSURE);
        }
        else if(elk_str_eq((ElkStr){ .start = "TMPC", .len = 4}, pair.left))
        {
            cols[n_cols++] = SBPC_T;
            profiles = sonde_profile_code_set(profiles, SONDE_PC_TEMPERATURE);
        }
        else if(elk_str_eq((ElkStr){ .start = "TMWC", .len = 4}, pair.left))
        {
            cols[n_cols++] = SBPC_WB;
            profiles = sonde_profile_code_set(profiles, SONDE_PC_WET_BULB);
        }
        else if(elk_str_eq((ElkStr){ .start = "DWPC", .len = 4}, pair.left))
        {
            cols[n_cols++] = SBPC_DP;
            profiles = sonde_profile_code_set(profiles, SONDE_PC_DEW_POINT);
        }
        else if(elk_str_eq((ElkStr){ .start = "THTE", .len = 4}, pair.left))
        {
            cols[n_cols++] = SBPC_THETA_E;
            profiles = sonde_profile_code_set(profiles, SONDE_PC_THETA_E);
        }
        else if(elk_str_eq((ElkStr){ .start = "DRCT", .len = 4}, pair.left))
        {
            cols[n_cols++] = SBPC_DRCT;
            profiles = sonde_profile_code_set(profiles, SONDE_PC_WIND_SPEED_DIR);
        }
        else if(elk_str_eq((ElkStr){ .start = "SKNT", .len = 4}, pair.left))
        {
            cols[n_cols++] = SBPC_SPD;
            profiles = sonde_profile_code_set(profiles, SONDE_PC_WIND_SPEED_DIR);
        }
        else if(elk_str_eq((ElkStr){ .start = "OMEG", .len = 4}, pair.left))
        {
            cols[n_cols++] = SBPC_PVV;
            profiles = sonde_profile_code_set(profiles, SONDE_PC_PVV);
        }
        else if(elk_str_eq((ElkStr){ .start = "CFRL", .len = 4}, pair.left))
        {
            cols[n_cols++] = SBPC_CLD;
            profiles = sonde_profile_code_set(profiles, SONDE_PC_CLOUD_FRACTION);
        }
        else if(elk_str_eq((ElkStr){ .start = "HGHT", .len = 4}, pair.left))
        {
            cols[n_cols++] = SBPC_HGT;
            profiles = sonde_profile_code_set(profiles, SONDE_PC_GEOPOTENTIAL_HGT);
        }
        else
        {
            n_cols++;
        }

        pair = elk_str_split_on_char(elk_str_strip(pair.right), ';');
    }

    /* Parse the upper section */
    ElkStr split_str = { .start = "STID = ", .len = 7 };
    pair = elk_str_split_on_substr(upper_air_body, split_str); /* Chop off leading STID */
    pair = elk_str_split_on_substr(pair.right, split_str);
    while(pair.left.len > 0)
    {
        ElkStr remaining_text = elk_str_strip(pair.left);

        /* Create the new node and sounding. */
        i64 capacity = elk_str_line_count(remaining_text);
        if(n_cols > 8) { capacity = capacity / 2 + 1; } else { capacity += 1; } /* "rows" can be stored on multiple lines */
        SondeSoundingList *new_node = eco_arena_malloc(alloc, SondeSoundingList);
        SondeSounding *new_sndg = sonde_sounding_alloc_and_init(alloc, capacity);
        StopIf(!(new_node && new_sndg), break);
        new_node->snd = new_sndg;

        /* Store this new node in the list */
        if(!sndgs) 
        {
            sndgs = new_node;
            current_sndg = new_node;
        }
        else
        {
            current_sndg->next = new_node;
            current_sndg = new_node;
        }

        /* Get the ledger for keeping track of levels, and skip index zero to save it for surface data */
        PakArrayLedger *levels = &new_sndg->levels;
        size idx = pak_array_ledger_push_back_index(levels); 

        /* Record what profiles are present */
        new_sndg->profiles = profiles;

        /* Set the source description */
        new_sndg->source = eco_str_alloc_copy(source_description, alloc);

        /* Parse the station id */
        ElkStrSplitPair inner_pair = elk_str_split_on_char(remaining_text, ' ');
        new_sndg->stn_info.id = eco_str_alloc_copy(elk_str_strip(inner_pair.left), alloc);
        remaining_text = inner_pair.right;

        /* Parse the station number */
        ElkStr stnm_pattern = { .start = "STNM = ", .len = 7 };
        inner_pair = elk_str_split_on_substr(remaining_text, stnm_pattern);
        inner_pair = elk_str_split_on_char(inner_pair.right, ' ');
        remaining_text = inner_pair.right;
        b32 success = elk_str_parse_i64(inner_pair.left, &new_sndg->stn_info.station_number);
        if(!success)
        {
            new_sndg->stn_info.station_number = 0;
            success = true;
        }

        /* Parse the time */
        ElkStr time_pattern = { .start = "TIME = ", .len = 7 };
        inner_pair = elk_str_split_on_substr(remaining_text, time_pattern);
        inner_pair = elk_str_split_on_char(inner_pair.right, '\n');
        remaining_text = inner_pair.right;
        success &= internal_sonde_bufkit_parse_datetime(elk_str_strip(inner_pair.left), &new_sndg->valid_time);
        new_node->valid_time = new_sndg->valid_time;
        Assert(success);

        /* Parse the the latitude */
        ElkStr lat_pattern = { .start = "SLAT = ", .len = 7 };
        inner_pair = elk_str_split_on_substr(remaining_text, lat_pattern);
        inner_pair = elk_str_split_on_char(inner_pair.right, ' ');
        remaining_text = inner_pair.right;
        success &= elk_str_fast_parse_f64(inner_pair.left, &new_sndg->stn_info.latitude);
        if(!success)
        {
            new_sndg->stn_info.latitude = sonde_error_create_nan(SONDE_ERROR_MISSING_DATA);
            success = true;
        }

        /* Parse the the longitude */
        ElkStr lon_pattern = { .start = "SLON = ", .len = 7 };
        inner_pair = elk_str_split_on_substr(remaining_text, lon_pattern);
        inner_pair = elk_str_split_on_char(inner_pair.right, ' ');
        remaining_text = inner_pair.right;
        success &= elk_str_fast_parse_f64(inner_pair.left, &new_sndg->stn_info.longitude);
        if(!success)
        {
            new_sndg->stn_info.longitude = sonde_error_create_nan(SONDE_ERROR_MISSING_DATA);
            success = true;
        }

        /* Parse the elevation in meters */
        ElkStr elev_pattern = { .start = "SELV = ", .len = 7 };
        inner_pair = elk_str_split_on_substr(remaining_text, elev_pattern);
        inner_pair = elk_str_split_on_char(inner_pair.right, '\n');
        remaining_text = inner_pair.right;
        success &= elk_str_fast_parse_f64(inner_pair.left, &new_sndg->stn_info.elevation.val);
        if(!success)
        {
            new_sndg->stn_info.elevation = SondeErrorWrap(SondeMeter, SONDE_ERROR_MISSING_DATA);
            success = true;
        }

        /* Parse the lead time. */
        ElkStr lt_pattern = { .start = "STIM = ", .len = 7 };
        inner_pair = elk_str_split_on_substr(remaining_text, lt_pattern);
        inner_pair = elk_str_split_on_char(inner_pair.right, '\n');
        remaining_text = inner_pair.right;
        i64 tmp = 0;
        success &= elk_str_parse_i64(elk_str_strip(inner_pair.left), &tmp);
        Assert(success); 
        new_sndg->lead_time = (i32)tmp;

        /* Skip the header row */
        ElkStr profiles_pattern = { .start = "PRES ", .len = 5 };
        inner_pair = elk_str_split_at_substr(remaining_text, profiles_pattern);
        remaining_text = inner_pair.right;
        size next_token_num = 0;
        do 
        {
            inner_pair = internal_sonde_bufkit_next_token(remaining_text);
            remaining_text = inner_pair.right;
            ++next_token_num;
        } while(next_token_num % n_cols);
        Assert(next_token_num % n_cols == 0);
        Assert(idx == 0);

        /* Parse the profiles */
        do
        {
            /* Go up a level once we've parsed all the columns */
            if(next_token_num % n_cols == 0)
            {
                idx = pak_array_ledger_push_back_index(levels);
                if(idx == PAK_COLLECTION_FULL) { break; }
            }

            /* Get the next token */
            inner_pair = internal_sonde_bufkit_next_token(remaining_text);
            ElkStr token = inner_pair.left;
            if(token.len == 0 || !token.start) { break; }
            remaining_text = inner_pair.right;

            f64 tmp;
            success &= elk_str_fast_parse_f64(token, &tmp); Assert(success);
            if(tmp == -9999.0) { tmp = sonde_error_create_nan(SONDE_ERROR_MISSING_DATA); } /* Check for missing data */

            switch(cols[next_token_num % n_cols])
            {
                case SBPC_PRES:    { new_sndg->p[idx]              = (SondeHectopascal)         { .val = tmp }; } break;
                case SBPC_T:       { new_sndg->t[idx]              = (SondeCelsius)             { .val = tmp }; } break;
                case SBPC_WB:      { new_sndg->wb[idx]             = (SondeCelsius)             { .val = tmp }; } break;
                case SBPC_DP:      { new_sndg->dp[idx]             = (SondeCelsius)             { .val = tmp }; } break;
                case SBPC_THETA_E: { new_sndg->theta_e[idx]        = (SondeKelvin)              { .val = tmp }; } break;
                case SBPC_DRCT:    { new_sndg->wind[idx].dir       =                                     tmp;   } break;
                case SBPC_SPD:     { new_sndg->wind[idx].spd       =                                     tmp;   } break;
                case SBPC_PVV:     { new_sndg->pvv[idx]            = (SondeHectopascalPerSecond){ .val = tmp }; } break;
                case SBPC_CLD:     { new_sndg->cloud_fraction[idx] =                                     tmp;   } break;
                case SBPC_HGT:     { new_sndg->hgt[idx]            = (SondeMeter)               { .val = tmp }; } break;
                default: { /* ignore unknown values */ }                                                          break;
            }

            ++next_token_num;
        } while(true);

        /* Move to the next profile */
        pair = elk_str_split_on_substr(pair.right, split_str);
    }

    /* Parse the column names for the surface data ------------------------------------------------------------------------*/
    /* STN  - 6-digit station number                              */
    /* YYMMDD/HHMM - Valid time (UTC) in numeric format           */
    /* PMSL - Mean sea level pressure (hPa)                       */
    /* PRES - Station pressure (hPa)                              */
    /* SKTC - Skin temperature (C)                                */
    /* STC1 - Layer 1 soil temperature (K)                        */
    /* SNFL - 1-hour accumulated snowfall (Kg/m**2)               */
    /* WTNS - Soil moisture availability (percent)                */
    /* P01M - 1-hour total precipitation (mm)                     */
    /* C01M - 1-hour convective precipitation (mm)                */
    /* STC2 - Layer 2 soil temperature (K)                        */
    /* LCLD - Low cloud coverage (percent)                        */
    /* MCLD - Middle cloud coverage (percent)                     */
    /* HCLD - High cloud coverage (percent)                       */
    /* SNRA - Snow ratio from explicit cloud scheme (percent)     */
    /* UWND - 10-meter U wind component (m/s)                     */
    /* VWND - 10-meter V wind component (m/s)                     */
    /* R01M - 1-hour accumulated surface runoff (mm)              */
    /* BFGR - 1-hour accumulated baseflow-groundwater runoff (mm) */
    /* T2MS - 2-meter temperature (C)                             */
    /* Q2MS - 2-meter specific humidity                           */
    /* WXTS - Snow precipitation type (1=Snow)                    */
    /* WXTP - Ice pellets precipitation type (1=Ice pellets)      */
    /* WXTZ - Freezing rain precipitation type (1=Freezing rain)  */
    /* WXTR - Rain precipitation type (1=Rain)                    */
    /* USTM - U-component of storm motion (m/s)                   */
    /* VSTM - V-component of storm motion (m/s)                   */
    /* HLCY - Storm relative helicity (m**2/s**2)                 */
    /* SLLH - 1-hour surface evaporation (mm)                     */
    /* EVAP - Evaportaion, units not given, mm?                   */
    /* WSYM - Weather type symbol number                          */
    /* CDBP - Pressure at the base of cloud (hPa)                 */
    /* VSBK - Visibility (km)                                     */
    /* TD2M - 2-meter dewpoint (C)                                */
    /* more paramters than listed here!                           */

    memset(cols, 0, sizeof(cols));

    /* Get the header row that lists the available columns in the surface data */
    size sfc_header_len = 0;
    for(char *c = surface.start; *c; ++c)
    {
        ++sfc_header_len;

        if(*c == '\n' && *(c + 1)) /* If this is a new line and not the end of the string... */
        {
            char c1 = *(c + 1);    /* Check if the next line starts with a number.           */
            if(c1 >= '0' && c1 <= '9') { break; }
        }
    }
    ElkStr sfc_header = elk_str_substr(surface, 0, sfc_header_len);
    ElkStr sfc_body = elk_str_substr(surface, sfc_header_len, surface.len - sfc_header_len);
    pair = elk_str_split_on_char(sfc_header, ' ');

    n_cols = 0;
    while(pair.left.len > 0 && n_cols < ECO_ARRAY_SIZE(cols))
    {
        if(elk_str_eq((ElkStr)     { .start = "PRES", .len = 4}, pair.left)) { cols[n_cols++] = SBPC_PRES; }
        else if(elk_str_eq((ElkStr){ .start = "T2MS", .len = 4}, pair.left)) { cols[n_cols++] = SBPC_T;    }
        else if(elk_str_eq((ElkStr){ .start = "TD2M", .len = 4}, pair.left)) { cols[n_cols++] = SBPC_DP;   }
        else if(elk_str_eq((ElkStr){ .start = "UWND", .len = 4}, pair.left)) { cols[n_cols++] = SBPC_UW;   }
        else if(elk_str_eq((ElkStr){ .start = "VWND", .len = 4}, pair.left)) { cols[n_cols++] = SBPC_VW;   }
        else { n_cols++; }

        ElkStrSplitPair p2 = elk_str_split_on_char(pair.right, ' ');
        ElkStrSplitPair p3 = elk_str_split_on_char(pair.right, '\n');
        pair = (p2.left.len > 0 && p2.left.len <= p3.left.len) ? p2 : p3;
        pair.left = elk_str_strip(pair.left);
    }

    /* Parse the surface section */
    b32 success = true;
    size next_token_num = 0;
    pair = elk_str_split_on_char(sfc_body, ' ');
    ElkStr token = elk_str_strip(pair.left);
    ElkStr remaining = elk_str_strip(pair.right);
    SondeHectopascal sfc_pres = SondeErrorWrap(SondeHectopascal, SONDE_ERROR_MISSING_DATA);
    SondeCelsius sfc_t = SondeErrorWrap(SondeCelsius, SONDE_ERROR_MISSING_DATA);
    SondeCelsius sfc_dp = SondeErrorWrap(SondeCelsius, SONDE_ERROR_MISSING_DATA);
    SondeMps sfc_u = SondeErrorWrap(SondeMps, SONDE_ERROR_MISSING_DATA);
    SondeMps sfc_v = SondeErrorWrap(SondeMps, SONDE_ERROR_MISSING_DATA);
    ElkTime valid_time = {0};
    current_sndg = sndgs;
    while(token.len > 0)
    {
        /* Check for and parse the valid time */
        if(next_token_num % n_cols == 1)
        {
            success &= internal_sonde_bufkit_parse_datetime(token, &valid_time); Assert(success);
        }
        else
        {
            f64 tmp;
            success &= elk_str_fast_parse_f64(token, &tmp); Assert(success);
            if(tmp == -9999.0 || tmp == 999.0)  /* Check for missing data */
            {
                tmp = sonde_error_create_nan(SONDE_ERROR_MISSING_DATA);
            }

            switch(cols[(next_token_num) % n_cols])
            {
                case SBPC_PRES: { sfc_pres.val = tmp; } break;
                case SBPC_T:    { sfc_t.val    = tmp; } break;
                case SBPC_DP:   { sfc_dp.val   = tmp; } break;
                case SBPC_UW:   { sfc_u.val    = tmp; } break;
                case SBPC_VW:   { sfc_u.val    = tmp; } break;
                default: { /* ignore unknown cols */  } break;
            }
        }

        ++next_token_num;

        /* Store the data once we've parsed all columns */
        if(next_token_num % n_cols == 0)
        {
            /* Match the valid time of the surface data and the soundings. Should ALWAYS be in order. */
            while(current_sndg && current_sndg->valid_time < valid_time)
            {
                current_sndg = current_sndg->next;
            }

            if(current_sndg->valid_time == valid_time)
            {
                current_sndg->snd->station_p = sfc_pres;
                current_sndg->snd->p[0] = sfc_pres;
                current_sndg->snd->surface_t = sfc_t;
                current_sndg->snd->t[0] = sfc_t;
                current_sndg->snd->surface_dp = sfc_dp;
                current_sndg->snd->dp[0] = sfc_dp;
                if(!(sonde_is_error(sfc_u.val) && sonde_is_error(sfc_v.val)))
                {
                    SondeUVMps sfc_uvw = { .u = sfc_u.val, .v = sfc_v.val };
                    SondeSpdDirKts sfc_spd_dir = sonde_uv_to_spd_dir(sfc_uvw);
                    current_sndg->snd->wind[0] = sfc_spd_dir;
                }

                /* Fill surface values for profiles that don't specifically have the values listed. */
                if(sonde_profile_code_present(current_sndg->snd->profiles, SONDE_PC_WET_BULB))
                {
                    current_sndg->snd->wb[0] = sonde_wet_bulb(sfc_t, sfc_dp, sfc_pres);
                }

                if(sonde_profile_code_present(current_sndg->snd->profiles, SONDE_PC_THETA))
                {
                    current_sndg->snd->theta[0] = sonde_potential_temperature(sfc_pres, sfc_t);
                }

                if(sonde_profile_code_present(current_sndg->snd->profiles, SONDE_PC_THETA_E))
                {
                    current_sndg->snd->theta_e[0] = sonde_equivalent_potential_temperature(sfc_t, sfc_dp, sfc_pres);
                }
            }
            
            sfc_pres = SondeErrorWrap(SondeHectopascal, SONDE_ERROR_MISSING_DATA);
            sfc_t = SondeErrorWrap(SondeCelsius, SONDE_ERROR_MISSING_DATA);
            sfc_dp = SondeErrorWrap(SondeCelsius, SONDE_ERROR_MISSING_DATA);
            sfc_u = SondeErrorWrap(SondeMps, SONDE_ERROR_MISSING_DATA);
            sfc_v = SondeErrorWrap(SondeMps, SONDE_ERROR_MISSING_DATA);
        }

        ElkStrSplitPair p2 = elk_str_split_on_char(remaining, ' ');
        ElkStrSplitPair p3 = elk_str_split_on_char(remaining, '\n');
        pair = (p2.left.len > 0 && p2.left.len <= p3.left.len) ? p2 : p3;
        token = elk_str_strip(pair.left);
        remaining = elk_str_strip(pair.right);
    }
    Assert(remaining.len == 0);

RETURN:

    return sndgs;
}

void 
sonde_sounding_fill_in_profiles(SondeSounding *snd, SondeProfileCode pcodes)
{
    /* These profiles are required, so turn them off. */
    // TODO: Once RH is added, dew point will not be required, and add the potential to calculate it!
    pcodes &= ~(SONDE_PC_PRESSURE | SONDE_PC_TEMPERATURE | SONDE_PC_DEW_POINT | SONDE_PC_GEOPOTENTIAL_HGT);

    if(sonde_profile_code_present(pcodes, SONDE_PC_WET_BULB) && !sonde_profile_code_present(snd->profiles, SONDE_PC_WET_BULB))
    {
        // TODO: add wet bulb profile
    }
    pcodes &= ~SONDE_PC_WET_BULB;

    if(sonde_profile_code_present(pcodes, SONDE_PC_VIRTUAL_TEMPERATURE) && !sonde_profile_code_present(snd->profiles, SONDE_PC_VIRTUAL_TEMPERATURE))
    {
        // TODO: add virtual temperature profile
    }
    pcodes &= ~SONDE_PC_VIRTUAL_TEMPERATURE;

    if(sonde_profile_code_present(pcodes, SONDE_PC_THETA) && !sonde_profile_code_present(snd->profiles, SONDE_PC_THETA))
    {
        // TODO: add potential temperature profile
    }
    pcodes &= ~SONDE_PC_THETA;

    if(sonde_profile_code_present(pcodes, SONDE_PC_THETA_E) && !sonde_profile_code_present(snd->profiles, SONDE_PC_THETA_E))
    {
        // TODO: add equivalent potential temperature profile
    }
    pcodes &= ~SONDE_PC_THETA_E;

    // TODO: add RH
    // TODO: add RH ice
    // TODO: add Frost point

    /* Check to make sure we got everything. */
    Assert(pcodes == 0);
}

void 
sonde_sounding_list_fill_in_profiles(SondeSoundingList *sndgs, SondeProfileCode pcodes)
{
    SondeSoundingList *current = sndgs;
    while(current)
    {
        sonde_sounding_fill_in_profiles(current->snd, pcodes);
        current = current->next;
    }
}

