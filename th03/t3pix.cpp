#pragma option -zCT3PIX_TEXT -zPT3PIX

#define TH03_PIXEL_CAPTURE_IMPLEMENTATION
#include "libs/master.lib/master.hpp"
#include "libs/master.lib/pc98_gfx.hpp"
#include "pc98.h"
#include "th03/pixel_capture.hpp"
#if (BINARY == 'M')
#include "th03/main/round.hpp"
#endif
#include "x86real.h"

#pragma option -a1

// T3PIX2 is a disposable developer oracle. It deliberately uses independent
// DOS handles instead of master.lib's single buffered file handle, which can
// be live while replay recording or playback is active.

static const unsigned T3PIX_STREAM_HEADER_SIZE = 64;
static const unsigned T3PIX_RECORD_HEADER_SIZE = 208;
static const unsigned T3PIX_SCRATCH_SIZE = (
	T3PIX_RECORD_HEADER_SIZE + (4 * sizeof(uint32_t))
);
static const unsigned long T3PIX_PLANE_RAW_SIZE = 32000UL;
static const unsigned T3PIX_TRAM_RAW_SIZE = (80 * 25 * 2);
static const unsigned T3PIX_GAIJI_RAW_SIZE = (256 * 32);
static const uint16_t T3PIX_RECORD_RAW = 0x0001;
static const uint16_t T3PIX_RECORD_TEXT_SHOWN = 0x0002;
static const uint32_t T3PIX_RAW_PAYLOAD_SIZE = (
	(T3PIX_PLANE_RAW_SIZE * 4UL) + (T3PIX_TRAM_RAW_SIZE * 2UL) +
	T3PIX_GAIJI_RAW_SIZE
);

static char T3PIX_STREAM_FN[11] = "T3PIX2.BIN";
static char T3PIX_CONTROL_FN[11] = "T3PIXC.BIN";

extern "C" {
int far pascal t3pix_dos_create(const char far *filename, int attribute);
int far pascal t3pix_dos_write(int fh, const void far *buffer, unsigned size);
long far pascal t3pix_dos_seek(int fh, long offset, int mode);
}

extern "C" unsigned graph_VramWords;
extern "C" unsigned graph_VramLines;
extern "C" unsigned graph_VramZoom;
extern "C" unsigned PaletteNote;
extern "C" unsigned int TextShown;
#if (BINARY == 'M')
extern bool palette_changed;
#endif

static bool t3pix_initialized;
static bool t3pix_disabled;
static uint8_t t3pix_shown_page;
static uint8_t t3pix_access_page;
static bool t3pix_shown_page_known;
static bool t3pix_access_page_known;
static uint16_t t3pix_scroll_y;
static uint32_t t3pix_publication;
static uint32_t t3pix_logical_sample = T3PIX_ID_NONE;
static uint32_t t3pix_replay_global_frame = T3PIX_ID_NONE;
static uint32_t t3pix_round_frame = T3PIX_ID_NONE;
static uint32_t t3pix_invocation = T3PIX_ID_NONE;
static uint32_t t3pix_transaction;
static uint16_t t3pix_commit_ordinal;
static uint16_t t3pix_scene_visit;
static t3pix_scene_t t3pix_scene = T3PIX_SCENE_UNKNOWN;
static bool t3pix_scene_entered;
static uint8_t t3pix_applied_palette[sizeof(Palettes)];
static int16_t t3pix_applied_tone;
static uint16_t t3pix_publication_stride = 1;
static uint16_t t3pix_control_version;
static uint8_t t3pix_target_process;
static uint8_t t3pix_replay_checkpoint;
static uint8_t t3pix_control_selection;
static uint32_t t3pix_target_invocation = T3PIX_ID_NONE;
static uint32_t t3pix_target_publication = T3PIX_ID_NONE;
static void t3pix_clear(uint8_t far *dst, unsigned size)
{
	while(size != 0) {
		*dst++ = 0;
		size--;
	}
}

static void t3pix_u16_put(uint8_t far *dst, uint16_t value)
{
	dst[0] = static_cast<uint8_t>(value);
	dst[1] = static_cast<uint8_t>(value >> 8);
}

