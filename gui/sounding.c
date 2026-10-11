/* This file has everything for drawing the sounding area. */

#define P_MAX 1050.0f
#define P_MIN 100.0f

#define T_MAX 51.5f
#define T_MIN -46.5f

/* --------------------------------------------------------- API --------------------------------------------------------- */

/* --------------------------------------------------- Implementations --------------------------------------------------- */
typedef struct
{
    SDL_FPoint p1;
    SDL_FPoint p2;
} SDL_FPoint_Pair;

/* Skew-T area configuration - Isobars */
static f32 const isobars[] = {1000.0f, 925.0f, 850.0f, 700.0f, 500.0f, 300.0f, 200.0f, 100.0f }; /* hPa */
static f32 const isobar_width = 1.0f;
static SDL_FColor const isobar_color = { .r = 0.862745f, .g = 0.388235f, .b = 0.156863f, .a = 1.0f };
static SDL_FPoint isobars_sdl[ECO_ARRAY_SIZE(isobars)] = {0};
static TTF_Text *isobar_labels[ECO_ARRAY_SIZE(isobars)] = {0};

/* Skew-T area configuration - Isotherms */
static f32 const isotherms[] = /* C */
    {
          -150.0f, -140.0f, -130.0f, -120.0f, -110.0f, -100.0f, -90.0f, -80.0f, -70.0f, -60.0f, -50.0f,
           -40.0f,  -30.0f,  -20.0f,  -10.0f,    0.0f,   10.0f,  20.0f,  30.0f,  40.0f,  50.0f
    };

static f32 const isotherm_width = 1.0f;
static SDL_FColor const isotherm_color = { .r = 0.862745f, .g = 0.388235f, .b = 0.156863f, .a = 1.0f };
static SDL_FPoint_Pair isotherms_sdl[ECO_ARRAY_SIZE(isotherms)] = {0};
static TTF_Text *isotherm_labels[ECO_ARRAY_SIZE(isotherms)] = {0};
static f32 const freezing_level_width = 3.0f;
static SDL_FColor const freezing_level_color =  { .r = 0.0f, .g = 0.466667f, .b = 0.780392f, .a = 1.0f };

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

    Assert(result.x <= 1.0f && result.x >= 0.0f);
    Assert(result.y <= 1.0f && result.y >= 0.0f);

    return result;
}

b32
sounding_update_static_data(AppState *app)
{
    ChartPanel *cp = &app->skewt;

    /* Calculate the screen coordinates of the isobars. */
    for(i32 i = 0; i < ECO_ARRAY_SIZE(isobars); ++i)
    {
        f32 chart_y = pressure_hpa_to_chart_y((SondeHectopascal) { .val = isobars[i]}, cp);
        SDL_FPoint chart_coords = { .x = 0.0f, .y = chart_y };
        chart_coords = chart_panel_apply_pan_and_zoom(chart_coords, cp);
        SDL_FPoint screen_coords = chart_panel_convert_to_screen_coords(chart_coords, cp);

        isobars_sdl[i] = screen_coords;
    }

    /* Calculate the screen coordinates of the isotherms. */
    for(i32 i = 0; i < ECO_ARRAY_SIZE(isotherms); ++i)
    {
        SDL_FPoint chart_p1 = pressure_hpa_temperature_c_to_chart_coords((SondeHectopascal){ .val = P_MAX }, (SondeCelsius){ .val = isotherms[i] }, cp);
        SDL_FPoint chart_p2 = pressure_hpa_temperature_c_to_chart_coords((SondeHectopascal){ .val = P_MIN }, (SondeCelsius){ .val = isotherms[i] }, cp);

        chart_p1 = chart_panel_apply_pan_and_zoom(chart_p1, cp);
        chart_p2 = chart_panel_apply_pan_and_zoom(chart_p2, cp);

        SDL_FPoint screen_p1 = chart_panel_convert_to_screen_coords(chart_p1, cp);
        SDL_FPoint screen_p2 = chart_panel_convert_to_screen_coords(chart_p2, cp);

        isotherms_sdl[i] = (SDL_FPoint_Pair){ .p1 = screen_p1, .p2 = screen_p2 };
    }

    return true;
}

b32
sounding_initialize_static_data(AppState *app)
{
    b32 success = true;

    /* Set up isobars for the Skew-T area */
    for(i32 i = 0; i < ECO_ARRAY_SIZE(isobars); ++i)
    {
        char cbuf[32] = {0};
        snprintf(cbuf, sizeof(cbuf), "%.0f hPa", isobars[i]);
        isobar_labels[i] = TTF_CreateText(app->text_engine, app->font, cbuf, 0);
        TTF_SetTextColorFloat(isobar_labels[i], isobar_color.r, isobar_color.g, isobar_color.b, isobar_color.a);
    }

    /* Set up isotherms for the Skew-T area */
    for(i32 i = 0; i < ECO_ARRAY_SIZE(isotherms); ++i)
    {
        char cbuf[32] = {0};
        snprintf(cbuf, sizeof(cbuf), "%.0f °C", isotherms[i]);
        isotherm_labels[i] = TTF_CreateText(app->text_engine, app->font, cbuf, 0);
        TTF_SetTextColorFloat(isotherm_labels[i], isotherm_color.r, isotherm_color.g, isotherm_color.b, isotherm_color.a);
    }

    success &= sounding_update_static_data(app);

    return success;
}

