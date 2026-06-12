#include "th03/formats/enedat.hpp"
#include <ctype.h>
#include <malloc.h>
#include <mem.h>
#include <process.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const unsigned int FORMATION_ENEMIES_MAX = 16;
static const unsigned int FORMATIONS_MAX = 24;

// Slice iteration
// ---------------

class FileSlice {
	uint8_t *start;
	uint8_t *p;
	size_t rem;

	bool check(size_t bytes_expected) const;
	void skip(size_t bytes_to_skip) {
		rem -= bytes_to_skip;
		p += bytes_to_skip;
	}

public:
	const size_t size;
	const char *fn;

	size_t tell(void) const {
		return (size - rem);
	}

	size_t remaining(void) const {
		return rem;
	}

	bool peek(void *out, size_t bytes_to_read);
	bool read(void *out, size_t bytes_to_read);

	FileSlice subslice(size_t bytes) {
		if(check(bytes)) {
			FileSlice ret(fn, nullptr, 0);
			return ret;
		}
		FileSlice ret(fn, start, (tell() + bytes));
		ret.skip(tell());
		skip(bytes);
		return ret;
	}

	FileSlice(const char *fn, uint8_t *buf, size_t size) :
		start(buf), p(buf), rem(size), size(size), fn(fn) {
	}
};

bool FileSlice::check(size_t bytes_expected) const
{
	if(rem < bytes_expected) {
		fprintf(
			stderr,
			"%s: Error reading %u bytes at position %u (buffer is only %u bytes large)\n",
			fn,
			bytes_expected,
			tell(),
			size
		);
		return true;
	}
	return false;
}

bool FileSlice::peek(void *out, size_t bytes_to_read)
{
	if(check(bytes_to_read)) {
		return true;
	}
	memcpy(out, p, bytes_to_read);
	return false;
}

bool FileSlice::read(void *out, size_t bytes_to_read)
{
	if(peek(out, bytes_to_read)) {
		return true;
	}
	skip(bytes_to_read);
	return false;
}
// ---------------

// Dumping
// -------

enum dump_value_type_t {
	T_BOOL,
	T_U8,
	T_ANGLE,
	T_ANGLE_SPEED,
	T_SUBPIXEL,
	T_POINT,
	T_SUBPIXEL_POINT,
	T_U8_ARRAY,
};

static const char SUBPIXEL_FRACT[SUBPIXEL_FACTOR][5] = {
	"0",
	"0625",
	"125",
	"1875",
	"25",
	"3125",
	"375",
	"4375",
	"5",
	"5625",
	"625",
	"6875",
	"75",
	"8125",
	"875",
	"9375",
};

uint8_t enedat_dump_param_len(va_list& ap, dump_value_type_t type)
{
	switch(type) {
	case T_BOOL:
		va_arg(ap, bool);
		return 5;
	case T_U8:
		va_arg(ap, uint8_t);
		return 3;
	case T_ANGLE:
		va_arg(ap, unsigned char);
		return 5;
	case T_ANGLE_SPEED:
		va_arg(ap, int8_t);
		return 2;
	case T_SUBPIXEL:
		va_arg(ap, Subpixel);
		return (2 + 4);
	case T_POINT:
	case T_SUBPIXEL_POINT:
		va_arg(ap, Subpixel);
		va_arg(ap, Subpixel);
		return (2 * (2 + 4));
	case T_U8_ARRAY: {
		const size_t count = va_arg(ap, size_t);
		va_arg(ap, const uint8_t *);
		return (1 + ((3 + 1) * count) + 1);
	}
	}
	return 0;
}

void enedat_dump_subpixel(int raw)
{
	int whole = (raw / SUBPIXEL_FACTOR);
	int rem = (raw % SUBPIXEL_FACTOR);
	if(rem < 0) {
		rem = -rem;
	}
	if((raw < 0) && (whole == 0)) {
		putchar('-');
	}
	printf("%d.%s", whole, SUBPIXEL_FRACT[rem]);
}

void enedat_dump_op(const char *name, ...)
{
	size_t len = (strlen(name) + 2);
	unsigned int param_count = 0;
	{
		va_list ap;
		va_start(ap, name);
		while(1) {
			const char *name = va_arg(ap, const char *);
			if(!name) {
				break;
			}
			len += (strlen(name) + 3);
			len += enedat_dump_param_len(ap, va_arg(ap, dump_value_type_t));
			param_count++;
		}
	}
	const bool wrap = (len >= 72);
	printf("\t\t%s(", name);

	va_list ap;
	va_start(ap, name);
	for(unsigned int i = 0; i < param_count; i++) {
		const char *name = va_arg(ap, const char *);
		if(i != 0) {
			printf(",");
		}
		if(wrap) {
			printf("\n\t\t\t");
		} else if(i != 0) {
			printf(" ");
		}
		printf("%s: ", name);

		switch(va_arg(ap, dump_value_type_t)) {
		case T_BOOL:
			printf("%s", (va_arg(ap, bool) ? "true" : "false"));
			break;

		case T_U8:
			printf("%u", va_arg(ap, uint8_t));
			break;

		case T_ANGLE: {
			unsigned char angle = va_arg(ap, unsigned char);
			char sign = ((angle >= 0x80) ? '-' : '+');
			uint8_t value = ((angle >= 0x80) ? (0x100 - angle) : angle);
			printf("%c0x%02X", sign, value);
			break;
		}

		case T_ANGLE_SPEED:
			printf("%+d", va_arg(ap, int8_t));
			break;

		case T_SUBPIXEL: {
			const Subpixel v = va_arg(ap, Subpixel);
			enedat_dump_subpixel(v);
			break;
		}

		case T_POINT: {
			const pixel_t x = va_arg(ap, Subpixel);
			const pixel_t y = va_arg(ap, Subpixel);
			printf("(x: %d, y: %d)", x, y);
			break;
		}

		case T_SUBPIXEL_POINT: {
			const Subpixel x = va_arg(ap, Subpixel);
			const Subpixel y = va_arg(ap, Subpixel);
			printf("(x: ");
			enedat_dump_subpixel(x);
			printf(", y: ");
			enedat_dump_subpixel(y);
			printf(")");
			break;
		}

		case T_U8_ARRAY: {
			const size_t count = va_arg(ap, size_t);
			const uint8_t *arr = va_arg(ap, uint8_t *);
			printf("[");
			for(size_t i = 0; i < count; i++) {
				if(i != 0) {
					printf(", ");
				}
				printf("%u", arr[i]);
			}
			printf("]");
			break;
		}
		}
	}
	printf("%s);\n", (wrap ? "\n\t\t" : ""));
}

