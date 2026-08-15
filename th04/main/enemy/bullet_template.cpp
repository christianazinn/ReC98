/// Enemy bullet templates
/// ----------------------

#pragma option -zPmain_03

#include <stddef.h>
#include "decomp.hpp"
#if (GAME == 5)
#include "th05/main/enemy/enemy.hpp"
#else
#include "th04/main/enemy/enemy.hpp"
#endif

#if (GAME == 5)
extern "C" void pascal near enemy_bullet_template_push(
	BulletTemplate near &tmpl
)
{
	_CX = (sizeof(BulletTemplate) / sizeof(uint16_t));
	prepare_si_di(FP_OFF(&bullet_template), 0, FP_OFF(&tmpl), 0);
	asm { push ds; pop es; }
	asm { rep movsw; }
}
#else
extern "C" void pascal near enemy_bullet_template_push(enemy_t near &enemy)
{
	_CX = (sizeof(BulletTemplate) / sizeof(uint16_t));
	prepare_si_di(
		FP_OFF(&bullet_template), 0,
		FP_OFF(&enemy), offsetof(enemy_t, bullet_template)
	);
	asm { push ds; pop es; }
	asm { rep movsw; }
}
#endif
