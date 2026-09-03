#ifndef TH01_SAVESTATE_ACCEPTANCE_HPP
#define TH01_SAVESTATE_ACCEPTANCE_HPP

// Private acceptance evidence for the savestate guard. This profile is never
// selected by a release build and does not alter T1RPY6 or the resident ABI.
#ifndef T1REPLAY_SAVESTATE_ACCEPTANCE
#ifdef T1SGA
#define T1REPLAY_SAVESTATE_ACCEPTANCE T1SGA
#else
#define T1REPLAY_SAVESTATE_ACCEPTANCE 0
#endif
#endif

#define T1SAVESTATE_ACCEPTANCE_SCHEMA 1
#define T1SAVESTATE_ACCEPTANCE_RECORD_SIZE 32

enum t1savestate_acceptance_event_t {
	T1SAE_BEGIN = 1,
	T1SAE_PAUSE = 2,
	T1SAE_HANDOFF = 3,
	T1SAE_FINALIZE = 4,
	T1SAE_END = 5,
};

// Keep every per-input guard sample behind the same facade as lifecycle
// events. Later private detector methods can be composed here without changing
// either REIIDEN/FUUIN callers or the public T1RPY6 format.
static inline bool t1replay_guard_sample(t1replay_guard_t far *guard)
{
	return t1rpg_sample(guard);
}

#if T1REPLAY_SAVESTATE_ACCEPTANCE

struct t1savestate_acceptance_record_t {
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

typedef char t1savestate_acceptance_record_size_check[
	(sizeof(t1savestate_acceptance_record_t) ==
	 T1SAVESTATE_ACCEPTANCE_RECORD_SIZE) ? 1 : -1
];

static bool t1savestate_acceptance_begin(t1replay_guard_t far *guard);
static bool t1savestate_acceptance_checkpoint(
	t1replay_guard_t far *guard, uint8_t event
);
static void t1savestate_acceptance_end(t1replay_guard_t far *guard);

#define t1replay_guard_begin(guard) \
	t1savestate_acceptance_begin(guard)
#define t1replay_guard_checkpoint(guard, event) \
	t1savestate_acceptance_checkpoint(guard, event)
#define t1replay_guard_end(guard) \
	t1savestate_acceptance_end(guard)

#else

#define t1replay_guard_begin(guard) t1rpg_begin(guard)
#define t1replay_guard_checkpoint(guard, event) t1rpg_checkpoint(guard)
#define t1replay_guard_end(guard) t1rpg_end(guard)

#endif

#endif /* TH01_SAVESTATE_ACCEPTANCE_HPP */
