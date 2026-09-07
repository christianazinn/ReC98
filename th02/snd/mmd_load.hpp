#ifndef TH02_SND_MMD_LOAD_HPP
#define TH02_SND_MMD_LOAD_HPP

#include "th02/snd/mmd_buffer.hpp"

static bool mmd_song_filename(const char *stem, char *fn, bool short_extension)
{
	unsigned i = 0;
	while((i < 8) && stem[i] && (stem[i] != '.')) {
		fn[i] = stem[i];
		i++;
	}
	if(!i || (stem[i] && (stem[i] != '.'))) {
		return false;
	}
	fn[i++] = '.';
	fn[i++] = 'm';
	if(!short_extension) {
		fn[i++] = 'm';
	}
	fn[i++] = 'd';
	fn[i] = '\0';
	return true;
}

static bool mmd_song_load(const char *fn)
{
	uint16_t seg = mmd_pause_driver();
	uint16_t capacity = mmd_song_capacity(seg);
	uint16_t song_seg;
	uint16_t song_off;
	uint16_t fn_seg = FP_SEG(fn);
	uint16_t fn_off = FP_OFF(fn);
	uint16_t length;
	uint16_t length_hi;
	uint16_t failed;
	uint16_t received;
	int fh;
	bool loaded = false;

	if(!capacity) {
		return false;
	}
	_AX = (KAJA_SONG_STOP << 8);
	geninterrupt(MMD);
	// Keep DOS I/O in this tail instead of pulling new CRT modules into _TEXT.
	_asm {
		push ds
		mov dx, fn_off
		mov ds, fn_seg
		mov ax, 3D00h
		int 21h
		pop ds
		sbb dx, dx
		or ax, dx
		mov fh, ax
	}
	if(fh < 0) {
		return false;
	}
	_asm {
		mov bx, fh
		xor cx, cx
		xor dx, dx
		mov ax, 4202h
		int 21h
		sbb cx, cx
		mov failed, cx
		mov length, ax
		mov length_hi, dx
	}
	if(!failed && !length_hi && length && (length <= capacity)) {
		_asm {
			mov bx, fh
			xor cx, cx
			xor dx, dx
			mov ax, 4200h
			int 21h
			sbb cx, cx
			or cx, ax
			or cx, dx
			mov failed, cx
		}
	}
	if(!failed && !length_hi && length && (length <= capacity)) {
		_asm {
			push ds
			mov ax, (KAJA_GET_SONG_ADDRESS shl 8)
			int MMD
			mov song_seg, ds
			mov song_off, dx
			pop ds
		}
		if((song_seg == seg) && (song_off == MMD_SONG_BUFFER_OFF)) {
			_asm {
				push ds
				mov bx, fh
				mov cx, length
				mov dx, song_off
				mov ds, song_seg
				mov ah, 3Fh
				int 21h
				pop ds
				sbb cx, cx
				mov failed, cx
				mov received, ax
			}
			loaded = (!failed && (received == length));
		}
	}
	_asm {
		mov bx, fh
		mov ah, 3Eh
		int 21h
		sbb ax, ax
		mov failed, ax
	}
	if(failed) {
		loaded = false;
	}
	return loaded;
}

#endif
