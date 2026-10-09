#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL_main.h>

#define TARGET_FRAME_RATE_FPS 60
#define FRAME_TARGET_TIME (1000000000 / TARGET_FRAME_RATE_FPS) /* in nanoseconds for SDL3 */
#define FONT_SIZE (12.0f)

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

/* ------------------------------------------------------ App State ------------------------------------------------------ */
typedef struct
{
    /* Base UI stuff for any SDL3 / Nuklear app using the Callback API. */
    SDL_Window   *window;
    SDL_Renderer *renderer;
    SDL_Texture *scratch_layer;
    TTF_TextEngine *text_engine;
    TTF_Font *font;
    struct nk_context *ctx;
    u64 last_frame;          /* For measuring actual FPS.    */
    f32 frame_time;          /* Time to render a frame in ms */

    /* Debug Window Information. */
    bool show_debug;
    f32 mouse_x;
    f32 mouse_y;

} AppState;

static AppState global_app_state = {0};

/* ------------------------------------------------------- Modules ------------------------------------------------------- */
#include "graphics.c"
#include "sounding.c"

#include "fonts/Roboto-Bold.h"

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

#if 0
    /* Soft buttons */
    ctx->style.button.rounding         = 6.0f;
    ctx->style.button.padding          = nk_vec2(12, 6);
    ctx->style.button.border           = 1.0f;
    ctx->style.button.normal           = nk_style_item_color(nk_rgba(40, 40, 50, 180));
    ctx->style.button.hover            = nk_style_item_color(nk_rgba(70, 70, 90, 220));
    ctx->style.button.active           = nk_style_item_color(nk_rgba(100, 100, 140, 255));
    ctx->style.button.text_normal      = nk_rgb(230, 230, 240);
    ctx->style.button.text_hover       = nk_rgb(255, 255, 255);
    ctx->style.button.text_active      = nk_rgb(255, 255, 255);
#endif
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
/* --------------------------------------------------- SDL3 Callbacks ---------------------------------------------------- */

f32 
GetCurrentWindowRefreshRate(SDL_Window *window) 
{
    /* FIX: Use SDL_GetDisplayForWindow instead of SDL_GetWindowDisplayID */
    SDL_DisplayID display_id = SDL_GetDisplayForWindow(window);
    
    if (display_id == 0)
    {
        SDL_Log("Failed to get Display for Window: %s", SDL_GetError());
        return 60.0f; /* Default fallback */
    }

    /* Fetch the active desktop configuration mode for that display */
    SDL_DisplayMode const *mode = SDL_GetDesktopDisplayMode(display_id);
    
    if (!mode)
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

    success = SDL_CreateWindowAndRenderer(
                "Sonde 2",
                1280,
                720,
                SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY,
                &app->window,
                &app->renderer);
    StopIf(!success, SDL_Log("CreateWindowAndRenderer failed: %s", SDL_GetError()); return SDL_APP_FAILURE);

    /* High-DPI handling */
    f32 scale = SDL_GetWindowDisplayScale(app->window);
    SDL_Log("Display Scale is: %f", scale);
    SDL_SetRenderScale(app->renderer, scale, scale);
    SDL_SetRenderVSync(app->renderer, 1);

    /* Set up the font. */
    app->text_engine = TTF_CreateRendererTextEngine(app->renderer);
    StopIf(!app->text_engine, SDL_Log("Failure TTF_CreateRendererTextEngine."); return SDL_APP_FAILURE);
    app->font = load_embedded_ttf_font(FONT_SIZE * scale);
    StopIf(!app->font, SDL_Log("Failure to load font."); return SDL_APP_FAILURE);

    /* Set up the scratch texture. */
    i32 w, h;
    SDL_GetCurrentRenderOutputSize(app->renderer, &w, &h);
    app->scratch_layer = SDL_CreateTexture(app->renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, w, h);

    /* Initialize Nuklear */
    app->ctx = nk_sdl_init(app->window, app->renderer, nk_sdl_allocator());

    /* Fonts for Nuklear */
    {
        struct nk_font_atlas *atlas = nk_sdl_font_stash_begin(app->ctx);
        struct nk_font *font = nk_font_atlas_add_from_memory(atlas, Roboto_Bold_ttf, Roboto_Bold_ttf_len, FONT_SIZE * 96.0 / 72.0 * scale, NULL);
        nk_sdl_font_stash_end(app->ctx);

        /* Compensate for the scale so text stays the right visual size */
        font->handle.height /= scale;
        nk_style_set_font(app->ctx, &font->handle);
    }

    /* Initialize application state. */
    app->show_debug = false;

    sounding_initialize_static_data(app);

    /* Start receiving events - required by the SDL3 backend. */
    nk_input_begin(app->ctx);

    return SDL_APP_CONTINUE;
}

