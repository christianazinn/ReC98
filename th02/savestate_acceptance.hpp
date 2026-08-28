#ifndef TH02_SAVESTATE_ACCEPTANCE_HPP
#define TH02_SAVESTATE_ACCEPTANCE_HPP

// Private TH02 acceptance evidence. The public replay format and the shared
// TH04/TH05 guard are intentionally untouched.
#ifndef T2REPLAY_SAVESTATE_ACCEPTANCE
#ifdef T2SGA
#define T2REPLAY_SAVESTATE_ACCEPTANCE T2SGA
#else
#define T2REPLAY_SAVESTATE_ACCEPTANCE 0
#endif
#endif

#define T2SAVESTATE_ACCEPTANCE_SCHEMA 1
#define T2SAVESTATE_ACCEPTANCE_RECORD_SIZE 32

enum t2savestate_acceptance_event_t {
	T2SAE_BEGIN = 1,
	T2SAE_PAUSE = 2,
	T2SAE_HANDOFF = 3,
	T2SAE_FINALIZE = 4,
	T2SAE_END = 5,
};

#if T2REPLAY_SAVESTATE_ACCEPTANCE

struct t2savestate_acceptance_record_t {
	char magic[8];
	uint8_t schema;
	uint8_t event;
	uint8_t checkpoint_result;
	uint8_t guard_flags;
	uint8_t raw_ok;
	uint8_t marker;
	uint16_t reserved0;
	uint32_t committed_size;
	uint32_t expected_size;
	uint32_t actual_size;
	uint16_t checksum;
	uint16_t reserved1;
};

typedef char t2savestate_acceptance_record_size_check[
	(sizeof(t2savestate_acceptance_record_t) ==
	 T2SAVESTATE_ACCEPTANCE_RECORD_SIZE) ? 1 : -1
];

static bool t2savestate_acceptance_begin(void);
static bool t2savestate_acceptance_checkpoint(uint8_t event);
static void t2savestate_acceptance_observe(uint8_t event);
static void t2savestate_acceptance_end(void);

#define t2replay_guard_begin() t2savestate_acceptance_begin()
#define t2replay_guard_checkpoint(event) \
	t2savestate_acceptance_checkpoint(event)
#define t2replay_guard_observe(event) \
	t2savestate_acceptance_observe(event)
#define t2replay_guard_end() t2savestate_acceptance_end()

#else

#define t2replay_guard_begin() replay_protect_begin()
#define t2replay_guard_checkpoint(event) replay_protect_checkpoint()
#define t2replay_guard_observe(event) ((void)0)
#define t2replay_guard_end() replay_protect_end()

#endif

#endif /* TH02_SAVESTATE_ACCEPTANCE_HPP */
