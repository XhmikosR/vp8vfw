/**************************************************************************
 *
 * VP80 VFW FRONTEND based on XVid 1.2.2 VFW Codec 
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *************************************************************************/

/**************************************************************************
 *
 * History:
 *
 *
 *************************************************************************/

#include <windows.h>
#include <vfw.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include "stdint.h"
#include "colorspace.h"
#include "vfwext.h"

#include <vpx/vpx_codec.h>
#include <vpx/vp8.h>
#include <vpx/vpx_encoder.h>
#include <vpx/vp8cx.h>
#include <vpx/vpx_decoder.h>
#include <vpx/vp8dx.h>


#include "debug.h"
#include "stats.h"
#include "codec.h"
#include "status.h"


extern int check_cpu_features(void);

#define VP80_CPU_MMX      (1<< 0) /*       mmx : pentiumMMX,k6 */


void codec_init(void)
{

	/* Initialize internal colorspace transformation tables */
	colorspace_init();

	/* All colorspace transformation functions User Format->YV12 */
	yv12_to_yv12    = yv12_to_yv12_c;
	rgb555_to_yv12  = rgb555_to_yv12_c;
	rgb565_to_yv12  = rgb565_to_yv12_c;
	rgb_to_yv12     = rgb_to_yv12_c;
	bgr_to_yv12     = bgr_to_yv12_c;
	bgra_to_yv12    = bgra_to_yv12_c;
	abgr_to_yv12    = abgr_to_yv12_c;
	rgba_to_yv12    = rgba_to_yv12_c;
	argb_to_yv12    = argb_to_yv12_c;
	yuyv_to_yv12    = yuyv_to_yv12_c;
	uyvy_to_yv12    = uyvy_to_yv12_c;

	rgb555i_to_yv12 = rgb555i_to_yv12_c;
	rgb565i_to_yv12 = rgb565i_to_yv12_c;
	bgri_to_yv12    = bgri_to_yv12_c;
	bgrai_to_yv12   = bgrai_to_yv12_c;
	abgri_to_yv12   = abgri_to_yv12_c;
	rgbai_to_yv12   = rgbai_to_yv12_c;
	argbi_to_yv12   = argbi_to_yv12_c;
	yuyvi_to_yv12   = yuyvi_to_yv12_c;
	uyvyi_to_yv12   = uyvyi_to_yv12_c;

	/* All colorspace transformation functions YV12->User format */
	yv12_to_rgb555  = yv12_to_rgb555_c;
	yv12_to_rgb565  = yv12_to_rgb565_c;
	yv12_to_rgb     = yv12_to_rgb_c;
	yv12_to_bgr     = yv12_to_bgr_c;
	yv12_to_bgra    = yv12_to_bgra_c;
	yv12_to_abgr    = yv12_to_abgr_c;
	yv12_to_rgba    = yv12_to_rgba_c;
	yv12_to_argb    = yv12_to_argb_c;
	yv12_to_yuyv    = yv12_to_yuyv_c;
	yv12_to_uyvy    = yv12_to_uyvy_c;

	yv12_to_rgb555i = yv12_to_rgb555i_c;
	yv12_to_rgb565i = yv12_to_rgb565i_c;
	yv12_to_bgri    = yv12_to_bgri_c;
	yv12_to_bgrai   = yv12_to_bgrai_c;
	yv12_to_abgri   = yv12_to_abgri_c;
	yv12_to_rgbai   = yv12_to_rgbai_c;
	yv12_to_argbi   = yv12_to_argbi_c;
	yv12_to_yuyvi   = yv12_to_yuyvi_c;
	yv12_to_uyvyi   = yv12_to_uyvyi_c;

}

/*
  perform safe packed colorspace conversion, by splitting
  the image up into an optimized area (pixel width divisible by 16),
  and two unoptimized/plain-c areas (pixel width divisible by 2)
*/

static void
safe_packed_conv(uint8_t * x_ptr, int x_stride,
             uint8_t * y_ptr, uint8_t * u_ptr, uint8_t * v_ptr,
             int y_stride, int uv_stride,
             int width, int height, int vflip,
             packedFunc * func_opt, packedFunc func_c,
                 int size, int interlacing)
{
   int width_opt, width_c, height_opt;

    if (width<0 || width==1 || height==1) return; /* forget about it */

   if (func_opt != func_c && x_stride < size*((width+15)/16)*16)
   {
      width_opt = width & (~15);
      width_c = (width - width_opt) & (~1);
   }
   else if (func_opt != func_c && !(width&1) && (size==3))
   {
        /* MMX reads 4 bytes per pixel for RGB/BGR */
        width_opt = width - 2;
        width_c = 2;
    }
    else {
        /* Enforce the width to be divisable by two. */
      width_opt = width & (~1);
      width_c = 0;
   }

    /* packed conversions require height to be divisable by 2
       (or even by 4 for interlaced conversion) */
    if (interlacing)
        height_opt = height & (~3);
    else
        height_opt = height & (~1);

   func_opt(x_ptr, x_stride,
         y_ptr, u_ptr, v_ptr, y_stride, uv_stride,
         width_opt, height_opt, vflip);

   if (width_c)
   {
      func_c(x_ptr + size*width_opt, x_stride,
         y_ptr + width_opt, u_ptr + width_opt/2, v_ptr + width_opt/2,
         y_stride, uv_stride, width_c, height_opt, vflip);
   }
}


