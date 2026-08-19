#include "platform.h"
#include "pc98.h"

extern int zz_out;
extern void near zz_a(void);
extern void near zz_b(void);
extern void near zz_c(void);
extern void near zz_d(void);
extern void near zz_e(void);
extern void near zz_f(void);
extern void near zz_g(void);
extern void near zz_h(void);

// A SPARSE switch: widely separated case values, enough of them that Turbo C++
// cannot use a dense jump table. `kb/conventions/handwritten-asm-tells.md`
// claims this dispatcher emits `loop` to walk a `cs:` case-value table.
void near zz_sparse(int v)
{
	switch(v) {
	case 3:    zz_a(); break;
	case 17:   zz_b(); break;
	case 42:   zz_c(); break;
	case 99:   zz_d(); break;
	case 150:  zz_e(); break;
	case 231:  zz_f(); break;
	case 400:  zz_g(); break;
	case 1000: zz_h(); break;
	default:   zz_out = v; break;
	}
}