bool enedat_dump_op_with_no_parameters(FileSlice& buf, const char *name)
{
	enedat_op_t p;
	if(buf.read(&p, sizeof(p))) {
		return true;
	}
	enedat_dump_op(name, nullptr, 0);
	return false;
}

bool enedat_dump_op_duration(FileSlice& buf, const char *name)
{
	enedat_op_duration_t p;
	if(buf.read(&p, sizeof(p))) {
		return true;
	}
	enedat_dump_op(name, "duration", T_U8, p.duration, nullptr);
	return false;
}

int enedat_dump_script(FileSlice& buf)
{
	while(buf.remaining() > 0) {
		enedat_op_code_t code;
		if(buf.peek(&code, sizeof(code))) {
			return 13;
		}
		switch(code) {
		case EO_STOP:
			if(enedat_dump_op_with_no_parameters(buf, "stop")) {
				return true;
			}
			break;

		case EO_MOVE_LINEAR:
		case EO_MOVE_LINEAR_STOP_AT_PLAYER_Y:
		case EO_MOVE_LINEAR_STOP_AT_PLAYER_X: {
			enedat_op_linear_t p;
			if(buf.read(&p, sizeof(p))) {
				return (14 + code);
			}
			const char *name;
			if(code == EO_MOVE_LINEAR_STOP_AT_PLAYER_X) {
				name = "move_linear_stop_at_player_x";
			} else if(code == EO_MOVE_LINEAR_STOP_AT_PLAYER_Y) {
				name = "move_linear_stop_at_player_y";
			} else {
				name = "move_linear";
			}
			enedat_dump_op(
				name,
				"angle", T_ANGLE, p.angle,
				"speed", T_SUBPIXEL, p.speed.v,
				"duration", T_U8, p.duration,
				nullptr
			);
			break;
		}

		case EO_MOVE_CIRCULAR: {
			enedat_op_circular_t p;
			if(buf.read(&p, sizeof(p))) {
				return (14 + code);
			}
			enedat_dump_op(
				"move_circular",
				"angle_start", T_ANGLE, p.angle_start,
				"speed", T_SUBPIXEL, p.speed.v,
				"angle_speed", T_ANGLE_SPEED, p.angle_speed,
				"duration", T_U8, p.duration,
				nullptr
			);
			break;
		}

		case EO_WAIT: {
			if(enedat_dump_op_duration(buf, "wait")) {
				return (14 + code);
			}
			break;
		}

		case EO_MOVE_SINE_X:
		case EO_MOVE_SINE_Y: {
			enedat_op_sine_t p;
			struct sine_op_t {
				const char *op;
				const char *speed;
				const char *velocity;
			};
			static const sine_op_t OP_LABELS[2] = {
				{ "move_sine_x", "speed_x", "velocity_y" },
				{ "move_sine_y", "speed_y", "velocity_x" },
			};
			if(buf.read(&p, sizeof(p))) {
				return (14 + code);
			}
			const sine_op_t& labels = OP_LABELS[code == EO_MOVE_SINE_Y];
			enedat_dump_op(
				labels.op,
				labels.speed, T_SUBPIXEL, p.speed_on_sine_axis.v,
				"angle_speed", T_ANGLE_SPEED, p.angle_speed,
				labels.velocity, T_SUBPIXEL, p.velocity_on_linear_axis.v,
				"duration", T_U8, p.duration,
				nullptr
			);
			break;
		}

		case EO_MOVE: {
			if(enedat_dump_op_duration(buf, "move")) {
				return (14 + code);
			}
			break;
		}

		case EO_MOVE_WITH_SPEED: {
			enedat_op_speed_t p;
			if(buf.read(&p, sizeof(p))) {
				return (14 + code);
			}
			enedat_dump_op(
				"move_with_speed",
				"speed", T_SUBPIXEL, p.speed.v,
				"duration", T_U8, p.duration,
				nullptr
			);
			break;
		}

		case EO_MOVE_CIRCULAR_PLUS: {
			enedat_op_circular_plus_t p;
			if(buf.read(&p, sizeof(p))) {
				return (14 + code);
			}
			enedat_dump_op(
				"move_circular_plus",
				"angle_start", T_ANGLE, p.angle_start,
				"speed", T_SUBPIXEL, p.speed.v,
				"angle_speed", T_ANGLE_SPEED, p.angle_speed,
				"velocity_plus",
					T_SUBPIXEL_POINT,
					p.velocity_x_plus.v,
					p.velocity_y_plus.v,
				"duration", T_U8, p.duration,
				nullptr
			);
			break;
		}

		case EO_SPAWN: {
			enum {
				SIZE_ARRAY_COUNT = (
					sizeof(((enedat_op_spawn_t *)(nullptr))->size_words) /
					sizeof(((enedat_op_spawn_t *)(nullptr))->size_words[0])
				),
				HP_ARRAY_COUNT = (
					sizeof(((enedat_op_spawn_t *)(nullptr))->hp) /
					sizeof(((enedat_op_spawn_t *)(nullptr))->hp[0])
				)
			};
			enedat_op_spawn_t p;
			if(buf.read(&p, sizeof(p))) {
				return (14 + code);
			}
			const pixel_t center_x = (p.center_x_divided_by_8 * 8);
			const pixel_t center_y = (p.center_y_divided_by_8 * 8);
			{for(int i = 0; i < SIZE_ARRAY_COUNT; i++) {
				p.size_words[i] *= 16;
			}}
			enedat_dump_op(
				"spawn",
				"center", T_POINT, center_x, center_y,
				"size", T_U8_ARRAY, SIZE_ARRAY_COUNT, p.size_words,
				"hp", T_U8_ARRAY, HP_ARRAY_COUNT, p.hp,
				"clip_x", T_BOOL, p.clip_x,
				"clip_bottom", T_BOOL, p.clip_bottom,
				"unused", T_U8, p.unused,
				nullptr
			);
			break;
		}

		case EO_LOOP_ABS:
		case EO_LOOP_REL: {
			enedat_op_loop_t p;
			if(buf.read(&p, sizeof(p))) {
				return (14 + code);
			}
			enedat_dump_op(
				(code == EO_LOOP_ABS) ? "loop_abs" : "loop_rel",
				(code == EO_LOOP_ABS) ? "target" : "disp", T_ANGLE_SPEED, p.ip_disp,
				"count", T_ANGLE_SPEED, p.count,
				nullptr
			);
			break;
		}

		case EO_CLIP_X:
		case EO_CLIP_BOTTOM:
			if(enedat_dump_op_with_no_parameters(
				buf, (code == EO_CLIP_X) ? "clip_x" : "clip_bottom"
			)) {
				return (14 + code);
			}
			break;

		default:
			fprintf(
				stderr,
				"%s: Unknown opcode %u at position %u\n",
				buf.fn,
				code,
				buf.tell()
			);
			return (14 + code);
		}
	}
	return 0;
}

