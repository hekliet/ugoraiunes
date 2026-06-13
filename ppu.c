#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#include "globals.h"
#include "ppu_regs.h"

uint8_t cart_read(uint16_t addr);
mirroring_t cart_nametable_mirroring(void);
uint8_t cpu_read(uint16_t addr);
void render_bg(unsigned y);
// void render_spr(unsigned y);

extern int cpu_extra_cycles;
extern int nmi_pending;

int ppu_frame_ready;

enum { CTRL, MASK, STATUS, OAMADDR, OAMDATA, SCROLL, ADDR, DATA, OAMDMA = 0x14 };

ppu_ctrl_t ppu_ctrl;
ppu_mask_t ppu_mask;
ppu_status_t ppu_status;
uint8_t oam_addr;
uint8_t ppu_scroll_x, ppu_scroll_y;
uint8_t ppu_oam[256];

static uint8_t vram[4096];
static uint8_t palette[32];
static mirroring_t mirroring;
static uint16_t ppu_address;
static uint8_t ppu_scroll_latch;
static int cycle, scanline = -1;

void ppu_init(void) {
    ppu_status.f.vblank = 1;
    mirroring = cart_nametable_mirroring();
}

void on_pre_line(void) {
    if (!ppu_mask.f.bg_ena && !ppu_mask.f.spr_ena) return;
    if (cycle == 1) {
        ppu_status.value = 0;
    }
}

void on_visible_line(void) {
    if (!cycle) {
        render_bg(scanline);
        render_spr(scanline);
    }
}

void on_vblank_line(void) {
    if (cycle == 1) {
        ppu_status.f.vblank = 1;
        if (ppu_ctrl.f.vblank_ena) nmi_pending = 1;
    }
}

void ppu_step(void) {
    if (scanline == -1)
        on_pre_line();
    else if (scanline < 240)
        on_visible_line();
    else if (scanline == 241)
        on_vblank_line();
    if (++cycle == 341) {
        cycle = 0;
        if (++scanline == 261) {
            scanline = -1;
            ppu_frame_ready = 1;  // nulled in main loop
        }
    }
}

static void ppu_addr_incr(void) { ppu_address += ppu_ctrl.f.vram_addr_incr ? 32 : 1; }

static uint16_t nametable_base[5][4] = {{0, 0, 0x400, 0x400},
                                        {0, 0x400, 0, 0x400},
                                        {0, 0, 0, 0},
                                        {0x400, 0x400, 0x400, 0x400},
                                        {0, 0x400, 0x800, 0xc00}};

uint8_t ppu_read(uint16_t addr) {
    if (addr < 0x2000) return cart_read(addr);

    if (addr < 0x3f00) {
        uint16_t key = (addr >> 10) & 3;
        uint16_t base = nametable_base[mirroring][key];
        return vram[base + (addr & 0x3ff)];
    }

    if (addr < 0x4000) {
        addr &= 0x1f;
        if (!(addr & 3)) return palette[addr & 15];
        return palette[addr & 0x1f];
    }

    assert(0);
}

static void ppu_write(uint16_t addr, uint8_t v) {
    if (addr < 0x2000) {
        fprintf(stderr, "Attempted unsupported CHR-RAM write.");
        return;
    }

    if (addr < 0x3f00) {
        uint16_t key = (addr >> 10) & 3;
        uint16_t base = nametable_base[mirroring][key];
        vram[base + (addr & 0x3ff)] = v;
        return;
    }

    if (addr < 0x4000) {
        addr &= 0x1f;
        if (!(addr & 3)) addr &= 15;
        palette[addr & 0x1f] = v;
        return;
    }

    assert(0);
}

uint8_t ppu_reg_read(uint16_t addr) {
    static uint8_t ppu_data_buffer = 0;
    switch (addr) {
        case STATUS:
            uint8_t oldvalue = ppu_status.value;
            ppu_status.f.vblank = 0;
            ppu_scroll_latch = 0;
            return oldvalue;
        case OAMDATA:
            return ppu_oam[oam_addr];
        case DATA:
            uint8_t data = ppu_data_buffer;
            ppu_data_buffer = ppu_read(ppu_address & 0x3fff);
            if (addr >= 0x3f00 && addr < 0x4000) data = ppu_data_buffer;
            ppu_addr_incr();
            return data;
        default:
            return 0;  // TODO: open bus read?
    }
}

void ppu_reg_write(uint16_t addr, uint8_t v) {
    switch (addr) {
        case CTRL:
            ppu_ctrl.value = v;
            break;
        case MASK:
            ppu_mask.value = v;
            break;
        case STATUS:
            ppu_scroll_latch = 0;  // ?
            break;
        case OAMADDR:
            oam_addr = v;
            break;
        case OAMDATA:
            ppu_oam[oam_addr++] = v;
            break;
        case SCROLL:
            if (ppu_scroll_latch)
                ppu_scroll_y = v;
            else
                ppu_scroll_x = v;
            ppu_scroll_latch ^= 1;
            break;
        case ADDR:
            if (ppu_scroll_latch)
                ppu_address = (ppu_address & 0xff00) | v;
            else
                ppu_address = ((v & 0xbf) << 8) | (ppu_address & 255);
            ppu_scroll_latch ^= 1;
            break;
        case DATA:
            ppu_write(ppu_address & 0x3fff, v);
            ppu_addr_incr();
            break;
        case OAMDMA:
            uint16_t addr = v << 8;
            for (int i = 0; i < 256; i++) ppu_oam[i] = cpu_read(addr++);
            cpu_extra_cycles += 513;
            break;
    }
}
