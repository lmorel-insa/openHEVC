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

DECLARE_ALIGNED(16, static const int8_t, up_sample_filter_luma_x2_h_sse[4][16] )= /*0 , 8 */
{
    { 0,  0,  -1,   4,  0,  0,  -1,   4, 0,  0,  -1,   4, 0,  0,  -1,   4},
    { 0, 64, -11,  40,  0, 64, -11,  40, 0, 64, -11,  40, 0, 64, -11,  40},
    { 0,  0,  40, -11,  0,  0,  40, -11, 0,  0,  40, -11, 0,  0,  40, -11},
    { 0,   0, 4,   -1,  0,  0,   4,  -1, 0,  0,   4,  -1, 0,  0,   4,  -1}
};

DECLARE_ALIGNED(16, static const int16_t, up_sample_filter_luma_x2_v_sse[5][8] )= /*0 , 8 */
{
    {  0,   64,   0,  64,   0,  64,   0,  64},
    {  -1,   4,  -1,   4,  -1,   4,  -1,   4},
    { -11,  40, -11,  40, -11,  40, -11,  40},
    {  40, -11,  40, -11,  40, -11,  40, -11},
    {   4,  -1,   4,  -1,   4,  -1,   4,  -1}
};

DECLARE_ALIGNED(16, static const int8_t, up_sample_filter_chroma_x2_h_sse[2][16])=
{
    { 0, 64, -4, 36, 0, 64, -4, 36, 0, 64, -4, 36, 0, 64, -4, 36},
    { 0,  0, 36, -4, 0,  0, 36, -4, 0,  0, 36, -4, 0,  0, 36, -4}
};

DECLARE_ALIGNED(16, static const int16_t, up_sample_filter_chroma_x2_v_sse[4][8])=
{
        { -2, 10, -2, 10, -2, 10, -2, 10},
        { 58, -2, 58, -2, 58, -2, 58, -2},
        { -6, 46, -6, 46, -6, 46, -6, 46},
        { 28, -4, 28, -4, 28, -4, 28, -4}
};

void ff_upsample_filter_block_luma_h_ALL_sse(int16_t *dst, ptrdiff_t dststride, uint8_t *_src, ptrdiff_t _srcstride,
            int x_EL, int x_BL, int block_w, int block_h, int widthEL,
            const struct HEVCWindow *Enhscal, struct UpsamplInf *up_info){

}

void ff_upsample_filter_block_luma_v_ALL_sse(uint8_t *dst, ptrdiff_t dststride, int16_t *_src, ptrdiff_t _srcstride,
            int y_BL, int x_EL, int y_EL, int block_w, int block_h, int widthEL, int heightEL,
            const struct HEVCWindow *Enhscal, struct UpsamplInf *up_info){

}

void ff_upsample_filter_block_cr_h_ALL_sse(int16_t *dst, ptrdiff_t dststride, uint8_t *_src, ptrdiff_t _srcstride,
            int x_EL, int x_BL, int block_w, int block_h, int widthEL,
            const struct HEVCWindow *Enhscal, struct UpsamplInf *up_info){

}

void ff_upsample_filter_block_cr_v_ALL_sse(uint8_t *dst, ptrdiff_t dststride, int16_t *_src, ptrdiff_t _srcstride,
            int y_BL, int x_EL, int y_EL, int block_w, int block_h, int widthEL, int heightEL,
            const struct HEVCWindow *Enhscal, struct UpsamplInf *up_info){

}

