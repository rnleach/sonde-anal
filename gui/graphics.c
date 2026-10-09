/* This file contains functions for drawing fundamentals. All drawing is done in the default SDL coordinate system. */

/* --------------------------------------------------------- API --------------------------------------------------------- */

b32 DrawPointRound(SDL_Renderer *r, SDL_FPoint point, f32 diameter, SDL_FColor color);

b32 DrawLineThick(SDL_Renderer *r, SDL_FPoint p1, SDL_FPoint p2,  f32 width, SDL_FColor color);
b32 DrawLineThickRoundedAA(SDL_Renderer *r, SDL_FPoint p1, SDL_FPoint p2, f32 width, SDL_FColor color);

b32 DrawPolylineThick(SDL_Renderer *r, SDL_FPoint const *points, i32 count, f32 width, SDL_FColor color, MagAllocator alloc_);
b32 DrawPolylineThickAA(SDL_Renderer *r, SDL_FPoint const *points, i32 count, f32 width, SDL_FColor color, MagAllocator alloc_);
b32 DrawPolylineFilletedThick(SDL_Renderer *r, SDL_Texture *scratch_layer, SDL_FPoint const *points, i32 count, f32 width, SDL_FColor color, MagAllocator alloc_);
b32 DrawPolylineSmoothThick(SDL_Renderer *r, SDL_FPoint const *points, i32 count, f32 width, SDL_FColor color, MagAllocator alloc_);
b32 DrawPolygonSmoothThick(SDL_Renderer *r, SDL_FPoint const *points, i32 count, f32 border_width, SDL_FColor border_color, SDL_FColor fill_color, MagAllocator alloc_);
b32 DrawPolygonThick(SDL_Renderer *renderer, SDL_FPoint const *points, i32 count, f32 border_width, SDL_FColor border_color, SDL_FColor fill_color, MagAllocator alloc_);

/* --------------------------------------------------- Implementations --------------------------------------------------- */

/* Helper to compute 2D vector length. */
static inline f32 vec_length(f32 x, f32 y) { return sqrtf(x * x + y * y); }

/*
 * Evaluates a 1D Catmull-Rom spline segment at position t in [0, 1].
 * p0, p1, p2, p3 are the control points surrounding the active segment (p1 -> p2).
 */
static inline f32 
catmull_rom_1d(f32 p0, f32 p1, f32 p2, f32 p3, f32 t) 
{
    f32 t2 = t * t;
    f32 t3 = t2 * t;

    return 0.5f * ((2.0f * p1) +
                  (-p0 + p2) * t +
                  (2.0f * p0 - 5.0f * p1 + 4.0f * p2 - p3) * t2 +
                  (-p0 + 3.0f * p1 - 3.0f * p2 + p3) * t3);
}

/* Evaluates a 2D Catmull-Rom point between p1 and p2. */
static inline SDL_FPoint 
catmull_rom_2d(SDL_FPoint p0, SDL_FPoint p1, SDL_FPoint p2, SDL_FPoint p3, float t) 
{
    return (SDL_FPoint)
    {
        catmull_rom_1d(p0.x, p1.x, p2.x, p3.x, t),
        catmull_rom_1d(p0.y, p1.y, p2.y, p3.y, t)
    };
}

static inline f32 
point_dist(SDL_FPoint a, SDL_FPoint b) 
{
    f32 dx = b.x - a.x;
    f32 dy = b.y - a.y;
    return sqrtf(dx * dx + dy * dy);
}

static inline SDL_FPoint
point_lerp(SDL_FPoint a, SDL_FPoint b, f32 t)
{
    return (SDL_FPoint)
    {
        a.x + (b.x - a.x) * t,
        a.y + (b.y - a.y) * t
    };
}

static inline SDL_FPoint 
quad_bezier(SDL_FPoint p0, SDL_FPoint p1, SDL_FPoint p2, f32 t)
{
    f32 inv_t = 1.0f - t;
    f32 w0 = inv_t * inv_t;
    f32 w1 = 2.0f * inv_t * t;
    f32 w2 = t * t;

    return (SDL_FPoint)
    {
        w0 * p0.x + w1 * p1.x + w2 * p2.x,
        w0 * p0.y + w1 * p1.y + w2 * p2.y
    };
}

/* Draws a filled round point (circle) with a specified diameter using SDL3 geometry. */
b32 
DrawPointRound(SDL_Renderer *renderer, SDL_FPoint point, f32 diameter, SDL_FColor color) 
{
    StopIf(!renderer || diameter <= 0.0f, return false);

    f32 radius = diameter * 0.5f;

    /* Dynamically scale circle smoothness with size (min 8, max 64 segments) */
    int segments = (int) diameter;
    if(segments < 8)  { segments = 8;  }
    if(segments > 64) { segments = 64; }

    /* Pre-allocate stacks for max segments (64 outer + 1 center vertex) */
    SDL_Vertex vertices[65] = {0};
    int indices[192] = {0};       /* 3 indices per triangle (segments * 3) */

    /* Center vertex */
    vertices[0].position = point;
    vertices[0].color = color;
    /* vertices[0].tex_coord = (SDL_FPoint){ 0.0f, 0.0f }; */

    f32 angle_step = (2.0f * (f32)M_PI) / (f32)segments;

    /* Outer circle vertices & triangle fan indices */
    for (i32 i = 0; i < segments; i++)
    {
        f32 angle = i * angle_step;

        vertices[i + 1].position = (SDL_FPoint)
        {
            point.x + cosf(angle) * radius,
            point.y + sinf(angle) * radius
        };

        vertices[i + 1].color = color;
        /* vertices[i + 1].tex_coord = (SDL_FPoint){ 0.0f, 0.0f }; */

        /* Triangle connecting center (0) -> current (i + 1) -> next (i + 2 or 1) */
        /* indices[i * 3]     = 0; */
        indices[i * 3 + 1] = i + 1;
        indices[i * 3 + 2] = (i + 1 == segments) ? 1 : i + 2;
    }

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    return SDL_RenderGeometry(renderer, NULL, vertices, segments + 1, indices, segments * 3);
}

