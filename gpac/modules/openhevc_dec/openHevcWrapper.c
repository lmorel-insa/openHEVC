#include <stdio.h>
#include "openHevcWrapper.h"
#include "libavcodec/avcodec.h"
#include "libavcodec/hevc_frame.h"
#include "libavutil/mem.h"
#include "libavutil/opt.h"

#define NAL_UNITS	1
typedef struct OpenHevcWrapperContext {
    AVCodec *codec;
    AVCodecContext *c;
    AVFrame *picture;
    AVPacket avpkt;
    AVCodecParserContext *parser;
    
    AVCodec *ecodec;
    AVCodecContext *ec;
    AVFrame *epicture;
    AVPacket eavpkt;
    int nb_layers;
} OpenHevcWrapperContext;

OpenHevc_Handle libOpenHevcInit(int nb_pthreads, int nb_layers, int enable_frame_based)
{
    /* register all the codecs */
    avcodec_register_all();

    OpenHevcWrapperContext * openHevcContext = av_malloc(sizeof(OpenHevcWrapperContext));
    openHevcContext->nb_layers = nb_layers; 
    av_init_packet(&openHevcContext->avpkt);
    openHevcContext->codec = avcodec_find_decoder(AV_CODEC_ID_HEVC);
    if (!openHevcContext->codec) {
        fprintf(stderr, "codec not found\n");
        return NULL;
    }
    openHevcContext->parser  = av_parser_init( openHevcContext->codec->id );
    openHevcContext->c       = avcodec_alloc_context3(openHevcContext->codec);
    openHevcContext->picture = avcodec_alloc_frame();
    
    
    if(openHevcContext->codec->capabilities&CODEC_CAP_TRUNCATED)
        openHevcContext->c->flags |= CODEC_FLAG_TRUNCATED; /* we do not send complete frames */

    /* For some codecs, such as msmpeg4 and mpeg4, width and height
         MUST be initialized there because this information is not
         available in the bitstream. */

    /* open it */

    if(nb_pthreads)	{
        if(enable_frame_based==3)
            av_opt_set(openHevcContext->c, "thread_type", "shvccodec", 0);
        else
            if(enable_frame_based==2){
                av_opt_set(openHevcContext->c, "thread_type", "codec", 0);
            }
            else
                if(enable_frame_based==1)
                    av_opt_set(openHevcContext->c, "thread_type", "frame", 0);
                else {
                    av_opt_set(openHevcContext->c, "thread_type", "slice", 0);
                 }
        if(nb_pthreads !=1)
        	av_opt_set_int(openHevcContext->c, "threads", nb_pthreads, 0);
        else
        	av_opt_set_int(openHevcContext->c, "threads", 1, 0);
    }
    openHevcContext->c->is_base_layer = 1;
    if (avcodec_open2(openHevcContext->c, openHevcContext->codec, NULL) < 0) {
        fprintf(stderr, "could not open codec\n");
        return NULL;
    }
    av_opt_set_int(openHevcContext->c->priv_data, "disable-au", 0, 0);

    if(nb_layers>1)  {// Create the second decoder
        av_init_packet(&openHevcContext->eavpkt);
        openHevcContext->ecodec  = avcodec_find_decoder(AV_CODEC_ID_SHVC);
        openHevcContext->ec     = avcodec_alloc_context3(openHevcContext->ecodec);
        openHevcContext->epicture = avcodec_alloc_frame();
        if(openHevcContext->ecodec->capabilities&CODEC_CAP_TRUNCATED)
            openHevcContext->ec->flags |= CODEC_FLAG_TRUNCATED; /* we do not send complete frames */

        if(nb_pthreads)	{
            if(enable_frame_based==3)
                av_opt_set(openHevcContext->ec, "thread_type", "shvccodec", 0);
            else
                if(enable_frame_based==2)
                    av_opt_set(openHevcContext->ec, "thread_type", "codec", 0);
                else
                    if(enable_frame_based==1)
                        av_opt_set(openHevcContext->ec, "thread_type", "frame", 0);
                    else
                        av_opt_set(openHevcContext->ec, "thread_type", "slice", 0);
            av_opt_set_int(openHevcContext->ec, "threads", nb_pthreads, 0);
        }
        openHevcContext->ec->is_base_layer = 0;
        if (avcodec_open2(openHevcContext->ec, openHevcContext->ecodec, NULL) < 0) {
            fprintf(stderr, "could not open codec\n");
            return NULL;
        }


        av_opt_set_int(openHevcContext->ec->priv_data, "disable-au", 0, 0);
    }
    av_opt_set_int(openHevcContext->c->priv_data, "layer-id", 1, 0);
    if(openHevcContext->nb_layers >1) {
    	av_opt_set_int(openHevcContext->ec->priv_data, "layer-id", 2, 0);
    }
    return (OpenHevc_Handle) openHevcContext;
}
static int first = 1;
static int read_layer_id(const unsigned char *buff) {
    if(buff[0] == 0 && buff[1] == 0 && buff[2] ==0)
        return ((buff[4]&0x01)<<5) + ((buff[5]&0xF8)>>3);
    else
        return ((buff[3]&0x01)<<5) + ((buff[4]&0xF8)>>3);
}