static int convert_output_color_space(
             vpx_image_t * image,
		     uint32_t width,
			 int height,
			 uint32_t edged_width,
			 uint8_t * dst[4],
			 int dst_stride[4],
			 vpx_img_fmt_t csp,
			 int interlacing,
			 int flip)
{
	const int edged_width2 = edged_width/2;
	int height2 = height/2;

	switch (csp) {
	case VPX_IMG_FMT_RGB555:
		safe_packed_conv(
			dst[0], dst_stride[0], image->planes[VPX_PLANE_Y], image->planes[VPX_PLANE_U], image->planes[VPX_PLANE_V],
			edged_width, edged_width2, width, height, flip,
			interlacing?yv12_to_rgb555i  :yv12_to_rgb555,
			interlacing?yv12_to_rgb555i_c:yv12_to_rgb555_c, 2, interlacing);
		return 0;

	case VPX_IMG_FMT_RGB565:
		safe_packed_conv(
			dst[0], dst_stride[0],image->planes[VPX_PLANE_Y], image->planes[VPX_PLANE_U], image->planes[VPX_PLANE_V],
			edged_width, edged_width2, width, height, flip,
			interlacing?yv12_to_rgb565i  :yv12_to_rgb565,
			interlacing?yv12_to_rgb565i_c:yv12_to_rgb565_c, 2, interlacing);
		return 0;

    case VPX_IMG_FMT_BGR24:
		safe_packed_conv(
			dst[0], dst_stride[0], image->planes[VPX_PLANE_Y], image->planes[VPX_PLANE_U], image->planes[VPX_PLANE_V],
			edged_width, edged_width2, width, height, flip,
			interlacing?yv12_to_bgri  :yv12_to_bgr,
			interlacing?yv12_to_bgri_c:yv12_to_bgr_c, 3, interlacing);
		return 0;

   case VPX_IMG_FMT_ARGB_LE:
   case VPX_IMG_FMT_RGB32_LE:
		safe_packed_conv(
			dst[0], dst_stride[0], image->planes[VPX_PLANE_Y], image->planes[VPX_PLANE_U], image->planes[VPX_PLANE_V],
			edged_width, edged_width2, width, height, flip,
			interlacing?yv12_to_bgrai  :yv12_to_bgra,
			interlacing?yv12_to_bgrai_c:yv12_to_bgra_c, 4, interlacing);
		return 0;
/*
	case XVID_CSP_ABGR:
		safe_packed_conv(
			dst[0], dst_stride[0], image->y, image->u, image->v,
			edged_width, edged_width2, width, height, flip,
			interlacing?yv12_to_abgri  :yv12_to_abgr,
			interlacing?yv12_to_abgri_c:yv12_to_abgr_c, 4, interlacing);
		return 0;
*/
   case VPX_IMG_FMT_RGB24:
		safe_packed_conv(
			dst[0], dst_stride[0], image->planes[VPX_PLANE_Y], image->planes[VPX_PLANE_U], image->planes[VPX_PLANE_V],
			edged_width, edged_width2, width, height, flip,
			interlacing?yv12_to_rgbi  :yv12_to_rgb,
			interlacing?yv12_to_rgbi_c:yv12_to_rgb_c, 3, interlacing);
		return 0;

/*
	case XVID_CSP_RGBA:
		safe_packed_conv(
			dst[0], dst_stride[0], image->y, image->u, image->v,
			edged_width, edged_width2, width, height, flip,
			interlacing?yv12_to_rgbai  :yv12_to_rgba,
			interlacing?yv12_to_rgbai_c:yv12_to_rgba_c, 4, interlacing);
		return 0;
*/
	case VPX_IMG_FMT_ARGB:
		safe_packed_conv(
			dst[0], dst_stride[0], image->planes[VPX_PLANE_Y], image->planes[VPX_PLANE_U], image->planes[VPX_PLANE_V],
			edged_width, edged_width2, width, height, flip,
			interlacing?yv12_to_argbi  :yv12_to_argb,
			interlacing?yv12_to_argbi_c:yv12_to_argb_c, 4, interlacing);
		return 0;

	case VPX_IMG_FMT_YUY2:
		safe_packed_conv(
			dst[0], dst_stride[0], image->planes[VPX_PLANE_Y], image->planes[VPX_PLANE_U], image->planes[VPX_PLANE_V],
			edged_width, edged_width2, width, height, flip,
			interlacing?yv12_to_yuyvi  :yv12_to_yuyv,
			interlacing?yv12_to_yuyvi_c:yv12_to_yuyv_c, 2, interlacing);
		return 0;

	case VPX_IMG_FMT_YVYU:		/* u,v swapped */
		safe_packed_conv(
			dst[0], dst_stride[0], image->planes[VPX_PLANE_Y], image->planes[VPX_PLANE_V], image->planes[VPX_PLANE_U],
			edged_width, edged_width2, width, height, flip,
			interlacing?yv12_to_yuyvi  :yv12_to_yuyv,
			interlacing?yv12_to_yuyvi_c:yv12_to_yuyv_c, 2, interlacing);
		return 0;

	case VPX_IMG_FMT_UYVY:
		safe_packed_conv(
			dst[0], dst_stride[0], image->planes[VPX_PLANE_Y], image->planes[VPX_PLANE_U], image->planes[VPX_PLANE_V],
			edged_width, edged_width2, width, height, flip,
			interlacing?yv12_to_uyvyi  :yv12_to_uyvy,
			interlacing?yv12_to_uyvyi_c:yv12_to_uyvy_c, 2, interlacing);
		return 0;

	case VPX_IMG_FMT_I420: /* YCbCr == YUV == internal colorspace for MPEG */
		yv12_to_yv12(dst[0], dst[0] + dst_stride[0]*height, dst[0] + dst_stride[0]*height + (dst_stride[0]/2)*height2,
			dst_stride[0], dst_stride[0]/2,
			image->planes[VPX_PLANE_Y], image->planes[VPX_PLANE_U], image->planes[VPX_PLANE_V], edged_width, edged_width2,
			width, height, flip);
		return 0;

	case VPX_IMG_FMT_YV12:	/* YCrCb == YVU == U and V plane swapped */
		yv12_to_yv12(dst[0], dst[0] + dst_stride[0]*height, dst[0] + dst_stride[0]*height + (dst_stride[0]/2)*height2,
			dst_stride[0], dst_stride[0]/2,
			image->planes[VPX_PLANE_Y], image->planes[VPX_PLANE_V], image->planes[VPX_PLANE_U], edged_width, edged_width2,
			width, height, flip);
		return 0;

	}

	return -1;
}


