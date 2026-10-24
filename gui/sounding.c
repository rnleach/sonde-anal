/* This file has everything for drawing the sounding area. */

#define P_MAX 1050.0f
#define P_MIN 98.0f

#define T_MAX 51.5f
#define T_MIN -46.5f

#define N_DP 200
#define DP ((P_MAX - P_MIN) / (f32)(N_DP - 1))

/* --------------------------------------------------------- API --------------------------------------------------------- */
static inline b32 sounding_initialize_static_data(AppState *app);
static inline b32 sounding_draw(AppState *app);
static inline void sounding_sample(AppState *app, SDL_FPoint loc);

/* ----------------------------------------------- Isopleth Configuration ------------------------------------------------ */

static f32 const isobar_width         = 1.0f;
static f32 const isotherm_width       = 1.0f;
static f32 const freezing_level_width = 2.5f;
static f32 const isentrop_width       = 1.0f;
static f32 const iso_theta_e_width    = 1.0f;
static f32 const isohume_width        = 1.0f;

static SDL_FColor const isobar_color                = { .r = 0.862745f, .g = 0.388235f, .b = 0.156863f, .a = 1.00f };
static SDL_FColor const isotherm_color              = { .r = 0.862745f, .g = 0.388235f, .b = 0.156863f, .a = 1.00f };
static SDL_FColor const isotherm_band_color         = { .r = 0.933333f, .g = 0.964705f, .b = 0.917647f, .a = 1.00f };
static SDL_FColor const freezing_level_color        = { .r = 0.0f,      .g = 0.466667f, .b = 0.780392f, .a = 1.00f };
static SDL_FColor const hail_growth_zone_color      = { .r = 0.0f,      .g = 0.803921f, .b = 0.803922f, .a = 0.55f };
static SDL_FColor const dendritic_growth_zone_color = { .r = 0.0f,      .g = 0.466667f, .b = 0.780393f, .a = 0.55f };
static SDL_FColor const isentrop_color              = { .r = 0.862745f, .g = 0.388235f, .b = 0.156863f, .a = 1.00f };
static SDL_FColor const iso_theta_e_color           = { .r = 0.333333f, .g = 0.662745f, .b = 0.278431f, .a = 1.00f };
static SDL_FColor const isohume_color               = { .r = 0.090196f, .g = 0.050980f, .b = 0.360784f, .a = 1.00f };

typedef struct
{
    SDL_FPoint p1;
    SDL_FPoint p2;
} SDL_FPoint_Pair;

/* Isobars configuration */
static f32 const isobars[] = {1000.0f, 925.0f, 850.0f, 700.0f, 500.0f, 300.0f, 200.0f, 100.0f, }; /* hPa */
static SDL_FPoint isobars_sdl[ECO_ARRAY_SIZE(isobars)] = {0};
static TTF_Text *isobar_labels[ECO_ARRAY_SIZE(isobars)] = {0};

/* Isotherms configuration */
static f32 const isotherms[] = /* °C */
    {
          -150.0f, -140.0f, -130.0f, -120.0f, -110.0f, -100.0f, -90.0f, -80.0f, -70.0f, -60.0f, -50.0f,
           -40.0f,  -30.0f,  -20.0f,  -10.0f,    0.0f,    5.0f,  10.0f,  15.0f,  20.0f,  25.0f,  30.0f,
            35.0f,   40.0f,   45.0f,   50.0f,
    };

static SDL_FPoint_Pair isotherms_sdl[ECO_ARRAY_SIZE(isotherms)] = {0};
static SDL_FPoint hail_growth_zone_points_sdl[4] = {0};
static SDL_FPoint dendritic_growth_zone_points_sdl[4] = {0};
static TTF_Text *isotherm_labels[ECO_ARRAY_SIZE(isotherms)] = {0};

/* Isentrops configuration */
static f32 const isentrops[] = /* Kelvin */
    {
        230.0f, 240.0f, 250.0f, 260.0f, 270.0f, 280.0f, 290.0f, 300.0f, 310.0f,
        320.0f, 330.0f, 340.0f, 350.0f, 360.0f, 370.0f, 380.0f, 390.0f,
    };
static SDL_FPoint isentrops_sdl[ECO_ARRAY_SIZE(isentrops)][N_DP] = {0};

/* Iso-equivalent potential temperature (theta-e) configuration */
static f32 const iso_theta_es[] = /* °C */
    {
        -20.0f, -18.0f, -16.0f, -14.0f, -12.0f, -10.0f,  -8.0f,  -6.0f,  -4.0f,  -2.0f,   0.0f,   2.0f,   4.0f,   6.0f,
          8.0f,  10.0f,  12.0f,  14.0f,  16.0f,  18.0f,  20.0f,  22.0f,  24.0f,  26.0f,  28.0f,  30.0f,  32.0f,  36.0f,
         40.0f,  42.0f,  44.0f,  46.0f,  48.0f,
    };
static SDL_FPoint iso_theta_es_sdl[ECO_ARRAY_SIZE(iso_theta_es)][N_DP] = {0};
static i32 iso_theta_es_max_pnts[ECO_ARRAY_SIZE(iso_theta_es)] = {0};


/* Isohume (mixing ratio) configuration */
static f32 const isohumes[] = /* g per kg */
    {
         0.125f,  0.25f,   0.5f,  0.75f,  1.00f,  1.50f,  2.00f,  2.50f,  3.00f,  4.00f,  5.00f,  6.00f,  7.00f,  8.00f,
         10.00f, 12.00f, 14.00f, 16.00f, 18.00f, 20.00f, 24.00f, 28.00f, 32.00f, 36.00f, 40.00f, 44.00f, 48.00f, 52.00f,
         56.00f, 60.00f, 68.00f, 76.00f,
    };
static SDL_FPoint_Pair isohumes_sdl[ECO_ARRAY_SIZE(isohumes)] = {0};

/* ------------------------------------------------- Data Configuration -------------------------------------------------- */

static f32 const temperature_width      =  2.0f;
static f32 const wet_bulb_width         =  1.0f;
static f32 const dew_point_width        =  2.0f;
static f32 const wind_barb_dot_width    =  7.0f;
static f32 const wind_barb_shaft_width  =  1.0f;
static f32 const wind_barb_shaft_length = 35.0f;
static f32 const wind_barb_barb_length  = 15.0f;

static SDL_FColor const temperature_color = { .r = 0.0f,      .g = 0.0f,      .b = 0.0f,      .a = 1.00f };
static SDL_FColor const wet_bulb_color    = { .r = 0.0f,      .g = 0.0f,      .b = 0.0f,      .a = 1.00f };
static SDL_FColor const dew_point_color   = { .r = 0.0f,      .g = 0.0f,      .b = 0.0f,      .a = 1.00f };
static SDL_FColor const wind_color        = { .r = 0.0f,      .g = 0.0f,      .b = 0.0f,      .a = 1.00f };
static SDL_FColor const rh_color          = { .r = 0.3f,      .g = 0.6f,      .b = 0.02f,     .a = 0.75f };
static SDL_FColor const rh_ice_color      = { .r = 0.0f,      .g = 0.6f,      .b = 0.6f,      .a = 1.50f };

