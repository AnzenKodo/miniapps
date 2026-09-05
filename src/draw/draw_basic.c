internal void draw_grid(Rng2_F32 rect, float cell_size, float thickness, Vec4_F32 color)
{
    for (float x = rect.x0; x < rect.x1; x += cell_size)
    {
        Rng2_F32 hori_line_rect = rng2p(x, rect.y0, x + thickness, rect.y1);
        draw_rect(hori_line_rect, color, 0.f, 0.f, 0.f);
    }
    for (float y = rect.y0; y < rect.y1; y += cell_size)
    {
        Rng2_F32 vert_line_rect = rng2p(rect.x0, y, rect.x1, y + thickness);
        draw_rect(vert_line_rect, color, 0.f, 0.f, 0.f);
    }
}
