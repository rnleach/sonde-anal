/* This file has everything for drawing the profiles. */

/* --------------------------------------------------------- API --------------------------------------------------------- */
static inline b32 profiles_initialize_static_data(AppState *app);
static inline b32 profiles_draw(AppState *app);

/* ----------------------------------------------- Isopleth Configuration ------------------------------------------------ */

/* iso-relative humidity */
static f32 const iso_rh[] = { 0.0f, 0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.6f, 0.7f, 0.8f, 0.9f, 1.0f };
static TTF_Text *iso_rh_lables[ECO_ARRAY_SIZE(iso_rh)] = {0};

/* isotachs */
static f32 const isotachs[] = { 1.0f, 2.0f, 4.0f, 6.0f, 8.0f, 10.0f, 20.0f, 40.0f, 60.0f, 80.0f, 100.0f, 200.0f };
static TTF_Text *isotach_lables[ECO_ARRAY_SIZE(isotachs)] = {0};

/* --------------------------------------------------- Implementations --------------------------------------------------- */

static inline b32 
profiles_initialize_static_data(AppState *app)
{
    b32 success = true;

    /* Set up iso-rh labels */
    for(i32 i = 0; i < ECO_ARRAY_SIZE(iso_rh); ++i)
    {
        char cbuf[32] = {0};
        snprintf(cbuf, sizeof(cbuf), "%.0f%%", iso_rh[i] * 100.0f);
        iso_rh_lables[i] = TTF_CreateText(app->text_engine, app->small_font, cbuf, 0);
        TTF_SetTextColorFloat(iso_rh_lables[i], isobar_color.r, isobar_color.g, isobar_color.b, isobar_color.a);
    }

    /* Set up isotach labels */
    for(i32 i = 0; i < ECO_ARRAY_SIZE(isotachs); ++i)
    {
        char cbuf[32] = {0};
        snprintf(cbuf, sizeof(cbuf), "%.0f", isotachs[i]);
        isotach_lables[i] = TTF_CreateText(app->text_engine, app->small_font, cbuf, 0);
        TTF_SetTextColorFloat(isotach_lables[i], isobar_color.r, isobar_color.g, isobar_color.b, isobar_color.a);
    }

    return success;
}

static inline SDL_FPoint
rh_profile_pressure_rh_to_chart_coords(SondeHectopascal p, f32 rh)
{
    f32 y = (logf(p.val) - logf(P_MIN)) / (logf(P_MAX) - logf(P_MIN));
    f32 x = rh;

    return (SDL_FPoint){ .x = x, .y = y };
}

static inline SDL_FPoint
pvv_profile_pressure_pvv_to_chart_coords(SondeHectopascal p, SondeHectopascalPerSecond pvv)
{
    f32 const pvv_max = 5.0f;
    f32 y = (logf(p.val) - logf(P_MIN)) / (logf(P_MAX) - logf(P_MIN));
    f32 x =  pvv.val * 100.0f / pvv_max / 2.0f + 0.5f;

    return (SDL_FPoint){ .x = x, .y = y };
}

