
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
    f32 const pvv_max = 3.0f;
    f32 y = (logf(p.val) - logf(P_MIN)) / (logf(P_MAX) - logf(P_MIN));
    f32 x =  pvv.val * 100.0f / pvv_max / 2.0f + 0.5f;

    return (SDL_FPoint){ .x = x, .y = y };
}

static inline b32 
profiles_draw_rh_data(AppState *app)
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
profiles_draw_rh_pvv(AppState *app)
{
    b32 success = true;

    success &= profiles_draw_rh_data(app); Assert(success);

    return success;
}

static inline b32
profiles_draw(AppState *app)
{
    b32 success = true;

    if(app->current_sndg >= 0)
    {
        VChartPanel *cp = &app->rh_profile;
        MagAllocator scratch = app->scratch_alloc;

        SDL_SetRenderClipRect(app->renderer, &cp->area);

        success &= profiles_draw_rh_pvv(app);

        /* Draw a border around the panel. */
        SDL_FPoint pnts[4] = {0};
        pnts[0] = (SDL_FPoint){ .x = cp->area.x, .y = cp->area.y };
        pnts[1] = (SDL_FPoint){ .x = cp->area.x, .y = cp->area.y + cp->area.h };
        pnts[2] = (SDL_FPoint){ .x = cp->area.x + cp->area.w, .y = cp->area.y + cp->area.h };
        pnts[3] = (SDL_FPoint){ .x = cp->area.x + cp->area.w, .y = cp->area.y };
        success &= DrawPolygonThick(app->renderer, pnts, 4, 4.0f, black, (SDL_FColor){0}, scratch);

        SDL_SetRenderClipRect(app->renderer, NULL);
    }

    return success;
}