/** Used to align a value to 16-byte alignment.
    Returns either the original value or the 16-byte 
	align if alignment is required. */
static align16(int value)
{
/*  if (value & 15)
  {
    return (value + (16 - (value&15)));
  } else
  {*/
	// Currently unused, libxvpx takes care of pixel alignment issues.
	return value;
/*  }*/

}

/* Convert color space to YV12, taken from the XVID library */
static int convert_input_color_space(vpx_image_t * image,
         uint32_t width,
         int height,
         uint32_t edged_width,
         uint8_t * src[4],
         int src_stride[4],
         vpx_img_fmt_t csp,
         int interlacing,
         int flip
         )
{
   const int edged_width2 = edged_width/2;
   const int width2 = width/2;
   const int height2 = height/2;

   switch (csp) {
   case VPX_IMG_FMT_RGB555:
      safe_packed_conv(
         src[0], src_stride[0], image->planes[VPX_PLANE_Y], image->planes[VPX_PLANE_U], image->planes[VPX_PLANE_V],
         edged_width, edged_width2, width, height, flip,
         interlacing?rgb555i_to_yv12  :rgb555_to_yv12,
         interlacing?rgb555i_to_yv12_c:rgb555_to_yv12_c, 2, interlacing);
      break;

   case VPX_IMG_FMT_RGB565:
      safe_packed_conv(
         src[0], src_stride[0], image->planes[VPX_PLANE_Y], image->planes[VPX_PLANE_U], image->planes[VPX_PLANE_V],
         edged_width, edged_width2, width, height, flip,
         interlacing?rgb565i_to_yv12  :rgb565_to_yv12,
         interlacing?rgb565i_to_yv12_c:rgb565_to_yv12_c, 2, interlacing);
      break;


   case VPX_IMG_FMT_BGR24:
      safe_packed_conv(
         src[0], src_stride[0], image->planes[VPX_PLANE_Y], image->planes[VPX_PLANE_U], image->planes[VPX_PLANE_V],
         edged_width, edged_width2, width, height, flip,
         interlacing?bgri_to_yv12  :bgr_to_yv12,
         interlacing?bgri_to_yv12_c:bgr_to_yv12_c, 3, interlacing);
      break;

   case VPX_IMG_FMT_ARGB_LE:
   case VPX_IMG_FMT_RGB32_LE:
      safe_packed_conv(
         src[0], src_stride[0], image->planes[VPX_PLANE_Y], image->planes[VPX_PLANE_U], image->planes[VPX_PLANE_V],
         edged_width, edged_width2, width, height, flip,
         interlacing?bgrai_to_yv12  :bgra_to_yv12,
         interlacing?bgrai_to_yv12_c:bgra_to_yv12_c, 4, interlacing);
      break;

/* case XVID_CSP_ABGR :
      safe_packed_conv(
         src[0], src_stride[0], image->y, image->u, image->v,
         edged_width, edged_width2, width, height, flip,
         interlacing?abgri_to_yv12  :abgr_to_yv12,
         interlacing?abgri_to_yv12_c:abgr_to_yv12_c, 4, interlacing);
      break;*/

   case VPX_IMG_FMT_RGB24:
      safe_packed_conv(
         src[0], src_stride[0], image->planes[VPX_PLANE_Y], image->planes[VPX_PLANE_U], image->planes[VPX_PLANE_V],
         edged_width, edged_width2, width, height, flip,
         interlacing?rgbi_to_yv12  :rgb_to_yv12,
         interlacing?rgbi_to_yv12_c:rgb_to_yv12_c, 3, interlacing);
      break;
/*
   case XVID_CSP_RGBA :
      safe_packed_conv(
         src[0], src_stride[0], image->y, image->u, image->v,
         edged_width, edged_width2, width, height, flip,
         interlacing?rgbai_to_yv12  :rgba_to_yv12,
         interlacing?rgbai_to_yv12_c:rgba_to_yv12_c, 4, interlacing);
      break;
*/
   case VPX_IMG_FMT_ARGB:
      safe_packed_conv(
         src[0], src_stride[0], image->planes[VPX_PLANE_Y], image->planes[VPX_PLANE_U], image->planes[VPX_PLANE_V],
         edged_width, edged_width2, width, height, flip,
         interlacing?argbi_to_yv12  : argb_to_yv12,
         interlacing?argbi_to_yv12_c: argb_to_yv12_c, 4, interlacing);
      break;

   case VPX_IMG_FMT_YUY2:
      safe_packed_conv(
         src[0], src_stride[0], image->planes[VPX_PLANE_Y], image->planes[VPX_PLANE_U], image->planes[VPX_PLANE_V],
         edged_width, edged_width2, width, height, flip,
         interlacing?yuyvi_to_yv12  :yuyv_to_yv12,
         interlacing?yuyvi_to_yv12_c:yuyv_to_yv12_c, 2, interlacing);
      break;

   case VPX_IMG_FMT_YVYU:     /* u/v swapped */
      safe_packed_conv(
         src[0], src_stride[0], image->planes[VPX_PLANE_Y], image->planes[VPX_PLANE_V], image->planes[VPX_PLANE_U],
         edged_width, edged_width2, width, height, flip,
         interlacing?yuyvi_to_yv12  :yuyv_to_yv12,
         interlacing?yuyvi_to_yv12_c:yuyv_to_yv12_c, 2, interlacing);
      break;

   case VPX_IMG_FMT_UYVY:
      safe_packed_conv(
         src[0], src_stride[0], image->planes[VPX_PLANE_Y], image->planes[VPX_PLANE_U], image->planes[VPX_PLANE_V],
         edged_width, edged_width2, width, height, flip,
         interlacing?uyvyi_to_yv12  :uyvy_to_yv12,
         interlacing?uyvyi_to_yv12_c:uyvy_to_yv12_c, 2, interlacing);
      break;

   case VPX_IMG_FMT_I420:  /* YCbCr == YUV == internal colorspace for MPEG */
    yv12_to_yv12(image->planes[VPX_PLANE_Y], image->planes[VPX_PLANE_U], image->planes[VPX_PLANE_V], edged_width, edged_width2,
       src[0], src[0] + src_stride[0]*height, src[0] + src_stride[0]*height + (src_stride[0]/2)*height2,
       src_stride[0], src_stride[0]/2, width, height, flip);
      break;

   case VPX_IMG_FMT_YV12: /* YCrCb == YVA == U and V plane swapped */
    yv12_to_yv12(image->planes[VPX_PLANE_Y], image->planes[VPX_PLANE_V], image->planes[VPX_PLANE_U],edged_width, edged_width2,
       src[0], src[0] + src_stride[0]*height, src[0] + src_stride[0]*height + (src_stride[0]/2)*height2,
       src_stride[0], src_stride[0]/2, width, height, flip);
      break;

   default :
      return -1;
   }


   /* pad out image when the width and/or height is not a multiple of 16 */
   /** Not necessary, handled by VPX */
/*
   if (width & 15)
   {
      int i;
      int pad_width = 16 - (width&15);
      for (i = 0; i < height; i++)
      {
         memset(image->planes[VPX_PLANE_Y] + i*edged_width + width,
             *(image->planes[VPX_PLANE_Y] + i*edged_width + width - 1), pad_width);
      }
      for (i = 0; i < height/2; i++)
      {
         memset(image->planes[VPX_PLANE_U] + i*edged_width2 + width2,
             *(image->planes[VPX_PLANE_U] + i*edged_width2 + width2 - 1),pad_width/2);
         memset(image->planes[VPX_PLANE_V] + i*edged_width2 + width2,
             *(image->planes[VPX_PLANE_V] + i*edged_width2 + width2 - 1),pad_width/2);
      }
   }

   if (height & 15)
   {
      int pad_height = 16 - (height&15);
      int length = ((width+15)/16)*16;
      int i;
      for (i = 0; i < pad_height; i++)
      {
         memcpy(image->planes[VPX_PLANE_Y] + (height+i)*edged_width,
               image->planes[VPX_PLANE_Y] + (height-1)*edged_width,length);
      }

      for (i = 0; i < pad_height/2; i++)
      {
         memcpy(image->planes[VPX_PLANE_U] + (height2+i)*edged_width2,
               image->planes[VPX_PLANE_U] + (height2-1)*edged_width2,length/2);
         memcpy(image->planes[VPX_PLANE_V] + (height2+i)*edged_width2,
               image->planes[VPX_PLANE_V] + (height2-1)*edged_width2,length/2);
      }
   }
*/
/*
   if (interlacing)
      image_printf(image, edged_width, height, 5,5, "[i]");
   image_dump_yuvpgm(image, edged_width, ((width+15)/16)*16, ((height+15)/16)*16, "\\encode.pgm");
*/
   return 0;
}


