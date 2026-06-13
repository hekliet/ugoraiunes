#include <stdint.h>
#include <string.h>

#include "ppu_regs.h"

#define MIN(a, b) (((a) < (b)) ? (a) : (b))

uint8_t ppu_read(uint16_t addr);

extern ppu_ctrl_t ppu_ctrl;
extern ppu_mask_t ppu_mask;
extern ppu_status_t ppu_status;
extern uint8_t ppu_scroll_x, ppu_scroll_y;
extern uint8_t ppu_oam[256];
extern const unsigned rgbpalette[64];

uint32_t render_pixels[3][256 * 240];
int render_front, render_back = 1, render_third = 2;

typedef struct {
    uint8_t y;
    uint8_t tile;
    uint8_t palette : 2;
    uint8_t         : 3;
    uint8_t behind  : 1;
    uint8_t hflip   : 1;
    uint8_t vflip   : 1;
    uint8_t x;
} oam_sprite_t;

static uint8_t tile_slice[8];
static uint8_t color_indices[256 * 240];
static oam_sprite_t *sprites = (oam_sprite_t *)ppu_oam;
static unsigned buf_col[256];
static int8_t buf_spi[256];

static void prep_tile(unsigned ptntable_id, unsigned tile_id, unsigned tile_y) {
    uint16_t table_addr = ptntable_id * 0x1000;
    uint16_t plane_lo = table_addr + (tile_id << 4);
    uint16_t plane_hi = plane_lo + 8;
    uint8_t b_lo = ppu_read(plane_lo + tile_y);
    uint16_t b_hi = ppu_read(plane_hi + tile_y) << 1;
    for (int x = 7; x >= 0; x--) {
        tile_slice[x] = (b_hi & 2) | (b_lo & 1);
        b_hi >>= 1;
        b_lo >>= 1;
    }
}

unsigned apply_emph(unsigned colour) {
    unsigned r = colour >> 16;
    unsigned g = (colour >> 8) & 255;
    unsigned b = colour & 255;
    if (ppu_mask.f.greyscale) r = g = b = (r + g + b) / 3;
    if (ppu_mask.f.emph_r || ppu_mask.f.emph_g || ppu_mask.f.emph_b) {
        unsigned all = ppu_mask.f.emph_r && ppu_mask.f.emph_g && ppu_mask.f.emph_b;
        if (all || ppu_mask.f.emph_r) r = (192 * r) >> 8;
        if (all || ppu_mask.f.emph_g) g = (192 * g) >> 8;
        if (all || ppu_mask.f.emph_b) b = (192 * b) >> 8;
    }
    return (r << 16) | (g << 8) | b;
}

static void plot_bg(unsigned x, unsigned y, unsigned colour, uint8_t colindex) {
    unsigned idx = (y << 8) | x;
    color_indices[idx] = colindex;
    render_pixels[render_third][idx] = apply_emph(colour);
}

unsigned bg_palette(unsigned nametable, unsigned x, unsigned y) {
    uint16_t meta_base = 0x2000 + 0x400 * nametable + 960;
    uint16_t meta_idx = meta_base + ((y >> 5) << 3) | (x >> 5);
    unsigned meta = ppu_read(meta_idx);
    unsigned mx = x & 31;
    unsigned my = y & 31;
    unsigned block = ((my >> 4) << 1) | (mx >> 4);
    return (meta >> (block << 1)) & 3;
}

unsigned get_colour(unsigned palette, unsigned colidx, unsigned is_sprite) {
    uint16_t base = 0x3f00 | (is_sprite ? 0x10 : 0) | (palette << 2);
    return rgbpalette[ppu_read(base | colidx)];
}

