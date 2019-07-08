/* clipboard, timer, file buffer related stuff in Q-Blue, and Iconify() */

#include <exec/memory.h>
#include <intuition/intuitionbase.h>
#include <workbench/startup.h>
#include <workbench/workbench.h>
#include <devices/clipboard.h>
#include <devices/timer.h>
#include <clib/wb_protos.h>
#ifndef __NO_PRAGMAS
#  include <pragmas/wb_lib.h>
#endif
#include "qblue.h"


#define MYBUFSIZE      4096
#define SCROLLSPEED    50000

#ifdef TEST13
#  define WINDOW_ICONIFY
#  define CreateMsgPort()   CreatePort(null, 0)
#  define DeleteMsgPort(p)  DeletePort(p)
#endif

#ifdef WINDOW_ICONIFY
#  define ICONWID      136
#  define ICONLEFT     300
#  define ICONTOP      0
#endif

#define CLAFTER(x1, y1, x2, y2)  (y2 < y1 || (y2 == y1 && x2 < x1))


void SetAllSharedWindowTitles(str title);
bool LScrollDown(struct Conf *cc, short dist);
bool LScrollUp(struct Conf *cc, short dist);
bool OpenDisplay(void);
void CloseDisplay(void);
bool MakeLotsaRoom(void);
void WorkbenchRequesterArgs(str body, str options, ...);
bool AnySharedPresent(void);
bool ForeignersPresent(void);
bool NoteWBArgPacket(struct WBArg *wa);
void PreloadConf(struct Conf *cc, ushort which);
bool ScrollUpLine(struct Mess *mm, short xtra);
bool ScrollDownLine(struct Mess *mm, short xtra);
void ShowHits(short look);
bool ScrollIDCMP(struct IntuiMessage *im, struct Mess *mm);
short XNominal(register short x);


import struct IntuitionBase *IntuitionBase;
import struct timeval legit_time, view_time;
import struct timerequest tiq;
import struct MsgPort *tipo;
import struct Gadget bgscroll, luparrow, bguparrow, *lastbutton, *lastbgbutton;
import struct Gadget egagcomm1, egagcomm2, pgagunpack, pgagpack, ygagcchead;
import struct Gadget ygagqhead, llgagqhead, ygagsignature, llgagsignature;
import struct TextAttr ta;
import struct RastPort *bgrp;

import struct Conf *listcc;

import ushort typecolors[];
import ubyte bogus_title[], *legit_title, title[];
import char screentitle[4][80], tryopename[], fontname[], newfontname[];

import short lastmouseX, lastmouseY, topline, lastcolor;
import ushort taheight, textop, winlines, slotop, slotbot;
import ushort deferline, whichtitle;
import bool laterlace, laterfourcolors, buttoning, holdopen, customode;
import bool obscuringslider;


struct timerequest clockq;

bool latercustomode = false, amclipping = false, casale /* clock out */ = false;

local bool outq = false;
local ushort startcliprow, startclipcol, nowcliprow, nowclipcol;
local long clipsize, timeshown = -1;

local struct IOClipReq clipio = { 0 };
local struct MsgPort *cliport;


#ifdef WINDOW_ICONIFY
local bool UnIconify(struct Window *iconw)
#else
local bool UnIconify(void)
#endif
{
    void UpdateClock(void);
    if (OpenDisplay() || (MakeLotsaRoom() && OpenDisplay())) {
	if (areaz.messct) {
	    SetWindowTitles(bgwin, null, null);
	    UpdateClock();
	}
	return true;
    }
    WorkbenchRequesterArgs("Could not open Q-Blue's\nscreen!  No chip ram?",
				"Okay");
#ifdef WINDOW_ICONIFY
    SetWindowTitles(iconw, "ERROR!", "COULD NOT REOPEN Q-Blue screen!");
#endif
    holdopen = true;
    return false;
}


#ifdef WINDOW_ICONIFY

struct ExtNewWindow newiconw = {
    ICONLEFT, ICONTOP, ICONWID, 11, 0, 1, IDCMP_CLOSEWINDOW,
    WFLG_DRAGBAR | WFLG_DEPTHGADGET | WFLG_CLOSEGADGET
		| WFLG_NOCAREREFRESH /* | WFLG_NW_EXTENDED */,
    null, null, "Q-Blue", null, null, 0, 0, 0, 0, WBENCHSCREEN, null
};

bool iconmoved = false;

