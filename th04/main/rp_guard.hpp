#ifndef TH04_MAIN_RP_GUARD_HPP
#define TH04_MAIN_RP_GUARD_HPP

#include "platform.h"

// Savestate rollback protection is deliberately outside the replay format.
// A new recording starts a fresh guard; INVALID and ERROR stay sticky only
// until that recording ends.
enum replay_protect_diag_t {
	RPD_NONE = 0,
	RPD_CTX_ALLOC,
	RPD_BPB_READ,
	RPD_BPB_UNSUPPORTED,
	RPD_ROOT_RANGE,
	RPD_SHORT_NAME,
	RPD_ROOT_SECTOR_READ,
	RPD_ROOT_NOT_FOUND,
	RPD_LOCATED_RANGE,
	RPD_LOCATED_SECTOR_READ,
	RPD_LOCATED_ENTRY_BAD,
	RPD_LOCATED_NAME,
	RPD_VERIFY_MISMATCH,
	RPD_GUARD_CREATE,
	RPD_GUARD_CREATE_NONZERO,
	RPD_CHECKPOINT_APPEND,
	RPD_CHECKPOINT_WRITE,
	RPD_CHECKPOINT_MISMATCH,
	RPD_CLOSE_COMMIT,
	RPD_MARKER_CLUSTER,
	RPD_MARKER_VALUE,
	RPD_MARKER_WRITE,
	RPD_MARKER_VERIFY,
	RPD_ROOT_CHAIN,
	RPD_ROOT_FAT_READ,
	RPD_EXTENDED_READ_UNAVAILABLE,
	RPD_MARKER_SECTOR_READ,
};

#define REPLAY_PROTECT_FLAG_INVALID 0x01
#define REPLAY_PROTECT_FLAG_LOCATED 0x02
#define REPLAY_PROTECT_FLAG_ERROR   0x04

#define REPLAY_PROTECT_INTERVAL_SAMPLES 128UL

struct replay_protect_diag_record_t {
	char magic[8];
	uint8_t code;
	uint8_t flags;
	uint8_t drive;
	uint8_t fat_type;
	uint16_t dos_ax;
	uint16_t bytes_per_sector;
	uint16_t root_entries;
	uint32_t root_start;
	uint16_t root_sectors;
	uint32_t sector;
	uint16_t offset;
	uint32_t expected;
	uint32_t actual;
	uint16_t int25_flags;
	uint16_t int25_stack_flags;
};

typedef char replay_protect_diag_size_check[
	(sizeof(replay_protect_diag_record_t) == 42) ? 1 : -1
];

// Starts and ends one recording's protection lifecycle. Failure is fail-closed
// for replay saving but never interrupts gameplay.
bool replay_protect_begin(void);
void replay_protect_end(void);

// Verifies the physical guard and advances it by one byte. On rollback, the
// guard is poisoned in place so repeated loads of the same savestate remain
// blocked.
bool replay_protect_checkpoint(void);
bool replay_protect_blocked(void);

#endif /* TH04_MAIN_RP_GUARD_HPP */
