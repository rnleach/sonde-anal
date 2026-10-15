/* This file includes code for managing the application state. There is no attempt at encapsulation for AppState.          */

/* --------------------------------------------------------- API --------------------------------------------------------- */
typedef struct
{
    /* Base UI stuff for any SDL3 / Nuklear app using the Callback API. */
    SDL_Window   *window;
    SDL_Renderer *renderer;
    SDL_Texture *scratch_layer;
    TTF_TextEngine *text_engine;
    TTF_Font *small_font;
    TTF_Font *large_font;
    struct nk_context *ctx;
    u64 last_frame;                  /* For measuring and controlling actual FPS.                                         */
    i32 window_width, window_height; /* Fore detecting resizes. SDL_AppEvent and SDL_AppIterate are out of sync on sizes. */
    MagAllocator scratch_alloc;
    MagAllocator data_alloc;

    /* Sounding data - actual data */
    SondeSounding **sndgs;
    i32 n_soundings;
    i32 current_sndg;

    /* Charts and drawing areas */
    ChartPanel skewt;
    SDL_Point context_menu_location;
    b32 show_skew_t_context_menu;

    /* Useful UI state for things like panning */
    SDL_FPoint mouse_loc;

    /* UI Element Options*/
    SDL_FColor background;
    b32 show_freezing_level;
    b32 show_hail_growth_zone;
    b32 show_dendritic_growth_zone;
    b32 show_wet_bulb;

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
        TTF_Font *small_font,
        TTF_Font *large_font,
        struct nk_context *ctx,
        i32 w,                            /* Window Width  */
        i32 h                             /* Window height */
    )
{
    /* Base UI stuff */
    app->window = window;
    app->renderer = renderer;
    app->scratch_layer = scratch_layer;
    app->text_engine = text_engine;
    app->small_font = small_font;
    app->large_font = large_font;
    app->ctx = ctx;
    app->scratch_alloc = mag_allocator_dyn_arena_create(ECO_MiB(2));
    app->data_alloc = mag_allocator_dyn_arena_create(ECO_MiB(2));

    app->last_frame = 0;
    app->background = (SDL_FColor){ .r = 0.95f, .g = 0.95f, .b = 0.95f, .a = 1.0f};

    /* Sounding data - actual data */
    app->sndgs = NULL;
    app->current_sndg = -1;

    /* Skew-t panel state */
    app->skewt = chart_panel_create(0, 0, w / 2, h, 1.0);
    app->show_freezing_level = true;
    app->show_hail_growth_zone = true;
    app->show_dendritic_growth_zone = true;
    app->show_wet_bulb = false;

    app->show_skew_t_context_menu = false;
}