void Iconify(bool instant)
{
    struct Screen *bench;
    short hite = 11;
    struct Window *iconw;
    struct Message *mes;
    long sig = 0;

    if (AnySharedPresent()) {		/* should never happen */
	Err("Close windows before\niconifying screen.");
	return;
    }
    if (ForeignersPresent()) {
	Err("Cannot iconify screen until\nforeign windows are all closed.");
	return;
    }
#ifndef TEST13
    bench = LockPubScreen(NULL);
    if (bench)
	hite = bench->Font->ta_YSize + bench->WBorTop + 1;
#else
    bench = null;
#endif
    newiconw.Height = /* iconzoom[3] = */ hite;
    if (!(iconw = OpenWindow((adr) &newiconw))
		&& ((newiconw.LeftEdge + ICONWID < 640
			&& newiconw.TopEdge + hite < 200)
		    || (newiconw.LeftEdge = /* iconzoom[0] = */ ICONLEFT,
			newiconw.TopEdge = /* iconzoom[1] = */ ICONTOP,
			!(iconw = OpenWindow((adr) &newiconw))))
		&& !(MakeLotsaRoom()
			&& (iconw = OpenWindow((adr) &newiconw)))) {
	Err("Could not open tiny\nwindow on Workbench.");
	if (bench)
	    UnlockPubScreen(null, bench);
	return;
    }
    if (bench)
	UnlockPubScreen(null, bench);
    ScreenToFront(iconw->WScreen);
    holdopen = true;
    timeshown = -1;
    CloseDisplay();			/* boom! */
    if (!instant)
	MakeLotsaRoom();		/* after close so no visible delay */
    strcpy(fontname, newfontname);
    lace = laterlace;
    fourcolors = laterfourcolors;
    customode = latercustomode;
    ta.ta_YSize = taheight;
    if (instant)
	if (!UnIconify(iconw) && MakeLotsaRoom() && !UnIconify(iconw))
	    instant = false;
    if (!instant)
	for (;;) {
	    while ((mes = GetMsg(iconw->UserPort)) || sig & SIGBREAKF_CTRL_F) {
		if (mes)
		    ReplyMsg(mes);
		if (UnIconify(iconw))
		    goto done;
	    }
	    sig = Wait(SIGBREAKF_CTRL_F | bit(iconw->UserPort->mp_SigBit));
	}
  done:
    if (screentitle[whichtitle][0])
	SetWindowTitles(bgwin, null, screentitle[whichtitle]);
    holdopen = false;
    if (iconw->LeftEdge != ICONLEFT || iconw->TopEdge != ICONTOP)
	iconmoved = true;
    newiconw.LeftEdge = /* iconzoom[0] = */ iconw->LeftEdge;
    newiconw.TopEdge = /* iconzoom[1] = */ iconw->TopEdge;
    CloseWindow(iconw);
/*  if (whicha >= 0 && readareaz.messct && waste)
	PreloadConf(readareaz.confs[whicha], whichm);  */
}

#else /* !WINDOW_ICONIFY */

#define ICONWIDTH  60
#define ICONHEIGHT 28

local UWORD iconplanes[336] = {	/* ooooooh, 8 colors! */
   0x7E00,0x0000,0x0000,0x0000,
   0x6600,0x0000,0xE980,0x0000,
   0x4200,0x001F,0xFF9A,0x0000,
   0x4200,0x0073,0x4B65,0x0000,
   0x4200,0x017E,0xFFF3,0x0000,
   0x4200,0x07FE,0x8D3D,0x4000,
   0x4200,0x1ACF,0xFFEF,0xB000,
   0x6600,0x17BF,0xFFBA,0xF800,
   0x6600,0xF7FF,0x6CE2,0x1400,
   0x7E01,0xDFFF,0xE46D,0x9C00,
   0x7E03,0xFFFF,0x88E0,0x1C00,
   0x660F,0xFFFE,0x00A9,0x1C00,
   0x660F,0xFFFE,0x00E0,0x1C00,
   0x7E7B,0x7FFC,0x00E0,0x1C00,
   0x00DF,0xFFFC,0x00E0,0x1C00,
   0x01FF,0xFFFC,0x00E0,0x1C00,
   0x07FF,0xFFFE,0x0071,0xB800,
   0x3FDF,0xFFFE,0x003F,0xF000,
   0xBFFF,0xFFFF,0x000F,0xE000,
   0xFEFF,0xFFFF,0x8000,0x7000,
   0x77FF,0x7FFF,0xE000,0x3800,
   0xFFFF,0xFFFF,0xF800,0x0000,
   0xFFFF,0xFFFF,0xFFE0,0x0000,
   0xFFFF,0xFFFF,0xFFFF,0xC000,
   0xFFFF,0xFFFF,0xFFFF,0xFFF0,
   0xFFFF,0xFFFF,0xFFFF,0xFFF0,
   0xFFFF,0xFFFF,0xFFFF,0xFFF0,
   0xFFFF,0xFFFF,0xFFFF,0xFFF0,

   0x0000,0x0000,0x0020,0x0000,
   0x0800,0x0000,0x5653,0x0000,
   0x1C00,0x000A,0xFF72,0x4000,
   0x1C00,0x003F,0xBFFB,0x1000,
   0x1C00,0x00FF,0xFEFF,0xC000,
   0x1C00,0x0BFF,0xF3EA,0xA000,
   0x1C00,0x0FFF,0xFFB5,0x4800,
   0x0800,0x3FFE,0xDEDB,0xC000,
   0x0800,0x7FFB,0xFD56,0x8800,
   0x0000,0xFFED,0x00AC,0x8000,
   0x0005,0x7FFE,0x0A14,0x4000,
   0x0807,0xFBE4,0x0049,0x0000,
   0x0807,0xFEB8,0x0000,0x0000,
   0x006F,0xBFF8,0x0000,0x0000,
   0x003E,0xFBA0,0x0000,0x0000,
   0x01FF,0xF7F8,0x0000,0x0000,
   0x02DF,0xDFB8,0x0000,0x0000,
   0x0BFF,0xFFE8,0x0000,0x0000,
   0x7DFF,0xF7FE,0x0000,0x0000,
   0xFFEF,0xFED4,0x0000,0x0000,
   0xFFBD,0xEFDF,0x0000,0x0000,
   0xFFFF,0xFFED,0xC000,0x0000,
   0xFFBF,0xFFFE,0xB800,0x0000,
   0xBFFE,0xFDF7,0xDF40,0x0000,
   0xF7F7,0x7FDF,0xFAFB,0xC000,
   0xF7FF,0xFFBE,0xD757,0x57D0,
   0xDFFF,0xFEFF,0xFFF5,0xEAE0,
   0xFBFF,0xBFFF,0x7ADF,0x7DB0,

   0x0000,0x0000,0x0000,0x0000,
   0x1800,0x0000,0x0202,0x0000,
   0x3C00,0x0000,0x0000,0x0000,
   0x3C00,0x0000,0x0010,0x0000,
   0x3C00,0x0000,0x0004,0x4000,
   0x3C00,0x0000,0x2041,0x4000,
   0x3C00,0x0000,0x0008,0x7800,
   0x1800,0x0000,0x0040,0x1800,
   0x1800,0x0000,0x0004,0x0400,
   0x0000,0x0000,0x0040,0x0C00,
   0x0000,0x0000,0x0060,0x0C00,
   0x1800,0x0000,0x0020,0x0C00,
   0x1800,0x0000,0x0060,0x0C00,
   0x0000,0x0000,0x0060,0x0C00,
   0x0000,0x0000,0x0060,0x0C00,
   0x0000,0x0000,0x0060,0x0C00,
   0x0000,0x0000,0x0030,0x9800,
   0x0000,0x0000,0x001F,0xF000,
   0x0000,0x0000,0x0000,0x6000,
   0x0000,0x0000,0x0000,0x3000,
   0x0000,0x0000,0x0000,0x0000,
   0x0000,0x0000,0x0000,0x0000,
   0x0000,0x0000,0x0000,0x0000,
   0x0000,0x0000,0x0000,0x0000,
   0x0000,0x0000,0x0000,0x0000,
   0x0000,0x0000,0x0000,0x0000,
   0x0000,0x0000,0x0000,0x0000,
   0x0000,0x0000,0x0000,0x0000
};