int libOpenHevcDecode(OpenHevc_Handle openHevcHandle, const unsigned char *buff, int au_len, int64_t pts/*, int nb_layers*/)
{
    int got_picture = 0, got_picture1 = 0, len;
    OpenHevcWrapperContext * openHevcContext = (OpenHevcWrapperContext *) openHevcHandle;
    int layer_id = 0;
    int nb_layers = openHevcContext->nb_layers;

    if(au_len > 3)
        layer_id = read_layer_id(buff);
#if    !NAL_UNITS
    if(!layer_id ){
#endif
        openHevcContext->avpkt.size = au_len;
        openHevcContext->avpkt.data = buff;
        openHevcContext->avpkt.pts  = pts;
        len = avcodec_decode_video2(openHevcContext->c, openHevcContext->picture, &got_picture, &openHevcContext->avpkt);
#if    !NAL_UNITS
    }
#endif
    if(nb_layers>1) {
#if	!NAL_UNITS
        if(layer_id || first) {
#endif
            openHevcContext->ec->BLheight = openHevcContext->c->height;
            openHevcContext->ec->BLwidth = openHevcContext->c->width;
            openHevcContext->ec->based_frame = openHevcContext->c->based_frame;

            openHevcContext->eavpkt.size = au_len;
            openHevcContext->eavpkt.data = buff;
            openHevcContext->eavpkt.pts  = pts;
            first = 0; 
             len = avcodec_decode_video2(openHevcContext->ec, openHevcContext->epicture, &got_picture1, &openHevcContext->eavpkt);
#if	!NAL_UNITS
        }
#endif
        if(nb_layers>1)
        if(got_picture1 )    {
            if (len < 0) {
                fprintf(stderr, "Error while decoding frame \n");
                return -1;
            }
            return 2;
        } else
        	return 0;
    }

    if (len < 0) {
        fprintf(stderr, "Error while decoding frame \n");
        return -1;
    }
    return got_picture;
}

void libOpenHevcGetPictureInfo(OpenHevc_Handle openHevcHandle, OpenHevc_FrameInfo *openHevcFrameInfo)
{
    OpenHevcWrapperContext * openHevcContext = (OpenHevcWrapperContext *) openHevcHandle;
    AVFrame *picture;
    AVCodecContext *c;
    if(openHevcContext->nb_layers>1) {
        c = openHevcContext->ec;
        picture= openHevcContext->epicture;
    } else {
        c = openHevcContext->c;
        picture= openHevcContext->picture;
    }
    openHevcFrameInfo->nYPitch    = c->width;
    openHevcFrameInfo->nUPitch    = c->width>>1;
    openHevcFrameInfo->nVPitch    = c->width>>1;
    openHevcFrameInfo->nBitDepth  = 8;
    openHevcFrameInfo->nWidth     = c->width;
    openHevcFrameInfo->nHeight    = c->height;

    openHevcFrameInfo->sample_aspect_ratio.num = picture->sample_aspect_ratio.num;
    openHevcFrameInfo->sample_aspect_ratio.den = picture->sample_aspect_ratio.den;
    openHevcFrameInfo->display_picture_number = picture->display_picture_number;
    openHevcFrameInfo->frameRate.num  = 0;
    openHevcFrameInfo->frameRate.den  = 0;
    openHevcFrameInfo->flag       = 0; //progressive, interlaced, interlaced top field first, interlaced bottom field first.
    openHevcFrameInfo->nTimeStamp = picture->pts;
}

void libOpenHevcGetPictureSize2(OpenHevc_Handle openHevcHandle, OpenHevc_FrameInfo *openHevcFrameInfo)
{
    OpenHevcWrapperContext * openHevcContext = (OpenHevcWrapperContext *) openHevcHandle;
    AVFrame *picture;
    libOpenHevcGetPictureInfo(openHevcHandle, openHevcFrameInfo);
    if(openHevcContext->nb_layers>1)
    	picture = openHevcContext->epicture;
    else
    	picture = openHevcContext->picture;

    openHevcFrameInfo->nYPitch = picture->linesize[0];
    openHevcFrameInfo->nUPitch = picture->linesize[1];
    openHevcFrameInfo->nVPitch = picture->linesize[2];

}

