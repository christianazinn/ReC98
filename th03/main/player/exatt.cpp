#include "th03/main/player/exatt.hpp"
#include "th03/main/bullet/bullet.hpp"
#include "th03/main/collmap.hpp"
#include "th03/main/hitbox.hpp"
#include "th03/main/player/cur.hpp"

extern "C" uint8_t exatt_buffers[];
extern "C" uint8_t byte_202B8[];
extern "C" uint8_t byte_202B9[];
extern "C" uint8_t byte_202BA[];
extern "C" uint16_t word_2028A;
extern "C" uint16_t far randring_far_next16_raw(void);
extern "C" void near sub_1A1A7(void);
extern "C" void near kotohime_19E2A(void);
extern "C" void near kotohime_19EF9(void);
extern "C" void near kotohime_19F1F(void);

extern "C" void near kotohime_19F87(void)
{
	uint8_t angle_delta;
	register int i;

	bullet_template.type = static_cast<bullet_type_t>(
		byte_202BA[pid_current * 4]
	);
	bullet_template.speed.v = (1 << 4);
	bullet_template.count = byte_202B8[pid_current * 4];
	if(byte_202B9[pid_current * 4] != 0) {
		angle_delta = 7;
	} else {
		angle_delta = -7;
	}

	for(i = 0; i < 10; i++) {
		bullets_add();
		bullet_template.speed.v += 2;
		bullet_template.angle += angle_delta;
	}
}

void far exatt_update_kotohime(void)
{
	uint8_t pid_other;
	register uint8_t near *slot;
	register int i;

	_AL = pid_current;
	_AH = 0;
	_AX <<= 9;
	_AX += reinterpret_cast<uint16_t>(exatt_buffers);
	word_2028A = _AX;

	_AL = 1;
	_AL -= pid_current;
	pid_other = _AL;
	collmap_stripe_tile_w.v = (12 / COLLMAP_TILE_W);
	collmap_tile_h.v = (12 / COLLMAP_TILE_H);
	collmap_pid = _AL;

	i = 0;
	goto loop_test;
loop:
	if(*reinterpret_cast<uint8_t near *>(word_2028A) != 0) {
		slot = reinterpret_cast<uint8_t near *>(word_2028A);
		if(slot[0] == 1) {
			if(
				*reinterpret_cast<subpixel_t near *>(slot + 4) >=
				*reinterpret_cast<subpixel_t near *>(slot + 0x0E)
			) {
				if(slot[0x12] == 4) {
					kotohime_19EF9();
					if(slot[0x11] != 0) {
						bullet_template.speed.v = ((1 << 4) + 12);
						bullet_template.angle = randring_far_next16_raw();
						bullet_template.group = BG_RING;
						bullet_template.pid = pid_other;
						bullet_template.center.x.v = (
							*reinterpret_cast<subpixel_t near *>(slot + 2)
						);
						bullet_template.center.y.v = (
							*reinterpret_cast<subpixel_t near *>(slot + 4)
						);
						kotohime_19F1F();
					}
				} else if((slot[0x12] == 8) || (slot[0x12] == 0x0C)) {
					kotohime_19EF9();
				} else if(slot[0x12] == 0x10) {
					kotohime_19EF9();
					bullet_template.speed.v = ((1 << 4) + 8);
					bullet_template.angle = randring_far_next16_raw();
					bullet_template.group = BG_RING;
					bullet_template.pid = pid_other;
					bullet_template.center.x.v = (
						*reinterpret_cast<subpixel_t near *>(slot + 2)
					);
					bullet_template.center.y.v = (
						*reinterpret_cast<subpixel_t near *>(slot + 4)
					);
					if(slot[0x11] == 0) {
						bullet_template.type = BT_BULLET16_DEFAULT;
						bullet_template.count = 10;
						bullets_add();
					} else {
						kotohime_19F87();
					}
					slot[0] = 0;
				}
				slot[0x12]++;
			} else {
				*reinterpret_cast<subpixel_t near *>(slot + 4) += (
					*reinterpret_cast<subpixel_t near *>(slot + 0x14)
				);
				hitbox_hittest_skip_explosions = true;
				hitbox.radius.x.v = (16 << 4);
				hitbox.radius.y.v = (16 << 4);
				hitbox.pid = pid_other;
				hitbox.origin.center.x.v = (
					*reinterpret_cast<subpixel_t near *>(slot + 2)
				);
				hitbox.origin.center.y.v = (
					*reinterpret_cast<subpixel_t near *>(slot + 4)
				);
				hitbox_hittest();
				hitbox_hittest_skip_explosions = false;
				collmap_center.x.v = (
					*reinterpret_cast<subpixel_t near *>(slot + 2)
				);
				collmap_center.y.v = (
					*reinterpret_cast<subpixel_t near *>(slot + 4)
				);
				collmap_set_rect_striped();
			}
		} else if(slot[0] == 2) {
			sub_1A1A7();
		} else if(slot[0] <= 0x1C) {
			slot[0]++;
		} else {
			slot[1] = 0;
			slot[0] = 1;
		}
		slot[1]++;
	}
	i++;
	word_2028A += 0x20;
loop_test:
	if(i < 10) {
		goto loop;
	}
}

void far exatt_render_kotohime(void)
{
	register int i;

	_AL = pid_current;
	_AH = 0;
	_AX <<= 9;
	_AX += reinterpret_cast<uint16_t>(exatt_buffers);
	word_2028A = _AX;

	i = 0;
	goto loop_test;
loop:
	if(*reinterpret_cast<uint8_t near *>(word_2028A) != 0) {
		kotohime_19E2A();
	}
	i++;
	word_2028A += 0x20;
loop_test:
	if(i < 10) {
		goto loop;
	}
}

void pascal exatt_add(Subpixel center_x_, Subpixel center_y_, pid_t pid)
{
	// ZUN bloat: Bloat within bloat!
	subpixel_t center_x = center_x_;
	subpixel_t center_y = center_y_;

	static_assert(PLAYER_COUNT == 2);
	if(pid == 0) {
		exatt_funcs[0].add(center_x, center_y, 0);
	} else {
		exatt_funcs[1].add(center_x, center_y, 1);
	}
}
