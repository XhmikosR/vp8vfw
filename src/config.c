/**************************************************************************
 *
 * VP80 VFW FRONTEND based on XVid 1.2.2 VFW Codec 
 * Modified/Copied by Carl Eric Codère
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
 *************************************************************************/


#include <windows.h>
#include <commctrl.h>
#include <tchar.h>
#include <stdio.h>  /* sprintf */
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <vpx/vpx_encoder.h>

#include "debug.h"
#include "config.h"
#include "resource.h"


#define CONSTRAINVAL(X,Y,Z) if((X)<(Y)) X=Y; if((X)>(Z)) X=Z;
#define IsDlgChecked(hwnd,idc)   (IsDlgButtonChecked(hwnd,idc) == BST_CHECKED)
#define CheckDlg(hwnd,idc,value) CheckDlgButton(hwnd,idc, value?BST_CHECKED:BST_UNCHECKED)
#define EnableDlgWindow(hwnd,idc,state) EnableWindow(GetDlgItem(hwnd,idc),state)
#define CHAR_BUFFER_SIZE 2048

HINSTANCE g_hInst;
HWND g_hTooltip;

static int g_use_bitrate = 1;


int pp_brightness, pp_dy, pp_duv, pp_fe, pp_dry, pp_druv; /* decoder options */

/* enumerates child windows, assigns tooltips */
BOOL CALLBACK enum_tooltips(HWND hWnd, LPARAM lParam)
{
   char help[500];

   if (LoadString(g_hInst, GetDlgCtrlID(hWnd), help, 500))
   {
      TOOLINFO ti;
      ti.cbSize = sizeof(TOOLINFO);
      ti.uFlags = TTF_SUBCLASS | TTF_IDISHWND;
      ti.hwnd = GetParent(hWnd);
      ti.uId   = (LPARAM)hWnd;
      ti.lpszText = help;
      SendMessage(g_hTooltip, TTM_ADDTOOL, 0, (LPARAM)&ti);
   }

   return TRUE;
}


/* ===================================================================================== */
/* VP8 PROFILES/LEVELS ================================================================= */
/* ===================================================================================== */

const profile_t profiles[] =
{
/*  name                p@l    w    h    fps     bps    %dev vbr */
  { "Simple Preset 0",      0x03,  176, 144, 30,   96000,    200},
  { "Simple Preset 1",      0x03,  176, 144, 30,  192000,    200},
  { "Simple Preset 2",      0x03,  352, 288, 30,  384000,    200},
  { "Simple Preset 3",      0x03,  352, 288, 30,  768000,    200},
  { "Simple Preset 4",      0x03,  640, 480, 30, 1200000,    200},
  { "Simple Preset 5",      0x03,  640, 480, 30, 2304000,    200},

  { "Advanced Simple Preset 0",      0x01,  176, 144, 30,   96000,    200},
  { "Advanced Simple Preset 1",      0x01,  176, 144, 30,  192000,    200},
  { "Advanced Simple Preset 2",      0x01,  352, 288, 30,  384000,    200},
  { "Advanced Simple Preset 3",      0x01,  352, 288, 30,  768000,    200},
  { "Advanced Simple Preset 4",      0x01,  640, 480, 30, 1200000,    200},
  { "Advanced Simple Preset 5",      0x01,  640, 480, 30, 2304000,    200},

  { "Main Preset 0",      0x00,  176, 144, 30,  192000,    400},
  { "Main Preset 1",      0x00,  352, 288, 30,  768000,    400},
  { "Main Preset 2",      0x00,  640, 480, 30,  2304000,   400},

  { "(unrestricted)",     0x00,    0,   0,  0,        0,   400},

  };


#define QUALITY_DEFAULT_STRING  "Good"

const quality_t quality_table[] =
{
    /* name            value */
  { "Real-time",       VPX_DL_REALTIME},
  { "Good"     ,       VPX_DL_GOOD_QUALITY},
  { "Best"     ,       VPX_DL_BEST_QUALITY}
};

const int quality_table_num = sizeof(quality_table)/sizeof(quality_t);

typedef struct {
   char * name;
   float value;
} named_float_t;

static const named_float_t video_fps_list[] = {
   {  "15.0",           15.0F },
   {  "23.976 (FILM)",     23.976F  },
   {  "25.0 (PAL)",     25.0F },
   {  "29.97 (NTSC)",      29.970F  },
   {  "30.0",           30.0F },
   {  "50.0 (HD PAL)",     50.0F },
   {  "59.94 (HD NTSC)",   59.940F  },
   {  "60.0",           60.0F },
};


/* ===================================================================================== */
/* REGISTRY ============================================================================ */
/* ===================================================================================== */

/* registry info structs */
CONFIG reg;

static const REG_INT reg_ints[] = {
   {"mode",             &reg.mode,                 RC_MODE_1PASS_VBR},
   {"bitrate",          &reg.bitrate,              768},
   {"use_bitrate",      &reg.use_bitrate,          1},

   /* quant */
   {"min_quant",           &reg.min_quant,            DEFAULT_QUANT},
   {"max_quant",           &reg.max_quant,            DEFAULT_QUANT},

   /* debug */
   {"debug",               &reg.debug,                0x0},

   /* smp */
   {"num_threads",            &reg.num_threads,          1},

};