b32 
DrawLineThick(SDL_Renderer *renderer, SDL_FPoint p1, SDL_FPoint p2,  f32 width, SDL_FColor color)
{
    StopIf(!renderer || width <= 0.0f,  return false);

    /* Direction vector from (x1, y1) to (x2, y2) */
    f32 dx = p2.x - p1.x;
    f32 dy = p2.y - p1.y;
    f32 length = sqrtf(dx * dx + dy * dy);

    /* Handle zero-length lines by rendering nothing or a small dot */
    if(length == 0.0f) { return DrawPointRound(renderer, p1, width, color); }

    /* Perpendicular unit vector scaled by half-width */
    f32 half_w = width * 0.5f;
    f32 nx = (-dy / length) * half_w;
    f32 ny = (dx / length) * half_w;

    /* Define 4 corner vertices of the thick line rectangle */
    SDL_Vertex vertices[4] = {0};

    /* Top-left offset from start */
    vertices[0].position = (SDL_FPoint){ p1.x + nx, p1.y + ny };
    vertices[0].color = color;
    /* vertices[0].tex_coord = (SDL_FPoint){ 0.0f, 0.0f }; */

    /* Bottom-left offset from start */
    vertices[1].position = (SDL_FPoint){ p1.x - nx, p1.y - ny };
    vertices[1].color = color;
    /* vertices[1].tex_coord = (SDL_FPoint){ 0.0f, 0.0f }; */

    /* Bottom-right offset from end */
    vertices[2].position = (SDL_FPoint){ p2.x - nx, p2.y - ny };
    vertices[2].color = color;
    /* vertices[2].tex_coord = (SDL_FPoint){ 0.0f, 0.0f }; */

    /* Top-right offset from end */
    vertices[3].position = (SDL_FPoint){ p2.x + nx, p2.y + ny };
    vertices[3].color = color;
    /* vertices[3].tex_coord = (SDL_FPoint){ 0.0f, 0.0f }; */

    /* Index array forming two triangles (0-1-2 and 0-2-3) */
    i32 const indices[6] = { 0, 1, 2, 0, 2, 3 };

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    return SDL_RenderGeometry(renderer, NULL, vertices, 4, indices, 6);
}

/* Draws a thick line with rounded end-caps and smooth anti-aliased edges in SDL3. */
b32 
DrawLineThickRoundedAA(SDL_Renderer *renderer, SDL_FPoint p1, SDL_FPoint p2, f32 width, SDL_FColor color) 
{
    StopIf(!renderer || width <= 0.0f, return false);

    f32 radius = width * 0.5f;
    f32 dx = p2.x - p1.x;
    f32 dy = p2.y - p1.y;
    f32 length = sqrtf(dx * dx + dy * dy);

    /* Handle single point (draw a smooth circle) */
    if(length == 0.0f) { return DrawPointRound(renderer, p1, width, color); }

    /* Direction vector & perpendicular normal */
    f32 ux = dx / length;
    f32 uy = dy / length;
    f32 nx = -uy * radius;
    f32 ny =  ux * radius;

    /* AA fringe thickness (1 pixel smooth border) */
    f32 aa_fringe = 1.0f;
    f32 nx_aa = -uy * (radius + aa_fringe);
    f32 ny_aa =  ux * (radius + aa_fringe);

    SDL_FColor color_transparent = color;
    color_transparent.a = 0.0f;

    /* Determine cap smoothness resolution based on size */
    i32 cap_segments = (i32)(radius * 1.5f);
    if(cap_segments <  8) { cap_segments =  8; }
    if(cap_segments > 32) { cap_segments = 32; }

    /* --- 1. Draw Main Body & Anti-Aliased Edges --- */
    /* Body rectangle (4 vertices for core, 4 for AA fringe) */
    SDL_Vertex body_verts[8] = {0};
    
    /* Solid core quad */
    body_verts[0].position = (SDL_FPoint){ p1.x + nx, p1.y + ny }; body_verts[0].color = color;
    body_verts[1].position = (SDL_FPoint){ p1.x - nx, p1.y - ny }; body_verts[1].color = color;
    body_verts[2].position = (SDL_FPoint){ p2.x - nx, p2.y - ny }; body_verts[2].color = color;
    body_verts[3].position = (SDL_FPoint){ p2.x + nx, p2.y + ny }; body_verts[3].color = color;

    /* Outer AA edge fringe */
    body_verts[4].position = (SDL_FPoint){ p1.x + nx_aa, p1.y + ny_aa }; body_verts[4].color = color_transparent;
    body_verts[5].position = (SDL_FPoint){ p1.x - nx_aa, p1.y - ny_aa }; body_verts[5].color = color_transparent;
    body_verts[6].position = (SDL_FPoint){ p2.x - nx_aa, p2.y - ny_aa }; body_verts[6].color = color_transparent;
    body_verts[7].position = (SDL_FPoint){ p2.x + nx_aa, p2.y + ny_aa }; body_verts[7].color = color_transparent;

    /* Triangles for body core (0-1-2, 0-2-3) and top/bottom AA strips */
    i32 body_indices[] =
    {
        /* Main solid rectangle */
        0, 1, 2,  0, 2, 3,
        /* Top AA edge strip    */
        4, 0, 3,  4, 3, 7,
        /* Bottom AA edge strip */
        1, 5, 6,  1, 6, 2
    };

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_RenderGeometry(renderer, NULL, body_verts, 8, body_indices, 18);

    /* --- 2. Helper Lambda/Function logic for Semicircular Caps --- */
    /* Helper to render half-circle end caps with AA fringe */
    for (i32 cap = 0; cap < 2; cap++)
    {
        f32 cx = (cap == 0) ? p1.x : p2.x;
        f32 cy = (cap == 0) ? p1.y : p2.y;
        /* Cap 0 points backwards (-u), Cap 1 points forwards (+u) */
        f32 base_angle = (cap == 0) ? atan2f(-uy, -ux) : atan2f(uy, ux);

        SDL_Vertex cap_verts[128] = {0};
        i32 cap_indices[384] = {0};

        cap_verts[0].position = (SDL_FPoint){ cx, cy };
        cap_verts[0].color = color;

        i32 vert_count = 1;
        i32 idx_count = 0;
        f32 angle_step = (f32)M_PI / cap_segments;

        for (i32 i = 0; i <= cap_segments; i++)
        {
            f32 angle = base_angle - (f32)M_PI_2 + (i * angle_step);
            f32 cos_a = cosf(angle);
            f32 sin_a = sinf(angle);

            /* Inner solid vertex */
            cap_verts[vert_count].position = (SDL_FPoint){ cx + cos_a * radius, cy + sin_a * radius };
            cap_verts[vert_count].color = color;

            /* Outer transparent AA vertex */
            cap_verts[vert_count + 1].position = (SDL_FPoint){ cx + cos_a * (radius + aa_fringe), cy + sin_a * (radius + aa_fringe) };
            cap_verts[vert_count + 1].color = color_transparent;

            if(i > 0)
            {
                i32 curr_inner = vert_count;
                i32 prev_inner = vert_count - 2;
                i32 curr_outer = vert_count + 1;
                i32 prev_outer = vert_count - 1;

                /* Solid center cap slice */
                cap_indices[idx_count++] = 0;
                cap_indices[idx_count++] = prev_inner;
                cap_indices[idx_count++] = curr_inner;

                /* AA outer cap fringe quad */
                cap_indices[idx_count++] = prev_inner;
                cap_indices[idx_count++] = prev_outer;
                cap_indices[idx_count++] = curr_outer;

                cap_indices[idx_count++] = prev_inner;
                cap_indices[idx_count++] = curr_outer;
                cap_indices[idx_count++] = curr_inner;
            }

            vert_count += 2;
        }

        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_RenderGeometry(renderer, NULL, cap_verts, vert_count, cap_indices, idx_count);
    }

    return true;
}

