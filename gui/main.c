#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include <SDL3/SDL.h>
#include <SDL3/SDL_dialog.h>
#include <SDL3_ttf/SDL_ttf.h>

#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL_main.h>

#define TARGET_FRAME_RATE_FPS 60
#define FRAME_TARGET_TIME (1000000000 / TARGET_FRAME_RATE_FPS) /* in nanoseconds for SDL3 */
#define SMALL_FONT_SIZE (10.5f)
#define LARGE_FONT_SIZE (14.0f)

/* ------------------------------------------------ Nuklear configuration ------------------------------------------------ */
#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#define NK_INCLUDE_COMMAND_USERDATA   /* required by SDL3 backend */

#define NK_IMPLEMENTATION
#include "nuklear.h"

#define NK_SDL3_RENDERER_IMPLEMENTATION
#include "nuklear_sdl3_renderer.h"

/* ------------------------------------------------------ Libraries ------------------------------------------------------ */

#include "elk.h"
#include "magpie.h"
#include "coyote.h"
#include "packrat.h"

#include "sonde-anal.c"

/* ------------------------------------------------------- Modules ------------------------------------------------------- */
/* Order matters for these includes. */
#include "chart_panel.c"
#include "app_state.c"
#include "graphics.c"

/* Specializations for each panel or drawing area. */
#include "sounding.c"

/* Embedded Font */
#include "fonts/Roboto-Bold.h"

/* ------------------------------------------------------ App State ------------------------------------------------------ */
static AppState global_app_state = {0};

/* --------------------------------------- Helper: make a panel fully transparent ---------------------------------------- */
static void 
style_hud_panel(struct nk_context *ctx)
{
    /* Make the window background invisible */
    ctx->style.window.fixed_background = nk_style_item_color(nk_rgba(0, 0, 0, 0));
    ctx->style.window.background       = nk_rgba(0, 0, 0, 0);
    ctx->style.window.border_color     = nk_rgba(255, 255, 255, 0);
    ctx->style.window.border           = 0.0f;
    ctx->style.window.rounding         = 0.0f;
    ctx->style.window.padding          = nk_vec2(10, 8);
    ctx->style.window.spacing          = nk_vec2(6, 4);

    /* Labels */
    ctx->style.text.color              = nk_rgba(245, 245, 245, 255);

    /* Soft buttons */
    ctx->style.button.rounding         = 6.0f;
    ctx->style.button.padding          = nk_vec2(12, 6);
    ctx->style.button.border           = 1.0f;
    ctx->style.button.normal           = nk_style_item_color(nk_rgba(40, 40, 50, 255));
    ctx->style.button.hover            = nk_style_item_color(nk_rgba(70, 70, 90, 255));
    ctx->style.button.active           = nk_style_item_color(nk_rgba(100, 100, 140, 255));
    ctx->style.button.text_normal      = nk_rgb(230, 230, 240);
    ctx->style.button.text_hover       = nk_rgb(255, 255, 255);
    ctx->style.button.text_active      = nk_rgb(255, 255, 255);
}

static void 
style_debug_panel(struct nk_context *ctx)
{
    /* Make the window background mostly opaque */
    ctx->style.window.fixed_background = nk_style_item_color(nk_rgba(20, 20, 20, 192));
    ctx->style.window.background       = nk_rgba(40, 40, 40, 255);
    ctx->style.window.border_color     = nk_rgba(255, 255, 255, 255);
    ctx->style.window.border           = 1.0f;
    ctx->style.window.rounding         = 6.0f;
    ctx->style.window.padding          = nk_vec2(10, 8);
    ctx->style.window.spacing          = nk_vec2(6, 4);

    /* Labels */
    ctx->style.text.color              = nk_rgba(245, 245, 245, 255);
}