static const REG_STR reg_strs[] = {
   {"profile",             reg.profile_name,          "(unrestricted)"},
   {"quality",             reg.quality_name,       QUALITY_DEFAULT_STRING},
   {"stats",               reg.stats,                 CONFIG_2PASS_FILE},
};




#define REG_GET_B(X, Y, Z) size=sizeof((Z));if(RegQueryValueEx(hKey, X, 0, 0, Y, &size) != ERROR_SUCCESS) {memcpy(Y, Z, sizeof((Z)));}


void config_reg_get(CONFIG * config)
{
   HKEY hKey;
   DWORD size;
   int i;

   RegOpenKeyEx(VP80_REG_KEY, VP80_REG_PARENT "\\" VP80_REG_CHILD, 0, KEY_READ, &hKey);

   /* read integer values */
   for (i=0 ; i<sizeof(reg_ints)/sizeof(REG_INT); i++) {
      size = sizeof(int);
      if (RegQueryValueEx(hKey, reg_ints[i].reg_value, 0, 0, (LPBYTE)reg_ints[i].config_int, &size) != ERROR_SUCCESS) {
         *reg_ints[i].config_int = reg_ints[i].def;
      }
   }

   /* read string values */
   for (i=0 ; i<sizeof(reg_strs)/sizeof(REG_STR); i++) {
      size = MAX_PATH;
      if (RegQueryValueEx(hKey, reg_strs[i].reg_value, 0, 0, (LPBYTE)reg_strs[i].config_str, &size) != ERROR_SUCCESS) {
         memcpy(reg_strs[i].config_str, reg_strs[i].def, MAX_PATH);
      }
   }

  /* find profile table index */
   reg.profile = 0;
   for (i=0; i<sizeof(profiles)/sizeof(profile_t); i++) {
      if (lstrcmpi(profiles[i].name, reg.profile_name) == 0) {
         reg.profile = i;
      }
   }

  /* find quality table index */
   reg.quality = quality_table_num;
   for (i=0; i<quality_table_num; i++) {
      if (lstrcmpi(quality_table[i].name, reg.quality_name) == 0) {
         reg.quality = i;
      }
   }

   memcpy(config, &reg, sizeof(CONFIG));



   RegCloseKey(hKey);
}


/* put config settings in registry */

#define REG_SET_B(X, Y) RegSetValueEx(hKey, X, 0, REG_BINARY, Y, sizeof((Y)))

void config_reg_set(CONFIG * config)
{
   HKEY hKey;
   DWORD dispo;
   int i;

   if (RegCreateKeyEx(
         VP80_REG_KEY,
         VP80_REG_PARENT "\\" VP80_REG_CHILD,
         0,
         VP80_REG_CLASS,
         REG_OPTION_NON_VOLATILE,
         KEY_WRITE,
         0,
         &hKey,
         &dispo) != ERROR_SUCCESS)
   {
      DPRINTF("Couldn't create VP80_REG_SUBKEY - GetLastError=%i", GetLastError());
      return;
   }

   memcpy(&reg, config, sizeof(CONFIG));

   /* set integer values */
   for (i=0 ; i<sizeof(reg_ints)/sizeof(REG_INT); i++) {
      RegSetValueEx(hKey, reg_ints[i].reg_value, 0, REG_DWORD, (LPBYTE)reg_ints[i].config_int, sizeof(int));
   }

   /* set string values */
   strcpy(reg.profile_name, profiles[reg.profile].name);
    strcpy(reg.quality_name,
      reg.quality<quality_table_num ? quality_table[reg.quality].name : QUALITY_DEFAULT_STRING);
   for (i=0 ; i<sizeof(reg_strs)/sizeof(REG_STR); i++) {
      RegSetValueEx(hKey, reg_strs[i].reg_value, 0, REG_SZ, reg_strs[i].config_str, lstrlen(reg_strs[i].config_str)+1);
   }

   RegCloseKey(hKey);
}


/* clear registry key, load defaults */

static void config_reg_default(CONFIG * config)
{
   HKEY hKey;

   if (RegOpenKeyEx(VP80_REG_KEY, VP80_REG_PARENT, 0, KEY_ALL_ACCESS, &hKey)) {
      DPRINTF("Couldn't open registry key for deletion - GetLastError=%i", GetLastError());
      return;
   }

   if (RegDeleteKey(hKey, VP80_REG_CHILD)) {
      DPRINTF("Couldn't delete registry key - GetLastError=%i", GetLastError());
      return;
   }

   RegCloseKey(hKey);
   config_reg_get(config);
   config_reg_set(config);
}


/* leaves current config value if dialog item is empty */
static int config_get_int(HWND hDlg, INT item, int config)
{
   BOOL success = FALSE;
   int tmp = GetDlgItemInt(hDlg, item, &success, TRUE);
   return (success) ? tmp : config;
}


