#include "config.h"
#include "libavutil/avassert.h"
#include "libavutil/pixdesc.h"
#include "libavcodec/get_bits.h"
#include "libavcodec/hevc.h"
#include "libavcodec/x86/hevcdsp.h"
#include "libavcodec/bit_depth_template.c"


#include <emmintrin.h>
#include <tmmintrin.h>
#ifdef __SSE4_1__
#include <smmintrin.h>
#endif

DECLARE_ALIGNED(16, static const int8_t, up_sample_filter_luma_x2_sse[4][16] )= /*0 , 8 */
{
    { 0,  0,  -1,  4, 0,  0,  -1,  4, 0,  0,  -1,  4, 0,  0,  -1,  4},
    { 0, 64, -11, 40, 0, 64, -11, 40, 0, 64, -11, 40, 0, 64, -11, 40},
    { 0,  0,  40, -11,  0,   0, 40, -11, 0,   0, 40, -11,  0,  0, 40, -11},
    { 0,   0, 4,  -1,  0,   0, 4,  -1,  0,   0, 4,  -1,  0,   0, 4,  -1},
};


#define BIT_DEPTH 8

#define LumHor_FILTER(pel, coeff) \
(pel[0]*coeff[0] + pel[1]*coeff[1] + pel[2]*coeff[2] + coeff[3]*pel[3] + pel[4]*coeff[4] + pel[5]*coeff[5] + pel[6]*coeff[6] + pel[7]*coeff[7])

#define CroHor_FILTER(pel, coeff) \
(pel[0]*coeff[0] + pel[1]*coeff[1] + pel[2]*coeff[2] + pel[3]*coeff[3])

#define LumVer_FILTER(pel, coeff) \
(pel[0]*coeff[0] + pel[1]*coeff[1] + pel[2]*coeff[2] + pel[3]*coeff[3] + pel[4]*coeff[4] + pel[5]*coeff[5] + pel[6]*coeff[6] + pel[7]*coeff[7])

#define CroVer_FILTER(pel, coeff) \
(pel[0]*coeff[0] + pel[1]*coeff[1] + pel[2]*coeff[2] + pel[3]*coeff[3])

#define LumVer_FILTER1(pel, coeff, width) \
(pel[0]*coeff[0] + pel[width]*coeff[1] + pel[width*2]*coeff[2] + pel[width*3]*coeff[3] + pel[width*4]*coeff[4] + pel[width*5]*coeff[5] + pel[width*6]*coeff[6] + pel[width*7]*coeff[7])
// Define the function for up-sampling
#define CroVer_FILTER1(pel, coeff, widthEL) \
(pel[0]*coeff[0] + pel[widthEL]*coeff[1] + pel[widthEL*2]*coeff[2] + pel[widthEL*3]*coeff[3])

#ifdef  SVC_EXTENSION




void ff_upsample_filter_block_luma_h_ALL_sse(int16_t *dst, ptrdiff_t dststride, uint8_t *_src, ptrdiff_t _srcstride,
            int x_EL, int x_BL, int block_w, int block_h, int widthEL,
            const struct HEVCWindow *Enhscal, struct UpsamplInf *up_info){




}
/*
int x, i, j;
    int16_t*   dst_tmp;
    pixel*   src_tmp, *src = (pixel *) _src - x_BL;
    const int8_t*   coeff;

    for( i = 0; i < block_w; i++ )	{
        x        = av_clip_c(i+x_EL, leftStartL, rightEndL);
        coeff    = up_sample_filter_luma_x2[x&0x01];
        dst_tmp  = _dst  + i;
        src_tmp  = src + ((x-leftStartL)>>1);
        for( j = 0; j < block_h ; j++ ) {
            *dst_tmp  = LumHor_FILTER_Block(src_tmp, coeff);
            src_tmp  += _srcstride;
            dst_tmp  += _dststride;
        }
    }


 */
void ff_upsample_filter_block_luma_h_X2_sse(int16_t *dst, ptrdiff_t dststride, uint8_t *_src, ptrdiff_t _srcstride,
            int x_EL, int x_BL, int width, int height, int widthEL,
            const struct HEVCWindow *Enhscal, struct UpsamplInf *up_info){
    int x, y;
    __m128i x1, x2, x3, x4, x5, x6, x7, x8, r1, c1, c2, c3, c4;
    uint8_t  *src       = (uint8_t*) _src - x_BL;
    ptrdiff_t srcstride = _srcstride;

    c1 = _mm_load_si128((__m128i *) up_sample_filter_luma_x2_sse[0]);
    c2 = _mm_load_si128((__m128i *) up_sample_filter_luma_x2_sse[1]);
    c3 = _mm_load_si128((__m128i *) up_sample_filter_luma_x2_sse[2]);
    c4 = _mm_load_si128((__m128i *) up_sample_filter_luma_x2_sse[3]);
    for (y = 0; y < height; y++) {
        for (x = 0; x < width; x += 8) {
            int e = (x+x_EL)>>1;
            x1 = _mm_loadu_si128((__m128i *) &src[e - 3]);
            x2 = _mm_loadu_si128((__m128i *) &src[e - 2]);
            x3 = _mm_loadu_si128((__m128i *) &src[e - 1]);
            x4 = _mm_loadu_si128((__m128i *) &src[e ]);
            x5 = _mm_loadu_si128((__m128i *) &src[e + 1]);
            x6 = _mm_loadu_si128((__m128i *) &src[e + 2]);
            x7 = _mm_loadu_si128((__m128i *) &src[e + 3]);
            x8 = _mm_loadu_si128((__m128i *) &src[e + 4]);

            x1 = _mm_unpacklo_epi8(x1, x1);
            x2 = _mm_unpacklo_epi8(x2, x2);
            x3 = _mm_unpacklo_epi8(x3, x3);
            x4 = _mm_unpacklo_epi8(x4, x4);
            x5 = _mm_unpacklo_epi8(x5, x5);
            x6 = _mm_unpacklo_epi8(x6, x6);
            x7 = _mm_unpacklo_epi8(x7, x7);
            x8 = _mm_unpacklo_epi8(x8, x8);

            x1 = _mm_unpacklo_epi8(x1, x2);
            x2 = _mm_unpacklo_epi8(x3, x4);
            x3 = _mm_unpacklo_epi8(x5, x6);
            x4 = _mm_unpacklo_epi8(x7, x8);

            x2 = _mm_maddubs_epi16(x2,c2);
            x3 = _mm_maddubs_epi16(x3,c3);
            x1 = _mm_maddubs_epi16(x1,c1);
            x4 = _mm_maddubs_epi16(x4,c4);

            x1 = _mm_add_epi16(x1, x2);
            x2 = _mm_add_epi16(x3, x4);
            r1 = _mm_add_epi16(x1, x2);
            _mm_store_si128((__m128i *) &dst[x], r1);
        }
        src += srcstride;
        dst += dststride;
    }
}