static inline void 
nk_menu_separator(struct nk_context *ctx)
{
    /* Allocate a thin row (e.g., 6 pixels high) for the separator */
    nk_layout_row_dynamic(ctx, 6, 1);
    
    /* Get the screen space bounds of this row allocation */
    struct nk_rect bounds = nk_widget_bounds(ctx);
    
    /* Draw a horizontal line through the middle of the allocated slot */
    struct nk_command_buffer *canvas = nk_window_get_canvas(ctx);
    float mid_y = bounds.y + (bounds.h / 2.0f);
    
    /* Adjust the color (nk_rgb) to match your menu theme's border color */
    nk_stroke_line(canvas, bounds.x, mid_y, bounds.x + bounds.w, mid_y, 1.0f, nk_rgb(65, 65, 65));
}

/* --------------------------------------- Helper: load a font from static memory ---------------------------------------- */
TTF_Font * 
load_embedded_ttf_font(f32 ptsize)
{
    /* Wrap the static array in an SDL3 IOStream */
    SDL_IOStream *stream = SDL_IOFromConstMem(Roboto_Bold_ttf, Roboto_Bold_ttf_len);
    StopIf(!stream, SDL_Log("Failed to create IOStream: %s", SDL_GetError()); return NULL);

    /* Load the font via the IOStream wrapper. */
    /* Setting the second parameter to 'true' automatically frees 'stream' when done/failed. */
    TTF_Font *font = TTF_OpenFontIO(stream, true, ptsize);
    StopIf(!font, SDL_Log("Failed to open embedded font: %s", SDL_GetError()); return NULL);

    return font;
}

/* -------------------------------------------- Helper: Load a file callback --------------------------------------------- */
/* This callback handles the chosen file path */
void SDLCALL 
load_bufkit_file_callback(void *userdata, char const * const *filelist, i32 filter)
{
    AppState *app = userdata;

    if(!filelist)
    {
        /* An error occurred */
        SDL_Log("Error opening file dialog: %s", SDL_GetError());
    }
    else if(!*filelist)
    {
        /* The user cancelled the dialog */
        SDL_Log("User cancelled the operation.");
    }
    else
    {
        /* The user selected a file (or files)            */
        /* filelist is a null-terminated array of strings */
        char const *chosen_file_path = filelist[0];
        SDL_Log("User selected file: %s", chosen_file_path);

        MagAllocator scratch_ = app->scratch_alloc;
        MagAllocator *scratch = &scratch_;
        MagAllocator *data = &app->data_alloc;

        eco_arena_reset(data);

        ElkStr fn_str = elk_str_from_cstring((char *)chosen_file_path);
        ElkStr f_string = eco_file_slurp_text(chosen_file_path, scratch);

        SondeSoundingList *sndgs = sonde_sounding_from_bufkit_str(data, f_string, fn_str);
        sonde_sounding_list_fill_in_profiles( /* Does the best it can, but ignores profiles that weren't fillable. */
                sndgs,
                SONDE_PC_DEW_POINT
                    | SONDE_PC_WET_BULB
                    | SONDE_PC_VIRTUAL_TEMPERATURE 
                    | SONDE_PC_THETA
                    | SONDE_PC_THETA_E 
                    | SONDE_PC_RH 
                    | SONDE_PC_RH_ICE 
                    | SONDE_PC_FROST_POINT
                    | SONDE_PC_WIND_UV
                    | SONDE_PC_WIND_SPEED_DIR
            );

        app->n_soundings = 0;
        SondeSoundingList *curr = sndgs;
        while(curr)
        {
            app->n_soundings += 1;
            curr = curr->next;
        }

        app->sndgs = eco_arena_nmalloc(data, app->n_soundings, SondeSounding *);
        app->current_sndg = 0;

        curr = sndgs;
        i32 i = 0;
        while(curr)
        {
            app->sndgs[i++] = curr->snd;
            curr = curr->next;
        }
    }

    app->needs_redraw = true;
}

/* --------------------------------------------------- SDL3 Callbacks ---------------------------------------------------- */

f32 
GetCurrentWindowRefreshRate(SDL_Window *window) 
{
    SDL_DisplayID display_id = SDL_GetDisplayForWindow(window);
    
    if(display_id == 0)
    {
        SDL_Log("Failed to get Display for Window: %s", SDL_GetError());
        return 0.0f; /* Default fallback */
    }

    /* Fetch the active desktop configuration mode for that display */
    SDL_DisplayMode const *mode = SDL_GetDesktopDisplayMode(display_id);
    
    if(!mode)
    {
        SDL_Log("Failed to get Desktop Display Mode: %s", SDL_GetError());
        return 60.0f; /* Default fallback */
    }

    return mode->refresh_rate;
}