void render_bg(unsigned y) {
    unsigned ptntable_id = ppu_ctrl.f.bg_ptntable;
    uint32_t bgcolor = get_colour(0, 0, 0);
    unsigned scrolled_y = y + ppu_scroll_y;
    unsigned nametable_y = scrolled_y % 240;
    unsigned tile_y = nametable_y & 7;

    for (unsigned x = 0; x < 256;) {
        unsigned scrolled_x = x + ppu_scroll_x;
        unsigned nametable_id =
            (ppu_ctrl.f.base_nametable + (scrolled_x >= 256 ? 1 : 0) + (scrolled_y >= 240 ? 2 : 0)) & 3;
        uint16_t nametable_addr = 0x2000 | nametable_id * 0x400;
        uint16_t nametable_x = scrolled_x & 255;
        uint16_t tile_x = nametable_x & 7;
        unsigned tile_pixels = MIN(8 - tile_x, 256 - x);
        uint16_t nametable_idx = nametable_addr + ((nametable_y >> 3) << 5) + (nametable_x >> 3);
        unsigned tile_id = ppu_read(nametable_idx);
        unsigned palette = bg_palette(nametable_id, nametable_x, nametable_y);
        prep_tile(ptntable_id, tile_id, tile_y);
        for (unsigned xx = 0; xx < tile_pixels; xx++) {
            if (!ppu_mask.f.bg_ena || (x + xx < 8 && !ppu_mask.f.show_leftmost_bg)) {
                plot_bg(x + xx, y, bgcolor, 0);
                continue;
            }
            uint8_t colidx = tile_slice[tile_x + xx];
            unsigned colour = colidx ? get_colour(palette, colidx, 0) : bgcolor;
            plot_bg(x + xx, y, colour, colidx);
        }
        x += tile_pixels;
    }
}

int evaluate(int8_t *sprlist, unsigned y, unsigned height) {
    oam_sprite_t *sp = sprites;
    int idx = 0;
    for (int si = 0; si < 64; si++, sp++) {
        int dy = y - sp->y - 1;
        if (!(dy >= 0 && dy < height)) continue;
        if (idx < 8)
            sprlist[idx++] = si;
        else {
            ppu_status.f.spr_overflow = 1;
            break;
        }
    }
    return idx - 1;
}

void render_spr(unsigned y) {
    if (!ppu_mask.f.spr_ena) return;
    uint8_t sprlist[8];
    unsigned tall = ppu_ctrl.f.spr_size;
    unsigned height = 8 << tall;
    int maxidx = evaluate(sprlist, y, height);
    if (maxidx < 0) return;
    memset(buf_spi, -1, 256);
    for (int i = maxidx; i >= 0; i--) {
        int spi = sprlist[i];
        oam_sprite_t *sp = sprites + spi;
        unsigned spry = y - sp->y - 1;
        unsigned tiley = spry & 7;
        unsigned ptntable = tall ? (sp->tile & 1) : ppu_ctrl.f.spr_ptntable;
        unsigned tile = sp->tile + ((spry > 8) ^ (tall & sp->vflip));
        unsigned rgbcols[4] = {get_colour(sp->palette, 0, 1), get_colour(sp->palette, 1, 1),
                               get_colour(sp->palette, 2, 1), get_colour(sp->palette, 3, 1)};
        prep_tile(ptntable, tile, sp->vflip ? (7 - tiley) : tiley);
        unsigned x = sp->x;
        int sprx_flipped, sprx_flipped_inc;
        if (sp->hflip) {
            sprx_flipped = 7;
            sprx_flipped_inc = -1;
        } else {
            sprx_flipped = 0;
            sprx_flipped_inc = 1;
        }
        for (unsigned sprx = 0; sprx < 8; sprx++, x++, sprx_flipped += sprx_flipped_inc) {
            if (x > 255) break;
            if (!ppu_mask.f.show_leftmost_spr && x < 8) continue;
            uint8_t colidx = tile_slice[sprx_flipped];
            if (colidx) {
                buf_spi[x] = spi;
                buf_col[x] = rgbcols[colidx];
                if (spi == 0 && ppu_mask.f.bg_ena && color_indices[(y << 8) | x])
                    ppu_status.f.spr_0hit = 1;
            }
        }
    }
    for (unsigned x = 0; x < 256; x++) {
        if (buf_spi[x] < 0) continue;
        oam_sprite_t *sp = sprites + buf_spi[x];
        unsigned idx = (y << 8) | x;
        if (!sp->behind || !color_indices[idx])
            render_pixels[render_third][idx] = apply_emph(buf_col[x]);
    }
}
