/*****************************************************************************
 *
 * VP80 VFW FRONTEND based on XVid 1.2.2 VFW Codec 
 *  - VFW configuration header  -
 *
 *  Copyright(C) 2002-2003 Anonymous <xvid-devel@xvid.org>
 *  Copied/Modified for VP8 Codec by Carl Eric Codère
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
 ****************************************************************************/
#ifndef _CONFIG_H_
#define _CONFIG_H_

#include <windows.h>
#include "vfwext.h"

extern HINSTANCE g_hInst;


/* small hack */
#ifndef IDC_HAND
#define IDC_HAND	MAKEINTRESOURCE(32649)
#endif

/* one kilobit */
#define CONFIG_KBPS 1000

/* min/max values when not specified by profile */
#define DEFAULT_MIN_KBPS	16
#define DEFAULT_MAX_KBPS	50000
#define DEFAULT_QUANT		4
#define DEFAULT_MIN_QUANT	4
#define DEFAULT_MAX_QUANT	63

#define DEFAULT_DEV         400
// Min max allowed values for quantizer
#define MIN_QUANT   0
#define MAX_QUANT   63

/* registry stuff */
#define VP80_REG_KEY	HKEY_CURRENT_USER
#define VP80_REG_PARENT	"Software\\GNU"
#define VP80_REG_CHILD	"vp80"
#define VP80_REG_CLASS	"config"

#define VP80_BUILD		__TIME__ ", " __DATE__
#define VP80_WEBSITE	"http://www.webmproject.org/"
#define VP80_SPECIAL_BUILD	"Standard build"

/* constants */
#define CONFIG_2PASS_FILE ".\\video.pass"

/* codec modes - index into string list */
#define RC_MODE_1PASS_VBR    0
#define RC_MODE_1PASS_CBR    1
#define RC_MODE_2PASS1		 2
#define RC_MODE_2PASS2		 3
#define RC_MODE_NULL		 4

/** Quality settings */
typedef struct
{
  char* name;           /** Friendly name for this quality setting */
  int   value;          /** Internal codec value for this quality setting */
} quality_t;


/** Profile settings */
typedef struct
{
  char* name;           /** Friendly name for this quality setting */
  int   id;             /** Internal profile value */
  int   width;          /** Preset suggested max width. */
  int   height;         /** Preset suggested max height */
  int   fps;            /** Preset suggested max framerate */
  int   max_bitrate;    /** Maximum average bitrate -- indicative only  */
  int   maxdev;         /** Maximum percent of bitrate deviation allowed in CBR mode */
} profile_t;


typedef struct
{
/********** ATTENTION **********/
	int mode;					/* Vidomi directly accesses these vars */
	int bitrate;				
	char stats[MAX_PATH];		
/*******************************/
   /* quant */
	int min_quant;			    /* for one-pass constant quant */
	int max_quant;			    /* for one-pass constant quant */

	/* profile  */
	char profile_name[MAX_PATH];
	int profile;			/* used internally; *not* written to registry */

    /* quality preset */
	char quality_name[MAX_PATH];
	int quality;			/* used internally; *not* written to registry */


    int use_bitrate;        /** non-zero if we use bitrate instead of quantizer */
	int display_aspect_ratio;				/* aspect ratio */
	int ar_x, ar_y;							/* picture aspect ratio */
	int par_x, par_y;						/* custom pixel aspect ratio */
	int ar_mode;							/* picture/pixel AR */

    int max_key_interval;
    int frame_drop_ratio;

	/* debug */
	int num_threads;
	int debug;
	int display_status;
	int full1pass;

	DWORD cpu;

	/* internal */
	int ci_valid;
	VFWEXT_CONFIGURE_INFO_T ci;

	BOOL save;
} CONFIG;

typedef struct PROPSHEETINFO
{
	int idd;
	CONFIG * config;
} PROPSHEETINFO;

typedef struct REG_INT
{
	char* reg_value;
	int* config_int;
	int def;
} REG_INT;

typedef struct REG_STR
{
	char* reg_value;
	char* config_str;
	char* def;
} REG_STR;






extern const profile_t profiles[];
extern const quality_t quality_table[];


void config_reg_get(CONFIG * config);
void config_reg_set(CONFIG * config);


INT_PTR CALLBACK main_proc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK about_proc(HWND, UINT, WPARAM, LPARAM);

#endif /* _CONFIG_H_ */
