/* clang-format off */
#include <stdint.h>

void adc(void); void and (void); void asl(void); void asla(void);
void bcc(void); void bcs (void); void beq(void); void bit (void);
void bmi(void); void bne (void); void bpl(void); void brk (void);
void bvc(void); void bvs (void); void clc(void); void cld (void);
void cli(void); void clv (void); void cmp(void); void cpx (void);
void cpy(void); void dec (void); void dex(void); void dey (void);
void eor(void); void inc (void); void inv(void); void inx (void);
void iny(void); void jmp (void); void jsr(void); void lda (void);
void ldx(void); void ldy (void); void lsr(void); void lsra(void);
void nop(void); void ora (void); void pha(void); void php (void);
void pla(void); void plp (void); void rol(void); void rola(void);
void ror(void); void rora(void); void rti(void); void rts (void);
void sbc(void); void sec (void); void sed(void); void sei (void);
void sta(void); void stx (void); void sty(void); void tax (void);
void tay(void); void tsx (void); void txa(void); void txs (void);
void tya(void);

void implicit(void);              void immediate_val(void);
void absolute_addr(void);         void absolute_val(void);
void zeropage_addr(void);         void zeropage_val(void);
void relative_addr(void);         void indirect_addr(void);
void indexed_zpx_addr(void);      void indexed_zpx_val(void);
void indexed_zpy_addr(void);      void indexed_zpy_val(void);
void indexed_absx_addr(void);     void indexed_absx_val(void);
void indexed_absy_addr(void);     void indexed_absy_val(void);
void indexed_indirect_addr(void); void indexed_indirect_val(void);
void indirect_indexed_addr(void); void indirect_indexed_val(void);

void (*instrs[256])(void);
void (*addrmodes[256])(void);


uint8_t cycles_by_op[256] = {
    7, 6, 0, 0, 0, 3, 5, 0, 3, 2, 2, 0, 0, 4, 6, 0, 2, 5, 0, 0, 0, 4, 6, 0, 2, 4, 0, 0, 0, 4, 7, 0,
    6, 6, 0, 0, 3, 3, 5, 0, 4, 2, 2, 0, 4, 4, 6, 0, 2, 5, 0, 0, 0, 4, 6, 0, 2, 4, 0, 0, 0, 4, 7, 0,
    6, 6, 0, 0, 0, 3, 5, 0, 3, 2, 2, 0, 3, 4, 6, 0, 2, 5, 0, 0, 0, 4, 6, 0, 2, 4, 0, 0, 0, 4, 7, 0,
    6, 6, 0, 0, 0, 3, 5, 0, 4, 2, 2, 0, 5, 4, 6, 0, 2, 5, 0, 0, 0, 4, 6, 0, 2, 4, 0, 0, 0, 4, 7, 0,
    0, 6, 0, 0, 3, 3, 3, 0, 2, 0, 2, 0, 4, 4, 4, 0, 2, 6, 0, 0, 4, 4, 4, 0, 2, 5, 2, 0, 0, 5, 0, 0,
    2, 6, 2, 0, 3, 3, 3, 0, 2, 2, 2, 0, 4, 4, 4, 0, 2, 5, 0, 0, 4, 4, 4, 0, 2, 4, 2, 0, 4, 4, 4, 0,
    2, 6, 0, 0, 3, 3, 5, 0, 2, 2, 2, 0, 4, 4, 6, 0, 2, 5, 0, 0, 0, 4, 6, 0, 2, 4, 0, 0, 0, 4, 7, 0,
    2, 6, 0, 0, 3, 3, 5, 0, 2, 2, 2, 0, 4, 4, 6, 0, 2, 5, 0, 0, 0, 4, 6, 0, 2, 4, 0, 0, 0, 4, 7, 0
};

uint8_t page_cross_extras[256] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 1, 1, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0
};

