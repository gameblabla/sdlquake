#include "quakedef.h"

volatile int snd_current_sample_pos;

qboolean SNDDMA_Init(void) {
	shm = (void *) Hunk_AllocName(sizeof(*shm), "shm");
	shm->splitbuffer = 0;
	shm->samplebits = 16;
	shm->speed = 22050;
	shm->channels = 2;
	shm->samples = 4096;
	shm->samplepos = 0;
	shm->soundalive = true;
	shm->gamealive = true;
	shm->submission_chunk = 1;
	shm->buffer = Hunk_AllocName(65536, "shmbuf");

	snd_current_sample_pos = (shm->samples >> 3);

	return true;
}

int SNDDMA_GetDMAPos(void) {
	shm->samplepos = snd_current_sample_pos;
	return shm->samplepos;
}

void SNDDMA_Submit(void) {
}

void SNDDMA_Shutdown(void) {
}
