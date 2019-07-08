/* display BBS messages on the screen */

#include <dos/dosextens.h>
#include <dos/dostags.h>
#include <intuition/intuition.h>
#include <devices/timer.h>
#include <stdlib.h>
#include "qblue.h"


#define RESTORETIME    4
#define TENTSHORTLINES 5
#define BGSCROLLWAIT   333333

#define FROMCOLOR    1
#define TOOCOLOR     7
#define SUBJECTCOLOR /* 5 */ 1
#define DATECOLOR    /* 1 */ 5
#define AREACOLOR    /* 7 */ 5
#define AREACOLOR4   3
#define NUMSCOLOR    7
#define RNUMSCOLOR   5
#define INUMSCOLOR   /* 5 */ 1
#define FBITCOLOR    5
#define BBITCOLOR    3
#define ENDLINECOLOR 3


#ifdef DELAY_MARK_AS_READ
/* const */ struct timeval TtV_S = { 0, 500000 };    /* half second */
/* const */ struct timeval TtV_L = { 1, 0 };         /* one second */

#  define TIMETILVALID_S  &TtV_S
#  define TIMETILVALID_L  &TtV_L
#endif


bool DoFileRequest(bool saving, bool special, str result,
				str deflt, str pattern, str hail);
bool PackReplies(void);
long ShellExecute(str command, BPTR in, BPTR out);
BPTR SafeOpenFromLock(BPTR lok);
ulong DS2ux(struct DateStamp *d);
bool CopyFile(BPTR inh, str outpath);

bool AskReallyQuit(void);
bool AskPackReplies(bool *cancellation);
bool AskDeleteArchive(bool quitting, bool *cancelflag);
bool AskNonReplyDelete(str archivename);
void Aboutness(void);
void Iconify(bool instant);
void SaveBookmarks(void);
void ShowSysopEtc(void);
void sprintjam(str dest, str format /* must contain "%s%s" only */, str path);
void SetAllSharedWindowTitles(str title);
void UndoTitleBarMsg(bool rightnow);
void StopClipping(bool abort);
void UpdateClock(void);
void AppendTitleTip(str shaft, str head);
bool FreeTaglines(void);

void SwitchRGag(bool replies, bool deleted);
void TweakCloseGag(void);
void SetMessGags(bool on);
void AdjustBGslider(struct Mess *vis, bool force);
void MakeBGslider(void);
void NukeBGslider(void);
void StartClipping(void);
void ContinueClipping(void);
void StopClipping(bool abort);
void SetAlarm(ulong micros);
short XNominal(register short x);

void ComposeMsg(struct Mess *editee, bool fixing,
			bool addressee, struct Mess *carbon);
void Pontificate(struct Mess *mm);
void GetBWReplyFilename(str fame, struct Mess *mm);

bool Soich(whatsearch whats);
void UnSearch(struct Conf *where, bool freshen);
void Resort(bool movement);
void TabulateBodyMatches(struct Mess *mm);
#ifdef FANCY_INVERSION
ushort HowManyInverted(register ushort line, register ushort col, bool *verse);
bool InvertedHere(ushort line, ushort col);
#endif
void ShowHits(short look);

bool LoadPktMessage(struct Mess *mm);
void PreloadConf(struct Conf *cc, ushort which);
void BGupdate(struct Conf *cc);
bool MakeRoom(bool here);
void StripMessage(struct Mess *mm);
bool SaveToFile(struct Mess *savee, str filename, short whichreply);
void WriteUPL(void);
void FormatNetAddress(str buf, ushort zone,
			ushort net, ushort node, ushort point);

void DoAllTimedScrolling(void);
short ListMessages(struct Conf *cc, short defaultm);
short ListAreas(short initialarea, bool empties, bool addrop, bool minusone);
void MyScrollRaster(struct RastPort *rp, /* short dx, */ short dy,
			short xmin, short ymin, short xmax, short ymax);
bool TaglineWindow(bool timid, bool maintenance);

void ConfigEditor(void), ConfigDirs(void), ConfigCompressors(void);
void ConfigOptions(void), ConfigReplying(void);
void FiddleFigFile(bool saving);
void PickDLRequests(void);
void ConfigBWDoor(void);
void ConfigFontScreen(void);
void ConfigBLocal(void);
#ifdef FUNCTION_KEYS
void ConfigFKeys(void);
bool DoFunctionKeyCommand(ubyte key);
#endif


import struct Window *lawin, *swin;
import struct RastPort *bgrp, bgrpblack;
import struct MenuItem passum[], misort0, misortlast, misave1, mirep4a;
import struct IntuiText bgt0p;
import struct Gadget bgdownarrow, bguparrow, bgscroll, tgagstr, *lastbutton;
import struct PropInfo bgprops;
import struct MsgPort *tipo;

import struct Conf *curconf;

import ustr *newsgroups;

import BPTR originalcd;

import char screentitle[4][80], thefilenote[];
import char printerpath[], savename[], defsavename[], darchivename[];

import long lastbgpot;
import ushort sorder, whichtitle, packers, currentpacker, nomwidth;
import ushort sliderspace;
import short lastmouseX, lastmouseY;

import bool showlistwin, alswitch, beepu, formfeed, showareawin, askdelete;
import bool filerequests_exist, quittitude, rupted, addsdrops, questionating;
import bool anysortclobbered, lastfirst, pdq_exists, backslide;
import bool quitleaveopen, amclipping, obscuringslider, popupscroller;


struct Mess *onscreen = null;
#ifdef DELAY_MARK_AS_READ
struct Mess *tentativity = null;
#endif

struct timeval legit_time, view_time;
struct Gadget *lastbgbutton = null;

ushort typecolors[] = { 1, 1, 7, 6, 5, 1, 1, 7, 6, 5, 1 };
/* when chopped to four colors = 1 1 3 2 1 1 1 3 2 1 1 */

ushort winlines, textop, extralines, deferline = 0, slotop = 0, slotbot = 65535;
short topline, remembertopline = 0, whicha, whichm, lastcolor = -1;
short oldcentread = 0;
long labelcolor;
ulong totaltoread, totalunread, oldcentsize, oldcenttime;

bool buttoning = false, deferrable = false, appendage, slidestifle = false;

ubyte bogus_title[80], *legit_title = null;

static struct Mess *lastseen = null;
static struct Conf *lastccshown;


void MoveNominal(struct RastPort *rp, short x, short y)
{
    Move(rp, (long) XActual(x), (long) y);
}


void MyClearEOL(void)
{
    long top = bgrp->cp_y - font->tf_Baseline;
    long right = !backslide && !obscuringslider ? bgwin->Width
                 : bgwin->Width - bgscroll.Width;
    
    RectFill(&bgrpblack, bgrp->cp_x, top,
             /* min(right, nomwidth) */ right - 1, top + fight - 1);
    /* ASSERT(bgrpblack.cp_x <= bgwin->Width && bgrpblack.cp_x <= nomwidth); */
}


