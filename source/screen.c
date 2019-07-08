/* screen shit for Q-Blue. */

#include <exec/memory.h>
#include <dos/dosextens.h>
#include <graphics/gfxmacros.h>
#include <graphics/gfxbase.h>
#include <graphics/displayinfo.h>
#include <intuition/sghooks.h>
#include <intuition/intuitionbase.h>
#include "qblue.h"
#include "pigment.h"
#include "semaphore.h"
#include "version.h"

#define POINTPALETTE

#ifdef TEST13
#  define CreateMsgPort() CreatePort(null, 0L)
#  define DeleteMsgPort   DeletePort
#  define ResetMenuStrip  SetMenuStrip
#endif

#ifdef __NO_PRAGMAS
#  pragma amicall(IntuitionBase, 0x330, SetWindowPointerA(a0,a1))
/* an exception to __NO_PRAGMAS because missing from Aztec c.lib */
#endif

#ifdef POINTPALETTE
#  define DARKPOINT    18
#  define PALELEVEL    3
#endif

#define SLIDERWIDTH    16

#define LB_SHADOWCOLOR 1
#define LB_UNDERCOLOR  6


void MakeBGslider(void);
void NukeBGslider(void);
void MakeBoxTop(void);
void MakeBotGadgets(void);
void NukeBotGadgets(void);
void StretchMenus(void);
bool AppendDotFont(str name);
long CheckSemaphoreOverlap(bool dodirs);
void WorkbenchRequesterArgs(str body, str options, ...);
void ClipShutdown(void);
void StopTimer(void);
StopTitleBarClock(void);
void UndoTitleBarMsg(bool rightnow);


import struct Menu mainu;
import struct Gadget ygagquote, ogagwaste, fakegagswatch;
import struct Border pupborder, upborderV, upborderR, underborder;
import struct Border ugreenfill, upynbor2, underborderV, underborderR;
import struct Border checkmarkno, checkmarkyes, checkthickenno, checkthickenyes;
import struct WhereWin cww, cbitww;
import struct TextAttr ta;
import long endtag;

import /* actually an extension of */ struct SignalSemaphore *qsem;
import struct QSemNode ourqsemnode;
import struct Conf *listcc;

import char screentitle[4][80], fontname[], rawtitle[];
import short under[4], ynupleft[10], yndownright[10];

import long hilicolor, labelcolor;
import ushort textop, winlines, extralines, whichtitle, laterfontwid;
import bool dos3, slidestifle, detested, onefigloaded, laterlace;


#ifdef POINTPALETTE
local long ppred = -1, ppgreen, ppblue, prepale;
#endif

bool lace, fourcolors, autoscroll, custom4color, customautoscroll;
bool foundbutbad, customode = false, lightbg = false;
bool rethinkmenus = false, backslide;
long backcolor, shadowcolor, undercolor, modeID = HIRES_KEY;
short shortwinlines, shortexbot, ourinstance;
ushort fontwid = 8, fakefight, checkspace, checkoff, checktall, nomwidth;
ushort tifight, tifontwid;
str oldtaskname;

APTR primordialwptr;

struct TextFont *font;

local struct TextFont *deffont;
local bool benchlace, lacesetonce = false;

#define WAITSPACE  72
#define RADIOSPACE 132
#define CHIPSPACE  (WAITSPACE + RADIOSPACE + RADIOSPACE)


/* Not exactly like Commodore's imagery: */
local ushort radio_off_data[66] =
{
   0x0000,0x0000,
   0x1ffe,0x0000,
   0x3000,0x0000,
   0x6000,0x0000,
   0x6000,0x0000,
   0x6000,0x0000,
   0x6000,0x0000,
   0x6000,0x0000,
   0x3000,0x0000,
   0x1000,0x0000,
   0x0000,0x0000,

   0x0000,0x0000,
   0x1ffe,0x0000,
   0x3ffe,0x0000,
   0x7fff,0x0000,
   0x7fff,0x0000,
   0x7fff,0x0000,
   0x7fff,0x0000,
   0x7fff,0x0000,
   0x3ffe,0x0000,
   0x1000,0x0000,
   0x0000,0x0000,

   0xffff,0xffff,
   0xfffe,0xffff,
   0xf000,0x7fff,
   0xe000,0x3fff,
   0xe000,0x3fff,
   0xe000,0x3fff,
   0xe000,0x3fff,
   0xe000,0x3fff,
   0xf000,0x7fff,
   0xf000,0xffff,
   0xffff,0xffff
};

local ushort radio_on_data[66] =
{
   0x0000,0x0000,
   0x0001,0x0000,
   0x0fff,0x8000,
   0x1fff,0xc000,
   0x1fff,0xc000,
   0x1fff,0xc000,
   0x1fff,0xc000,
   0x1fff,0xc000,
   0x0fff,0x8000,
   0x0fff,0x0000,
   0x0000,0x0000,

   0x0000,0x0000,
   0x0001,0x0000,
   0x0001,0x8000,
   0x01f0,0xc000,
   0x03f8,0xc000,
   0x03f8,0xc000,
   0x03f8,0xc000,
   0x01f0,0xc000,
   0x0001,0x8000,
   0x0fff,0x0000,
   0x0000,0x0000,

   0xffff,0xffff,
   0xe001,0xffff,
   0xcfff,0xffff,
   0x9e0f,0xffff,
   0x9c07,0xffff,
   0x9c07,0xffff,
   0x9c07,0xffff,
   0x9e0f,0xffff,
   0xcfff,0xffff,
   0xefff,0xffff,
   0xffff,0xffff
};

struct Image radio_on =
{
	0,0,			/* LeftEdge, TopEdge  */
	19,11,3,		/* Width, Height, Depth */
	&radio_on_data[0],	/* Pointer to Image data */
	7,0,			/* PlanePick, PlaneOnOff */
	NULL,			/* NextImage pointer */
};

struct Image radio_off =
{
	0,0,			/* LeftEdge, TopEdge  */
	19,11,3,		/* Width, Height, Depth */
	&radio_off_data[0],	/* Pointer to Image data */
	7,0,			/* PlanePick, PlaneOnOff */
	NULL,			/* NextImage pointer */
};


/* Copied from Commodore sources: */
local ushort WaitPointerImage[] = {
    0x0000, 0x0000,	/* space for vert. and horiz. start posn. */
    0x0400, 0x07C0,
    0x0000, 0x07C0,
    0x0100, 0x0380,
    0x0000, 0x07E0,
    0x07C0, 0x1FF8,
    0x1FF0, 0x3FEC,
    0x3FF8, 0x7FDE,
    0x3FF8, 0x7FBE,
    0x7FFC, 0xFF7F,
    0x7EFC, 0xFFFF,
    0x7FFC, 0xFFFF,
    0x3FF8, 0x7FFE,
    0x3FF8, 0x7FFE,
    0x1FF0, 0x3FFC,
    0x07C0, 0x1FF8,
    0x0000, 0x07E0,
    0x0000, 0x0000	/* reserved, must be NULL */
};

