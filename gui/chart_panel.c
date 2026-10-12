/* This file includes the code for building aa zoomable, panable chart.                                                    */

/* --------------------------------------------------------- API --------------------------------------------------------- */
typedef struct
{
    /* Screen area properties. */
    SDL_Rect clipping;      /* x & y are upper left corner, w & h are width and height, all i32                 */
    f32 chart_w, chart_h;   /* The chart dimensions in screen coordinates.                                      */
    f32 y_offset, x_offset; /* These are used to center the chart after accounting for different aspect ratios. */

    b32 needs_background_update; /* If the zoom, pan, or window size change, this needs updated. */

    /* Aspect ratio for data - always plot the chart with this aspect ratio. */
    f32 ard;

    /* Pan and zoom data */
    SDL_FPoint pan;
    f32 zf;

} ChartPanel;

static inline ChartPanel chart_panel_create(i32 ulx, i32 uly, i32 w, i32 h, f32 aspect_ratio);
static inline void chart_panel_update_for_screen_resize(ChartPanel *cp, i32 ulx, i32 uly, i32 w, i32 h);
static inline SDL_FPoint chart_panel_apply_pan_and_zoom(SDL_FPoint p, ChartPanel *cp);
static inline SDL_FPoint chart_panel_unapply_pan_and_zoom(SDL_FPoint p, ChartPanel *cp);
static inline SDL_FPoint chart_panel_convert_to_screen_coords(SDL_FPoint p, ChartPanel *cp);
static inline SDL_FPoint chart_panel_convert_from_screen_coords(SDL_FPoint p, ChartPanel *cp);
static inline void chart_panel_handle_zoom(ChartPanel *cp, SDL_FPoint focus_point, f32 zoom_factor);
static inline void chart_panel_pan(ChartPanel *cp, SDL_FPoint from, SDL_FPoint to);

/* --------------------------------------------------- Implementations --------------------------------------------------- */
static inline ChartPanel
chart_panel_create(i32 ulx, i32 uly, i32 w, i32 h, f32 aspect_ratio)
{
    ChartPanel cp = {0};

    cp.clipping.x = ulx; cp.clipping.y = uly; cp.clipping.w = w; cp.clipping.h = h;
    f32 ars = (f32)w / (f32)h;

    cp.ard = aspect_ratio;

    cp.chart_w = ars > cp.ard ? (f32)h * cp.ard : (f32)w;
    cp.chart_h = ars < cp.ard ? (f32)w / cp.ard : (f32)h;
    cp.x_offset = ((f32)w - cp.chart_w) * 0.50f;
    cp.y_offset = ((f32)h - cp.chart_h) * 0.50f;

    cp.pan = (SDL_FPoint) { .x = 0.0f, .y = 0.0f };
    cp.zf = 1.0f;

    return cp;
}

static inline void
chart_panel_update_for_screen_resize(ChartPanel *cp, i32 ulx, i32 uly, i32 w, i32 h)
{
    cp->clipping.x = ulx; cp->clipping.y = uly; cp->clipping.w = w; cp->clipping.h = h;
    f32 ars = (f32)w / (f32)h;

    cp->chart_w = ars > cp->ard ? (f32)h * cp->ard : (f32)w;
    cp->chart_h = ars < cp->ard ? (f32)w / cp->ard : (f32)h;
    cp->x_offset = ((f32)w - cp->chart_w) * 0.50f;
    cp->y_offset = ((f32)h - cp->chart_h) * 0.50f;
}

static inline SDL_FPoint
chart_panel_apply_pan_and_zoom(SDL_FPoint p, ChartPanel *cp)
{
    /* pan */
    p.x += cp->pan.x;
    p.y += cp->pan.y;

    /* zoom */
    p.x = (p.x - 0.5f) * cp->zf + 0.5f;
    p.y = (p.y - 0.5f) * cp->zf + 0.5f;

    return p;
}

static inline SDL_FPoint
chart_panel_unapply_pan_and_zoom(SDL_FPoint p, ChartPanel *cp)
{
    /* unzoom */
    p.x = (p.x - 0.5f) / cp->zf + 0.5f;
    p.y = (p.y - 0.5f) / cp->zf + 0.5f;

    /* pan */
    p.x -= cp->pan.x;
    p.y -= cp->pan.y;

    return p;
}

static inline SDL_FPoint
chart_panel_convert_to_screen_coords(SDL_FPoint p, ChartPanel *cp)
{
    p.x = p.x * cp->chart_w + cp->x_offset;
    p.y = p.y * cp->chart_h + cp->y_offset;

    return p;
}

static inline SDL_FPoint
chart_panel_convert_from_screen_coords(SDL_FPoint p, ChartPanel *cp)
{
    p.x = (p.x - cp->x_offset) / cp->chart_w;
    p.y = (p.y - cp->y_offset) / cp->chart_h;

    return p;
}

static inline void
chart_panel_clamp_pan(ChartPanel *cp)
{
    f32 upper_limit = 1.0f / (2.0f * cp->zf) * (cp->zf - 1.0f);
    f32 lower_limit = 1.0f / (2.0f * cp->zf) * (1.0f - cp->zf);

    cp->pan.x = cp->pan.x < lower_limit ? lower_limit : cp->pan.x;
    cp->pan.x = cp->pan.x > upper_limit ? upper_limit : cp->pan.x;

    cp->pan.y = cp->pan.y < lower_limit ? lower_limit : cp->pan.y;
    cp->pan.y = cp->pan.y > upper_limit ? upper_limit : cp->pan.y;
}

static inline void
chart_panel_handle_zoom(ChartPanel *cp, SDL_FPoint focus_point, f32 zoom_factor)
{
    SDL_FPoint chart_loc = chart_panel_convert_from_screen_coords(focus_point, cp);

    f32 zf1 = cp->zf;
    f32 zf2 = zf1 * zoom_factor;
    zf2 = zf2 < 1.0f ? 1.0f : zf2 > 100.0f ? 100.0f : zf2;

    f32 dx1 = cp->pan.x;
    f32 dy1 = cp->pan.y;

    f32 x = chart_loc.x;
    f32 y = chart_loc.y;

    f32 zfac = (1.0f / zf2 - 1.0f / zf1);
    f32 dx2 = zfac * (x - 0.5f) + dx1;
    f32 dy2 = zfac * (y - 0.5f) + dy1;

    cp->zf = zf2;
    cp->pan.x = dx2;
    cp->pan.y = dy2;

    chart_panel_clamp_pan(cp);
}

static inline void 
chart_panel_pan(ChartPanel *cp, SDL_FPoint from, SDL_FPoint to)
{
    from = chart_panel_convert_from_screen_coords(from, cp);
    to = chart_panel_convert_from_screen_coords(to, cp);

    f32 dx = (to.x - from.x) / cp->zf;
    f32 dy = (to.y - from.y) / cp->zf;

    cp->pan.x += dx;
    cp->pan.y += dy;

    chart_panel_clamp_pan(cp);
}