int enedat_dump_payload(FileSlice& buf)
{
	unsigned int formation_i = 0;
	while(buf.remaining() > 0) {
		uint16_t enemy_count;
		if(buf.read(&enemy_count, sizeof(enemy_count))) {
			return 11;
		}
		if(enemy_count == 0) {
			break;
		}
		if(formation_i != 0) {
			printf("\n");
		}
		printf("Formation %u\n", formation_i);
		for(uint16_t enemy_i = 0; enemy_i < enemy_count; enemy_i++) {
			uint8_t script_size;
			if(buf.read(&script_size, sizeof(script_size))) {
				return 12;
			}

			printf("\tEnemy %u\n", enemy_i);
			int script_ret = enedat_dump_script(buf.subslice(script_size));
			if(script_ret) {
				return script_ret;
			}
		}
		formation_i++;
	}
	return 0;
}

int enedat_dump(FileSlice& buf)
{
	enedat_header_t header;
	if(buf.read(&header, sizeof(header))) {
		return 9;
	}
	if(header.zero != 0) {
		fprintf(stderr, "%s: Header zero field is not zero\n", buf.fn);
		return 10;
	}
	if(header.size > buf.remaining()) {
		fprintf(
			stderr,
			"%s: Size from header (%u bytes) larger than remaining file size (%u bytes)\n",
			buf.fn,
			header.size,
			buf.remaining()
		);
		return 10;
	}

	FileSlice payload = buf.subslice(header.size);
	if(buf.remaining() != 0) {
		fprintf(stderr, "%s: Trailing bytes after declared payload\n", buf.fn);
		return 10;
	}
	return enedat_dump_payload(payload);
}

int file_size_error(const char *fn, int ret)
{
	fprintf(stderr, "%s: Error determining file size", fn);
	perror("");
	return ret;
}

int enedat_dump(const char *fn, FILE *fp)
{
	long origin = ftell(fp);
	if(origin == -1) {
		return file_size_error(fn, 3);
	}
	if(fseek(fp, 0, SEEK_END)) {
		return file_size_error(fn, 4);
	}
	long size = ftell(fp);
	if(size == -1) {
		return file_size_error(fn, 5);
	}
	if(fseek(fp, origin, SEEK_SET)) {
		return file_size_error(fn, 6);
	}

	uint8_t *buf = static_cast<uint8_t *>(malloc(size));
	if(!buf) {
		fprintf(stderr, "%s: Error allocating %ld bytes\n", fn, size);
		return 7;
	}

	size_t ret = fread(buf, 1, size, fp);
	if(ret != size) {
		fprintf(
			stderr,
			"%s: Error reading %ld bytes? (only read %u bytes)\n",
			fn,
			size,
			ret
		);
		free(buf);
		return 8;
	}

	FileSlice buffer(fn, buf, size);
	ret = enedat_dump(buffer);
	free(buf);
	return ret;
}