/* Draws a thick, anti-aliased polyline with mitered joints using SDL3 geometry. */
b32 
DrawPolylineThickAA(SDL_Renderer *renderer, SDL_FPoint const *points, i32 count, f32 width, SDL_FColor color, MagAllocator alloc_) 
{
    StopIf(!renderer || !points || count < 2 || width <= 0.0f, return false);
    MagAllocator *alloc = &alloc_;

    f32 miter_limit = 2.0f * width; 
    f32 half_w = width * 0.5f;

    /* 
     * Core width is shrunk by 0.5px, fade width is expanded by 0.5px.
     * If the line is very thin (width < 1.0), core shrinks to 0. 
     */
    f32 aa_fringe = 0.5f;
    f32 core_w = (half_w > aa_fringe) ? (half_w - aa_fringe) : 0.0f;
    f32 fade_w = half_w + aa_fringe;

    /* 4 vertices per point (Outer Left, Inner Left, Inner Right, Outer Right) */
    i32 num_vertices = count * 4;
    
    /* 3 quads (6 triangles) per segment */
    i32 num_triangles = (count - 1) * 6;
    i32 num_indices = num_triangles * 3;

    SDL_Vertex *vertices = eco_arena_nmalloc(alloc, num_vertices, SDL_Vertex);
    i32 *indices = eco_arena_nmalloc(alloc, num_indices, i32);
    StopIf(!vertices || !indices, return false); 

    /* Define our inner (solid) and outer (transparent) colors */
    SDL_FColor color_inner = color;
    SDL_FColor color_outer = color;
    color_outer.a = 0.0f; /* Fade to transparent at the absolute edges */

    for (i32 i = 0; i < count; i++)
    {
        f32 nx = 0.0f, ny = 0.0f;

        if(i == 0)
        {
            f32 dx = points[1].x - points[0].x;
            f32 dy = points[1].y - points[0].y;
            f32 len = vec_length(dx, dy);
            if(len > 0.0f) { nx = -dy / len; ny =  dx / len; }
        }
        else if(i == count - 1)
        {
            f32 dx = points[count - 1].x - points[count - 2].x;
            f32 dy = points[count - 1].y - points[count - 2].y;
            f32 len = vec_length(dx, dy);
            if(len > 0.0f) { nx = -dy / len; ny =  dx / len; }
        } 
        else
        {
            f32 dx1 = points[i].x - points[i - 1].x;
            f32 dy1 = points[i].y - points[i - 1].y;
            f32 len1 = vec_length(dx1, dy1);

            f32 dx2 = points[i + 1].x - points[i].x;
            f32 dy2 = points[i + 1].y - points[i].y;
            f32 len2 = vec_length(dx2, dy2);

            if(len1 > 0.0f && len2 > 0.0f)
            {
                f32 u1x = dx1 / len1, u1y = dy1 / len1;
                f32 u2x = dx2 / len2, u2y = dy2 / len2;

                f32 n1x = -u1y, n1y =  u1x;

                f32 tx = u1x + u2x;
                f32 ty = u1y + u2y;
                f32 t_len = vec_length(tx, ty);

                if(t_len > 0.001f) 
                {
                    f32 miter_x = -ty / t_len;
                    f32 miter_y =  tx / t_len;

                    f32 dot = miter_x * n1x + miter_y * n1y;
                    f32 miter_len = (dot != 0.0f) ? (1.0f / dot) : 1.0f;

                    if(fabsf(miter_len) > miter_limit)
                    {
                        miter_len = (miter_len < 0.0f) ? -miter_limit : miter_limit;
                    }

                    nx = miter_x * miter_len;
                    ny = miter_y * miter_len;
                } 
                else
                {
                    nx = n1x;
                    ny = n1y;
                }
            }
        }

        /* Generate 4 vertices per point */
        i32 v_idx = i * 4;

        /* Outer Left */
        vertices[v_idx + 0].position = (SDL_FPoint){ points[i].x + nx * fade_w, points[i].y + ny * fade_w };
        vertices[v_idx + 0].color = color_outer;

        /* Inner Left */
        vertices[v_idx + 1].position = (SDL_FPoint){ points[i].x + nx * core_w, points[i].y + ny * core_w };
        vertices[v_idx + 1].color = color_inner;

        /* Inner Right */
        vertices[v_idx + 2].position = (SDL_FPoint){ points[i].x - nx * core_w, points[i].y - ny * core_w };
        vertices[v_idx + 2].color = color_inner;

        /* Outer Right */
        vertices[v_idx + 3].position = (SDL_FPoint){ points[i].x - nx * fade_w, points[i].y - ny * fade_w };
        vertices[v_idx + 3].color = color_outer;
    }

    /* Build triangle index ribbon linking the 3 quads (left fringe, core, right fringe) */
    i32 idx_count = 0;
    for (i32 i = 0; i < count - 1; i++)
    {
        i32 L0  = i * 4 + 0;      /* Current Outer Left */
        i32 L1  = i * 4 + 1;      /* Current Inner Left */
        i32 R1  = i * 4 + 2;      /* Current Inner Right */
        i32 R0  = i * 4 + 3;      /* Current Outer Right */

        i32 nL0 = (i + 1) * 4 + 0; /* Next Outer Left */
        i32 nL1 = (i + 1) * 4 + 1; /* Next Inner Left */
        i32 nR1 = (i + 1) * 4 + 2; /* Next Inner Right */
        i32 nR0 = (i + 1) * 4 + 3; /* Next Outer Right */

        /* Quad 1: Left Fringe */
        indices[idx_count++] = L0; indices[idx_count++] = L1; indices[idx_count++] = nL1;
        indices[idx_count++] = L0; indices[idx_count++] = nL1; indices[idx_count++] = nL0;

        /* Quad 2: Solid Core */
        indices[idx_count++] = L1; indices[idx_count++] = R1; indices[idx_count++] = nR1;
        indices[idx_count++] = L1; indices[idx_count++] = nR1; indices[idx_count++] = nL1;

        /* Quad 3: Right Fringe */
        indices[idx_count++] = R1; indices[idx_count++] = R0; indices[idx_count++] = nR0;
        indices[idx_count++] = R1; indices[idx_count++] = nR0; indices[idx_count++] = nR1;
    }

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    return SDL_RenderGeometry(renderer, NULL, vertices, num_vertices, indices, num_indices);
}