int libOpenHevcGetOutput(OpenHevc_Handle openHevcHandle, int got_picture, OpenHevc_Frame *openHevcFrame)
{
    OpenHevcWrapperContext * openHevcContext = (OpenHevcWrapperContext *) openHevcHandle;
    AVFrame *picture;
    if(openHevcContext->nb_layers>1)
        picture = openHevcContext->epicture;
    else
        picture = openHevcContext->picture;
    if (got_picture) {
        openHevcFrame->pvY       = (void *) picture->data[0];
        openHevcFrame->pvU       = (void *) picture->data[1];
        openHevcFrame->pvV       = (void *) picture->data[2];
        libOpenHevcGetPictureInfo(openHevcHandle, &openHevcFrame->frameInfo);
    }
    return 1;
}

int libOpenHevcGetOutputCpy(OpenHevc_Handle openHevcHandle, int got_picture, OpenHevc_Frame_cpy *openHevcFrame)
{
    int y;
    int y_offset, y_offset2;
    AVFrame *picture;
    AVCodecContext *c;
    OpenHevcWrapperContext * openHevcContext = (OpenHevcWrapperContext *) openHevcHandle;

    if(openHevcContext->nb_layers>1){
    	picture = openHevcContext->epicture;
    	c = openHevcContext->ec;
    } else{
    	picture = openHevcContext->picture;
    	c = openHevcContext->c;
    }
    if( got_picture ) {
        unsigned char *Y = (unsigned char *) openHevcFrame->pvY;
        unsigned char *U = (unsigned char *) openHevcFrame->pvU;
        unsigned char *V = (unsigned char *) openHevcFrame->pvV;
        y_offset = y_offset2 = 0;
        for(y = 0; y < c->height; y++) {
            memcpy(&Y[y_offset2], &picture->data[0][y_offset], c->width);
            y_offset  += picture->linesize[0];
            y_offset2 += c->width;
        }
        y_offset = y_offset2 = 0;
        for(y = 0; y < c->height/2; y++) {
            memcpy(&U[y_offset2], &picture->data[1][y_offset], c->width/2);
            memcpy(&V[y_offset2], &picture->data[2][y_offset], c->width/2);
            y_offset  += picture->linesize[1];
            y_offset2 += c->width / 2;
        }
        libOpenHevcGetPictureInfo(openHevcHandle, &openHevcFrame->frameInfo);
    }
    return 1;
}
void libOpenHevcSetCheckMD5(OpenHevc_Handle openHevcHandle, int val)
{
    OpenHevcWrapperContext * openHevcContext = (OpenHevcWrapperContext *) openHevcHandle;
    av_opt_set_int(openHevcContext->c->priv_data, "decode-checksum", val, 0);
    if(openHevcContext->nb_layers>1){
        av_opt_set_int(openHevcContext->ec->priv_data, "decode-checksum", val, 0);
    }
    
}
void libOpenHevcSetDisableAU(OpenHevc_Handle openHevcHandle, int val)
{
    OpenHevcWrapperContext * openHevcContext = (OpenHevcWrapperContext *) openHevcHandle;
    av_opt_set_int(openHevcContext->c->priv_data, "disable-au", val, 0);
    if(openHevcContext->nb_layers >1){
        av_opt_set_int(openHevcContext->ec->priv_data, "disable-au", val, 0);
    }
}
void libOpenHevcSetLayerId(OpenHevc_Handle openHevcHandle) {
    OpenHevcWrapperContext * openHevcContext = (OpenHevcWrapperContext *) openHevcHandle;
    av_opt_set_int(openHevcContext->c->priv_data, "layer-id", 1, 0);
    if(openHevcContext->nb_layers >1) {
        av_opt_set_int(openHevcContext->ec->priv_data, "layer-id", 2, 0);
    }
}

void libOpenHevcSetTemporalLayer_id(OpenHevc_Handle openHevcHandle, int val)
{
    OpenHevcWrapperContext * openHevcContext = (OpenHevcWrapperContext *) openHevcHandle;
    av_opt_set_int(openHevcContext->c->priv_data, "temporal-layer-id", val+1, 0);
    if(openHevcContext->nb_layers >1)
            av_opt_set_int(openHevcContext->c->priv_data, "temporal-layer-id", val+1, 0);
}

void libOpenHevcClose(OpenHevc_Handle openHevcHandle)
{
    OpenHevcWrapperContext * openHevcContext = (OpenHevcWrapperContext *) openHevcHandle;

    avcodec_close(openHevcContext->c);
    if(openHevcContext->nb_layers >1){

        avcodec_close(openHevcContext->ec);
        av_free(openHevcContext->ec);
        av_free(openHevcContext->epicture);
    }

    av_free(openHevcContext->c);
    av_free(openHevcContext->picture);
    av_free(openHevcContext);
}

void libOpenHevcFlush(OpenHevc_Handle openHevcHandle)
{
    OpenHevcWrapperContext * openHevcContext = (OpenHevcWrapperContext *) openHevcHandle;
    openHevcContext->codec->flush(openHevcContext->c);
}

const char *libOpenHevcVersion(OpenHevc_Handle openHevcHandle)
{
    return "OpenHEVC v"NV_VERSION;
}