local struct Image iconimage = {
    0, 0, ICONWIDTH, ICONHEIGHT, 3, iconplanes, 7, 0, null
};

local struct DiskObject icon = {
    0, 0, /* struct Gadget: */ {
	null, 0, 0, ICONWIDTH, ICONHEIGHT, 0, 0, 0, &iconimage, null,
	null, 0, null, 0, null
    }, 0, null, null, NO_ICON_POSITION, NO_ICON_POSITION, null, null, 0
};

struct Library *WorkbenchBase;


void Iconify(bool instant)
{
    str iconame = readareaz.messct ? packetname : "Q-Blue", badname;
    struct AppIcon *ai = null;
    struct MsgPort *imp = null;
    struct AppMessage *mes;
    long sig = 0;
    UWORD *chem = null;
    bool baddrop = false;
    static long icon_x = NO_ICON_POSITION, icon_y = NO_ICON_POSITION;

    if (AnySharedPresent()) {		/* should never happen */
	Err("Close windows before\niconifying screen.");
	return;
    }
    if (ForeignersPresent()) {
	Err("Cannot iconify screen until\nforeign windows are all closed.");
	return;
    }
    if (!(imp = CreateMsgPort())) {
	Err("Cannot create AppIcon;\nno memory for port.");
	return;
    }
    if (!(WorkbenchBase = OpenL("workbench"))) {    /* should never happen */
	Err("Cannot create AppIcon;\nno workbench.library");
	goto done;
    }
    if (!(chem = AllocCP(sizeof(iconplanes)))) {
	Err("Cannot create AppIcon;\nno chip memory.");
	goto done;
    }
    iconimage.ImageData = chem;		/* seems to work fine with fast mem! */
    CopyMem(iconplanes, chem, sizeof(iconplanes));
    icon.do_CurrentX = icon_x, icon.do_CurrentY = icon_y;
    if (!(ai = AddAppIcon(0, 0, iconame, imp, null, &icon, TAG_DONE))) {
	Err("Cannot create AppIcon.\nNo memory?");
	goto done;
    }
    WBenchToFront();
    holdopen = true;
    timeshown = -1;
    CloseDisplay();			/* boom! */
    if (!instant)
	MakeLotsaRoom();		/* after close so no visible delay */
    strcpy(fontname, newfontname);
    lace = laterlace;
    fourcolors = laterfourcolors;
    customode = latercustomode;
    ta.ta_YSize = taheight;
    if (instant)
	if (!UnIconify())
	    instant = false;
    if (!instant)
	for (;;) {
	    while ((mes = (adr) GetMsg(imp)) || sig & SIGBREAKF_CTRL_F) {
		if (mes) {
		    if (mes->am_NumArgs) {
			baddrop = NoteWBArgPacket(&mes->am_ArgList[0]);
			badname = mes->am_ArgList[0].wa_Name;
		    }
		    ReplyMsg((adr) mes);
		}
		if (UnIconify())
		    goto done;
	    }
	    sig = Wait(SIGBREAKF_CTRL_F | bit(imp->mp_SigBit));
	}
  done:
    if (baddrop) {
	Err("Cannot open file %s as a mail\npacket; %s.", badname,
			readareaz.messct ? "another packet is already open"
					 : "error getting path from icon");
	tryopename[0] = 0;
    }
    if (ai) {
	icon_x = icon.do_CurrentX, icon_y = icon.do_CurrentY;	/* useless */
	RemoveAppIcon(ai);
	if (screentitle[whichtitle][0])
	    SetWindowTitles(bgwin, null, screentitle[whichtitle]);
	holdopen = false;
/*	if (whicha >= 0 && readareaz.messct && waste)
	    PreloadConf(readareaz.confs[whicha], whichm);	*/
    }
    if (imp) {
	while (mes = (adr) GetMsg(imp))
	    ReplyMsg((adr) mes);
	DeleteMsgPort(imp);
    }
    if (WorkbenchBase) {
	CloseLibrary(WorkbenchBase);
	WorkbenchBase = null;
    }
    if (chem)
	FreeMem(chem, sizeof(iconplanes));
}