static void t3pix_u32_put(uint8_t far *dst, uint32_t value)
{
	dst[0] = static_cast<uint8_t>(value);
	dst[1] = static_cast<uint8_t>(value >> 8);
	dst[2] = static_cast<uint8_t>(value >> 16);
	dst[3] = static_cast<uint8_t>(value >> 24);
}

static uint16_t t3pix_u16_get(const uint8_t far *src)
{
	return static_cast<uint16_t>(src[0] | (src[1] << 8));
}

static uint32_t t3pix_u32_get(const uint8_t far *src)
{
	return (
		static_cast<uint32_t>(src[0]) |
		(static_cast<uint32_t>(src[1]) << 8) |
		(static_cast<uint32_t>(src[2]) << 16) |
		(static_cast<uint32_t>(src[3]) << 24)
	);
}

static int t3pix_dos_read(int fh, void far *buffer, unsigned size)
{
	int result;
	asm {
		push	ds
		mov 	bx, fh
		lds 	dx, buffer
		mov 	cx, size
		mov 	ah, 3Fh
		int 	21h
		jnc 	read_done
		neg 	ax
	read_done:
		mov 	result, ax
		pop 	ds
	}
	return result;
}

static int t3pix_open(const char far *fn, unsigned mode)
{
	long ret = dos_axdx(static_cast<int>(0x3D00 | mode), fn);
	return ((ret < 0) ? static_cast<int>(ret) : static_cast<int>(ret & 0xFFFF));
}

static bool t3pix_write(int fh, const void far *data, unsigned size)
{
	return (t3pix_dos_write(fh, data, size) == static_cast<int>(size));
}

static void t3pix_palette_snapshot(void)
{
	const uint8_t near *src = reinterpret_cast<const uint8_t near *>(&Palettes);
	int tone = static_cast<int16_t>(PaletteTone);
	unsigned i;

	for(i = 0; i < sizeof(Palettes); i++) {
		t3pix_applied_palette[i] = src[i];
	}
	if(tone < 0) {
		tone = 0;
	} else if(tone > 200) {
		tone = 200;
	}
	t3pix_applied_tone = static_cast<int16_t>(tone);
}

static void t3pix_control_read(void)
{
	uint8_t control[24];
	int fh = t3pix_open(T3PIX_CONTROL_FN, 0);
	if(fh < 0) {
		return;
	}
	if(
		(t3pix_dos_read(fh, control, sizeof(control)) == sizeof(control)) &&
		(control[0] == 'T') && (control[1] == '3') &&
		(control[2] == 'P') && (control[3] == 'C') &&
		(t3pix_u16_get(&control[4]) == 2) &&
		(t3pix_u16_get(&control[6]) == sizeof(control)) &&
		(control[9] < 15) && (control[10] <= 1)
	) {
		t3pix_control_version = 2;
		t3pix_target_process = control[8];
		t3pix_replay_checkpoint = control[9];
		t3pix_control_selection = control[10];
		t3pix_target_invocation = t3pix_u32_get(&control[12]);
		t3pix_invocation = t3pix_target_invocation;
		t3pix_target_publication = t3pix_u32_get(&control[16]);
		t3pix_publication_stride = t3pix_u16_get(&control[20]);
	}
	dos_close(fh);
}

static void t3pix_initialize(void)
{
	int fh;
	if(t3pix_initialized) {
		return;
	}
	t3pix_initialized = true;
	t3pix_shown_page = 0;
	t3pix_access_page = 0;
	t3pix_scroll_y = 0;
#if (BINARY == 'O')
	// OP initializes the GDC itself before the first source publication.
	t3pix_shown_page_known = true;
	t3pix_access_page_known = true;
	t3pix_scene = T3PIX_SCENE_TITLE;
	t3pix_scene_entered = true;
#elif (BINARY == 'M')
	t3pix_scene = T3PIX_SCENE_GAMEPLAY;
	t3pix_scene_entered = true;
#endif
	t3pix_palette_snapshot();
	t3pix_control_read();
	// An exact v2 plan produces one route artifact. Once that stream exists,
	// later executables on the same route must not relabel themselves as the
	// externally selected process instance.
	if((t3pix_control_version == 2) && (t3pix_publication_stride == 0)) {
		fh = t3pix_open(T3PIX_STREAM_FN, 0);
		if(fh >= 0) {
			dos_close(fh);
			t3pix_disabled = true;
		}
	}
}