static int config_get_uint(HWND hDlg, UINT item, int config)
{
   BOOL success = FALSE;
   int tmp = GetDlgItemInt(hDlg, item, &success, FALSE);
   return (success) ? tmp : config;
}

/* get uint from combobox
   GetDlgItemInt doesnt work properly for cb list items */
#define UINT_BUF_SZ  20
static int config_get_cbuint(HWND hDlg, UINT item, int def)
{
   LRESULT sel = SendMessage(GetDlgItem(hDlg, item), CB_GETCURSEL, 0, 0);
   char buf[UINT_BUF_SZ];

   if (sel<0) {
      return config_get_uint(hDlg, item, def);
   }

   if (SendMessage(GetDlgItem(hDlg, item), CB_GETLBTEXT, sel, (LPARAM)buf) == CB_ERR) {
      return def;
   }

   return atoi(buf);
}


/* we use "100 base" floats */

#define FLOAT_BUF_SZ 20
static int get_dlgitem_float(HWND hDlg, UINT item, int def)
{
   char buf[FLOAT_BUF_SZ];

   if (GetDlgItemText(hDlg, item, buf, FLOAT_BUF_SZ) == 0)
      return def;

   return (int)(atof(buf)*100);
}




static void set_dlgitem_float(HWND hDlg, UINT item, int value)
{
   char buf[FLOAT_BUF_SZ];
   sprintf(buf, "%.2f", (float)value/100);
   SetDlgItemText(hDlg, item, buf);
}

static void set_dlgitem_float1000(HWND hDlg, UINT item, int value)
{
   char buf[FLOAT_BUF_SZ];
   sprintf(buf, "%.3f", (float)value/1000);
   SetDlgItemText(hDlg, item, buf);
}

#define HEX_BUF_SZ  16
static unsigned int get_dlgitem_hex(HWND hDlg, UINT item, unsigned int def)
{
   char buf[HEX_BUF_SZ];
   unsigned int value;

   if (GetDlgItemText(hDlg, item, buf, HEX_BUF_SZ) == 0)
      return def;

   if (sscanf(buf,"0x%x", &value)==1 || sscanf(buf,"%x", &value)==1) {
      return value;
   }

   return def;
}

static void set_dlgitem_hex(HWND hDlg, UINT item, int value)
{
   char buf[HEX_BUF_SZ];
   wsprintf(buf, "0x%x", value);
   SetDlgItemText(hDlg, item, buf);
}


static int get_version_info(int *major, int *minor, int *patch)
{
    DWORD lp;
    DWORD dwSize;
    DWORD dwBufSize;
    VS_FIXEDFILEINFO* lpFFI;
    uint8_t* buffer;
    TCHAR filename[CHAR_BUFFER_SIZE];
    if (GetModuleFileName(g_hInst, filename, CHAR_BUFFER_SIZE) == 0)
        return -1;
    
    dwSize = GetFileVersionInfoSize(filename,&lp);
    if (dwSize == 0)
        return -1;

    buffer = malloc(dwSize);
    if (buffer) 
    {
        if (GetFileVersionInfo(filename, 0, dwSize, buffer) != 0)
        {
            if (VerQueryValue(buffer, _T("\\"), (LPVOID *) &lpFFI, (UINT *) &dwBufSize))
            {
                *major = HIWORD(lpFFI->dwFileVersionMS);
                *minor = LOWORD(lpFFI->dwFileVersionMS);
                *patch = HIWORD(lpFFI->dwFileVersionLS);
            }
        }
    }
    free(buffer);
    return 0;

}

/* ===================================================================================== */
/* ADVANCED DIALOG PAGES ================================================================ */
/* ===================================================================================== */

/* initialise pages */
static void adv_init(HWND hDlg, int idd, CONFIG * config)
{
	unsigned int i;

	switch(idd) {

	case IDD_LEVEL :
		for (i=0; i<sizeof(profiles)/sizeof(profile_t); i++)
			SendDlgItemMessage(hDlg, IDC_LEVEL_PROFILE, CB_ADDSTRING, 0, (LPARAM)profiles[i].name);
		break;


	case IDD_ENC :
		SendDlgItemMessage(hDlg, IDC_FOURCC, CB_ADDSTRING, 0, (LPARAM)"XVID");
		SendDlgItemMessage(hDlg, IDC_FOURCC, CB_ADDSTRING, 0, (LPARAM)"DIVX");
		SendDlgItemMessage(hDlg, IDC_FOURCC, CB_ADDSTRING, 0, (LPARAM)"DX50");
		break;

	}
}


/* enable/disable controls based on encoder-mode or user selection */

