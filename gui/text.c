/* This file contains the code for the text panel which shows a text representation of the sounding. */

static SDL_FColor const text_hightlight_color = { .r = 0.862745f, .g = 0.0f, .b = 0.0f, .a = 1.00f };

static inline void 
build_sounding_text(SoundingAmalgam *amalgam, MagAllocator *data)
{
#define TEXT_BUF_INITIAL_CAPACITY 10000
#define TEXT_BUF_GROW_AMOUNT 500

    if(amalgam->text.start) { return; } /* It's already been done. */

    SondeSounding *sndg = amalgam->sndg;

    char *buf = eco_arena_nmalloc(data, TEXT_BUF_INITIAL_CAPACITY, char);
    size buf_size = TEXT_BUF_INITIAL_CAPACITY;
    size buf_remaining = TEXT_BUF_INITIAL_CAPACITY;
    char *next = buf;
    size n_lines = 0;

    i32 max_idx = (i32)pak_len(&sndg->levels) - 1;

    /* Header Row 1 - column labels*/
    size nw = snprintf(next, buf_remaining, "%-5s", "Pres");
    next += nw;
    buf_remaining -= nw;

    nw = snprintf(next, buf_remaining, " %-5s", "Temp");
    next += nw;
    buf_remaining -= nw;

    nw = snprintf(next, buf_remaining, " %-5s", "Dew");
    next += nw;
    buf_remaining -= nw;

    *next = '\n';
    ++next;
    --buf_remaining;
    n_lines += 1;

    /* Header Row 2 - column labels*/
    nw = snprintf(next, buf_remaining, "%-5s", "hPa");
    next += nw;
    buf_remaining -= nw;

    nw = snprintf(next, buf_remaining, " %-5s", " °C ");
    next += nw;
    buf_remaining -= nw;

    nw = snprintf(next, buf_remaining, " %-5s", " °C ");
    next += nw;
    buf_remaining -= nw;

    *next = '\n';
    ++next;
    --buf_remaining;
    n_lines += 1;

    /* Work top of the atmosphere to the bottom. */
    for(i32 l = max_idx; l >= 0; --l)
    {
        SondeHectopascal p = sndg->p[l];
        SondeCelsius t = sndg->t[l];
        SondeCelsius dp = sndg->dp[l];

        size num_written = 0;
        char *line_start = next;

        if(SondeIsError(p)) { continue; }

        size nw = snprintf(next, buf_remaining, "%5.1lf", p.val);
        next += nw;
        num_written += nw;
        buf_remaining -= nw;

        if(!SondeIsError(t))
        {
            nw = snprintf(next, buf_remaining, " %5.1lf", t.val);
            next += nw;
            num_written += nw;
            buf_remaining -= nw;
        }

        if(!SondeIsError(dp))
        {
            nw = snprintf(next, buf_remaining, " %5.1lf", dp.val);
            next += nw;
            num_written += nw;
            buf_remaining -= nw;
        }

        if(buf_remaining <= 0) /* We ran out of space for this line, allocate more memory */
        {
            size dstart = line_start - buf;
            buf_size += TEXT_BUF_GROW_AMOUNT;
            buf = eco_arena_nrealloc(data, buf, buf_size, char); Assert(buf); 

            /* Reset to where we were when we started this line */
            next = buf + dstart;
            buf_remaining += num_written + TEXT_BUF_GROW_AMOUNT;
            ++l; /* Redo this line */
        }
        else
        {

            /* Finish the line */
            *next = '\n';
            ++next;
            ++num_written;
            --buf_remaining;
            n_lines += 1;
        }
    }

    size len = (size)(next-buf);
    buf = eco_arena_nrealloc(data, buf, len, char); Assert(buf); 

    amalgam->text.start = buf;
    amalgam->text.len = len;
    amalgam->text_n_lines = n_lines;

    /* Subtract 2 for the header rows */
    Assert(n_lines - 2 == pak_len(&amalgam->sndg->levels));

#undef TEXT_BUF_GROW_AMOUNT
#undef TEXT_BUF_INITIAL_CAPACITY
}

