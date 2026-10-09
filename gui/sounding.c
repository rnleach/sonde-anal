/* This file has everything for drawing the sounding area. */

#define P_MAX 1050.0f
#define P_MIN 85.0f

#define T_MAX 80.0f
#define T_MIN -80.0f

/* --------------------------------------------------------- API --------------------------------------------------------- */

/* --------------------------------------------------- Implementations --------------------------------------------------- */

/* Skew-T area configuration - Isobars */
static f32 const isobars[] = {1000.0f, 925.0f, 850.0f, 700.0f, 500.0f, 300.0f, 200.0f, 100.0f }; /* hPa */
static f32 const isobar_width = 1.0f;
static SDL_FColor const isobar_color = { .r = 0.862745f, .g = 0.388235f, .b = 0.156863f, .a = 1.0f };
static f32 isobars_sdl[ECO_ARRAY_SIZE(isobars)] = {0};
static TTF_Text *isobar_labels[ECO_ARRAY_SIZE(isobars)] = {0};

static inline f32
pressure_hpa_to_sdl_fpoint_y(SondeHectopascal p, f32 h)
{
    return (logf(p.val) - logf(P_MIN)) / (logf(P_MAX) - logf(P_MIN)) * h;
}

b32
sounding_update_static_data(AppState *app)
{
    i32 w_i, h_i;
    SDL_GetCurrentRenderOutputSize(app->renderer, &w_i, &h_i);
    /* f32 w = (f32)w_i; */
    f32 h = (f32)h_i;

    /* Calculate the screen coordinates of the isobars. */
    for(i32 i = 0; i < ECO_ARRAY_SIZE(isobars); ++i)
    {
        isobars_sdl[i] = pressure_hpa_to_sdl_fpoint_y((SondeHectopascal) { .val = isobars[i]}, h);
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
    success &= sounding_update_static_data(app);

    return success;
}

b32
sounding_draw(AppState *app)
{
    b32 success = true;

    i32 w_i, h_i;
    SDL_GetCurrentRenderOutputSize(app->renderer, &w_i, &h_i);
    f32 w = (f32)w_i;
    /* f32 h = (f32)h_i; */

    /* Set up clipping area. */
    SDL_Rect clip = { .x = 0, .y = 0, .w = w_i / 2, .h = h_i };
    SDL_SetRenderClipRect(app->renderer, &clip);

    /* Draw the isobars. */
    for(i32 i = 0; i < ECO_ARRAY_SIZE(isobars); ++i)
    {
        SDL_FPoint p1 = {.x = 0, .y = isobars_sdl[i] };
        SDL_FPoint p2 = {.x = w, .y = isobars_sdl[i] };
        success &= DrawLineThick(app->renderer, p1, p2, isobar_width, isobar_color);
        i32 tw, th;
        TTF_GetTextSize(isobar_labels[i], &tw, &th);
        TTF_DrawRendererText(isobar_labels[i], 5.0, isobars_sdl[i] - (f32)th);
    }

    SDL_SetRenderClipRect(app->renderer, NULL);
    return success;
}