static inline b32
profiles_draw_rh_pvv_background(AppState *app)
{
    VChartPanel *cp = &app->rh_profile;
    MagAllocator alloc_ = app->scratch_alloc;

    b32 success = true;

    /* Draw the filled bands in the background. */
    for(i32 i = 0; i < ECO_ARRAY_SIZE(iso_rh) - 1; i += 2)
    {
        SDL_FPoint pnts[4] = {0};
        pnts[0] = rh_profile_pressure_rh_to_chart_coords((SondeHectopascal){ .val = P_MAX }, iso_rh[i + 0]);
        pnts[1] = rh_profile_pressure_rh_to_chart_coords((SondeHectopascal){ .val = P_MAX }, iso_rh[i + 1]);
        pnts[2] = rh_profile_pressure_rh_to_chart_coords((SondeHectopascal){ .val = P_MIN }, iso_rh[i + 1]);
        pnts[3] = rh_profile_pressure_rh_to_chart_coords((SondeHectopascal){ .val = P_MIN }, iso_rh[i + 0]);

        pnts[0] = vchart_panel_apply_pan_and_zoom(pnts[0], cp);
        pnts[1] = vchart_panel_apply_pan_and_zoom(pnts[1], cp);
        pnts[2] = vchart_panel_apply_pan_and_zoom(pnts[2], cp);
        pnts[3] = vchart_panel_apply_pan_and_zoom(pnts[3], cp);

        pnts[0] = vchart_panel_convert_to_screen_coords(pnts[0], cp);
        pnts[1] = vchart_panel_convert_to_screen_coords(pnts[1], cp);
        pnts[2] = vchart_panel_convert_to_screen_coords(pnts[2], cp);
        pnts[3] = vchart_panel_convert_to_screen_coords(pnts[3], cp);

        success &= DrawPolygonThick(app->renderer, pnts, 4, 0.0f, isotherm_band_color, isotherm_band_color, alloc_);
    }

    for(i32 l = 0; l < ECO_ARRAY_SIZE(isobars); ++l)
    {
        SDL_FPoint p1 = rh_profile_pressure_rh_to_chart_coords((SondeHectopascal){ .val = isobars[l] }, 0.0f);
        p1 = vchart_panel_apply_pan_and_zoom(p1, cp);
        p1 = vchart_panel_convert_to_screen_coords(p1, cp);
        SDL_FPoint p2 = rh_profile_pressure_rh_to_chart_coords((SondeHectopascal){ .val = isobars[l] }, 1.0f);
        p2 = vchart_panel_apply_pan_and_zoom(p2, cp);
        p2 = vchart_panel_convert_to_screen_coords(p2, cp);

        success &= DrawLineThickRoundedAA(app->renderer, p1, p2, isobar_width, isobar_color); Assert(success);
    }

    for(i32 i = 0; i < ECO_ARRAY_SIZE(iso_rh); ++i)
    {
        SDL_FPoint p1 = rh_profile_pressure_rh_to_chart_coords((SondeHectopascal){ .val = P_MIN }, iso_rh[i]);
        p1 = vchart_panel_apply_pan_and_zoom(p1, cp);
        p1 = vchart_panel_convert_to_screen_coords(p1, cp);
        SDL_FPoint p2 = rh_profile_pressure_rh_to_chart_coords((SondeHectopascal){ .val = P_MAX }, iso_rh[i]);
        p2 = vchart_panel_apply_pan_and_zoom(p2, cp);
        p2 = vchart_panel_convert_to_screen_coords(p2, cp);

        success &= DrawLineThickRoundedAA(app->renderer, p1, p2, isobar_width, isobar_color); Assert(success);
    }
    
    /* Label the iso-rhs */
    f32 const padding = 3.0f;
    for(i32 i = 1; i < ECO_ARRAY_SIZE(iso_rh); i += 2)
    {
        SDL_FPoint pnt = rh_profile_pressure_rh_to_chart_coords((SondeHectopascal){ .val = P_MAX }, iso_rh[i]);
        pnt = vchart_panel_apply_pan_and_zoom(pnt, cp);
        pnt = vchart_panel_convert_to_screen_coords(pnt, cp);
        pnt.y = pnt.y > cp->area.y + cp->area.h ? cp->area.y + cp->area.h : pnt.y;

        i32 tw, th;
        TTF_GetTextSize(iso_rh_lables[i], &tw, &th);
        pnt.y -= (f32)th + padding;
        pnt.x -= (f32)(tw / 2);
        SDL_FRect trect = { .x = pnt.x, .y = pnt.y - padding, .w = (f32)(tw) + 2.0f * padding, .h = (f32)(th) + 2.0f * padding };
        SDL_SetRenderDrawColorFloat(app->renderer, app->background.r, app->background.g, app->background.b, app->background.a);
        SDL_RenderFillRect(app->renderer, &trect);
        TTF_DrawRendererText(iso_rh_lables[i], pnt.x + padding, pnt.y + padding);
    }

    return success;
}