static void adv_mode(HWND hDlg, int idd, CONFIG * config)
{
	int profile;

	switch(idd) {

	case IDD_LEVEL :
		profile = SendDlgItemMessage(hDlg, IDC_LEVEL_PROFILE, CB_GETCURSEL, 0, 0);
		SetDlgItemInt(hDlg, IDC_LEVEL_WIDTH, profiles[profile].width, FALSE);
		SetDlgItemInt(hDlg, IDC_LEVEL_HEIGHT, profiles[profile].height, FALSE);
		SetDlgItemInt(hDlg, IDC_LEVEL_FPS, profiles[profile].fps, FALSE);
        SetDlgItemInt(hDlg, IDC_LEVEL_STREAM, profiles[profile].id, FALSE);
        SetDlgItemInt(hDlg, IDC_LEVEL_AVGRATE_DEV, profiles[profile].maxdev, FALSE);
        set_dlgitem_float1000(hDlg, IDC_LEVEL_BITRATE, profiles[profile].max_bitrate);

    {
      int en_dim = profiles[profile].width && profiles[profile].height;
      EnableDlgWindow(hDlg, IDC_LEVEL_LEVEL_G, en_dim);
      EnableDlgWindow(hDlg, IDC_LEVEL_DIM_S, en_dim);
      EnableDlgWindow(hDlg, IDC_LEVEL_WIDTH, en_dim);
      EnableDlgWindow(hDlg, IDC_LEVEL_HEIGHT,en_dim);
      EnableDlgWindow(hDlg, IDC_LEVEL_FPS,   en_dim);
    }
    {
      int en_br = profiles[profile].max_bitrate;
      
      EnableDlgWindow(hDlg, IDC_LEVEL_BITRATE_S,  en_br);
      EnableDlgWindow(hDlg, IDC_LEVEL_BITRATE,    en_br);
    }
		break;
    }

}


/* upload config data into dialog */
static void adv_upload(HWND hDlg, int idd, CONFIG * config)
{
	switch (idd)
	{
	case IDD_LEVEL :
		SendDlgItemMessage(hDlg, IDC_LEVEL_PROFILE, CB_SETCURSEL, config->profile, 0);
		break;
/*
  case IDD_ENC:
		SetDlgItemInt(hDlg, IDC_NUMTHREADS, config->num_threads, FALSE);
		SendDlgItemMessage(hDlg, IDC_FOURCC, CB_SETCURSEL, config->fourcc_used, 0);
		CheckDlg(hDlg, IDC_VOPDEBUG, config->vop_debug);
		CheckDlg(hDlg, IDC_DISPLAY_STATUS, config->display_status);
		break;
*/
    }
}


/* download config data from dialog */

static void adv_download(HWND hDlg, int idd, CONFIG * config)
{
	switch (idd)
	{

	case IDD_LEVEL :
		config->profile = SendDlgItemMessage(hDlg, IDC_LEVEL_PROFILE, CB_GETCURSEL, 0, 0);
		break;

/*
  case IDD_ENC :
		config->num_threads = min(4, config_get_uint(hDlg, IDC_NUMTHREADS, config->num_threads));
		config->fourcc_used = SendDlgItemMessage(hDlg, IDC_FOURCC, CB_GETCURSEL, 0, 0);
		config->vop_debug = IsDlgChecked(hDlg, IDC_VOPDEBUG);
		config->display_status = IsDlgChecked(hDlg, IDC_DISPLAY_STATUS);
		break;
*/
    }
}



/* advanced dialog proc */

static INT_PTR CALLBACK adv_proc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	PROPSHEETINFO *psi;

	psi = (PROPSHEETINFO*)GetWindowLongPtr(hDlg, GWLP_USERDATA);

	switch (uMsg)
	{
	case WM_INITDIALOG :
		psi = (PROPSHEETINFO*) ((LPPROPSHEETPAGE)lParam)->lParam;
		SetWindowLongPtr(hDlg, GWLP_USERDATA, (LPARAM)psi);

		if (g_hTooltip)
			EnumChildWindows(hDlg, enum_tooltips, 0);

		adv_init(hDlg, psi->idd, psi->config);
		break;

	case WM_COMMAND :
		if (HIWORD(wParam) == BN_CLICKED)
		{
			switch (LOWORD(wParam))
			{
			default :
				return TRUE;
			}
		}else if (HIWORD(wParam) == LBN_SELCHANGE &&
			(LOWORD(wParam) == IDC_PROFILE_PROFILE ||
			 LOWORD(wParam) == IDC_LEVEL_PROFILE ||
			 LOWORD(wParam) == IDC_BITRATE_FPS)) {
			adv_mode(hDlg, psi->idd, psi->config);
		} else
			return 0;
		break;

	case WM_HSCROLL :
		return 0;

 
	case WM_NOTIFY :
		switch (((NMHDR *)lParam)->code)
		{
		case PSN_SETACTIVE :
			DPRINTF("PSN_SET");
			adv_upload(hDlg, psi->idd, psi->config);
			adv_mode(hDlg, psi->idd, psi->config);
			SetWindowLongPtr(hDlg, DWLP_MSGRESULT, FALSE);
			break;

		case PSN_KILLACTIVE :
			DPRINTF("PSN_KILL");
			adv_download(hDlg, psi->idd, psi->config);
			SetWindowLongPtr(hDlg, DWLP_MSGRESULT, FALSE);
			break;

		case PSN_APPLY :
			DPRINTF("PSN_APPLY");
			psi->config->save = TRUE;
			SetWindowLongPtr(hDlg, DWLP_MSGRESULT, FALSE);
			break;
		}
		break;

	default :
		return 0;
	}

	return 1;
}




