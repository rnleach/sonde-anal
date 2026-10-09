#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include <SDL3/SDL.h>

#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL_main.h>

#define TARGET_FRAME_RATE_FPS 60
#define FRAME_TARGET_TIME (1000000000 / TARGET_FRAME_RATE_FPS) /* in nanoseconds for SDL3 */

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
#include "graphics.c"

/* --------------------------------------- Helper: make a panel fully transparent ---------------------------------------- */
static void 
style_hud_panel(struct nk_context *ctx)
{
    /* Make the window background almost invisible */
    ctx->style.window.fixed_background = nk_style_item_color(nk_rgba(0, 0, 0, 120));
    ctx->style.window.background       = nk_rgba(0, 0, 0, 0);
    ctx->style.window.border_color     = nk_rgba(255, 255, 255, 255);
    ctx->style.window.border           = 1.0f;
    ctx->style.window.rounding         = 0.0f;
    ctx->style.window.padding          = nk_vec2(10, 8);
    ctx->style.window.spacing          = nk_vec2(6, 4);

    /* Soft buttons */
    ctx->style.button.rounding         = 4.0f;
    ctx->style.button.padding          = nk_vec2(12, 6);
    ctx->style.button.border           = 0.0f;
    ctx->style.button.normal           = nk_style_item_color(nk_rgba(40, 40, 50, 180));
    ctx->style.button.hover            = nk_style_item_color(nk_rgba(70, 70, 90, 220));
    ctx->style.button.active           = nk_style_item_color(nk_rgba(100, 100, 140, 255));
    ctx->style.button.text_normal      = nk_rgb(230, 230, 240);
    ctx->style.button.text_hover       = nk_rgb(255, 255, 255);
    ctx->style.button.text_active      = nk_rgb(255, 255, 255);
}

/* ------------------------------------------------------ App State ------------------------------------------------------ */
typedef struct
{
    /* Base UI stuff for any SDL3 / Nuklear app using the Callback API. */
    SDL_Window   *window;
    SDL_Renderer *renderer;
    SDL_Texture *scratch_layer;
    struct nk_context *ctx;
    u64 last_frame;          /* For measuring actual FPS.    */
    f32 frame_time;          /* Time to render a frame in ms */

    /* Debug Window Information. */
    bool show_debug;
    f32 mouse_x;
    f32 mouse_y;

} AppState;

static AppState global_app_state = {0};

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
    (void)argc; (void)argv;

    /* Prefer wayland if possible on linux to get rid of flicker when resizing. */
    SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "wayland,x11");

    b32 success = SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS);
    StopIf(!success, SDL_Log("SDL_Init failed: %s", SDL_GetError()); return SDL_APP_FAILURE);

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
    float scale = SDL_GetWindowDisplayScale(app->window);
    SDL_SetRenderScale(app->renderer, scale, scale);
    SDL_SetRenderVSync(app->renderer, 1);

    i32 w, h;
    SDL_GetCurrentRenderOutputSize(app->renderer, &w, &h);
    app->scratch_layer = SDL_CreateTexture(app->renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, w, h);

    /* Init Nuklear */
    app->ctx = nk_sdl_init(app->window, app->renderer, nk_sdl_allocator());

    /* Fonts */
    {
        struct nk_font_atlas *atlas = nk_sdl_font_stash_begin(app->ctx);
        struct nk_font *font = nk_font_atlas_add_default(atlas, 16.0f * scale, NULL);
        nk_sdl_font_stash_end(app->ctx);

        /* Compensate for the scale so text stays the right visual size */
        font->handle.height /= scale;
        nk_style_set_font(app->ctx, &font->handle);
    }

    /* Initialize application state. */
    app->show_debug = false;

    SDL_Log("Frame Rate: %.2f", GetCurrentWindowRefreshRate(app->window));

    nk_input_begin(app->ctx);   /* required by the backend */

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

            } break;
    }

    nk_sdl_handle_event(app->ctx, event);

    return SDL_APP_CONTINUE;
}

static byte buffer[ECO_MiB(16)] = {0};

