#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

int uses_chrram;

static int prg_pages, chr_pages;
static int vmirror, mapper;
static uint8_t *prg, *chr;

static void cart_free(void) {
    free(chr);
    free(prg);
}

void cart_load(const char *path) {
    FILE *f = fopen(path, "rb");
    uint32_t magic;
    fread(&magic, 4, 1, f);
    if (magic != 0x1a53454e) {
        fprintf(stderr, "Not an iNES file.\n");
        exit(1);
    }
    prg_pages = fgetc(f);
    chr_pages = fgetc(f);
    uses_chrram = chr_pages == 0;
    uint8_t b = fgetc(f);
    vmirror = b & 1;
    mapper = fgetc(f) | (b >> 4);
    fprintf(stderr, "%dx16k PRG, %dx8k CHR, mapper %d, hard-vmirror: %s\n", prg_pages, chr_pages,
            mapper, vmirror ? "yes" : "no");
    prg = malloc(prg_pages * 16384);
    chr = malloc(chr_pages * 8192);
    atexit(cart_free);
    fseek(f, 8, SEEK_CUR);
    fread(prg, 16384, prg_pages, f);
    fread(chr, 8192, chr_pages, f);
    fclose(f);
}

uint8_t cart_read(uint16_t addr) {
    if (addr & 0x8000) return prg[addr & (prg_pages == 2 ? 0x7fff : 0x3fff)];
    assert(addr < 0x2000);
    return chr[addr];
}

uint16_t cart_read16(uint16_t addr) { return cart_read(addr) | (cart_read(addr + 1) << 8); }
