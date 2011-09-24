/*****************************************************************************
 *
 * VP80 VFW FRONTEND based on XVid 1.2.2 VFW Codec 
 *  - Debug header  -
 *
 *  Copyright(C) 2002-2003 Anonymous <xvid-devel@xvid.org>
 *  Copied/Modified for VP8 Codec by Carl Eric Cod�re
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
 * $Id: debug.h,v 1.2 2004/03/22 22:36:25 edgomez Exp $
 *
 ****************************************************************************/

#ifndef _DEBUG_H_
#define _DEBUG_H_

#if defined(_DEBUG)
#include <stdio.h>	/* vsprintf */
#define DPRINTF_BUF_SZ  1024
static __inline void DPRINTF(char *fmt, ...)
{
	va_list args;
	char buf[DPRINTF_BUF_SZ];

	va_start(args, fmt);
	vsprintf(buf, fmt, args);
	OutputDebugString(buf);
}
#else
static __inline void DPRINTF(char *fmt, ...) { }
#endif

#endif /* _DEBUG_H_ */
