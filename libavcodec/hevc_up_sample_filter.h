/*
 * HEVC data tables
 *
 * Copyright (C) 2012 Guillaume Martres
 *
 * This file is part of Libav.
 *
 * Libav is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * Libav is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with Libav; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef AVCODEC_HEVC_UP_SAMPLE_FILTER_H
#define AVCODEC_HEVC_UP_SAMPLE_FILTER_H

#include "hevc.h"
//#ifdef SVC_EXTENSION
#define NTAPS_LUMA 8
#define NTAPS_CHROMA 4
#define US_FILTER_PREC  6

#define MAX_EDGE  4
#define MAX_EDGE_CR  2
#define N_SHIFT (20-8)
#define I_OFFSET (1 << (N_SHIFT - 1))



//#endif



    /*
DECLARE_ALIGNED(16, static const int8_t, up_sample_filter_luma[16][NTAPS_LUMA] )=
{
    {  0,   0,   0, 64,   0,   0,   0,   0 },
    { -1,  -1,  -1, -1,  -1,  -1,  -1,  -1 },
    { -1,  -1,  -1, -1,  -1,  -1,  -1,  -1 },
    { -1,  -1,  -1, -1,  -1,  -1,  -1,  -1 },
    { -1,  -1,  -1, -1,  -1,  -1,  -1,  -1 },
    { -1,   4, -11, 52,  26,  -8,   3,  -1 },
    { -1,  -1,  -1, -1,  -1,  -1,  -1,  -1 },
    { -1,  -1,  -1, -1,  -1,  -1,  -1,  -1 },
    { -1,  4,  -11, 40,  40,  -11,  4,  -1 },
    { -1,  -1,  -1, -1,  -1,  -1,  -1,  -1 },
    { -1,  -1,  -1, -1,  -1,  -1,  -1,  -1 },
    { -1,   3,  -8, 26,  52, -11,   4,  -1 },
    { -1,  -1,  -1, -1,  -1,  -1,  -1,  -1 },
    { -1,  -1,  -1, -1,  -1,  -1,  -1,  -1 },
    { -1,  -1,  -1, -1,  -1,  -1,  -1,  -1 },
    { -1,  -1,  -1, -1,  -1,  -1,  -1,  -1 }
};
#else
DECLARE_ALIGNED(16, static const int32_t, up_sample_filter_luma[12][NTAPS_LUMA] )=
{
    {  0,   0,   0, 64,   0,   0,   0,   0 },
    { -1,  -1,  -1, -1,  -1,  -1,  -1,  -1 },
    { -1,  -1,  -1, -1,  -1,  -1,  -1,  -1 },
    { -1,  -1,  -1, -1,  -1,  -1,  -1,  -1 },
    { -1,   4, -11, 52,  26,  -8,   3,  -1 },
    { -1,  -1,  -1, -1,  -1,  -1,  -1,  -1 },
    { -1,  4,  -11, 40,  40,  -11,  4,  -1 },
    { -1,  -1,  -1, -1,  -1,  -1,  -1,  -1 },
    { -1,   3,  -8, 26,  52, -11,   4,  -1 },
    { -1,  -1,  -1, -1,  -1,  -1,  -1,  -1 },
    { -1,  -1,  -1, -1,  -1,  -1,  -1,  -1 },
    { -1,  -1,  -1, -1,  -1,  -1,  -1,  -1 }
};
*/



DECLARE_ALIGNED(16, static const int32_t, up_sample_filter_chroma32[16][NTAPS_CHROMA])=
{
    {  0,  64,   0,  0},//
    { -2,  62,   4,  0},
    { -2,  58,  10, -2},
    { -4,  56,  14, -2},
    { -4,  54,  16, -2},// <-> actual phase shift 1/4,equal to HEVC MC, used for spatial scalability x1.5 (only for accurate Chroma alignement)
    { -6,  52,  20, -2},// <-> actual phase shift 1/3, used for spatial scalability x1.5
    { -6,  46,  28, -4},// <-> actual phase shift 3/8,equal to HEVC MC, used for spatial scalability x2 (only for accurate Chroma alignement)
    { -4,  42,  30, -4},
    { -4,  36,  36, -4},// <-> actual phase shift 1/2,equal to HEVC MC, used for spatial scalability x2
    { -4,  30,  42, -4},// <-> actual phase shift 7/12, used for spatial scalability x1.5 (only for accurate Chroma alignement)
    { -4,  28,  46, -6},
    { -2,  20,  52, -6},// <-> actual phase shift 2/3, used for spatial scalability x1.5
    { -2,  16,  54, -4},
    { -2,  14,  56, -4},
    { -2,  10,  58, -2},// <-> actual phase shift 7/8,equal to HEVC MC, used for spatial scalability x2 (only for accurate Chroma alignement)
    {  0,   4,  62, -2} // <-> actual phase shift 11/12, used for spatial scalability x1.5 (only for accurate Chroma alignement)
};

DECLARE_ALIGNED(16, static const int32_t, up_sample_filter_luma32[16][NTAPS_LUMA] )=
{
    {  0,  0,   0,  64,   0,   0,  0,  0},
    {  0,  1,  -3,  63,   4,  -2,  1,  0},
    { -1,  2,  -5,  62,   8,  -3,  1,  0},
    { -1,  3,  -8,  60,  13,  -4,  1,  0},
    { -1,  4, -10,  58,  17,  -5,  1,  0},
    { -1,  4, -11,  52,  26,  -8,  3, -1}, // <-> actual phase shift 1/3, used for spatial scalability x1.5
    { -1,  3,  -9,  47,  31, -10,  4, -1},
    { -1,  4, -11,  45,  34, -10,  4, -1},
    { -1,  4, -11,  40,  40, -11,  4, -1}, // <-> actual phase shift 1/2, equal to HEVC MC, used for spatial scalability x2
    { -1,  4, -10,  34,  45, -11,  4, -1},
    { -1,  4, -10,  31,  47,  -9,  3, -1},
    { -1,  3,  -8,  26,  52, -11,  4, -1}, // <-> actual phase shift 2/3, used for spatial scalability x1.5
    {  0,  1,  -5,  17,  58, -10,  4, -1},
    {  0,  1,  -4,  13,  60,  -8,  3, -1},
    {  0,  1,  -3,   8,  62,  -5,  2, -1},
    {  0,  1,  -2,   4,  63,  -3,  1,  0}
};

#endif /* AVCODEC_HEVC_UP_SAMPLE_FILTER_H */
