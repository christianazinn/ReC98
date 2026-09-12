#pragma option -zCT1PFSTREAM_TEXT -G-

#include <ctype.h>
#include <stdlib.h>
#include "libs/master.lib/master.hpp"
#include "th01/formats/pf.hpp"
#include "th01/formats/pf_state.hpp"

extern pf_header_t *arc_pfs;
extern pf_header_t *file_pf;
extern int arc_pf_count;
extern bool file_compressed;
extern uint8_t *file_data;
extern char arc_fn[PF_FN_LEN];
extern size_t file_pos;

// BOS callers only read forwards, so decode directly into the sprite planes
// instead of keeping a second, whole-file copy alive during their allocation.
#pragma codeseg T1PFSTREAM_TEXT

struct bos_file_stream_t {
	uint8_t cache[256];
	unsigned cache_pos;
	unsigned cache_size;
	long packed_left;
	uint8_t next;
	uint8_t runs;
};

static bos_file_stream_t *near bos_file_stream(void)
{
	return reinterpret_cast<bos_file_stream_t *>(file_data);
}

static uint8_t near bos_file_raw(void)
{
	bos_file_stream_t *s = bos_file_stream();
	if(s->cache_pos == s->cache_size) {
		unsigned count = sizeof(s->cache);
		if(s->packed_left < count) {
			count = static_cast<unsigned>(s->packed_left);
		}
		if(!count || (file_read(s->cache, count) != count)) {
			exit(1);
		}
		s->packed_left -= count;
		s->cache_size = count;
		s->cache_pos = 0;
	}
	return (s->cache[s->cache_pos++] ^ arc_key);
}

void pascal bos_file_load(const char fn[PF_FN_LEN])
{
	int i;
	int c;
	for(i = 0; i < arc_pf_count; i++) {
		for(c = 0; c < PF_FN_LEN; c++) {
			if(arc_pfs[i].fn[c] != toupper(fn[c])) {
				break;
			}
			if(fn[c] == '\0') {
				break;
			}
		}
		if((c == PF_FN_LEN) || (fn[c] == '\0' && arc_pfs[i].fn[c] == '\0')) {
			break;
		}
	}
	if(i == arc_pf_count) {
		exit(1);
	}
	file_pf = &arc_pfs[i];
	file_compressed = (file_pf->type[0] == 0x95 && file_pf->type[1] == 0x95);
	file_data = new uint8_t[sizeof(bos_file_stream_t)];
	bos_file_stream_t *s = bos_file_stream();
	s->cache_pos = s->cache_size = 0;
	s->packed_left = file_pf->packsize;
	s->runs = 0;
	file_pos = 0;
	file_ropen(arc_fn);
	file_seek(file_pf->offset, SEEK_SET);
	s->next = bos_file_raw();
}

void pascal bos_file_get(uint8_t *buf, size_t size)
{
	bos_file_stream_t *s = bos_file_stream();
	for(size_t i = 0; i < size && file_pos < file_pf->orgsize; i++) {
		uint8_t previous = s->next;
		buf[i] = previous;
		file_pos++;
		if(file_pos == file_pf->orgsize) {
			break;
		}
		if(!file_compressed) {
			s->next = bos_file_raw();
		} else if(s->runs) {
			s->runs--;
		} else {
			s->next = bos_file_raw();
			if(s->next == previous) {
				s->runs = bos_file_raw();
			}
		}
	}
}

void pascal bos_file_seek(int8_t pos)
{
	uint8_t discard;
	if(pos < 0 || pos < file_pos || pos > file_pf->orgsize) {
		exit(1);
	}
	while(file_pos < pos) {
		bos_file_get(&discard, 1);
	}
}

void bos_file_free(void)
{
	file_close();
	delete[] file_data;
	file_data = 0;
}

#pragma codeseg