/* return vp80 compatbile colorspace,
   or VPX_IMG_FMT_NONE if failure
*/

static vpx_img_fmt_t get_colorspace(int *flip, BITMAPINFOHEADER * hdr)
{
   /* rgb only: negative height specifies top down image */
  int rgb_flip = (hdr->biHeight < 0 ? 0 : 1);

  /** By default non-flip mode for non-RGB */
  *flip = 0;

   switch(hdr->biCompression)
   {
   case BI_RGB :
      if (hdr->biBitCount == 16)
      {
         DPRINTF("RGB16 (RGB555)");
		 *flip = rgb_flip;
         return VPX_IMG_FMT_RGB555;
      }
      if (hdr->biBitCount == 24)
      {
         DPRINTF("RGB24");
		 *flip = rgb_flip;
         return VPX_IMG_FMT_BGR24;
      }
      if (hdr->biBitCount == 32)
      {
         DPRINTF("RGB32");
		 *flip = rgb_flip;
         return VPX_IMG_FMT_RGB32_LE;
      }

      DPRINTF("unsupported BI_RGB biBitCount=%i", hdr->biBitCount);
      return VPX_IMG_FMT_NONE;

   case BI_BITFIELDS :
      if (hdr->biSize >= sizeof(BITMAPV4HEADER))
      {
         BITMAPV4HEADER * hdr4 = (BITMAPV4HEADER *)hdr;

         if (hdr4->bV4BitCount == 16 &&
            hdr4->bV4RedMask == 0x7c00 &&
            hdr4->bV4GreenMask == 0x3e0 &&
            hdr4->bV4BlueMask == 0x1f)
         {
            DPRINTF("RGB555");
   		    *flip = rgb_flip;
            return VPX_IMG_FMT_RGB555;
         }

         if(hdr4->bV4BitCount == 16 &&
            hdr4->bV4RedMask == 0xf800 &&
            hdr4->bV4GreenMask == 0x7e0 &&
            hdr4->bV4BlueMask == 0x1f)
         {
            DPRINTF("RGB565");
   		    *flip = rgb_flip;
            return VPX_IMG_FMT_RGB565;
         }

         DPRINTF("unsupported BI_BITFIELDS mode");
         return VPX_IMG_FMT_NONE;
      }

      DPRINTF("unsupported BI_BITFIELDS/BITMAPHEADER combination");
      return VPX_IMG_FMT_NONE;

   case FOURCC_I420 :
   case FOURCC_IYUV :
      DPRINTF("IYUY");
      return VPX_IMG_FMT_I420;

   case FOURCC_YV12 :
      DPRINTF("YV12");
      return VPX_IMG_FMT_YV12;

   case FOURCC_YUYV :
   case FOURCC_YUY2 :
      DPRINTF("YUY2");
      return VPX_IMG_FMT_YUY2;

   case FOURCC_YVYU :
      DPRINTF("YVYU");
      return VPX_IMG_FMT_YVYU;

   case FOURCC_UYVY :
      DPRINTF("UYVY");
      return VPX_IMG_FMT_UYVY;

   default :
      DPRINTF("unsupported colorspace %c%c%c%c",
         hdr->biCompression&0xff,
         (hdr->biCompression>>8)&0xff,
         (hdr->biCompression>>16)&0xff,
         (hdr->biCompression>>24)&0xff);
      return VPX_IMG_FMT_NONE;
   }
}