void ff_upsample_filter_block_luma_h_X1_5_sse(int16_t *dst, ptrdiff_t dststride, uint8_t *_src, ptrdiff_t _srcstride,
            int x_EL, int x_BL, int block_w, int block_h, int widthEL,
            const struct HEVCWindow *Enhscal, struct UpsamplInf *up_info){

}

void ff_upsample_filter_block_luma_v_ALL_sse(uint8_t *dst, ptrdiff_t dststride, int16_t *_src, ptrdiff_t _srcstride,
            int y_BL, int x_EL, int y_EL, int block_w, int block_h, int widthEL, int heightEL,
            const struct HEVCWindow *Enhscal, struct UpsamplInf *up_info){
}

void ff_upsample_filter_block_luma_v_X2_sse(uint8_t *dst, ptrdiff_t dststride, int16_t *_src, ptrdiff_t _srcstride,
            int y_BL, int x_EL, int y_EL, int block_w, int block_h, int widthEL, int heightEL,
            const struct HEVCWindow *Enhscal, struct UpsamplInf *up_info){

}

void ff_upsample_filter_block_luma_v_X1_5_sse(uint8_t *dst, ptrdiff_t dststride, int16_t *_src, ptrdiff_t _srcstride,
            int y_BL, int x_EL, int y_EL, int block_w, int block_h, int widthEL, int heightEL,
            const struct HEVCWindow *Enhscal, struct UpsamplInf *up_info){

}

void ff_upsample_filter_block_cr_h_ALL_sse(int16_t *dst, ptrdiff_t dststride, uint8_t *_src, ptrdiff_t _srcstride,
            int x_EL, int x_BL, int block_w, int block_h, int widthEL,
            const struct HEVCWindow *Enhscal, struct UpsamplInf *up_info){

}

void ff_upsample_filter_block_cr_h_X2_sse(int16_t *dst, ptrdiff_t dststride, uint8_t *_src, ptrdiff_t _srcstride,
            int x_EL, int x_BL, int block_w, int block_h, int widthEL,
            const struct HEVCWindow *Enhscal, struct UpsamplInf *up_info){

}

void ff_upsample_filter_block_cr_h_X1_5_sse(int16_t *dst, ptrdiff_t dststride, uint8_t *_src, ptrdiff_t _srcstride,
            int x_EL, int x_BL, int block_w, int block_h, int widthEL,
            const struct HEVCWindow *Enhscal, struct UpsamplInf *up_info){

}


void ff_upsample_filter_block_cr_v_ALL_sse(uint8_t *dst, ptrdiff_t dststride, int16_t *_src, ptrdiff_t _srcstride,
            int y_BL, int x_EL, int y_EL, int block_w, int block_h, int widthEL, int heightEL,
            const struct HEVCWindow *Enhscal, struct UpsamplInf *up_info){

}

void ff_upsample_filter_block_cr_v_X2_sse(uint8_t *dst, ptrdiff_t dststride, int16_t *_src, ptrdiff_t _srcstride,
		int y_BL, int x_EL, int y_EL, int block_w, int block_h, int widthEL, int heightEL,
		const struct HEVCWindow *Enhscal, struct UpsamplInf *up_info){

}

void ff_upsample_filter_block_cr_v_X1_5_sse(uint8_t *dst, ptrdiff_t dststride, int16_t *_src, ptrdiff_t _srcstride,
		int y_BL, int x_EL, int y_EL, int block_w, int block_h, int widthEL, int heightEL,
		const struct HEVCWindow *Enhscal, struct UpsamplInf *up_info){

}