int enedat_dump(const char *fn)
{
	FILE *fp = fopen(fn, "rb");
	if(!fp) {
		fprintf(stderr, "%s: ", fn);
		perror("");
		return 2;
	}
	int ret = enedat_dump(fn, fp);
	fclose(fp);
	return ret;
}
// -------

// Compiling
// ---------

static char compile_error[256];

static bool fail(const char *message)
{
	strcpy(compile_error, message);
	return true;
}

static bool fail_name(const char *prefix, const char *name)
{
	strcpy(compile_error, prefix);
	strcat(compile_error, name);
	return true;
}

static bool starts_with(const char *s, const char *prefix)
{
	return (strncmp(s, prefix, strlen(prefix)) == 0);
}

static void trim(char *s)
{
	char *start = s;
	while(*start && isspace(static_cast<unsigned char>(*start))) {
		start++;
	}
	if(start != s) {
		memmove(s, start, strlen(start) + 1);
	}

	char *end = (s + strlen(s));
	while((end > s) && isspace(static_cast<unsigned char>(end[-1]))) {
		end--;
	}
	*end = '\0';
}

static bool compact(char *out, size_t out_size, const char *in)
{
	size_t o = 0;
	for(size_t i = 0; in[i] != '\0'; i++) {
		if(!isspace(static_cast<unsigned char>(in[i]))) {
			if((o + 1) >= out_size) {
				return fail("operation is too long");
			}
			out[o++] = in[i];
		}
	}
	out[o] = '\0';
	return false;
}

static int find_top_level(const char *s, char needle, size_t start)
{
	int paren = 0;
	int bracket = 0;
	for(size_t i = start; s[i] != '\0'; i++) {
		if(s[i] == '(') {
			paren++;
		} else if(s[i] == ')') {
			paren--;
		} else if(s[i] == '[') {
			bracket++;
		} else if(s[i] == ']') {
			bracket--;
		} else if((s[i] == needle) && (paren == 0) && (bracket == 0)) {
			return i;
		}
	}
	return -1;
}

static bool copy_range(char *out, size_t out_size, const char *start, size_t len)
{
	if(len >= out_size) {
		return fail("parameter is too long");
	}
	memcpy(out, start, len);
	out[len] = '\0';
	return false;
}

static bool param_value(
	char *out, size_t out_size, const char *params, const char *name
)
{
	char key[64];
	strcpy(key, name);
	strcat(key, ":");
	const size_t key_len = strlen(key);

	for(const char *p = params; *p != '\0'; p++) {
		if(((p == params) || (p[-1] == ',')) && (strncmp(p, key, key_len) == 0)) {
			const char *value = (p + key_len);
			int end = find_top_level(params, ',', (value - params));
			if(end < 0) {
				end = strlen(params);
			}
			return copy_range(out, out_size, value, (params + end) - value);
		}
	}
	return fail_name("missing parameter: ", name);
}

static bool parse_int(const char *s, long& out)
{
	if(s[0] == '\0') {
		return fail("empty integer");
	}

	size_t p = 0;
	long sign = 1;
	if(s[p] == '+') {
		p++;
	} else if(s[p] == '-') {
		sign = -1;
		p++;
	}

	int base = 10;
	if((s[p] == '0') && ((s[p + 1] == 'x') || (s[p + 1] == 'X'))) {
		base = 16;
		p += 2;
	}

	long ret = 0;
	bool any_digit = false;
	for(; s[p] != '\0'; p++) {
		int digit;
		if((s[p] >= '0') && (s[p] <= '9')) {
			digit = (s[p] - '0');
		} else if((s[p] >= 'a') && (s[p] <= 'f')) {
			digit = (s[p] - 'a' + 10);
		} else if((s[p] >= 'A') && (s[p] <= 'F')) {
			digit = (s[p] - 'A' + 10);
		} else {
			return fail_name("invalid integer: ", s);
		}
		if(digit >= base) {
			return fail_name("invalid integer: ", s);
		}
		ret = ((ret * base) + digit);
		any_digit = true;
	}
	if(!any_digit) {
		return fail_name("invalid integer: ", s);
	}
	out = (ret * sign);
	return false;
}

static bool checked_u8(long v, uint8_t& out)
{
	if((v < 0) || (v > 255)) {
		return fail("unsigned byte out of range");
	}
	out = static_cast<uint8_t>(v);
	return false;
}

static bool checked_i8(long v, int8_t& out)
{
	if((v < -128) || (v > 127)) {
		return fail("signed byte out of range");
	}
	out = static_cast<int8_t>(v);
	return false;
}

static bool parse_u8(const char *s, uint8_t& out)
{
	long v;
	return (parse_int(s, v) || checked_u8(v, out));
}

static bool parse_i8(const char *s, int8_t& out)
{
	long v;
	return (parse_int(s, v) || checked_i8(v, out));
}

static bool parse_angle(const char *s, uint8_t& out)
{
	long v;
	if(parse_int(s, v)) {
		return true;
	}
	if((v < -128) || (v > 255)) {
		return fail_name("angle out of range: ", s);
	}
	out = static_cast<uint8_t>(v);
	return false;
}

static bool parse_fraction(const char *s, int& out)
{
	char normalized[16];
	if(strlen(s) >= sizeof(normalized)) {
		return fail_name("subpixel fraction is too long: .", s);
	}
	strcpy(normalized, s);
	while((strlen(normalized) > 1) && (normalized[strlen(normalized) - 1] == '0')) {
		normalized[strlen(normalized) - 1] = '\0';
	}
	for(int i = 0; i < SUBPIXEL_FACTOR; i++) {
		if(strcmp(normalized, SUBPIXEL_FRACT[i]) == 0) {
			out = i;
			return false;
		}
	}
	return fail_name("subpixel fraction is not a sixteenth: .", s);
}