SDL_AppResult 
SDL_AppInit(void **appstate, int argc, char *argv[])
{
    /* Prefer wayland if possible on linux to get rid of flicker when resizing. */
    SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "wayland,x11");

    b32 success = SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS);
    StopIf(!success, SDL_Log("SDL_Init failed: %s", SDL_GetError()); return SDL_APP_FAILURE);
    success = TTF_Init();
    StopIf(!success, SDL_Log("TTF_Init failed: %s", SDL_GetError()); return SDL_APP_FAILURE);

    AppState *app = &global_app_state;
    *appstate = app;

    SDL_Window *window;
    SDL_Renderer *renderer;
    success = SDL_CreateWindowAndRenderer(
                "Sonde 2",
                1280,
                720,
                SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY,
                &window,
                &renderer);
    StopIf(!success, SDL_Log("CreateWindowAndRenderer failed: %s", SDL_GetError()); return SDL_APP_FAILURE);

    /* High-DPI handling */
    f32 scale = SDL_GetWindowDisplayScale(window);
    SDL_Log("Display Scale is: %f", scale);
    SDL_Log("Display Refresh rate: %f", GetCurrentWindowRefreshRate(window));
    SDL_SetRenderScale(renderer, scale, scale);
    SDL_SetRenderVSync(renderer, 1);

    /* Set up the fonts. */
    TTF_TextEngine *text_engine = TTF_CreateRendererTextEngine(renderer);
    StopIf(!text_engine, SDL_Log("Failure TTF_CreateRendererTextEngine."); return SDL_APP_FAILURE);
    TTF_Font *small_font = load_embedded_ttf_font(SMALL_FONT_SIZE * scale);
    StopIf(!small_font, SDL_Log("Failure to load small font."); return SDL_APP_FAILURE);
    TTF_Font *large_font = load_embedded_ttf_font(LARGE_FONT_SIZE * scale);
    StopIf(!large_font, SDL_Log("Failure to load large font."); return SDL_APP_FAILURE);

    /* Set up the scratch texture. */
    i32 w, h;
    SDL_GetRenderOutputSize(renderer, &w, &h);
    SDL_Texture *scratch_layer = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, w, h);

    /* Initialize Nuklear */
    struct nk_context *ctx = nk_sdl_init(window, renderer, nk_sdl_allocator());

    /* Fonts for Nuklear */
    {
        struct nk_font_atlas *atlas = nk_sdl_font_stash_begin(ctx);
        struct nk_font *font = nk_font_atlas_add_from_memory(atlas, Roboto_Bold_ttf, Roboto_Bold_ttf_len, SMALL_FONT_SIZE * 96.0 / 72.0 * scale, NULL);
        nk_sdl_font_stash_end(ctx);

        /* Compensate for the scale so text stays the right visual size */
        font->handle.height /= scale;
        nk_style_set_font(ctx, &font->handle);
    }

    /* Initialize the main application state. */
    app_state_initialize(app, window, renderer, scratch_layer, text_engine, small_font, large_font, ctx, w, h);

    /* Initialize the drawing areas. */
    sounding_initialize_static_data(app);

    /* Start receiving events - required by the SDL3 backend. */
    nk_input_begin(app->ctx);

    return SDL_APP_CONTINUE;
}

i32 const skew_t_context_menu_width = 200;
i32 const skew_t_context_menu_height = 400;