ushort *WaitPointer;		/* CHIP ram */


#define SHAR 10

/* this points to all the windows which use idcort as their IDCMP port, except */
/* for bgwin, which "owns" the port and isn't created by OpenShareWin: */
struct Window *(sharers[SHAR]) = { null /* ... */ };

short countoff = 0;
ulong oldflags[SHAR];

char pubscrname[20] = "";

char yellbanner[] =
"                   PLEASE CLOSE ALL WINDOWS ON THIS SCREEN!                  ";

/* char justa[] = "                          ---  Just a second...  ---"; */

ubyte title[80], titletail[] =
		" Blue Wave & QWK mail reader by Paul Kienitz";


/* detail, block, bg_text, shine, shadow, fill, fill_text, bg, hilight_text */
ushort penzez[]     = { 1, 4, 5, 1, 2, 5, 0, 0, 1, ~0 };
ushort lb_penzez[]  = { 6, 4, 1, 6, 1, 5, 1, 0, 6, ~0 };
ushort penzez4[]    = { 1, 2, 3, 1, 2, 3, 0, 0, 1, ~0 };
ushort lb_penzez4[] = { 3, 2, 1, 3, 1, 2, 1, 0, 3, ~0 };


struct NewScreen news = {
    0, 0, 640, STDSCREENHEIGHT, 3, 1, 4, HIRES, CUSTOMSCREEN,
    /***  &ta  ***/ null, title, null, null
};


/* black, white, green, red, blue, cyan, magenta, yellow */
ushort palette[8] = { 0x000, 0xEEE, 0x070, 0xD00, 0x11A, 0x0EE, 0xF5F, 0xFF0 };
/* black, white, blue, light green */
ushort palette4[4] = { 0x000, 0xFFF, 0x44F, 0xAF0 };

#ifdef SOMEDAY
/* gray, black, white, blue */
ushort lb_palt4[4] = { 0xAAA, 0x000, 0xFFF, 0x66F };
/* gray, black, green, red, blue, cyan, white, yellow */
ushort lb_palt[8] = { 0xAAA, 0x000, 0x090, 0xC00, 0x33A, 0x0FF, 0xFFF, 0xEE0 };
#endif


local struct NewWindow newbgw = {
    0, 12, 640, 188, 0, 1,
    IDCMP_MOUSEBUTTONS | IDCMP_MOUSEMOVE | IDCMP_GADGETUP | IDCMP_GADGETDOWN
		| IDCMP_MENUPICK | IDCMP_RAWKEY | IDCMP_INTUITICKS
		| IDCMP_INACTIVEWINDOW,
    WFLG_BORDERLESS | WFLG_BACKDROP | WFLG_SMART_REFRESH | WFLG_ACTIVATE
		| WFLG_REPORTMOUSE,
    null, null, null, null, null, 0, 0, 0, 0, CUSTOMSCREEN
};


local struct {
    long t1;
    ustr s;
    long t2, d2, t3, d3, t4, d4, t5;
    adr chain;
} sharetags = { WA_ScreenTitle, screentitle[0],
		WA_MouseQueue, 2,
		WA_RptQueue, 1,
		WA_AutoAdjust, TRUE,
		TAG_MORE, null };


struct Screen *scr;
struct Window *bgwin;
struct RastPort *bgrp;
struct MsgPort *idcort;

adr TimerBase, DiskfontBase, LayersBase, ConsoleDevice;
struct IntuitionBase *IntuitionBase = null;
struct GfxBase *GfxBase;

ubyte *chiparea;

struct timerequest tiq;
struct IOStdReq condev;
struct MsgPort *tipo;
bool menuset = false, holdopen = false;

local bool timeropen = false;
local bool lastiffed = false, bgstiffed = false;

local struct Requester lastifle = { 0 }, bgstifle = { 0 };


short division[4] = { 20, 2, 20, 10 };

short arrowextra[4] = { 6, 9, 6, 3 };

short arrow[22] = { 14, 9, 13, 9, 13, 10, 7, 10, 7, 2, 13, 2,
		    13, 7, 11, 5, 16, 5, 14, 7, 14, 3 };

short cydownright[10] = { 1, 13, 104, 13, 104, 1, 105, 0, 105, 13 };

short cyupleft[10] = { 0, 0, 0, 13, 1, 12, 1, 0, 104, 0 };


struct Border arrowextraborder = {
    0, 0, /* SHADOWCOLOR */ 5, 0, JAM2, 2, arrowextra, null
};

struct Border arrowborder = {
    0, 0, /* SHADOWCOLOR */ 5, 0, JAM2, 11, arrow, &arrowextraborder
};

struct Border updivbor2 = {
    1, 0, SHINECOLOR, 0, JAM2, 2, division, &arrowborder
};

struct Border updivbor = {
    0, 0, SHADOWCOLOR, 0, JAM2, 2, division, &updivbor2
};

struct Border upcyclebor2 = {
    0, 0, SHADOWCOLOR, 0, JAM2, 5, cydownright, &updivbor
};

struct Border upcyclebor = {
    0, 0, SHINECOLOR, 0, JAM2, 5, cyupleft, &upcyclebor2
};


struct RastPort bgrpblack;	/* for clearing background areas optimizedly */


local char sgagwork[SIGNATURELEN + 1];

ulong EscapeHook(struct Hook *h, struct SGWork *sgw, ulong *msg);
#pragma intfunc(EscapeHook(a0, a2, a1))

struct Hook EscapeHookHook = {
    { null, null }, &EscapeHook, null, null
};

struct StringExtend stringex = {
    null, { OFFTXCOLOR, OFFBGCOLOR }, { ONTXCOLOR, ONBGCOLOR }, 0L,
    &EscapeHookHook, sgagwork  /* , zeroes */
};


/* ------------------------------------------------------------------------- */


/* #pragma intfunc above -- called in input.device context! */

#ifdef C_STRHOOK

ulong EscapeHook(struct Hook *h, register struct SGWork *sgw, ulong *msg)
{
    if (*msg == SGH_KEY && sgw->IEvent->ie_Class == IECLASS_RAWKEY)
	if (sgw->Code == ESC)
	    sgw->Actions = SGA_END | SGA_REUSE;
	else if (_tolower(sgw->Code) == 'v'
			&& sgw->IEvent->ie_Qualifier & IEQUALIFIER_RCOMMAND) {
	    sgw->Code = 'V';		/* in case it was lowercase */
	    sgw->Actions = SGA_END;
	}
    return (ulong) -1;
}
/* Test for compressors signature gag and allow only hex+space+'?' ?  Nah. */

#else
#  asm

	xdef	_EscapeHook	; a0 -> Hook, a2 -> SGWork, a1 -> ulong
_EscapeHook:
	cmp.l		#1,(a1)
	bne.s		dehook
	 move.l		20(a2),a0	; a0 = SGWork->IEvent
	 cmp.b		#1,4(a0)	; IEvent->ie_Class == IECLASS_RAWKEY ?
	 bne.s		dehook
	  move.w	24(a2),d0	; d0 = SGWork->Code
	  cmp.w		#27,d0		; d0 == Esc ?
	  bne.s		clipq
	   move.l	#$A,30(a2)	; SGWork->Actions = SGA_END | SGA_REUSE
	   bra.s	dehook