b32
sounding_draw(AppState *app)
{
    ChartPanel *cp = &app->skewt;
    b32 success = true;

    f32 w = (f32)cp->chart_w;

    /* Set up clipping area. */
    SDL_SetRenderClipRect(app->renderer, &cp->clipping);

    /* Draw the isobars. */
    for(i32 i = 0; i < ECO_ARRAY_SIZE(isobars); ++i)
    {
        SDL_FPoint p1 = {.x = cp->x_offset,     .y = isobars_sdl[i].y };
        SDL_FPoint p2 = {.x = w + cp->x_offset, .y = isobars_sdl[i].y };
        success &= DrawLineThickRoundedAA(app->renderer, p1, p2, isobar_width, isobar_color);
    }

    /* Draw the isotherms. */
    for(i32 i = 0; i < ECO_ARRAY_SIZE(isotherms); ++i)
    {
        SDL_FColor color = isotherms[i] == 0.0f && app->show_freezing_level ? freezing_level_color : isotherm_color;
        f32 width = isotherms[i] == 0.0f && app->show_freezing_level ? freezing_level_width : isotherm_width;

        SDL_FPoint_Pair pair = isotherms_sdl[i];
        success &= DrawLineThickRoundedAA(app->renderer, pair.p1, pair.p2, width, color);
        i32 tw, th;
        TTF_GetTextSize(isotherm_labels[i], &tw, &th);
        TTF_DrawRendererText(isotherm_labels[i], pair.p1.x - (f32)(tw / 4),  pair.p1.y - (f32)th);
        TTF_DrawRendererText(isotherm_labels[i], pair.p2.x - (f32)(tw / 4),  pair.p2.y);
    }

    /* ---------- Do a second pass to draw the labels on top of everything else in the background. */

    /* Label the isobars */
    for(i32 i = 0; i < ECO_ARRAY_SIZE(isobars); ++i)
    {
        SDL_FPoint p1 = {.x = cp->x_offset,     .y = isobars_sdl[i].y };

        i32 tw, th;
        TTF_GetTextSize(isobar_labels[i], &tw, &th);
        SDL_FRect trect = { .x = p1.x + 5.0f, .y = p1.y - (f32)(th /2), .w = (f32)(tw), .h = (f32)(th) };
        SDL_SetRenderDrawColorFloat(app->renderer, app->background.r, app->background.g, app->background.b, app->background.a);
        SDL_RenderFillRect(app->renderer, &trect);
        SDL_SetRenderDrawColorFloat(app->renderer, isobar_color.r, isobar_color.g, isobar_color.b, isobar_color.a);
        TTF_DrawRendererText(isobar_labels[i], p1.x + 5.0f, p1.y - (f32)(th / 2));
    }

    /* Label the isotherms. */
    for(i32 i = 0; i < ECO_ARRAY_SIZE(isotherms); ++i)
    {
        SDL_FPoint_Pair pair = isotherms_sdl[i];
        SDL_FPoint p1 = pair.p1;
        SDL_FPoint p2 = pair.p2;

        i32 tw, th;
        TTF_GetTextSize(isotherm_labels[i], &tw, &th);

        SDL_FRect trect1 = { .x = p1.x - (f32)(tw / 4), .y = p1.y - (f32)(th), .w = (f32)(tw), .h = (f32)(th) };
        SDL_FRect trect2 = { .x = p2.x - (f32)(tw / 4), .y = p2.y, .w = (f32)(tw), .h = (f32)(th) };

        SDL_SetRenderDrawColorFloat(app->renderer, app->background.r, app->background.g, app->background.b, app->background.a);
        SDL_RenderFillRect(app->renderer, &trect1);
        SDL_RenderFillRect(app->renderer, &trect2);
        SDL_SetRenderDrawColorFloat(app->renderer, isotherm_color.r, isotherm_color.g, isotherm_color.b, isotherm_color.a);
        TTF_DrawRendererText(isotherm_labels[i], pair.p1.x - (f32)(tw / 4),  pair.p1.y - (f32)th);
        TTF_DrawRendererText(isotherm_labels[i], pair.p2.x - (f32)(tw / 4),  pair.p2.y);
    }

    /* Reset the clipping rect and return. */
    SDL_SetRenderClipRect(app->renderer, NULL);
    return success;
}

