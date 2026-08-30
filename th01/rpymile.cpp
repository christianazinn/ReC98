/* Private TH01 replay process-handoff witness. */

#pragma option -zCT1REPLAY_PROCESS_MILESTONE -G-

#include <stdio.h>
#include "th01/replay_milestone.hpp"

#if T1REPLAY_PROCESS_MILESTONES

static void t1replay_process_milestone_fn(char *fn)
{
	fn[0] = 'T'; fn[1] = '1'; fn[2] = 'M'; fn[3] = 'I';
	fn[4] = 'L'; fn[5] = '.'; fn[6] = 'B'; fn[7] = 'I';
	fn[8] = 'N'; fn[9] = '\0';
}

static void t1replay_process_milestone_config_fn(char *fn)
{
	fn[0] = 'T'; fn[1] = '1'; fn[2] = 'M'; fn[3] = 'I';
	fn[4] = 'L'; fn[5] = '.'; fn[6] = 'C'; fn[7] = 'F';
	fn[8] = 'G'; fn[9] = '\0';
}

// The hidden runtime runner ends DOSBox-X on a bounded timeout rather than
// walking the game back through OP.  Flush each private witness record so its
// final process state survives that intentional teardown.
static void t1replay_process_milestone_flush(void)
{
	asm {
		mov ah, 0Dh
		int 21h
	}
}

void far t1replay_process_milestone_reset(void)
{
	char fn[10];

	t1replay_process_milestone_fn(fn);
	remove(fn);
}

void far t1replay_process_milestone(t1replay_process_milestone_t milestone)
{
	char fn[10];
	char mode[3];
	uint8_t record[8];
	FILE *fp;

	t1replay_process_milestone_fn(fn);
	mode[0] = 'a'; mode[1] = 'b'; mode[2] = '\0';
	fp = fopen(fn, mode);
	if(!fp) {
		return;
	}
	record[0] = 'T'; record[1] = '1'; record[2] = 'M'; record[3] = 1;
	record[4] = static_cast<uint8_t>(milestone);
	record[5] = 0; record[6] = 0; record[7] = 0;
	fwrite(record, 1, sizeof(record), fp);
	fclose(fp);
	t1replay_process_milestone_flush();
}

bool far t1replay_process_milestone_handoff_probe(void)
{
	char fn[10];
	char mode[3];
	uint8_t marker;
	uint8_t extra;
	FILE *fp;
	bool ret;

	t1replay_process_milestone_config_fn(fn);
	mode[0] = 'r'; mode[1] = 'b'; mode[2] = '\0';
	fp = fopen(fn, mode);
	if(!fp) {
		return false;
	}
	ret = (
		(fread(&marker, 1, 1, fp) == 1) &&
		(fread(&extra, 1, 1, fp) == 0) &&
		(marker == 'H')
	);
	fclose(fp);
	if(ret) {
		remove(fn);
		t1replay_process_milestone_flush();
	}
	return ret;
}

bool far t1replay_process_milestone_stage4_clear_probe(void)
{
	char fn[10];
	char mode[3];
	uint8_t marker;
	uint8_t extra;
	FILE *fp;
	bool ret;

	t1replay_process_milestone_config_fn(fn);
	mode[0] = 'r'; mode[1] = 'b'; mode[2] = '\0';
	fp = fopen(fn, mode);
	if(!fp) {
		return false;
	}
	ret = (
		(fread(&marker, 1, 1, fp) == 1) &&
		(fread(&extra, 1, 1, fp) == 0) &&
		(marker == '4')
	);
	fclose(fp);
	if(ret) {
		remove(fn);
		t1replay_process_milestone_flush();
	}
	return ret;
}

#endif
