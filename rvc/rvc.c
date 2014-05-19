#include "rvc.h"

#include <libavformat/avformat.h>

void av_readFrameForCalOpen(void *formatCtx, void * packet, const char *filename) {
	AVFormatContext *pFormatCtx = NULL;
	AVPacket        *pPacket = av_malloc(sizeof(AVPacket));

	av_register_all();
	pFormatCtx = avformat_alloc_context();

	if (avformat_open_input(&pFormatCtx, filename, NULL, NULL) != 0) {
		printf("%s", filename);
		exit(1); // Couldn't open file
	}
	av_dump_format(pFormatCtx, 0, filename, 0);

	formatCtx = (void*) pFormatCtx;
	packet    = (void*) pPacket;
}

int av_readFrameForCal(void *formatCtx, void * packet, uint8_t **data, int *size) {
	AVFormatContext *pFormatCtx = (AVFormatContext *) formatCtx;
	AVPacket        *pPacket    = (AVPacket *) packet;
	int ret = av_read_frame(pFormatCtx, pPacket);
	(*data) = pPacket->data;
	(*size) = pPacket->size;
	return ret;
}

void av_readFrameForCalClose(void * packet) {
	AVPacket        *pPacket = (AVPacket *)packet;
	av_free(pPacket);
}