SDL_AppResult 
SDL_AppIterate(void *appstate)
{
    MagAllocator scratch = mag_allocator_static_arena_create(sizeof(buffer), buffer);

    u64 start_time = SDL_GetTicksNS();

    AppState *app = appstate;
    struct nk_context *ctx = app->ctx;

    /* Required for Nuklear to handle deltas correctly. */
    nk_input_end(ctx);

    /* ---- Clear + draw “game” background ---- */
    SDL_SetRenderDrawColor(app->renderer, 18, 28, 42, 255);
    SDL_RenderClear(app->renderer);

    /* Draw stuff */
    b32 success = true;

    SDL_FPoint start = { .x=100.0f, .y=100.0f };
    SDL_FPoint end   = { .x=200.0f, .y=200.0f };

    SDL_FColor const white  = { .r=1.0f, .g=1.0f, .b=1.0f, .a=1.0f };
    SDL_FColor const white2 = { .r=1.0f, .g=1.0f, .b=1.0f, .a=0.5f };
    SDL_FColor const green  = { .r=0.0f, .g=1.0f, .b=0.0f, .a=0.75f };
    SDL_FColor const red    = { .r=1.0f, .g=0.0f, .b=0.0f, .a=0.45f };
    SDL_FColor const red2   = { .r=1.0f, .g=0.0f, .b=0.0f, .a=1.00f };
    SDL_FColor const yellow = { .r=1.0f, .g=1.0f, .b=0.0f, .a=0.25f };

    success &= DrawLineThick(app->renderer, start, end, 10.0f, white); Assert(success);

    start = (SDL_FPoint){ .x=200.0f, .y=200.0f };
    end   = (SDL_FPoint){ .x=400.0f, .y=100.0f };
    success &= DrawLineThick(app->renderer, start, end, 10.0, white); Assert(success);

    start = (SDL_FPoint){ .x=180.0f, .y=125.0f };
    success &= DrawPointRound(app->renderer, start, 30.0, green); Assert(success);

    start = (SDL_FPoint){ .x=150.0f, .y=125.0f };
    end   = (SDL_FPoint){ .x=450.0f, .y=159.0f };
    success &= DrawLineThickRoundedAA(app->renderer, start, end, 11.0, red); Assert(success);

    SDL_FPoint points[5] = {0};
    points[0].x =  50.0; points[0].y = 300.0;
    points[1].x = 100.0; points[1].y = 500.0;
    points[2].x = 200.0; points[2].y = 600.0;
    points[3].x = 400.0; points[3].y = 200.0;
    points[4].x = 800.0; points[4].y = 500.0;

    success &= DrawPolylineThick(app->renderer, points, 5, 5.0, white, scratch); Assert(success);

    success &= DrawPolylineSmoothThick(app->renderer, points, 5, 30.0, white2, scratch); Assert(success);

    points[0].x =  65.0; points[0].y = 335.0;
    points[1].x = 115.0; points[1].y = 535.0;
    points[2].x = 215.0; points[2].y = 635.0;
    points[3].x = 415.0; points[3].y = 235.0;
    points[4].x = 815.0; points[4].y = 535.0;

    success &= DrawPolylineFilletedThick(app->renderer, app->scratch_layer, points, 5, 50.0, red, scratch); Assert(success);

    points[0].x = 850.0; points[0].y = 135.0;
    points[1].x = 750.0; points[1].y = 235.0;
    points[2].x = 750.0; points[2].y = 335.0;
    points[3].x = 950.0; points[3].y = 335.0;
    points[4].x = 950.0; points[4].y = 235.0;

    success &= DrawPolygonSmoothThick(app->renderer, app->scratch_layer, points, 5, 20.0, red, yellow, scratch); Assert(success);

    points[0].x =  850.0; points[0].y = 235.0;
    points[1].x =  550.0; points[1].y = 535.0;
    points[2].x =  550.0; points[2].y = 835.0;
    points[3].x = 1150.0; points[3].y = 835.0;
    points[4].x = 1150.0; points[4].y = 535.0;

    success &= DrawPolygonThick(app->renderer, points, 5, 25.0, red2, yellow, scratch); Assert(success);

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
    SDL_DestroyTexture(app->scratch_layer);
    SDL_DestroyRenderer(app->renderer);
    SDL_DestroyWindow(app->window);
    SDL_Quit();
}