/** Utility routines used to free allocated memory */
static void freemem(CODEC* codec)
{
   if(codec->ehandle) {
      vpx_codec_destroy(codec->ehandle);
      free(codec->ehandle);
      codec->ehandle = NULL;
   }
   vpx_img_free(&codec->frame);

}


/* compressor */


/* test the output format */

LRESULT compress_query(CODEC * codec, BITMAPINFO * lpbiInput, BITMAPINFO * lpbiOutput)
{
   BITMAPINFOHEADER * inhdr = &lpbiInput->bmiHeader;
   BITMAPINFOHEADER * outhdr = &lpbiOutput->bmiHeader;
   int flip;

   /* VFWEXT detection */
   if (inhdr->biCompression == VFWEXT_FOURCC) {
      return (ICM_USER+0x0fff);
   }

   if (get_colorspace(&flip, inhdr) == VPX_IMG_FMT_NONE)
   {
      return ICERR_BADFORMAT;
   }

   // We only support even sizes of images.
   if ((inhdr->biWidth & 0x01 == 0x01) || (inhdr->biHeight & 0x01 == 0x01))
   {
      return ICERR_BADFORMAT;
   }


   if (lpbiOutput == NULL)
   {
      return ICERR_OK;
   }

   if (inhdr->biWidth != outhdr->biWidth || inhdr->biHeight != outhdr->biHeight ||
      outhdr->biCompression != FOURCC_VP80)
   {
      return ICERR_BADFORMAT;
   }


   return ICERR_OK;
}


LRESULT compress_get_format(CODEC * codec, BITMAPINFO * lpbiInput, BITMAPINFO * lpbiOutput)
{
   BITMAPINFOHEADER * inhdr = &lpbiInput->bmiHeader;
   BITMAPINFOHEADER * outhdr = &lpbiOutput->bmiHeader;
   int flip;

   if (get_colorspace(&flip, inhdr) == VPX_IMG_FMT_NONE)
   {
      return ICERR_BADFORMAT;
   }

   if (lpbiOutput == NULL)
   {
      return sizeof(BITMAPINFOHEADER);
   }

   memcpy(outhdr, inhdr, sizeof(BITMAPINFOHEADER));
   outhdr->biSize = sizeof(BITMAPINFOHEADER);
   outhdr->biSizeImage = compress_get_size(codec, lpbiInput, lpbiOutput);
   outhdr->biXPelsPerMeter = 0;
   outhdr->biYPelsPerMeter = 0;
   outhdr->biClrUsed = 0;
   outhdr->biClrImportant = 0;
   outhdr->biCompression = FOURCC_VP80;

   return ICERR_OK;
}