static inline b32 
profiles_draw_rh_pvv_data(AppState *app)
{
    b32 success = true;

    VChartPanel *cp = &app->rh_profile;
    SondeSounding *snd = app->sndg_amalgams[app->current_sndg].sndg;
    MagAllocator scratch_ = app->scratch_alloc;
    MagAllocator *scratch = &scratch_;

    size n_levels = pak_len(&snd->levels);
    f64 *rhs = snd->rh;
    f64 *rh_ices = snd->rh_ice;
    SondeHectopascal *ps = snd->p;

    SDL_FPoint pnts[4] = {0};

    for(size l = 0; l < n_levels - 1; ++l)
    {
        SondeHectopascal p0 = ps[l + 0];
        SondeHectopascal p1 = ps[l + 1];
        f64 rh0 = rhs[l + 0];
        f64 rh1 = rhs[l + 1];
        f64 rh_ice0 = rh_ices[l + 0];
        f64 rh_ice1 = rh_ices[l + 1];

        b32 proceed = !isnan(p0.val) && !isnan(p1.val) && !isnan(rh0) && !isnan(rh1);
        if(proceed)
        {
            pnts[0] = rh_profile_pressure_rh_to_chart_coords(p0, rh0);
            pnts[1] = rh_profile_pressure_rh_to_chart_coords(p1, rh1);
            pnts[2] = rh_profile_pressure_rh_to_chart_coords(p1, 0.0f);
            pnts[3] = rh_profile_pressure_rh_to_chart_coords(p0, 0.0f);

            pnts[0] = vchart_panel_apply_pan_and_zoom(pnts[0], cp);
            pnts[1] = vchart_panel_apply_pan_and_zoom(pnts[1], cp);
            pnts[2] = vchart_panel_apply_pan_and_zoom(pnts[2], cp);
            pnts[3] = vchart_panel_apply_pan_and_zoom(pnts[3], cp);

            pnts[0] = vchart_panel_convert_to_screen_coords(pnts[0], cp);
            pnts[1] = vchart_panel_convert_to_screen_coords(pnts[1], cp);
            pnts[2] = vchart_panel_convert_to_screen_coords(pnts[2], cp);
            pnts[3] = vchart_panel_convert_to_screen_coords(pnts[3], cp);

            success &= DrawPolygonThick(app->renderer, pnts, 4, 0.0f, rh_color, rh_color, scratch_);

            if(!isnan(rh_ice0) && !isnan(rh_ice1))
            {
                pnts[0] = rh_profile_pressure_rh_to_chart_coords(p0, rh_ice0);
                pnts[1] = rh_profile_pressure_rh_to_chart_coords(p1, rh_ice1);
                pnts[2] = rh_profile_pressure_rh_to_chart_coords(p1, rh1);
                pnts[3] = rh_profile_pressure_rh_to_chart_coords(p0, rh0);

                pnts[0] = vchart_panel_apply_pan_and_zoom(pnts[0], cp);
                pnts[1] = vchart_panel_apply_pan_and_zoom(pnts[1], cp);
                pnts[2] = vchart_panel_apply_pan_and_zoom(pnts[2], cp);
                pnts[3] = vchart_panel_apply_pan_and_zoom(pnts[3], cp);

                pnts[0] = vchart_panel_convert_to_screen_coords(pnts[0], cp);
                pnts[1] = vchart_panel_convert_to_screen_coords(pnts[1], cp);
                pnts[2] = vchart_panel_convert_to_screen_coords(pnts[2], cp);
                pnts[3] = vchart_panel_convert_to_screen_coords(pnts[3], cp);

                success &= DrawPolygonThick(app->renderer, pnts, 4, 0.0f, rh_ice_color, rh_ice_color, scratch_);
            }
        }
    }

    SondeHectopascalPerSecond *pvvs = snd->pvv;
    SDL_FPoint *pvv_pnts = eco_arena_nmalloc(scratch, n_levels, SDL_FPoint); Assert(pvv_pnts);
    PakArrayLedger pvv_ledger = pak_array_ledger_create(n_levels);

    for(size l = 0; l < n_levels; ++l)
    {
        SondeHectopascal p = ps[l];
        SondeHectopascalPerSecond pvv = pvvs[l];
        
        if(!isnan(p.val) && !isnan(pvv.val))
        {
            SDL_FPoint pnt = pvv_profile_pressure_pvv_to_chart_coords(p, pvv);
            pnt = vchart_panel_apply_pan_and_zoom(pnt, cp);
            pnt = vchart_panel_convert_to_screen_coords(pnt, cp);

            size idx = pak_array_ledger_push_back_index(&pvv_ledger);
            pvv_pnts[idx] = pnt;
        }
    }

    success &= DrawPolylineThickAA(app->renderer, pvv_pnts, pak_len(&pvv_ledger), pvv_width, pvv_color, scratch_);

    return success;
}