void Show1Line(struct Mess *mm, short n, short lmar)
{
    ustr line;
    short rmar = 80;
    long color, base = n * fight + textop + font->tf_Baseline;
    ushort limit;
    struct Attachment *att = ATTACHED(mm);
#ifdef FANCY_INVERSION
    ushort sofar, off, kholer;
    bool verse, ov;
#endif

    if (obscuringslider)
	rmar -= (fontwid + bgscroll.Width - 1) / fontwid;
    ASSERT(n >= 0 && n < winlines);
    ASSERT(mm && bgrp);
    ASSERT(lmar < rmar);
    Move(bgrp, XActual(lmar * 8), base);
    n += topline - !!att;
    if (n == -1) {
	if (att) {
	    char buf[80];
	    att->arrivename[30] = '\0';		/* paranoia */
	    sprintf(buf, " * Attached file: %s %lc(use Alt-T to save) ",
		    att->arrivename, (long) (lastccshown == &replies ? '\0' : ' '));
	    limit = strlen(buf);
	    if (limit > rmar) limit = rmar;
	    SetAPen(bgrp, lastcolor = !fourcolors);
	    SetBPen(bgrp, ENDLINECOLOR);
	    Text(bgrp, buf + lmar, (ulong) limit - lmar);
	    SetBPen(bgrp, 0);
	}
	MyClearEOL();
	return;
    }
    if (n == mm->linect) {
	long l1 = XActual(200), l2 = XActual(440) - 1;
	MyClearEOL();
	if (lmar < 56) {
	    SetAPen(bgrp, lastcolor = ENDLINECOLOR);
	    Move(bgrp, l1, base - 2);
	    Draw(bgrp, l2, base - 2);
	    Move(bgrp, l1, base - 3);
	    Draw(bgrp, l2, base - 3);
	}
	return;
    }
    if (n > mm->linect || !(line = mm->lines[n]) || (limit = line[-1]) <= lmar) {
	if (lmar < 75)
	    MyClearEOL();
	return;
    }
    color = typecolors[mm->linetypes[n] >> TYPE_SHIFT];
    if (limit > rmar)
	limit = rmar;
#ifdef FANCY_INVERSION
    if (mm->bits & BODYMATCH) {
	ov = false;
	verse = InvertedHere(n, sofar = 0);
	Move(bgrp, 0, base);	/* don't try to optimize this shit for lmar */
	do {
	    kholer = color;
	    if (ov != verse)
		if (ov = verse) {
		    SetDrMd(bgrp, JAM2 | INVERSVID);
		    kholer = SEARCHLITE(color);
		} else
		    SetDrMd(bgrp, JAM2);
	    sofar = HowManyInverted(n, off = sofar, &verse);
	    if (sofar > limit)
		sofar = limit;
	    if (kholer != lastcolor)
		SetAPen(bgrp, lastcolor = kholer);
	    Text(bgrp, line + off, (ulong) sofar - off);
	} while (sofar < limit);
	if (ov)
	    SetDrMd(bgrp, JAM2);
    } else {
	if (color != lastcolor)
	    SetAPen(bgrp, lastcolor = color);
	Text(bgrp, line + lmar, (ulong) limit - lmar);
    }
#else
    if (color != lastcolor)
	SetAPen(bgrp, lastcolor = color);
    Text(bgrp, line + lmar, (ulong) limit - lmar);
    ASSERT(bgrp->cp_x <= XActual(rmar * 8));
#endif
    if (limit < rmar && lmar < 75)
	MyClearEOL();
}


void ShowAllLines(struct Mess *mm)
{
    short i;
    lastcolor = -1;
    for (i = 0; i < winlines; i++)
	Show1Line(mm, i, 0);
#ifndef FANCY_INVERSION
    if (mm->bits & BODYMATCH && !amclipping)
	ShowHits(-1);
#endif
}


void PaintOverPopupScroller(struct Mess *mm)
{
    short i;
    short lmar = 80 - (fontwid + bgscroll.Width + 1) / fontwid;
    lastcolor = -1;
    for (i = 0; i < winlines; i++)
	Show1Line(mm, i, lmar);
#ifndef FANCY_INVERSION
    if (mm->bits & BODYMATCH && !amclipping)
	ShowHits(-1);
#endif
}


bool ScrollDownLine(struct Mess *mm, short xtra)
{
    short tlimit = mm->linect + !!ATTACHED(mm) - (slidestifle ? 1 : winlines - 1);
    short redge = nomwidth;

    if (obscuringslider)
	redge = min(bgwin->Width - bgscroll.Width - 1, nomwidth);
    if (topline >= tlimit)
	return false;
    if (topline + xtra > tlimit)
	xtra = tlimit - topline;
    topline += xtra;
    AdjustBGslider(mm, true);
    MyScrollRaster(bgrp, /* 0, */ fight * xtra,
				0, textop, redge - 1, texbot - 1);
    lastcolor = -1;
    slotop = winlines - xtra;
    while (xtra > 0)
	Show1Line(mm, winlines - xtra--, 0);
#ifndef FANCY_INVERSION
    if (mm->bits & BODYMATCH && !amclipping)
	ShowHits(-1);
#endif
    slotop = 0;
    return true;
}


bool ScrollUpLine(struct Mess *mm, short xtra)
{
    short redge = nomwidth;

    if (obscuringslider)
	redge = min(bgwin->Width - bgscroll.Width - 1, nomwidth);
    if (topline <= 0)
	return false;
    if (xtra > topline)
	xtra = topline;
    topline -= xtra;
    AdjustBGslider(mm, true);
    MyScrollRaster(bgrp, /* 0, */ - fight * xtra,
				0, textop, redge - 1, texbot - 1);
    lastcolor = -1;
    slotbot = xtra;
    while (xtra > 0)
	Show1Line(mm, --xtra, 0);
#ifndef FANCY_INVERSION
    if (mm->bits & BODYMATCH && !amclipping)
	ShowHits(-1);
#endif
    slotbot = 65535;
    return true;
}


void ScrollDownPage(struct Mess *mm)
{
    short limit = mm->linect + !!ATTACHED(mm) + 1 - winlines;
    if (topline >= limit)
	return;
    if (limit - topline <= 2)
	ScrollDownLine(mm, 2);
    else {
	topline += winlines - 1;
	AdjustBGslider(mm, true);
	ShowAllLines(mm);
    }
}


void ScrollUpPage(struct Mess *mm)
{
    if (!topline)
	return;
    topline -= winlines - 1;
    if (topline < 0)
	topline = 0;
    AdjustBGslider(mm, true);
    ShowAllLines(mm);
}


void ScrollBottom(struct Mess *mm)
{
    short i = mm->linect + !!ATTACHED(mm) + 1 - winlines;
    if (topline == i)
	return;
    topline = i;
    if (topline < 0)
	topline = 0;
    else {
	AdjustBGslider(mm, true);
	ShowAllLines(mm);
    }
}


void ScrollTop(struct Mess *mm)
{
    if (!topline)
	return;
    topline = 0;
    AdjustBGslider(mm, true);
    ShowAllLines(mm);
}


#define Putext(s) Text(bgrp, s, strlen(s))


/* Rule: nobody sets bgrp->BgPen nonzero without resetting it afterwards. */

void DoBit(short line, ushort bit, str werd)
{
    short color = (bit ? (fourcolors ? 0 : FBITCOLOR) : backcolor);
    Move(bgrp, bgwin->Width - XActual(40),
				fight * line + 4 + font->tf_Baseline);
    if (color != lastcolor)
	SetAPen(bgrp, lastcolor = color);
    SetBPen(bgrp, (bit ? BBITCOLOR : backcolor));
    Putext(werd);
}