LRESULT compress_get_size(CODEC * codec, BITMAPINFO * lpbiInput, BITMAPINFO * lpbiOutput)
{
   return 2 * lpbiOutput->bmiHeader.biWidth * lpbiOutput->bmiHeader.biHeight * 3;
}


LRESULT compress_frames_info(CODEC * codec, ICCOMPRESSFRAMES * icf)
{
#if 0
   DPRINTF("%i %i", icf->lStartFrame, icf->lFrameCount);
#endif
   codec->fincr = icf->dwScale;
   codec->fbase = icf->dwRate;
//   codec->framenum = icf->lStartFrame;
   codec->framecount = icf->lFrameCount;
   return ICERR_OK;
}


static int vfw_debug(void *handle,
          int opt,
          void *param1,
          void *param2)
{
}




LRESULT compress_begin(CODEC * codec, BITMAPINFO * lpbiInput, BITMAPINFO * lpbiOutput)
{
   BITMAPINFOHEADER * inhdr = &lpbiInput->bmiHeader;
   SYSTEM_INFO sysinfo;

   vpx_codec_enc_cfg_t  cfg;

   // We only support even sizes of images.
   if ((inhdr->biWidth & 0x01 == 0x01) || (inhdr->biHeight & 0x01 == 0x01))
   {
      return ICERR_BADFORMAT;
   }


   /* destroy previously created codec */
   if(codec->ehandle) {
      freemem(codec);
   }


   vpx_codec_enc_config_default(vpx_encoder_interface, &cfg, 0);

  /* reset stats buffer */
  memset(&codec->stats, 0, sizeof(codec->stats));


   /* plugins */
   switch (codec->config.mode)
   {
   case RC_MODE_1PASS_VBR :
      cfg.rc_end_usage = VPX_VBR;
      cfg.g_pass = VPX_RC_ONE_PASS;
      if (!codec->config.use_bitrate) /* constant-quant mode */
        {
            cfg.rc_min_quantizer = codec->config.min_quant;
            cfg.rc_max_quantizer = codec->config.max_quant;
        } else
        {
         cfg.rc_target_bitrate = codec->config.bitrate;
        }
      break;
   case RC_MODE_1PASS_CBR :
      cfg.rc_end_usage = VPX_CBR;
      cfg.g_pass = VPX_RC_ONE_PASS;
      if (!codec->config.use_bitrate) /* constant-quant mode */
        {
            cfg.rc_min_quantizer = codec->config.min_quant;
            cfg.rc_max_quantizer = codec->config.max_quant;
        } else
        {
         cfg.rc_target_bitrate = codec->config.bitrate;
        }
      break;
   case RC_MODE_2PASS1 :
      /* reset stats buffer */
      cfg.rc_end_usage = VPX_VBR;
      cfg.g_pass = VPX_RC_FIRST_PASS;
      cfg.rc_2pass_vbr_maxsection_pct = profiles[codec->config.profile].maxdev;
      cfg.rc_target_bitrate = codec->config.bitrate;
      if (!stats_open_file(&codec->stats, CONFIG_2PASS_FILE, 0))
      {
          return ICERR_ERROR;
      }
/*      else
      {
            if (!stats_open_mem(&stats, pass))
            {
                fprintf(stderr, "Failed to open statistics store\n");
                return EXIT_FAILURE;
            }
        }*/
      break;
   case RC_MODE_2PASS2 :
      cfg.rc_end_usage = VPX_VBR;
      cfg.g_pass = VPX_RC_LAST_PASS;
      cfg.rc_2pass_vbr_maxsection_pct = profiles[codec->config.profile].maxdev;
      cfg.rc_target_bitrate = codec->config.bitrate;
      if (!stats_open_file(&codec->stats, CONFIG_2PASS_FILE, 1))
      {
          return ICERR_BADPARAM;
      }
      cfg.rc_twopass_stats_in = stats_get(&codec->stats);
      break;

   case RC_MODE_NULL :
      return ICERR_OK;

   default :
      break;
   }

   codec->framenum = 0;


   cfg.g_w = align16(lpbiInput->bmiHeader.biWidth);
   cfg.g_h = align16(lpbiInput->bmiHeader.biHeight);
   cfg.g_profile = profiles[codec->config.profile].id;



// cfg.rc_dropframe_thresh = quality_preset->frame_drop_ratio;
   // Disable frame dropping
   cfg.rc_dropframe_thresh = codec->config.frame_drop_ratio;

    /* Encoder threads - Suggested count is the number of CPU -1, to
	   a minimal number of 1.
	*/
    GetSystemInfo( &sysinfo );

	if (sysinfo.dwNumberOfProcessors > 1)
      cfg.g_threads = sysinfo.dwNumberOfProcessors - 1;
	else
      cfg.g_threads = 1;

	/* Before it was taken from the registry, now automatically detected */
/*
    if (codec->config.num_threads == 0)
        cfg.g_threads = 0; // No threads defined, use default of 1
    else
        cfg.g_threads = codec->config.num_threads;*/


   // Allocate the codec instance
   codec->ehandle = (vpx_codec_ctx_t*)malloc(sizeof(vpx_codec_ctx_t));


   switch(vpx_codec_enc_init(codec->ehandle, vpx_encoder_interface, &cfg, 0))
   {
   case VPX_CODEC_OK   :
      break;

   case VPX_CODEC_MEM_ERROR  :
      return ICERR_MEMORY;

   default :
      return ICERR_ERROR;
   }

   // We must allocate enough memory in case we need to pad the image.
    if (!vpx_img_alloc(&codec->frame, VPX_IMG_FMT_YV12, align16(lpbiInput->bmiHeader.biWidth), align16(lpbiInput->bmiHeader.biHeight), 1))
      return ICERR_MEMORY;



   return ICERR_OK;
}