static bool parse_subpixel_raw(const char *s, long& out)
{
	const char *dot = strchr(s, '.');
	if(!dot) {
		long whole;
		if(parse_int(s, whole)) {
			return true;
		}
		out = (whole * SUBPIXEL_FACTOR);
		return false;
	}

	char whole_s[32];
	if(copy_range(whole_s, sizeof(whole_s), s, (dot - s))) {
		return true;
	}

	long whole;
	int frac;
	if(parse_int(whole_s, whole) || parse_fraction(dot + 1, frac)) {
		return true;
	}

	if((s[0] == '-') && (whole == 0)) {
		out = -frac;
	} else {
		out = ((whole * SUBPIXEL_FACTOR) + ((whole < 0) ? -frac : frac));
	}
	return false;
}

static bool parse_subpixel_u8(const char *s, uint8_t& out)
{
	long v;
	return (parse_subpixel_raw(s, v) || checked_u8(v, out));
}

static bool parse_subpixel_i8(const char *s, int8_t& out)
{
	long v;
	return (parse_subpixel_raw(s, v) || checked_i8(v, out));
}

static bool parse_point(const char *s, int& x, int& y)
{
	const size_t len = strlen(s);
	if((len < 5) || (s[0] != '(') || (s[len - 1] != ')')) {
		return fail_name("invalid point: ", s);
	}

	char body[128];
	char value[32];
	long parsed;
	if(copy_range(body, sizeof(body), s + 1, len - 2)) {
		return true;
	}
	if(param_value(value, sizeof(value), body, "x") || parse_int(value, parsed)) {
		return true;
	}
	x = static_cast<int>(parsed);
	if(param_value(value, sizeof(value), body, "y") || parse_int(value, parsed)) {
		return true;
	}
	y = static_cast<int>(parsed);
	return false;
}

static bool parse_subpixel_point(const char *s, int8_t& x, int8_t& y)
{
	const size_t len = strlen(s);
	if((len < 5) || (s[0] != '(') || (s[len - 1] != ')')) {
		return fail_name("invalid point: ", s);
	}

	char body[128];
	char value[32];
	if(copy_range(body, sizeof(body), s + 1, len - 2)) {
		return true;
	}
	if(
		param_value(value, sizeof(value), body, "x") ||
		parse_subpixel_i8(value, x)
	) {
		return true;
	}
	if(
		param_value(value, sizeof(value), body, "y") ||
		parse_subpixel_i8(value, y)
	) {
		return true;
	}
	return false;
}

static bool parse_array(
	const char *s, int *values, unsigned int max_count, unsigned int& count
)
{
	const size_t len = strlen(s);
	if((len < 2) || (s[0] != '[') || (s[len - 1] != ']')) {
		return fail_name("invalid array: ", s);
	}

	count = 0;
	const char *p = (s + 1);
	const char *end = (s + len - 1);
	while(p < end) {
		if(count >= max_count) {
			return fail("too many array values");
		}

		const char *comma = strchr(p, ',');
		if(!comma || (comma > end)) {
			comma = end;
		}

		char item[32];
		long parsed;
		if(
			copy_range(item, sizeof(item), p, (comma - p)) ||
			parse_int(item, parsed)
		) {
			return true;
		}
		values[count++] = static_cast<int>(parsed);
		p = (comma + 1);
	}
	return false;
}

static bool parse_bool(const char *s, bool& out)
{
	if(strcmp(s, "true") == 0) {
		out = true;
		return false;
	}
	if(strcmp(s, "false") == 0) {
		out = false;
		return false;
	}
	return fail_name("invalid bool: ", s);
}

struct OpBuffer {
	uint8_t bytes[32];
	size_t size;
};

static bool op_u8(OpBuffer& op, uint8_t v)
{
	if(op.size >= sizeof(op.bytes)) {
		return fail("operation became too large");
	}
	op.bytes[op.size++] = v;
	return false;
}

static bool op_i8(OpBuffer& op, int8_t v)
{
	return op_u8(op, static_cast<uint8_t>(v));
}

static bool op_write(FILE *fp, const OpBuffer& op)
{
	if(fwrite(op.bytes, 1, op.size, fp) != op.size) {
		return fail("error writing operation");
	}
	return false;
}