#define UPSAMPLE_H_TABLE()                                     \
for( i = 0; i < block_w; i+=8 ){                                \
    m5      =   _mm_adds_epu16(m0,m3);                          \
    m5      =   _mm_min_epu16(m5,m4);                           \
    m7      =   _mm_mullo_epi16(m5,m1);                         \
    m5      =   _mm_mulhi_epu16(m5,m1);                         \
    m8      =   _mm_unpackhi_epi16(m7,m5);                      \
    m5      =   _mm_unpacklo_epi16(m7,m5);                      \
    m5      =   _mm_add_epi32(m5,m2);                           \
    m5      =   _mm_srli_epi32(m5,12);                          \
    m8      =   _mm_add_epi32(m8,m2);                           \
    m8      =   _mm_srli_epi32(m8,12);                          \
    _mm_storeu_si128((__m128i *) &temp[i],m5);                  \
    _mm_storeu_si128((__m128i *) &temp[i+4],m8);                \
    m0      =   _mm_add_epi16(m0,m6);                           \
}

void ff_upsample_filter_block_luma_h_2_8_sse( int16_t *dst, ptrdiff_t dststride, uint8_t *_src, ptrdiff_t _srcstride,
                                    int x_EL, int x_BL, int block_w, int block_h, int widthEL,
                                    const struct HEVCWindow *Enhscal, struct UpsamplInf *up_info) {


    int rightEndL  = widthEL - Enhscal->right_offset;
    int leftStartL = Enhscal->left_offset;
    int  i, j, phase, refPos16, refPos, refPos16_2, refPos_2, refPos_3,phase_2;
    int16_t*   dst_tmp= dst;
    uint8_t*   src_tmp= _src;
    const int8_t*   coeff;
    int * temp= NULL;
    __m128i     m0, m1, m2,m3,m4,m5, m6,m7,m8, r0,c0,c1,c2;
    c0= _mm_setzero_si128();
    temp= av_malloc((block_w+16)*sizeof(int));
    m0= _mm_set_epi16(7,6,5,4,3,2,1,0);
    m1= _mm_set1_epi16(up_info->scaleXLum);
    m2= _mm_set1_epi32(up_info->addXLum);
    m3= _mm_set1_epi16(x_EL-leftStartL);
    m4= _mm_set1_epi16(rightEndL-leftStartL);
    m6= _mm_set1_epi16(8);

    UPSAMPLE_H_TABLE();

    for( j = 0; j < block_h ; j++ ) {
        src_tmp  = _src+_srcstride*j;
        dst_tmp  = dst + dststride*j;
        refPos= 0;
        for( i = 0; i < block_w-1; i+=2 ){
            refPos16 = temp[i];
            refPos16_2 = temp[i+1];
            phase    = refPos16 & 15;
            phase_2  = refPos16_2 & 15;
            coeff    = up_sample_filter_luma[phase];
            refPos   = (refPos16 >> 4) - x_BL;

            c1       = _mm_loadl_epi64((__m128i *)&up_sample_filter_luma[refPos16 & 15]);
            c2       = _mm_loadl_epi64((__m128i *)&up_sample_filter_luma[refPos16_2 & 15]);
            c2       = _mm_slli_si128(c2,8);
            c1       = _mm_or_si128(c1,c2);

            refPos_2   = (refPos16_2 >> 4) - x_BL;
            refPos_3   = refPos_2 - refPos;

            m0      =   _mm_loadu_si128((__m128i *) (src_tmp+ refPos - 3));
            if(refPos_3){
            m1      =   _mm_srli_si128(m0,1);
            }
            else{
                m1  =   _mm_srli_si128(m0,0);
            }
            m0      =   _mm_unpacklo_epi64(m0,m1);

            r0      =   _mm_maddubs_epi16(m0,c1);

            r0      =   _mm_hadd_epi16(r0,c0);
            r0      =   _mm_hadd_epi16(r0,c0);

            _mm_maskmoveu_si128(r0, _mm_set_epi32(0,0,0,-1),(char *)(dst_tmp+i));

        }

    }
    av_freep(&temp);
}



#define UPSAMPLE_L_LOAD_H()                                                                     \
m14       = _mm_loadl_epi64((__m128i *)&up_sample_filter_luma[refPos16 & 15]);      \
m13       = _mm_loadl_epi64((__m128i *)&up_sample_filter_luma[refPos16_2 & 15]);    \
m12       = _mm_loadl_epi64((__m128i *)&up_sample_filter_luma[refPos16_3 & 15]);    \
m11       = _mm_loadl_epi64((__m128i *)&up_sample_filter_luma[refPos16_4 & 15]);    \
m13       = _mm_slli_si128(m13,8);                                                          \
m14       = _mm_or_si128(m14,m13);                                                          \
m11       = _mm_slli_si128(m11,8);                                                          \
m12       = _mm_or_si128(m12,m11)