static inline b32
profiles_draw_rh_pvv_sample(AppState *app)
{
    VChartPanel *cp = &app->rh_profile;
    b32 success = true;

    b32 do_draw_sample = app->sample.type == SAMPLE_TYPE_SKEWT && chart_panel_point_inside(&app->skewt, app->mouse_loc);

    if(do_draw_sample)
    {
        SondeHectopascal p = app->sample.row.p;
        SDL_FPoint p1 = rh_profile_pressure_rh_to_chart_coords(p, 0.0);
        p1 = vchart_panel_apply_pan_and_zoom(p1, cp);
        p1 = vchart_panel_convert_to_screen_coords(p1, cp);
        SDL_FPoint p2 = rh_profile_pressure_rh_to_chart_coords(p, 1.0);
        p2 = vchart_panel_apply_pan_and_zoom(p2, cp);
        p2 = vchart_panel_convert_to_screen_coords(p2, cp);

        success &= DrawLineThickRoundedAA(app->renderer, p1, p2, sample_bar_width, sample_bar_color); Assert(success);
    }

    return success;
}

static inline b32
profiles_draw_rh_pvv_legend(AppState *app)
{
    b32 success = true;

    VChartPanel *cp = &app->rh_profile;
    MagAllocator alloc_ = app->scratch_alloc;

    f32 const padding = 6.0f;
    f32 const border_width = 3.0f;

    SDL_FPoint legend_ul = { .x = cp->area.x + 10.0f, .y = cp->area.y + 20.0f };
    SDL_FPoint legend_lr = { .x = legend_ul.x + padding, .y = legend_ul.y + padding };
    i32 tw, th;

    TTF_Text *rh_water_txt = TTF_CreateText(app->text_engine, app->large_font, "RH Water (%)", 0);
    TTF_SetTextColorFloat(rh_water_txt, rh_color.r, rh_color.g, rh_color.b, rh_color.a);
    TTF_GetTextSize(rh_water_txt, &tw, &th);
    SDL_FPoint rh_txt_start = legend_lr;
    legend_lr.y += (f32)th;
    legend_lr.x = legend_ul.x + 2.0f * padding + (f32)tw < legend_lr.x ? legend_lr.x : legend_ul.x + 2.0f * padding +(f32)tw;

    TTF_Text *rh_ice_txt = TTF_CreateText(app->text_engine, app->large_font, "RH Ice (%)", 0);
    TTF_SetTextColorFloat(rh_ice_txt, rh_ice_color.r, rh_ice_color.g, rh_ice_color.b, rh_ice_color.a);
    TTF_GetTextSize(rh_ice_txt, &tw, &th);
    SDL_FPoint rh_ice_txt_start = legend_lr;
    rh_ice_txt_start.x = legend_ul.x + padding;
    legend_lr.y += (f32)th;
    legend_lr.x = legend_ul.x + 2.0f * padding + (f32)tw < legend_lr.x ? legend_lr.x : legend_ul.x + 2.0f * padding +(f32)tw;

    TTF_Text *pvv_txt = TTF_CreateText(app->text_engine, app->large_font, "PVV (Pa/s)", 0);
    TTF_SetTextColorFloat(pvv_txt, pvv_color.r, pvv_color.g, pvv_color.b, pvv_color.a);
    TTF_GetTextSize(pvv_txt, &tw, &th);
    SDL_FPoint pvv_txt_start = legend_lr;
    pvv_txt_start.x = legend_ul.x + padding;
    legend_lr.y += (f32)th;
    legend_lr.x = legend_ul.x + 2.0f * padding + (f32)tw < legend_lr.x ? legend_lr.x : legend_ul.x + 2.0f * padding +(f32)tw;

    legend_lr.y += padding;

    SDL_FPoint border[4] = {0};
    border[0] = legend_ul;
    border[1] = (SDL_FPoint){ .x = legend_ul.x, .y = legend_lr.y };
    border[2] = legend_lr;
    border[3] = (SDL_FPoint){ .x = legend_lr.x, .y = legend_ul.y };

    success &= DrawPolygonThick(app->renderer, border, 4, border_width, isobar_color, app->background, alloc_);

    TTF_DrawRendererText(rh_water_txt, rh_txt_start.x, rh_txt_start.y);
    TTF_DrawRendererText(rh_ice_txt, rh_ice_txt_start.x, rh_ice_txt_start.y);
    TTF_DrawRendererText(pvv_txt, pvv_txt_start.x, pvv_txt_start.y);

    TTF_DestroyText(pvv_txt);
    TTF_DestroyText(rh_ice_txt);
    TTF_DestroyText(rh_water_txt);

    return success;
}