SDL_AppResult 
SDL_AppEvent(void *appstate, SDL_Event *event)
{
    /* SDL_AppEvent and SDL_AppIterate are out of sync when it comes to window resizes render state in general, so you
     * can't query dimensions here and have them still be correct when SDL_AppIterate is called. The coordinates and
     * renderer state always represent the state of the renderer on the last frame, not the one you're going into.
     */

    AppState *app = appstate;
    app->needs_redraw = true; /* On every event we need a redraw. */

    SDL_ConvertEventToRenderCoordinates(app->renderer, event);

    nk_sdl_handle_event(app->ctx, event);

    /* Watch specifically for Right Mouse Clicks to handle context menus */
    if(event->type == SDL_EVENT_MOUSE_BUTTON_DOWN && event->button.button == SDL_BUTTON_RIGHT) 
    {
        SDL_Point loc = { .x = (i32)event->button.x, .y = (i32)event->button.y };
        SDL_FPoint locf = { .x = event->button.x, .y = event->button.y };
        app->mouse_loc = locf;

        /* Check for right click in Skew-T area */
        if(SDL_PointInRect(&loc, &app->skewt.area))
        {
            app->context_menu_location = loc;
            app->show_skew_t_context_menu = true;
            return SDL_APP_CONTINUE;  /* Consume this event so nothing else handles it! */
        }
    }

    /* Watch specifically for Left Mouse Clicks to handle closing context menus */
    if(event->type == SDL_EVENT_MOUSE_BUTTON_DOWN && event->button.button == SDL_BUTTON_LEFT) 
    {
        SDL_Point loc = { .x = (i32)event->button.x, .y = (i32)event->button.y };
        SDL_FPoint locf = { .x = event->button.x, .y = event->button.y };
        app->mouse_loc = locf;

        SDL_Rect skew_t_context_menu_rect =
            {
                .x = app->context_menu_location.x,
                .y = app->context_menu_location.y,
                .w = skew_t_context_menu_width,
                .h = skew_t_context_menu_height
            };

        /* Check for left click outside menu area */
        if(!SDL_PointInRect(&loc, &skew_t_context_menu_rect))
        {
            app->show_skew_t_context_menu = false;
        }
    }

    /* Guard: Bypass background chart logic ONLY when interacting within context menu rect */
    if(app->show_skew_t_context_menu)
    {
        SDL_Rect skew_t_context_menu_rect = 
        {
            .x = app->context_menu_location.x,
            .y = app->context_menu_location.y,
            .w = skew_t_context_menu_width,
            .h = skew_t_context_menu_height
        };
        
        SDL_Point loc = { .x = (i32)event->button.x, .y = (i32)event->button.y };
        if(event->type == SDL_EVENT_MOUSE_MOTION) 
        {
            loc.x = (i32)event->motion.x;
            loc.y = (i32)event->motion.y;
        }
        else if(event->type == SDL_EVENT_MOUSE_WHEEL) 
        {
            loc.x = (i32)event->wheel.mouse_x;
            loc.y = (i32)event->wheel.mouse_y;
        }

        if(SDL_PointInRect(&loc, &skew_t_context_menu_rect)) { return SDL_APP_CONTINUE; }
    }
    
    /* Skip mouse event processing if interacting with standard active Nuklear UI elements. */
    if(nk_item_is_any_active(app->ctx) && event->type >= SDL_EVENT_MOUSE_MOTION && event->type <= SDL_EVENT_MOUSE_REMOVED )
    {
        return SDL_APP_CONTINUE; 
    }

    switch (event->type)
    {
        case SDL_EVENT_QUIT:
            {
                return SDL_APP_SUCCESS;
            } break;

        case SDL_EVENT_KEY_DOWN:
            {
                if(event->key.key == SDLK_ESCAPE) { return SDL_APP_SUCCESS; }
                if(event->key.key == SDLK_F1)     { app->show_debug = !app->show_debug; }

                if(event->key.key == SDLK_RIGHT)
                {
                    app->current_sndg = (app->current_sndg + 1) % app->n_soundings; 

                    SDL_FPoint locf = app->mouse_loc;
                    if(chart_panel_point_inside(&app->skewt, locf)) { sounding_sample(app, locf); }
                }

                if(event->key.key == SDLK_LEFT)
                {
                    app->current_sndg = (app->current_sndg + app->n_soundings - 1) % app->n_soundings; 

                    SDL_FPoint locf = app->mouse_loc;
                    if(chart_panel_point_inside(&app->skewt, locf)) { sounding_sample(app, locf); }
                }

            } break;

        case SDL_EVENT_MOUSE_MOTION:
            {
                SDL_FPoint locf = { .x = event->motion.x, .y = event->motion.y };

                if(event->motion.state == SDL_BUTTON_LEFT)
                {
                    if(chart_panel_point_inside(&app->skewt, locf)) { chart_panel_pan(&app->skewt, app->mouse_loc, locf); }
                    /* TODO: Add other panels here as needed. */
                }
                else if(event->motion.state == 0)
                {
                    if(chart_panel_point_inside(&app->skewt, locf)) { sounding_sample(app, locf); }
                    /* TODO: Add other panels here as needed. */
                }

                app->mouse_loc = locf;
            } break;

        case SDL_EVENT_MOUSE_WHEEL:
            {
                SDL_FPoint locf = { .x = event->wheel.mouse_x, .y = event->wheel.mouse_y };
                if(chart_panel_point_inside(&app->skewt, locf))
                {
                    ChartPanel *cp = &app->skewt;
                    SDL_FPoint locf = { .x = event->wheel.mouse_x, .y = event->wheel.mouse_y };

                    f32 const factor = 1.05;
                    f32 zf = event->wheel.y > 0.0f ? factor : event->wheel.y < 0.0f ?  1.0f / factor : 1.0f;

                    chart_panel_handle_zoom(cp, locf, zf);

                    if(chart_panel_point_inside(&app->skewt, app->mouse_loc)) { sounding_sample(app, app->mouse_loc); }
                    /* TODO: Add other panels here as needed. */
                }

            } break;

        case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED: /* fall through */
        case SDL_EVENT_WINDOW_MAXIMIZED:          /* fall through */
        case SDL_EVENT_WINDOW_RESTORED:           /* fall through */
        case SDL_EVENT_WINDOW_RESIZED:
            {
                /* This doesn't work like you think it should. Always just check it in the main loop. SDL_AppEvent and
                 * SDL_AppIterate are out of sync when it comes to window resizes, so you can't query dimensions here and
                 * have them still be correct when SDL_AppIterate is called. The coordinates and renderer state always 
                 * represent the state of the renderer on the last frame, not the one you're going into.
                 */
            } break;
    }

    return SDL_APP_CONTINUE;
}