clipq:	  or.w		#$20,d0		; d0 = _tolower(d0)
	  cmp.w		#'v',d0
	  bne.s		dehook
	   btst.b	#7,9(a0)	; IEvent->IE_Qualifier & *_RCOMMAND ?
	   beq.s	dehook
	    move.w	#'V',24(a2)	; SGWork->Code = 'V'
	    move.l	#$2,30(a2)	; SGWork->Actions = SGA_END
dehook:	moveq		#-1,d0
	rts

#  endasm
#endif


#ifdef C_XACTUAL

short XActual(register short x)
{
//    return ((x *= fontwid) < 0 ? x - 4 : x + 4) / 8;
    return (x * fontwid + 4) >> 3;
    /* we add 4 even when x is negative because shifting right by 3 to    */
    /* divide always rounds negatively, not toward zero.  To make it be   */
    /* exactly symmetrical we should add 3 when negative, but this only   */
    /* affects the case where rounding up or down are equally inaccurate. */
}

#else
#  asm
; This asm version was done for no good reason, but as long as it's there...

	xdef	_XActual	; d0 = short parm
	xref	_fontwid
_XActual:
	muls		_fontwid,d0
;;	blt.s		xneg
;;	 addq.w		#4,d0
;;	 bra.s		xnn
;;xneg:	 subq.w		#4,d0
;;xnn:	asr.w		#3,d0
        addq.w		#4,d0
	asr.w		#3,d0
	rts

#  endasm
#endif


short XTiActual(register short x)	/* used only for menu layout so far */
{
//    return ((x *= tifontwid) < 0 ? x - 4 : x + 4) / 8;
    return (x * tifontwid + 4) >> 3;
}


short XNominal(register short x)
{
    return ((x <<= 4) < 0 ? x - fontwid : x + fontwid) / (fontwid << 1);
}


short GadgetConstancy(struct Gadget *gg)
{
    struct Border *gb = gg->GadgetRender;
    if (gg->GadgetType & GTYP_STRGADGET)
	return 0;				/* string gadgets */
    else if (gb == &upcyclebor)
	return 26;				/* cycle gadgets */
    else if (gb == &upborder || gb == &pupborder || gb == &upborderV ||
				gb == &upborderR || gb->NextBorder == &upborder)
	return 8;				/* command gadgets */
    else
	return gg->Width;		/* checkmarks and odd misfits */
}


void FixShine(struct Gadget *example)
{
    struct Border *bb = (adr) example->GadgetRender;
    short *xy = bb->XY, *xyz = bb->NextBorder->XY;
    short fudge = GadgetConstancy(example);
    short wid = XActual(example->Width - fudge) + fudge;
    short margin = (example->GadgetType & GTYP_GTYPEMASK) == GTYP_PROPGADGET;

    xy[3] = xyz[1] = xyz[3] = xyz[9] = example->Height + margin - 1;
    xy[5] = xy[3] - 1;
    xy[8] = xyz[2] = xyz[4] = wid - 2 * (1 - margin);
    xyz[6] = xyz[8] = xy[8] + 1;
    if (example != &fakegagswatch)
	bb = bb->NextBorder;
    bb->FrontPen = shadowcolor;
    if (example != &fakegagswatch) {
	bb = bb->NextBorder;
	if (bb)
	    bb->FrontPen = (bb == &updivbor ? shadowcolor : undercolor);
    }
}


void FixCycles(struct Gadget *example)
{
    short offset = fight < 10;

    FixShine(example);
    division[3] = fight + 2;
    arrow[5] = arrow[7] = fight + 1 + offset;
    arrowextra[1] = arrow[1] = arrow[3] = fight + offset;
    offset = (fight > 8 ? (fight > 11 ? 2 + (fight > 13) : 1) : 0);
    arrow[13] = arrow[19] = 6 + offset;
    arrow[15] = arrow[17] = 4 + offset;
    offset = fight > 11;
    arrow[9] = arrow[11] = 2 + offset;
    arrow[21] = arrowextra[3] = 3 + offset;
}


void FixStringGad(struct Gadget *gg, struct Window *ww)
{
    char tempstring[SIGNATURELEN], *tss;
    struct StringInfo *si = (adr) gg->SpecialInfo;
    struct Gadget *ggn = gg->NextGadget;
#ifdef STRING_LINER
    struct Border *bbb = (adr) gg->GadgetRender,
			*bb = bbb->NextBorder->NextBorder,
			*bbbb = bb->NextBorder->NextBorder;
    short *box4 = bbb->XY, *box5 = bbb->NextBorder->XY;
#else
    struct Border *bb = (adr) gg->GadgetRender,
			*bbbb = bb->NextBorder->NextBorder;
#endif
    short *box0 = bb->XY, *box1 = bb->NextBorder->XY, *box2 = bbbb->XY,
			*box3 = bbbb->NextBorder->XY;
    short wid = gg->Width / fontwid + 1;
    short i = gg->Width + 2, l = strlen(si->Buffer);

    gg->Height = fight;
    if (gg->Flags & GFLG_RELWIDTH)
	i += ww->Width;
/* We create our borders with their dimensions not filled in.  We have */
/* to set all the points in the border, if they're not already set.    */
    if (!box0[0]) {
	box1[6] = box1[8] = -6;
	box1[2] = box1[4] = box3[0] = -5;
	box2[6] = box2[8] = -4;
	box0[0] = box1[1] = box1[3] = box1[9] = box2[2] = box2[4] =box3[7] = -3;
	box0[7] = box2[1] = box2[3] = box2[9] = box3[5] = -2;
	box0[5] = -1;
#ifdef STRING_LINER
	box4[0] = box4[2] = -2;
	box4[3] = box4[5] = box5[1] = box5[4] = box5[6] = box5[7] = -1;
#endif
    }
    if (box2[5] != fight) {
	box2[5] = fight;
	box0[1] = box0[3] = box0[9] = box1[5] = box2[7] = fight + 1;
	box1[7] = box3[1] = box3[3] = box3[9] = fight + 2;
#ifdef STRING_LINER
	box4[1] = box4[7] = box5[3] = box5[5] = fight;
#endif
    }
    if (box0[2] != i) {
	box0[2] = box0[4] = box2[0] = i;
	box0[6] = box0[8] = i + 1;
	box1[0] = box3[2] = box3[4] = i + 2;
	box3[6] = box3[8] = i + 3;
#ifdef STRING_LINER
	box4[4] = box4[6] = i - 2;	/* == gg->Width except when RELWIDTH */
	box5[0] = box5[2] = i - 1;
#endif
    }
    bbbb->FrontPen = bbbb->NextBorder->FrontPen = shadowcolor;
#ifdef STRING_LINER
    bbb->FrontPen = bbb->NextBorder->FrontPen = OFFBGCOLOR;
#endif
    si->DispPos = 0;				/* Intuition will fix */
    if (gg->Flags & GFLG_DISABLED || l >= wid - 1)
	si->BufferPos = 0;
    else
	si->BufferPos = l;
/* we refresh the gadget with all trailing space filled with blanks,  */
/* otherwise Intuition messes up the rendering after the end of the   */
/* string, when combining nonstandard colors with a ghosting pattern. */
    strcpy(tempstring, si->Buffer);
    for (i = l; i < wid; i++)
	tempstring[i] = ' ';
    if (wid >= l)
	tempstring[wid] = 0;
    tss = si->Buffer;
    si->Buffer = tempstring;
    AddGadget(ww, gg, 0);
    if (gg->Flags & GFLG_DISABLED)
	GhostOn(ww);
    RefreshGList(gg, ww, null, 1);
    si->Buffer = tss;
    if (gg->Flags & GFLG_DISABLED)
	GhostOff(ww);
    else {
/* We do another refresh with no actual rendering here, because this lets */
/* the internals know that the length of the string has been changed.     */
	SetWrMsk(ww->RPort, 0);
	RefreshGList(gg, ww, null, 1);
	SetWrMsk(ww->RPort, -1);
    }
    RemoveGadget(ww, gg);
    gg->NextGadget = ggn;
}


