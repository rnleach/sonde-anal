b32 text_draw(AppState *app)
{
    /* TODO: implement */

    i32 tw, th;
    TTF_Text *txt = TTF_CreateText(app->text_engine, app->large_font, "UNIMPLEMENTED", 0);
    TTF_SetTextColorFloat(txt, black.r, black.g, black.b, black.a);
    TTF_GetTextSize(txt, &tw, &th);
    SDL_FPoint txt_start = { .x = app->skewt.area.w + 20.0f, .y = 100.0f };
    TTF_DrawRendererText(txt, txt_start.x, txt_start.y);
    TTF_DestroyText(txt);

    return true;
}

