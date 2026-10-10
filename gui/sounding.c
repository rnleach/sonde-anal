/* This file has everything for drawing the sounding area. */

#define P_MAX 1050.0f
#define P_MIN 82.0f

#define T_MAX 60.0f
#define T_MIN -50.0f

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
static f32 isobars_sdl[ECO_ARRAY_SIZE(isobars)] = {0};
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
pressure_hpa_to_sdl_fpoint_y(SondeHectopascal p, f32 h)
{
    return (logf(p.val) - logf(P_MIN)) / (logf(P_MAX) - logf(P_MIN)) * h;
}

static inline SDL_FPoint
pressure_hpa_temperature_c_to_sdl_fpoint(SondeHectopascal p, SondeCelsius t, f32 h, f32 w)
{
    w *= 0.5; /* To force it into the left half plane. */

    SDL_FPoint result = {0};
    result.y = (logf(p.val) - logf(P_MIN)) / (logf(P_MAX) - logf(P_MIN)) * h;
    f32 rise = h - result.y;
    result.x = (t.val - T_MIN) / (T_MAX - T_MIN) * w + rise; /* rise = run for 45 degree slope! */

    return result;
}

b32
sounding_update_static_data(AppState *app)
{
    i32 w_i, h_i;
    SDL_GetCurrentRenderOutputSize(app->renderer, &w_i, &h_i);
    f32 w = (f32)w_i;
    f32 h = (f32)h_i;

    /* Calculate the screen coordinates of the isobars. */
    for(i32 i = 0; i < ECO_ARRAY_SIZE(isobars); ++i)
    {
        isobars_sdl[i] = pressure_hpa_to_sdl_fpoint_y((SondeHectopascal) { .val = isobars[i]}, h);
    }

    /* Calculate the screen coordinates of the isotherms. */
    for(i32 i = 0; i < ECO_ARRAY_SIZE(isotherms); ++i)
    {
        SDL_FPoint p1 = pressure_hpa_temperature_c_to_sdl_fpoint((SondeHectopascal){ .val = P_MAX }, (SondeCelsius){ .val = isotherms[i] }, h, w);
        SDL_FPoint p2 = pressure_hpa_temperature_c_to_sdl_fpoint((SondeHectopascal){ .val = 100.0 }, (SondeCelsius){ .val = isotherms[i] }, h, w);
        isotherms_sdl[i] = (SDL_FPoint_Pair){ .p1 = p1, .p2 = p2 };
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
    b32 success = true;

    f32 w = (f32)app->skewt_clipping.w;

    /* Set up clipping area. */
    SDL_SetRenderClipRect(app->renderer, &app->skewt_clipping);

    /* Draw the isobars. */
    for(i32 i = 0; i < ECO_ARRAY_SIZE(isobars); ++i)
    {
        SDL_FPoint p1 = {.x = 0, .y = isobars_sdl[i] };
        SDL_FPoint p2 = {.x = w, .y = isobars_sdl[i] };
        success &= DrawLineThickRoundedAA(app->renderer, p1, p2, isobar_width, isobar_color);
        i32 tw, th;
        TTF_GetTextSize(isobar_labels[i], &tw, &th);
        TTF_DrawRendererText(isobar_labels[i], 5.0, isobars_sdl[i] - (f32)th);
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
        TTF_DrawRendererText(isotherm_labels[i], pair.p1.x,  pair.p1.y - (f32)th);
        TTF_DrawRendererText(isotherm_labels[i], pair.p2.x,  pair.p2.y);
    }

    SDL_SetRenderClipRect(app->renderer, NULL);
    return success;
}

