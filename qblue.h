/* header for Q-Blue (c) 1993, 1994 by Paul Kienitz, ALL RIGHTS RESERVED. */

/* In this file we have elements that vary depending on version or source */
/* file.  The stable elements, which can be pre-compiled, are in qbs.h:   */

#if defined(BETA) && !defined(__NO_PRAGMAS)
#  define __NO_PRAGMAS
#endif

#ifdef BETA
void _assert(char *, const char *, unsigned int);
#  define ASSERT(x) ((x) ? (void) 0 : _assert(#x, __FILE__, __LINE__))
#else
#  define ASSERT(x)
#endif

#include "qbs.h"
/* pulls in string.h, ctype.h, Paul.h, and bluewave.h */


#include <clib/exec_protos.h>
#ifdef DOS_DOS_H
#  include <clib/dos_protos.h>
#endif
#if defined(GRAPHICS_GFX_H) || defined(GRAPHICS_GFXBASE_H)
#  include <clib/graphics_protos.h>
#endif
#ifdef INTUITION_INTUITION_H
#  include <clib/intuition_protos.h>
#endif

#ifndef __NO_PRAGMAS
#  include <pragmas/exec_lib.h>
#  ifdef DOS_DOS_H
#    include <pragmas/dos_lib.h>
#  endif
#  if defined(GRAPHICS_GFX_H) || defined(GRAPHICS_GFXBASE_H)
#    include <pragmas/graphics_lib.h>
#  endif
#  ifdef INTUITION_INTUITION_H
#    include <pragmas/intuition_lib.h>
#  endif
#endif


/* some commonly used DOS-related functions and variables: */

#ifdef DOS_DOS_H
  long FileSize(BPTR lock);
  long RRead(BPTR hand, adr buf, long len);
  long SSeek(BPTR file, long position, long mode);

  import BPTR texthand, replylock;		/* used when pkt open */
  import struct FileInfoBlock *fib;		/* multi-purpose */

  typedef adr BHandle;

  BHandle BOpen(str path, bool write);
  long BClose(BHandle h);
  long BFlush(BHandle h);
  long BSeek(BHandle h, long offset, long mode);
  long BRead(BHandle h, APTR buf, ulong blocklen, ulong blocks);
  long BWrite(BHandle h, APTR buf, ulong blocklen, ulong blocks);
  long BGetline(BHandle h, str buf, ulong bufsize);

#  define DOSHANDLE(bhandle) (*((BPTR *) bhandle))

#  ifdef DOS_DOSEXTENS_H
    import struct Process *me;
#  endif
#endif DOS_DOS_H


/* some commonly used Intuition-related global variables: */

#ifdef INTUITION_INTUITION_H
  import struct Screen *scr;
  import struct Window *bgwin, *cwin, *cbitwin, *lawin, *ynwin;
  import struct TextFont *font;

  import struct Gadget readgag0, readgag1, readgag2, readgag3;
  import struct Gadget readgag4, readgag5, readgag6;
  import struct IntuiText bgt0, bgt1, bgt2w, bgt3r, bgt3re, bgt4, bgt5, bgt6;
  import struct Border upborder, upborderleft, upborderright, upcyclebor;
  import struct Border checkmarkno, checkmarkyes, defarrow;
#endif


#ifdef INTUITION_SGHOOKS_H
  import struct StringExtend stringex;
#endif

/* type of function pointer for displaying lines in a list window: */
typedef void (*ListLineFType)(short, struct Mess *, bool);