/* load advanced options property sheet
  returns true, if the user accepted the changes
  or fasle if changes were canceled.

  */

#ifndef PSH_NOCONTEXTHELP
#define PSH_NOCONTEXTHELP 0x02000000
#endif

static BOOL adv_dialog(HWND hParent, CONFIG * config, const int * dlgs, int size)
{
	PROPSHEETINFO psi[6];
	PROPSHEETPAGE psp[6];
	PROPSHEETHEADER psh;
	CONFIG temp;
	int i;

	config->save = FALSE;
	memcpy(&temp, config, sizeof(CONFIG));

	for (i=0; i<size; i++)
	{
		psp[i].dwSize = sizeof(PROPSHEETPAGE);
		psp[i].dwFlags = 0;
		psp[i].hInstance = g_hInst;
		psp[i].pfnDlgProc = adv_proc;
		psp[i].lParam = (LPARAM)&psi[i];
		psp[i].pfnCallback = NULL;
		psp[i].pszTemplate = MAKEINTRESOURCE(dlgs[i]);

		psi[i].idd = dlgs[i];
		psi[i].config = &temp;
	}

	psh.dwSize = sizeof(PROPSHEETHEADER);
	psh.dwFlags = PSH_PROPSHEETPAGE | PSH_NOAPPLYNOW | PSH_NOCONTEXTHELP;
	psh.hwndParent = hParent;
	psh.hInstance = g_hInst;
	psh.pszCaption = (LPSTR) "VP8 Configuration";
	psh.nPages = size;
	psh.nStartPage = 0;
	psh.ppsp = (LPCPROPSHEETPAGE)&psp;
	psh.pfnCallback = NULL;
	PropertySheet(&psh);

	if (temp.save)
		memcpy(config, &temp, sizeof(CONFIG));

	return temp.save;
}


/* ===================================================================================== */
/* MAIN DIALOG ========================================================================= */
/* ===================================================================================== */




static void main_mode(HWND hDlg, CONFIG * config)
{
   const int profile = SendDlgItemMessage(hDlg, IDC_PROFILE, CB_GETCURSEL, 0, 0);
   const int rc_mode = SendDlgItemMessage(hDlg, IDC_MODE, CB_GETCURSEL, 0, 0);
   /* enable target rate/size control only for 1pass and 2pass  modes*/
   const int target_en = rc_mode==RC_MODE_1PASS_VBR || rc_mode==RC_MODE_2PASS2 || rc_mode==RC_MODE_1PASS_CBR;
   const int target_en_slider = rc_mode==RC_MODE_1PASS_VBR ||
      (rc_mode==RC_MODE_2PASS2) || (rc_mode==RC_MODE_1PASS_CBR);

   char buf[32];
   int max;
   int currentRate;

   g_use_bitrate = config->use_bitrate;

   if (g_use_bitrate) {
      SetDlgItemText(hDlg, IDC_BITRATE_S, "Target bitrate (kbps):");

      wsprintf(buf, "%i kbps", DEFAULT_MIN_KBPS);
      SetDlgItemText(hDlg, IDC_BITRATE_MIN, buf);

      max = profiles[profile].max_bitrate / 1000;
      if (max == 0) max = DEFAULT_MAX_KBPS;
      // Verify the maximum bitrate value - must always be in range
      currentRate = config_get_uint(hDlg, IDC_BITRATE, DEFAULT_MIN_KBPS);
      if (currentRate >= max)
      {
          currentRate = max;
      }
      // Verify the minimum quantizer value - must always be in range
      if (currentRate <= DEFAULT_MIN_KBPS)
      {
          currentRate = DEFAULT_MIN_KBPS;
      }
      SetDlgItemInt(hDlg, IDC_BITRATE, currentRate, FALSE);
      wsprintf(buf, "%i kbps", max);
      SetDlgItemText(hDlg, IDC_BITRATE_MAX, buf);

      SendDlgItemMessage(hDlg, IDC_SLIDER, TBM_SETRANGEMIN, TRUE, DEFAULT_MIN_KBPS);
      SendDlgItemMessage(hDlg, IDC_SLIDER, TBM_SETRANGEMAX, TRUE, max);
      SendDlgItemMessage(hDlg, IDC_SLIDER, TBM_SETPOS, TRUE,
        config_get_uint(hDlg, IDC_BITRATE, DEFAULT_MIN_KBPS) );

   } else if (rc_mode==RC_MODE_1PASS_VBR) {
      SetDlgItemText(hDlg, IDC_BITRATE_S, "Target quantizer:");
      SendDlgItemMessage(hDlg, IDC_SLIDER, TBM_SETRANGE, TRUE, MAKELONG(MIN_QUANT, MAX_QUANT));

      // Verify the maximum bitrate value - must always be in range
      currentRate = config_get_uint(hDlg, IDC_BITRATE, MIN_QUANT);
      if (currentRate >= MAX_QUANT)
      {
          currentRate = MAX_QUANT;
      }
      // Verify the minimum quantizer value - must always be in range
      if (currentRate <= MIN_QUANT)
      {
          currentRate = MIN_QUANT;
      }
      SetDlgItemInt(hDlg, IDC_BITRATE, currentRate, FALSE);
      SendDlgItemMessage(hDlg, IDC_SLIDER, TBM_SETPOS, TRUE,
                     config_get_uint(hDlg, IDC_BITRATE, DEFAULT_QUANT ));
      wsprintf(buf, "%i (maximum quality)", MIN_QUANT);
      SetDlgItemText(hDlg, IDC_BITRATE_MIN, buf);
      wsprintf(buf, "%i (smallest file)", MAX_QUANT);
      SetDlgItemText(hDlg, IDC_BITRATE_MAX, buf);

   }

   EnableDlgWindow(hDlg, IDC_BITRATE_S, target_en);
   EnableDlgWindow(hDlg, IDC_BITRATE, target_en);

   EnableDlgWindow(hDlg, IDC_BITRATE_MIN, target_en_slider);
   EnableDlgWindow(hDlg, IDC_BITRATE_MAX, target_en_slider);
   EnableDlgWindow(hDlg, IDC_SLIDER, target_en_slider);
}