void ff_upsample_filter_block_luma_h_8_8_sse( int16_t *dst, ptrdiff_t dststride, uint8_t *_src, ptrdiff_t _srcstride,
                                    int x_EL, int x_BL, int block_w, int block_h, int widthEL,
                                    const struct HEVCWindow *Enhscal, struct UpsamplInf *up_info) {

    int rightEndL  = widthEL - Enhscal->right_offset;
    int leftStartL = Enhscal->left_offset;
    int i, j, phase, refPos16, refPos, refPos16_2, refPos_2, refPos_3,refPos16_4, refPos16_3;
    int16_t*   dst_tmp= dst;
    uint8_t*   src_tmp= _src;
    const int8_t*   coeff;
    int * temp= NULL;
    __m128i     m0, m1, m2,m3,m4,m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15;
    m15= _mm_setzero_si128();
    temp= av_malloc((block_w+16)*sizeof(int));
    m0= _mm_set_epi16(7,6,5,4,3,2,1,0);
    m1= _mm_set1_epi16(up_info->scaleXLum);
    m2= _mm_set1_epi32(up_info->addXLum);
    m3= _mm_set1_epi16(x_EL-leftStartL);
    m4= _mm_set1_epi16(rightEndL-leftStartL);
    m6= _mm_set1_epi16(8);

    UPSAMPLE_H_TABLE();

    for( j = 0; j < block_h ; j++ ) {
        src_tmp  = _src+_srcstride*j;
        dst_tmp  = dst + dststride*j;
        refPos= 0;
        for( i = 0; i < block_w; i+=8){
            refPos16 = temp[i];
            refPos16_2 = temp[i+1];
            refPos16_3 = temp[i+2];
            refPos16_4 = temp[i+3];


            phase    = refPos16 & 15;
            coeff    = up_sample_filter_luma[phase];


            UPSAMPLE_L_LOAD_H();


            refPos   = (refPos16 >> 4) - x_BL;
            refPos_2   = (refPos16_2 >> 4) - x_BL;
            refPos_3   = refPos_2 - refPos;

            m0      =   _mm_loadu_si128((__m128i *) (src_tmp+ refPos - 3));
            if(refPos_3){
            m1      =   _mm_srli_si128(m0,1);
            }
            else{
                m1  =   _mm_srli_si128(m0,0);
            }

            refPos   = (refPos16_3 >> 4) - x_BL;
            refPos_3   = refPos - refPos_2;
            if(refPos){
            m2      =   _mm_srli_si128(m1,1);
            }
            else{
                m2  =   _mm_srli_si128(m1,0);
            }
            refPos_2   = (refPos16_4 >> 4) - x_BL;
            refPos_3   = refPos_2 - refPos;
            if(refPos_3){
            m3      =   _mm_srli_si128(m2,1);
            }
            else{
                m3  =   _mm_srli_si128(m2,0);
            }

            m0      =   _mm_unpacklo_epi64(m0,m1);
            m2      =   _mm_unpacklo_epi64(m2,m3);
            m10      =   _mm_maddubs_epi16(m0,m14);
            m2      =   _mm_maddubs_epi16(m2,m12);
            m10      =   _mm_hadd_epi16(m10,m2);


            refPos16 = temp[i+4];
            refPos16_2 = temp[i+5];
            refPos16_3 = temp[i+6];
            refPos16_4 = temp[i+7];


            refPos_3   = (refPos16 >> 4) - x_BL;


            refPos_2   = refPos - refPos_3;
            if(refPos_2){
            m0      =   _mm_srli_si128(m3,1);
            }
            else{
            m0  =   _mm_srli_si128(m3,0);
            }

            UPSAMPLE_L_LOAD_H();

            refPos   = (refPos16 >> 4) - x_BL;

            refPos_2   = (refPos16_2 >> 4) - x_BL;
            refPos_3   = refPos_2 - refPos;
            if(refPos_3){
            m1      =   _mm_srli_si128(m0,1);
            }
            else{
                m1  =   _mm_srli_si128(m0,0);
            }

            refPos_3   = (refPos16_3 >> 4) - x_BL;
            refPos   = refPos_3 - refPos_2;
            if(refPos){
            m2      =   _mm_srli_si128(m1,1);
            }
            else{
                m2  =   _mm_srli_si128(m1,0);
            }
            refPos   = (refPos16_4 >> 4) - x_BL;
            refPos_2   = refPos - refPos_3;
            if(refPos_2){
            m3      =   _mm_srli_si128(m2,1);
            }
            else{
                m3  =   _mm_srli_si128(m2,0);
            }

            m0      =   _mm_unpacklo_epi64(m0,m1);
            m2      =   _mm_unpacklo_epi64(m2,m3);

            m9      =   _mm_maddubs_epi16(m0,m14);
            m2      =   _mm_maddubs_epi16(m2,m12);
            m9      =   _mm_hadd_epi16(m9,m2);

            m10      =   _mm_hadd_epi16(m10,m9);
            _mm_storeu_si128(&dst_tmp[i],m10);

        }

    }
    av_freep(&temp);
}