/* Draws a thick polyline with mitered joints using SDL3 geometry. */
b32 
DrawPolylineThick(SDL_Renderer *renderer, SDL_FPoint const *points, i32 count, f32 width, SDL_FColor color, MagAllocator alloc_) 
{
    StopIf(!renderer || !points || count < 2 || width <= 0.0f, return false);
    MagAllocator *alloc = &alloc_;

    f32 miter_limit = 2.0f * width; /* Default to this value instead of keeping as argument. */

    f32 half_w = width * 0.5f;
    i32 num_vertices = count * 2;
    i32 num_triangles = (count - 1) * 2;
    i32 num_indices = num_triangles * 3;

    /* Allocate temporary vertex and index arrays */
    SDL_Vertex *vertices = eco_arena_nmalloc(alloc, num_vertices, SDL_Vertex);
    i32 *indices = eco_arena_nmalloc(alloc, num_indices, i32);
    StopIf(!vertices || !indices, return false); 

    /* Compute normals and miter vectors for each point */
    for (i32 i = 0; i < count; i++)
    {
        f32 nx = 0.0f, ny = 0.0f;

        if(i == 0)
        {
            /* Start cap: perpendicular to first segment */
            f32 dx = points[1].x - points[0].x;
            f32 dy = points[1].y - points[0].y;
            f32 len = vec_length(dx, dy);
            if(len > 0.0f)
            {
                nx = -dy / len;
                ny =  dx / len;
            }
        }
        else if(i == count - 1)
        {
            /* End cap: perpendicular to last segment */
            f32 dx = points[count - 1].x - points[count - 2].x;
            f32 dy = points[count - 1].y - points[count - 2].y;
            f32 len = vec_length(dx, dy);
            if(len > 0.0f)
            {
                nx = -dy / len;
                ny =  dx / len;
            }

        } 
        else
        {
            /* Interior vertex: calculate miter bisector vector */
            f32 dx1 = points[i].x - points[i - 1].x;
            f32 dy1 = points[i].y - points[i - 1].y;
            f32 len1 = vec_length(dx1, dy1);

            f32 dx2 = points[i + 1].x - points[i].x;
            f32 dy2 = points[i + 1].y - points[i].y;
            f32 len2 = vec_length(dx2, dy2);

            if(len1 > 0.0f && len2 > 0.0f)
            {
                /* Normalized segment directions */
                f32 u1x = dx1 / len1, u1y = dy1 / len1;
                f32 u2x = dx2 / len2, u2y = dy2 / len2;

                /* Perpendicular normals of segment 1 and 2 */
                f32 n1x = -u1y, n1y =  u1x;
                //f32 n2x = -u2y, n2y =  u2x;

                /* Tangent bisector */
                f32 tx = u1x + u2x;
                f32 ty = u1y + u2y;
                f32 t_len = vec_length(tx, ty);

                if(t_len > 0.001f) 
                {
                    /* Miter vector direction (perpendicular to tangent bisector) */
                    f32 miter_x = -ty / t_len;
                    f32 miter_y =  tx / t_len;

                    /* Miter length scaling = 1 / dot(miter, n1) */
                    f32 dot = miter_x * n1x + miter_y * n1y;
                    f32 miter_len = (dot != 0.0f) ? (1.0f / dot) : 1.0f;

                    /* Clamp extreme miter spikes on acute sharp angles */
                    if(fabsf(miter_len) > miter_limit)
                    {
                        miter_len = (miter_len < 0.0f) ? -miter_limit : miter_limit;
                    }

                    nx = miter_x * miter_len;
                    ny = miter_y * miter_len;
                } 
                else
                {
                    /* Opposite direction (180 deg turn fallback) */
                    nx = n1x;
                    ny = n1y;
                }
            }
        }

        /* Generate left and right offset vertices for point i */
        i32 v_idx = i * 2;

        vertices[v_idx].position = (SDL_FPoint){ points[i].x + nx * half_w, points[i].y + ny * half_w };
        vertices[v_idx].color = color;
        /* vertices[v_idx].tex_coord = (SDL_FPoint){ 0.0f, 0.0f }; */

        vertices[v_idx + 1].position = (SDL_FPoint){ points[i].x - nx * half_w, points[i].y - ny * half_w };
        vertices[v_idx + 1].color = color;
        /* vertices[v_idx + 1].tex_coord = (SDL_FPoint){ 0.0f, 0.0f }; */
    }

    /* Build triangle index ribbon linking vertex pairs */
    i32 idx_count = 0;
    for (i32 i = 0; i < count - 1; i++)
    {
        i32 top_left     = i * 2;
        i32 bottom_left  = i * 2 + 1;
        i32 top_right    = (i + 1) * 2;
        i32 bottom_right = (i + 1) * 2 + 1;

        /* Triangle 1 */
        indices[idx_count++] = top_left;
        indices[idx_count++] = bottom_left;
        indices[idx_count++] = bottom_right;

        /* Triangle 2 */
        indices[idx_count++] = top_left;
        indices[idx_count++] = bottom_right;
        indices[idx_count++] = top_right;
    }

    /* Single accelerated draw call for the entire path */
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    return SDL_RenderGeometry(renderer, NULL, vertices, num_vertices, indices, num_indices);
}

