#ifndef TERM_H
#define TERM_H

#if defined(__MINGW32__)
#include "vga.h"
#endif

typedef struct Term Term;
#if !defined(__MINGW32__)
/*see vga.h*/
typedef struct VGAState VGAState;
#endif
Term *term_init(
	VGAState *vga,
	void (*kbd_cb)(void *, unsigned char scan_code, int is_pressed),
	void *kbd);
void term_step(Term *s);


#endif /* TERM_H */