static uint8_t t3pix_process(void)
{
#if (BINARY == 'O')
	return 1;
#elif (BINARY == 'M')
	return 2;
#elif (BINARY == 'L')
	return 3;
#else
	return 0;
#endif
}

static uint32_t t3pix_fnv1a(const uint8_t far *src, unsigned size)
{
	uint32_t hash = 2166136261UL;
	while(size != 0) {
		hash ^= *src++;
		hash *= 16777619UL;
		size--;
	}
	return hash;
}

static uint32_t t3pix_fnv1a_extend(
	uint32_t hash, const uint8_t far *src, unsigned size
)
{
	while(size != 0) {
		hash ^= *src++;
		hash *= 16777619UL;
		size--;
	}
	return hash;
}

static bool t3pix_raw_selected(
	t3pix_event_t event, t3pix_boundary_t boundary
)
{
	if(
		(t3pix_control_version == 2) &&
		(t3pix_control_selection == 1)
	) {
		return (
			(event == T3PIX_EVENT_SHOW_PAGE) &&
			(boundary == T3PIX_BOUNDARY_STATE) &&
			(t3pix_target_publication == t3pix_logical_sample) &&
			(t3pix_target_publication == t3pix_replay_global_frame) &&
			(t3pix_round_frame == 0)
		);
	}
	return (
		(t3pix_control_version == 2) &&
		(t3pix_target_publication == t3pix_publication)
	);
}

static bool t3pix_target_domain_selected(void)
{
	if(t3pix_control_version != 2) {
		return true;
	}
	return (
		((t3pix_target_process == 0) ||
		 (t3pix_target_process == t3pix_process())) &&
		((t3pix_target_invocation == T3PIX_ID_NONE) ||
		 (t3pix_target_invocation == t3pix_invocation))
	);
}

static void t3pix_stream_header_fill(uint8_t far *header)
{
	t3pix_clear(header, T3PIX_STREAM_HEADER_SIZE);
	header[0] = 'T'; header[1] = '3'; header[2] = 'P'; header[3] = 'I';
	header[4] = 'X'; header[5] = '2'; header[6] = 0x0D; header[7] = 0x0A;
	t3pix_u16_put(&header[8], 2);
	t3pix_u16_put(&header[10], T3PIX_STREAM_HEADER_SIZE);
	t3pix_u16_put(&header[12], T3PIX_RECORD_HEADER_SIZE);
	t3pix_u16_put(&header[14], 1);
	t3pix_u16_put(&header[16], 640);
	t3pix_u16_put(&header[18], 400);
	t3pix_u16_put(&header[20], 80);
	t3pix_u32_put(&header[22], T3PIX_PLANE_RAW_SIZE);
	t3pix_u16_put(&header[26], T3PIX_TRAM_RAW_SIZE);
	t3pix_u32_put(&header[28], 0x43028F09UL);
	t3pix_u32_put(&header[32], 0x00000415UL);
	header[36] = 'R'; header[37] = 'e'; header[38] = 'p'; header[39] = 'l';
	header[40] = 'a'; header[41] = 'y'; header[42] = '-'; header[43] = '0';
	header[44] = '.'; header[45] = '4'; header[46] = '.'; header[47] = '1';
	header[48] = '5';
	t3pix_u16_put(&header[50], T3PIX_GAIJI_RAW_SIZE);
	t3pix_u32_put(&header[52], T3PIX_RAW_PAYLOAD_SIZE);
}

static int t3pix_stream_open_append(void)
{
	uint8_t header[T3PIX_STREAM_HEADER_SIZE];
	int fh = t3pix_open(T3PIX_STREAM_FN, 2);
	long size;

	if(fh < 0) {
		fh = t3pix_dos_create(T3PIX_STREAM_FN, 0);
		if(fh < 0) {
			return fh;
		}
		t3pix_stream_header_fill(header);
		if(!t3pix_write(fh, header, sizeof(header))) {
			dos_close(fh);
			return -1;
		}
	}
	size = t3pix_dos_seek(fh, 0, SEEK_END);
	if(size < 0) {
		dos_close(fh);
		return -1;
	}
	if(size == 0) {
		t3pix_stream_header_fill(header);
		if(!t3pix_write(fh, header, sizeof(header))) {
			dos_close(fh);
			return -1;
		}
	}
	return fh;
}