void ShowHeader(struct Conf *cc, struct Mess *mm)
{
    long b = font->tf_Baseline + 4;
    char buf[100];
    struct Conf *scc = cc;
    short slideoff = 8 * backslide;
    bool longtoo;
    str tooth = mm->too;

    ASSERT(bgrp && cc && mm);
    if (cc == &replies || cc == &personals)
	if (!(scc = Confind(mm->confnum)))
	    scc = cc;
    if (cc == &replies && scc->morebits & MULTI_NEWSGROUP && newsgroups[whichm])
	tooth = newsgroups[whichm];
    longtoo = !(mm->bluebits & UPL_INACTIVE)
				&& strlen(tooth) > backslide + NAMELEN - 1;
    SetAPen(bgrp, backcolor);
    RectFill(bgrp, 4, 3, bgwin->Width - 5, fight * 4 + 4);
    SetBPen(bgrp, backcolor);
    SetAPen(bgrp, labelcolor);
    if (cc != &bullstuff) {
	MoveNominal(bgrp, 8, b);
	Putext(" From: ");
	if (!longtoo) {
	    MoveNominal(bgrp, 360 + slideoff, fight + b);
	    Putext("Msg#: ");
	}
    }
    MoveNominal(bgrp, 8, fight + b);
    if (cc == &bullstuff)
	Putext(" File: ");
    else
	Putext("   To: ");
    MoveNominal(bgrp, 8, 2 * fight + b);
    Putext("Subj.: ");
    MoveNominal(bgrp, 360 + slideoff, b);
    Putext("Date: ");

    if (cc != &bullstuff) {
	MoveNominal(bgrp, 64, b);
	SetAPen(bgrp, FROMCOLOR);
	Putext(mm->from);
    }
    SetAPen(bgrp, TOOCOLOR);
    MoveNominal(bgrp, 64, fight + b);
    if (scc->morebits & NEWSGROUP && !tooth[0]) {
	Putext("All");
    } else {
	strncpy0(buf, tooth, longtoo ? 66 + 2 * backslide
					    : backslide + NAMELEN - 1);
	Putext(buf);
    }
    if (mm->subject) {
	MoveNominal(bgrp, 64, 2 * fight + b);
	SetAPen(bgrp, SUBJECTCOLOR);
	Putext(mm->subject);
    }
    MoveNominal(bgrp, 408 + slideoff, b);
    SetAPen(bgrp, DATECOLOR);
    Putext(mm->date + (mm->date[0] == ' '));
    MoveNominal(bgrp, 472 + slideoff, 3 * fight + b);
    SetAPen(bgrp, lastcolor = INUMSCOLOR);
    sprintf(buf, "#%lu of %lu", whichm + 1L, (ulong) cc->messct);
    Putext(buf);
    if (cc == &bullstuff) {
	SetBPen(bgrp, 0);
	return;
    }

    if (!longtoo) {
	MoveNominal(bgrp, 408 + slideoff, fight + b);
	if (cc == &replies) {
	    if (mm->bluebits & UPL_INACTIVE) {
		SetAPen(bgrp, fourcolors ? 0 : 1);
		SetBPen(bgrp, 3);
		Putext("DELETED");
		SetBPen(bgrp, backcolor);
	    }
	    SetAPen(bgrp, NUMSCOLOR);
	    if (mm->replyto) {
		sprintf(buf, " reply to #%lu", mm->replyto);
		Putext(buf);
	    } else
		Text(bgrp, " new message", 12);
	} else {
	    bool cr1 = false, cr3 = false;
	    if (mm->replyto && mm->replyat && mm->ixinbase > 9999)
		cr1 = mm->replyto > 99999, cr3 = mm->replyat > 99999;
	    if (mm->replyto) {
		SetAPen(bgrp, RNUMSCOLOR);
		sprintf(buf, "%lu%s<- ", mm->replyto, cr1 ? "" : " ");
		Putext(buf);
	    }
	    SetAPen(bgrp, NUMSCOLOR);
	    sprintf(buf, "%lu", mm->ixinbase);
	    Putext(buf);
	    if (mm->replyat) {
		SetAPen(bgrp, RNUMSCOLOR);
		sprintf(buf, " ->%s%lu", cr3 ? "" : " ", mm->replyat);
		Putext(buf);
	    }
	}
    }
    MoveNominal(bgrp, 8, fight * 3 + b);
    if (mm->bluebits & UPL_NETMAIL) {
	SetAPen(bgrp, labelcolor);
	if (cc == &replies)
	    Putext("Netmail destination: ");
	else
	    Putext("Netmail from: ");
	SetAPen(bgrp, lastcolor = (fourcolors ? AREACOLOR4 : AREACOLOR));
	if (mm->net)
	    FormatNetAddress(buf, mm->zone, mm->net, mm->node, mm->point);
	else
	    strcpy(buf, "(no address)");
	Putext(buf);
    } else {
	SetAPen(bgrp, labelcolor);
	Putext(" Area: ");
	SetAPen(bgrp, lastcolor = (fourcolors ? AREACOLOR4 : AREACOLOR));
	Putext(LONGNAME(scc));
    }
    DoBit(0, mm->bluebits & UPL_PRIVATE, "Priv");
    DoBit(1, mm->bits & SEENINBASE, "Rcvd");
    DoBit(3, mm->bits & MEREPLIED, "Repl");
    SetBPen(bgrp, 0);
}


bool Irped(void)
{
    return deferrable && SetSignal(0, 0)
			& (bit(tipo->mp_SigBit) | bit(idcort->mp_SigBit));
}


void ShowNewMessage(struct Conf *cc, struct Mess *mm)
{
#ifdef DELAY_MARK_AS_READ
    struct timeval now;
#endif
    short i;
    bool flummoxed;

    if (!cc)	 /* bgupdate is finishing off leftover display work, or */
	cc = lastccshown, flummoxed = false;	/* we are un-iconifying */
    else {
	if (flummoxed = whichm < 0 || whichm >= cc->messct) {
#ifdef BETA
	    Err("INTERNAL ERROR -- attempted to display\n"
			"message #%ld out of only %ld in this area,\n%s",
			whichm + 1L, (ulong) cc->messct, LONGNAME(cc));
#endif
	    mm = cc->messes[whichm = 0];
	}
	cc->current = whichm;
    }
    if (mm != onscreen)
	deferline = 1;
    if (!deferline)
	return;
    StopClipping(true);
#ifdef DELAY_MARK_AS_READ
    if (tentativity && tentativity != mm && !lawin && !cwin) {
	GetSysTime(&now);
	if (CmpTime(&view_time, &now) < 0)
	    tentativity->bits &= ~MESEEN;		/* Revoke! */
	tentativity = null;
    }
#endif
    if (!waste && onscreen && mm != onscreen)
	if (!(onscreen->bits & DONTSTRIP))
	    StripMessage(onscreen);
    if (!flummoxed && Irped())
	return;
    LoadPktMessage(mm);
    if (!flummoxed && Irped())
	return;
    if (!(mm->bits & POINTLESS) && mm->bluebits & UPL_NETMAIL && cc != &replies)
	Pontificate(mm);
    topline = remembertopline;
    if (deferline <= 1) {
	SwitchRGag(cc == &replies, mm->bluebits & UPL_INACTIVE);
	ShowHeader(cc, mm);
	AdjustBGslider(mm, true);
	if (mm == onscreen) {
	    deferline = 0;
	    return;
	}
	if (mm->bits & BODYMATCH)
	    TabulateBodyMatches(mm);
    }
    onscreen = mm;
    lastccshown = cc;
    lastcolor = -1;
    for (i = (deferline ? deferline - 1 : 0); i < winlines; i++) {
	Show1Line(mm, i, 0);
	if (Irped()) {
	    deferline = i + 2;
	    return;
	}
    }
#ifndef FANCY_INVERSION
    if (mm->bits & BODYMATCH)
	ShowHits(-1);
#endif
    deferline = 0;
    if (!deferrable) {
	if (beepu && cc != &replies && mm->bits & TOME)
	    DisplayBeep(scr);
/*	if (rupted && waste)
	    PreloadConf(cc, whichm);  */	/* useless! */
    }
    while (waste && (AvailMem(0) < 50000 || !lawin) && !Irped()
						&& MakeRoom(false)) ;
}


void SlideBGslider(struct Mess *mm)
{
    short line = (bgprops.VertPot * (sliderspace - winlines)
				+ (MAXPOT >> 1)) / (MAXPOT + 1); /* >> 16 */
    lastbgpot = bgprops.VertPot;
    if (line < 0) line = 0;
    if (line != topline) {
	if (line > topline && line - topline < winlines - 1)
	    ScrollDownLine(mm, line - topline);
	else if (line < topline && topline - line < winlines - 1)
	    ScrollUpLine(mm, topline - line);
	else {
	    topline = line;
	    ShowAllLines(mm);
	}
    }
}


/* like all our *IDCMP()s, returns true if the event has been handled */

