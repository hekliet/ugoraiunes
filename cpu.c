#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "globals.h"

#define push(v) write(0x100 | sp--, (v))
#define pop() cpu_read(0x100 | ++sp)
#define push16(v)   \
    push((v) >> 8); \
    push((v) & 255)
#define sign8(v) (1 - ((v >> 7) << 1))

uint8_t cart_read(uint16_t addr);
uint16_t cart_read16(uint16_t addr);
void cpu_tables_init(void);

uint8_t ppu_reg_read(uint16_t addr);
void ppu_reg_write(uint16_t addr, uint8_t v);

extern void (*instrs[256])(void);
extern void (*addrmodes[256])(void);
extern uint8_t cycles_by_op[256];
extern uint8_t page_cross_extras[256];
extern uint8_t presentation_keys[8];

uint8_t fi = 1;
int cpu_extra_cycles;

static uint8_t a, x, y, sp;
static uint16_t pc;
static uint8_t fc, fz, fd, fv, fn;

static uint8_t ram[2048];

static uint8_t op;
static uint16_t arg;

static unsigned joy_strobe, joy_idx;

uint8_t cpu_read(uint16_t addr) {
    if (addr < 0x2000) return ram[addr & 0x7ff];

    if (addr < 0x4000) {
        return ppu_reg_read(addr & 7);
    }

    if (addr < 0x4015) return 0; /* open bus */

    if (addr == 0x4015) {
        // TODO: return APU status
        return 0;
    }

    if (addr == 0x4016 || addr == 0x4017) {
        if (addr == 0x4016) {
            if (joy_strobe) return presentation_keys[0];
            if (joy_idx < 8) return presentation_keys[joy_idx++];
            return 1;
        }
        return 0;
    }

    if (addr >= 0x4020) return cart_read(addr);

    assert(0);
}

static void write(uint16_t addr, uint8_t val) {
    if (addr < 0x2000) {
        ram[addr & 0x7ff] = val;
        return;
    }

    if (addr < 0x4000) {
        ppu_reg_write(addr & 7, val);
    }

    if (addr < 0x4014 || addr == 0x4015 || addr == 0x4017) {
        // TODO: apu_write(addr & 0x1f, val)
        return;
    }

    if (addr == 0x4014) {
        ppu_reg_write(0x14, val);
        return;
    }

    if (addr == 0x4016) {
        joy_strobe = val & 1;
        joy_idx = 0;
        return;
    }

    assert(0);
}

static uint16_t read16(uint16_t addr) { return cpu_read(addr) | (cpu_read(addr + 1) << 8); }

int cpu_step(void) {
    op = cpu_read(pc++);
    addrmodes[op]();
    instrs[op]();
    int cycles = cycles_by_op[op] + cpu_extra_cycles;
    cpu_extra_cycles = 0;
    return cycles;
}

static uint8_t flags(void) {
    return (fn << 7) | (fv << 6) | 32 | (fd << 3) | (fi << 2) | (fz << 1) | fc;
}

static void set_flags(uint8_t v) {
    /* clang-format off */
    fc = v & 1; v >>= 1;
    fz = v & 1; v >>= 1;
    fi = v & 1; v >>= 1;
    fd = v & 1; v >>= 3;
    fv = v & 1; v >>= 1;
    fn = v & 1;
    /* clang-format on */
}

int cpu_interrupt(int type) {
    /* ASSUME fi is checked before calling cpu_interrupt */
    if (type == RESET)
        sp -= 3;
    else {
        push16(pc);
        push(flags() | ((type == BRK) << 4));
    }
    fi = 1;
    uint16_t vect;
    switch (type) {
        case NMI:
            vect = 0xfffa;
            break;
        case RESET:
            vect = 0xfffc;
            break;
        default:
            vect = 0xfffe;
    }
    pc = read16(vect);
    return 7;
}

static void update_zn(uint8_t v) {
    fz = !v;
    fn = v >> 7;
}

static uint16_t pop16() {
    uint8_t lo = pop();
    return (pop() << 8) | lo;
}