void ff_upsample_filter_block_cr_h_8_8_sse( int16_t *dst, ptrdiff_t dststride, uint8_t *_src, ptrdiff_t _srcstride,
                                   int x_EL, int x_BL, int block_w, int block_h, int widthEL,
                                   const struct HEVCWindow *Enhscal, struct UpsamplInf *up_info) {


    int leftStartC = Enhscal->left_offset>>1;
    int rightEndC  = widthEL - (Enhscal->right_offset>>1);
    int i, j, phase, refPos16, refPos,refPos16_2, refPos_2,refPos_3,refPos16_3,refPos16_4;
    int16_t*  dst_tmp;
    uint8_t*  src_tmp;
    const int8_t*  coeff;

    int * temp= NULL;
    __m128i     m0, m1, m2,m3,m4,m5, m6,m7,m8, r0,r1,m15,m14,m13,m12,m11;
    m15= _mm_setzero_si128();
            temp= av_malloc((block_w+16)*sizeof(int));
            m0= _mm_set_epi16(7,6,5,4,3,2,1,0);
            m1= _mm_set1_epi16(up_info->scaleXCr);
            m2= _mm_set1_epi32(up_info->addXCr);
            m3= _mm_set1_epi16(x_EL-leftStartC);            //!= from luma
            m4= _mm_set1_epi16(rightEndC-leftStartC);       //!= from luma
            m6= _mm_set1_epi16(8);

            UPSAMPLE_H_TABLE();

    m5= _mm_set_epi32(0,0,0,-1);
    for( j = 0; j < block_h ; j++ ) {
           src_tmp  = _src+_srcstride*j;
           dst_tmp  = dst + dststride*j;
           refPos= 0;
           for( i = 0; i < block_w; i+=8 ){
               refPos16 = temp[i];
               refPos16_2 = temp[i+1];
               refPos16_3 = temp[i+2];
               refPos16_4 = temp[i+3];

               coeff    = up_sample_filter_chroma[phase];
               refPos   = (refPos16 >> 4) - x_BL;

               m14       = _mm_loadl_epi64((__m128i *)&up_sample_filter_chroma[refPos16 & 15]);
               m14      = _mm_and_si128(m14,m5);
               m13       = _mm_loadl_epi64((__m128i *)&up_sample_filter_chroma[refPos16_2 & 15]);
               m13       = _mm_and_si128(m13,m5);
               m13       = _mm_slli_si128(m13,4);
               m14       = _mm_or_si128(m14,m13);

               m12       = _mm_loadl_epi64((__m128i *)&up_sample_filter_chroma[refPos16_3 & 15]);
               m12      = _mm_and_si128(m12,m5);
               m11       = _mm_loadl_epi64((__m128i *)&up_sample_filter_chroma[refPos16_4 & 15]);
               m11       = _mm_and_si128(m11,m5);
               m11       = _mm_slli_si128(m11,4);
               m12       = _mm_or_si128(m12,m11);

               m14      =   _mm_unpacklo_epi64(m14,m12);

               m0      =   _mm_loadu_si128((__m128i *) (src_tmp+ refPos - 1));

               refPos_2   = (refPos16_2 >> 4) - x_BL;
               refPos_3   = refPos_2 - refPos;
               if(refPos_3){
               m1      =   _mm_srli_si128(m0,1);
               }
               else{
               m1  =   _mm_srli_si128(m0,0);
               }

               refPos_3   = (refPos16_3 >> 4) - x_BL;
               refPos   = refPos_3 - refPos_2;
               if(refPos){
               m2      =   _mm_srli_si128(m1,1);
               }
               else{
               m2  =   _mm_srli_si128(m1,0);
               }

               refPos_2   = (refPos16_4 >> 4) - x_BL;
               refPos   = refPos_2 - refPos_3;
               if(refPos){
               m3      =   _mm_srli_si128(m2,1);
               }
               else{
               m3  =   _mm_srli_si128(m2,0);
               }
               m0      =   _mm_unpacklo_epi32(m0,m1);
               m2      =   _mm_unpacklo_epi32(m2,m3);
               m0      =   _mm_unpacklo_epi64(m0,m2);

               r0      =   _mm_maddubs_epi16(m0,m14);

               refPos16 = temp[i+4];
               refPos16_2 = temp[i+5];
               refPos16_3 = temp[i+6];
               refPos16_4 = temp[i+7];

               refPos   = (refPos16 >> 4) - x_BL;
               refPos_3   = refPos - refPos_2;
               if(refPos_3){
               m0      =   _mm_srli_si128(m3,1);
               }
               else{
               m0  =   _mm_srli_si128(m3,0);
               }

               m14       = _mm_loadl_epi64((__m128i *)&up_sample_filter_chroma[refPos16 & 15]);
               m14      = _mm_and_si128(m14,m5);
               m13       = _mm_loadl_epi64((__m128i *)&up_sample_filter_chroma[refPos16_2 & 15]);
               m13       = _mm_and_si128(m13,m5);
               m13       = _mm_slli_si128(m13,4);
               m14       = _mm_or_si128(m14,m13);

               m12       = _mm_loadl_epi64((__m128i *)&up_sample_filter_chroma[refPos16_3 & 15]);
               m12      = _mm_and_si128(m12,m5);
               m11       = _mm_loadl_epi64((__m128i *)&up_sample_filter_chroma[refPos16_4 & 15]);
               m11       = _mm_and_si128(m11,m5);
               m11       = _mm_slli_si128(m11,4);
               m12       = _mm_or_si128(m12,m11);

               m14      =   _mm_unpacklo_epi64(m14,m12);

               m0      =   _mm_loadu_si128((__m128i *) (src_tmp+ refPos - 1));

               refPos_2   = (refPos16_2 >> 4) - x_BL;
               refPos_3   = refPos_2 - refPos;
               if(refPos_3){
               m1      =   _mm_srli_si128(m0,1);
               }
               else{
               m1  =   _mm_srli_si128(m0,0);
               }

               refPos_3   = (refPos16_3 >> 4) - x_BL;
               refPos   = refPos_3 - refPos_2;
               if(refPos){
               m2      =   _mm_srli_si128(m1,1);
               }
               else{
               m2  =   _mm_srli_si128(m1,0);
               }

               refPos_2   = (refPos16_4 >> 4) - x_BL;
               refPos   = refPos_2 - refPos_3;
               if(refPos){
               m3      =   _mm_srli_si128(m2,1);
               }
               else{
               m3  =   _mm_srli_si128(m2,0);
               }

               m0      =   _mm_unpacklo_epi32(m0,m1);
               m2      =   _mm_unpacklo_epi32(m2,m3);
               m0      =   _mm_unpacklo_epi64(m0,m2);

               r1      =   _mm_maddubs_epi16(m0,m14);

               r0      =   _mm_hadd_epi16(r0,r1);

               _mm_storeu_si128(&dst_tmp[i],r0);


           }
       }
    av_freep(&temp);
}
#define UPSAMPLE_CR_V_COEFF()                                      \
c1      =   _mm_loadl_epi64((__m128i *)coeff);                      \
c1      =   _mm_unpacklo_epi8(c0,c1);                               \
c1      =   _mm_unpacklo_epi16(c0,c1);                              \
c1      =   _mm_srai_epi32(c1,24);                                  \
c2      =   _mm_srli_si128(c1,4);                                   \
c3      =   _mm_srli_si128(c1,8);                                   \
c4      =   _mm_srli_si128(c1,12);                                  \
c1      =   _mm_shuffle_epi32(c1,0);                                \
c2      =   _mm_shuffle_epi32(c2,0);                                \
c3      =   _mm_shuffle_epi32(c3,0);                                \
c4      =   _mm_shuffle_epi32(c4,0)