static inline b32 
profiles_draw_rh_pvv(AppState *app)
{
    b32 success = true;

    VChartPanel *rh_pvv_cp = &app->rh_profile;

    SDL_SetRenderClipRect(app->renderer, &rh_pvv_cp->area);

    success &= profiles_draw_rh_pvv_background(app); Assert(success);

    if(app->current_sndg >= 0)
    {
        success &= profiles_draw_rh_pvv_data(app);   Assert(success);
        success &= profiles_draw_rh_pvv_sample(app); Assert(success);
    }

    profiles_draw_rh_pvv_legend(app);

    /* Draw a border around the RH-PVV panel. */
    SDL_FPoint pnts[4] = {0};
    pnts[0] = (SDL_FPoint){ .x = rh_pvv_cp->area.x, .y = rh_pvv_cp->area.y };
    pnts[1] = (SDL_FPoint){ .x = rh_pvv_cp->area.x, .y = rh_pvv_cp->area.y + rh_pvv_cp->area.h };
    pnts[2] = (SDL_FPoint){ .x = rh_pvv_cp->area.x + rh_pvv_cp->area.w, .y = rh_pvv_cp->area.y + rh_pvv_cp->area.h };
    pnts[3] = (SDL_FPoint){ .x = rh_pvv_cp->area.x + rh_pvv_cp->area.w, .y = rh_pvv_cp->area.y };

    MagAllocator scratch = app->scratch_alloc;
    success &= DrawPolygonThick(app->renderer, pnts, 4, 4.0f, isobar_color, (SDL_FColor){0}, scratch);

    SDL_SetRenderClipRect(app->renderer, NULL);

    return success;
}

static inline SDL_FPoint
wind_profile_pressure_kt_to_chart_coords(SondeHectopascal p, SondeSpdDirKts w)
{
    f32 const MAX_WS = 200.0f;

    f32 y = (logf((f32)p.val) - logf((f32)P_MIN)) / (logf((f32)P_MAX) - logf((f32)P_MIN));
    f32 x = logf((f32)w.spd) / logf(MAX_WS);
    x = x < 0.0f ? 0.0f : x;

    return (SDL_FPoint){ .x = x, .y = y };
}