static bool compile_op(FILE *out, const char *op_text)
{
	char s[1024];
	if(compact(s, sizeof(s), op_text)) {
		return true;
	}

	const size_t len = strlen(s);
	if((len < 3) || (strcmp(s + len - 2, ");") != 0)) {
		return fail("operation does not end with );");
	}

	char *open = strchr(s, '(');
	if(!open) {
		return fail("operation is missing (");
	}
	s[len - 2] = '\0';
	*open = '\0';
	const char *name = s;
	const char *params = (open + 1);

	OpBuffer op;
	op.size = 0;
	char value[160];

	if(strcmp(name, "stop") == 0) {
		if(op_u8(op, EO_STOP)) {
			return true;
		}
	} else if(
		(strcmp(name, "move_linear") == 0) ||
		(strcmp(name, "move_linear_stop_at_player_y") == 0) ||
		(strcmp(name, "move_linear_stop_at_player_x") == 0)
	) {
		uint8_t angle;
		uint8_t speed;
		uint8_t duration;
		if(strcmp(name, "move_linear") == 0) {
			op_u8(op, EO_MOVE_LINEAR);
		} else if(strcmp(name, "move_linear_stop_at_player_y") == 0) {
			op_u8(op, EO_MOVE_LINEAR_STOP_AT_PLAYER_Y);
		} else {
			op_u8(op, EO_MOVE_LINEAR_STOP_AT_PLAYER_X);
		}
		if(
			param_value(value, sizeof(value), params, "angle") ||
			parse_angle(value, angle) ||
			param_value(value, sizeof(value), params, "speed") ||
			parse_subpixel_u8(value, speed) ||
			param_value(value, sizeof(value), params, "duration") ||
			parse_u8(value, duration) ||
			op_u8(op, angle) ||
			op_u8(op, speed) ||
			op_u8(op, duration)
		) {
			return true;
		}
	} else if(strcmp(name, "move_circular") == 0) {
		uint8_t angle_start;
		uint8_t speed;
		int8_t angle_speed;
		uint8_t duration;
		if(
			op_u8(op, EO_MOVE_CIRCULAR) ||
			param_value(value, sizeof(value), params, "angle_start") ||
			parse_angle(value, angle_start) ||
			param_value(value, sizeof(value), params, "speed") ||
			parse_subpixel_u8(value, speed) ||
			param_value(value, sizeof(value), params, "angle_speed") ||
			parse_i8(value, angle_speed) ||
			param_value(value, sizeof(value), params, "duration") ||
			parse_u8(value, duration) ||
			op_u8(op, angle_start) ||
			op_u8(op, speed) ||
			op_i8(op, angle_speed) ||
			op_u8(op, duration)
		) {
			return true;
		}
	} else if((strcmp(name, "wait") == 0) || (strcmp(name, "move") == 0)) {
		uint8_t duration;
		if(
			op_u8(op, (strcmp(name, "wait") == 0) ? EO_WAIT : EO_MOVE) ||
			param_value(value, sizeof(value), params, "duration") ||
			parse_u8(value, duration) ||
			op_u8(op, duration)
		) {
			return true;
		}
	} else if((strcmp(name, "move_sine_x") == 0) || (strcmp(name, "move_sine_y") == 0)) {
		const bool x = (strcmp(name, "move_sine_x") == 0);
		uint8_t speed;
		int8_t angle_speed;
		int8_t velocity;
		uint8_t duration;
		if(
			op_u8(op, x ? EO_MOVE_SINE_X : EO_MOVE_SINE_Y) ||
			param_value(value, sizeof(value), params, x ? "speed_x" : "speed_y") ||
			parse_subpixel_u8(value, speed) ||
			param_value(value, sizeof(value), params, "angle_speed") ||
			parse_i8(value, angle_speed) ||
			param_value(value, sizeof(value), params, x ? "velocity_y" : "velocity_x") ||
			parse_subpixel_i8(value, velocity) ||
			param_value(value, sizeof(value), params, "duration") ||
			parse_u8(value, duration) ||
			op_u8(op, speed) ||
			op_i8(op, angle_speed) ||
			op_i8(op, velocity) ||
			op_u8(op, duration)
		) {
			return true;
		}
	} else if(strcmp(name, "move_with_speed") == 0) {
		uint8_t speed;
		uint8_t duration;
		if(
			op_u8(op, EO_MOVE_WITH_SPEED) ||
			param_value(value, sizeof(value), params, "speed") ||
			parse_subpixel_u8(value, speed) ||
			param_value(value, sizeof(value), params, "duration") ||
			parse_u8(value, duration) ||
			op_u8(op, speed) ||
			op_u8(op, duration)
		) {
			return true;
		}
	} else if(strcmp(name, "move_circular_plus") == 0) {
		uint8_t angle_start;
		uint8_t speed;
		int8_t angle_speed;
		int8_t velocity_x;
		int8_t velocity_y;
		uint8_t duration;
		if(
			op_u8(op, EO_MOVE_CIRCULAR_PLUS) ||
			param_value(value, sizeof(value), params, "angle_start") ||
			parse_angle(value, angle_start) ||
			param_value(value, sizeof(value), params, "speed") ||
			parse_subpixel_u8(value, speed) ||
			param_value(value, sizeof(value), params, "angle_speed") ||
			parse_i8(value, angle_speed) ||
			param_value(value, sizeof(value), params, "velocity_plus") ||
			parse_subpixel_point(value, velocity_x, velocity_y) ||
			param_value(value, sizeof(value), params, "duration") ||
			parse_u8(value, duration) ||
			op_u8(op, angle_start) ||
			op_u8(op, speed) ||
			op_i8(op, angle_speed) ||
			op_i8(op, velocity_x) ||
			op_i8(op, velocity_y) ||
			op_u8(op, duration)
		) {
			return true;
		}
	} else if(strcmp(name, "spawn") == 0) {
		int center_x;
		int center_y;
		int size[ENEMY_SPEED_COUNT];
		int hp[ENEMY_SPEED_COUNT];
		unsigned int size_count;
		unsigned int hp_count;
		bool clip_x;
		bool clip_bottom;
		int8_t unused;
		if(
			op_u8(op, EO_SPAWN) ||
			param_value(value, sizeof(value), params, "center") ||
			parse_point(value, center_x, center_y) ||
			param_value(value, sizeof(value), params, "size") ||
			parse_array(value, size, ENEMY_SPEED_COUNT, size_count) ||
			param_value(value, sizeof(value), params, "hp") ||
			parse_array(value, hp, ENEMY_SPEED_COUNT, hp_count) ||
			param_value(value, sizeof(value), params, "clip_x") ||
			parse_bool(value, clip_x) ||
			param_value(value, sizeof(value), params, "clip_bottom") ||
			parse_bool(value, clip_bottom) ||
			param_value(value, sizeof(value), params, "unused") ||
			parse_i8(value, unused)
		) {
			return true;
		}
		if((center_x % 8) || (center_y % 8)) {
			return fail("spawn center must be divisible by 8");
		}
		if((size_count != ENEMY_SPEED_COUNT) || (hp_count != ENEMY_SPEED_COUNT)) {
			return fail("spawn size and hp arrays must have 4 values");
		}

		int8_t center_divided;
		if(checked_i8(center_x / 8, center_divided) || op_i8(op, center_divided)) {
			return true;
		}
		if(checked_i8(center_y / 8, center_divided) || op_i8(op, center_divided)) {
			return true;
		}
		{for(unsigned int i = 0; i < ENEMY_SPEED_COUNT; i++) {
			uint8_t out;
			if(size[i] % 16) {
				return fail("spawn sizes must be divisible by 16");
			}
			if(checked_u8(size[i] / 16, out) || op_u8(op, out)) {
				return true;
			}
		}}
		{for(unsigned int i = 0; i < ENEMY_SPEED_COUNT; i++) {
			uint8_t out;
			if(checked_u8(hp[i], out) || op_u8(op, out)) {
				return true;
			}
		}}
		if(
			op_u8(op, clip_x) ||
			op_u8(op, clip_bottom) ||
			op_i8(op, unused)
		) {
			return true;
		}
	} else if((strcmp(name, "loop_abs") == 0) || (strcmp(name, "loop_rel") == 0)) {
		const bool abs = (strcmp(name, "loop_abs") == 0);
		int8_t target;
		int8_t count;
		if(
			op_u8(op, abs ? EO_LOOP_ABS : EO_LOOP_REL) ||
			param_value(value, sizeof(value), params, abs ? "target" : "disp") ||
			parse_i8(value, target) ||
			param_value(value, sizeof(value), params, "count") ||
			parse_i8(value, count) ||
			op_i8(op, target) ||
			op_i8(op, count)
		) {
			return true;
		}
	} else if((strcmp(name, "clip_x") == 0) || (strcmp(name, "clip_bottom") == 0)) {
		if(op_u8(op, (strcmp(name, "clip_x") == 0) ? EO_CLIP_X : EO_CLIP_BOTTOM)) {
			return true;
		}
	} else {
		return fail_name("unknown operation: ", name);
	}

	return op_write(out, op);
}