#endif !WINDOW_ICONIFY


void StopTimer(void)
{
    if (outq) {
	AbortIO((adr) &tiq);
	WaitIO((adr) &tiq);
	outq = false;
    }
}


void StopTitleBarClock(void)
{
    if (casale) {
	AbortIO((adr) &clockq);
	WaitIO((adr) &clockq);
	casale = false;
    }
}


void StopScroll(void)
{
    void StopClipping(bool abort);

    StopTimer();
    StopClipping(true);
    buttoning = false;
    lastbgbutton = lastbutton = null;
}


void SetAlarm(ulong micros)
{
    if (outq) StopTimer();
    tiq.tr_node.io_Command = TR_ADDREQUEST;
    tiq.tr_time.tv_secs = micros / 1000000;
    tiq.tr_time.tv_micro = micros % 1000000;
    SendIO((adr) &tiq);
    outq = true;
}


void ClipShutdown(void)
{
    amclipping = false;		/* don't bother restoring display */
    if (clipio.io_Device) {
	CloseDevice((adr) &clipio);
	DeleteMsgPort(cliport);
    }
}


bool ClipSetup(void)
{
    static bool cliperrsaid = false;
    if (!clipio.io_Device) {
	if (!(cliport = CreateMsgPort())) {
	    if (!cliperrsaid)
		Err("Insufficient memory or no signal\n"
				"for clipboard.device MsgPort");
	    cliperrsaid = true;
	    return false;
	}
	clipio.io_Message.mn_ReplyPort = cliport;
	if (OpenDevice("clipboard.device", PRIMARY_CLIP, (adr) &clipio, 0)) {
	    if (!cliperrsaid)
		Err("Cannot open clipboard.device");
	    cliperrsaid = true;
	    DeleteMsgPort(cliport);
	    return false;
	}
    }
    return true;
}


bool ClipboardStringPaste(struct IntuiMessage *im)
{
    struct Gadget *gg;
    struct Window *ww;
    ulong clipbuf[(SIGNATURELEN / 4) + 6];
    str cliptext, linend, tp;
    short blen, atens, atnums;

    if (im->Class != IDCMP_GADGETUP || im->Code != 'V')    /* set by hook */
	return false;
    gg = (struct Gadget *) im->IAddress;
    ww = im->IDCMPWindow;
    if (!gg || (gg->GadgetType & GTYP_GTYPEMASK) != GTYP_STRGADGET)
	return false;
    /* cause event to not trigger GAGFINISH if we return false: */
    im->Code = '\t';
    atnums = gg == &ygagsignature || gg == &llgagsignature;	/* someday... */
    atens = atnums || gg == &egagcomm1 || gg == &egagcomm2 || gg == &pgagunpack
			|| gg == &pgagpack || gg == &llgagqhead
			|| gg == &ygagqhead || gg == &ygagcchead;
    if (!ClipSetup())
	return false;
    clipio.io_ClipID = 0;
    clipio.io_Command = CMD_READ;
    clipio.io_Offset = 0;
    clipio.io_Length = sizeof(clipbuf);
    clipio.io_Data = (adr) clipbuf;
    if (!DoIO((adr) &clipio) && clipbuf[0] == 'FORM' && clipbuf[2] == 'FTXT'
			&& clipbuf[3] == 'CHRS' && (blen = clipbuf[4])) {
	cliptext = (adr) &clipbuf[5];
	if (blen > SIGNATURELEN - 1)
	    blen = SIGNATURELEN - 1;
	cliptext[blen] = 0;
	if (*cliptext == '\n')
	    cliptext++, blen--;
	while (blen && cliptext[blen - 1] == '\n')
	    cliptext[--blen] = '\0';
	linend = cliptext;
	if (atens) {
	    /* if (atnums) ....... convert long strings of whitespace */
	    if (atnums)
		while (linend = strchr(linend, '@')) {	/* literal ats */
		    *linend++ = '@';
		    memmove(linend + 1, linend, blen - (linend - cliptext));
		    *linend++ = '@';
		    if (++blen > SIGNATURELEN - 1)
			cliptext[blen = SIGNATURELEN - 1] = 0;
		}
	    linend = cliptext;
	    while (linend = strchr(linend, '\n')) {
		*linend++ = '@';
		memmove(linend + 1, linend, blen - (linend - cliptext));
		*linend++ = 'N';
		if (++blen > SIGNATURELEN - 1)
		    cliptext[blen = SIGNATURELEN - 1] = 0;
	    }
	} else
	    while (linend = strchr(linend, '\n')) {
		*linend++ = ' ';
		for (tp = linend; *tp == ' '; tp++) ;
		if (tp > linend)
		    strcpy(linend, tp), blen -= tp - linend;
	    }
    } else
	blen = 0;
    clipio.io_Offset = maxlong;		/* signal done reading */
    clipio.io_Data = null;
    clipio.io_Length = 0;
    DoIO((adr) &clipio);
    if (blen) {
	char tempundo[SIGNATURELEN];
	struct StringInfo *si = (adr) gg->SpecialInfo;
	long p = RemoveGadget(ww, gg);

	strcpy(tempundo, undospace);
	if (blen >= si->MaxChars)
	    blen = si->MaxChars - 1;
	strncpy0(si->Buffer, cliptext, blen);
	si->BufferPos = blen;
	AddGadget(ww, gg, p);
	RefreshGList(gg, ww, null, 1);
	ActivateGag(gg, ww);
	strcpy(undospace, tempundo);	/* slightly unkosher, but it works... */
	return true;
    }
    return false;
}