SDL_AppResult 
SDL_AppEvent(void *appstate, SDL_Event *event)
{
    AppState *app = appstate;

    SDL_ConvertEventToRenderCoordinates(app->renderer, event);

    switch (event->type)
    {
        case SDL_EVENT_QUIT:
            {
                return SDL_APP_SUCCESS;
            } break;

        case SDL_EVENT_KEY_DOWN:
            {
                if (event->key.key == SDLK_ESCAPE) { return SDL_APP_SUCCESS; }
                if (event->key.key == SDLK_F1) { app->show_debug = !app->show_debug; }
            } break;

        case SDL_EVENT_MOUSE_MOTION:
            {
                app->mouse_x = event->motion.x;
                app->mouse_y = event->motion.y;
            } break;

        case SDL_EVENT_MOUSE_BUTTON_DOWN:  /* Fall through */
        case SDL_EVENT_MOUSE_BUTTON_UP:
            {
            } break;

        case SDL_EVENT_MOUSE_WHEEL:
            {
            } break;

        case SDL_EVENT_WINDOW_RESIZED:
            {
                /* Rebuild the scratch texture. */
                SDL_DestroyTexture(app->scratch_layer);
                i32 w, h;
                SDL_GetCurrentRenderOutputSize(app->renderer, &w, &h);
                app->scratch_layer = SDL_CreateTexture(app->renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, w, h);

                /* Rescale the sounding area. */
                sounding_update_static_data(app);

            } break;
    }

    nk_sdl_handle_event(app->ctx, event);

    return SDL_APP_CONTINUE;
}

SDL_AppResult 
SDL_AppIterate(void *appstate)
{
    u64 start_time = SDL_GetTicksNS();

    AppState *app = appstate;
    struct nk_context *ctx = app->ctx;

    /* Required for Nuklear to handle deltas correctly. */
    nk_input_end(ctx);

    /* ---- Clear + draw “game” background ---- */
    SDL_SetRenderDrawColor(app->renderer, 250, 250, 250, 255);
    SDL_RenderClear(app->renderer);

    /* Draw stuff */
    b32 success = true;

    /* Draw the sounding area. */
    success &= sounding_draw(app); Assert(success);

    /* ======================================================================================================================
     *                                                           HUD
     * =================================================================================================================== */
    style_hud_panel(ctx);   /* apply transparent style */

    {
        i32 w, h;
        SDL_GetRenderOutputSize(app->renderer, &w, &h);
        f32 scale_x, scale_y;
        SDL_GetRenderScale(app->renderer, &scale_x, &scale_y);
        i32 logical_w = (i32)(w / scale_x);

        f32 const bar_w = logical_w;
        f32 const bar_h = 60.0f;
        f32 x = (logical_w - bar_w) * 0.5f;
        f32 y = 0.0f;

        if(nk_begin(ctx, "Actions", nk_rect(x, y, bar_w, bar_h), NK_WINDOW_NO_SCROLLBAR | NK_WINDOW_BORDER))
        {
            nk_layout_row_dynamic(ctx, 40, 5);

            if (nk_button_label(ctx, "Load"))
            {
                SDL_Log("Load");
            }

            if (nk_button_label(ctx, "First"))
            {
                SDL_Log("First");
            }

            if (nk_button_label(ctx, "Previous"))
            {
                SDL_Log("Previous");
            }

            if (nk_button_label(ctx, "Next"))
            {
                SDL_Log("Next");
            }

            if (nk_button_label(ctx, "Last"))
            {
                SDL_Log("Last");
            }
        }
        nk_end(ctx);
    }

    /* --- Optional debug panel (toggle with F1) --- */
    if(app->show_debug)
    {
        style_debug_panel(ctx);   /* apply debug style */

        u64 time = SDL_GetTicks();
        f32 frame_rate = 1000.0f / (f32)(time - app->last_frame);
        app->last_frame = time;
        if(nk_begin(ctx, "Debug", nk_rect(300, 150, 320, 200), NK_WINDOW_BORDER | NK_WINDOW_MOVABLE | NK_WINDOW_SCALABLE | NK_WINDOW_TITLE))
        {
            nk_layout_row_dynamic(ctx, 25, 1);
            nk_labelf(ctx, NK_TEXT_LEFT, "   mouse_x: %.1f", app->mouse_x);
            nk_labelf(ctx, NK_TEXT_LEFT, "   mouse_y: %.1f", app->mouse_y);
            nk_labelf(ctx, NK_TEXT_LEFT, "frame rate: %.1f FPS", frame_rate);
            nk_labelf(ctx, NK_TEXT_LEFT, "frame time: %.1f ms [%.1f FPS]", app->frame_time, 1000.0f / app->frame_time);
        }
        nk_end(ctx);
    }

    /* ---- Draw the UI on top of everything ---- */
    nk_sdl_render(ctx, NK_ANTI_ALIASING_ON);

    /* Required for Nuklear to handle deltas correctly. */
    nk_input_begin(ctx);

    u64 elapsed = SDL_GetTicksNS() - start_time;
    app->frame_time = (f32)elapsed * 1.0e-6f;

    SDL_RenderPresent(app->renderer);

    return SDL_APP_CONTINUE;
}

void 
SDL_AppQuit(void *appstate, SDL_AppResult result)
{
    (void)result;
    AppState *app = appstate;
    if (!app) return;

    nk_sdl_shutdown(app->ctx);
    TTF_CloseFont(app->font);
    TTF_DestroyRendererTextEngine(app->text_engine);
    SDL_DestroyTexture(app->scratch_layer);
    SDL_DestroyRenderer(app->renderer);
    SDL_DestroyWindow(app->window);
    SDL_Quit();
}