bool ScrollIDCMP(struct IntuiMessage *im, struct Mess *mm)
{
    short gid;
    bool downer;

    if (questionating || !mm)
	return false;
    switch (im->Class) {
      case IDCMP_RAWKEY:
	if (lawin || swin)		/* these windows use arrow keys */
	    return false;
	if (im->Code & IECODE_UP_PREFIX)
	    return true;				/* ignore key release */
	switch (im->Code) {
	  case 0x3E:					/* uparrow, kp8 */
	  case 0x4C:
	    if (im->Qualifier & IEQUALIFIER_CONTROL)
		ScrollTop(mm);
	    if (im->Qualifier & (SHIFTKEYS | ALTKEYS))
		ScrollUpPage(mm);
	    else
		ScrollUpLine(mm, 1);
	    break;
	  case 0x4D:					/* downarrow, kp2 */
	  case 0x1E:
	    if (im->Qualifier & IEQUALIFIER_CONTROL)
		ScrollBottom(mm);
	    if (im->Qualifier & (SHIFTKEYS | ALTKEYS))
		ScrollDownPage(mm);
	    else
		ScrollDownLine(mm, 1);
	    break;
	  case 0x3F:					/* kp9 / PgUp */
	    ScrollUpPage(mm);
	    break;
	  case 0x1F:					/* kp3 / PgDn */
	    ScrollDownPage(mm);
	    break;
	  case 0x3D:					/* kp7 / Home */
	    ScrollTop(mm);
	    break;
	  case 0x1D:					/* kp1 / End */
	    ScrollBottom(mm);
	    break;
	  default:
	    return false;
	}
	break;
      case IDCMP_MOUSEBUTTONS:
	if (im->Code & IECODE_UP_PREFIX)
	    StopClipping(false);
	if (im->IDCMPWindow != bgwin)
	    return false;
	if (buttoning = !(im->Code & IECODE_UP_PREFIX) && onscreen) {
	    lastbgbutton = lastbutton = null;
	    if (lastmouseY >= textop && lastmouseY < texbot - fight
					    && im->Code == IECODE_LBUTTON) {
		StartClipping();	/* middle button = scroll but no clip */
	    } else
		SetAlarm(BGSCROLLSPEED);
	} else
	    StopScroll();
	break;
      case IDCMP_MOUSEMOVE:
	if (bgscroll.Flags & GFLG_SELECTED) {
	    slidestifle = true;
	    SlideBGslider(onscreen);
	    slidestifle = false;
	} else if (buttoning) {
	    DoAllTimedScrolling();	/* won't otherwise get called... */
	    return true;                /* it calls ContinueClipping() */
	} else if (popupscroller && !backslide && !obscuringslider
		   && im->MouseX >= nomwidth - 32)
	    MakeBGslider();
	else if (popupscroller && !backslide && obscuringslider
	         && im->MouseX < nomwidth - 32)
	    NukeBGslider();
	else
	    return false;
	break;
      case IDCMP_INACTIVEWINDOW:
	StopScroll();
	break;
      case IDCMP_GADGETDOWN:
      case IDCMP_GADGETUP:
	downer = im->Class == IDCMP_GADGETDOWN;
	gid = ((struct Gadget *) im->IAddress)->GadgetID;
	if (gid != 12 && (!downer || (gid != 10 && gid != 11)))
	    return false;
	buttoning = false;
	StopScroll();
	switch (gid) {
	  case 10:				/* scroll down */
	    lastbgbutton = &bgdownarrow;
	    if (ScrollDownLine(onscreen, 1))
		SetAlarm(BGSCROLLWAIT);
	    break;
	  case 11:				/* scroll up */
	    lastbgbutton = &bguparrow;
	    if (ScrollUpLine(onscreen, 1))
		SetAlarm(BGSCROLLWAIT);
	    break;
	  case 12:				/* slider */
	    SlideBGslider(onscreen);
	    slidestifle = false;
	    if (!downer) {
		StopScroll();
		AdjustBGslider(onscreen, false);
	    }
	    break;
	}
	break;
      default:
	return false;
    }
    return true;
}


void DoList(struct Conf *cc)
{
    void PickArea(bool force);		/* forward reference */

    whichm = ListMessages(cc, whichm);
    cc = readareaz.confs[whicha];	/* may have changed */
    if (alswitch)
	PickArea(true);
    else
	ShowNewMessage(cc, cc->messes[whichm]);
}


void AdjustToNewArea(bool muffle)
{
    struct Conf *cc = readareaz.confs[whicha];
    bool reps = cc == &replies;
    str oldt;

    ASSERT(whicha >= 0 && whicha < readareaz.messct);
    if (cc->current < 0 || cc->current >= cc->messct) {
#ifdef BETA
	Err("INTERNAL ERROR -- invalid current field\n"
			"in read area %ld (%s) -- #%ld of %ld!", (long) whicha,
			cc->shortname, cc->current + 1L, (long) cc->messct);
#endif
	cc->current = 0;
    }
    whichm = cc->current;
    readareaz.current = whicha;
    if (cc == &bullstuff)
	whichtitle = 3;
    else if (reps)
	whichtitle = 2;
    else if (cc == &personals)
	whichtitle = 1;
    else
	whichtitle = 0;
    if ((oldt = bgwin->ScreenTitle) != screentitle[whichtitle]) {
	SetWindowTitles(bgwin, null, null);
	if (oldt)
	    AppendTitleTip(oldt, "");
	UpdateClock();		   /* sets titles to screentitle[whichtitle] */
    }
    SwitchRGag(reps, reps && cc->messes[whichm]->bluebits & UPL_INACTIVE);
    FlipBGadgets(cc == &bullstuff ? 0x77 : 0x7f);
    if (((showlistwin && !(fakery && cc == &bullstuff)) || alswitch) && !muffle)
	DoList(cc);	/* will call AdjustToNewArea again if area changed */
    else
	ShowNewMessage(cc, cc->messes[whichm]);
}


void PickArea(bool force)
{
    short sewage;
    wasescaped = false;
    if (force | showareawin) {
	sewage = ListAreas(whicha, false, false, false);
	if (!wasescaped)
	    whicha = sewage;
    }
    AdjustToNewArea(wasescaped);
}


void DecideIfAnythingToSave(void)
{
    short i;
    if (!(anythingtosave = filerequests_exist ||
				(qwk ? addsdrops : pdq_exists)))
	for (i = 0; i < replies.messct; i++)
	    if (!(replies.messes[i]->bluebits & UPL_INACTIVE)) {
		anythingtosave = true;
		break;
	    }
    TweakCloseGag();
}


void ToggleDeleted(struct Conf *cc)
{
    struct Mess *mm = cc->messes[whichm];
    char fame[14];

    if (cc != &replies)
	return;
    if (!mm->bits & LOADED) {		/* should never happen */
	Err("INTERNAL ERROR: For some reason, no\ncopy of the text is saved"
				"in memory.\nNot possible to undelete.");
	if (mm->bluebits & UPL_INACTIVE)
	    return;
    }
    PortOff();
    mm->bluebits ^= UPL_INACTIVE;
    if (!qwk) {
	BPTR oldcd = CurrentDir(replylock);
	ASSERT(replylock);
	GetBWReplyFilename(fame, mm);
	if (mm->bluebits & UPL_INACTIVE)
	    DeleteFile(fame);
	else if (!SaveToFile(mm, fame, whichm))
	    mm->bluebits |= UPL_INACTIVE;
	CurrentDir(oldcd);
    }
    repchanges = true;
    SwitchRGag(true, mm->bluebits & UPL_INACTIVE);
    ShowHeader(cc, mm);
    DecideIfAnythingToSave();
    WriteUPL();
    PortOn();
}


void SaveAttachment(struct Mess *mm, bool isreply)
{
    BPTR fan, ocd, wd;
    char fame[COMMANDLEN];
    struct Attachment *att = ATTACHED(mm);

    if (!att) {
	Err("This message has no\nattached file to save");
	return;
    }
    if (isreply) {
	ocd = CurrentDir(replylock);
	fan = OOpen(att->tempname);
	if (!fan)
	    DosErr("Could not open attached file\n%s in replies directory",
		   att->tempname);
	CurrentDir(ocd);
    } else {
	wd = RLock(workdirinst);
	if (!wd) {
	    DosErr("Unexpected failure to CD to\nwork directory %s", workdirinst);
	    return;
	}
	ocd = CurrentDir(wd);
	fan = OOpen(att->tempname);
	if (!fan)
	    DosErr("Could not open attached file\n%s in work directory",
		   att->tempname);
	CurrentDir(ocd);
	UnLock(wd);
    }
    if (!fan)
	return;
    if (DoFileRequest(true, true, fame, att->arrivename, null,
		      "Save Attached File As"))
	if (CopyFile(fan, fame))
	    DosErr("Could not copy file to\n%s", fame);
    Close(fan);
}


