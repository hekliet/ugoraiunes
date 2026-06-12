#include <stdint.h>

typedef union {
    uint8_t value;
    struct {
        uint8_t base_nametable : 2;
        uint8_t vram_addr_incr : 1;
        uint8_t spr_ptntable   : 1;
        uint8_t bg_ptntable    : 1;
        uint8_t spr_size       : 1;
        uint8_t                : 1;
        uint8_t vblank_ena     : 1;
    } f;
} ppu_ctrl_t;

typedef union {
    uint8_t value;
    struct {
        uint8_t greyscale         : 1;
        uint8_t show_leftmost_bg  : 1;
        uint8_t show_leftmost_spr : 1;
        uint8_t bg_ena            : 1;
        uint8_t spr_ena           : 1;
        uint8_t emph_r            : 1;
        uint8_t emph_g            : 1;
        uint8_t emph_b            : 1;
    } f;
} ppu_mask_t;

typedef union {
    uint8_t value;
    struct {
        uint8_t              : 5;
        uint8_t spr_overflow : 1;
        uint8_t spr_0hit     : 1;
        uint8_t vblank       : 1;
    } f;
} ppu_status_t;
