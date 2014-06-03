
#ifndef RVC_H
#define RVC_H

#include <stdint.h>

void av_readFrameForCalOpen(void **formatCtx, void **packet, const char *filename);

int av_readFrameForCal(void *formatCtx, void *packet, uint8_t *data, int *size);

void av_readFrameForCalClose(void *packet);

#endif //RVC_H