#ifdef EXAMINE_FONT_TYPE

bool font_ibm, font_a8;


/* This returns two bytes representing the which'th character of the font.   */
/* The high byte is the second row of pixels below the top, and the low byte */
/* is the row above the baseline row.  Zero if no glyph for this number.     */

/* OBSOLETE??  ***  OBSOLETE??  ***    won't work for fontwid != 8 */

local ushort CoreSample(short which)
{
    ushort ret, off, bas;

    if (which < font->tf_LoChar || which > font->tf_HiChar)
	return 0;
    which -= font->tf_LoChar;
    off = (((ushort *) font->tf_CharLoc)[which << 1] >> 3) + font->tf_Modulo;
    ret = ((ubyte *) font->tf_CharData)[off] << 8;
    if (font->tf_Baseline >= font->tf_YSize)
	bas = font->tf_YSize - 2;
    else
	bas = font->tf_Baseline - 2;
    off += bas * font->tf_Modulo;
    ret |= ((ubyte *) font->tf_CharData)[off];
    return ret;
}


void ScrutinizeFont(void)
{
    ushort c5, c219, c255, fl = font->tf_LoChar << 1, *clc = font->tf_CharLoc;
    bool dif = clc[256 - fl] != clc[288 - fl];

    /* IBM: char 5 has bits set in some middle byte, char 219 is all ones.   */
    /* A8: no value for chars 5 and 146, char 255 has bits.  dif also tests. */
    font_a8 = font_ibm = false;
    c5 = CoreSample(5);
    c219 = CoreSample(219);
    c255 = CoreSample(255);
    if (dif && c219 == 0xFFFF && c5 & 0xFF)
	font_ibm = true;
    else if (!dif && !c5 && c255 & 0xFF)
	font_a8 = true;
}

#endif EXAMINE_FONT_TYPE


bool CoolFont(struct TextFont *ff, short height, short scrheight)
{
    return ff->tf_YSize == height && ff->tf_XSize >= 5
			&& ff->tf_XSize <= 16 && height <= scrheight / 12
			&& ff->tf_LoChar <= 32 && ff->tf_HiChar >= 126
			&& !(ff->tf_Flags & FPF_PROPORTIONAL)
			&& ff->tf_Flags & (FPF_ROMFONT | FPF_DISKFONT);
}			/* make sure it's not a  ^^ scaled bitmap ^^ */


struct TextFont *FindFont(struct TextAttr *fa, short tall)
{
    struct TextFont *ff;
    bool faulted = false;

    if (!fa->ta_Name[0]) {
	if ((deffont->tf_XSize == fontwid || deffont->tf_XSize == 8) &&
				(benchlace || deffont->tf_YSize < 11)) {
	    strcpy(fa->ta_Name, deffont->tf_Message.mn_Node.ln_Name);
	    fa->ta_YSize = deffont->tf_YSize;
	    faulted = true;
	} else {
	    strcpy(fa->ta_Name, "topaz.font");
	    fa->ta_YSize = 8;
	}
    }
    fa->ta_Style = 0;
    foundbutbad = false;
    fa->ta_Flags = FPF_DESIGNED;
/* avoid looking on disk before we know it's necessary, or shit loads twice: */
    ff = OpenFont(fa);
    if (ff && !CoolFont(ff, fa->ta_YSize, tall)) {
	CloseFont(ff);
	ff = null;
    }
    if (!ff) {
	if (faulted) {	/* we must not return null if given a blank name */
	    strcpy(fa->ta_Name, "topaz.font");
	    fa->ta_YSize = 8;
	    return OpenFont(fa);	/* guaranteed to succeed */
	}
	fa->ta_Flags = FPF_DESIGNED | FPF_DISKFONT;
	if (DiskfontBase = OpenL("diskfont")) {
	    ff = OpenDiskFont(fa);
	    CloseLibrary(DiskfontBase);
	    if (ff && !CoolFont(ff, fa->ta_YSize, tall)) {
		CloseFont(ff);
		ff = null;
		foundbutbad = true;
	    }
	}
    }
    return ff;
}


struct TextFont *GetFont(short tall)
{
    struct TextFont *ff;
    str errorkind = "found", recoverykind = "system default font";
    char failedname[32];
    long failedsize;

    if (!ta.ta_Name)
	ta.ta_Name = fontname;
    if (!ta.ta_Name[0])
	errorkind = null, ff = null;
    else if (!(ff = FindFont(&ta, tall))) {
	if (foundbutbad)
	    errorkind = "usable";
	else if (!strchr(ta.ta_Name, '.') && AppendDotFont(ta.ta_Name))
	    return GetFont(tall);
    }
    if (!ff) {
	strncpy0(failedname, ta.ta_Name, 31);
	failedsize = ta.ta_YSize;
	ta.ta_Name[0] = 0;
	ff = FindFont(&ta, tall);	/* will open a default */
	if (ff->tf_YSize == 8 && !strcmp("topaz.font",
					ff->tf_Message.mn_Node.ln_Name))
	    recoverykind = "topaz.font size 8";
	if (errorkind)
	    WorkbenchRequesterArgs("Font %s size %ld not\n%s; using %s",
					"Okay", failedname, failedsize,
					errorkind, recoverykind);
    }
    return ff;
}


/* Not sure how cool this function actually is... can't find any */
/* definite documentation of the fields in struct Layer. */

bool IsInFrontOf(struct Window *wa, struct Window *wb)
{
    struct Layer *la = wa->WLayer, *lb = wb->WLayer;
    struct Layer *aa = la->front, *bb = lb->front;
    bool ret = false;

    if (wa && !wb)
	return true;
    if (!wa)
	return false;
    LockLayerInfo(&scr->LayerInfo);
    while (aa || bb) {
	if (aa == lb) {
	    ret = false;
	    break;
	}
	if (bb == la) {
	    ret = true;
	    break;
	}
	if (aa)
	    aa = aa->front;
	if (bb)
	    bb = bb->front;
    }
    UnlockLayerInfo(&scr->LayerInfo);
    return ret;
}