#define UPSAMPLE_CR_V_LOAD()                                       \
m1= _mm_loadl_epi64((__m128i*)(src_tmp-  _srcstride));              \
m2= _mm_loadl_epi64((__m128i*)src_tmp);                             \
m3= _mm_loadl_epi64((__m128i*)(src_tmp+  _srcstride));              \
m4= _mm_loadl_epi64((__m128i*)(src_tmp+2*_srcstride))

#define UPSAMPLE_COMPUTE_V_4()                                      \
m1= _mm_unpacklo_epi16(c0,m1);                                      \
m2= _mm_unpacklo_epi16(c0,m2);                                      \
m3= _mm_unpacklo_epi16(c0,m3);                                      \
m4= _mm_unpacklo_epi16(c0,m4);                                      \
m1= _mm_srai_epi32(m1,16);                                          \
m2= _mm_srai_epi32(m2,16);                                          \
m3= _mm_srai_epi32(m3,16);                                          \
m4= _mm_srai_epi32(m4,16);                                          \
m1= _mm_mullo_epi32(m1,c1);                                         \
m2= _mm_mullo_epi32(m2,c2);                                         \
m3= _mm_mullo_epi32(m3,c3);                                         \
m4= _mm_mullo_epi32(m4,c4);                                         \
m1= _mm_add_epi32(m1,m2);                                           \
m3= _mm_add_epi32(m3,m4);                                           \
m1= _mm_add_epi32(m1,m3);                                           \
m1= _mm_add_epi32(m1,m9);                                           \
m1= _mm_srli_epi32(m1,N_SHIFT);                                     \
m1= _mm_packus_epi32(m1,c0);                                        \
m1= _mm_packus_epi16(m1,c0)


#define UPSAMPLE_UNPACK_V()                                         \
m1= _mm_unpacklo_epi8(m1,m1);                                        \
m1= _mm_unpacklo_epi16(m1,m1);                                       \
m1= _mm_unpacklo_epi32(m1,m1)

#define UPSAMPLE_STORE_32()                                         \
        _mm_maskmoveu_si128(m1, m8,(char *)&dst_tmp[i])

void ff_upsample_filter_block_cr_v_8_8_sse(uint8_t *dst, ptrdiff_t dststride, int16_t *_src, ptrdiff_t _srcstride,
                                   int y_BL, int x_EL, int y_EL, int block_w, int block_h, int widthEL, int heightEL,
                                   const struct HEVCWindow *Enhscal, struct UpsamplInf *up_info) {
    int leftStartC = Enhscal->left_offset>>1;
    int rightEndC  = widthEL - (Enhscal->right_offset>>1);
    int topStartC  = Enhscal->top_offset>>1;
    int bottomEndC = heightEL - (Enhscal->bottom_offset>>1);
    int y, i, j, phase, refPos16, refPos;
    const int8_t* coeff;
    int16_t *   src_tmp;
    uint8_t * dst_tmp;
    int refPos0 = av_clip_c(y_EL, topStartC, bottomEndC-1);
    int x;
    refPos0     = (((( refPos0 - topStartC )* up_info->scaleYCr + up_info->addYCr) >> 12) -4 )>>4;
    if((rightEndC-2 - x_EL) <= block_w - 1){
        x=rightEndC-2 - x_EL;
    }
    else
    {
        x= block_w - 1;
    }

    __m128i     m1, m2,m3,m4,m8, m9,c0,c1,c2,c3,c4;
        m8= _mm_set_epi32(0,0,0,-1);
        c0= _mm_setzero_si128();
        m9= _mm_set1_epi32(I_OFFSET);

    for( j = 0; j < block_h; j++ )  {
        y =   av_clip_c(y_EL+j - topStartC, 0, bottomEndC-1- topStartC);
        refPos16 =      ((( y  )* up_info->scaleYCr + up_info->addYCr) >> 12)-4;
        phase    = refPos16 & 15;
        coeff    = up_sample_filter_chroma[phase];
        refPos16 = (refPos16>>4);
        refPos   = refPos16 - refPos0;
        src_tmp  = _src  + refPos  * _srcstride;
        dst_tmp  =  dst  + y* dststride + x_EL;

        UPSAMPLE_CR_V_COEFF();

        UPSAMPLE_CR_V_LOAD();
        UPSAMPLE_COMPUTE_V_4();
        UPSAMPLE_UNPACK_V();
        for(i = 0; i < leftStartC - x_EL;i+=4){
            UPSAMPLE_STORE_32();
        }
        for( i; i <= x; i+=4 )  {
            UPSAMPLE_CR_V_LOAD();
            UPSAMPLE_COMPUTE_V_4();
            UPSAMPLE_STORE_32();
            src_tmp+=4;
        }
        UPSAMPLE_CR_V_LOAD();
        UPSAMPLE_COMPUTE_V_4();
        UPSAMPLE_UNPACK_V();
        for(i;i<block_w;i+=4){
            UPSAMPLE_STORE_32();
        }
    }
}