local void CopyToClipboard(ushort x1, ushort y1, ushort x2, ushort y2)
{
    static char ftxthead[20] = "FORM\0\0\0\0FTXTCHRS\0\0\0";
    ushort t, n, left, right;
    ustr lin;

    if ((x1 == x2 && y1 == y2) || !onscreen)    /* just in case */
	return;
    if (CLAFTER(x1, y1, x2, y2)) {
	t = y2, y2 = y1, y1 = t;
	t = x2, x2 = x1, x1 = t;
    }
    if (!ClipSetup())
	return;
    clipio.io_ClipID = 0;
    clipio.io_Command = CMD_WRITE;
    clipio.io_Offset = 20;		/* FTXT header gets done last */
    for (n = y1; n <= y2; n++) {
	if (n < onscreen->linect && (lin = onscreen->lines[n])
						&& (t = lin[-1])) {
	    left = (n == y1 ? x1 : 0);
	    right = (n == y2 && x2 < t ? x2 : t);
	    if (right > left) {
		clipio.io_Data = lin + left;
		clipio.io_Length = right - left;
		if (DoIO((adr) &clipio))
		    goto flargh;
	    }
	}
	if (n < y2) {
	    clipio.io_Data = "\n";
	    clipio.io_Length = 1;
	    if (DoIO((adr) &clipio))
		goto flargh;
	}
    }
    *(long *) (ftxthead + 16) = clipio.io_Offset - 20;     /* total written */
    if (clipio.io_Offset & 1) {		/* pad to even */
	clipio.io_Data = "";		/* pointer to zero byte */
	clipio.io_Length = 1;
	if (DoIO((adr) &clipio))
	    goto flargh;
    }
    *(long *) (ftxthead + 4) = clipio.io_Offset - 8;       /* form size */
    clipio.io_Length = 20;
    clipio.io_Data = ftxthead;
    clipio.io_Offset = 0;			/* stick ahead of other data */
    if (DoIO((adr) &clipio))
	goto flargh;
    clipio.io_Command = CMD_UPDATE;
    if (DoIO((adr) &clipio))
	Err("Write to clipboard.device\napparently failed to complete.");
    return;
  flargh:
    Err("Failure writing to\nclipboard.device");
    clipio.io_Command = CMD_UPDATE;
    DoIO((adr) &clipio);
}


/* look = 0: plain;  look < 0: search hilite;  look > 0: clip invert */
void DisplayMsgRegion(ushort x1, ushort y1, ushort x2, ushort y2, short look)
{
    long color, tit = y2 - y1;
    ushort n, t, left, right, rmar = 80;
    ustr lin;
    short vidded = 0, faketopline;

    if (!onscreen)
	return;				/* just in case */
    if (CLAFTER(x1, y1, x2, y2)) {
	t = y2, y2 = y1, y1 = t;
	t = x2, x2 = x1, x1 = t;
    }
    ASSERT(bgrp);
    if (obscuringslider)
	rmar -= (fontwid + bgscroll.Width) / fontwid;
    SetDrMd(bgrp, (look ? JAM2 | INVERSVID : JAM2));
    faketopline = topline - !!ATTACHED(onscreen);
    for (n = y1; n <= y2; n++) {
	if ((signed) n < faketopline + slotop || n >= faketopline + winlines
			    || n >= onscreen->linect || n - faketopline >= slotbot)
	    continue;
	if (!(lin = onscreen->lines[n]) || !(t = lin[-1]))
	    continue;
	left = (n == y1 ? x1 : 0);
	right = (n == y2 && x2 < t ? x2 : t);
	if (min(right, rmar) <= left)
	    continue;
	color = typecolors[onscreen->linetypes[n] >> TYPE_SHIFT];
	if (look < 0)
	    color = SEARCHLITE(color);
	if (color != lastcolor)
	    SetAPen(bgrp, lastcolor = color);
	Move(bgrp, left * fontwid, (n - faketopline) * fight
					+ textop + font->tf_Baseline);
	Text(bgrp, lin + left, min(right, rmar) - left);
	tit += right - left;
    }
    if (look) {
	SetDrMd(bgrp, JAM2);
	if (look >= 0)
	    clipsize += (look ? tit : -tit);
    }
}


void RowColFromLastMouse(ushort *row, ushort *col)
{
    short off = !!ATTACHED(onscreen);
    ushort end = min(onscreen->linect, topline + winlines - off);
    long trow = topline + (lastmouseY - textop) / fight - off;
    
    if (trow >= end) {
	*row = end;
	*col = 0;
    } else if (trow + off < topline || trow < 0) {
	*row = (topline > off ? topline - off : 0);
	*col = 0;
    } else {
	*row = trow;
	*col = (lastmouseX + fontwid / 2) / fontwid;
    }
}


void ContinueClipping(void)
{
    ushort oncrow = nowcliprow, onccol = nowclipcol;
    bool afterlast, afterstart;
    bool forward = CLAFTER(onccol, oncrow, startclipcol, startcliprow);

    if (!amclipping)
	return;
    RowColFromLastMouse(&nowcliprow, &nowclipcol);
    afterlast = CLAFTER(nowclipcol, nowcliprow, onccol, oncrow);
    afterstart = CLAFTER(nowclipcol, nowcliprow, startclipcol, startcliprow);
    if (afterstart != forward) {	/* crossed over starting point! */
	DisplayMsgRegion(onccol, oncrow, startclipcol, startcliprow, 0);
	DisplayMsgRegion(nowclipcol, nowcliprow,
				startclipcol, startcliprow, 1);
    } else
	DisplayMsgRegion(onccol, oncrow, nowclipcol, nowcliprow,
		/* 0 when clip is shrinking: */	afterlast == forward);
}