/* ------------------------------------------------ Sample Configuration ------------------------------------------------- */
static f32 const sample_bar_width = 2.0f;

static SDL_FColor const sample_bar_color  = { .r = 0.95f,     .g = 0.0f,      .b = 0.0f,      .a = 1.00f };
static SDL_FColor const black             = { .r = 0.0f,      .g = 0.0f,      .b = 0.0f,      .a = 1.00f };

/* --------------------------------------------------- Implementations --------------------------------------------------- */

/* Utility functions for coordinate transforms. */
static inline f32
pressure_hpa_to_chart_y(SondeHectopascal p, ChartPanel *cp)
{
    /* TODO: Precalculate logf(P_MIN) and (logf(P_MAX) - logf(P_MIN)) and hardcode those in here. */
    return (logf(p.val) - logf(P_MIN)) / (logf(P_MAX) - logf(P_MIN));
}

static inline SDL_FPoint
pressure_hpa_temperature_c_to_chart_coords(SondeHectopascal p, SondeCelsius t, ChartPanel *cp)
{
    SDL_FPoint result = {0};

    result.y = pressure_hpa_to_chart_y(p, cp);
    f32 rise = (1.0 - result.y) * cp->ard;
    result.x = (t.val - T_MIN) / (T_MAX - T_MIN) + rise; 

    return result;
}

static inline void
chart_coords_to_pressure_hpa_temperature_c(SDL_FPoint pnt, ChartPanel *cp, SondeHectopascal *p, SondeCelsius *t)
{
    f32 y = pnt.y;
    f32 x = pnt.x;

    p->val = expf(y * (logf(P_MAX) - logf(P_MIN)) +  logf(P_MIN));
    t->val = (x - cp->ard*(1.0f - y)) * (T_MAX - T_MIN) + T_MIN;
}