static bool write_u8(FILE *fp, uint8_t v)
{
	return (fwrite(&v, sizeof(v), 1, fp) != 1);
}

static bool write_u16(FILE *fp, uint16_t v)
{
	return (fwrite(&v, sizeof(v), 1, fp) != 1);
}

static bool patch_u8(FILE *fp, long pos, uint8_t v)
{
	const long cur = ftell(fp);
	if((cur < 0) || fseek(fp, pos, SEEK_SET) || write_u8(fp, v) || fseek(fp, cur, SEEK_SET)) {
		return fail("error patching output file");
	}
	return false;
}

static bool patch_u16(FILE *fp, long pos, uint16_t v)
{
	const long cur = ftell(fp);
	if((cur < 0) || fseek(fp, pos, SEEK_SET) || write_u16(fp, v) || fseek(fp, cur, SEEK_SET)) {
		return fail("error patching output file");
	}
	return false;
}

struct CompileState {
	FILE *out;
	long formation_count_pos;
	long enemy_size_pos;
	long enemy_script_start;
	unsigned int formation_count;
	unsigned int enemy_count;
	bool formation_open;
	bool enemy_open;
};

static bool finish_enemy(CompileState& s)
{
	if(!s.enemy_open) {
		return false;
	}

	const long pos = ftell(s.out);
	if(pos < 0) {
		return fail("error determining output position");
	}

	const long script_size = (pos - s.enemy_script_start);
	if(script_size > 255) {
		return fail("script exceeds 255 bytes");
	}
	if(patch_u8(s.out, s.enemy_size_pos, static_cast<uint8_t>(script_size))) {
		return true;
	}
	s.enemy_open = false;
	return false;
}

static bool finish_formation(CompileState& s)
{
	if(!s.formation_open) {
		return false;
	}
	if(finish_enemy(s)) {
		return true;
	}
	if(s.enemy_count == 0) {
		return fail("formation has no enemies");
	}
	if(patch_u16(s.out, s.formation_count_pos, s.enemy_count)) {
		return true;
	}
	s.formation_open = false;
	return false;
}

static bool start_formation(CompileState& s, unsigned int seen)
{
	if(finish_formation(s)) {
		return true;
	}
	if(seen != s.formation_count) {
		return fail("formation IDs must be contiguous");
	}
	if(s.formation_count >= FORMATIONS_MAX) {
		return fail("too many formations");
	}

	s.formation_count_pos = ftell(s.out);
	if(s.formation_count_pos < 0) {
		return fail("error determining output position");
	}
	if(write_u16(s.out, 0)) {
		return fail("error writing output file");
	}
	s.formation_open = true;
	s.enemy_open = false;
	s.enemy_count = 0;
	s.formation_count++;
	return false;
}

