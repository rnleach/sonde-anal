/* This file includes code for managing the application state. There is no attempt at encapsulation for AppState.          */

/* --------------------------------------------------------- API --------------------------------------------------------- */
typedef struct
{
    /* Base UI stuff for any SDL3 / Nuklear app using the Callback API. */
    SDL_Window   *window;
    SDL_Renderer *renderer;
    SDL_Texture *scratch_layer;
    TTF_TextEngine *text_engine;
    TTF_Font *font;
    struct nk_context *ctx;
    u64 last_frame;                /* For measuring and controlling actual FPS.    */

    /* Charts and drawing areas */
    ChartPanel skewt;

    /* Useful UI state for things like panning */
    SDL_FPoint mouse_loc;

    /* UI Element Options*/
    SDL_FColor background;
    b32 show_freezing_level;

    /* Debug Window Information. */
    bool show_debug;
    f32 frame_time;          /* Time to render a frame in ms */

} AppState;

static inline void
app_state_initialize(
        AppState *app,
        SDL_Window *window,
        SDL_Renderer *renderer,
        SDL_Texture *scratch_layer,
        TTF_TextEngine *text_engine,
        TTF_Font *font,
        struct nk_context *ctx,
        i32 w,                            /* Window Width  */
        i32 h                             /* Window height */
    )
{
    app->window = window;
    app->renderer = renderer;
    app->scratch_layer = scratch_layer;
    app->text_engine = text_engine;
    app->font = font;
    app->ctx = ctx;

    app->last_frame = 0;
    app->skewt = chart_panel_create(0, 0, w / 2, h, 1.2);
    app->background = (SDL_FColor){ .r = 0.95f, .g = 0.95f, .b = 0.95f, .a = 1.0f};
    app->show_freezing_level = true;
}