static inline b32
profiles_draw_wind_background(AppState *app)
{
    VChartPanel *cp = &app->wind_profile;
    MagAllocator alloc_ = app->scratch_alloc;

    b32 success = true;

    /* Draw the filled bands in the background. */
    for(i32 i = 0; i < ECO_ARRAY_SIZE(isotachs) - 1; i += 2)
    {
        SDL_FPoint pnts[4] = {0};
        pnts[0] = wind_profile_pressure_kt_to_chart_coords((SondeHectopascal){ .val = P_MAX }, (SondeSpdDirKts){ .dir = 0.0, .spd = isotachs[i + 0] });
        pnts[1] = wind_profile_pressure_kt_to_chart_coords((SondeHectopascal){ .val = P_MAX }, (SondeSpdDirKts){ .dir = 0.0, .spd = isotachs[i + 1] });
        pnts[2] = wind_profile_pressure_kt_to_chart_coords((SondeHectopascal){ .val = P_MIN }, (SondeSpdDirKts){ .dir = 0.0, .spd = isotachs[i + 1] });
        pnts[3] = wind_profile_pressure_kt_to_chart_coords((SondeHectopascal){ .val = P_MIN }, (SondeSpdDirKts){ .dir = 0.0, .spd = isotachs[i + 0] });

        pnts[0] = vchart_panel_apply_pan_and_zoom(pnts[0], cp);
        pnts[1] = vchart_panel_apply_pan_and_zoom(pnts[1], cp);
        pnts[2] = vchart_panel_apply_pan_and_zoom(pnts[2], cp);
        pnts[3] = vchart_panel_apply_pan_and_zoom(pnts[3], cp);

        pnts[0] = vchart_panel_convert_to_screen_coords(pnts[0], cp);
        pnts[1] = vchart_panel_convert_to_screen_coords(pnts[1], cp);
        pnts[2] = vchart_panel_convert_to_screen_coords(pnts[2], cp);
        pnts[3] = vchart_panel_convert_to_screen_coords(pnts[3], cp);

        success &= DrawPolygonThick(app->renderer, pnts, 4, 0.0f, isotherm_band_color, isotherm_band_color, alloc_);
    }

    for(i32 l = 0; l < ECO_ARRAY_SIZE(isobars); ++l)
    {
        SDL_FPoint p1 = wind_profile_pressure_kt_to_chart_coords((SondeHectopascal){ .val = isobars[l] }, (SondeSpdDirKts){ .dir = 0.0f });
        p1.x = 0.0f;
        p1 = vchart_panel_apply_pan_and_zoom(p1, cp);
        p1 = vchart_panel_convert_to_screen_coords(p1, cp);

        SDL_FPoint p2 = wind_profile_pressure_kt_to_chart_coords((SondeHectopascal){ .val = isobars[l] }, (SondeSpdDirKts){ .dir = 1.0f });
        p2.x = 1.0f;
        p2 = vchart_panel_apply_pan_and_zoom(p2, cp);
        p2 = vchart_panel_convert_to_screen_coords(p2, cp);

        success &= DrawLineThickRoundedAA(app->renderer, p1, p2, isobar_width, isobar_color); Assert(success);
    }

    for(i32 i = 0; i < ECO_ARRAY_SIZE(isotachs); ++i)
    {
        SDL_FPoint p1 = wind_profile_pressure_kt_to_chart_coords((SondeHectopascal){ .val = P_MIN }, (SondeSpdDirKts){ .dir = 0.0, .spd = isotachs[i] });
        p1 = vchart_panel_apply_pan_and_zoom(p1, cp);
        p1 = vchart_panel_convert_to_screen_coords(p1, cp);

        SDL_FPoint p2 = wind_profile_pressure_kt_to_chart_coords((SondeHectopascal){ .val = P_MAX }, (SondeSpdDirKts){ .dir = 0.0, .spd = isotachs[i] });
        p2 = vchart_panel_apply_pan_and_zoom(p2, cp);
        p2 = vchart_panel_convert_to_screen_coords(p2, cp);

        success &= DrawLineThickRoundedAA(app->renderer, p1, p2, isobar_width, isobar_color); Assert(success);
    }
    
    /* Label the isotachs */
    f32 const padding = 3.0f;
    for(i32 i = 0; i < ECO_ARRAY_SIZE(isotachs); i += 1)
    {
        f32 isotach = isotachs[i];

        /* Skip troublesome values for labels */
        if(isotach == 8.0f || isotach == 80.0f) { continue; }

        SDL_FPoint pnt = wind_profile_pressure_kt_to_chart_coords((SondeHectopascal){ .val = P_MAX }, (SondeSpdDirKts){ .dir = 0.0, .spd = isotach });
        pnt = vchart_panel_apply_pan_and_zoom(pnt, cp);
        pnt = vchart_panel_convert_to_screen_coords(pnt, cp);
        pnt.y = pnt.y > cp->area.y + cp->area.h ? cp->area.y + cp->area.h : pnt.y;

        i32 tw, th;
        TTF_GetTextSize(isotach_lables[i], &tw, &th);
        pnt.y -= (f32)th + padding;
        pnt.x -= (f32)(tw / 2);
        SDL_FRect trect = { .x = pnt.x, .y = pnt.y - padding, .w = (f32)(tw) + 2.0f * padding, .h = (f32)(th) + 2.0f * padding };
        SDL_SetRenderDrawColorFloat(app->renderer, app->background.r, app->background.g, app->background.b, app->background.a);
        SDL_RenderFillRect(app->renderer, &trect);
        TTF_DrawRendererText(isotach_lables[i], pnt.x + padding, pnt.y + padding);
    }

    return success;
}

