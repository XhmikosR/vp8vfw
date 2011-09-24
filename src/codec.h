/*****************************************************************************
 *
 * VP80 VFW FRONTEND based on XVid 1.2.2 VFW Codec 
 *  - VFW codec header  -
 *
 *  Copyright(C) 2002-2003 Anonymous <xvid-devel@xvid.org>
 *
 *  This program is free software ; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation ; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY ; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program ; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 * $Id: codec.h,v 1.6 2008/11/26 10:11:16 Isibaar Exp $
 *
 ****************************************************************************/
#ifndef _CODEC_H_
#define _CODEC_H_

#include <vfw.h>
#include "config.h"
#include "status.h"

#define VP80_NAME_L		L"VP80"
#define VP80_DESC_L		L"Google VP8 Video Codec"


#define FOURCC_VP80	mmioFOURCC('V','P','8','0')
/* yuyu		4:2:2 16bit, y-u-y-v, packed*/
#define FOURCC_YUYV	mmioFOURCC('Y','U','Y','V')
#define FOURCC_YUY2	mmioFOURCC('Y','U','Y','2')
/* yvyu		4:2:2 16bit, y-v-y-u, packed*/
#define FOURCC_YVYU	mmioFOURCC('Y','V','Y','U')
/* uyvy		4:2:2 16bit, u-y-v-y, packed */
#define FOURCC_UYVY	mmioFOURCC('U','Y','V','Y')
/* i420		y-u-v, planar */
#define FOURCC_I420	mmioFOURCC('I','4','2','0')
#define FOURCC_IYUV	mmioFOURCC('I','Y','U','V')
/* yv12		y-v-u, planar */
#define FOURCC_YV12	mmioFOURCC('Y','V','1','2')

#define vpx_encoder_interface (&vpx_codec_vp8_cx_algo)
#define vpx_decoder_interface (&vpx_codec_vp8_dx_algo)


typedef struct
{
	CONFIG config;

	// decoder
	vpx_codec_ctx_t      *dhandle;

	// encoder
	vpx_codec_ctx_t      *ehandle;
    stats_io_t           stats;
	vpx_image_t          frame;
	unsigned int fincr;
	unsigned int fbase;
    status_t status;
    
    /* encoder min keyframe internal */
	int framenum;  
	int framecount;
	int keyspacing;

	HINSTANCE m_hdll;

} CODEC;



LRESULT compress_query(CODEC *, BITMAPINFO *, BITMAPINFO *);
LRESULT compress_get_format(CODEC *, BITMAPINFO *, BITMAPINFO *);
LRESULT compress_get_size(CODEC *, BITMAPINFO *, BITMAPINFO *);
LRESULT compress_frames_info(CODEC *, ICCOMPRESSFRAMES *);
LRESULT compress_begin(CODEC *, BITMAPINFO *, BITMAPINFO *);
LRESULT compress_end(CODEC *);
LRESULT compress(CODEC *, ICCOMPRESS *);

LRESULT decompress_query(CODEC *, BITMAPINFO *, BITMAPINFO *);
LRESULT decompress_get_format(CODEC *, BITMAPINFO *, BITMAPINFO *);
LRESULT decompress_begin(CODEC *, BITMAPINFO *, BITMAPINFO *);
LRESULT decompress_end(CODEC *);
LRESULT decompress(CODEC *, ICDECOMPRESS *);
void codec_init(void);

extern int pp_brightness, pp_dy, pp_duv, pp_fe, pp_dry, pp_druv; /* decoder options */

#endif /* _CODEC_H_ */