static void main_upload(HWND hDlg, CONFIG * config)
{

   SendDlgItemMessage(hDlg, IDC_PROFILE, CB_SETCURSEL, config->profile, 0);
   SendDlgItemMessage(hDlg, IDC_MODE, CB_SETCURSEL, config->mode, 0);
   SendDlgItemMessage(hDlg, IDC_QUALITY, CB_SETCURSEL, config->quality, 0);

   g_use_bitrate = config->use_bitrate;;

   if (g_use_bitrate) {
      SetDlgItemInt(hDlg, IDC_BITRATE, config->bitrate, FALSE);
   } else if (config->mode == RC_MODE_1PASS_VBR)  {
      SetDlgItemInt(hDlg, IDC_BITRATE, config->min_quant, FALSE);
   }

}


/* downloads data from main dialog */
static void main_download(HWND hDlg, CONFIG * config)
{
   config->profile = SendDlgItemMessage(hDlg, IDC_PROFILE, CB_GETCURSEL, 0, 0);
   config->mode = SendDlgItemMessage(hDlg, IDC_MODE, CB_GETCURSEL, 0, 0);
   config->quality = SendDlgItemMessage(hDlg, IDC_QUALITY, CB_GETCURSEL, 0, 0);

   if (g_use_bitrate) {
      config->bitrate = config_get_uint(hDlg, IDC_BITRATE, config->bitrate);
   } else if (config->mode == RC_MODE_1PASS_VBR) {
      config->min_quant = config_get_uint(hDlg, IDC_BITRATE, config->min_quant);
      // Both min and max quantizer values are set to equal values
      config->max_quant = config->min_quant;
   }
}


/* main dialog proc */

static const int profile_dlgs[] = { IDD_PROFILE, IDD_LEVEL, IDD_AR };
static const int single_dlgs[] = { IDD_RC_CBR };
static const int pass1_dlgs[] = { IDD_RC_2PASS1 };
static const int pass2_dlgs[] = { IDD_RC_2PASS2 };
static const int bitrate_dlgs[] = { IDD_BITRATE };
static const int zone_dlgs[] = { IDD_ZONE };
static const int quality_dlgs[] = { IDD_MOTION, IDD_QUANT };
static const int other_dlgs[] = { IDD_ENC, IDD_DEC, IDD_COMMON };