static inline b32 
profiles_draw_wind_data(AppState *app)
{
    b32 success = true;

    VChartPanel *cp = &app->wind_profile;
    SondeSounding *snd = app->sndg_amalgams[app->current_sndg].sndg;
    MagAllocator scratch_ = app->scratch_alloc;
    MagAllocator *scratch = &scratch_;

    size n_levels = pak_len(&snd->levels);
    SondeSpdDirKts *winds = snd->wind;
    SondeHectopascal *ps = snd->p;

    SDL_FPoint *pnts = eco_arena_nmalloc(scratch, n_levels, SDL_FPoint);
    PakArrayLedger ledger = pak_array_ledger_create(n_levels);

    for(size l = 0; l < n_levels; ++l)
    {
        SondeHectopascal p = ps[l];
        SondeSpdDirKts spd = winds[l];

        b32 proceed = !isnan(p.val) && !isnan(spd.spd);
        if(proceed)
        {
            size idx = pak_array_ledger_push_back_index(&ledger);
            pnts[idx] = wind_profile_pressure_kt_to_chart_coords(p, spd);
            pnts[idx] = vchart_panel_apply_pan_and_zoom(pnts[idx], cp);
            pnts[idx] = vchart_panel_convert_to_screen_coords(pnts[idx], cp);
        }

        success &= DrawPolylineThickAA(app->renderer, pnts, pak_len(&ledger), wind_spd_width, wind_color, scratch_);
    }

    return success;
}

static inline b32
profiles_draw_wind_sample(AppState *app)
{
    /* TODO: This could be combined with the other profile sample bars? */
    VChartPanel *cp = &app->wind_profile;
    b32 success = true;

    b32 do_draw_sample = app->sample.type == SAMPLE_TYPE_SKEWT && chart_panel_point_inside(&app->skewt, app->mouse_loc);

    if(do_draw_sample)
    {
        SondeHectopascal p = app->sample.row.p;
        SDL_FPoint p1 = wind_profile_pressure_kt_to_chart_coords(p, (SondeSpdDirKts){ .dir = 0.0, .spd = 0.0});
        p1.x = 0.0f;
        p1 = vchart_panel_apply_pan_and_zoom(p1, cp);
        p1 = vchart_panel_convert_to_screen_coords(p1, cp);
        SDL_FPoint p2 = wind_profile_pressure_kt_to_chart_coords(p, (SondeSpdDirKts){ .dir = 0.0, .spd = 10.0});
        p2.x = 1.0f;
        p2 = vchart_panel_apply_pan_and_zoom(p2, cp);
        p2 = vchart_panel_convert_to_screen_coords(p2, cp);

        success &= DrawLineThickRoundedAA(app->renderer, p1, p2, sample_bar_width, sample_bar_color); Assert(success);
    }

    return success;
}