static inline b32
sounding_update_static_data(ChartPanel *cp)
{
    /* Calculate the screen coordinates of the isobars. */
    for(i32 i = 0; i < ECO_ARRAY_SIZE(isobars); ++i)
    {
        f32 chart_y = pressure_hpa_to_chart_y((SondeHectopascal) { .val = isobars[i]}, cp);
        SDL_FPoint chart_coords = { .x = 0.0f, .y = chart_y };
        chart_coords = chart_panel_apply_pan_and_zoom(chart_coords, cp);
        SDL_FPoint screen_coords = chart_panel_convert_to_screen_coords(chart_coords, cp);

        isobars_sdl[i] = screen_coords;
    }


    /* Find pressure bounds. */
    SDL_FPoint lower_right = { .x = cp->area.x + cp->area.w, .y = cp->area.y + cp->area.h };
    SDL_FPoint upper_left = { .x = cp->area.x, .y = cp->area.y };

    lower_right = chart_panel_convert_from_screen_coords(lower_right, cp);
    upper_left = chart_panel_convert_from_screen_coords(upper_left, cp);

    lower_right = chart_panel_unapply_pan_and_zoom(lower_right, cp);
    upper_left = chart_panel_unapply_pan_and_zoom(upper_left, cp);

    lower_right.x = lower_right.x < 0.0f ? 0.0f : lower_right.x;
    lower_right.y = lower_right.y < 0.0f ? 0.0f : lower_right.y;
    upper_left.x = upper_left.x < 0.0f ? 0.0f : upper_left.x;
    upper_left.y = upper_left.y < 0.0f ? 0.0f : upper_left.y;
    lower_right.x = lower_right.x > 1.0f ? 1.0f : lower_right.x;
    lower_right.y = lower_right.y > 1.0f ? 1.0f : lower_right.y;
    upper_left.x = upper_left.x > 1.0f ? 1.0f : upper_left.x;
    upper_left.y = upper_left.y > 1.0f ? 1.0f : upper_left.y;

    SondeHectopascal p_max = {0};
    SondeHectopascal p_min = {0};
    SondeCelsius dummy = {0};

    chart_coords_to_pressure_hpa_temperature_c(lower_right, cp, &p_max, &dummy);
    chart_coords_to_pressure_hpa_temperature_c(upper_left, cp, &p_min, &dummy);

    p_max.val = p_max.val > P_MAX ? P_MAX : p_max.val;
    p_min.val = p_min.val < P_MIN ? P_MIN : p_min.val;

    /* Calculate the screen coordinates of the isotherms. */
    for(i32 i = 0; i < ECO_ARRAY_SIZE(isotherms); ++i)
    {
        SDL_FPoint chart_p1 = pressure_hpa_temperature_c_to_chart_coords(p_max, (SondeCelsius){ .val = isotherms[i] }, cp);
        SDL_FPoint chart_p2 = pressure_hpa_temperature_c_to_chart_coords(p_min, (SondeCelsius){ .val = isotherms[i] }, cp);

        chart_p1 = chart_panel_apply_pan_and_zoom(chart_p1, cp);
        chart_p2 = chart_panel_apply_pan_and_zoom(chart_p2, cp);

        SDL_FPoint screen_p1 = chart_panel_convert_to_screen_coords(chart_p1, cp);
        SDL_FPoint screen_p2 = chart_panel_convert_to_screen_coords(chart_p2, cp);

        isotherms_sdl[i] = (SDL_FPoint_Pair){ .p1 = screen_p1, .p2 = screen_p2 };
    }

    /* Calculate the screen coordinates of the hail growth zone */
    {
        SDL_FPoint chart_p1 = pressure_hpa_temperature_c_to_chart_coords(p_max, (SondeCelsius){ .val = -10.0f }, cp);
        SDL_FPoint chart_p2 = pressure_hpa_temperature_c_to_chart_coords(p_min, (SondeCelsius){ .val = -10.0f }, cp);
        SDL_FPoint chart_p3 = pressure_hpa_temperature_c_to_chart_coords(p_min, (SondeCelsius){ .val = -30.0f }, cp);
        SDL_FPoint chart_p4 = pressure_hpa_temperature_c_to_chart_coords(p_max, (SondeCelsius){ .val = -30.0f }, cp);

        chart_p1 = chart_panel_apply_pan_and_zoom(chart_p1, cp);
        chart_p2 = chart_panel_apply_pan_and_zoom(chart_p2, cp);
        chart_p3 = chart_panel_apply_pan_and_zoom(chart_p3, cp);
        chart_p4 = chart_panel_apply_pan_and_zoom(chart_p4, cp);

        SDL_FPoint screen_p1 = chart_panel_convert_to_screen_coords(chart_p1, cp);
        SDL_FPoint screen_p2 = chart_panel_convert_to_screen_coords(chart_p2, cp);
        SDL_FPoint screen_p3 = chart_panel_convert_to_screen_coords(chart_p3, cp);
        SDL_FPoint screen_p4 = chart_panel_convert_to_screen_coords(chart_p4, cp);

        hail_growth_zone_points_sdl[0] = screen_p1;
        hail_growth_zone_points_sdl[1] = screen_p2;
        hail_growth_zone_points_sdl[2] = screen_p3;
        hail_growth_zone_points_sdl[3] = screen_p4;
    }

    /* Calculate the screen coordinates of the dendritic growth zone */
    {
        SDL_FPoint chart_p1 = pressure_hpa_temperature_c_to_chart_coords(p_max, (SondeCelsius){ .val = -12.0f }, cp);
        SDL_FPoint chart_p2 = pressure_hpa_temperature_c_to_chart_coords(p_min, (SondeCelsius){ .val = -12.0f }, cp);
        SDL_FPoint chart_p3 = pressure_hpa_temperature_c_to_chart_coords(p_min, (SondeCelsius){ .val = -18.0f }, cp);
        SDL_FPoint chart_p4 = pressure_hpa_temperature_c_to_chart_coords(p_max, (SondeCelsius){ .val = -18.0f }, cp);

        chart_p1 = chart_panel_apply_pan_and_zoom(chart_p1, cp);
        chart_p2 = chart_panel_apply_pan_and_zoom(chart_p2, cp);
        chart_p3 = chart_panel_apply_pan_and_zoom(chart_p3, cp);
        chart_p4 = chart_panel_apply_pan_and_zoom(chart_p4, cp);

        SDL_FPoint screen_p1 = chart_panel_convert_to_screen_coords(chart_p1, cp);
        SDL_FPoint screen_p2 = chart_panel_convert_to_screen_coords(chart_p2, cp);
        SDL_FPoint screen_p3 = chart_panel_convert_to_screen_coords(chart_p3, cp);
        SDL_FPoint screen_p4 = chart_panel_convert_to_screen_coords(chart_p4, cp);

        dendritic_growth_zone_points_sdl[0] = screen_p1;
        dendritic_growth_zone_points_sdl[1] = screen_p2;
        dendritic_growth_zone_points_sdl[2] = screen_p3;
        dendritic_growth_zone_points_sdl[3] = screen_p4;
    }

    /* Calculate the screen coordinates of the isentrops */
    for(i32 i = 0; i < ECO_ARRAY_SIZE(isentrops); ++i)
    {
        SondeKelvin theta = { .val = isentrops[i] };
        for(i32 ip = 0; ip < N_DP; ++ip)
        {
            SondeHectopascal p = { .val = P_MIN + ip * DP };
            SondeCelsius tc = sonde_temperature_from_pot_temperature(theta, p);
            SDL_FPoint chart_point = pressure_hpa_temperature_c_to_chart_coords(p, tc, cp);
            chart_point = chart_panel_apply_pan_and_zoom(chart_point, cp);
            chart_point = chart_panel_convert_to_screen_coords(chart_point, cp);
            isentrops_sdl[i][ip] = chart_point;
        }
    }

    /* Calculate the screen coordinates of the iso-theta-e lines */
    f32 const theta_e_p_min = 200.0f;
    f32 const theta_e_dp = (P_MAX - theta_e_p_min) / (f32)(N_DP - 1);
    for(i32 i = 0; i < ECO_ARRAY_SIZE(iso_theta_es); ++i)
    {
        i32 n_pnts = 0;
        SondeCelsius theta_e_c = { .val = iso_theta_es[i] };
        SondeKelvin theta_e_k = sonde_equivalent_potential_temperature(theta_e_c, theta_e_c, (SondeHectopascal){ .val = 1000.0 });
        for(i32 ip = 0; ip < N_DP; ++ip)
        {
            SondeHectopascal p = { .val = P_MAX - ip * theta_e_dp };
            SondeCelsius tc = sonde_temperature_from_equiv_pot_temp_saturated_pressure(p, theta_e_k);

            if(isnan(tc.val)) { break; }
            n_pnts += 1;

            SDL_FPoint chart_point = pressure_hpa_temperature_c_to_chart_coords(p, tc, cp);
            chart_point = chart_panel_apply_pan_and_zoom(chart_point, cp);
            chart_point = chart_panel_convert_to_screen_coords(chart_point, cp);
            iso_theta_es_sdl[i][ip] = chart_point;
        }

        iso_theta_es_max_pnts[i] = n_pnts;
    }

    /* Calculate the screen coordinates of the isohumes */
    SondeHectopascal p_min_isohumes = { .val = 300.0f };
    for(i32 i = 0; i < ECO_ARRAY_SIZE(isohumes); ++i)
    {
        f32 mw = isohumes[i] / 1000.0; /* convert from g/kg to unitless */
        SondeCelsius tc_pmax = sonde_dew_point_from_p_and_mw(p_max, mw);
        SondeCelsius tc_pmin = sonde_dew_point_from_p_and_mw(p_min_isohumes, mw);

        SDL_FPoint chart_p1 = pressure_hpa_temperature_c_to_chart_coords(p_max, tc_pmax, cp);
        SDL_FPoint chart_p2 = pressure_hpa_temperature_c_to_chart_coords(p_min_isohumes, tc_pmin, cp);

        chart_p1 = chart_panel_apply_pan_and_zoom(chart_p1, cp);
        chart_p2 = chart_panel_apply_pan_and_zoom(chart_p2, cp);

        SDL_FPoint screen_p1 = chart_panel_convert_to_screen_coords(chart_p1, cp);
        SDL_FPoint screen_p2 = chart_panel_convert_to_screen_coords(chart_p2, cp);

        isohumes_sdl[i] = (SDL_FPoint_Pair){ .p1 = screen_p1, .p2 = screen_p2 };
    }

    return true;
}

static inline b32
sounding_initialize_static_data(AppState *app)
{
    b32 success = true;

    /* Set up isobars for the Skew-T area */
    for(i32 i = 0; i < ECO_ARRAY_SIZE(isobars); ++i)
    {
        char cbuf[32] = {0};
        snprintf(cbuf, sizeof(cbuf), "%.0f hPa", isobars[i]);
        isobar_labels[i] = TTF_CreateText(app->text_engine, app->small_font, cbuf, 0);
        TTF_SetTextColorFloat(isobar_labels[i], isobar_color.r, isobar_color.g, isobar_color.b, isobar_color.a);
    }

    /* Set up isotherms for the Skew-T area */
    for(i32 i = 0; i < ECO_ARRAY_SIZE(isotherms); ++i)
    {
        char cbuf[32] = {0};
        snprintf(cbuf, sizeof(cbuf), "%.0f °C", isotherms[i]);
        isotherm_labels[i] = TTF_CreateText(app->text_engine, app->small_font, cbuf, 0);
        TTF_SetTextColorFloat(isotherm_labels[i], isotherm_color.r, isotherm_color.g, isotherm_color.b, isotherm_color.a);
    }

    app->skewt.needs_background_update = true;

    return success;
}