void SetAllSharedWindowTitles(str title)
{
    short i;
    for (i = 0; i < SHAR; i++)
	if (sharers[i])
	    SetWindowTitles(sharers[i], (ustr) -1, title);
}


bool AnySharedPresent(void)
{
    short i;
    for (i = 0; i < SHAR; i++)
	if (sharers[i] && sharers[i] != bgwin)
	    return true;
    return false;
}


bool ForeignersPresent(void)	/* true if somebody else's window open on scr */
{
    struct Window *ww;
    short i;

    LockLayerInfo(&scr->LayerInfo);
    for (ww = scr->FirstWindow; ww; ww = ww->NextWindow) {
	for (i = 0; i < SHAR; i++)
	    if (sharers[i] == ww)
		goto continue2;
	UnlockLayerInfo(&scr->LayerInfo);
	return true;
      continue2:
	;
    }
    UnlockLayerInfo(&scr->LayerInfo);
    return false;
}


void CloseDisplay(void)
{
    short ticker;

    StopTitleBarClock();
    StopTimer();
    ClipShutdown();
    if (timeropen)
	CloseDevice((adr) &tiq);
    if (tipo)
	DeleteMsgPort(tipo);
    tipo = NULL;	/* make sure UpdateClock() won't crash if called */
    if (menuset)
	ClearMenuStrip(bgwin);
    if (bgwin) {
	me->pr_WindowPtr = detested ? primordialwptr : null;
	NukeBotGadgets();
	NukeBGslider();
	CloseWindow(bgwin);
	if (sharers[0] == bgwin)
	    sharers[0] = null;
    }
    if (scr) {
	if (ForeignersPresent()) {
	    struct RastPort *r = &scr->RastPort;
	    ticker = 3;
	    ScreenToFront(scr);
	    SetRGB4(&scr->ViewPort, 1, 15, 15, 15);	/* white */
	    SetRGB4(&scr->ViewPort, 3, 15, 0, 0);	/* on red */
	    do {
		if (!(ticker = (ticker + 1) & 3)) {
		    SetAPen(r, 1);
		    SetBPen(r, 3);
		    Move(r, 0, font->tf_Baseline + 1);
		    Text(r, yellbanner, 77);
		}
		Delay(20);
	    } while (ForeignersPresent());
	}
	CloseScreen(scr);
    }
    if (font)
	CloseFont(font);
    if (chiparea)
	FreeMem(chiparea, CHIPSPACE);
    if (ConsoleDevice)
	CloseDevice((struct IORequest *) &condev);
    if (!holdopen) {
	if (IntuitionBase)
	    CloseLibrary((struct Library *) IntuitionBase);
	if (GfxBase)
	    CloseLibrary((struct Library *) GfxBase);
	if (LayersBase)
	    CloseLibrary(LayersBase);
	IntuitionBase = (adr) GfxBase = (adr) LayersBase = null;
    }
    ConsoleDevice = (adr) chiparea = (adr) font
			= (adr) scr = (adr) bgwin = (adr) tipo = null;
    menuset = timeropen = false;
}


struct Screen *OpenPubQScreen(void)
{
    long screrr;
    struct Screen *ss = null;
    ushort *penz;

    ObtainSemaphore(qsem);
    if ((ourinstance = ourqsemnode.instancenum) < 0)
	ourinstance = 0;
    do {
	sprintf(pubscrname, "Q-BLUE%lu", (ulong) ourinstance);
	if (!ourinstance) {
	    pubscrname[6] = 0;
	    sprintf(title, "%s: %s", rawtitle, titletail);
	} else
	    sprintf(title, "(%lu) %s:  %s", (ulong) ourinstance,
				rawtitle, titletail);
	if (CheckSemaphoreOverlap(false) & (SOF_PUBSCRNAME | SOF_INSTANCENUM)) {
	    screrr = OSERR_PUBNOTUNIQUE;
	    ourinstance++;
	    continue;
	}
	penz = lightbg ? (fourcolors ? lb_penzez4 : lb_penzez)
			: (fourcolors ? penzez4 : penzez);
	news.DetailPen = penz[0];
	news.BlockPen = penz[1];
#ifndef TEST13
	if (!(ss = OpenScreenTags(&news, SA_ErrorCode, &screrr, SA_PubName,
				pubscrname,  SA_AutoScroll, (ulong) autoscroll,
				TAG_SKIP, (long) !(customode && modeID),
				SA_DisplayID, modeID, SA_Overscan, OSCAN_TEXT,
				SA_Pens, penz, TAG_DONE))) {
	    if (screrr == OSERR_UNKNOWNMODE)
		customode = false;
	    else
		ourinstance++;
	}
#else
	ss = OpenScreen(&news);
	screrr = 0;
#endif
    } while (ourinstance <= 99 && !ss && (screrr == OSERR_PUBNOTUNIQUE
				|| screrr == OSERR_UNKNOWNMODE));
    if (ourqsemnode.instancenum < 0) {
	ourqsemnode.instancenum = ourinstance;		/* only set ONCE. */
	oldtaskname = me->pr_Task.tc_Node.ln_Name;
	me->pr_Task.tc_Node.ln_Name = pubscrname;
    } else
	ourinstance = ourqsemnode.instancenum;
    pubscrname[19] = '!';	/* mark QSemNode as having non-1.0 fields */
    ReleaseSemaphore(qsem);
#ifndef TEST13
    if (ss)
	PubScreenStatus(ss, 0);
#endif
    return ss;
}