/* Draws a smooth, thick polyline curve passing through control points using Catmull-Rom interpolation. */
b32 
DrawPolylineSmoothThick(SDL_Renderer *renderer, SDL_FPoint const *points, i32 count, f32 width, SDL_FColor color, MagAllocator alloc_)
{
    StopIf(!renderer || !points || count < 2 || width <= 0.0f, return false);
    MagAllocator *alloc = &alloc_;

    /* Default segment density for smooth rendering */
    i32 segments_per_curve = 12; /* Just use default so it's not passed as an argument. */

    /* Total interpolated points across all segments */
    i32 total_sub_points = (count - 1) * segments_per_curve + 1;
    SDL_FPoint *sub_points = eco_arena_nmalloc(alloc, total_sub_points, SDL_FPoint);

    StopIf(!sub_points, return false);

    i32 sub_idx = 0;

    /* Generate interpolated curve points across each pair of control points */
    for (i32 i = 0; i < count - 1; i++)
    {
        /* Clamp boundary control points (P0 and P3) for end segments */
        SDL_FPoint p0 = (i == 0) ? points[0] : points[i - 1];
        SDL_FPoint p1 = points[i];
        SDL_FPoint p2 = points[i + 1];
        SDL_FPoint p3 = (i + 2 >= count) ? points[count - 1] : points[i + 2];

        /* Evaluate sub-segments along the current Catmull-Rom curve */
        for (int s = 0; s < segments_per_curve; s++) {
            float t = (float)s / (float)segments_per_curve;
            sub_points[sub_idx++] = catmull_rom_2d(p0, p1, p2, p3, t);
        }
    }
    /* Add final endpoint */
    sub_points[sub_idx++] = points[count - 1];

    /* Delegate thick geometry generation and rendering to the polyline function */
    return DrawPolylineThickAA(renderer, sub_points, total_sub_points, width, color, alloc_);
}

/* Helper to render a semicircular cap fan at a path endpoint. */
static void 
RenderCapFan(SDL_Renderer *renderer, SDL_FPoint center, SDL_FPoint dir, f32 width, SDL_FColor color)
{
    f32 radius = width * 0.5f;
    f32 len = sqrtf(dir.x * dir.x + dir.y * dir.y);
    StopIf(len <= 0.0f, return);

    /* Normalize direction vector */
    f32 ux = dir.x / len;
    f32 uy = dir.y / len;

    /* Angle of cap orientation */
    f32 base_angle = atan2f(uy, ux);

    i32 segments = (int)(radius * 1.5f);
    if(segments <  8) { segments =  8; }
    if(segments > 32) { segments = 32; }

    SDL_Vertex verts[34] = {0};
    i32 indices[96] = {0};

    /* Center vertex */
    verts[0].position = center;
    verts[0].color = color;
    /* verts[0].tex_coord = (SDL_FPoint){0.0f, 0.0f}; */

    f32 angle_step = (f32)M_PI / (f32)segments;

    /* Generate fan vertices along semicircle arc */
    for (i32 i = 0; i <= segments; i++)
    {
        f32 angle = base_angle - (f32)M_PI_2 + (i * angle_step);
        verts[i + 1].position = (SDL_FPoint)
        {
            center.x + cosf(angle) * radius,
            center.y + sinf(angle) * radius
        };

        verts[i + 1].color = color;
        /* verts[i + 1].tex_coord = (SDL_FPoint){0.0f, 0.0f}; */

        if(i > 0)
        {
            /* indices[(i - 1) * 3]     = 0; */
            indices[(i - 1) * 3 + 1] = i;
            indices[(i - 1) * 3 + 2] = i + 1;
        }
    }

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_RenderGeometry(renderer, NULL, verts, segments + 2, indices, segments * 3);
}