void cpu_tables_init(void) {
    for (int i = 0; i < 256; i++) {
        instrs[i] = inv;
        addrmodes[i] = implicit;
    }
    instrs[0x00] = brk ; instrs[0x01] = ora; instrs[0x05] = ora ; instrs[0x06] = asl;
    instrs[0x08] = php ; instrs[0x09] = ora; instrs[0x0a] = asla; instrs[0x0d] = ora;
    instrs[0x0e] = asl ; instrs[0x10] = bpl; instrs[0x11] = ora ; instrs[0x15] = ora;
    instrs[0x16] = asl ; instrs[0x18] = clc; instrs[0x19] = ora ; instrs[0x1d] = ora;
    instrs[0x1e] = asl ; instrs[0x20] = jsr; instrs[0x21] = and ; instrs[0x24] = bit;
    instrs[0x25] = and ; instrs[0x26] = rol; instrs[0x28] = plp ; instrs[0x29] = and;
    instrs[0x2a] = rola; instrs[0x2c] = bit; instrs[0x2d] = and ; instrs[0x2e] = rol;
    instrs[0x30] = bmi ; instrs[0x31] = and; instrs[0x35] = and ; instrs[0x36] = rol;
    instrs[0x38] = sec ; instrs[0x39] = and; instrs[0x3d] = and ; instrs[0x3e] = rol;
    instrs[0x40] = rti ; instrs[0x41] = eor; instrs[0x45] = eor ; instrs[0x46] = lsr;
    instrs[0x48] = pha ; instrs[0x49] = eor; instrs[0x4a] = lsra; instrs[0x4c] = jmp;
    instrs[0x4d] = eor ; instrs[0x4e] = lsr; instrs[0x50] = bvc ; instrs[0x51] = eor;
    instrs[0x55] = eor ; instrs[0x56] = lsr; instrs[0x58] = cli ; instrs[0x59] = eor;
    instrs[0x5d] = eor ; instrs[0x5e] = lsr; instrs[0x60] = rts ; instrs[0x61] = adc;
    instrs[0x65] = adc ; instrs[0x66] = ror; instrs[0x68] = pla ; instrs[0x69] = adc;
    instrs[0x6a] = rora; instrs[0x6c] = jmp; instrs[0x6d] = adc ; instrs[0x6e] = ror;
    instrs[0x70] = bvs ; instrs[0x71] = adc; instrs[0x75] = adc ; instrs[0x76] = ror;
    instrs[0x78] = sei ; instrs[0x79] = adc; instrs[0x7d] = adc ; instrs[0x7e] = ror;
    instrs[0x81] = sta ; instrs[0x84] = sty; instrs[0x85] = sta ; instrs[0x86] = stx;
    instrs[0x88] = dey ; instrs[0x8a] = txa; instrs[0x8c] = sty ; instrs[0x8d] = sta;
    instrs[0x8e] = stx ; instrs[0x90] = bcc; instrs[0x91] = sta ; instrs[0x94] = sty;
    instrs[0x95] = sta ; instrs[0x96] = stx; instrs[0x98] = tya ; instrs[0x99] = sta;
    instrs[0x9a] = txs ; instrs[0x9d] = sta; instrs[0xa0] = ldy ; instrs[0xa1] = lda;
    instrs[0xa2] = ldx ; instrs[0xa4] = ldy; instrs[0xa5] = lda ; instrs[0xa6] = ldx;
    instrs[0xa8] = tay ; instrs[0xa9] = lda; instrs[0xaa] = tax ; instrs[0xac] = ldy;
    instrs[0xad] = lda ; instrs[0xae] = ldx; instrs[0xb0] = bcs ; instrs[0xb1] = lda;
    instrs[0xb4] = ldy ; instrs[0xb5] = lda; instrs[0xb6] = ldx ; instrs[0xb8] = clv;
    instrs[0xb9] = lda ; instrs[0xba] = tsx; instrs[0xbc] = ldy ; instrs[0xbd] = lda;
    instrs[0xbe] = ldx ; instrs[0xc0] = cpy; instrs[0xc1] = cmp ; instrs[0xc4] = cpy;
    instrs[0xc5] = cmp ; instrs[0xc6] = dec; instrs[0xc8] = iny ; instrs[0xc9] = cmp;
    instrs[0xca] = dex ; instrs[0xcc] = cpy; instrs[0xcd] = cmp ; instrs[0xce] = dec;
    instrs[0xd0] = bne ; instrs[0xd1] = cmp; instrs[0xd5] = cmp ; instrs[0xd6] = dec;
    instrs[0xd8] = cld ; instrs[0xd9] = cmp; instrs[0xdd] = cmp ; instrs[0xde] = dec;
    instrs[0xe0] = cpx ; instrs[0xe1] = sbc; instrs[0xe4] = cpx ; instrs[0xe5] = sbc;
    instrs[0xe6] = inc ; instrs[0xe8] = inx; instrs[0xe9] = sbc ; instrs[0xea] = nop;
    instrs[0xec] = cpx ; instrs[0xed] = sbc; instrs[0xee] = inc ; instrs[0xf0] = beq;
    instrs[0xf1] = sbc ; instrs[0xf5] = sbc; instrs[0xf6] = inc ; instrs[0xf8] = sed;
    instrs[0xf9] = sbc ; instrs[0xfd] = sbc; instrs[0xfe] = inc ;
    addrmodes[0x01] = indexed_indirect_val; addrmodes[0x05] = zeropage_val;
    addrmodes[0x06] = zeropage_addr       ; addrmodes[0x09] = immediate_val;
    addrmodes[0x0d] = absolute_val        ; addrmodes[0x0e] = absolute_addr;
    addrmodes[0x10] = relative_addr       ; addrmodes[0x11] = indirect_indexed_val;
    addrmodes[0x15] = indexed_zpx_val     ; addrmodes[0x16] = indexed_zpx_addr;
    addrmodes[0x19] = indexed_absy_val    ; addrmodes[0x1d] = indexed_absx_val;
    addrmodes[0x1e] = indexed_absx_addr   ; addrmodes[0x20] = absolute_addr;
    addrmodes[0x21] = indexed_indirect_val; addrmodes[0x24] = zeropage_val;
    addrmodes[0x25] = zeropage_val        ; addrmodes[0x26] = zeropage_addr;
    addrmodes[0x29] = immediate_val       ; addrmodes[0x2c] = absolute_val;
    addrmodes[0x2d] = absolute_val        ; addrmodes[0x2e] = absolute_addr;
    addrmodes[0x30] = relative_addr       ; addrmodes[0x31] = indirect_indexed_val;
    addrmodes[0x35] = indexed_zpx_val     ; addrmodes[0x36] = indexed_zpx_addr;
    addrmodes[0x39] = indexed_absy_val    ; addrmodes[0x3d] = indexed_absx_val;
    addrmodes[0x3e] = indexed_absx_addr   ; addrmodes[0x41] = indexed_indirect_val;
    addrmodes[0x45] = zeropage_val        ; addrmodes[0x46] = zeropage_addr;
    addrmodes[0x49] = immediate_val       ; addrmodes[0x4c] = absolute_addr;
    addrmodes[0x4d] = absolute_val        ; addrmodes[0x4e] = absolute_addr;
    addrmodes[0x50] = relative_addr       ; addrmodes[0x51] = indirect_indexed_val;
    addrmodes[0x55] = indexed_zpx_val     ; addrmodes[0x56] = indexed_zpx_addr;
    addrmodes[0x59] = indexed_absy_val    ; addrmodes[0x5d] = indexed_absx_val;
    addrmodes[0x5e] = indexed_absx_addr   ; addrmodes[0x61] = indexed_indirect_val;
    addrmodes[0x65] = zeropage_val        ; addrmodes[0x66] = zeropage_addr;
    addrmodes[0x69] = immediate_val       ; addrmodes[0x6c] = indirect_addr;
    addrmodes[0x6d] = absolute_val        ; addrmodes[0x6e] = absolute_addr;
    addrmodes[0x70] = relative_addr       ; addrmodes[0x71] = indirect_indexed_val;
    addrmodes[0x75] = indexed_zpx_val     ; addrmodes[0x76] = indexed_zpx_addr;
    addrmodes[0x79] = indexed_absy_val    ; addrmodes[0x7d] = indexed_absx_val;
    addrmodes[0x7e] = indexed_absx_addr   ; addrmodes[0x81] = indexed_indirect_addr;
    addrmodes[0x84] = zeropage_addr       ; addrmodes[0x85] = zeropage_addr;
    addrmodes[0x86] = zeropage_addr       ; addrmodes[0x8c] = absolute_addr;
    addrmodes[0x8d] = absolute_addr       ; addrmodes[0x8e] = absolute_addr;
    addrmodes[0x90] = relative_addr       ; addrmodes[0x91] = indirect_indexed_addr;
    addrmodes[0x94] = indexed_zpx_addr    ; addrmodes[0x95] = indexed_zpx_addr;
    addrmodes[0x96] = indexed_zpy_addr    ; addrmodes[0x99] = indexed_absy_addr;
    addrmodes[0x9d] = indexed_absx_addr   ; addrmodes[0xa0] = immediate_val;
    addrmodes[0xa1] = indexed_indirect_val; addrmodes[0xa2] = immediate_val;
    addrmodes[0xa4] = zeropage_val        ; addrmodes[0xa5] = zeropage_val;
    addrmodes[0xa6] = zeropage_val        ; addrmodes[0xa9] = immediate_val;
    addrmodes[0xac] = absolute_val        ; addrmodes[0xad] = absolute_val;
    addrmodes[0xae] = absolute_val        ; addrmodes[0xb0] = relative_addr;
    addrmodes[0xb1] = indirect_indexed_val; addrmodes[0xb4] = indexed_zpx_val;
    addrmodes[0xb5] = indexed_zpx_val     ; addrmodes[0xb6] = indexed_zpy_val;
    addrmodes[0xb9] = indexed_absy_val    ; addrmodes[0xbc] = indexed_absx_val;
    addrmodes[0xbd] = indexed_absx_val    ; addrmodes[0xbe] = indexed_absy_val;
    addrmodes[0xc0] = immediate_val       ; addrmodes[0xc1] = indexed_indirect_val;
    addrmodes[0xc4] = zeropage_val        ; addrmodes[0xc5] = zeropage_val;
    addrmodes[0xc6] = zeropage_addr       ; addrmodes[0xc9] = immediate_val;
    addrmodes[0xcc] = absolute_val        ; addrmodes[0xcd] = absolute_val;
    addrmodes[0xce] = absolute_addr       ; addrmodes[0xd0] = relative_addr;
    addrmodes[0xd1] = indirect_indexed_val; addrmodes[0xd5] = indexed_zpx_val;
    addrmodes[0xd6] = indexed_zpx_addr    ; addrmodes[0xd9] = indexed_absy_val;
    addrmodes[0xdd] = indexed_absx_val    ; addrmodes[0xde] = indexed_absx_addr;
    addrmodes[0xe0] = immediate_val       ; addrmodes[0xe1] = indexed_indirect_val;
    addrmodes[0xe4] = zeropage_val        ; addrmodes[0xe5] = zeropage_val;
    addrmodes[0xe6] = zeropage_addr       ; addrmodes[0xe9] = immediate_val;
    addrmodes[0xec] = absolute_val        ; addrmodes[0xed] = absolute_val;
    addrmodes[0xee] = absolute_addr       ; addrmodes[0xf0] = relative_addr;
    addrmodes[0xf1] = indirect_indexed_val; addrmodes[0xf5] = indexed_zpx_val;
    addrmodes[0xf6] = indexed_zpx_addr    ; addrmodes[0xf9] = indexed_absy_val;
    addrmodes[0xfd] = indexed_absx_val    ; addrmodes[0xfe] = indexed_absx_addr;
}