#define UNPACK_SRAI16_4(inst, dst, src)                                        \
    dst ## 1 = _mm_srai_epi32(inst(c0, src ## 1), 16);                         \
    dst ## 2 = _mm_srai_epi32(inst(c0, src ## 2), 16);                         \
    dst ## 3 = _mm_srai_epi32(inst(c0, src ## 3), 16);                         \
    dst ## 4 = _mm_srai_epi32(inst(c0, src ## 4), 16)
#define UNPACK_SRAI16_8(inst, dst, src)                                        \
    dst ## 1 = _mm_srai_epi32(inst(c0, src ## 1), 16);                         \
    dst ## 2 = _mm_srai_epi32(inst(c0, src ## 2), 16);                         \
    dst ## 3 = _mm_srai_epi32(inst(c0, src ## 3), 16);                         \
    dst ## 4 = _mm_srai_epi32(inst(c0, src ## 4), 16);                         \
    dst ## 5 = _mm_srai_epi32(inst(c0, src ## 5), 16);                         \
    dst ## 6 = _mm_srai_epi32(inst(c0, src ## 6), 16);                         \
    dst ## 7 = _mm_srai_epi32(inst(c0, src ## 7), 16);                         \
    dst ## 8 = _mm_srai_epi32(inst(c0, src ## 8), 16)