static inline b32
sounding_draw_background(AppState *app, MagAllocator alloc_)
{
    ChartPanel *cp = &app->skewt;
    b32 success = true;

    f32 w = (f32)cp->chart_w;

    /* Draw the filled bands in the background. */
    for(i32 i = 0; i < ECO_ARRAY_SIZE(isotherms) - 1; ++i)
    {
        if(fmodf(isotherms[i], 20.0f) == 0.0f)
        {
            SDL_FPoint_Pair pair1 = isotherms_sdl[i + 0];
            SDL_FPoint_Pair pair2;

            i32 j;
            for(j = i + 1; j < ECO_ARRAY_SIZE(isotherms); ++j)
            {
                if(fabsf(isotherms[j] - isotherms[i] - 10.0f) < 0.01f) { pair2 = isotherms_sdl[j]; }
            }
            if(j > ECO_ARRAY_SIZE(isotherms)) { break; }

            SDL_FPoint vs[4] = { pair1.p1, pair1.p2, pair2.p2, pair2.p1 };

            DrawPolygonThick(app->renderer, vs, ECO_ARRAY_SIZE(vs), 0.0f, isotherm_band_color, isotherm_band_color, alloc_);
        }
    }

    /* Draw the hail growth zone */
    if(app->show_hail_growth_zone)
    {
        success &= DrawPolygonThick(
                app->renderer,
                hail_growth_zone_points_sdl,
                ECO_ARRAY_SIZE(hail_growth_zone_points_sdl),
                0.0f,
                hail_growth_zone_color,
                hail_growth_zone_color,
                alloc_);
        Assert(success);
    }

    /* Draw the dendritic growth zone */
    if(app->show_dendritic_growth_zone)
    {
        success &= DrawPolygonThick(
                app->renderer,
                dendritic_growth_zone_points_sdl,
                ECO_ARRAY_SIZE(dendritic_growth_zone_points_sdl),
                0.0f,
                dendritic_growth_zone_color,
                dendritic_growth_zone_color,
                alloc_);
        Assert(success);
    }

    /* Draw the isohumes */
    if(app->show_isohumes)
    {
        for(i32 i = 0; i < ECO_ARRAY_SIZE(isohumes); ++i)
        {
            SDL_FPoint p1 = isohumes_sdl[i].p1;
            SDL_FPoint p2 = isohumes_sdl[i].p2;
            success &= DrawLineThickDashedAA(app->renderer, p1, p2, isohume_width, 15.0f, 15.0f, isohume_color); 
            Assert(success);
        }
    }

    /* Draw the iso_theta_es */
    if(app->show_equivalent_pot_temp)
    {
        for(i32 i = 0; i < ECO_ARRAY_SIZE(iso_theta_es); ++i)
        {
            success &= DrawPolylineThickAA(
                    app->renderer,
                    &iso_theta_es_sdl[i][0],
                    iso_theta_es_max_pnts[i],
                    iso_theta_e_width,
                    iso_theta_e_color,
                    alloc_);
            Assert(success);
        }
    }

    /* Draw the isentropes. */
    for(i32 i = 0; i < ECO_ARRAY_SIZE(isentrops); ++i)
    {
        success &= DrawPolylineThickAA(
                app->renderer,
                &isentrops_sdl[i][0],
                N_DP,
                isentrop_width,
                isentrop_color,
                alloc_);
        Assert(success);
    }

    /* Draw the isobars. */
    for(i32 i = 0; i < ECO_ARRAY_SIZE(isobars); ++i)
    {
        SDL_FPoint p1 = {.x = cp->x_offset,     .y = isobars_sdl[i].y };
        SDL_FPoint p2 = {.x = w + cp->x_offset, .y = isobars_sdl[i].y };
        success &= DrawLineThickRoundedAA(app->renderer, p1, p2, isobar_width, isobar_color); Assert(success);
    }

    /* Draw the isotherms. */
    for(i32 i = 0; i < ECO_ARRAY_SIZE(isotherms); ++i)
    {
        SDL_FColor color = isotherms[i] == 0.0f && app->show_freezing_level ? freezing_level_color : isotherm_color;
        f32 width = isotherms[i] == 0.0f && app->show_freezing_level ? freezing_level_width : isotherm_width;

        SDL_FPoint_Pair pair = isotherms_sdl[i];
        success &= DrawLineThickRoundedAA(app->renderer, pair.p1, pair.p2, width, color); Assert(success);
    }

    /* ---------- Do a second pass to draw the labels on top of everything else in the background. */

    /* Label the isobars */
    f32 const padding = 3.0f;
    for(i32 i = 0; i < ECO_ARRAY_SIZE(isobars); ++i)
    {
        SDL_FPoint p1 = {.x = cp->x_offset, .y = isobars_sdl[i].y };

        i32 tw, th;
        TTF_GetTextSize(isobar_labels[i], &tw, &th);
        SDL_FRect trect = { .x = p1.x + 5.0f, .y = p1.y - (f32)(th /2) - padding, .w = (f32)(tw) + 2.0f * padding, .h = (f32)(th) + 2.0f * padding };
        SDL_SetRenderDrawColorFloat(app->renderer, app->background.r, app->background.g, app->background.b, app->background.a);
        SDL_RenderFillRect(app->renderer, &trect);
        SDL_SetRenderDrawColorFloat(app->renderer, isobar_color.r, isobar_color.g, isobar_color.b, isobar_color.a);
        TTF_DrawRendererText(isobar_labels[i], p1.x + 5.0f + padding, p1.y - (f32)(th / 2));
    }

    /* Label the isotherms. */
    for(i32 i = 0; i < ECO_ARRAY_SIZE(isotherms); ++i)
    {
        SDL_FPoint_Pair pair = isotherms_sdl[i];
        SDL_FPoint p1 = pair.p1;
        SDL_FPoint p2 = pair.p2;

        i32 tw, th;
        TTF_GetTextSize(isotherm_labels[i], &tw, &th);

        SDL_FRect trect1 = { .x = p1.x - padding,             .y = p1.y - (f32)(th) - padding, .w = (f32)(tw) + 2.0f * padding, .h = (f32)(th) + 2.0f * padding };
        SDL_FRect trect2 = { .x = p2.x - (f32)(tw) - padding, .y = p2.y - padding,             .w = (f32)(tw) + 2.0f * padding, .h = (f32)(th) + 2.0f * padding };

        SDL_SetRenderDrawColorFloat(app->renderer, app->background.r, app->background.g, app->background.b, app->background.a);
        SDL_RenderFillRect(app->renderer, &trect1);
        SDL_RenderFillRect(app->renderer, &trect2);
        SDL_SetRenderDrawColorFloat(app->renderer, isotherm_color.r, isotherm_color.g, isotherm_color.b, isotherm_color.a);
        TTF_DrawRendererText(isotherm_labels[i], trect1.x + padding, trect1.y + padding);
        TTF_DrawRendererText(isotherm_labels[i], trect2.x + padding, trect2.y + padding);
    }

    return success;
}