static inline b32
profiles_draw_wind_legend(AppState *app)
{
    b32 success = true;

    VChartPanel *cp = &app->wind_profile;
    MagAllocator alloc_ = app->scratch_alloc;

    f32 const padding = 6.0f;
    f32 const border_width = 3.0f;

    SDL_FPoint legend_ul = { .x = cp->area.x + 10.0f, .y = cp->area.y + 20.0f };
    SDL_FPoint legend_lr = { .x = legend_ul.x + padding, .y = legend_ul.y + padding };
    i32 tw, th;

    TTF_Text *legend_txt = TTF_CreateText(app->text_engine, app->large_font, "Wind Spd (kt)", 0);
    TTF_SetTextColorFloat(legend_txt, wind_color.r, wind_color.g, wind_color.b, wind_color.a);
    TTF_GetTextSize(legend_txt, &tw, &th);
    SDL_FPoint legend_txt_start = legend_lr;
    legend_lr.y += (f32)th;
    legend_lr.x = legend_ul.x + 2.0f * padding + (f32)tw < legend_lr.x ? legend_lr.x : legend_ul.x + 2.0f * padding +(f32)tw;

    legend_lr.y += padding;

    SDL_FPoint border[4] = {0};
    border[0] = legend_ul;
    border[1] = (SDL_FPoint){ .x = legend_ul.x, .y = legend_lr.y };
    border[2] = legend_lr;
    border[3] = (SDL_FPoint){ .x = legend_lr.x, .y = legend_ul.y };

    success &= DrawPolygonThick(app->renderer, border, 4, border_width, isobar_color, app->background, alloc_);

    TTF_DrawRendererText(legend_txt, legend_txt_start.x, legend_txt_start.y);
    TTF_DestroyText(legend_txt);

    return success;
}

static inline b32 
profiles_draw_wind(AppState *app)
{
    b32 success = true;

    VChartPanel *wind_cp = &app->wind_profile;

    SDL_SetRenderClipRect(app->renderer, &wind_cp->area);

    success &= profiles_draw_wind_background(app); Assert(success);

    if(app->current_sndg >= 0)
    {
        success &= profiles_draw_wind_data(app);   Assert(success);
        success &= profiles_draw_wind_sample(app); Assert(success);
    }

    profiles_draw_wind_legend(app);

    /* Draw a border around the RH-PVV panel. */
    SDL_FPoint pnts[4] = {0};
    pnts[0] = (SDL_FPoint){ .x = wind_cp->area.x, .y = wind_cp->area.y };
    pnts[1] = (SDL_FPoint){ .x = wind_cp->area.x, .y = wind_cp->area.y + wind_cp->area.h };
    pnts[2] = (SDL_FPoint){ .x = wind_cp->area.x + wind_cp->area.w, .y = wind_cp->area.y + wind_cp->area.h };
    pnts[3] = (SDL_FPoint){ .x = wind_cp->area.x + wind_cp->area.w, .y = wind_cp->area.y };

    MagAllocator scratch = app->scratch_alloc;
    success &= DrawPolygonThick(app->renderer, pnts, 4, 4.0f, isobar_color, (SDL_FColor){0}, scratch);

    SDL_SetRenderClipRect(app->renderer, NULL);

    return success;
}

static inline b32
profiles_draw(AppState *app)
{
    b32 success = true;

    /* Draw the RH-PVV profiles. */
    success &= profiles_draw_rh_pvv(app);

    /* Draw the wind speed profiles. */
    success &= profiles_draw_wind(app);

    return success;
}