static void t3pix_record_fill(
	uint8_t far *record, uint16_t flags, uint32_t far *plane_hash,
	uint32_t tram_jis_hash, uint32_t tram_atrb_hash, uint32_t payload_hash,
	t3pix_event_t event, t3pix_boundary_t boundary
)
{
	const uint8_t near *pending = (
		reinterpret_cast<const uint8_t near *>(&Palettes)
	);
	uint32_t record_size = T3PIX_RECORD_HEADER_SIZE;
	unsigned i;

	if(flags & T3PIX_RECORD_RAW) {
		record_size += T3PIX_RAW_PAYLOAD_SIZE;
	}
	t3pix_clear(record, T3PIX_RECORD_HEADER_SIZE);
	record[0] = 'P'; record[1] = 'X'; record[2] = 'F'; record[3] = 'R';
	t3pix_u32_put(&record[4], record_size);
	t3pix_u32_put(&record[8], t3pix_publication);
	t3pix_u32_put(&record[12], t3pix_logical_sample);
	t3pix_u32_put(&record[16], t3pix_replay_global_frame);
	t3pix_u16_put(&record[20], vsync_Count2);
	record[22] = t3pix_process();
	record[23] = static_cast<uint8_t>(t3pix_scene);
	t3pix_u16_put(&record[24], flags);
	record[26] = t3pix_shown_page;
	record[27] = t3pix_access_page;
	t3pix_u16_put(&record[28], graph_VramLines);
	record[30] = static_cast<uint8_t>(graph_VramZoom);
#if (BINARY == 'M')
	record[31] = static_cast<uint8_t>(palette_changed != false);
#endif
	t3pix_u16_put(&record[32], t3pix_scroll_y);
	t3pix_u16_put(&record[34], PaletteTone);
	t3pix_u16_put(&record[36], t3pix_applied_tone);
	t3pix_u16_put(&record[38], PaletteNote);
	for(i = 0; i < sizeof(Palettes); i++) {
		record[40 + i] = pending[i];
		record[88 + i] = t3pix_applied_palette[i];
	}
	for(i = 0; i < 4; i++) {
		t3pix_u32_put(&record[136 + (i * 4)], plane_hash[i]);
	}
	t3pix_u32_put(&record[152], tram_jis_hash);
	t3pix_u32_put(&record[156], tram_atrb_hash);
	t3pix_u32_put(&record[160], t3pix_invocation);
	t3pix_u32_put(&record[164], T3PIX_ID_NONE);
	t3pix_u32_put(&record[168], t3pix_replay_global_frame);
	t3pix_u32_put(&record[172], t3pix_round_frame);
	t3pix_u16_put(&record[176], t3pix_scene_visit);
	record[178] = static_cast<uint8_t>(event);
	record[179] = static_cast<uint8_t>(boundary);
	t3pix_u32_put(&record[180], t3pix_transaction);
	t3pix_u16_put(&record[184], t3pix_commit_ordinal);
	record[186] = 0; // Raster origin: full 400-line reservation.
	record[187] = static_cast<uint8_t>(t3pix_control_version);
	t3pix_u32_put(&record[188],
		((flags & T3PIX_RECORD_RAW) ? T3PIX_RAW_PAYLOAD_SIZE : 0)
	);
	t3pix_u16_put(&record[192],
		((flags & T3PIX_RECORD_RAW) ? T3PIX_GAIJI_RAW_SIZE : 0)
	);
	t3pix_u16_put(&record[194], t3pix_publication_stride);
	t3pix_u32_put(&record[196], payload_hash);
	record[200] = t3pix_replay_checkpoint;
	record[201] = t3pix_control_selection;
	t3pix_u32_put(&record[204], t3pix_target_publication);
}

static void t3pix_publication_advance(t3pix_boundary_t boundary)
{
	t3pix_publication++;
	if(boundary == T3PIX_BOUNDARY_COMMIT) {
		t3pix_transaction++;
	}
	t3pix_commit_ordinal++;
}