SDL_AppResult 
SDL_AppIterate(void *appstate)
{
    AppState *app = appstate;
    struct nk_context *ctx = app->ctx;

    u64 const target_frame_rate_ns = SDL_NS_PER_SECOND / 60 - 1000;
    u64 start_time = SDL_GetTicksNS();
    u64 elapsed = start_time - app->last_frame;

    if(elapsed < target_frame_rate_ns) 
    {
        SDL_DelayNS(target_frame_rate_ns - elapsed); 
        return SDL_APP_CONTINUE;
    }

    f32 frame_rate = 1000000000.0f / (f32)(start_time - app->last_frame);
    app->last_frame = start_time;

    if(!app->needs_redraw) { return SDL_APP_CONTINUE; }
    app->needs_redraw = false;

    /* Required for Nuklear to handle deltas correctly. */
    nk_input_end(ctx);

    i32 w, h;
    SDL_GetRenderOutputSize(app->renderer, &w, &h);

    /* Check for resizes. */
    if(h != app->window_height || w != app->window_width)
    {
        /* Rebuild the scratch texture. */
        SDL_DestroyTexture(app->scratch_layer);
        app->scratch_layer = SDL_CreateTexture(app->renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, w, h);

        /* Rescale the sounding area. */
        chart_panel_update_for_screen_resize(&app->skewt, 0, 0, w / 2, h);
        app->window_width = w;
        app->window_height = h;
    }

    /* ---- Clear + draw background ---- */
    SDL_SetRenderDrawColorFloat(app->renderer, app->background.r, app->background.g, app->background.b, app->background.a);
    SDL_RenderClear(app->renderer);

    /* Draw the sounding area. */
    b32 success = sounding_draw(app); Assert(success);

    /* ======================================================================================================================
     *                                                     Interactive UI
     * =================================================================================================================== */
    style_hud_panel(ctx);

    f32 scale_x, scale_y;
    SDL_GetRenderScale(app->renderer, &scale_x, &scale_y);
    i32 logical_w = (i32)(w / scale_x);

    f32 const bar_w = logical_w * 0.5;
    f32 const bar_h = 60.0f;

    {
        if(nk_begin(ctx, "Actions", nk_rect(0.0f, 0.0f, bar_w, bar_h), NK_WINDOW_NO_SCROLLBAR | NK_WINDOW_BORDER))
        {
            nk_layout_row_dynamic(ctx, 40, 5);

            if(nk_button_label(ctx, "Load"))
            {
                SDL_DialogFileFilter const filters[] =
                    {
                        { "Bufkit Files (*.buf)", "buf" },
                        { "All Files", "*" }
                    };

                SDL_ShowOpenFileDialog(
                    load_bufkit_file_callback, /* Open callback function                             */
                    app,                       /* Optional userdata pointer to pass context          */
                    app->window,               /* Your SDL_Window * pointer (makes it modal)         */
                    filters,                   /* File filters pointer                               */
                    2,                         /* Number of filters                                  */
                    NULL,                      /* Default start location (NULL defaults to home/cwd) */
                    false                      /* allow_many (false restricts to picking 1 file)     */
                );
            }

            if(nk_button_label(ctx, "|<")) { app->current_sndg = 0; }
            if(nk_button_label(ctx, "<"))  { app->current_sndg = app->current_sndg - 1 < 0 ? app->n_soundings - 1 : app->current_sndg - 1; }
            if(nk_button_label(ctx, ">"))  { app->current_sndg = (app->current_sndg + 1) % app->n_soundings; }
            if(nk_button_label(ctx, ">|")) { app->current_sndg = app->n_soundings - 1; }

            /* --- Attached Popup Layout (Eliminates separate blocking window frame) --- */
        }
        nk_end(ctx);
    }

    if(app->show_skew_t_context_menu)
    {
        nk_style_push_color(ctx, &ctx->style.window.background, nk_rgba(40, 40, 40, 255));
        nk_style_push_style_item(ctx, &ctx->style.window.fixed_background, nk_style_item_color(nk_rgba(40, 40, 40, 255)));

        struct nk_rect popup_bounds = nk_rect(
            app->context_menu_location.x,
            app->context_menu_location.y,
            skew_t_context_menu_width,
            skew_t_context_menu_height
        );

        if(nk_begin(ctx, "SkewTContextMenuWindow", popup_bounds, NK_WINDOW_BORDER | NK_WINDOW_NO_SCROLLBAR))
        {
            nk_layout_row_dynamic(ctx, skew_t_context_menu_height - 10, 1);
            
            if (nk_group_begin(ctx, "MenuScrollGroup", 0)) 
            {
                nk_layout_row_dynamic(app->ctx, 25, 1);
                
                if(nk_checkbox_label(ctx, "Show Wet Bulb", &app->show_wet_bulb)) 
                { 
                    app->show_skew_t_context_menu = false; 
                }

                nk_menu_separator(app->ctx);
                nk_layout_row_dynamic(app->ctx, 25, 1);

                if(nk_checkbox_label(ctx, "Show Freezing Level", &app->show_freezing_level)) 
                { 
                    app->show_skew_t_context_menu = false; 
                }

                if(nk_checkbox_label(ctx, "Show Hail Growth", &app->show_hail_growth_zone)) 
                { 
                    app->show_skew_t_context_menu = false; 
                }

                if(nk_checkbox_label(ctx, "Show Dendritic Growth", &app->show_dendritic_growth_zone)) 
                { 
                    app->show_skew_t_context_menu = false; 
                }
                
                if(nk_checkbox_label(ctx, "Show Mixing Ratios", &app->show_isohumes)) 
                { 
                    app->show_skew_t_context_menu = false; 
                }
                
                if(nk_checkbox_label(ctx, "Show ThetaE", &app->show_equivalent_pot_temp)) 
                { 
                    app->show_skew_t_context_menu = false; 
                }
                
                nk_group_end(ctx);
            }
            nk_end(ctx);
        }
        else
        {
            app->show_skew_t_context_menu = false;
        }

        nk_style_pop_color(ctx);
        nk_style_pop_style_item(ctx);
    }

    /* --- Debug panel --- */
    if(app->show_debug)
    {
        style_debug_panel(ctx);
        if(nk_begin(ctx, "Debug", nk_rect(150, 150, 320, 400), NK_WINDOW_BORDER | NK_WINDOW_MOVABLE | NK_WINDOW_SCALABLE | NK_WINDOW_TITLE))
        {
            nk_layout_row_dynamic(ctx, 25, 1);
            nk_labelf(ctx, NK_TEXT_LEFT, "   mouse_loc: (%.2f, %.2f)", app->mouse_loc.x, app->mouse_loc.y);
            nk_labelf(ctx, NK_TEXT_LEFT, "  frame rate: %.2f FPS", frame_rate);
            nk_labelf(ctx, NK_TEXT_LEFT, "  frame time: %.2f ms [%.1f FPS]", app->frame_time, 1000.0f / app->frame_time);
            nk_labelf(ctx, NK_TEXT_LEFT, " n_soundings: %d", app->n_soundings);
            nk_labelf(ctx, NK_TEXT_LEFT, "current_sndg: %d", app->current_sndg);
            nk_labelf(ctx, NK_TEXT_LEFT, "          p: %.2lf hPa", app->sample.row.p.val);
            nk_labelf(ctx, NK_TEXT_LEFT, "          t: %.2lf °C", app->sample.row.t.val);
            nk_labelf(ctx, NK_TEXT_LEFT, "         dp: %.2lf °C", app->sample.row.dp.val);
            nk_labelf(ctx, NK_TEXT_LEFT, "         wb: %.2lf °C", app->sample.row.wb.val);
            nk_labelf(ctx, NK_TEXT_LEFT, "         vt: %.2lf °C", app->sample.row.vt.val);
            nk_labelf(ctx, NK_TEXT_LEFT, "      theta: %.2lf K", app->sample.row.theta.val);
            nk_labelf(ctx, NK_TEXT_LEFT, "    theta_e: %.2lf K", app->sample.row.theta_e.val);
            nk_labelf(ctx, NK_TEXT_LEFT, "        pvv: %.2lf hPa/s", app->sample.row.pvv.val);
            nk_labelf(ctx, NK_TEXT_LEFT, "        hgt: %.2lf m", app->sample.row.hgt.val);
            nk_labelf(ctx, NK_TEXT_LEFT, "         cf: %.2lf %%", app->sample.row.cloud_fraction * 100.0);
            nk_labelf(ctx, NK_TEXT_LEFT, "         rh: %.2lf %%", app->sample.row.rh * 100.0);
            nk_labelf(ctx, NK_TEXT_LEFT, "     rh ice: %.2lf %%", app->sample.row.rh_ice * 100.0);
            nk_labelf(ctx, NK_TEXT_LEFT, "frost point: %.2lf °C", app->sample.row.fp.val);
            nk_labelf(ctx, NK_TEXT_LEFT, "    UV Wind: (%.2lf, %.2lf) mps", app->sample.row.uv_wind.u, app->sample.row.uv_wind.v);
            nk_labelf(ctx, NK_TEXT_LEFT, "       Wind: %.0lf @ %.2lf kts", app->sample.row.wind.dir, app->sample.row.wind.spd);
        }
        nk_end(ctx);
    }

    nk_sdl_render(ctx, NK_ANTI_ALIASING_ON);
    nk_input_begin(ctx);

    u64 elapsed_this_frame = SDL_GetTicksNS() - start_time;
    app->frame_time = (f32)elapsed_this_frame * 1.0e-6f;

    SDL_RenderPresent(app->renderer);

    return SDL_APP_CONTINUE;
}

void 
SDL_AppQuit(void *appstate, SDL_AppResult result)
{
    (void)result;
    AppState *app = appstate;
    if(!app) return;

    nk_sdl_shutdown(app->ctx);
    TTF_CloseFont(app->small_font);
    TTF_CloseFont(app->large_font);
    TTF_DestroyRendererTextEngine(app->text_engine);
    SDL_DestroyTexture(app->scratch_layer);
    SDL_DestroyRenderer(app->renderer);
    SDL_DestroyWindow(app->window);
    SDL_Quit();
}