b32 text_draw(AppState *app)
{
    b32 success = true;
    if(app->n_soundings > 0)
    {
        SoundingAmalgam *amalgam = &app->sndg_amalgams[app->current_sndg];
        MagAllocator scratch_ = app->scratch_alloc;
        MagAllocator *scratch = &scratch_;
        MagAllocator *data = &app->data_alloc;

        /* Set up clipping area */
        SDL_SetRenderClipRect(app->renderer, &app->text_area.area);
        
        build_sounding_text(amalgam, data); /* Caches result and returns early if possible. */
        size n_lines = amalgam->text_n_lines;

        /* Build the strings and calculate how much height we'll need.
         *
         * DO NOT PRESERVE THESES IN THE DATA ARENA - SDL3 may create resources on the GPU, and those need to be cleaned
         * up, which just clearing the arena will not do. 
         */
        TTF_Text **txt_lines = eco_arena_nmalloc(scratch, n_lines, TTF_Text*);    Assert(txt_lines);
        SDL_FPoint *line_start = eco_arena_nmalloc(scratch, n_lines, SDL_FPoint); Assert(line_start);
        f32 total_height = 0.0f;

        f32 const x0 = (f32)app->text_area.area.x + 10.0f;
        f32 y = (f32)app->text_area.area.y;
        SDL_FRect highlight_row_y0 = {0};
        SDL_FRect highlight_row_y1 = {0};
        SDL_FColor highlight_row_color0 = text_hightlight_color;
        SDL_FColor highlight_row_color1 = text_hightlight_color;

        ElkStrSplitPair pair = elk_str_split_on_char(amalgam->text, '\n');
        size row = 0;
        while(pair.left.start)
        {
            i32 tw, th;
            TTF_Text *txt = TTF_CreateText(app->text_engine, app->large_font, pair.left.start, pair.left.len);
            TTF_GetTextSize(txt, &tw, &th);
            SDL_FPoint txt_start = { .x = x0, .y = y };
            y += (f32)th;

            /* -1 for the max index, +2 for the header rows */
            size prow = pak_len(&amalgam->sndg->levels) - 1 + 2 - row;
            if(prow < pak_len(&amalgam->sndg->levels) && prow - 1 >= 0 && app->sample.type == SAMPLE_TYPE_SKEWT)
            {
                SondeHectopascal p0 = amalgam->sndg->p[prow - 0];
                SondeHectopascal p1 = amalgam->sndg->p[prow - 1];
                SondeHectopascal sp = app->sample.row.p;
                if(sp.val == p0.val)
                {
                    highlight_row_y0 = (SDL_FRect){ .x = (f32)app->text_area.area.x, .y = txt_start.y, .w = (f32)app->text_area.area.w, .h = (f32)th };
                    highlight_row_color0.a = 1.0f;
                    highlight_row_color1.a = 0.0f;
                }
                else if(sp.val > p0.val && sp.val < p1.val)
                {
                    highlight_row_y0 = (SDL_FRect){ .x = (f32)app->text_area.area.x, .y = txt_start.y, .w = (f32)app->text_area.area.w, .h = (f32)th };
                    highlight_row_y1 = (SDL_FRect){ .x = (f32)app->text_area.area.x, .y = y, .w = (f32)app->text_area.area.w, .h = (f32)th };
                    f32 alpha_split = (sp.val - p0.val) / (p1.val - p0.val);
                    highlight_row_color0.a = 1.0f - alpha_split;
                    highlight_row_color1.a = alpha_split;
                }
            }

            txt_lines[row] = txt;
            line_start[row] = txt_start;
            total_height += (f32)th;

            pair = elk_str_split_on_char(pair.right, '\n');
            row += 1;
        }

        /* Set the position on screen, or the scroll to match the sample point or default to the bottom, and draw the 
         * highlight. 
         */
        f32 y_adjust = total_height - (f32)app->text_area.area.h; /* Default to the bottom of the sounding. */
        if(app->sample.type == SAMPLE_TYPE_SKEWT)
        {
            /* Adjust to make sure highlighted row is always visible */
            f32 y_adjust_candidate = (highlight_row_y1.y - app->text_area.area.y) - (f32)app->text_area.area.h * 0.5f;
            if(y_adjust_candidate < y_adjust) { y_adjust = y_adjust_candidate; }
            if(y_adjust < 0.0f) { y_adjust = 0.0f; }

            /* Draw the highlight rectangles */
            highlight_row_y0.y -= y_adjust;
            highlight_row_y1.y -= y_adjust;

            SDL_FPoint pnts0[4] =
                {
                    (SDL_FPoint){ .x = highlight_row_y0.x, .y = highlight_row_y0.y},
                    (SDL_FPoint){ .x = highlight_row_y0.x, .y = highlight_row_y0.y + highlight_row_y0.h},
                    (SDL_FPoint){ .x = highlight_row_y0.x + highlight_row_y0.w, .y = highlight_row_y0.y + highlight_row_y0.h},
                    (SDL_FPoint){ .x = highlight_row_y0.x + highlight_row_y0.w, .y = highlight_row_y0.y},
                };

            SDL_FPoint pnts1[4] =
                {
                    (SDL_FPoint){ .x = highlight_row_y1.x, .y = highlight_row_y1.y},
                    (SDL_FPoint){ .x = highlight_row_y1.x, .y = highlight_row_y1.y + highlight_row_y1.h},
                    (SDL_FPoint){ .x = highlight_row_y1.x + highlight_row_y1.w, .y = highlight_row_y1.y + highlight_row_y1.h},
                    (SDL_FPoint){ .x = highlight_row_y1.x + highlight_row_y1.w, .y = highlight_row_y1.y},
                };

            MagAllocator scratch2 = *scratch;
            success &= DrawPolygonThick(app->renderer, pnts0, ECO_ARRAY_SIZE(pnts0), 0.0f, highlight_row_color0, highlight_row_color0, scratch2);
            success &= DrawPolygonThick(app->renderer, pnts1, ECO_ARRAY_SIZE(pnts1), 0.0f, highlight_row_color1, highlight_row_color1, scratch2);
        }
        
        /* Now draw the text and delete the TTF_Text from the GPU */
        for(size row = 2; row < n_lines; ++row) /* Start at 2 to skip the header rows */
        {
            TTF_Text *txt = txt_lines[row];
            TTF_SetTextColorFloat(txt, black.r, black.g, black.b, black.a);
            SDL_FPoint txt_start = line_start[row];
            TTF_DrawRendererText(txt, txt_start.x, txt_start.y - y_adjust);
            TTF_DestroyText(txt);
        }

        /* Go back and draw the header rows */
        SDL_FPoint pnts0[4] =  /* Cover other text if any with a solid background. */
            {
                (SDL_FPoint){ .x = line_start[0].x, .y = (f32)app->text_area.area.y },
                (SDL_FPoint){ .x = line_start[0].x, .y = (f32)app->text_area.area.y + (line_start[2].y - line_start[0].y) },
                (SDL_FPoint){ .x = line_start[0].x + (f32)app->text_area.area.w, .y = (f32)app->text_area.area.y + (line_start[2].y - line_start[0].y) },
                (SDL_FPoint){ .x = line_start[0].x + (f32)app->text_area.area.h, .y = (f32)app->text_area.area.y },
            };

        MagAllocator scratch2 = *scratch;
        success &= DrawPolygonThick(app->renderer, pnts0, ECO_ARRAY_SIZE(pnts0), 0.0f, app->background, app->background, scratch2);

        /* Header row 1 */
        TTF_Text *txt = txt_lines[0];
        TTF_SetTextColorFloat(txt, black.r, black.g, black.b, black.a);
        SDL_FPoint txt_start = line_start[0];
        txt_start.y = (f32)app->text_area.area.y;
        TTF_DrawRendererText(txt, txt_start.x, txt_start.y);
        TTF_DestroyText(txt);

        /* Header row 2 */
        txt = txt_lines[1];
        TTF_SetTextColorFloat(txt, black.r, black.g, black.b, black.a);
        txt_start = line_start[1];
        txt_start.y = (f32)app->text_area.area.y + (line_start[1].y - line_start[0].y);
        TTF_DrawRendererText(txt, txt_start.x, txt_start.y);
        TTF_DestroyText(txt);

        /* Restore clipping area */
        SDL_SetRenderClipRect(app->renderer, NULL);
    }

    return success;
}