/* Draws a polyline with straight segments, filleted corners, and rounded end-caps. */
b32 
DrawPolylineFilletedThick(SDL_Renderer *renderer, SDL_Texture *scratch_layer, SDL_FPoint const *points, i32 count, f32 width, SDL_FColor color, MagAllocator alloc_)
{
    StopIf(!renderer || !points || count < 2 || width <= 0.0f, return false);
    MagAllocator *alloc = &alloc_;

    f32 corner_radius = 0.5 * width; /* Default value instead of passing as argument. */
    i32 corner_segments = 16;        /* Default value instead of passing as argument. */

    /* Build filleted path point buffer */
    i32 max_points = count + (count - 2) * (corner_segments + 2);
    SDL_FPoint *out_points = eco_arena_nmalloc(alloc, max_points, SDL_FPoint);
    StopIf(!out_points, return false);

    i32 out_count = 0;
    out_points[out_count++] = points[0];

    /* Process interior corners */
    for (i32 i = 1; i < count - 1; i++)
    {
        SDL_FPoint prev = points[i - 1];
        SDL_FPoint curr = points[i];
        SDL_FPoint next = points[i + 1];

        f32 d_prev = point_dist(prev, curr);
        f32 d_next = point_dist(curr, next);

        f32 max_allowed_r = fminf(d_prev * 0.49f, d_next * 0.49f);
        f32 r = fminf(corner_radius, max_allowed_r);

        if(r <= 0.5f)
        {
            out_points[out_count++] = curr;
            continue;
        }

        f32 t_in = 1.0f - (r / d_prev);
        SDL_FPoint p_start = point_lerp(prev, curr, t_in);

        f32 t_out = r / d_next;
        SDL_FPoint p_end = point_lerp(curr, next, t_out);

        for (i32 s = 0; s <= corner_segments; s++)
        {
            f32 t = (f32)s / (f32)corner_segments;
            out_points[out_count++] = quad_bezier(p_start, curr, p_end, t);
        }
    }

    out_points[out_count++] = points[count - 1];

    /* Backup current render target */
    SDL_Texture *prev_target = SDL_GetRenderTarget(renderer);

    /* Ensure the texture background is entirely transparent */
    SDL_SetTextureBlendMode(scratch_layer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderTarget(renderer, scratch_layer);
    SDL_SetRenderDrawColorFloat(renderer, 0.0f, 0.0f, 0.0f, 0.0f);
    SDL_RenderClear(renderer);

    /* Force the geometry color to be 100% OPAQUE */
    SDL_FColor opaque_color = color;
    opaque_color.a = 1.0f; 

    /* Render main body and caps to the scratch layer fully opaque */
    b32 success = DrawPolylineThickAA(renderer, out_points, out_count, width, opaque_color, alloc_);
    if(success)
    {
        SDL_FPoint dir_start = { out_points[0].x - out_points[1].x, out_points[0].y - out_points[1].y };
        RenderCapFan(renderer, out_points[0], dir_start, width, opaque_color);

        SDL_FPoint dir_end = { out_points[out_count - 1].x - out_points[out_count - 2].x, out_points[out_count - 1].y - out_points[out_count - 2].y };
        RenderCapFan(renderer, out_points[out_count - 1], dir_end, width, opaque_color);
    }

    /* Restore the original render target */
    SDL_SetRenderTarget(renderer, prev_target);

    if(success)
    {
        /* Draw the scratch layer to the screen, applying the user's requested transparency uniformly */
        SDL_SetTextureAlphaModFloat(scratch_layer, color.a);
        SDL_RenderTexture(renderer, scratch_layer, NULL, NULL);
    }

    return success;
}

/* Helper function that draws a filled convex/simple polygon with smooth filleted corners. */
static b32 
RenderFilledSmoothPolygon(SDL_Renderer *renderer, const SDL_FPoint *smooth_pts, i32 count, SDL_FColor fill_color, MagAllocator alloc_)
{
    StopIf(count < 3 || fill_color.a <= 0.0f, return true);
    MagAllocator *alloc = &alloc_;

    /* Allocate geometry for central triangle fan */
    SDL_Vertex *verts = eco_arena_nmalloc(alloc, count, SDL_Vertex);
    i32 num_indices = (count - 2) * 3;
    i32 *indices = eco_arena_nmalloc(alloc, num_indices, i32);

    StopIf(!verts || !indices, return false);

    for (i32 i = 0; i < count; i++)
    {
        verts[i].position = smooth_pts[i];
        verts[i].color = fill_color;
        /* verts[i].tex_coord = (SDL_FPoint){0.0f, 0.0f}; */
    }

    /* Convex triangle fan from vertex 0 */
    i32 idx = 0;
    for(i32 i = 1; i < count - 1; i++)
    {
        indices[idx++] = 0;
        indices[idx++] = i;
        indices[idx++] = i + 1;
    }

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    return SDL_RenderGeometry(renderer, NULL, verts, count, indices, num_indices);
}

/* Draws a closed, filled polygon with smooth filleted corners and a thick border. */
b32
DrawPolygonSmoothThick(SDL_Renderer *renderer, SDL_FPoint const *points, i32 count, f32 border_width, SDL_FColor border_color, SDL_FColor fill_color, MagAllocator alloc_)
{
    StopIf(!renderer || !points || count < 3, return false);
    MagAllocator *alloc = &alloc_;

    f32 const corner_radius = 0.5 * border_width; /* Default value so it's not passed in as argument. */
    i32 const corner_segments = 12;               /* Default value so it's not passed in as argument. */

    /* Generate smooth closed path with filleted corners on ALL vertices */
    i32 max_points = count * (corner_segments + 2);

    SDL_FPoint *smooth_pts = eco_arena_nmalloc(alloc, max_points, SDL_FPoint);
    StopIf(!smooth_pts, return false);

    i32 smooth_count = 0;

    for (i32 i = 0; i < count; i++)
    {
        /* Wrap-around indices for closed loop connections */
        SDL_FPoint prev = points[(i - 1 + count) % count];
        SDL_FPoint curr = points[i];
        SDL_FPoint next = points[(i + 1) % count];

        f32 d_prev = point_dist(prev, curr);
        f32 d_next = point_dist(curr, next);

        /* Clamp rounding radius to half of the shortest adjacent edge */
        f32 max_r = fminf(d_prev * 0.49f, d_next * 0.49f);
        f32 r = fminf(corner_radius, max_r);

        if(r <= 0.5f)
        {
            smooth_pts[smooth_count++] = curr;
            continue;
        }

        /* Tangent entry and exit points on edges */
        f32 t_in = 1.0f - (r / d_prev);
        SDL_FPoint p_start = point_lerp(prev, curr, t_in);

        f32 t_out = r / d_next;
        SDL_FPoint p_end = point_lerp(curr, next, t_out);

        /* Tessellate corner arc using quadratic Bezier curve */
        for(i32 s = 0; s <= corner_segments; s++)
        {
            f32 t = (f32)s / (f32)corner_segments;
            smooth_pts[smooth_count++] = quad_bezier(p_start, curr, p_end, t);
        }
    }

    b32 status = true;

    /* Render solid filled interior if fill alpha > 0 */
    if(fill_color.a > 0.0f)
    {
        status &= RenderFilledSmoothPolygon(renderer, smooth_pts, smooth_count, fill_color, alloc_);
    }

    /* Render thick closed border outline if border_width > 0 and border alpha > 0 */
    if(status && border_width > 0.0f && border_color.a > 0.0f)
    {
        /* Duplicate first smooth vertex at end to form a seamless closed loop */
        SDL_FPoint *closed_border_pts = eco_arena_nmalloc(alloc, smooth_count + 1, SDL_FPoint); Assert(closed_border_pts);
        for (i32 i = 0; i < smooth_count; i++)
        {
            closed_border_pts[i] = smooth_pts[i];
        }

        closed_border_pts[smooth_count] = smooth_pts[0]; /* Close loop */
        status &= DrawPolylineThickAA(renderer, closed_border_pts, smooth_count + 1, border_width, border_color, alloc_);
    }

    return status;
}

/**
 * Helper to render the filled interior of a convex/simple polygon.
 */
static b32 
RenderFilledPolygon(SDL_Renderer *renderer, const SDL_FPoint *points, int count, SDL_FColor fill_color, MagAllocator alloc_)
{
    StopIf(count < 3 || fill_color.a <= 0.0f, return true);
    MagAllocator *alloc = &alloc_;

    SDL_Vertex *verts = eco_arena_nmalloc(alloc, count, SDL_Vertex); Assert(verts);
    i32 num_indices = (count - 2) * 3;

    i32 *indices = eco_arena_nmalloc(alloc, num_indices, i32); Assert(indices);

    for (i32 i = 0; i < count; i++)
    {
        verts[i].position = points[i];
        verts[i].color = fill_color;
        /* verts[i].tex_coord = (SDL_FPoint){0.0f, 0.0f}; */
    }

    /* Triangle fan tessellation from vertex 0 */
    i32 idx = 0;
    for(i32 i = 1; i < count - 1; i++)
    {
        indices[idx++] = 0;
        indices[idx++] = i;
        indices[idx++] = i + 1;
    }

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    return SDL_RenderGeometry(renderer, NULL, verts, count, indices, num_indices);
}

/* Helper function that draws a thick polyline with robust miter joints for open or closed loops. */
b32 
DrawPolylineThickEx(SDL_Renderer *renderer, SDL_FPoint const *points, i32 count, f32 width, SDL_FColor color, b32 closed, MagAllocator alloc_)
{
    StopIf(!renderer || !points || count < 2 || width <= 0.0f, return false);
    MagAllocator *alloc = &alloc_;

    f32 const miter_limit = 2.0f * width; /* Default to this value instead of keeping as argument. */

    /* FIX: If it's a closed loop and the last point duplicates the first point, */
    /* ignore the duplicate so our wrap-around math doesn't calculate a 0-length segment. */
    if(closed && count > 2)
    {
        f32 dx = points[count - 1].x - points[0].x;
        f32 dy = points[count - 1].y - points[0].y;
        if((dx * dx + dy * dy) < 0.0001f) { count--; }
    }

    f32 half_w = width * 0.5f;

    i32 segment_count = closed ? count : (count - 1);
    i32 num_vertices = count * 2;
    i32 num_indices = segment_count * 6;

    SDL_Vertex *vertices = eco_arena_nmalloc(alloc, num_vertices, SDL_Vertex);
    i32 *indices = eco_arena_nmalloc(alloc, num_indices, i32);
    StopIf(!vertices || !indices, return false);

    for(i32 i = 0; i < count; i++)
    {
        f32 nx = 0.0f, ny = 0.0f;

        if(!closed && i == 0)
        {
            /* Open start cap */
            f32 dx = points[1].x - points[0].x;
            f32 dy = points[1].y - points[0].y;
            f32 len = vec_length(dx, dy);
            if(len > 0.0f) { nx = -dy / len; ny = dx / len; }
        }
        else if(!closed && i == count - 1)
        {
            /* Open end cap */
            f32 dx = points[count - 1].x - points[count - 2].x;
            f32 dy = points[count - 1].y - points[count - 2].y;
            f32 len = vec_length(dx, dy);
            if(len > 0.0f) { nx = -dy / len; ny = dx / len; }
        } 
        else
        {
            /* Closed loop vertex or interior open vertex */
            i32 prev_idx = (i - 1 + count) % count;
            i32 next_idx = (i + 1) % count;

            /* Incoming segment direction vector (prev -> curr) */
            f32 u1x = points[i].x - points[prev_idx].x;
            f32 u1y = points[i].y - points[prev_idx].y;
            f32 len1 = vec_length(u1x, u1y);

            /* Outgoing segment direction vector (curr -> next) */
            f32 u2x = points[next_idx].x - points[i].x;
            f32 u2y = points[next_idx].y - points[i].y;
            f32 len2 = vec_length(u2x, u2y);

            if(len1 > 0.0f && len2 > 0.0f)
            {
                u1x /= len1; u1y /= len1;
                u2x /= len2; u2y /= len2;

                /* Perpendicular normals of segment 1 & 2 */
                f32 n1x = -u1y, n1y = u1x;
                f32 n2x = -u2y, n2y = u2x;

                /* Average normal direction */
                f32 miter_x = n1x + n2x;
                f32 miter_y = n1y + n2y;
                f32 miter_dist = vec_length(miter_x, miter_y);

                if(miter_dist > 0.001f)
                {
                    miter_x /= miter_dist;
                    miter_y /= miter_dist;

                    /* Half-angle dot product for exact miter length: 1 / cos(theta / 2) */
                    f32 dot_u = u1x * u2x + u1y * u2y;
                    if(dot_u < -1.0f) { dot_u = -1.0f; }
                    if(dot_u >  1.0f) { dot_u =  1.0f; }

                    f32 cos_half = sqrtf((1.0f + dot_u) * 0.5f);
                    f32 miter_len = (cos_half > 0.001f) ? (1.0f / cos_half) : 1.0f;

                    if(miter_len > miter_limit) { miter_len = miter_limit; }

                    nx = miter_x * miter_len;
                    ny = miter_y * miter_len;
                }
                else
                {
                    /* Fallback for hairpin turn */
                    nx = n1x;
                    ny = n1y;
                }
            }
        }

        i32 v_idx = i * 2;
        vertices[v_idx].position = (SDL_FPoint){ points[i].x + nx * half_w, points[i].y + ny * half_w };
        vertices[v_idx].color = color;
        /* vertices[v_idx].tex_coord = (SDL_FPoint){ 0.0f, 0.0f }; */

        vertices[v_idx + 1].position = (SDL_FPoint){ points[i].x - nx * half_w, points[i].y - ny * half_w };
        vertices[v_idx + 1].color = color;
        /* vertices[v_idx + 1].tex_coord = (SDL_FPoint){ 0.0f, 0.0f }; */
    }

    /* Connect adjacent pairs into quad strip */
    i32 idx_count = 0;
    for(i32 i = 0; i < segment_count; i++)
    {
        i32 i_next = (i + 1) % count;

        i32 top_left     = i * 2;
        i32 bottom_left  = i * 2 + 1;
        i32 top_right    = i_next * 2;
        i32 bottom_right = i_next * 2 + 1;

        indices[idx_count++] = top_left;
        indices[idx_count++] = bottom_left;
        indices[idx_count++] = bottom_right;

        indices[idx_count++] = top_left;
        indices[idx_count++] = bottom_right;
        indices[idx_count++] = top_right;
    }

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    return SDL_RenderGeometry(renderer, NULL, vertices, num_vertices, indices, num_indices);
}

/* Helper function that draws a thick, anti-aliased polyline with robust miter joints for open or closed loops. */
b32 
DrawPolylineThickExAA(SDL_Renderer *renderer, SDL_FPoint const *points, i32 count, f32 width, SDL_FColor color, b32 closed, MagAllocator alloc_)
{
    StopIf(!renderer || !points || count < 2 || width <= 0.0f, return false);
    MagAllocator *alloc = &alloc_;

    f32 const miter_limit = 2.0f * width; 

    /* Ignore duplicate last point on closed loops */
    if(closed && count > 2)
    {
        f32 dx = points[count - 1].x - points[0].x;
        f32 dy = points[count - 1].y - points[0].y;
        if((dx * dx + dy * dy) < 0.0001f) { count--; }
    }

    f32 half_w = width * 0.5f;
    f32 aa_fringe = 0.5f;
    f32 core_w = (half_w > aa_fringe) ? (half_w - aa_fringe) : 0.0f;
    f32 fade_w = half_w + aa_fringe;

    i32 segment_count = closed ? count : (count - 1);
    
    /* 4 vertices per point */
    i32 num_vertices = count * 4;
    /* 3 quads (18 indices) per segment */
    i32 num_indices = segment_count * 18;

    SDL_Vertex *vertices = eco_arena_nmalloc(alloc, num_vertices, SDL_Vertex);
    i32 *indices = eco_arena_nmalloc(alloc, num_indices, i32);
    StopIf(!vertices || !indices, return false);

    SDL_FColor color_inner = color;
    SDL_FColor color_outer = color;
    color_outer.a = 0.0f; /* Alpha fade for the outer fringes */

    for(i32 i = 0; i < count; i++)
    {
        f32 nx = 0.0f, ny = 0.0f;

        if(!closed && i == 0)
        {
            /* Open start cap */
            f32 dx = points[1].x - points[0].x;
            f32 dy = points[1].y - points[0].y;
            f32 len = vec_length(dx, dy);
            if(len > 0.0f) { nx = -dy / len; ny = dx / len; }
        }
        else if(!closed && i == count - 1)
        {
            /* Open end cap */
            f32 dx = points[count - 1].x - points[count - 2].x;
            f32 dy = points[count - 1].y - points[count - 2].y;
            f32 len = vec_length(dx, dy);
            if(len > 0.0f) { nx = -dy / len; ny = dx / len; }
        } 
        else
        {
            /* Closed loop vertex or interior open vertex */
            i32 prev_idx = (i - 1 + count) % count;
            i32 next_idx = (i + 1) % count;

            f32 u1x = points[i].x - points[prev_idx].x;
            f32 u1y = points[i].y - points[prev_idx].y;
            f32 len1 = vec_length(u1x, u1y);

            f32 u2x = points[next_idx].x - points[i].x;
            f32 u2y = points[next_idx].y - points[i].y;
            f32 len2 = vec_length(u2x, u2y);

            if(len1 > 0.0f && len2 > 0.0f)
            {
                u1x /= len1; u1y /= len1;
                u2x /= len2; u2y /= len2;

                f32 n1x = -u1y, n1y = u1x;
                f32 n2x = -u2y, n2y = u2x;

                f32 miter_x = n1x + n2x;
                f32 miter_y = n1y + n2y;
                f32 miter_dist = vec_length(miter_x, miter_y);

                if(miter_dist > 0.001f)
                {
                    miter_x /= miter_dist;
                    miter_y /= miter_dist;

                    f32 dot_u = u1x * u2x + u1y * u2y;
                    if(dot_u < -1.0f) { dot_u = -1.0f; }
                    if(dot_u >  1.0f) { dot_u =  1.0f; }

                    f32 cos_half = sqrtf((1.0f + dot_u) * 0.5f);
                    f32 miter_len = (cos_half > 0.001f) ? (1.0f / cos_half) : 1.0f;

                    if(miter_len > miter_limit) { miter_len = miter_limit; }

                    nx = miter_x * miter_len;
                    ny = miter_y * miter_len;
                }
                else
                {
                    /* Fallback for hairpin turn */
                    nx = n1x;
                    ny = n1y;
                }
            }
        }

        i32 v_idx = i * 4;

        /* Outer Left */
        vertices[v_idx + 0].position = (SDL_FPoint){ points[i].x + nx * fade_w, points[i].y + ny * fade_w };
        vertices[v_idx + 0].color = color_outer;

        /* Inner Left */
        vertices[v_idx + 1].position = (SDL_FPoint){ points[i].x + nx * core_w, points[i].y + ny * core_w };
        vertices[v_idx + 1].color = color_inner;

        /* Inner Right */
        vertices[v_idx + 2].position = (SDL_FPoint){ points[i].x - nx * core_w, points[i].y - ny * core_w };
        vertices[v_idx + 2].color = color_inner;

        /* Outer Right */
        vertices[v_idx + 3].position = (SDL_FPoint){ points[i].x - nx * fade_w, points[i].y - ny * fade_w };
        vertices[v_idx + 3].color = color_outer;
    }

    /* Connect adjacent pairs into 3 quad strips (Left fade, solid core, right fade) */
    i32 idx_count = 0;
    for(i32 i = 0; i < segment_count; i++)
    {
        i32 i_next = (i + 1) % count;

        i32 L0  = i * 4 + 0;
        i32 L1  = i * 4 + 1;
        i32 R1  = i * 4 + 2;
        i32 R0  = i * 4 + 3;

        i32 nL0 = i_next * 4 + 0;
        i32 nL1 = i_next * 4 + 1;
        i32 nR1 = i_next * 4 + 2;
        i32 nR0 = i_next * 4 + 3;

        /* Quad 1: Left Fringe */
        indices[idx_count++] = L0; indices[idx_count++] = L1; indices[idx_count++] = nL1;
        indices[idx_count++] = L0; indices[idx_count++] = nL1; indices[idx_count++] = nL0;

        /* Quad 2: Solid Core */
        indices[idx_count++] = L1; indices[idx_count++] = R1; indices[idx_count++] = nR1;
        indices[idx_count++] = L1; indices[idx_count++] = nR1; indices[idx_count++] = nL1;

        /* Quad 3: Right Fringe */
        indices[idx_count++] = R1; indices[idx_count++] = R0; indices[idx_count++] = nR0;
        indices[idx_count++] = R1; indices[idx_count++] = nR0; indices[idx_count++] = nR1;
    }

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    return SDL_RenderGeometry(renderer, NULL, vertices, num_vertices, indices, num_indices);
}

/* Draws a closed, filled polygon with sharp corners and a thick border. */
b32 
DrawPolygonThick(SDL_Renderer *renderer, SDL_FPoint const *points, i32 count, f32 border_width, SDL_FColor border_color, SDL_FColor fill_color, MagAllocator alloc_) 
{
    StopIf(!renderer || !points || count < 3, return false);
    MagAllocator *alloc = &alloc_;

    b32 status = true;

    /* Render solid interior fill */
    if(fill_color.a > 0.0f) { status &= RenderFilledPolygon(renderer, points, count, fill_color, alloc_); }

    /* Render thick mitered border along the closed loop */
    if(border_width > 0.0f && border_color.a > 0.0f)
    {
        /* Append first point to the end to form a continuous closed loop */
        SDL_FPoint *closed_pts = eco_arena_nmalloc(alloc, count + 1, SDL_FPoint); Assert(closed_pts);
        for (i32 i = 0; i < count; i++)
        {
            closed_pts[i] = points[i];
        }
        closed_pts[count] = points[0]; /* Close loop */

        status &= DrawPolylineThickExAA(renderer, closed_pts, count + 1, border_width, border_color, true, alloc_);
    }

    return status;
}