void inx(void) { update_zn(++x); }
void inc(void) {
    uint8_t v = cpu_read(arg);
    write(arg, v);  // dummy write in read-modify-write instructions
    write(arg, ++v);
    update_zn(v);
}
void adc(void) {
    uint8_t olda = a;
    uint16_t w = a + arg + fc;
    a = w;
    update_zn(a);
    fc = w > 255;
    uint8_t sign_olda = sign8(olda);
    uint8_t sign_arg = sign8(arg);
    uint8_t sign_a = sign8(a);
    fv = sign_olda == sign_arg && sign_a != sign_arg;
}
void asl(void) {
    uint8_t v = cpu_read(arg);
    write(arg, v);  // dummy write in read-modify-write instructions
    fc = v >> 7;
    v <<= 1;
    write(arg, v);
    update_zn(v);
}
void asla(void) {
    fc = a >> 7;
    a <<= 1;
    update_zn(a);
}
void dec(void) {
    uint8_t v = cpu_read(arg);
    write(arg, v);  // dummy write in read-modify-write instructions
    write(arg, --v);
    update_zn(v);
}
void dex(void) { update_zn(--x); }
void dey(void) { update_zn(--y); }
void iny(void) { update_zn(++y); }
void lsr(void) {
    uint8_t v = cpu_read(arg);
    write(arg, v);  // dummy write in read-modify-write instructions
    fc = v & 1;
    v >>= 1;
    write(arg, v);
    update_zn(v);
}
void lsra(void) {
    fc = a & 1;
    a >>= 1;
    update_zn(a);
}
void rol(void) {
    uint8_t v = cpu_read(arg);
    write(arg, v);  // dummy write in read-modify-write instructions
    uint8_t oldc = fc;
    fc = v >> 7;
    v = (v << 1) | oldc;
    write(arg, v);
    update_zn(v);
}
void rola(void) {
    uint8_t oldc = fc;
    fc = a >> 7;
    a = (a << 1) | oldc;
    update_zn(a);
}
void ror(void) {
    uint8_t v = cpu_read(arg);
    write(arg, v);  // dummy write in read-modify-write instructions
    uint8_t oldc = fc;
    fc = v & 1;
    v = (oldc << 7) | (v >> 1);
    write(arg, v);
    update_zn(v);
}
void rora(void) {
    uint8_t oldc = fc;
    fc = a & 1;
    a = (oldc << 7) | (a >> 1);
    update_zn(a);
}
void sbc(void) {
    arg = ~arg & 255;
    adc();
}
void clc(void) { fc = 0; }
void cld(void) { fd = 0; }
void cli(void) { fi = 0; }
void clv(void) { fv = 0; }
void lda(void) {
    a = arg;
    update_zn(arg);
}
void ldx(void) {
    x = arg;
    update_zn(arg);
}
void ldy(void) {
    y = arg;
    update_zn(arg);
}
void pha(void) { push(a); }
void php(void) { push(flags() | 16); }
void pla(void) {
    a = pop();
    update_zn(a);
}
void plp(void) { set_flags(pop()); }
void sec(void) { fc = 1; }
void sed(void) { fd = 1; }
void sei(void) { fi = 1; }
void sta(void) { write(arg, a); }
void stx(void) { write(arg, x); }
void sty(void) { write(arg, y); }
void tax(void) {
    x = a;
    update_zn(x);
}
void txa(void) {
    a = x;
    update_zn(a);
}
void tay(void) {
    y = a;
    update_zn(y);
}
void tya(void) {
    a = y;
    update_zn(a);
}
void tsx(void) {
    x = sp;
    update_zn(x);
}
void txs(void) { sp = x; }
void bit(void) {
    fz = !(arg & a);
    fn = arg >> 7;
    fv = (arg >> 6) & 1;
}
void cmp(void) {
    fz = arg == a;
    fn = ((uint8_t)(a - arg)) >> 7;
    fc = a >= arg;
}
void cpx(void) {
    fz = arg == x;
    fn = ((uint8_t)(x - arg)) >> 7;
    fc = x >= arg;
}
void cpy(void) {
    fz = arg == y;
    fn = ((uint8_t)(y - arg)) >> 7;
    fc = y >= arg;
}
void and(void) {
    a &= arg;
    update_zn(a);
}
void eor(void) {
    a ^= arg;
    update_zn(a);
}
void ora(void) {
    a |= arg;
    update_zn(a);
}
void bcc(void) {
    if (!fc) {
        cpu_extra_cycles++;
        pc = arg;
    } else
        cpu_extra_cycles = 0;
}
void bcs(void) {
    if (fc) {
        cpu_extra_cycles++;
        pc = arg;
    } else
        cpu_extra_cycles = 0;
}
void beq(void) {
    if (fz) {
        cpu_extra_cycles++;
        pc = arg;
    } else
        cpu_extra_cycles = 0;
}
void bmi(void) {
    if (fn) {
        cpu_extra_cycles++;
        pc = arg;
    } else
        cpu_extra_cycles = 0;
}
void bne(void) {
    if (!fz) {
        cpu_extra_cycles++;
        pc = arg;
    } else
        cpu_extra_cycles = 0;
}
void bpl(void) {
    if (!fn) {
        cpu_extra_cycles++;
        pc = arg;
    } else
        cpu_extra_cycles = 0;
}
void bvc(void) {
    if (!fv) {
        cpu_extra_cycles++;
        pc = arg;
    } else
        cpu_extra_cycles = 0;
}
void bvs(void) {
    if (fv) {
        cpu_extra_cycles++;
        pc = arg;
    } else
        cpu_extra_cycles = 0;
}
void jmp(void) { pc = arg; }
void jsr(void) {
    push16(pc - 1);
    pc = arg;
}
void rts(void) { pc = pop16() + 1; }
void rti(void) {
    set_flags(pop());
    pc = pop16();
}
void brk(void) {
    pc++;
    cpu_interrupt(BRK);
}
void nop(void) {}
void inv(void) {
    fprintf(stderr, "%04x: Invalid opcode (%02x)\n", pc, op);
    exit(1);
}