bool OpenDisplay(void)
{
    short loud, tall, botneed;
    bool oldhold;
    struct DimensionInfo dii;
    struct Screen *bench;

    if (!IntuitionBase)
	IntuitionBase = OpenL("intuition");
    if (!GfxBase)
	GfxBase = OpenL("graphics");
    if (!LayersBase)
	LayersBase = OpenL("layers");
    if (!OpenDevice("console.device", -1, (struct IORequest *) &condev, 0))
	ConsoleDevice = condev.io_Device;
    else
	ConsoleDevice = null;
    if (tipo = CreateMsgPort()) {
	tiq.tr_node.io_Message.mn_ReplyPort = tipo;
	if (!OpenDevice("timer.device", UNIT_VBLANK, (adr) &tiq, 0L)) {
	    TimerBase = tiq.tr_node.io_Device;
	    timeropen = true;
	}
    }
    if (!modeID)
	customode = false;
    if (customode)
	if (!GetDisplayInfoData(null, (adr) &dii, sizeof(dii),
				DTAG_DIMS, modeID) || dii.MaxDepth < 2)
	    customode = false;
	else {
	    if (dii.MaxDepth == 2)
		fourcolors = true;
	    else
		fourcolors = custom4color;
	    tall = 1 + dii.TxtOScan.MaxY - dii.TxtOScan.MinY;
	    loud = 1 + dii.TxtOScan.MaxX - dii.TxtOScan.MinX;
	    autoscroll = customautoscroll;
	}
    deffont = GfxBase->DefaultFont;
#ifdef TEST13
    bench = (adr) OpenWorkBench();
    benchlace = bench && bench->Height >= 400;
#else
    bench = LockPubScreen("Workbench");
    benchlace = bench && bench->Height >= 400;
    UnlockPubScreen(null, bench);
#endif
    if (!onefigloaded && !lacesetonce)
	lace = laterlace = benchlace, lacesetonce = true;
    if (!customode) {
	tall = GfxBase->NormalDisplayRows << lace;
	loud = GfxBase->NormalDisplayColumns;
    }
    if (IntuitionBase && GfxBase && LayersBase && ConsoleDevice && timeropen
		&& (chiparea = AllocCP(CHIPSPACE)) && (font = GetFont(tall))) {
	WaitPointer = (ushort *) chiparea;
	memcpy(WaitPointer, WaitPointerImage, WAITSPACE);
	radio_on.ImageData = (ushort *) (chiparea + WAITSPACE);
	memcpy(radio_on.ImageData, radio_on_data, RADIOSPACE);
	radio_off.ImageData = (ushort *) (chiparea + WAITSPACE + RADIOSPACE);
	memcpy(radio_off.ImageData, radio_off_data, RADIOSPACE);
#ifdef EXAMINE_FONT_TYPE
	ScrutinizeFont();
#endif
	stringex.Font = font;
	fight = font->tf_YSize;
	laterfontwid = fontwid = font->tf_XSize;
	tifight = GfxBase->DefaultFont->tf_YSize;
	tifontwid = GfxBase->DefaultFont->tf_XSize;
	if (fourcolors) {
	    news.Depth = 2;
	    backcolor = 2 + lightbg;
	    hilicolor = 3;
	    labelcolor = 0;
	} else {
	    news.Depth = 3;
	    backcolor = 4;
	    hilicolor = 2;
	    labelcolor = 6;
	}
	if (lightbg)
	    shadowcolor = LB_SHADOWCOLOR, undercolor = LB_UNDERCOLOR;
	else
	    shadowcolor = SHADOWCOLOR, undercolor = UNDERCOLOR;
	ygagquote.Height = readgag0.Height = fight + 5;
	FixCycles(&ygagquote);		/* adjusts all cycle borders */
	FixShine(&readgag0);		/* all command button borders */
	underborderR.LeftEdge = underborder.LeftEdge + fontwid;  
	underborderV.LeftEdge = underborder.LeftEdge + 2 * fontwid;  
	under[2] = XActual(16) + 4;
	under[0] = XActual(7) + 4 + (fontwid >= 13);	/* fix slippage */
	if (lace)
	    news.ViewModes |= LACE;
	else
	    news.ViewModes &= ~LACE;
	nomwidth = fontwid * 80;
	backslide = loud < nomwidth || loud >= nomwidth + SLIDERWIDTH;
	news.Width = nomwidth + backslide * SLIDERWIDTH;
	if (loud < news.Width)
	    news.LeftEdge = 0;
	else
	    news.LeftEdge = (ushort) (loud - news.Width) >> 1;
	if (!customode)
	    autoscroll = loud < news.Width;
	if (scr = OpenPubQScreen()) {
#ifdef SOMEDAY
	    if (fourcolors)
		LoadRGB4(&scr->ViewPort, lightbg ? lb_palt4 : palette4, 4);
	    else
		LoadRGB4(&scr->ViewPort, lightbg ? lb_palt : palette, 8);
#else
	    if (fourcolors)
		LoadRGB4(&scr->ViewPort, palette4, 4);
	    else
		LoadRGB4(&scr->ViewPort, palette, 8);
#endif
#ifdef POINTPALETTE
	    prepale = GetRGB4(scr->ViewPort.ColorMap, DARKPOINT);
#endif
	    fakefight = scr->Height / 18;	/* 200 => 11 */
	    if (fakefight > fight)
		fakefight = fight;
	    checktall = lace ? 13 : 11;
	    if (fakefight <= checktall) {
		checkspace = checktall + 6;
		checkoff = 0;
	    } else {
		checkspace = fakefight + 6;
		checkoff = (fakefight - checktall) / 2;
	    }
	    yndownright[1] = yndownright[3] = yndownright[9]
					= ynupleft[3] = checktall - 1;
	    ynupleft[5] = checktall - 2;
	    checkmarkno.TopEdge = checkthickenno.TopEdge = checkmarkyes.TopEdge
					= checkthickenyes.TopEdge = lace;
	    checkmarkyes.FrontPen = checkthickenyes.FrontPen = undercolor;
	    upynbor2.FrontPen = shadowcolor;
	    newbgw.DetailPen = lightbg ? 6 : 0;
	    newbgw.Screen = scr;
	    newbgw.TopEdge = tifight + 3;
	    newbgw.Width = scr->Width;
	    newbgw.Height = scr->Height - newbgw.TopEdge;
	    if (customode && modeID)
		lace = scr->Height >= 400;		/**** appropriate?? */
	    if (bgwin = OpenWindow(&newbgw)) {
		primordialwptr = me->pr_WindowPtr;
		me->pr_WindowPtr = bgwin;
		bgrp = bgwin->RPort;
		bgrpblack = *bgrp;		/* rastport just for erasing */
		SetAPen(&bgrpblack, 0);
		SetBPen(&bgrpblack, 0);
		SetDrMd(&bgrpblack, JAM2);
		SetBPen(bgrp, 0);	    /* restore whenever it's changed */
		sharers[0] = bgwin;
		SetFont(bgrp, font);
		botneed = fight + 8;
		textop = 4 * fight + 9;
		loud = bgwin->Height - textop - botneed;
		winlines = shortwinlines = loud / fight;
		shortexbot = textop + fight * winlines;
		extralines = loud - winlines * fight;
		idcort = bgwin->UserPort;
		MakeBoxTop();
		slidestifle = true;
		MakeBotGadgets();
		slidestifle = false;
		if (backslide)
		    MakeBGslider();
		StretchMenus();
		SetMenuStrip(bgwin, &mainu);
		menuset = true;
		return true;
	    }
	}
    }
    oldhold = holdopen;
    holdopen = true;
    CloseDisplay();
    holdopen = oldhold;
    if (IntuitionBase && !holdopen) {
	DisplayBeep(null);
	CloseLibrary((struct Library *) IntuitionBase);
	IntuitionBase = null;
    }
    return false;
}