static inline b32
sounding_draw_temperature_dew_point_wet_bulb(AppState *app, MagAllocator alloc_)
{
    ChartPanel *cp = &app->skewt;
    MagAllocator *alloc = &alloc_;

    b32 success = true;

    if(app->current_sndg >= 0)
    {
        SondeSounding *snd = app->sndg_amalgams[app->current_sndg].sndg;

        /* Draw all at once! */
        PakArrayLedger t_ledger = pak_array_ledger_create(pak_len(&snd->levels));
        SDL_FPoint *t_pnts = eco_arena_nmalloc(alloc, pak_len(&snd->levels), SDL_FPoint); Assert(t_pnts);

        PakArrayLedger dp_ledger = pak_array_ledger_create(pak_len(&snd->levels));
        SDL_FPoint *dp_pnts = eco_arena_nmalloc(alloc, pak_len(&snd->levels), SDL_FPoint); Assert(dp_pnts);

        PakArrayLedger wb_ledger = pak_array_ledger_create(pak_len(&snd->levels));
        SDL_FPoint *wb_pnts = eco_arena_nmalloc(alloc, pak_len(&snd->levels), SDL_FPoint); Assert(wb_pnts);

        for(size l = 0; l < pak_len(&snd->levels); ++l)
        {
            SondeHectopascal p = snd->p[l];

            if(!isnan(p.val))
            {
                SondeCelsius t = snd->t[l];
                if(!isnan(t.val))
                {
                    SDL_FPoint pnt = pressure_hpa_temperature_c_to_chart_coords(p, t, cp);
                    pnt = chart_panel_apply_pan_and_zoom(pnt, cp);
                    pnt = chart_panel_convert_to_screen_coords(pnt, cp);

                    size idx = pak_array_ledger_push_back_index(&t_ledger); 
                    t_pnts[idx] = pnt;
                }

                SondeCelsius dp = snd->dp[l];
                if(!isnan(dp.val))
                {
                    SDL_FPoint pnt = pressure_hpa_temperature_c_to_chart_coords(p, dp, cp);
                    pnt = chart_panel_apply_pan_and_zoom(pnt, cp);
                    pnt = chart_panel_convert_to_screen_coords(pnt, cp);

                    size idx = pak_array_ledger_push_back_index(&dp_ledger); 
                    dp_pnts[idx] = pnt;
                }

                SondeCelsius wb = snd->wb[l];
                if(app->show_wet_bulb && !isnan(wb.val))
                {
                    SDL_FPoint pnt = pressure_hpa_temperature_c_to_chart_coords(p, wb, cp);
                    pnt = chart_panel_apply_pan_and_zoom(pnt, cp);
                    pnt = chart_panel_convert_to_screen_coords(pnt, cp);

                    size idx = pak_array_ledger_push_back_index(&wb_ledger); 
                    wb_pnts[idx] = pnt;
                }
            }
        }

        if(!pak_array_ledger_empty(&t_ledger))
        {
            success &= DrawPolylineThickAA(app->renderer, t_pnts, pak_len(&t_ledger), temperature_width, temperature_color, alloc_); Assert(success);
        }

        if(!pak_array_ledger_empty(&dp_ledger))
        {
            success &= DrawPolylineThickAA(app->renderer, dp_pnts, pak_len(&dp_ledger), dew_point_width, dew_point_color, alloc_); Assert(success);
        }

        if(!pak_array_ledger_empty(&wb_ledger))
        {
            success &= DrawPolylineThickAA(app->renderer, wb_pnts, pak_len(&wb_ledger), wet_bulb_width, wet_bulb_color, alloc_); Assert(success);
        }
    }

    return success;
}

static inline b32
sounding_draw_legend(AppState *app, MagAllocator alloc_)
{
    static char const *doy[] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
    b32 success = true;

    if(app->current_sndg >= 0)
    {
        f32 const padding = 6.0f;
        f32 const border_width = 3.0f;

        SondeSounding *snd = app->sndg_amalgams[app->current_sndg].sndg;
        SondeStationInfo *stn = &snd->stn_info;

        SDL_FPoint legend_ul = { .x = 10.0f, .y = 70.0f };
        SDL_FPoint legend_lr = { .x = legend_ul.x + padding, .y = legend_ul.y + padding };
        i32 tw, th;

        ElkStr source = snd->source;
        ElkStr source_truncated = { .start = source.start + source.len - 1, .len = 1 };
        char *c = source_truncated.start;
        while(*c != '/' && *c != '\\' && c >= source.start) { c--; source_truncated.len++; }
        source_truncated.len--;
        source_truncated.start = c + 1;
        TTF_Text *source_txt = TTF_CreateText(app->text_engine, app->large_font, source_truncated.start, source_truncated.len);
        TTF_SetTextColorFloat(source_txt, isobar_color.r, isobar_color.g, isobar_color.b, isobar_color.a);
        TTF_GetTextSize(source_txt, &tw, &th);
        SDL_FPoint source_txt_start = legend_lr;
        legend_lr.y += (f32)th;
        legend_lr.x = legend_ul.x + 2.0f * padding + (f32)tw < legend_lr.x ? legend_lr.x : legend_ul.x + 2.0f * padding +(f32)tw;

        char loc_str[64] = {0};
        snprintf(loc_str, sizeof(loc_str), "%.*s (%.2lf, %.2lf)", (int)stn->id.len, stn->id.start, stn->latitude, stn->longitude);
        TTF_Text *loc_txt = TTF_CreateText(app->text_engine, app->large_font, loc_str, 0);
        TTF_SetTextColorFloat(loc_txt, isobar_color.r, isobar_color.g, isobar_color.b, isobar_color.a);
        TTF_GetTextSize(loc_txt, &tw, &th);
        SDL_FPoint loc_txt_start = { .x = legend_ul.x + padding, .y = legend_lr.y};
        legend_lr.y += (f32)th;
        legend_lr.x = legend_ul.x + 2.0f * padding + (f32)tw < legend_lr.x ? legend_lr.x : legend_ul.x + 2.0f * padding +(f32)tw;

        char date_str[42] = {0};
        ElkStructTime vt = elk_make_struct_time(snd->valid_time);
        snprintf(date_str, sizeof(date_str), "%s %02d/%02d/%4d %02dZ F%03d",
                doy[vt.day_of_week], vt.month, vt.day, vt.year, vt.hour, snd->lead_time);
        TTF_Text *date_txt = TTF_CreateText(app->text_engine, app->large_font, date_str, 0);
        TTF_SetTextColorFloat(date_txt, isobar_color.r, isobar_color.g, isobar_color.b, isobar_color.a);
        TTF_GetTextSize(date_txt, &tw, &th);
        SDL_FPoint date_txt_start = { .x = legend_ul.x + padding, .y = legend_lr.y};
        legend_lr.y += (f32)th;
        legend_lr.x = legend_ul.x + 2.0f * padding + (f32)tw < legend_lr.x ? legend_lr.x : legend_ul.x + 2.0f * padding +(f32)tw;

        legend_lr.y += padding;

        SDL_FPoint border[4] = {0};
        border[0] = legend_ul;
        border[1] = (SDL_FPoint){ .x = legend_ul.x, .y = legend_lr.y };
        border[2] = legend_lr;
        border[3] = (SDL_FPoint){ .x = legend_lr.x, .y = legend_ul.y };

        success &= DrawPolygonThick(app->renderer, border, ECO_ARRAY_SIZE(border), border_width, isobar_color, app->background, alloc_);

        TTF_DrawRendererText(source_txt, source_txt_start.x, source_txt_start.y);
        TTF_DrawRendererText(loc_txt, loc_txt_start.x, loc_txt_start.y);
        TTF_DrawRendererText(date_txt, date_txt_start.x, date_txt_start.y);

        TTF_DestroyText(source_txt);
        TTF_DestroyText(loc_txt);
        TTF_DestroyText(date_txt);
    }

    return success;
}