void ff_upsample_filter_block_luma_h_X2_sse(int16_t *dst, ptrdiff_t dststride, uint8_t *_src, ptrdiff_t srcstride,
            int x_EL, int x_BL, int width, int height, int widthEL,
            const struct HEVCWindow *Enhscal, struct UpsamplInf *up_info){
    int x, y;
    __m128i x1, x2, x3, x4, x5, x6, x7, x8, r1, c1, c2, c3, c4;
    uint8_t  *src       = (uint8_t*) _src - x_BL;

    c1 = _mm_load_si128((__m128i *) up_sample_filter_luma_x2_h_sse[0]);
    c2 = _mm_load_si128((__m128i *) up_sample_filter_luma_x2_h_sse[1]);
    c3 = _mm_load_si128((__m128i *) up_sample_filter_luma_x2_h_sse[2]);
    c4 = _mm_load_si128((__m128i *) up_sample_filter_luma_x2_h_sse[3]);

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

void ff_upsample_filter_block_luma_v_X2_sse(uint8_t *_dst, ptrdiff_t _dststride, int16_t *_src, ptrdiff_t srcstride,
            int y_BL, int x_EL, int y_EL, int width, int height, int widthEL, int heightEL,
            const struct HEVCWindow *Enhscal, struct UpsamplInf *up_info){
    int x, y;
    __m128i x1, x2, x3, x4, x5, x6, x7, x8, x0, c1, c2, c3, c4, c5, add;
    int16_t  *src;
    uint8_t  *dst    = (uint8_t *)_dst + y_EL * _dststride + x_EL;
    uint8_t  shift = 12;
    add  = _mm_set1_epi32((1 << (shift - 1)));

    c1 = _mm_load_si128((__m128i *) up_sample_filter_luma_x2_v_sse[0]);
    c2 = _mm_load_si128((__m128i *) up_sample_filter_luma_x2_v_sse[1]);
    c3 = _mm_load_si128((__m128i *) up_sample_filter_luma_x2_v_sse[2]);
    c4 = _mm_load_si128((__m128i *) up_sample_filter_luma_x2_v_sse[3]);
    c5 = _mm_load_si128((__m128i *) up_sample_filter_luma_x2_v_sse[4]);

    for (y = 0; y < height; y++) {
        src  = _src  + (((y_EL+y)>>1) - y_BL)  * srcstride;
        for (x = 0; x < width; x += 8) {
            x1 = _mm_loadu_si128((__m128i *) &src[x - 3 * srcstride]);
            x2 = _mm_loadu_si128((__m128i *) &src[x - 2 * srcstride]);
            x3 = _mm_loadu_si128((__m128i *) &src[x -     srcstride]);
            x4 = _mm_loadu_si128((__m128i *) &src[x                ]);
            x5 = _mm_loadu_si128((__m128i *) &src[x +     srcstride]);
            x6 = _mm_loadu_si128((__m128i *) &src[x + 2 * srcstride]);
            x7 = _mm_loadu_si128((__m128i *) &src[x + 3 * srcstride]);
            x8 = _mm_loadu_si128((__m128i *) &src[x + 4 * srcstride]);

            x0 = x1;
            x1 = _mm_unpacklo_epi16(x0, x2);
            x2 = _mm_unpackhi_epi16(x0, x2);
            x0 = x3;
            x3 = _mm_unpacklo_epi16(x0, x4);
            x4 = _mm_unpackhi_epi16(x0, x4);
            x0 = x5;
            x5 = _mm_unpacklo_epi16(x0, x6);
            x6 = _mm_unpackhi_epi16(x0, x6);
            x0 = x7;
            x7 = _mm_unpacklo_epi16(x0, x8);
            x8 = _mm_unpackhi_epi16(x0, x8);

            if(!(y&0x1)) {
                x3 = _mm_madd_epi16(x3,c1);
                x4 = _mm_madd_epi16(x4,c1);

                x1 = x3;
                x3 = _mm_setzero_si128();
                x2 =  x4;
                x4 = x3;
            } else {
                x1 = _mm_madd_epi16(x1,c2);
                x3 = _mm_madd_epi16(x3,c3);
                x5 = _mm_madd_epi16(x5,c4);
                x7 = _mm_madd_epi16(x7,c5);
                x2 = _mm_madd_epi16(x2,c2);
                x4 = _mm_madd_epi16(x4,c3);
                x6 = _mm_madd_epi16(x6,c4);
                x8 = _mm_madd_epi16(x8,c5);

                x1 = _mm_add_epi32(x1, x3);
                x3 = _mm_add_epi32(x5, x7);
                x2 = _mm_add_epi32(x2, x4);
                x4 = _mm_add_epi32(x6, x8);
            }
            x8 = _mm_add_epi32(x1, x3);
            x7 = _mm_add_epi32(x2, x4);
            x8 = _mm_add_epi32(x8, add);
            x7 = _mm_add_epi32(x7, add);

            x8 = _mm_srai_epi32(x8, shift);
            x7 = _mm_srai_epi32(x7, shift);

            x8 = _mm_packus_epi32(x8, x7);
            x8 = _mm_packus_epi16(x8, x8);

            _mm_storel_epi64((__m128i *) &dst[x], x8);
        }
        dst += _dststride;
    }
}

void ff_upsample_filter_block_cr_h_X2_sse(int16_t *dst, ptrdiff_t dststride, uint8_t *_src, ptrdiff_t _srcstride,
            int x_EL, int x_BL, int width, int height, int widthEL,
            const struct HEVCWindow *Enhscal, struct UpsamplInf *up_info) {
    int x, y;
    __m128i x1, x2, x3, x4, f1, f2, r1;
    uint8_t  *src = ((uint8_t*) _src) - x_BL;
    ptrdiff_t srcstride = _srcstride;

    f1 = _mm_load_si128((__m128i *) up_sample_filter_chroma_x2_h_sse[0]);
    f2 = _mm_load_si128((__m128i *) up_sample_filter_chroma_x2_h_sse[1]);

    for (y = 0; y < height; y++) {
        for (x = 0; x < width; x += 8) {
            int e = (x+x_EL)>>1;
                x1 = _mm_loadu_si128((__m128i *) &src[e - 1]);
                x2 = _mm_loadu_si128((__m128i *) &src[e ]);
                x3 = _mm_loadu_si128((__m128i *) &src[e + 1]);
                x4 = _mm_loadu_si128((__m128i *) &src[e + 2]);

                x1 = _mm_unpacklo_epi8(x1, x1);
                x2 = _mm_unpacklo_epi8(x2, x2);
                x3 = _mm_unpacklo_epi8(x3, x3);
                x4 = _mm_unpacklo_epi8(x4, x4);

                x1 = _mm_unpacklo_epi8(x1, x2);
                x2 = _mm_unpacklo_epi8(x3, x4);

                x2 = _mm_maddubs_epi16(x2,f2);
                x1 = _mm_maddubs_epi16(x1,f1);

                r1 = _mm_add_epi16(x1, x2);
                _mm_store_si128((__m128i *) &dst[x], r1);
            }
            src += srcstride;
            dst += dststride;
        }
}

void ff_upsample_filter_block_cr_v_X2_sse(uint8_t *_dst, ptrdiff_t dststride, int16_t *_src, ptrdiff_t srcstride,
		int y_BL, int x_EL, int y_EL, int width, int height, int widthEL, int heightEL,
		const struct HEVCWindow *Enhscal, struct UpsamplInf *up_info){
    int x, y;
    __m128i x1, x2, x3, x4, x0, c1, c2, c3, c4, add;
    int16_t  *src;
    uint8_t  *dst    = (uint8_t *)_dst + y_EL * dststride + x_EL;
    uint8_t  shift = 12;
    add  = _mm_set1_epi32((1 << (shift - 1)));

    c1 = _mm_load_si128((__m128i *) up_sample_filter_chroma_x2_v_sse[0]);
    c2 = _mm_load_si128((__m128i *) up_sample_filter_chroma_x2_v_sse[1]);
    c3 = _mm_load_si128((__m128i *) up_sample_filter_chroma_x2_v_sse[2]);
    c4 = _mm_load_si128((__m128i *) up_sample_filter_chroma_x2_v_sse[3]);

    for (y = 0; y < height; y++) {
        int refPos16 = ( ((y+y_EL) * up_info->scaleYCr + up_info->addYCr) >> 12)-4;
        src  = _src  + ((refPos16>>4) - y_BL)  * srcstride;
        for (x = 0; x < width; x += 8) {
            x1 = _mm_loadu_si128((__m128i *) &src[x -     srcstride]);
            x2 = _mm_loadu_si128((__m128i *) &src[x                ]);
            x3 = _mm_loadu_si128((__m128i *) &src[x +     srcstride]);
            x4 = _mm_loadu_si128((__m128i *) &src[x + 2 * srcstride]);

            x0 = x1;
            x1 = _mm_unpacklo_epi16(x0, x2);
            x2 = _mm_unpackhi_epi16(x0, x2);
            x0 = x3;
            x3 = _mm_unpacklo_epi16(x0, x4);
            x4 = _mm_unpackhi_epi16(x0, x4);

            if(!((y+y_EL)&0x1)) {
                x1 = _mm_madd_epi16(x1,c1);
                x3 = _mm_madd_epi16(x3,c2);
                x2 = _mm_madd_epi16(x2,c1);
                x4 = _mm_madd_epi16(x4,c2);
            } else {
                x1 = _mm_madd_epi16(x1,c3);
                x3 = _mm_madd_epi16(x3,c4);
                x2 = _mm_madd_epi16(x2,c3);
                x4 = _mm_madd_epi16(x4,c4);
            }
            x1 = _mm_add_epi32(x1, x3);
            x2 = _mm_add_epi32(x2, x4);

            x1 = _mm_add_epi32(x1, add);
            x2 = _mm_add_epi32(x2, add);

            x1 = _mm_srai_epi32(x1, shift);
            x2 = _mm_srai_epi32(x2, shift);

            x1 = _mm_packus_epi32(x1, x2);
            x1 = _mm_packus_epi16(x1, x1);
            _mm_storel_epi64((__m128i *) &dst[x], x1);
        }
        dst += dststride;
    }
}

void ff_upsample_filter_block_luma_h_X1_5_sse(int16_t *dst, ptrdiff_t dststride, uint8_t *_src, ptrdiff_t _srcstride,
            int x_EL, int x_BL, int block_w, int block_h, int widthEL,
            const struct HEVCWindow *Enhscal, struct UpsamplInf *up_info){

}

void ff_upsample_filter_block_luma_v_X1_5_sse(uint8_t *dst, ptrdiff_t dststride, int16_t *_src, ptrdiff_t _srcstride,
            int y_BL, int x_EL, int y_EL, int block_w, int block_h, int widthEL, int heightEL,
            const struct HEVCWindow *Enhscal, struct UpsamplInf *up_info){

}

void ff_upsample_filter_block_cr_h_X1_5_sse(int16_t *dst, ptrdiff_t dststride, uint8_t *_src, ptrdiff_t _srcstride,
            int x_EL, int x_BL, int block_w, int block_h, int widthEL,
            const struct HEVCWindow *Enhscal, struct UpsamplInf *up_info){

}

void ff_upsample_filter_block_cr_v_X1_5_sse(uint8_t *dst, ptrdiff_t dststride, int16_t *_src, ptrdiff_t _srcstride,
		int y_BL, int x_EL, int y_EL, int block_w, int block_h, int widthEL, int heightEL,
		const struct HEVCWindow *Enhscal, struct UpsamplInf *up_info){
}
