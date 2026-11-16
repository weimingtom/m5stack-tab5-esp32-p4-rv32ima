#ifndef VGA_H
#define VGA_H

#include <stdbool.h>
#include <stdint.h>
#if defined(__MINGW32__)
#include "pci.h"
#endif

typedef struct FBDevice FBDevice;

typedef void SimpleFBDrawFunc(void *opaque,
                              int x, int y, int w, int h);

typedef struct VGAState VGAState;
VGAState *vga_init(char *vga_ram, int vga_ram_size,
                   uint8_t *fb, int width, int height);
void vga_set_force_8dm(VGAState *s, int v);

int vga_step(VGAState *vga);
void vga_refresh(VGAState *s,
                 SimpleFBDrawFunc *redraw_func, void *opaque, int full_update);

void vga_ioport_write(VGAState *s, uint32_t addr, uint32_t val);
uint32_t vga_ioport_read(VGAState *s, uint32_t addr);

void vbe_write(VGAState *s, uint32_t offset, uint32_t val);
uint32_t vbe_read(VGAState *s, uint32_t offset);

void vga_mem_write(VGAState *s, uint32_t addr, uint8_t val);
uint8_t vga_mem_read(VGAState *s, uint32_t addr);
void vga_mem_write16(VGAState *s, uint32_t addr, uint16_t val);
void vga_mem_write32(VGAState *s, uint32_t addr, uint32_t val);
bool vga_mem_write_string(VGAState *s, uint32_t addr, uint8_t *buf, int len);

#if !defined(__MINGW32__)
/*see pci.h*/
typedef struct PCIDevice PCIDevice;
typedef struct PCIBus PCIBus;
#endif
PCIDevice *vga_pci_init(VGAState *s, PCIBus *bus,
                        void *o, void (*set_bar)(void *, int, uint32_t, bool));

#ifndef BPP
#define BPP 32
#endif

typedef struct VGATextOps {
    void (*set_cursor)(void *o, int y, int x);
    void (*put_char2)(void *o, int y, int x, int ch, int attr);
    void (*refresh)(void *o, const uint8_t *ram, int w, int h);
} VGATextOps;

void vga_set_text_ops(VGAState *s, VGATextOps *ops, void *o);

#endif /* VGA_H */