void StopClipping(bool abort)
{
    if (!amclipping)
	return;
    if (!abort) {
	ContinueClipping();		/* get up-to-date */
	CopyToClipboard(startclipcol, startcliprow, nowclipcol, nowcliprow);
    }
    amclipping = false;
    DisplayMsgRegion(startclipcol, startcliprow, nowclipcol, nowcliprow, 0);
    if (onscreen->bits & BODYMATCH && !deferline)
	ShowHits(-1);
}


void StartClipping(void)
{
    StopClipping(true);
    StopTimer();		/* do NOT call StopScroll()! */
    if (!onscreen)
	return;
    clipsize = 0;
    RowColFromLastMouse(&startcliprow, &startclipcol);
    nowcliprow = startcliprow;
    nowclipcol = startclipcol;
    amclipping = true;
    lastcolor = -1;
    if (onscreen->bits & BODYMATCH)
	ShowHits(0);
    ContinueClipping();
}


void DoAllTimedScrolling(void)
{
    bool up, down, did;
    short diss;

    if (outq) {
	if (!CheckIO((adr) &tiq)) {
	    long sigz = Wait(bit(tipo->mp_SigBit) | bit(idcort->mp_SigBit)
						| SIGBREAKF_CTRL_F);
	    if (sigz & SIGBREAKF_CTRL_F) {
		ScreenToFront(scr);
		if (IntuitionBase->ActiveScreen != scr)
		    ActivateWindow(scr->FirstWindow);
	    }
	    if (!CheckIO((adr) &tiq))
		return;
	}
	WaitIO((adr) &tiq);	/* returns immediately */
    }
    outq = did = up = down = false;
    if (lastbutton) {
	if (!(lastbutton->Flags & GFLG_SELECTED))
	    StopScroll();
	else {
	    if (lastbutton == &luparrow)
		did = LScrollUp(listcc, 1);
	    else
		did = LScrollDown(listcc, 1);
	    if (did)
		SetAlarm(SCROLLSPEED);
	}
	return;
    }
    if (lastbgbutton) {
	if (lastbgbutton->Flags & GFLG_SELECTED)
	    up = lastbgbutton == &bguparrow, down = !up;
	else
	    StopScroll();
	diss = 1;
    } else if (buttoning) {
	up = lastmouseY < textop;
	down = lastmouseY >= texbot - fight;
	diss = 1 + lace;
	did = true;
    }
    if (up | down) {
	if (up)
	    did |= ScrollUpLine(onscreen, diss);
	else
	    did |= ScrollDownLine(onscreen, diss);
    }
    if (did) {
	ContinueClipping();
	SetAlarm(BGSCROLLSPEED);
    }
}


void AppendTitleTip(ustr shaft, ustr head /* 12 chars max! */ )
{
    ustr ridge = shaft + 76 - strlen(head), drop;
    shaft[64] = 0;
    if (*head) {
	for (drop = strend(shaft); drop < ridge; drop++)
	    *drop = ' ';
	strcpy(drop, head);
    } else
	for (drop = strend(shaft); drop > shaft && isspace(*drop); drop--)
	    *drop = '\0';
}