INT_PTR CALLBACK main_proc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
   CONFIG* config = (CONFIG*)GetWindowLongPtr(hDlg, GWLP_USERDATA);
   unsigned int i;

   switch (uMsg)
   {
   case WM_INITDIALOG :
      SetWindowLongPtr(hDlg, GWLP_USERDATA, lParam);
      config = (CONFIG*)lParam;

      for (i=0; i<sizeof(profiles)/sizeof(profile_t); i++)
         SendDlgItemMessage(hDlg, IDC_PROFILE, CB_ADDSTRING, 0, (LPARAM)profiles[i].name);

      SendDlgItemMessage(hDlg, IDC_MODE, CB_ADDSTRING, 0, (LPARAM)"Single pass - VBR");
      SendDlgItemMessage(hDlg, IDC_MODE, CB_ADDSTRING, 0, (LPARAM)"Single pass - CBR");
      SendDlgItemMessage(hDlg, IDC_MODE, CB_ADDSTRING, 0, (LPARAM)"Twopass - 1st pass");
      SendDlgItemMessage(hDlg, IDC_MODE, CB_ADDSTRING, 0, (LPARAM)"Twopass - 2nd pass");
#ifdef _DEBUG
      SendDlgItemMessage(hDlg, IDC_MODE, CB_ADDSTRING, 0, (LPARAM)"Null test speed");
#endif

      for (i=0; i<quality_table_num; i++)
         SendDlgItemMessage(hDlg, IDC_QUALITY, CB_ADDSTRING, 0, (LPARAM)quality_table[i].name);

      InitCommonControls();

      if ((g_hTooltip = CreateWindow(TOOLTIPS_CLASS, NULL, TTS_ALWAYSTIP,
            CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
            NULL, NULL, g_hInst, NULL)))
      {
         SetWindowPos(g_hTooltip, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE);
         SendMessage(g_hTooltip, TTM_SETDELAYTIME, TTDT_AUTOMATIC, MAKELONG(1500, 0));
#if (_WIN32_IE >= 0x0300)
         SendMessage(g_hTooltip, TTM_SETMAXTIPWIDTH, 0, 400);
#endif

         EnumChildWindows(hDlg, enum_tooltips, 0);
      }

      SetClassLongPtr(GetDlgItem(hDlg, IDC_BITRATE_S), GCLP_HCURSOR, (LONG_PTR)LoadCursor(NULL, IDC_HAND));

      /* XXX: main_mode needs RC_MODE_xxx, main_upload needs g_use_bitrate set correctly... */
      main_upload(hDlg, config);
      main_mode(hDlg, config);
      main_upload(hDlg, config);
      break;

   case WM_NOTIFY :
      {
         NMHDR * n = (NMHDR*)lParam;

         if (n->code == NM_DBLCLK) {
             NMLISTVIEW * nmlv = (NMLISTVIEW*) lParam;

             main_download(hDlg, config);
             break;
         }

      break;
      }

   case WM_COMMAND :
      if (HIWORD(wParam) == BN_CLICKED) {

         switch(LOWORD(wParam)) {
		 case IDC_PROFILE_ADV :
			main_download(hDlg, config);
			adv_dialog(hDlg, config, profile_dlgs, sizeof(profile_dlgs)/sizeof(int));

			SendDlgItemMessage(hDlg, IDC_PROFILE, CB_SETCURSEL, config->profile, 0);
			main_mode(hDlg, config);
			break;

         case IDC_BITRATE_S :
            /* alternate between bitrate/desired_length metrics */
            main_download(hDlg, config);
            config->use_bitrate = !config->use_bitrate;
            main_mode(hDlg, config);
            main_upload(hDlg, config);
            break;

         case IDC_DEFAULTS :
            config_reg_default(config);
            SendDlgItemMessage(hDlg, IDC_PROFILE, CB_SETCURSEL, config->profile, 0);
            SendDlgItemMessage(hDlg, IDC_MODE, CB_SETCURSEL, config->mode, 0);
            main_mode(hDlg, config);
            main_upload(hDlg, config);
            break;

         case IDOK :
            main_download(hDlg, config);
            config->save = TRUE;
            EndDialog(hDlg, IDOK);
            break;

         case IDCANCEL :
            config->save = FALSE;
            EndDialog(hDlg, IDCANCEL);
            break;
         }
      } else if (HIWORD(wParam) == LBN_SELCHANGE &&
         (LOWORD(wParam)==IDC_PROFILE || LOWORD(wParam)==IDC_MODE)) {

         config->mode = SendDlgItemMessage(hDlg, IDC_MODE, CB_GETCURSEL, 0, 0);
         config->profile = SendDlgItemMessage(hDlg, IDC_PROFILE, CB_GETCURSEL, 0, 0);

         // Only bitrate mode is allowed for these two modes.
         if ((config->mode == RC_MODE_1PASS_CBR) || (config->mode == RC_MODE_2PASS2))
         {
             config->use_bitrate = 1;
             g_use_bitrate = config->use_bitrate;
         }

         if (!g_use_bitrate) {
            if (config->mode == RC_MODE_1PASS_VBR) 
                    SetDlgItemInt(hDlg, IDC_BITRATE, config->min_quant, FALSE);
//             set_dlgitem_int(hDlg, IDC_BITRATE, config->min_quant);
         }

         main_mode(hDlg, config);
         main_upload(hDlg, config);

      }else if (HIWORD(wParam)==EN_CHANGE && LOWORD(wParam)==IDC_BITRATE) {

         if (g_use_bitrate) {
            SendDlgItemMessage(hDlg, IDC_SLIDER, TBM_SETPOS, TRUE,
                  config_get_uint(hDlg, IDC_BITRATE, DEFAULT_MIN_KBPS) );
         } else if (config->mode == RC_MODE_1PASS_VBR) {
            SendDlgItemMessage(hDlg, IDC_SLIDER, TBM_SETPOS, TRUE,
                  config_get_uint(hDlg, IDC_BITRATE, DEFAULT_QUANT) );
         }
         main_download(hDlg, config);

      }else {
         return 0;
      }
      break;

   case WM_HSCROLL :
      if((HWND)lParam == GetDlgItem(hDlg, IDC_SLIDER)) {
         if (g_use_bitrate)
            SetDlgItemInt(hDlg, IDC_BITRATE, SendMessage((HWND)lParam, TBM_GETPOS, 0, 0), FALSE);
         else
            SetDlgItemInt(hDlg, IDC_BITRATE, SendMessage((HWND)lParam, TBM_GETPOS, 0, 0), FALSE);

         main_download(hDlg, config);
         break;
      }
      return 0;

   default :
      return 0;
   }

   return 1;
}


