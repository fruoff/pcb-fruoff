/* $Id$ */

/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *  Copyright (C) 1994,1995,1996 Thomas Nau
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *  Contact addresses for paper mail and Email:
 *  Thomas Nau, Schlehenweg 15, 88471 Baustetten, Germany
 *  Thomas.Nau@rz.uni-ulm.de
 *
 */


/* error and debug funtions
 * getpid() needs a cast to (int) to get rid of compiler warnings
 * on several architectures
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <stdarg.h>
#include <signal.h>
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <fcntl.h>

#include "global.h"

#include "data.h"
#include "error.h"
#include "file.h"
#include "gui.h"

#include "misc.h"

RCSID("$Id$");




/* ----------------------------------------------------------------------
 * some external identifiers
 */
extern int errno;		/* system error code */

#if !defined(HAVE_STRERROR)
extern int sys_nerr;		/* number of messages available from array */
#define USE_SYS_ERRLIST
#endif

/* the list is already defined for some OS */
#if !defined(__NetBSD__) && !defined(__FreeBSD__) && !defined(__linux__)
#ifdef USE_SYS_ERRLIST
extern char *sys_errlist[];	/* array of error messages */
#endif
#endif


/* ---------------------------------------------------------------------------
 * output of message in a dialog window or log window
 */
void
Message (char *Format, ...)
	{
	static int line = 1;
	va_list args;
	char s[1024];

	sprintf(s,"%d: ",line++);
	va_start (args, Format);
	vsprintf (s + strlen(s), Format, args);
	va_end (args);

	if (Settings.UseLogWindow)
		gui_log_append_string(s);
	else
		{
		gui_beep(Settings.Volume);
		gui_dialog_message(s);
		}
	}


/* ---------------------------------------------------------------------------
 * print standard 'open error'
 */
void
OpenErrorMessage (char *Filename)
{
gchar	*utf8	= NULL;

utf8_dup_string(&utf8, Filename);
#ifdef USE_SYS_ERRLIST
  Message (_("Can't open file\n"
	   "   '%s'\nfopen() returned: '%s'\n"),
	   utf8, errno <= sys_nerr ? sys_errlist[errno] : "???");
#else
  Message (_("Can't open file\n"
	   "   '%s'\nfopen() returned: '%s'\n"), utf8, strerror (errno));
#endif
g_free(utf8);
}

/* ---------------------------------------------------------------------------
 * print standard 'popen error'
 */
void
PopenErrorMessage (char *Filename)
{
gchar	*utf8	= NULL;

utf8_dup_string(&utf8, Filename);
#ifdef USE_SYS_ERRLIST
  Message (_("Can't execute command\n"
	   "   '%s'\npopen() returned: '%s'\n"),
	   utf8, errno <= sys_nerr ? sys_errlist[errno] : "???");
#else
  Message (_("Can't execute command\n"
	   "   '%s'\npopen() returned: '%s'\n"), utf8, strerror (errno));
#endif
g_free(utf8);
}

/* ---------------------------------------------------------------------------
 * print standard 'opendir'
 */
void
OpendirErrorMessage (char *DirName)
{
gchar	*utf8	= NULL;

utf8_dup_string(&utf8, DirName);
#ifdef USE_SYS_ERRLIST
  Message (_("Can't scan directory\n"
	   "   '%s'\nopendir() returned: '%s'\n"),
	   utf8, errno <= sys_nerr ? sys_errlist[errno] : "???");
#else
  Message (_("Can't scan directory\n"
	   "   '%s'\nopendir() returned: '%s'\n"), utf8, strerror (errno));
#endif
g_free(utf8);
}

/* ---------------------------------------------------------------------------
 * print standard 'chdir error'
 */
void
ChdirErrorMessage (char *DirName)
{
gchar	*utf8	= NULL;

utf8_dup_string(&utf8, DirName);
#ifdef USE_SYS_ERRLIST
  Message (_("Can't change working directory to\n"
	   "   '%s'\nchdir() returned: '%s'\n"),
	   utf8, errno <= sys_nerr ? sys_errlist[errno] : "???");
#else
  Message (_("Can't change working directory to\n"
	   "   '%s'\nchdir() returned: '%s'\n"), utf8, strerror (errno));
#endif
g_free(utf8);
}

/* ---------------------------------------------------------------------------
 * output of fatal error message
 */
void
MyFatal (char *Format, ...)
{
  va_list args;

  va_start (args, Format);

  /* try to save the layout and do some cleanup */
  EmergencySave ();
  fprintf (stderr, "%s (%i): fatal, ", Progname, (int) getpid ());
  vfprintf (stderr, Format, args);
  fflush (stderr);
  va_end (args);
  exit (1);
}

/* ---------------------------------------------------------------------------
 * catches signals which abort the program
 */
void
CatchSignal (int Signal)
{
  char *s;

  switch (Signal)
    {
    case SIGHUP:
      s = "SIGHUP";
      break;
    case SIGINT:
      s = "SIGINT";
      break;
    case SIGQUIT:
      s = "SIGQUIT";
      break;
    case SIGABRT:
      s = "SIGABRT";
      break;
    case SIGTERM:
      s = "SIGTERM";
      break;
    case SIGSEGV:
      s = "SIGSEGV";
      break;
    default:
      s = "unknown";
      break;
    }
  MyFatal ("aborted by %s signal\n", s);
}