local char prtsavename[40];

local bool WriteOneMessageAsText(BPTR savhand, struct Mess *mm,
					bool separate, bool reply)
{
    char buf[260];
    struct Conf *cc;
    short i;

    if (mm->bits & ISBULLETIN) {
	if (FPrintf(savhand, "%s File: %-36s  Date: %s\n%s",
				(appendage ? "\n\n\n" : ""), mm->too, mm->date,
				(mm->subject ? "" : "\n")) < 0)
	    return false;
	if (mm->subject && FPrintf(savhand, "Subj.: %-45s   BBS: %s\n\n",
				mm->subject, packetname) < 0)
	    return false;
    } else {
	if (FPrintf(savhand, "%s From: %-36s  Date: %s%s\n",
				(appendage ? "\n\n" : ""), mm->from, mm->date,
				(mm->bluebits & UPL_PRIVATE
					? " (Priv)" : "")) < 0)
	    return false;
	if (!reply) {
	    if (mm->replyto && mm->replyat)
		sprintf(buf, "   To: %-36s  Msg#: %lu <- %lu -> %lu\n", mm->too,
				mm->replyto, mm->ixinbase, mm->replyat);
	    else if (mm->replyto)
		sprintf(buf, "   To: %-36s  Msg#: %lu <- %lu\n", mm->too,
				mm->replyto, mm->ixinbase);
	    else if (mm->replyat)
		sprintf(buf, "   To: %-36s  Msg#: %lu -> %lu\n", mm->too,
				mm->ixinbase, mm->replyat);
	    else
		sprintf(buf, "   To: %-36s  Msg#: %lu\n",
				mm->too, mm->ixinbase);
	} else
	    sprintf(buf, "   To: %s\n", mm->too);
	if (FPuts(savhand, buf))
	    return false;
	if (FPrintf(savhand, "Subj.: %s\n", mm->subject) < 0)
	    return false;
	cc = Confind(mm->confnum);
	if (FPrintf(savhand, " Area: %-45s   BBS: %s\n\n",
				(cc ? LONGNAME(cc) : "??"), packetname) < 0)
	    return false;
    }
    for (i = 0; i < mm->linect; i++) {
	size_t ll;
	if (mm->lines[i]) {
	    ll = mm->lines[i][-1];
	    strncpy(buf, mm->lines[i], ll);
	} else
	    ll = 0;
	strcpy(buf + ll, "\n");
	if (separate && i == mm->linect - 1)
	    strcpy(buf + ll + 1, (formfeed ? "\f" : "\n\n\n"));
	if (FPuts(savhand, buf))
	    return false;
    }
    return true;
}


bool SaveMessageAsText(struct Mess *mm, bool reply, BPTR prand,
			bool awto, bool wholearea)
{
    BPTR ocd, savhand;
    str sname = (prand ? &prtsavename[0] : &savename[0]);
    long der, i;
    bool suck;

    if (awto && !prand && !savename[0]) {
	Err("You must first manually select a filename for saving\n"
			"text.  Use \"ask filename\" subitem, or press V.");
	return false;
    }
    appendage = suck = false;
    ocd = CurrentDir(originalcd);
    if (!prand && !savename[0] && defsavename[0]) {
	strcpy(savename, defsavename);
	i = strlen(savename);
	if (savename[i - 1] != ':' && savename[i - 1] != '/')
	    if (savhand = RLock(savename)) {
		if (Examine(savhand, fib) && fib->fib_DirEntryType > 0)
		    savename[i] = '/', savename[i + 1] = 0;
		UnLock(savhand);
	    }
    }
    if ((!(mm->bits & LOADED) && !wholearea) || (!prand && !awto &&
				 !DoFileRequest(true, false, sname, null, null,
				   wholearea ? "Select file to save messages in"
				   : "Select file to save message in")))
	goto earlyfail;
    if (prand && !awto) {	/* prand is an open filehandle */
	savhand = prand;
	appendage = false;
	sname = printerpath;	/* for screen title message */
    } else {
	if (awto)
	    if (prand)		/* prand is a dummy nonzero value */
		sprintf(sname, "RAM:Q-Blue-temp-%lx", me);
	    else
		appendage = true;
	if (appendage) {
	    if (!(savhand = OOpen(sname)) || SSeek(savhand, 0, OFFSET_END) < 0
				|| (awto && !prand &&
				    SSeek(savhand, 0, OFFSET_CURRENT) <= 0)) {
		if (savhand)
		    Close(savhand);
		Err("Can't append to file %s;\n%s failed.", savename,
					(savhand ? "Seek" : "Open"));
		goto earlyfail;
	    }
	} else if (mm->bits & ISBULLETIN && !prand && !wholearea) {
	    if (CopyFile((BPTR) mm->from, sname))	/* make BINARY copy! */
		DosErr("Could not copy bulletin to\nfile %s", sname);
	    goto earlyfail;	/* leave suck false! */
	} else if (!(savhand = NOpen(sname))) {
	    DosErr("Can't create output file\n%s", sname);
	    goto earlyfail;
	}
	appendage = !prand && SSeek(savhand, 0, OFFSET_CURRENT) > 0;
    }
    PortOff();
    if (wholearea) {
	struct Conf *wc = readareaz.confs[whicha];
	bool unloaded;
	ushort i;
	suck = true;
	for (i = 0; i < wc->messct; i++) {
	    mm = wc->messes[i];
	    if (unloaded = !(mm->bits & LOADED))
		if (!LoadPktMessage(mm)) {
		    suck = false;
		    break;
		}
	    if (!WriteOneMessageAsText(savhand, mm, false, reply)) {
		suck = false;
		break;
	    }
	    if (unloaded)
		StripMessage(mm);
	    appendage = true;
	}
    } else if (WriteOneMessageAsText(savhand, mm, !!prand, reply))
	suck = true;
    der = IoErr();
    if (!Close(savhand))
	der = IoErr(), suck = false;
    PortOn();
    if (!suck)
	DosErr("Failure writing the message to the file.", null);
    else if (awto ? !prand : prand == savhand) {
	UndoTitleBarMsg(true);
	legit_title = bgwin->ScreenTitle;
	sprintjam(bogus_title, "Appended to \"%s%s\"  ", sname);
	SetAllSharedWindowTitles(bogus_title);
	GetSysTime(&legit_time);
	legit_time.tv_secs += RESTORETIME;
    }
  earlyfail:
    if (suck)
	OnMenu(bgwin, FULLMENUNUM(1, 5, 1));		/* append prev. */
    else
	OffMenu(bgwin, FULLMENUNUM(1, 5, 1));
    CurrentDir(ocd);
    return suck;
}