void far pascal t3pix_publish(
	t3pix_event_t event, t3pix_boundary_t boundary
)
{
	static const uint16_t PLANE_SEGMENTS[4] = {
		SEG_PLANE_B, SEG_PLANE_R, SEG_PLANE_G, SEG_PLANE_E
	};
	uint8_t __seg *scratch_seg;
	uint8_t far *record;
	uint32_t far *plane_hash;
	uint32_t tram_jis_hash;
	uint32_t tram_atrb_hash;
	uint32_t payload_hash = 0;
	uint8_t __seg *gaiji_seg = 0;
	uint8_t far *gaiji = 0;
	unsigned active_plane_size;
	unsigned i;
	uint16_t flags = 0;
	int fh;

	t3pix_initialize();
	if(t3pix_disabled) {
		return;
	}
	// MAIN and MAINL inherit the GDC across executable transitions. Until the
	// source explicitly selects both pages, a zero-initialized shadow would be
	// invented evidence. Preserve the source-event ordinal but emit no record.
	if(!t3pix_shown_page_known || !t3pix_access_page_known) {
		t3pix_publication_advance(boundary);
		return;
	}
	// A v2 control selects one process-instance domain. Do not enter any of
	// the capture call chain for unmatched domains; apart from wasting work,
	// its depth can exceed these binaries' original 128-byte stack.
	if(!t3pix_target_domain_selected()) {
		t3pix_publication_advance(boundary);
		return;
	}
	if(t3pix_raw_selected(event, boundary)) {
		flags |= T3PIX_RECORD_RAW;
	} else if(
		(t3pix_publication_stride == 0) ||
		((t3pix_publication % t3pix_publication_stride) != 0)
	) {
		t3pix_publication_advance(boundary);
		return;
	}
	// Avoid overflowing the original 128-byte stack without shifting BSS in
	// binaries that have not yet been made fully position-independent.
	scratch_seg = reinterpret_cast<uint8_t __seg *>(
		hmem_allocbyte(T3PIX_SCRATCH_SIZE)
	);
	if(scratch_seg == 0) {
		t3pix_disabled = true;
		return;
	}
	record = reinterpret_cast<uint8_t far *>(MK_FP(
		reinterpret_cast<uint16_t>(scratch_seg), 0
	));
	plane_hash = reinterpret_cast<uint32_t far *>(
		&record[T3PIX_RECORD_HEADER_SIZE]
	);
	active_plane_size = (graph_VramWords * 2);
	if(active_plane_size > T3PIX_PLANE_RAW_SIZE) {
		t3pix_disabled = true;
		hmem_free(scratch_seg);
		return;
	}
	outportb(0xA6, t3pix_shown_page);
	for(i = 0; i < 4; i++) {
		plane_hash[i] = t3pix_fnv1a(
			reinterpret_cast<const uint8_t far *>(MK_FP(PLANE_SEGMENTS[i], 0)),
			active_plane_size
		);
	}
	outportb(0xA6, t3pix_access_page);
	tram_jis_hash = t3pix_fnv1a(
		reinterpret_cast<const uint8_t far *>(MK_FP(SEG_TRAM_JIS, 0)),
		T3PIX_TRAM_RAW_SIZE
	);
	tram_atrb_hash = t3pix_fnv1a(
		reinterpret_cast<const uint8_t far *>(MK_FP(SEG_TRAM_ATRB, 0)),
		T3PIX_TRAM_RAW_SIZE
	);
	if(flags & T3PIX_RECORD_RAW) {
		gaiji_seg = reinterpret_cast<uint8_t __seg *>(
			hmem_allocbyte(T3PIX_GAIJI_RAW_SIZE)
		);
		if(gaiji_seg == 0) {
			t3pix_disabled = true;
			hmem_free(scratch_seg);
			return;
		}
		gaiji = reinterpret_cast<uint8_t far *>(MK_FP(
			reinterpret_cast<uint16_t>(gaiji_seg), 0
		));
		gaiji_read_all(gaiji);
		payload_hash = 2166136261UL;
		outportb(0xA6, t3pix_shown_page);
		for(i = 0; i < 4; i++) {
			payload_hash = t3pix_fnv1a_extend(
				payload_hash,
				reinterpret_cast<const uint8_t far *>(
					MK_FP(PLANE_SEGMENTS[i], 0)
				),
				T3PIX_PLANE_RAW_SIZE
			);
		}
		outportb(0xA6, t3pix_access_page);
		payload_hash = t3pix_fnv1a_extend(
			payload_hash,
			reinterpret_cast<const uint8_t far *>(MK_FP(SEG_TRAM_JIS, 0)),
			T3PIX_TRAM_RAW_SIZE
		);
		payload_hash = t3pix_fnv1a_extend(
			payload_hash,
			reinterpret_cast<const uint8_t far *>(MK_FP(SEG_TRAM_ATRB, 0)),
			T3PIX_TRAM_RAW_SIZE
		);
		payload_hash = t3pix_fnv1a_extend(
			payload_hash, gaiji, T3PIX_GAIJI_RAW_SIZE
		);
	}
	if(TextShown != 0) {
		flags |= T3PIX_RECORD_TEXT_SHOWN;
	}
	t3pix_record_fill(
		record, flags, plane_hash, tram_jis_hash, tram_atrb_hash,
		payload_hash, event, boundary
	);
	fh = t3pix_stream_open_append();
	if(
		(fh < 0) ||
		!t3pix_write(fh, record, T3PIX_RECORD_HEADER_SIZE)
	) {
		if(fh >= 0) {
			dos_close(fh);
		}
		t3pix_disabled = true;
		if(gaiji_seg != 0) {
			hmem_free(gaiji_seg);
		}
		hmem_free(scratch_seg);
		return;
	}
	if(flags & T3PIX_RECORD_RAW) {
		outportb(0xA6, t3pix_shown_page);
		for(i = 0; i < 4; i++) {
			if(!t3pix_write(
				fh, MK_FP(PLANE_SEGMENTS[i], 0), T3PIX_PLANE_RAW_SIZE
			)) {
				t3pix_disabled = true;
				break;
			}
		}
		outportb(0xA6, t3pix_access_page);
		if(
			!t3pix_disabled &&
			!t3pix_write(fh, MK_FP(SEG_TRAM_JIS, 0), T3PIX_TRAM_RAW_SIZE)
		) {
			t3pix_disabled = true;
		}
		if(
			!t3pix_disabled &&
			!t3pix_write(fh, MK_FP(SEG_TRAM_ATRB, 0), T3PIX_TRAM_RAW_SIZE)
		) {
			t3pix_disabled = true;
		}
		if(!t3pix_disabled && !t3pix_write(fh, gaiji, T3PIX_GAIJI_RAW_SIZE)) {
			t3pix_disabled = true;
		}
	}
	dos_close(fh);
	if(gaiji_seg != 0) {
		hmem_free(gaiji_seg);
	}
	hmem_free(scratch_seg);
	t3pix_publication_advance(boundary);
}