#define MUL_ADD_V_4(mul, add, dst, src, coeff1,coeff2,coeff3,coeff4)           \
    dst = mul(src ## 1, coeff1);                                                \
    dst = add(dst, mul(src ## 2, coeff2));                                      \
    dst = add(dst, mul(src ## 3, coeff3));                                      \
    dst = add(dst, mul(src ## 4, coeff4))

#define UPSAMPLE_L_COEFF()                                         \
c1      =   _mm_loadl_epi64((__m128i *)coeff);                      \
c1      =   _mm_unpacklo_epi8(c0,c1);                               \
c5      =   _mm_unpackhi_epi16(c0,c1);                              \
c1      =   _mm_unpacklo_epi16(c0,c1);                              \
c1      =   _mm_srai_epi32(c1,24);                                  \
c2      =   _mm_srli_si128(c1,4);                                   \
c3      =   _mm_srli_si128(c1,8);                                   \
c4      =   _mm_srli_si128(c1,12);                                  \
c5      =   _mm_srai_epi32(c5,24);                                  \
c6      =   _mm_srli_si128(c5,4);                                   \
c7      =   _mm_srli_si128(c5,8);                                   \
c8      =   _mm_srli_si128(c5,12);                                  \
c1      =   _mm_shuffle_epi32(c1,0);                                \
c2      =   _mm_shuffle_epi32(c2,0);                                \
c3      =   _mm_shuffle_epi32(c3,0);                                \
c4      =   _mm_shuffle_epi32(c4,0);                                \
c5      =   _mm_shuffle_epi32(c5,0);                                \
c6      =   _mm_shuffle_epi32(c6,0);                                \
c7      =   _mm_shuffle_epi32(c7,0);                                \
c8      =   _mm_shuffle_epi32(c8,0)

#define UPSAMPLE_L_LOAD(src1, src2, src3,src4)                                         \
m1= _mm_loadu_si128((__m128i*)(src1));                                                  \
m2= _mm_loadu_si128((__m128i*)(src2));                                                  \
m3= _mm_loadu_si128((__m128i*)(src3));                                                  \
m4= _mm_loadu_si128((__m128i*)(src4));

#define UPSAMPLE_L_COMPUTE_LO_V(result,coeff1,coeff2,coeff3,coeff4)               \
UNPACK_SRAI16_4(_mm_unpacklo_epi16, m, m);          \
MUL_ADD_V_4(_mm_mullo_epi32, _mm_add_epi32, result, m, coeff1,coeff2,coeff3,coeff4)


#define UPSAMPLE_L_COMPUTE_ALL_V(resultlo,resulthi,coeff1,coeff2,coeff3,coeff4)               \
UNPACK_SRAI16_4(_mm_unpackhi_epi16, t, m);          \
MUL_ADD_V_4(_mm_mullo_epi32, _mm_add_epi32, resulthi, t, coeff1,coeff2,coeff3,coeff4); \
UPSAMPLE_L_COMPUTE_LO_V(resultlo, coeff1,coeff2,coeff3,coeff4)

void ff_upsample_filter_block_luma_v_8_8_sse(uint8_t *dst, ptrdiff_t dststride, int16_t *_src, ptrdiff_t _srcstride,
                                      int y_BL, int x_EL, int y_EL, int block_w, int block_h, int widthEL, int heightEL,
                                      const struct HEVCWindow *Enhscal, struct UpsamplInf *up_info) {
    int topStartL  = Enhscal->top_offset;
    int bottomEndL = heightEL - Enhscal->bottom_offset;
    int rightEndL  = widthEL - Enhscal->right_offset;
    int leftStartL = Enhscal->left_offset;

    int y, i, j, phase, refPos16, refPos;
    int x;
    const int8_t* coeff;
    int16_t *   src_tmp;
    uint8_t * dst_tmp;
    int refPos0 = av_clip_c(y_EL, topStartL, bottomEndL-1);
    __m128i      m1, m2,m3,m4, m9, r0,r1,r2, r3,c0,c1,c2,c3,c4,c5,c6,c7,c8;
    __m128i     t1,t2,t3,t4;
    c0= _mm_setzero_si128();
    m9= _mm_set1_epi32(I_OFFSET);
    refPos0     = (((( refPos0 - topStartL )* up_info->scaleYLum + up_info->addYLum) >> 12) >> 4);

    if((rightEndL-2 - x_EL) <= block_w-1){
        x=rightEndL-2 - x_EL;
    }
    else
    {
        x= block_w-1;
    }

    for( j = 0; j < block_h; j++ )  {
        y        =   av_clip_c(y_EL+j, topStartL, bottomEndL-1);
        refPos16 = ((( y - topStartL )* up_info->scaleYLum + up_info->addYLum) >> 12);
        phase    = refPos16 & 15;
        coeff    = up_sample_filter_chroma[phase];
        refPos   = (refPos16 >> 4) -refPos0;
        src_tmp  = _src  + refPos  * _srcstride;
        dst_tmp  =  dst  + (y_EL+j)* dststride + x_EL;

        UPSAMPLE_L_COEFF();

       UPSAMPLE_L_LOAD(src_tmp- 3*_srcstride,src_tmp- 2*_srcstride,src_tmp- _srcstride,src_tmp);
       UPSAMPLE_L_COMPUTE_LO_V(r1,c1,c2,c3,c4);

       UPSAMPLE_L_LOAD(src_tmp + _srcstride,src_tmp + 2*_srcstride,src_tmp +  3*_srcstride,src_tmp +  4*_srcstride);
       UPSAMPLE_L_COMPUTE_LO_V(r0,c5,c6,c7,c8);

        r0= _mm_add_epi32(r0,r1);
        m1= _mm_add_epi32(r0,m9); //+ I_OFFSET

        m1= _mm_srli_epi32(m1,N_SHIFT); //shift

        m1= _mm_packus_epi32(m1,c0);
        m1= _mm_packus_epi16(m1,c0);
        m1= _mm_unpacklo_epi8(m1,m1);
        m1= _mm_unpacklo_epi16(m1,m1);
        m1= _mm_unpacklo_epi32(m1,m1);
        m1= _mm_unpacklo_epi64(m1,m1);

        for( i = 0; i < leftStartL - x_EL; i+=8 )  {
            _mm_storel_epi64((__m128i *) &dst_tmp[i],m1);
        }

        for( i; i <= x; i+=8 ){

            UPSAMPLE_L_LOAD(src_tmp- 3*_srcstride,src_tmp- 2*_srcstride,src_tmp- _srcstride,src_tmp);
            UPSAMPLE_L_COMPUTE_ALL_V(r0,r1,c1,c2,c3,c4);

            UPSAMPLE_L_LOAD(src_tmp + _srcstride,src_tmp + 2*_srcstride,src_tmp +  3*_srcstride,src_tmp +  4*_srcstride);
            UPSAMPLE_L_COMPUTE_ALL_V(r2,r3,c5,c6,c7,c8);

            r0= _mm_add_epi32(r0,r2);
            r1= _mm_add_epi32(r1,r3);

            r0= _mm_add_epi32(r0,m9); //+ I_OFFSET
            r1= _mm_add_epi32(r1,m9); //+ I_OFFSET
            r0= _mm_srli_epi32(r0,N_SHIFT); //shift
            r1= _mm_srli_epi32(r1,N_SHIFT); //shift
            m1= _mm_packus_epi32(r0,r1);
            m1= _mm_packus_epi16(m1,c0);

            _mm_storel_epi64((__m128i *) &dst_tmp[i],m1);
            src_tmp+=8;
        }

        UPSAMPLE_L_LOAD(src_tmp- 3*_srcstride,src_tmp- 2*_srcstride,src_tmp- _srcstride,src_tmp);
        UPSAMPLE_L_COMPUTE_LO_V(r1,c1,c2,c3,c4);

        UPSAMPLE_L_LOAD(src_tmp + _srcstride,src_tmp + 2*_srcstride,src_tmp +  3*_srcstride,src_tmp +  4*_srcstride);
        UPSAMPLE_L_COMPUTE_LO_V(r0,c5,c6,c7,c8);

        r0= _mm_add_epi32(r0,r1);
        m1= _mm_add_epi32(r0,m9); //+ I_OFFSET

        m1= _mm_srli_epi32(m1,N_SHIFT); //shift

        m1= _mm_packus_epi32(m1,c0);
        m1= _mm_packus_epi16(m1,c0);
        m1= _mm_unpacklo_epi8(m1,m1);
        m1= _mm_unpacklo_epi16(m1,m1);
        m1= _mm_unpacklo_epi32(m1,m1);
        m1= _mm_unpacklo_epi64(m1,m1);
        for(i;i < block_w; i+=8){
            _mm_storel_epi64((__m128i *) &dst_tmp[i],m1);
        }
    }
}



#undef LumHor_FILTER
#undef LumCro_FILTER
#undef LumVer_FILTER
#undef CroVer_FILTER
#endif