void PrintMessage(struct Mess *mm, bool reply)
{
    BPTR prand = 0, plok;
    char cmdbuf[129];
    APTR oldwptr, oldcont;
    long err;

    if (!printerpath[0]) {
	Err("No path is defined for printer output\n"
				"in the Directories setup window.");
	return;
    }
    if (plok = RLock(printerpath)) {          /* only succeeds for disk files */
	if (!(prand = SafeOpenFromLock(plok))) {
	    UnLock(plok);
	    Err("Your printing output file seems\n"
				"to be busy, or not really a file.");
	    return;
	}
    } else {
	err = IoErr();
	if (err == ERROR_DEVICE_NOT_MOUNTED)    /* "please insert" cancelled */
	    return;
	else if (err == ERROR_OBJECT_NOT_FOUND) {
	    if (!(prand = NOpen(printerpath))) {	/* try to create */
		DosErr("Cannot open or create\nyour printing output file.");
		return;
	    }
	} else if (err != ERROR_ACTION_NOT_KNOWN) {	/* not non-FS device */
	    Err("Your printing output device\nseems to be invalid or busy.");
	    return;
	}
    }
    if (prand && SSeek(prand, 0L, OFFSET_END) >= 0) {
	SaveMessageAsText(mm, reply, prand, false, false);  /* closes prand */
	return;
    }
    if (prand)
	Close(prand);
    if (!SaveMessageAsText(mm, reply, -1L, true, false))    /* dummy prand */
	return;
    sprintf(cmdbuf, "run Failat 99+\nCOPY %s \"%s\"+\nDelete %s",
				prtsavename, printerpath, prtsavename);
    oldwptr = me->pr_WindowPtr;
    oldcont = me->pr_ConsoleTask;
    me->pr_WindowPtr = (APTR) -1;
    prand = OOpen("NIL:");
    me->pr_WindowPtr = oldwptr;
    if (ShellExecute(cmdbuf, 0L, prand) >= 0) {
	UndoTitleBarMsg(true);
	legit_title = bgwin->ScreenTitle;
	sprintjam(bogus_title, "Copying to \"%s%s\"  ", printerpath);
	SetAllSharedWindowTitles(bogus_title);
	GetSysTime(&legit_time);
	legit_time.tv_secs += RESTORETIME;
    }
    Close(prand);
    me->pr_ConsoleTask = oldcont;
}


void NewSort(short nso)
{
    struct MenuItem *mi;
    short i;

    if (cwin) return;
    sorder = nso;
    if (nso == 5)		/* 0, 1, 2, 5, 3, 4 */
	nso = 3;
    else if (nso >= 3)
	nso++;
    for (i = 0, mi = &misort0; mi && i <= 5; mi = mi->NextItem, i++)
	if (i == nso)
	    mi->Flags |= CHECKED;
	else
	    mi->Flags &= ~CHECKED;
    Resort(true);
}


#define ctl(c) (c & 0x1F)

void DoSetupIDCMP(char k)
{
    short i;
    struct MenuItem *mi;

    if (k & 0x80)
	switch (k & 0x7f) {
	  case 20:				/* Packer select */
	    for (i = 0; i < packers; i++)
		if (passum[i].Flags & CHECKED) {
		    currentpacker = i;
		    break;
		}
	    break;
	  case 0:				/* Sort order */
	    if (cwin)	/* disable while composing */
		break;
	    for (i = 0, mi = &misort0; i <= 5; i++, mi = mi->NextItem)
		if (mi->Flags & CHECKED) {
		    sorder = (i >= 3 ? (i == 3 ? 5 : i - 1) : i);
		    /* 0, 1, 2, 5, 3, 4 -- backward compatibility */
		    break;
		}
	    lastfirst = !!(misortlast.Flags & CHECKED);
	    Resort(true);
	    break;
	  case 1:				/* Directories */
	    ConfigDirs();
	    break;
	  case 2:				/* Editor */
	    ConfigEditor();
	    break;
	  case 3:				/* Replying */
	    ConfigReplying();
	    break;
	  case 4:				/* Compressors */
	    ConfigCompressors();
	    break;
	  case 5:				/* Font & screen */
	    ConfigFontScreen();
	    break;
#ifdef FUNCTION_KEYS
	  case 6:				/* Function keys */
	    ConfigFKeys();
	    break;
#  define FK_OFF 1
#else
#  define FK_OFF 0
#endif
	  case 6+FK_OFF:			/* Options */
	    ConfigOptions();
	    break;
	  case 7+FK_OFF:			/* BBS Local */
	    ConfigBLocal();
	    break;
	  case 9+FK_OFF:			/* Load config */
	    FiddleFigFile(false);
	    break;
	  case 10+FK_OFF:			/* Save config */
	    FiddleFigFile(true);
	    break;
	}
    else
	switch (k) {
	  case '1': case '2': case '3': case '4':
	  case '5': case '6': case '7': case '8':
	    i = k - '1';
	    if (i >= packers) break;
	    passum[currentpacker].Flags &= ~CHECKED;
	    currentpacker = i;
	    passum[i].Flags |= CHECKED;
	    break;
	  case 'O':
	    ConfigOptions();
	    break;
	  case 'Y':
	    ConfigReplying();
	    break;
	  case 'F':
	    ConfigFontScreen();
	    break;
	  case 'L':
	  case 'S':
	    FiddleFigFile(k == 'S');
	    break;
	  case 'C':
	    ConfigCompressors();
	    break;
	  case 'E':
	    ConfigEditor();
	    break;
	  case 'D':
	    ConfigDirs();
	    break;
#ifdef FUNCTION_KEYS
	  case 'K':
	    ConfigFKeys();
	    break;
#endif
	  case 'B':
	    ConfigBLocal();
	    break;
	  case ctl('N'):
	    NewSort(0);
	    break;
	  case ctl('A'):
	    NewSort(1);
	    break;
	  case ctl('S'):
	    NewSort(2);
	    break;
	  case ctl('F'):
	    NewSort(3);
	    break;
	  case ctl('T'):
	    NewSort(4);
	    break;
	  case ctl('D'):
	    NewSort(5);
	    break;
	  case ctl('L'):
	    if (cwin)
		break;
	    if (lastfirst = !lastfirst)
		misortlast.Flags |= CHECKED;
	    else
		misortlast.Flags &= ~CHECKED;
	    if (sorder == 3 || sorder == 4)
		Resort(true);
	    break;
	}
}


void ReplyFlip(struct Mess *mm, bool isreply)
{
    struct Conf *cc, *ct;
    short a, m, zz = readareaz.messct - 1;
    struct Mess *mre = mm->mreplyee;

    if (!mre) return;
    if (isreply) {
	m = 0;
	cc = Confind(mre->confnum);
	if (mre->personalink) {
	    ushort mine = mre->bits, hers = mre->personalink->bits;
	    if (!(mine & MESEEN) && hers & MESEEN)
		mre = mre->personalink, cc = &personals;	/* unlikely */
	    else if (!(mine & LASTREALLYREAD) && (hers & LASTREALLYREAD
						  || !(mine & PERSONALCOPY)))
		mre = mre->personalink, cc = &personals;
	}	/* we default to the personal copy if neither is marked */
    } else
	cc = &replies, m = zz;
    if (cc)
	for (a = m; a <= zz; a++) {
	    ct = readareaz.confs[a];
	    if (ct == cc)
		for (m = 0; m < ct->messct; m++)
		    if (ct->messes[m] == mre) {
			whicha = a;
			ct->current = m;
			AdjustToNewArea(true);
			return;
		    }
	}
    Err("The message that this is a reply to\n"
			"has been excluded by a word search.");
}


void GoPrev(struct Conf *cc)
{
    if (whichm > 0)
	ShowNewMessage(cc, cc->messes[--whichm]);
    else if (whicha > 0) {
	whichm = readareaz.confs[--whicha]->current;
	PickArea(false);
    } else
	PickArea(true);
}


void GoNext(struct Conf *cc)
{
    if (whichm < cc->messct - 1)
	ShowNewMessage(cc, cc->messes[++whichm]);
    else if (whicha < readareaz.messct - 1) {
	whichm = readareaz.confs[++whicha]->current;
	PickArea(false);
    } else
	PickArea(true);
}


void GoToNextUnread(struct Conf *cc)
{
    ushort m, a;
    for (m = whichm + 1; m < cc->messct; m++)
	if (!(cc->messes[m]->bits & MESEEN)) {
	    ShowNewMessage(cc, cc->messes[whichm = m]);
	    return;
	}
    for (a = whicha + 1; a != whicha; a >= readareaz.messct - 1 ? a = 0 : a++) {
	cc = a >= readareaz.messct ? &bullstuff : readareaz.confs[a];
	if (cc != &bullstuff && cc != &replies)
	    for (m = 0; m < cc->messct; m++)
		if (!(cc->messes[m]->bits & MESEEN)) {
		    whicha = a;
		    cc->current = m;
		    AdjustToNewArea(false);
		    return;
		}
    }
    cc = readareaz.confs[whicha];
    for (m = 0; m <= whichm; m++)
	if (!(cc->messes[m]->bits & MESEEN)) {
	    ShowNewMessage(cc, cc->messes[whichm = m]);
	    return;
	}
    Err("All messages have been read.");
}