static bool start_enemy(CompileState& s, unsigned int seen)
{
	if(!s.formation_open) {
		return fail("enemy outside formation");
	}
	if(finish_enemy(s)) {
		return true;
	}
	if(seen != s.enemy_count) {
		return fail("enemy IDs must be contiguous");
	}
	if(s.enemy_count >= FORMATION_ENEMIES_MAX) {
		return fail("too many enemies in formation");
	}

	s.enemy_size_pos = ftell(s.out);
	if(s.enemy_size_pos < 0) {
		return fail("error determining output position");
	}
	if(write_u8(s.out, 0)) {
		return fail("error writing output file");
	}
	s.enemy_script_start = ftell(s.out);
	if(s.enemy_script_start < 0) {
		return fail("error determining output position");
	}
	s.enemy_open = true;
	s.enemy_count++;
	return false;
}

static bool check_current_script_size(CompileState& s)
{
	if(!s.enemy_open) {
		return fail("operation outside enemy");
	}

	const long pos = ftell(s.out);
	if(pos < 0) {
		return fail("error determining output position");
	}
	if((pos - s.enemy_script_start) > 255) {
		return fail("script exceeds 255 bytes");
	}
	return false;
}

static bool parse_heading_index(const char *line, const char *heading, unsigned int& out)
{
	long parsed;
	if(!starts_with(line, heading)) {
		return fail_name("expected heading: ", heading);
	}
	if(parse_int(line + strlen(heading), parsed)) {
		return true;
	}
	if(parsed < 0) {
		return fail("heading ID must be nonnegative");
	}
	out = static_cast<unsigned int>(parsed);
	return false;
}

static bool append_op_line(char *op, size_t op_size, const char *line)
{
	if((strlen(op) + strlen(line) + 1) >= op_size) {
		return fail("operation is too long");
	}
	strcat(op, line);
	return false;
}

int enedat_compile(const char *in_fn, const char *out_fn)
{
	FILE *in = fopen(in_fn, "rt");
	if(!in) {
		fprintf(stderr, "%s: ", in_fn);
		perror("");
		return 2;
	}

	FILE *out = fopen(out_fn, "wb");
	if(!out) {
		fprintf(stderr, "%s: ", out_fn);
		perror("");
		fclose(in);
		return 3;
	}

	CompileState state;
	memset(&state, 0, sizeof(state));
	state.out = out;

	bool failed = false;
	unsigned int line_no = 0;
	char line[1024];
	char op[2048];
	op[0] = '\0';

	if(write_u16(out, 0) || write_u16(out, 0)) {
		fail("error writing output file");
		failed = true;
	}

	while(!failed && fgets(line, sizeof(line), in)) {
		line_no++;
		line[strcspn(line, "\r\n")] = '\0';
		trim(line);
		if(line[0] == '\0') {
			continue;
		}

		if(op[0] != '\0') {
			failed = append_op_line(op, sizeof(op), line);
			if(!failed && strstr(line, ");")) {
				failed = (compile_op(out, op) || check_current_script_size(state));
				op[0] = '\0';
			}
			continue;
		}

		if(starts_with(line, "#") || starts_with(line, "//")) {
			continue;
		}

		if(starts_with(line, "Formation ")) {
			unsigned int seen;
			failed = (parse_heading_index(line, "Formation ", seen) || start_formation(state, seen));
		} else if(starts_with(line, "Enemy ")) {
			unsigned int seen;
			failed = (parse_heading_index(line, "Enemy ", seen) || start_enemy(state, seen));
		} else {
			if(!state.enemy_open) {
				failed = fail("operation outside enemy");
			} else {
				strcpy(op, line);
				if(strstr(line, ");")) {
					failed = (compile_op(out, op) || check_current_script_size(state));
					op[0] = '\0';
				}
			}
		}
	}

	if(!failed && ferror(in)) {
		failed = fail("error reading input file");
	}
	if(!failed && (op[0] != '\0')) {
		failed = fail("unterminated operation");
	}
	if(!failed) {
		failed = finish_formation(state);
	}
	if(!failed && write_u16(out, 0)) {
		failed = fail("error writing output file");
	}
	if(!failed) {
		const long end = ftell(out);
		const long payload_size = (end - sizeof(enedat_header_t));
		if((end < 0) || (payload_size < 0) || (payload_size > 0xFFFF)) {
			failed = fail("compiled ENEDAT payload exceeds 64 KiB");
		} else {
			failed = patch_u16(out, 0, static_cast<uint16_t>(payload_size));
		}
	}

	fclose(in);
	fclose(out);

	if(failed) {
		fprintf(stderr, "%s:%u: %s\n", in_fn, line_no, compile_error);
		remove(out_fn);
		return 4;
	}
	return 0;
}
// ---------

static void usage(const char *argv0)
{
	fprintf(
		stderr,
		"Usage:\n"
		"  %s ENEDAT.DAT\n"
		"  %s dump ENEDAT.DAT\n"
		"  %s compile ENEDAT.txt ENEDAT.DAT\n",
		argv0,
		argv0,
		argv0
	);
}

int __cdecl main(int argc, const char **argv)
{
	if(argc == 2) {
		return enedat_dump(argv[1]);
	}
	if((argc == 3) && (strcmp(argv[1], "dump") == 0)) {
		return enedat_dump(argv[2]);
	}
	if((argc == 4) && (strcmp(argv[1], "compile") == 0)) {
		return enedat_compile(argv[2], argv[3]);
	}

	usage(argv[0]);
	return 1;
}