void far pascal t3pix_scene_set(t3pix_scene_t scene)
{
	t3pix_initialize();
	if(!t3pix_scene_entered) {
		t3pix_scene = scene;
		t3pix_scene_entered = true;
		t3pix_commit_ordinal = 0;
	} else if(scene != t3pix_scene) {
		t3pix_scene = scene;
		t3pix_scene_visit++;
		t3pix_commit_ordinal = 0;
	}
}

void far pascal t3pix_logical_identity_set(
	uint32_t logical_sample, uint32_t replay_global_frame,
	uint32_t round_frame
)
{
	if(
		(logical_sample != t3pix_logical_sample) ||
		(replay_global_frame != t3pix_replay_global_frame) ||
		(round_frame != t3pix_round_frame)
	) {
		t3pix_commit_ordinal = 0;
	}
	t3pix_logical_sample = logical_sample;
	t3pix_replay_global_frame = replay_global_frame;
	t3pix_round_frame = round_frame;
}

void far pascal t3pix_graph_showpage(unsigned page)
{
	t3pix_initialize();
	t3pix_shown_page = static_cast<uint8_t>(page & 1);
	t3pix_shown_page_known = true;
	outportb(0xA4, page);
	t3pix_publish(T3PIX_EVENT_SHOW_PAGE, T3PIX_BOUNDARY_STATE);
}

void far pascal t3pix_graph_accesspage(unsigned page)
{
	t3pix_initialize();
	t3pix_access_page = static_cast<uint8_t>(page & 1);
	t3pix_access_page_known = true;
	outportb(0xA6, page);
}