local void CountRead(void)
{
    ushort i, j;
    totaltoread = totalunread = 0;
    for (i = 0; i < readareaz.messct; i++) {
	struct Conf *cc = readareaz.confs[i];
	if (cc != &replies && cc != &personals && cc != &bullstuff) {
	    if (cc->unfiltered)
		cc = cc->unfiltered;
	    totaltoread += cc->messct;
	    for (j = 0; j < cc->messct; j++) {
		struct Mess *mm = cc->messes[j];
		if (!(mm->bits & (MESEEN | MEREPLIED)) && (!mm->personalink ||
					!(mm->personalink->bits & MESEEN)))
		    totalunread++;
	    } 
	}
    }
}


local bool DoArchiveDeletion(bool quitting, bool *cancelflag)
{
    if (!darchivename[0] || !AskDeleteArchive(quitting, cancelflag))
	return false;
    if (DeleteFile(darchivename)) {
	strcat(darchivename, ".info");
	DeleteFile(darchivename);
	darchivename[0] = 0;
	return true;
    } else {
	DosErr("Deletion of %s failed.", darchivename);
	return false;
    }
}


local void DoReplyDeletion(void)
{
    char name[30], rname[30];
    BPTR lok, ocd, cd;
    if (anythingtosave || (fakery && !replies.messct))
	return;		/* ideally should remember if door config had existed */
    if (!(cd = RLock(uploaddir)))
	return;
    ocd = CurrentDir(cd);
    sprintf(name, "%s.%s", packetname, qwk ? "REP" : "NEW");
    if (!(lok = RLock(name))) {
	UnLock(CurrentDir(ocd));
	return;
    }
    if (AskNonReplyDelete(name)) {
	strcpy(rname, name);
	strcat(rname, ".old");
	DeleteFile(rname);
	if (!Rename(name, rname))
	    DosErr("Renaming of %s failed.", name);
    }
    UnLock(lok);
    UnLock(CurrentDir(ocd));
}


bool OneOfOurNotes(str fn)
{
    short i = strlen(fn), j;
    if (i <= 7)
	return !i;
    i -= 7;
    if (strcmp(fn + i, "% read.") || !isdigit(fn[0]))
	return false;
    fn[i] = 0;
    j = atoi(fn);
    fn[i] = '%';
    if (j >= 0 && j <= 100) {
	while (--i > 0 && isdigit(fn[i])) ;
	if (!i) {
	    oldcentread = j;
	    return true;
	}
    }
    return false;
}


bool StillTheRightSize(void)
{
    BPTR lk;
    bool ret;
    if (fakery || !darchivename[0] || !(lk = RLock(darchivename)))
	return false;
    if (Examine(lk, fib)) {
	strcpy(thefilenote, fib->fib_Comment);
	ret = oldcentsize && oldcentsize == fib->fib_Size
				&& oldcenttime == DS2ux(&fib->fib_Date);
    } else {
	thefilenote[0] = 0;
	ret = false;
    }
    UnLock(lk);
    return ret;
}


local void DoArchiveNotation(void)	/* call after StillTheRightSize() */
{
    char com[80];
    short i;

    if ( /* !totaltoread || */ !darchivename[0])
	return;
    i = totaltoread ? (totaltoread - totalunread) * 100 / totaltoread : 100;
    if (OneOfOurNotes(thefilenote) && i >= oldcentread) {
	sprintf(com, "%ld%% read.", oldcentread = i);
	SetComment(darchivename, com);
    }
}


bool Quittify(void)
{
    bool spancel = true, gorg;
    if (gorg = repchanges && anythingtosave && AskPackReplies(&spancel))
	quittitude = PackReplies() && AskReallyQuit() && FreeTaglines();
    else
	quittitude = spancel && AskReallyQuit() && FreeTaglines();
    if (quittitude) {
	CountRead();
	if (StillTheRightSize()) {
	    if (!quitleaveopen && askdelete && !fakery
				&& DoArchiveDeletion(true, &quittitude))
		thefilenote[0] = 0;
	    DoArchiveNotation();
	}
	if (!quitleaveopen && askdelete)
	    DoReplyDeletion();
    } else if (gorg)
	SaveBookmarks();	/* they are also saved if we return true */
    return quittitude;
}


bool GetSomeClosure(void)
{
    bool spancel = true, gorg;
    if (gorg = repchanges && anythingtosave && AskPackReplies(&spancel))
	spancel = PackReplies();
    if (spancel) {
	CountRead();
	if (StillTheRightSize()) {
	    if (askdelete && !fakery && DoArchiveDeletion(false, &spancel))
		thefilenote[0] = 0;
	    DoArchiveNotation();
	}
	if (askdelete)
	    DoReplyDeletion();
    } else if (gorg)
	SaveBookmarks();	/* they are also saved if we return true */
    return spancel;
}


void MarkUnread(struct Mess *mm)
{
    struct Conf *cc = Confind(mm->confnum);
    mm->bits &= ~MESEEN;
    if (cc)
	cc->morebits &= ~ALLREAD;
    if (mm->personalink) {
	mm->personalink->bits &= ~MESEEN;
	personals.morebits &= ~ALLREAD;
    }
}