void DamnClock(struct Window *exceptfor)
{
    short i;
    struct Window *w;
    static long waittags[5] = {
	WA_BusyPointer, TRUE, WA_PointerDelay, TRUE, TAG_DONE
    };
#ifdef POINTPALETTE
    long pall = !prepale ? PALELEVEL : 0;
#endif

    for (i = 0; i < SHAR; i++)
	if ((w = sharers[i]) && w != exceptfor) {
	    if (dos3)
		SetWindowPointerA(w, (adr) waittags);
	    else {
		SetPointer(w, WaitPointer, 16, 16, -8, 0);
#ifdef POINTPALETTE
		if (ppred == -1 && prepale == palette[0] && !lightbg) {
		    short k = GetRGB4(scr->ViewPort.ColorMap, DARKPOINT);
		    ppred = k >> 8;
		    ppgreen = (k >> 4) & 15;
		    ppblue = k & 15;
		    SetRGB4(&scr->ViewPort, DARKPOINT, pall, pall, pall);
		}
#endif
	    }
	}
#ifndef TEST13
    if (lawin) {
	lastifle.Flags |= NOISYREQ;
	lastiffed = Request(&lastifle, lawin);
    }
    bgstifle.Flags |= NOISYREQ;
    bgstiffed = Request(&bgstifle, bgwin);
#endif
}


void NoDamnClock(struct Window *exceptfor)
{
    short i;
    struct Window *w;

    for (i = 0; i < SHAR; i++)
	if ((w = sharers[i]) && w != exceptfor) {
	    if (dos3)
		SetWindowPointerA(w, (adr) &endtag);
	    else
		ClearPointer(w);
#ifdef POINTPALETTE
	    if (ppred != -1) {
		SetRGB4(&scr->ViewPort, DARKPOINT, ppred, ppgreen, ppblue);
		ppred = -1;
	    }
#endif
	}
    if (lastiffed)
	EndRequest(&lastifle, lawin);
    if (bgstiffed)
	EndRequest(&bgstifle, bgwin);
    lastiffed = bgstiffed = 0;
}


/* this is taken straight from the RKM, except for one mod:  the window parm   */
/* can be null, in which case it strips all messages from the port -- use this */
/* when you want to shut off IDCMP input to all windows using the port.        */

void StripIntuiMessages(struct MsgPort *mp, struct Window *win)
{
    struct IntuiMessage *msg;
    struct Node *succ;

    msg = (struct IntuiMessage *) mp->mp_MsgList.lh_Head;
    while (succ = msg->ExecMessage.mn_Node.ln_Succ) {
	if (!win || msg->IDCMPWindow ==  win) {
	    Remove((struct Node *) msg);
	    ReplyMsg((struct Message *) msg);
	}
	msg = (struct IntuiMessage *) succ;
    }
}


/* Each call to PortOff must be matched by a call to PortOn, pairs may be
nested, OpenShareWin and CloseShareWin may not be called in between,
but Question and its derivatives (XxxErr, AskXXX) may be called. */

void PortOff(void)
{
    short i;
    register struct Window *w;

    if (countoff++ > 0)
	return;
    DamnClock(null);
    Forbid();
    StripIntuiMessages(idcort, null);
    for (i = 0; i < SHAR; i++)
	if (w = sharers[i]) {
	    oldflags[i] = w->IDCMPFlags;
	    w->UserPort = null;
	    ModifyIDCMP(w, 0);
	}
    Permit();
}


void PortOn(void)
{
    register struct Window *w;
    short i;

    if (--countoff > 0)
	return;
    if (countoff < 0) {
	countoff = 0;
	Err("INTERNAL ERROR -- message port reactivated\n"
				"more times than it was switched off.");
	return;
    }
    Forbid();
    for (i = 0; i < SHAR; i++)
	if (w = sharers[i]) {
	    w->UserPort = idcort;
	    ModifyIDCMP(w, oldflags[i]);
	}
    NoDamnClock(null);
    Permit();
}


short menunest = 0;

void NoMenus(void)
{
    if (!menunest++) {
	ClearMenuStrip(bgwin);
	if (cwin)
	    ClearMenuStrip(cwin);
	if (cbitwin)
	    ClearMenuStrip(cbitwin);
    }
}


void YesMenus(void)
{
    if (!--menunest) {
	if (rethinkmenus) {
	    SetMenuStrip(bgwin, &mainu);
	    if (cwin)
		SetMenuStrip(cwin, &mainu);
	    if (cbitwin)
		SetMenuStrip(cbitwin, &mainu);
	} else {
	    ResetMenuStrip(bgwin, &mainu);
	    if (cwin)
		ResetMenuStrip(cwin, &mainu);
	    if (cbitwin)
		ResetMenuStrip(cbitwin, &mainu);
	}
    }
}


void CloseShareWin(struct Window *win)
{
    short i;
    struct WhereWin *ww = (adr) win->UserData;

    for (i = 0; i < SHAR; i++)
	if (sharers[i] == win) {
	    sharers[i] = null;
	    break;
	}
    Forbid();		/* per RKM */
    StripIntuiMessages(idcort, win);
    win->UserPort = null;
    ModifyIDCMP(win, 0);
    Permit();
    if (ww) {
	struct ExtNewWindow *nw = ww->nw;
	if (XActual(nw->LeftEdge) != win->LeftEdge
		    || XActual(nw->Width) != win->Width
		    || nw->TopEdge != win->TopEdge || nw->Height != win->Height)
	    ww->moved = true;
	ww->left = XNominal(win->LeftEdge);
	ww->top = win->TopEdge;
	ww->width = XNominal(win->Width);
	ww->height = win->Height;
	ww->scrheight = scr->Height;
    }
    CloseWindow(win);
    if (IntuitionBase->ActiveScreen != scr)
	ActivateWindow(cwin ? cwin : bgwin);
}


void FitHeight(struct WhereWin *ww)
{
    short diff, nomscrwid = XNominal(scr->Width);	/* 640 or 656 */

    diff = ww->top + ww->height - scr->Height;
    if (diff > 0) {
	ww->top -= diff;
	if (ww->top < 0) {
	    ww->top = 0;
	    ww->height = scr->Height;
	}
    }
    diff = ww->left + ww->width - nomscrwid;
    if (diff > 0) {
	ww->left -= diff;
	if (ww->left < 0) {
	    ww->left = 0;
	    ww->width = nomscrwid;
	}
    }
}


struct Window *OpenShareWin(struct WhereWin *ww)
{
    register struct ExtNewWindow *nw = ww->nw;
    ulong idflags = nw->IDCMPFlags;
    struct Window *w;
    adr oldext = nw->Extension;
    short i, lll, ttt, www, hhh;