LRESULT compress_end(CODEC * codec)
{
  if (codec==NULL)
    return ICERR_OK;

   return ICERR_OK;
}



#define CALC_BI_STRIDE(width,bitcount)  ((((width * bitcount) + 31) & ~31) >> 3)

LRESULT compress(CODEC * codec, ICCOMPRESS * icc)
{
   BITMAPINFOHEADER * inhdr = icc->lpbiInput;
   BITMAPINFOHEADER * outhdr = icc->lpbiOutput;
   uint8_t* planes[4];
   int stride[4];
   int length;
   int flags = 0;
   int flip = 0;
   int i;
   int loops;
   vpx_image_t* ctx[2];
   vpx_img_fmt_t csp;

   const vpx_codec_cx_pkt_t *pkt;

   if ((csp = get_colorspace(&flip, inhdr)) == VPX_IMG_FMT_NONE)
      return ICERR_BADFORMAT;
   planes[0] = (uint8_t*)icc->lpInput;
   stride[0] = CALC_BI_STRIDE(inhdr->biWidth, inhdr->biBitCount);

   /** Special handling for I420 and YV12 formats */
   if (csp == VPX_IMG_FMT_I420 || csp == VPX_IMG_FMT_YV12) {
	  stride[0] = (4 * icc->lpbiInput->biWidth + 3) / 4;
	  stride[1] = stride[2] = stride[0] / 2 ;
	}


   convert_input_color_space(
       &codec->frame,
	   // The codec->frame might contain the adjusted with and height with
	   // padding, therefore we must use the original image size.
	   icc->lpbiInput->biWidth,
	   icc->lpbiInput->biHeight,
	   icc->lpbiInput->biWidth,
       planes,
       stride,
       csp,
       0,
       flip);


   if (codec->config.mode == RC_MODE_NULL) {
      outhdr->biSizeImage = 0;
      *icc->lpdwFlags = AVIIF_KEYFRAME;
      return ICERR_OK;
   }


   // Kludge: When the last frame is found, then we should call
   // the encode two times and flush the last packet.

   loops = 1;
   ctx[0] = &codec->frame;
   ctx[1] = NULL;
   if ((codec->framecount) == (codec->framenum+1))
   {
       loops++;
   }

   length = 0;
   *icc->lpdwFlags = 0;

   // Loop several times to flush last section
   for (i = 0; i < loops; i++)
   {

     vpx_codec_iter_t iter = NULL;

    if(vpx_codec_encode(codec->ehandle, ctx[i], codec->framenum, 1, flags,
            quality_table[codec->config.quality].value))
        return ICERR_ERROR;

    while( (pkt = vpx_codec_get_cx_data(codec->ehandle, &iter)) )
    {
           switch(pkt->kind)
        {
             case VPX_CODEC_CX_FRAME_PKT:
                if (pkt->data.frame.flags & VPX_FRAME_IS_KEY) 
                {
        			*icc->lpdwFlags = AVIIF_KEYFRAME;
                }
                memcpy(icc->lpOutput, pkt->data.frame.buf, pkt->data.frame.sz);
                // Adjust output pointer data
                (UINT8*)icc->lpOutput += pkt->data.frame.sz;
                length += pkt->data.frame.sz;
                break;
            case VPX_CODEC_STATS_PKT:                                     //
                stats_write(&codec->stats,
                        pkt->data.twopass_stats.buf,
                        pkt->data.twopass_stats.sz);
                //codec->stats.sz +=  pkt->data.raw.sz;
                break;                                                    //
            default:
                break;
            }
    }

   }

    if (length == 0)  /* no encoder output */
    {
      *icc->lpdwFlags = 0;
      ((char*)icc->lpOutput)[0] = 0x7f;   /* virtual dub skip frame */
      outhdr->biSizeImage = 1;

    }else
    {

      outhdr->biSizeImage = length;

      if (codec->config.mode == RC_MODE_2PASS1)
      {
         outhdr->biSizeImage = 0;
      }
    }


   codec->framenum++;

   if ((codec->framenum==codec->framecount) &&
      ((codec->config.mode == RC_MODE_2PASS1)
       || (codec->config.mode == RC_MODE_2PASS2)))
   {
       stats_close(&codec->stats);
   }

   return ICERR_OK;
}


/* decompressor */


