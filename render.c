#include <assert.h>
#include <stdint.h>

#include "ppu_regs.h"

#define MIN(a, b) (((a) < (b)) ? (a) : (b))

uint8_t ppu_read(uint16_t addr);

extern ppu_ctrl_t ppu_ctrl;
extern ppu_mask_t ppu_mask;
extern uint8_t ppu_scroll_x, ppu_scroll_y;

uint32_t render_pixels[256 * 240];

static uint8_t tile_slice[8];
static uint8_t color_indices[256 * 240];

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

static void plot_bg(unsigned x, unsigned y, unsigned colour, uint8_t colindex) {
    unsigned idx = (y << 8) | x;
    color_indices[idx] = colindex;
    render_pixels[idx] = colour;
}

void render_bg(unsigned y) {
    unsigned ptntable_id = ppu_ctrl.f.bg_ptntable;
    uint32_t bgcolor = 0xcccccc;
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
        prep_tile(ptntable_id, tile_id, tile_y);
        for (unsigned xx = 0; xx < tile_pixels; xx++) {
            if (!ppu_mask.f.bg_ena || (x + xx < 8 && !ppu_mask.f.show_leftmost_bg)) {
                plot_bg(x + xx, y, bgcolor, 0);
                continue;
            }
            uint8_t colidx = tile_slice[tile_x + xx];
            uint8_t v = 12 - 4 * colidx;
            v = (v << 4) | v;
            unsigned colour = (v << 16) | (v << 8) | v;
            plot_bg(x + xx, y, colour, colidx);
        }
        x += tile_pixels;
    }
}