#define N_BARB_SLOTS 5
typedef struct
{
    SDL_FPoint center;
    SDL_FPoint tail;
    SDL_FPoint penants[N_BARB_SLOTS][3];
    i8 n_penants;
    i8 n_barbs;
    i8 n_half_barbs;
} WindBarb;

static inline WindBarb
sounding_draw_wind_create_barb(SondeUVMps w)
{
    f32 speed_kts = sqrt(w.u * w.u + w.v * w.v) * 1.9438444924406048;
    i32 speed = (i32)roundf(speed_kts);

    WindBarb b = {0};
    b.n_penants = (speed / 50) & 0xFF;
    b.n_barbs = ((speed - b.n_penants * 50) / 10) &0xFF;
    b.n_half_barbs = ((speed - b.n_penants * 50 - b.n_barbs * 10) / 5) & 0xFF;

    b.tail.x = -wind_barb_shaft_length;

    /* Calculate Penants And Barbs */
    f32 x = -wind_barb_shaft_length;
    
    i32 i = 0;
    for(; i < b.n_penants && i < N_BARB_SLOTS; ++i)
    {
        b.penants[i][0] = (SDL_FPoint){ .x = x + 0.0f, .y =  0.0f };
        b.penants[i][1] = (SDL_FPoint){ .x = x + 0.0f, .y = 15.0f };
        b.penants[i][2] = (SDL_FPoint){ .x = x + 5.0f, .y =  0.0f };
        x += 7.0f;
    }

    for(; i < b.n_penants + b.n_barbs && i < N_BARB_SLOTS; ++i)
    {
        b.penants[i][0] = (SDL_FPoint){ .x = x + 0.0f, .y =  0.0f };
        b.penants[i][1] = (SDL_FPoint){ .x = x + 0.0f, .y = 15.0f };
        x += 7.0f;
    }

    for(; i < b.n_penants + b.n_barbs + b.n_half_barbs && i < N_BARB_SLOTS; ++i)
    {
        b.penants[i][0] = (SDL_FPoint){ .x = x + 0.0f, .y = 0.0f };
        b.penants[i][1] = (SDL_FPoint){ .x = x + 0.0f, .y = 7.5f };
        x += 7.0f;
    }

    return b;
}

static inline SDL_FPoint
sounding_draw_wind_rotate_point(SDL_FPoint p, f32 u, f32 v)
{
    SDL_FPoint pr = p;
    f32 hypot = sqrtf(u *u + v * v);

    f32 s = v / hypot;
    f32 c = u / hypot;
    pr.x = p.x * c - p.y * s;
    pr.y = -(p.x * s + p.y * c);
    
    return pr;
}

static inline WindBarb
sounding_draw_wind_rotate_and_translate(WindBarb b, SondeUVMps w, SondeHectopascal p, ChartPanel *cp, f32 *ymax, f32 *ymin)
{
    WindBarb b2 = b;
    *ymax = cp->clipping.y;
    *ymin = cp->clipping.y + cp->clipping.h;

    /* Get the location to plot the point */
    SDL_FPoint loc = pressure_hpa_temperature_c_to_chart_coords(p, (SondeCelsius){ .val = 0.0f }, cp);
    loc = chart_panel_apply_pan_and_zoom(loc, cp);
    loc = chart_panel_convert_to_screen_coords(loc, cp);
    loc.x = cp->clipping.x + cp->clipping.w - wind_barb_shaft_length - 10.0;

    /* Rotate and translate each point */
    b2.center.x += loc.x; b2.center.y += loc.y;
    *ymax = b2.center.y < *ymax ? *ymax : b2.center.y;
    *ymin = b2.center.y > *ymin ? *ymin : b2.center.y;

    b2.tail = sounding_draw_wind_rotate_point(b.tail, w.u, w.v);
    b2.tail.x += loc.x; b2.tail.y += loc.y;
    *ymax = b2.tail.y < *ymax ? *ymax : b2.tail.y;
    *ymin = b2.tail.y > *ymin ? *ymin : b2.tail.y;

    SDL_FPoint *arr = &b2.penants[0][0];
    for(i32 i = 0; i < sizeof(b.penants) / sizeof(b.penants[0][0]); ++i)
    {
        arr[i] = sounding_draw_wind_rotate_point(arr[i], w.u, w.v);
        arr[i].x += loc.x; arr[i].y += loc.y; 
        *ymax = arr[i].y < *ymax ? *ymax : arr[i].y;
        *ymin = arr[i].y > *ymin ? *ymin : arr[i].y;
    }

    return b2;
}

static inline b32
sounding_draw_wind_draw_barb(AppState *app, WindBarb b, MagAllocator alloc_)
{
    b32 success = true;

    success &= DrawPointRoundAA(app->renderer, b.center, wind_barb_dot_width, wind_color);
    success &= DrawLineThickRoundedAA(app->renderer, b.center, b.tail, wind_barb_shaft_width, wind_color);

    i32 i = 0;
    for(; i < b.n_penants && i < N_BARB_SLOTS; ++i)
    {
        success &= DrawPolygonThick(app->renderer, &b.penants[i][0], 3, 1.0f, wind_color, wind_color, alloc_); Assert(success);
    }

    for(; i < b.n_penants + b.n_barbs + b.n_half_barbs && i < N_BARB_SLOTS; ++i)
    {
        success &= DrawLineThickRoundedAA(app->renderer, b.penants[i][0], b.penants[i][1], wind_barb_shaft_width, wind_color); Assert(success);
    }

    return success;
}
#undef N_BARB_SLOTS

static inline b32
sounding_draw_wind(AppState *app, MagAllocator alloc_)
{
    ChartPanel *cp = &app->skewt;

    b32 success = true;

    if(app->current_sndg >= 0)
    {
        SondeSounding *snd = app->sndg_amalgams[app->current_sndg].sndg;

        f32 last_min_y = cp->clipping.y + cp->clipping.h;

        for(size l = 0; l < pak_len(&snd->levels); ++l)
        {
            SondeHectopascal p = snd->p[l];
            SondeUVMps w = snd->uv_wind[l];

            if(!isnan(p.val) && !isnan(w.u) && !isnan(w.v))
            {
                /* Some variables for tracking overlap and thinning. */
                f32 max_y;
                f32 min_y;

                WindBarb b = sounding_draw_wind_create_barb(w);
                b = sounding_draw_wind_rotate_and_translate(b, w, p, cp, &max_y, &min_y);

                if(max_y < last_min_y) /* Only draw if it doesn't overlap the one below it. */
                {
                    success &= sounding_draw_wind_draw_barb(app, b, alloc_); Assert(success);
                    last_min_y = min_y;
                }
            }
        }
    }

    return success;
}