void far pascal t3pix_graph_accesspage_track(unsigned page)
{
	t3pix_initialize();
	t3pix_access_page = static_cast<uint8_t>(page & 1);
	t3pix_access_page_known = true;
}

int far pascal t3pix_graph_copy_page(int page)
{
	int ret = graph_copy_page(page);
	if(ret) {
		t3pix_graph_accesspage_track(page);
	}
	return ret;
}

void far pascal t3pix_graph_scrollup(unsigned line)
{
	t3pix_initialize();
	t3pix_scroll_y = static_cast<uint16_t>(
		(line < graph_VramLines) ? line : 0
	);
#if (BINARY == 'L')
	// Only MAINL contains ZUN's graph_scrollup() contribution, and is also the
	// only TH03 binary that calls it.
	graph_scrollup(line);
#endif
	t3pix_publish(T3PIX_EVENT_SCROLL_APPLY, T3PIX_BOUNDARY_STATE);
}

void far pascal t3pix_palette_show(void)
{
	palette_show();
	t3pix_palette_snapshot();
	t3pix_publish(T3PIX_EVENT_PALETTE_APPLY, T3PIX_BOUNDARY_STATE);
}

void far pascal t3pix_vsync_wait(void)
{
	t3pix_publish(T3PIX_EVENT_VSYNC_BOUNDARY, T3PIX_BOUNDARY_COMMIT);
	vsync_wait();
}

static void t3pix_palette_fade_wait(unsigned speed)
{
	while(speed != 0) {
		vsync_wait();
		speed--;
	}
}

void far pascal t3pix_palette_black_in(unsigned speed)
{
	PaletteTone = 0;
	vsync_wait();
	do {
		palette_show();
		t3pix_palette_snapshot();
		t3pix_publish(T3PIX_EVENT_FADE_STEP_COMMIT, T3PIX_BOUNDARY_COMMIT);
		t3pix_palette_fade_wait(speed);
		PaletteTone += 6;
	} while(static_cast<int16_t>(PaletteTone) < 100);
	PaletteTone = 100;
	palette_show();
	t3pix_palette_snapshot();
	t3pix_publish(T3PIX_EVENT_FADE_STEP_COMMIT, T3PIX_BOUNDARY_COMMIT);
}

void far pascal t3pix_palette_black_out(unsigned speed)
{
	PaletteTone = 100;
	vsync_wait();
	do {
		palette_show();
		t3pix_palette_snapshot();
		t3pix_publish(T3PIX_EVENT_FADE_STEP_COMMIT, T3PIX_BOUNDARY_COMMIT);
		t3pix_palette_fade_wait(speed);
		PaletteTone -= 6;
	} while(static_cast<int16_t>(PaletteTone) > 0);
	PaletteTone = 0;
	palette_show();
	t3pix_palette_snapshot();
	t3pix_publish(T3PIX_EVENT_FADE_STEP_COMMIT, T3PIX_BOUNDARY_COMMIT);
}

void far pascal t3pix_palette_white_in(unsigned speed)
{
	PaletteTone = 200;
	vsync_wait();
	do {
		palette_show();
		t3pix_palette_snapshot();
		t3pix_publish(T3PIX_EVENT_FADE_STEP_COMMIT, T3PIX_BOUNDARY_COMMIT);
		t3pix_palette_fade_wait(speed);
		PaletteTone -= 6;
	} while(static_cast<int16_t>(PaletteTone) > 100);
	PaletteTone = 100;
	palette_show();
	t3pix_palette_snapshot();
	t3pix_publish(T3PIX_EVENT_FADE_STEP_COMMIT, T3PIX_BOUNDARY_COMMIT);
}

void far pascal t3pix_palette_white_out(unsigned speed)
{
	PaletteTone = 100;
	vsync_wait();
	do {
		palette_show();
		t3pix_palette_snapshot();
		t3pix_publish(T3PIX_EVENT_FADE_STEP_COMMIT, T3PIX_BOUNDARY_COMMIT);
		t3pix_palette_fade_wait(speed);
		PaletteTone += 6;
	} while(static_cast<int16_t>(PaletteTone) < 200);
	PaletteTone = 200;
	palette_show();
	t3pix_palette_snapshot();
	t3pix_publish(T3PIX_EVENT_FADE_STEP_COMMIT, T3PIX_BOUNDARY_COMMIT);
}