local bool DoReadIDCMP(struct IntuiMessage *im)
{
    register char k;
    struct Conf *cc = readareaz.confs[whicha];
    struct Mess *mm = cc->messes[whichm];
    ushort qual = im->Qualifier;

    UndoTitleBarMsg(false);
#ifdef DELAY_MARK_AS_READ
    /* ***** Don't set MESEEN flag unless on screen for long enough */
    if (!(mm->bits & MESEEN)) {		/******  DOES onscreen == mm always?? */
	ushort i, j;
	tentativity = mm;
	mm->bits |= MESEEN;
	GetSysTime(&view_time);
	for (j = i = 0; i < mm->linect && j <= TENTSHORTLINES; i++)
	    if (mm->lines[i]) {
		k = mm->linetypes[i] & TYPE_MASK;
		if (k != TRASHTYPE && k != TRASHWRAPTYPE)
		    j++;
	    }
	if (j <= TENTSHORTLINES)
	    AddTime(&view_time, TIMETILVALID_S);
	else
	    AddTime(&view_time, TIMETILVALID_L);
    } else if (tentativity != mm)
	tentativity = null;
    /* =*=*=*=  Support LASTREALLYREAD here as below: */
#else
    if (mm != lastseen) {
	mm->bits |= MESEEN;
	if (mm->personalink && cc != &replies) {
	    mm->bits |= LASTREALLYREAD;
	    mm->personalink->bits &= ~LASTREALLYREAD;
	    /* if (!(mm->bits & PERSONALCOPY)) */ mm->personalink->bits |= MESEEN;
	}
    }
    lastseen = mm;
#endif
    switch (im->Class) {
      case IDCMP_RAWKEY: {
	k = KeyToAscii(im->Code, qual);
#ifdef FUNCTION_KEYS
	if (DoFunctionKeyCommand(k))
	    return false;
#endif
	if (k == (HELPKEY | 0x80))
	    k = 'H';
	else if (k == '\r')				/* return == space */
	    k = ' ';
	else if (k == '\b') {				/* bs = alt-space */
	    k = ' ';
	    qual |= IEQUALIFIER_LALT;
	} else if (im->Code == 0x4f || im->Code == 0x2d)
	    GoPrev(cc);					/* leftarrow, kp4 */
	else if (im->Code == 0x4e || im->Code == 0x2f)
	    GoNext(cc);					/* rightarrow, kp6 */
	if (qual & ALTCTLKEYS)
	    switch (k) {
	      case ' ':
		if (topline)
		    ScrollUpPage(mm);
		else
		    GoPrev(cc);
		break;
	      case 'M':
		ConfigBWDoor();
		break;
	      case 'P':
		PrintMessage(mm, cc == &replies);
		break;
	      case 'R':
		PickDLRequests();
		break;
	      case 'U':
		UnSearch(&readareaz, true);
		break;
	      case 'V':
		/* if (misave1.Flags & ITEMENABLED) */
		SaveMessageAsText(onscreen, cc == &replies, 0L, true, false);
		break;
	      case ctl('V'):
		SaveMessageAsText(onscreen, cc == &replies, 0L, false, true);
		break;
	      case 'T':
	        SaveAttachment(onscreen, cc == &replies);
		break;
	      case 'W':
		if (!fakery || cc != &bullstuff)
		    ComposeMsg(null, cc == &replies, false, mm);
		break;
	      case '?':
	      case '/':
		Aboutness();
		break;
	      default:
		DoSetupIDCMP(k);
	    }
	else
	    switch (k) {
	      case 'U':
		UnSearch(cc, true);
		break;
	      case 'H':
		/* do context sensitive help... haha */
		break;
	      case '[':
		if (whicha > 0)
		    whicha--;
		AdjustToNewArea(true);
		break;
	      case ']':
		if (whicha < readareaz.messct - 1)
		    whicha++;
		AdjustToNewArea(true);
		break;
	      case 'P':
		SaveBookmarks();
		PackReplies();
		break;
	      case 'R':
		if (cc != &bullstuff)
		    ComposeMsg(mm, cc == &replies, false, null);
		break;
	      case 'E':
		if (cc != &replies)
		    ComposeMsg(mm, false, true, null);
		break;
	      case 'W':
		ComposeMsg(null, false, false, null);
		break;
	      case '@':
		if (mirep4a.Flags & ITEMENABLED)
		    ComposeMsg(null, true, true, null);
		break;
	      case 'L':
		DoList(cc);
		break;
	      case 'A':
		PickArea(true);
		break;
	      case 'F':
		ReplyFlip(onscreen, cc == &replies);
		break;
	      case 127: /* Del */
	      case 'D':
		ToggleDeleted(cc);
		break;
	      case 'S':
		Soich(normal);
		break;
	      case 'V':
		SaveMessageAsText(onscreen, cc == &replies, 0L, false, false);
		break;
	      case ' ':
		if (topline <= onscreen->linect + !!ATTACHED(onscreen) - winlines)
		    ScrollDownPage(onscreen);
		else
		    GoNext(cc);
		break;
	      case 'I':
		Iconify(false);
		break;
	      case 'B':
		ShowSysopEtc();
		break;
	      case 'T':
		TaglineWindow(false, true);
		break;
	      case 'X':
		MarkUnread(mm);
		break;
	      case 'N':
		GoToNextUnread(cc);
		break;
	      case 'C':
		return GetSomeClosure();
	      case 'Q':
		return Quittify();
	    }
	}
	break;
      case IDCMP_GADGETUP:
	switch (((struct Gadget *) im->IAddress)->GadgetID) {
	  case 0:					/* Close / Pack */
	    if (readgag0.GadgetText == &bgt0p) {
		SaveBookmarks();
		PackReplies();
		return false;
	    } else
		return GetSomeClosure();
	  case 1:					/* Area */
	    PickArea(true);
	    break;
	  case 2:					/* Write */
	    ComposeMsg(null, false, false, null);
	    break;
	  case 3:					/* Reply / Re-edit */
	    ComposeMsg(mm, cc == &replies, false, null);
	    break;
	  case 4:					/* List */
	    DoList(cc);
	    break;
	  case 5:					/* Prev. */
	    GoPrev(cc);
	    break;
	  case 6:					/* Next */
	    GoNext(cc);
	    break;
	}
	break;
      case IDCMP_MENUPICK:
	if (MENUNUM(im->Code) == 3)
	    DoSetupIDCMP((char) (ITEMNUM(im->Code) | 0x80));
	else switch (20 * MENUNUM(im->Code) + ITEMNUM(im->Code)) {
	  case 2:					/* Pack replies */
	    SaveBookmarks();
	    PackReplies();
	    break;
	  case 3:					/* Close packet */
	    return GetSomeClosure();
	  case 4:
	    DoSetupIDCMP((char) 20 | 0x80);		/* Compression type */
	    break;
	  case 5:					/* BBS information */
	    ShowSysopEtc();
	    break;
	  case 6:					/* Iconify screen */
	    Iconify(false);
	    break;
	  case 7:					/* About Q-Blue... */
	    Aboutness();
	    break;
	  case 9:					/* Quit */
	    return Quittify();
	  case 20:					/* Next area */
	    if (whicha < readareaz.messct - 1)
		whicha++;
	    AdjustToNewArea(true);
	    break;
	  case 21:					/* Previous area */
	    if (whicha > 0)
		whicha--;
	    AdjustToNewArea(true);
	    break;
	  case 22:					/* Search */
	    Soich(normal);
	    break;
	  case 23:					/* Undo search */
	    UnSearch(SUBNUM(im->Code) ? &readareaz : cc, true);
	    break;
	  case 24:					/* Print */
	    PrintMessage(mm, cc == &replies);
	    break;
	  case 25:					/* Save as text */
	    if (SUBNUM(im->Code) == 3)
		SaveAttachment(mm, cc == &replies);
	    else
		SaveMessageAsText(mm, cc == &replies, 0L,
				  SUBNUM(im->Code) == 1, SUBNUM(im->Code) == 2);
	    break;
	  case 26:					/* Next unread */
	    GoToNextUnread(cc);
	    break;
	  case 27:					/* Mark as unread */
	    MarkUnread(mm);
	    break;
//	  case 28:					/* Go back */
//	    /* whatever... */
//	    break;
	  case 28:					/* Areas */
	    PickArea(true);
	    break;
	  case 29:					/* List */
	    DoList(cc);
	    break;
	  case 40:					/* Flip */
	    ReplyFlip(mm, cc == &replies);
	    break;
	  case 41:					/* Delete / Undelete */
	    ToggleDeleted(cc);
	    break;
	  case 42:					/* Reply / Re-edit */
	    ComposeMsg(mm, cc == &replies, false, null);
	    break;
	  case 43:					/* Reply to addressee */
	    if (cc != &replies)
		ComposeMsg(mm, false, true, null);
	    break;
	  case 44:					/* Write */
	    ComposeMsg(null, false, false, null);
	    break;
	  case 45:
	    ComposeMsg(null, true, true, null);		/* Write email */
	    break;
	  case 46:					/* Carbon copy */
	    if (!fakery || cc != &bullstuff)
		ComposeMsg(null, cc == &replies, false, mm);
	    break;
	  case 47:					/* Request D/L... */
	    PickDLRequests();
	    break;
	  case 48:					/* Mail door... */
	    ConfigBWDoor();
	    break;
	  case 49:
	    TaglineWindow(false, true);			/* Maintain taglines */
	    break;
	}
	break;
      case IDCMP_INTUITICKS:
	if (waste)
	    PreloadConf(cc, whichm);
	break;
    }
    return false;
}


void ViewMessages(ushort initialarea, bool areafirst)
{
    lastseen = null;
    SetMessGags(true);
    if ((whicha = initialarea) >= readareaz.messct)
	whicha = readareaz.messct - 1;
    whichm = readareaz.confs[whicha]->current;	/* in case list cancelled */
    if (areafirst)
	PickArea(true);
    else
	AdjustToNewArea(false);
    curconf = readareaz.confs[whicha];
    if (!areafirst)
	FlipBGadgets(curconf == &bullstuff ? 0x77 : 0x7f);
    EventLoop(&DoReadIDCMP);
    PortOff();			/* !!!  CALLER MUST DO PortOn()! */
    UndoTitleBarMsg(true);
    if (popupscroller && !backslide)
	NukeBGslider();
    SetMessGags(false);
}