static inline b32
sounding_draw_sample(AppState *app, MagAllocator alloc_)
{
    ChartPanel *cp = &app->skewt;
    b32 success = true;

    b32 do_draw_sample =
        app->sample.type == SAMPLE_TYPE_SKEWT        && 
        chart_panel_point_inside(cp, app->mouse_loc) &&
        app->current_sndg >= 0                       &&
        app->sndg_amalgams;

    if(do_draw_sample)
    {
        SondeSounding *snd = app->sndg_amalgams[app->current_sndg].sndg;

        SDL_FPoint p1 = app->mouse_loc; p1.x = (f32)app->skewt.clipping.x;
        SDL_FPoint p2 = app->mouse_loc; p2.x = (f32)(app->skewt.clipping.x + app->skewt.clipping.w);
        success &= DrawLineThickRoundedAA(app->renderer, p1, p2, sample_bar_width, sample_bar_color); Assert(success);

        f32 const padding = 4.0f;
        f32 const border_width = 3.0f;

        SDL_FPoint sample_ul = { .x = 10.0f, .y = p1.y };
        SDL_FPoint sample_lr = { .x = sample_ul.x + padding, .y = sample_ul.y + padding };
        i32 tw, th;

        char str[64] = {0};

        /* Temperatures, humidity, and pressure vertical velocity Line. */
        SondeCelsius t_c = app->sample.row.t;
        SondeCelsius dp_c = app->sample.row.dp;
        f64 rh = app->sample.row.rh * 100.0f;
        f64 rh_ice = app->sample.row.rh_ice * 100.0f;
        SondeHectopascalPerSecond pvv = app->sample.row.pvv;

        f32 width = 0.0f;
        snprintf(str, sizeof(str), "%.0lf°C/", t_c.val); 
        TTF_Text *t_txt = TTF_CreateText(app->text_engine, app->small_font, str, 0);
        TTF_SetTextColorFloat(t_txt, temperature_color.r, temperature_color.g, temperature_color.b, temperature_color.a);
        TTF_GetTextSize(t_txt, &tw, &th);
        SDL_FPoint t_txt_start = { .x = sample_ul.x + padding, .y = sample_lr.y};
        width += tw;

        if(!isnan(dp_c.val))
        {
            snprintf(str, sizeof(str), "%.0lf°C", dp_c.val); 
        }
        else
        {
            snprintf(str, sizeof(str), "---°C "); 
        }
        TTF_Text *dp_txt = TTF_CreateText(app->text_engine, app->small_font, str, 0);
        TTF_SetTextColorFloat(dp_txt, dew_point_color.r, dew_point_color.g, dew_point_color.b, dew_point_color.a);
        TTF_GetTextSize(dp_txt, &tw, &th);
        SDL_FPoint dp_txt_start = { .x = sample_ul.x + width + padding, .y = sample_lr.y};
        width += tw;

        if(!isnan(rh))
        {
            snprintf(str, sizeof(str), "  %.0lf%%", rh);
        }
        else { str[0] = 0; }
        TTF_Text *rh_txt = TTF_CreateText(app->text_engine, app->small_font, str, 0);
        TTF_SetTextColorFloat(rh_txt, rh_color.r, rh_color.g, rh_color.b, rh_color.a);
        TTF_GetTextSize(rh_txt, &tw, &th);
        SDL_FPoint rh_txt_start = { .x = sample_ul.x + width + padding, .y = sample_lr.y};
        width += tw;

        if(!isnan(rh_ice))
        {
            snprintf(str, sizeof(str), "/%.0lf%%", rh_ice);
        }
        else { str[0] = 0; }
        TTF_Text *rh_ice_txt = TTF_CreateText(app->text_engine, app->small_font, str, 0);
        TTF_SetTextColorFloat(rh_ice_txt, rh_ice_color.r, rh_ice_color.g, rh_ice_color.b, rh_ice_color.a);
        TTF_GetTextSize(rh_ice_txt, &tw, &th);
        SDL_FPoint rh_ice_txt_start = { .x = sample_ul.x + width + padding, .y = sample_lr.y};
        width += tw;

        if(!isnan(pvv.val))
        {
            snprintf(str, sizeof(str), "  %.1lf Pa/s", pvv.val / 100.0);
        }
        else { str[0] = 0; }
        TTF_Text *pvv_txt = TTF_CreateText(app->text_engine, app->small_font, str, 0);
        TTF_SetTextColorFloat(pvv_txt, black.r, black.g, black.b, black.a);
        TTF_GetTextSize(pvv_txt, &tw, &th);
        SDL_FPoint pvv_txt_start = { .x = sample_ul.x + width + padding, .y = sample_lr.y};
        width += tw;

        sample_lr.y += (f32)th;
        sample_lr.x = sample_ul.x + width + 2.0f * padding < sample_lr.x ? sample_lr.x : sample_ul.x + width + 2.0f * padding;
        sample_lr.y += padding;

        /* Pressure and wind line. */
        width = 0.0f;
        SondeSpdDirKts wind = app->sample.row.wind;
        SondeHectopascal pres = app->sample.row.p;

        snprintf(str, sizeof(str), "%.0lfhPa", pres.val);
        TTF_Text *pres_txt = TTF_CreateText(app->text_engine, app->small_font, str, 0);
        TTF_SetTextColorFloat(pres_txt, isobar_color.r, isobar_color.g, isobar_color.b, isobar_color.a);
        TTF_GetTextSize(pres_txt, &tw, &th);
        SDL_FPoint pres_txt_start = { .x = sample_ul.x + padding, .y = sample_lr.y};
        width += tw;

        if(!isnan(wind.spd) && !isnan(wind.dir))
        {
            snprintf(str, sizeof(str), "  %03.0lf %.0lf kts", wind.dir, wind.spd);
        }
        else { str[0] = 0; }
        TTF_Text *wind_txt = TTF_CreateText(app->text_engine, app->small_font, str, 0);
        TTF_SetTextColorFloat(wind_txt, wind_color.r, wind_color.g, wind_color.b, wind_color.a);
        TTF_GetTextSize(wind_txt, &tw, &th);
        SDL_FPoint wind_txt_start = { .x =sample_ul.x + width + padding, .y = sample_lr.y};
        width += tw;

        sample_lr.y += (f32)th;
        sample_lr.x = sample_ul.x + width + 2.0f * padding < sample_lr.x ? sample_lr.x : sample_ul.x + width + 2.0f * padding;
        sample_lr.y += padding;

        /* Height ASL line */
        width = 0.0f;
        SondeMeter hgt_asl_m = app->sample.row.hgt;
        SondeFeet hgt_asl_ft = sonde_meters_to_feet(hgt_asl_m);

        snprintf(str, sizeof(str), "ASL: %5.0lfm ( %5.0lfft )", hgt_asl_m.val, hgt_asl_ft.val);
        TTF_Text *asl_txt = TTF_CreateText(app->text_engine, app->small_font, str, 0);
        TTF_SetTextColorFloat(asl_txt, sample_bar_color.r, sample_bar_color.g, sample_bar_color.b, sample_bar_color.a);
        TTF_GetTextSize(asl_txt, &tw, &th);
        SDL_FPoint asl_txt_start = { .x = sample_ul.x + padding, .y = sample_lr.y};
        width += tw;

        sample_lr.y += (f32)th;
        sample_lr.x = sample_ul.x + width + 2.0f * padding < sample_lr.x ? sample_lr.x : sample_ul.x + width + 2.0f * padding;
        sample_lr.y += padding;

        /* Height AGL line */
        width = 0.0f;
        SondeMeter elevation = snd->stn_info.elevation;
        SondeMeter hgt_agl_m = (SondeMeter){ .val = hgt_asl_m.val - elevation.val };
        SondeFeet hgt_agl_ft = sonde_meters_to_feet(hgt_agl_m);

        snprintf(str, sizeof(str), "AGL: %5.0lfm ( %5.0lfft )", hgt_agl_m.val, hgt_agl_ft.val);
        TTF_Text *agl_txt = TTF_CreateText(app->text_engine, app->small_font, str, 0);
        TTF_SetTextColorFloat(agl_txt, sample_bar_color.r, sample_bar_color.g, sample_bar_color.b, sample_bar_color.a);
        TTF_GetTextSize(agl_txt, &tw, &th);
        SDL_FPoint agl_txt_start = { .x = sample_ul.x + padding, .y = sample_lr.y};
        width += tw;

        sample_lr.y += (f32)th;
        sample_lr.x = sample_ul.x + width + 2.0f * padding < sample_lr.x ? sample_lr.x : sample_ul.x + width + 2.0f * padding;
        sample_lr.y += padding;

        /* Set up the sample readout box */
        SDL_FPoint border[4] = {0};
        border[0] = sample_ul;
        border[1] = (SDL_FPoint){ .x = sample_ul.x, .y = sample_lr.y };
        border[2] = sample_lr;
        border[3] = (SDL_FPoint){ .x = sample_lr.x, .y = sample_ul.y };

        /* Make adjustments for bottom of the screen */
        if(border[2].y > (f32)(cp->clipping.y + cp->clipping.h))
        {
            f32 dy = (f32)cp->clipping.h - border[2].y;

            t_txt_start.y += dy;
            dp_txt_start.y += dy;
            rh_txt_start.y += dy;
            rh_ice_txt_start.y += dy;
            pvv_txt_start.y += dy;
            pres_txt_start.y += dy;
            wind_txt_start.y += dy;
            asl_txt_start.y += dy;
            agl_txt_start.y += dy;

            border[0].y += dy;
            border[1].y += dy;
            border[2].y += dy;
            border[3].y += dy;
            /* TODO - don't forget this when adding lines to the sample */
        }

        /* Draw box for sample. */
        success &= DrawPolygonThick(app->renderer, border, ECO_ARRAY_SIZE(border), border_width, isobar_color, app->background, alloc_);

        /* Draw text */
        TTF_DrawRendererText(t_txt, t_txt_start.x, t_txt_start.y);
        TTF_DrawRendererText(dp_txt, dp_txt_start.x, dp_txt_start.y);
        TTF_DrawRendererText(rh_txt, rh_txt_start.x, rh_txt_start.y);
        TTF_DrawRendererText(rh_ice_txt, rh_ice_txt_start.x, rh_ice_txt_start.y);
        TTF_DrawRendererText(pvv_txt, pvv_txt_start.x, pvv_txt_start.y);
        TTF_DrawRendererText(pres_txt, pres_txt_start.x, pres_txt_start.y);
        TTF_DrawRendererText(wind_txt, wind_txt_start.x, wind_txt_start.y);
        TTF_DrawRendererText(asl_txt, asl_txt_start.x, asl_txt_start.y);
        TTF_DrawRendererText(agl_txt, agl_txt_start.x, agl_txt_start.y);

        /* Clean up allocated text. */
        TTF_DestroyText(agl_txt);
        TTF_DestroyText(asl_txt);
        TTF_DestroyText(wind_txt);
        TTF_DestroyText(pres_txt);
        TTF_DestroyText(pvv_txt);
        TTF_DestroyText(rh_ice_txt);
        TTF_DestroyText(rh_txt);
        TTF_DestroyText(dp_txt);
        TTF_DestroyText(t_txt);
    }

    return success;
}

