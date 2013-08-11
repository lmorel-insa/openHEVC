/*
 * HEVC video Decoder
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


#ifndef AVCODEC_HEVC_FRAME_H
#define AVCODEC_HEVC_FRAME_H

typedef struct Mv {
    int16_t x;     ///< horizontal component of motion vector
    int16_t y;     ///< vertical component of motion vector
} Mv;




typedef struct MvField {
    Mv  mv[2];
    int8_t ref_idx[2];
    int8_t pred_flag[2];
    uint8_t is_intra;
} MvField;

typedef struct RefPicList {
    int list[16];
    int idx[16];
    int isLongTerm[16];
    int numPic;
} RefPicList;

typedef struct RefPicListTab {
    RefPicList refPicList[2];
} RefPicListTab;


typedef struct HEVCFrame {
    AVFrame *frame;
    int poc;
    MvField *tab_mvf;
    RefPicList *refPicList;
    RefPicListTab **refPicListTab;
    /**
     * A combination of HEVC_FRAME_FLAG_*
     */
    uint8_t flags;
    
    /**
     * A sequence counter, so that old frames are output first
     * after a POC reset
     */
    uint16_t sequence;
#ifdef SVC_EXTENSION
    uint16_t up_sample_base;
#endif
} HEVCFrame;


#endif // AVCODEC_HEVC_H