    if (countoff) {
	WorkbenchRequesterArgs("INTERNAL ERROR -- attempt to open shared\n"
			    "window while IDCMP shut off.  Window title:"
			    "\n\"%s\"", "Okay", nw->Title);
	return null;
    }
    UndoTitleBarMsg(true);
    nw->DetailPen = lightbg ? 6 : 0;
    nw->IDCMPFlags = 0;
    if (bgwin->ScreenTitle) {
	sharetags.chain = oldext;
	sharetags.s = bgwin->ScreenTitle;
	nw->Extension = (adr) &sharetags;
    }
    lll = nw->LeftEdge;
    ttt = nw->TopEdge;
    www = nw->Width;
    hhh = nw->Height;
    if (ww->scrheight != scr->Height)
	ww->moved = false;
    if (ww->moved) {
	if (!(nw->Flags & WFLG_SIZEGADGET)) {
	    ww->width = nw->Width;
	    ww->height = nw->Height;
	}
	FitHeight(ww);
	nw->LeftEdge = ww->left;
	nw->TopEdge = ww->top;
	if (nw->Flags & WFLG_SIZEGADGET) {
	    nw->Width = ww->width;
	    nw->Height = ww->height;
	}
    }
    nw->LeftEdge = XActual(nw->LeftEdge);
    nw->Width = XActual(nw->Width);
    if (nw->Width < nw->MinWidth && nw->MinWidth < 2000)
	nw->Width = nw->MinWidth;
    if (nw->Width > nw->MaxWidth && nw->MaxWidth > 0)
	nw->Width = nw->MaxWidth;
    nw->Screen = scr;
    if (w = OpenWindow((adr) nw)) {
	w->UserPort = idcort;
	ModifyIDCMP(w, idflags);
	w->UserData = (adr) ww;
	SetFont(w->RPort, font);
    }
    nw->LeftEdge = lll;
    nw->TopEdge = ttt;
    nw->Width = www;
    nw->Height = hhh;
    nw->Extension = oldext;
    nw->IDCMPFlags = idflags;
    if (!w)
	return null;
    for (i = 0; i < SHAR; i++)
	if (!sharers[i])
	    return sharers[i] = w;
    CloseShareWin(w);
    Err("INTERNAL ERROR -- table thinks\nten shared windows are open.");
    return null;
}


local void ScaleGTextFudge(struct Gadget *gg, short fudge)
{
    struct IntuiText *tt = gg->GadgetText;
    if (tt) {
	tt->ITextFont = &ta;
	if (tt->LeftEdge < 0 || tt->TopEdge < 0)
	    tt->BackPen = backcolor;
	if (gg->Width > fudge) {
	    gg->UserData = (APTR) (((ulong) tt->LeftEdge & 0x7FF)
					| ((ulong) gg->UserData & 0xFFFFFC00L));
	    if (tt->LeftEdge <= 0)
		fudge = 0;
	    else if (fudge)
		fudge -= 4;
	    tt->LeftEdge = XActual(tt->LeftEdge - fudge) + fudge;
	}
    }
}


void ScaleGText(struct Gadget *gg)
{
    ScaleGTextFudge(gg, GadgetConstancy(gg));
}


void UnScaleGText(struct Gadget *gg)
{
    struct IntuiText *tt = gg->GadgetText;
    if (tt) {
	tt->LeftEdge = ((ulong) gg->UserData & 0x7FF);
	if (tt->LeftEdge & 0x400)		/* sign extend it! */
	    tt->LeftEdge |= 0xF800;		/* -1024..1023 */
    }
}


void ScaleGadget(struct Gadget *gg)
{
    short fudge = GadgetConstancy(gg);
    gg->UserData = (APTR) (((ulong) gg->LeftEdge << 21)
				+ ((((ulong) gg->Width) & 0x3FF) << 11));
    gg->LeftEdge = XActual(gg->LeftEdge);
    gg->Width = XActual(gg->Width - fudge) + fudge;
    if (!fudge && (fontwid < 8 || fontwid & 1))
	gg->Width++;			/* bullshit string gadget band-aid */
    ScaleGTextFudge(gg, fudge);
}


void UnScaleGadget(struct Gadget *gg)
{
    if (gg->UserData) {
	gg->LeftEdge = (signed long) gg->UserData >> 21;   /* -1024..1023 */
	gg->Width = ((ulong) gg->UserData >> 11) & 0x3FF;  /* 0..1023 */
	UnScaleGText(gg);
	gg->UserData = null;
    }
}


/* This currently refreshes only string gadgets.  It ought to also refresh  */
/* command and cycle gadgets, but not checkmark gadgets.  Some RefreshGList */
/* calls would need fixing in setup.c, door.c, search.c, and compose.c.     */

struct Window *OpenBlueGagWin(struct WhereWin *wh, struct Gadget *first)
{
    struct Window *w;
    short topdiff = tifight - fakefight;
    bool strg;

    StopScroll();
    wh->nw->Height += topdiff;
    if (!(w = OpenShareWin(wh)))
	return null;
    wh->oldgagmask = FlipBGadgets(0);
    if (wh != &cbitww && wh != &cww)	/* these get to have menus */
	NoMenus();
    SetAPen(w->RPort, backcolor);
    RectFill(w->RPort, 5, tifight + 4, w->Width - 6, w->Height - 4);
    wh->gacount = 0;
    for (wh->gfirst = first; first; first = first->NextGadget) {
	ScaleGadget(first);
	first->TopEdge += topdiff;
	if (first->GadgetText && first->GadgetText->TopEdge < 0)
	    first->GadgetText->TopEdge = -4 - fight - lace;
	if (first->GadgetType & GTYP_STRGADGET)
	    FixStringGad(first, w);
	else {
	    if (first->GadgetRender == &checkmarkno)
		first->Height = checktall;
	    FixGreenGad(first, w);
	}
	wh->gacount++;
	wh->glast = first;
    }
    AddGList(w, wh->gfirst, -1, (long) wh->gacount, null);
    GhostOn(w);
    for (first = wh->gfirst; first; first = first->NextGadget) {
	strg = !!(first->GadgetType & GTYP_STRGADGET);
	if (!strg && first->GadgetRender != &checkmarkno
			&& first->GadgetRender != &upcyclebor)
	    RefreshGList(first, w, null, 1);	/* command buttons only */
	    /* maybe later we'll let it do cycle gadgets too */
    }
    GhostOff(w);
    StripIntuiMessages(idcort, null);
    return w;
}


void CloseBlueGagWin(struct Window *w)
{
    struct WhereWin *wh = (adr) w->UserData;
    struct Gadget *gg;
    ushort ogm = wh->oldgagmask;
    short topdiff = tifight - fakefight;

    RemoveGList(w, wh->gfirst, (long) wh->gacount);
    wh->glast->NextGadget = null;
    for (gg = wh->gfirst; gg; gg = gg->NextGadget) {
	gg->TopEdge -= topdiff;
	UnScaleGadget(gg);
    }
    CloseShareWin(w);
    wh->nw->Height -= topdiff;
    if (wh != &cbitww && wh != &cww)
	YesMenus();
    FlipBGadgets(ogm);
}


local short ghostpen;

/* GhostOn(ww) must be followed SOON by GhostOff(ww)! */
/* Do NOT do IO or wait for messages in between.      */

void GhostOn(struct Window *ww)
{
    ModifyIDCMP(ww, ww->IDCMPFlags | IDCMP_MENUVERIFY);
    ghostpen = ww->BlockPen;
    ww->BlockPen = fourcolors && lightbg ? 1 : 4;	/* blue or black */
}


void GhostOff(struct Window *ww)
{
    ww->BlockPen = ghostpen;
    ModifyIDCMP(ww, ww->IDCMPFlags & ~IDCMP_MENUVERIFY);
}