static inline b32
sounding_draw(AppState *app)
{
    ChartPanel *cp = &app->skewt;
    MagAllocator alloc_ = app->scratch_alloc;

    b32 success = true;

    if(cp->needs_background_update)
    {
        cp->needs_background_update = false;
        success &= sounding_update_static_data(cp);                       Assert(success);
    }

    /* Set up clipping area. */
    SDL_SetRenderClipRect(app->renderer, &cp->clipping);

    success &= sounding_draw_background(app, alloc_);                     Assert(success);
    success &= sounding_draw_wind(app, alloc_);                           Assert(success);
    success &= sounding_draw_temperature_dew_point_wet_bulb(app, alloc_); Assert(success);
    success &= sounding_draw_sample(app, alloc_);                         Assert(success);
    success &= sounding_draw_legend(app, alloc_);                         Assert(success);

    /* Reset the clipping rect and return. */
    SDL_SetRenderClipRect(app->renderer, NULL);
    return success;
}

static inline void 
sounding_sample(AppState *app, SDL_FPoint loc)
{
    ChartPanel *cp = &app->skewt;

    loc = chart_panel_convert_from_screen_coords(loc, cp);
    loc = chart_panel_unapply_pan_and_zoom(loc, cp);

    SondeHectopascal p = {0};
    SondeCelsius t = {0};

    chart_coords_to_pressure_hpa_temperature_c(loc, cp, &p, &t);

    if(app->sndg_amalgams)
    {
        SondeSounding *snd = app->sndg_amalgams[app->current_sndg].sndg;

        SondeSoundingRow sample = sonde_sounding_sample_at_pressure(snd, p);

        if(sample.p.val != 0.0)
        {
            app->sample.type = SAMPLE_TYPE_SKEWT;
            app->sample.row = sample;
        }
        else { app->sample.type = SAMPLE_TYPE_NONE; }
    }
    else { app->sample.type = SAMPLE_TYPE_NONE; }
}

#undef P_MAX
#undef P_MIN
#undef T_MAX
#undef T_MIN
#undef DP
#undef N_DP