void UpdateClock(void)
{
    struct timeval now;
    short hour, minute;
    char kalok[16];
    static ustr lastitle = null;
    ustr bs = bgwin->ScreenTitle;

    if (!scr || !tipo || !screentitle[0][0] || (bs && bs != (ustr)
					&screentitle[whichtitle][0])) {
	lastitle = null;
	return;		/* do NOT update the default inactive-window title */
    }
    GetSysTime(&now);
    minute = (now.tv_secs / 60) % 1440;
#ifdef CLOCK_HAS_SECONDS
    if (!lastitle || bs != lastitle || now.tv_secs != timeshown) {
#else
    if (!lastitle || bs != lastitle || now.tv_secs / 60 != timeshown / 60) {
#endif
	lastitle = screentitle[whichtitle];
	hour = ((minute / 60 + 11) % 12) + 1;
	if (bs)
	    SetAllSharedWindowTitles(null);	/* safety */
#ifdef CLOCK_HAS_SECONDS
	sprintf(kalok, "%ld:%02ld:%02ld %lcm", hour, minute % 60,
				now.tv_secs % 60, minute >= 720 ? 'p' : 'a');
#else
	sprintf(kalok, "%ld:%02ld %lcm", hour, minute % 60,
				minute >= 720 ? 'p' : 'a');
#endif
	AppendTitleTip(lastitle, kalok);
	SetAllSharedWindowTitles(lastitle);
	timeshown = now.tv_secs;
    }
    if (!casale) {
	clockq = tiq;
	clockq.tr_node.io_Command = TR_ADDREQUEST;
	GetSysTime(&clockq.tr_time);
#ifdef CLOCK_HAS_SECONDS
	clockq.tr_time.tv_secs = 0;
#else
	clockq.tr_time.tv_secs = 59 - clockq.tr_time.tv_secs % 60;
#endif
	if (clockq.tr_time.tv_micro == 0)
	    clockq.tr_time.tv_secs++;
	else
	    clockq.tr_time.tv_micro = 1000000 - clockq.tr_time.tv_micro;
	SendIO((adr) &clockq);
	casale = true;
    }
}


void UndoTitleBarMsg(bool rightnow)
{
    struct timeval now;
    if (legit_title) {
	if (!rightnow) {
	    GetSysTime(&now);
	    if (CmpTime(&legit_time, &now) < 0)		/* not yet */
		return;
	}
	if (bgwin && bgwin->ScreenTitle == &bogus_title[0]) {
	    SetAllSharedWindowTitles(legit_title);
	    timeshown = -1;
	    UpdateClock();
	}
	legit_title = null;
    }
}


/* RECURSIVE!  handler() often calls EventLoop() when a window is opened. */

void EventLoop(IntuiHandler handler)
{
    struct IntuiMessage mim, *im;
    ulong sigz = 0, timerbit = bit(tipo->mp_SigBit);

    for (;;) {
	if (!(im = (adr) GetMsg(idcort))) {
	    sigz = Wait(bit(idcort->mp_SigBit) | timerbit | SIGBREAKF_CTRL_F);
	    im = (adr) GetMsg(idcort);
	}
	if (sigz & SIGBREAKF_CTRL_F && scr) {
	    ScreenToFront(scr);
	    if (IntuitionBase->ActiveScreen != scr)
		ActivateWindow(scr->FirstWindow);
	}
	if (!im || im->Seconds != timeshown) {		/* partial filter */
	    if (casale && CheckIO((adr) &clockq)) {
		WaitIO((adr) &clockq);			/* immediate */
		casale = false;
	    }
	    UpdateClock();
	}
	while (im) {
	    mim = *im;
	    ReplyMsg((adr) im);
	    lastmouseX = mim.MouseX;
	    lastmouseY = mim.MouseY;
	    if (mim.Class != IDCMP_MENUVERIFY && !ClipboardStringPaste(&mim)
			    && !ScrollIDCMP(&mim, onscreen) && handler(&mim)) {
		StopScroll();
		return;
	    }
	    im = (adr) GetMsg(idcort);
	}
	if (sigz & timerbit && (onscreen || listcc))
	    DoAllTimedScrolling();
	sigz = 0;
    }
}


/* ------------- Higher performance buffered disk IO functions -------------- */


struct BHand {
    BPTR doshand;				/* MUST BE FIRST FIELD! */
    ushort bufoffset, bufend;			/* bufend ignored for writing */
    ushort /* bool */ writing, pad;		/* pad longword aligns buffer */
    ubyte buffer[MYBUFSIZE];
};


BHandle BOpen(str path, bool write)
{
    register struct BHand *bh;

    if (!NEWP(bh)) {
	SetIoErr(ERROR_NO_FREE_STORE);
	return null;
    }
    if (!(bh->doshand = Open(path, write ? MODE_NEWFILE : MODE_OLDFILE))) {
	FREE(bh);
	return null;
    }
    bh->bufoffset = bh->bufend = bh->pad = 0;
    bh->writing = write;
    return bh;
}
/* do NOT read from file opened for writing, or vice versa! */


long BFlush(BHandle h)
{
    register struct BHand *bh = h;
    register long suck = DOSTRUE;
    if (bh) {
	if (bh->writing) {
	    if (bh->bufoffset > 0)
		suck = Write(bh->doshand, bh->buffer, (long) bh->bufoffset)
					== bh->bufoffset ? DOSTRUE : 0;
	} else if (bh->bufoffset < bh->bufend)
	    suck = SSeek(bh->doshand, (long) bh->bufoffset - (long) bh->bufend,
					OFFSET_CURRENT) >= 0 ? DOSTRUE : 0;
	bh->bufoffset = bh->bufend = 0;
    }
    return suck;
}


long BClose(BHandle h)
{
    register struct BHand *bh = h;
    register long suck;
    if (!bh) return 0;
    suck = (bh->writing ? BFlush(h) : DOSTRUE) & Close(bh->doshand);
    FREE(bh);
    return suck;
}


long BSeek(BHandle h, long offset, long mode)
{
    register struct BHand *bh = h;
    if (!bh)
	return -1;
    if (mode == OFFSET_CURRENT && ((offset < 0 && -offset <= bh->bufoffset)
		    || (offset >= 0 && !bh->writing && offset < bh->bufend))) {
	long oldpos = SSeek(bh->doshand, 0, OFFSET_CURRENT);
	if (oldpos >= 0)	/* calculate virtual position from physical */
	    if (bh->writing)
		oldpos += bh->bufoffset;
	    else
		oldpos -= bh->bufend - bh->bufoffset;
	bh->bufoffset += offset;
	return oldpos;
    } else
	return BFlush(h) & SSeek(bh->doshand, offset, mode);
}


#pragma regcall(BRefill(a0))

local long BRefill(struct BHand *bh)
{
    long r = Read(bh->doshand, bh->buffer, MYBUFSIZE);
    bh->bufoffset = 0;
    bh->bufend = max(r, 0);
    ASSERT(!bh->writing);
    return r;
}


long BRead(BHandle h, APTR buf, ulong blocklen, ulong blocks)
{
    register struct BHand *bh = h;
    register ulong done = 0;
    register short chunk;
    ulong size = blocklen * blocks;

    if (!bh || !buf || !size || bh->writing)
	return -1;
    do {
	chunk = bh->bufend - bh->bufoffset;
	if (chunk > size - done)
	    chunk = size - done;
	if (chunk) {
	    memmove((char *) buf + done, bh->buffer + bh->bufoffset, chunk);
	    bh->bufoffset += chunk;
	    done += chunk;
	}
    } while (done < size && BRefill(bh) > 0);
    if (size = done % blocklen)		/* compensate for partial block */
	BSeek(h, -size, OFFSET_CURRENT);
    return done / blocklen;
}


long BWrite(BHandle h, APTR buf, ulong blocklen, ulong blocks)
{
    register struct BHand *bh = h;
    ulong size = blocklen * blocks;
    register ulong done = 0;
    register short chunk;

    if (!bh || !buf || !size || !bh->writing)
	return 0;
    do {
	chunk = MYBUFSIZE - bh->bufoffset;
	if (chunk > size - done)
	    chunk = size - done;
	if (chunk) {
	    memmove(bh->buffer + bh->bufoffset, (char *) buf + done, chunk);
	    bh->bufoffset += chunk;
	    done += chunk;
	}
    } while (done < size && BFlush(h));
    if (size = done % blocklen)		/* compensate for partial block */
	BSeek(h, -size, OFFSET_CURRENT);
    return done / blocklen;
}

/* A quick test shows this speeding up WriteREP by about a factor of 3! */


/* Unlike FGets, this returns the number of bytes read, or -1 for error.    */
/* Like strac(), it stops reading at either a \r or a \n, and skips over \n */
/* after \r, the terminating newline is NOT copied into the buffer, and if  */
/* the buffer fills it reads and DISCARDS data until it reaches a newline.  */
/* Returns zero when reading a blank line; returns -1 if no data to read.   */
/* bufsize is the number of bytes to copy NOT counting terminating null.    */

#ifdef C_STRS

long BGetline(BHandle h, str buf, ulong bufsize)
{
    register struct BHand *bh = h;
    register short chunk;
    register long done = 0;
    bool anything = false;

    if (!bh || bh->writing || !buf)    /* bufsize == 0 is okay, zeroes buf[0] */
	return -1;
    for (;;) {
	while (bh->bufoffset < bh->bufend) {
	    register char c = bh->buffer[bh->bufoffset++];
	    anything = true;
	    if (c == '\r' || c == '\n') {
		if (c == '\r' && (bh->bufoffset < bh->bufend || (BRefill(bh) > 0
					/* && bh->bufoffset < bh->bufend */ ))
					&& bh->buffer[bh->bufoffset] == '\n')
		    bh->bufoffset++;
		goto break2;
	    }
	    if (done < bufsize)
		buf[done++] = c;
	}
	chunk = BRefill(bh);
/*	if (chunk < 0) {
/*	    buf[done] = '\0';
/*	    return -1;
/*	}
/*	if (!chunk)
*/	if (chunk <= 0)
	    break;
    }
  break2:
    buf[done] = '\0';
    return anything ? done : -1;
}

#else

long BGetline(BHandle h, str buf, ulong bufsize);

#  asm

; CHANGE THIS to stay in sync if struct BHand is ever altered!
doshand		EQU	0
bufoffset	EQU	4
bufend		EQU	6
writing		EQU	8
pad		EQU	10
buffer		EQU	12

regz		REG	a2/d2-d3

bh		EQUR	a0
buf		EQUR	a1
bufsize		EQUR	d1
done		EQUR	d2
anything	EQUR	d3

		XREF	_BRefill
		XDEF	_BGetline

_BGetline:
	move.l		4(sp),bh
	move.l		8(sp),buf
	move.l		12(sp),bufsize
	movem.l		regz,-(sp)
	moveq		#0,anything
	move.l		bh,d0			; fake tst.l
	beq.s		out
	tst.w		writing(bh)
	bne.s		out
	move.l		buf,d0			; fake tst.l
	beq.s		out
	moveq		#0,done			; safety checks OK: go ahead
	lea		buffer(bh),a2

loupe:	  move.w	bufoffset(bh),d0
	  cmp.w		bufend(bh),d0		; have chars to process?
	  bhs.s		harris			; ran out -- read some more
	    move.b	(a2,d0.w),d0		; get the next input byte
	    addq.w	#1,bufoffset(bh)	; increment buffer position
	    moveq	#1,anything		; we will return success
	    cmp.b	#10,d0			; reached a linefeed?
	    beq.s	gout			; yes, we are finished
	    cmp.b	#13,d0			; reached a carriage return?
	    bne.s	noeol			; no, keep going
	      move.w	bufoffset(bh),d0	; yes -- check for following LF
	      cmp.w	bufend(bh),d0		; anything in the buffer?
	      blo.s	norfl			; yes
		movem.l	bh/buf/bufsize,-(sp)	; no, get more chars
		jsr	_BRefill		; the arg bh is already in a0
		movem.l	(sp)+,bh/buf/bufsize
		tst.w	d0			; did we get any?
		ble.s	gout			; no, we are finished
	        move.w	bufoffset(bh),d0
norfl:	      move.b	(a2,d0.w),d0		; check buf's next char
	      cmp.b	#10,d0			; is it a linefeed?
	      bne.s	gout			; no -- we are finished
	      addq.w	#1,bufoffset(bh)	; yes, discard it from buffer
	      bra.s	gout			; and we are finished

noeol:	    cmp.l	done,bufsize		; still room for output?
	    bls.s	loupe			; no, discard but keep reading
	      move.b	d0,(buf,done.l)		; yes, copy this char
	      addq.l	#1,done
	    bra.s	loupe			; do the next byte
harris:	  movem.l	bh/buf/bufsize,-(sp)
	  jsr		_BRefill		; the arg bh is already in a0
	  movem.l	(sp)+,bh/buf/bufsize
	  tst.w		d0
	  bgt.s		loupe			; we now have more to process
;;;	moveq		#0,anything		; if BRefill fails so do we

gout:	move.l		done,d0
	clr.b		(buf,done.l)		; null-terminate our result
out:	tst.w		anything
	bne.s		fiout
	  moveq		#-1,d0			; EOF or other failure
fiout:	movem.l		(sp)+,regz
	rts

#  endasm
#endif