/* ===================================================================================== */
/* LICENSE DIALOG ====================================================================== */
/* ===================================================================================== */

static INT_PTR CALLBACK license_proc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
   switch (uMsg)
   {
   case WM_INITDIALOG :
      {
         HRSRC hRSRC;
         HGLOBAL hGlobal = NULL;
         if ((hRSRC = FindResource(g_hInst, MAKEINTRESOURCE(IDR_GPL), "TEXT"))) {
            if ((hGlobal = LoadResource(g_hInst, hRSRC))) {
               LPVOID lpData;
               if ((lpData = LockResource(hGlobal))) {
                  SendDlgItemMessage(hDlg, IDC_LICENSE_TEXT, WM_SETFONT, (WPARAM)GetStockObject(ANSI_FIXED_FONT), MAKELPARAM(TRUE, 0));
                  SetDlgItemText(hDlg, IDC_LICENSE_TEXT, lpData);
                  SendDlgItemMessage(hDlg, IDC_LICENSE_TEXT, EM_SETSEL, (WPARAM)-1, (LPARAM)0);
               }
            }
         }
         SetWindowLongPtr(hDlg, GWLP_USERDATA, (LONG_PTR)hGlobal);
      }
      break;

   case WM_DESTROY :
      {
         HGLOBAL hGlobal = (HGLOBAL)GetWindowLongPtr(hDlg, GWLP_USERDATA);
         if (hGlobal) {
            FreeResource(hGlobal);
         }
      }
      break;

   case WM_COMMAND :
      if (HIWORD(wParam) == BN_CLICKED) {
         switch(LOWORD(wParam)) {
         case IDOK :
         case IDCANCEL :
            EndDialog(hDlg, 0);
            break;
         default :
            return 0;
         }
         break;
      }
      break;

   default :
      return 0;
   }

   return 1;
}

/* ===================================================================================== */
/* ABOUT DIALOG ======================================================================== */
/* ===================================================================================== */

INT_PTR CALLBACK about_proc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
   switch (uMsg)
   {
   case WM_INITDIALOG :
      {
         char core[100];
         HFONT hFont;
         LOGFONT lfData;
         int major = 0;
         int minor = 0;
         int patch = 0;

         SetDlgItemText(hDlg, IDC_BUILD, VP80_BUILD);
         get_version_info(&major,&minor,&patch);
#ifdef _WIN64
         wsprintf(core, "VP8 VFW Video codec, 64bit version %d.%d.%d",
            major,
            minor,
            patch);
#else
         wsprintf(core, "VP8 VFW Video codec, version %d.%d.%d",
            major,
            minor,
            patch);
#endif
         SetDlgItemText(hDlg, IDC_CORE, core);

         wsprintf(core, "VP8 version %d.%d.%d",
            vpx_codec_version_major(),
            vpx_codec_version_minor(),
            vpx_codec_version_patch());

         SetDlgItemText(hDlg, IDC_SPECIAL_BUILD, core);

         hFont = (HFONT)SendDlgItemMessage(hDlg, IDC_WEBSITE, WM_GETFONT, 0, 0L);

         if (GetObject(hFont, sizeof(LOGFONT), &lfData)) {
            lfData.lfUnderline = 1;

            hFont = CreateFontIndirect(&lfData);
            if (hFont) {
               SendDlgItemMessage(hDlg, IDC_WEBSITE, WM_SETFONT, (WPARAM)hFont, 1L);
            }
         }

         SetClassLongPtr(GetDlgItem(hDlg, IDC_WEBSITE), GCLP_HCURSOR, (LONG_PTR)LoadCursor(NULL, IDC_HAND));
         SetDlgItemText(hDlg, IDC_WEBSITE, VP80_WEBSITE);
      }
      break;
   case WM_CTLCOLORSTATIC :
      if ((HWND)lParam == GetDlgItem(hDlg, IDC_WEBSITE))
      {
         SetBkMode((HDC)wParam, TRANSPARENT) ;
         SetTextColor((HDC)wParam, RGB(0x00,0x00,0xc0));
         return (INT_PTR)GetStockObject(NULL_BRUSH);
      }
      return 0;

   case WM_COMMAND :
      if (LOWORD(wParam) == IDC_WEBSITE && HIWORD(wParam) == STN_CLICKED)  {
         ShellExecute(hDlg, "open", VP80_WEBSITE, NULL, NULL, SW_SHOWNORMAL);
      }else if (LOWORD(wParam) == IDC_LICENSE) {
         DialogBoxParam(g_hInst, MAKEINTRESOURCE(IDD_LICENSE), hDlg, license_proc, (LPARAM)0);
      } else if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) {
         EndDialog(hDlg, LOWORD(wParam));
      }
      break;

   default :
      return 0;
   }

   return 1;
}