void implicit(void) {}
void immediate_val(void) { arg = cpu_read(pc++); }
void absolute_addr(void) {
    arg = read16(pc);
    pc += 2;
}
void absolute_val(void) {
    absolute_addr();
    arg = cpu_read(arg);
}
void zeropage_addr(void) { arg = cpu_read(pc++); }
void zeropage_val(void) {
    zeropage_addr();
    arg = cpu_read(arg);
}
void relative_addr(void) {
    int8_t offset = cpu_read(pc++);
    arg = pc + offset;
    if (pc >> 8 != arg >> 8) cpu_extra_cycles = page_cross_extras[op];
}
void indirect_addr(void) {
    uint16_t addr_lo = read16(pc);
    pc += 2;
    uint16_t addr_hi = (addr_lo & 255) == 255 ? (addr_lo & 0xff00) : (addr_lo + 1);
    arg = (cpu_read(addr_hi) << 8) | cpu_read(addr_lo);
}
void indexed_zpx_addr(void) { arg = (x + cpu_read(pc++)) & 255; }
void indexed_zpx_val(void) {
    indexed_zpx_addr();
    arg = cpu_read(arg);
}
void indexed_zpy_addr(void) { arg = (y + cpu_read(pc++)) & 255; }
void indexed_zpy_val(void) {
    indexed_zpy_addr();
    arg = cpu_read(arg);
}
void indexed_absx_addr(void) {
    uint16_t abs = read16(pc);
    pc += 2;
    arg = x + abs;
    if (abs >> 8 != arg >> 8) cpu_extra_cycles = page_cross_extras[op];
}
void indexed_absx_val(void) {
    indexed_absx_addr();
    arg = cpu_read(arg);
}
void indexed_absy_addr(void) {
    uint16_t abs = read16(pc);
    pc += 2;
    arg = y + abs;
    if (abs >> 8 != arg >> 8) cpu_extra_cycles = page_cross_extras[op];
}
void indexed_absy_val(void) {
    indexed_absy_addr();
    arg = cpu_read(arg);
}
void indexed_indirect_addr(void) {
    uint8_t addr_lo = x + cpu_read(pc++);
    uint8_t addr_hi = addr_lo + 1;
    arg = (cpu_read(addr_hi) << 8) | cpu_read(addr_lo);
}
void indexed_indirect_val(void) {
    indexed_indirect_addr();
    arg = cpu_read(arg);
}
void indirect_indexed_addr(void) {
    uint8_t addr_lo = cpu_read(pc++);
    uint8_t addr_hi = addr_lo + 1;
    uint16_t base = (cpu_read(addr_hi) << 8) | cpu_read(addr_lo);
    arg = y + base;
    if (base >> 8 != arg >> 8) cpu_extra_cycles = page_cross_extras[op];
}
void indirect_indexed_val(void) {
    indirect_indexed_addr();
    arg = cpu_read(arg);
}