LRESULT decompress_query(CODEC * codec, BITMAPINFO *lpbiInput, BITMAPINFO *lpbiOutput)
{
   BITMAPINFOHEADER * inhdr = &lpbiInput->bmiHeader;
   BITMAPINFOHEADER * outhdr = &lpbiOutput->bmiHeader;
   int flip;

   if (lpbiInput == NULL)
   {
      return ICERR_ERROR;
   }

   if (inhdr->biCompression != FOURCC_VP80)
   {
      return ICERR_BADFORMAT;
   }

   if (lpbiOutput == NULL)
   {
      return ICERR_OK;
   }

   if (inhdr->biWidth != outhdr->biWidth ||
      inhdr->biHeight != outhdr->biHeight ||
	  (get_colorspace(&flip, outhdr) == VPX_IMG_FMT_NONE))
   {
      return ICERR_BADFORMAT;
   }

   return ICERR_OK;
}


LRESULT decompress_get_format(CODEC * codec, BITMAPINFO * lpbiInput, BITMAPINFO * lpbiOutput)
{
   BITMAPINFOHEADER * inhdr = &lpbiInput->bmiHeader;
   BITMAPINFOHEADER * outhdr = &lpbiOutput->bmiHeader;
   LRESULT result;

   if (lpbiOutput == NULL)
   {
      return sizeof(BITMAPINFOHEADER);
   }

   result = decompress_query(codec, lpbiInput, lpbiOutput);
   if (result != ICERR_OK)
   {
      return result;
   }

   outhdr->biSize = sizeof(BITMAPINFOHEADER);
   outhdr->biWidth = inhdr->biWidth;
   outhdr->biHeight = inhdr->biHeight;
   outhdr->biPlanes = 1;
   outhdr->biBitCount = 24;
   outhdr->biCompression = BI_RGB;	/* sonic foundry vegas video v3 only supports BI_RGB */
   outhdr->biSizeImage = outhdr->biHeight * CALC_BI_STRIDE(outhdr->biWidth, outhdr->biBitCount);

   outhdr->biXPelsPerMeter = 0;
   outhdr->biYPelsPerMeter = 0;
   outhdr->biClrUsed = 0;
   outhdr->biClrImportant = 0;

   return ICERR_OK;
}

#define REG_GET_N(X, Y, Z) \
{ \
   DWORD size = sizeof(int); \
   if (RegQueryValueEx(hKey, X, 0, 0, (LPBYTE)&Y, &size) != ERROR_SUCCESS) { \
      Y=Z; \
   } \
}while(0)

LRESULT decompress_begin(CODEC * codec, BITMAPINFO * lpbiInput, BITMAPINFO * lpbiOutput)
{
   int flags = 0;

   /* destroy previously created codec */
   if(codec->dhandle) {
      free(codec->dhandle);
      codec->dhandle = NULL;
   }

   codec->dhandle = (vpx_codec_ctx_t*)malloc(sizeof(vpx_codec_ctx_t));

   switch(vpx_codec_dec_init(codec->dhandle, vpx_decoder_interface, NULL, flags))
   {
   case VPX_CODEC_OK :
      break;

   case VPX_CODEC_MEM_ERROR:
      return ICERR_MEMORY;

   default:
      return ICERR_ERROR;
   }

   return ICERR_OK;
}


LRESULT decompress_end(CODEC * codec)
{
   if (codec->dhandle != NULL)
   {
      vpx_codec_destroy(codec->dhandle);
      codec->dhandle = NULL;
   }

   return ICERR_OK;
}


LRESULT decompress(CODEC * codec, ICDECOMPRESS * icd)
{

    vpx_codec_iter_t  iter = NULL;
    vpx_image_t      *img;
	int              flip;
	vpx_img_fmt_t    csp;
    uint8_t*         bufoutput[4];
	uint8_t*  output = icd->lpOutput;
	int              stride[4];

   bufoutput[0] = icd->lpOutput;
   stride[0] = CALC_BI_STRIDE(icd->lpbiOutput->biWidth, icd->lpbiOutput->biBitCount);
 
   csp = get_colorspace(&flip, icd->lpbiOutput);
   if (csp == VPX_IMG_FMT_I420 || csp == VPX_IMG_FMT_YV12)
	  stride[0] = CALC_BI_STRIDE(icd->lpbiOutput->biWidth, 8);


   if (~((icd->dwFlags & ICDECOMPRESS_HURRYUP) | (icd->dwFlags & ICDECOMPRESS_UPDATE) | (icd->dwFlags & ICDECOMPRESS_PREROLL)))
   {
      if (csp == VPX_IMG_FMT_NONE)
      {
         return ICERR_BADFORMAT;
      }
   }
   else
   {
   }

   if (vpx_codec_decode(codec->dhandle, (const uint8_t *)icd->lpInput, icd->lpbiInput->biSizeImage, NULL, 0)!=VPX_CODEC_OK)
      return ICERR_ERROR;

    /* Write decoded data  */
   while((img = vpx_codec_get_frame(codec->dhandle, &iter)))
   {
        /* Native format is YV12, so return as is */
    
	    convert_output_color_space(
		   img,
		   img->d_w,
           img->d_h,
           img->stride[VPX_PLANE_Y],
		   // dst data
		   bufoutput,
	       stride,
           csp,
           0,
           1);
   }


   return ICERR_OK;
}

