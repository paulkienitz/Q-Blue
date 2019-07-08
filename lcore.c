/* core fundamentals used by list.c and maybe tagline.c, */
/* and the new area adding window. */

#include <intuition/intuitionbase.h>
#include <intuition/sghooks.h>
#include <dos/dos.h>
#include "qblue.h"
#include "pigment.h"


#define ARROWLEFT      (-16)
#define WSHINCOLOR     1
#define WSHADCOLOR     2

#define INCREMENT      20

#define SCROLLWAIT     500000

/* keep synchronized with list.c: */
#define NORMCOLOR   7

bool Soich(whatsearch whats);
void UnSearch(struct Conf *where, bool freshen);
void ToggleInUseify(void);
bool Do1ListAreaIDCMP(struct IntuiMessage *im);
bool Do1TaglineIDCMP(struct IntuiMessage *im);
void RealizeTagline(void);
bool Irped(void);
void CreateBBSFile(bool artificial);
void PreloadConf(struct Conf *cc, ushort which);

void SetAlarm(ulong micros);
struct Conf *NewPoolConf(void);
ustr NewPoolString(ushort length);
void FixDefarrow(void);
bool ScrollUpLine(struct Mess *mm, short xtra);
bool ScrollDownLine(struct Mess *mm, short xtra);
void ContinueClipping(void);

void CloseKeyFilWin(void);
void CloseBitsWin(void);
void PaintListWin(struct Conf *cc);
void PaintTaglineWin(void);
void LeaveListArea(struct Conf *cc);


import ListLineFType ListLine;

import struct IntuitionBase *IntuitionBase;
import struct Window *bkfwin, *bwdwin, *llwin;
import struct RastPort *larp;
import struct ExtNewWindow lneww;
import struct Gadget tgagstr, *lastbgbutton;

import struct Conf *listcc, tagfaconf;
import short bignums[], tagbignums[];
import str lasttagrealized;

import short linez, topmess, pick;
import ushort lepix, topix, pxswide, deferline, areazroom, winlines;
import ushort tifight;
import bool packeting, airing, emptyairing, addropping, bgupdate;
import bool deferrable, unsearchulated, anyunsearched, alswitch;
import bool inuseified, newareawin, tagging, buttoning, rupted, taintag;


struct timeval lastclick;

short lastmouseX, lastmouseY;
local long lastlistpot;

local struct RastPort larpspare;

bool poseclosed, bgustifle = false;


#ifdef GOOL

import struct TextAttr ta;

struct List listlines;

struct line {
    struct Node nod;
    char lin[80];
};


struct NewGadget listng = {
    4, 12, LWINWIDTH - 8, LWINHEIGHT - 14, null, &ta, 209, 0, null, null
};

#else /* !GOOL */

local struct Image manualautoimage = {
    0 /*, 0, 0, 0, 3, null, 7, 0, null */
};

struct PropInfo lprops = {
    AUTOKNOB | FREEVERT | PROPNEWLOOK, 0, 0, MAXBODY, MAXBODY, 0, 0, 0, 0, 0, 0
};

struct Gadget lscroll = {
    null, -13, 12, 10, -40,
    GFLG_GADGHNONE | GFLG_RELRIGHT | GFLG_RELHEIGHT | GFLG_GADGIMAGE,
    GACT_RELVERIFY | GACT_FOLLOWMOUSE | GACT_RIGHTBORDER, GTYP_PROPGADGET,
    &manualautoimage, null, null, 0, &lprops, 202, null
};


local short larboxdotsshad[6] = { 17, 0, 17, 9, 1, 9 };

local short larboxdots[6] = { 0, 9, 0, 0, 16, 0 };

local short lupdots[6] = { 5, 6, 8, 3, 11, 6 };

local short ldowndots[6] = { 5, 3, 8, 6, 11, 3 };

local struct Border laltarrowboxshad = {
    0, 0, WSHINCOLOR, 0, JAM2, 3, larboxdotsshad, null
};

local struct Border laltarrowbox = {
    0, 0, WSHADCOLOR, 0, JAM2, 3, larboxdots, &laltarrowboxshad
};

local struct Border laltuppoint = {
    0, 0, 0, 0, JAM2, 3, lupdots, &laltarrowbox
};

local struct Border laltdownpoint = {
    0, 0, 0, 0, JAM2, 3, ldowndots, &laltarrowbox
};

local struct Border larrowboxshad = {
    0, 0, WSHADCOLOR, 0, JAM2, 3, larboxdotsshad, null
};

local struct Border larrowbox = {
    0, 0, WSHINCOLOR, 0, JAM2, 3, larboxdots, &larrowboxshad
};

struct Border luppoint = {
    0, 0, WSHADCOLOR, 0, JAM2, 3, lupdots, &larrowbox
};

struct Border ldownpoint = {
    0, 0, WSHADCOLOR, 0, JAM2, 3, ldowndots, &larrowbox
};


struct Gadget luparrow = {
    &lscroll, -17, -29, 18, 10,
    GFLG_GADGHIMAGE | GFLG_RELRIGHT | GFLG_RELBOTTOM,
    GACT_IMMEDIATE | GACT_RELVERIFY | GACT_RIGHTBORDER, GTYP_BOOLGADGET,
    &luppoint, &laltuppoint, null, 0, null, 201, null
};


struct Gadget ldownarrow = {
    &luparrow, -17, -19, 18, 10,
    GFLG_GADGHIMAGE | GFLG_RELRIGHT | GFLG_RELBOTTOM,
    GACT_IMMEDIATE | GACT_RELVERIFY | GACT_RIGHTBORDER, GTYP_BOOLGADGET,
    &ldownpoint, &laltdownpoint, null, 0, null, 200, null
};

#endif GOOL


struct Gadget *lastbutton;


struct IntuiText nagtcancel = {
    TEXTCOLOR, FILLCOLOR, JAM2, 12, 2, null, "Cancel", null
};

struct Gadget nagagcancel = {
    null, 254, 68, 80, 12, GFLG_GADGHCOMP,
    GACT_RELVERIFY, GTYP_BOOLGADGET,
    &upborder, null /* &downborder */, &nagtcancel, 0, null, 1103, null
};

struct IntuiText nagtokay = {
    TEXTCOLOR, FILLCOLOR, JAM2, 12, 2, null, "Okay", null
};

struct Gadget nagagokay = {
    &nagagcancel, 128, 68, 80, 12, GFLG_GADGHCOMP,
    GACT_RELVERIFY, GTYP_BOOLGADGET,
    &upborder, null /* &downborder */, &nagtokay, 0, null, 1102, null
};

ubyte nanumber[9], naname[LONCOLEN + 2];

struct StringInfo nastrnum = {
    nanumber, null, 0, 8, 0, 0, 0, 0, 0, 0, &stringex, 0, null
};

struct IntuiText nalabelnum = {
    LABELCOLOR, 0, JAM1, -268, 0, null,
    "Area number as known to the BBS:", null
};

STRINGBORDER(naboxnum)

struct Gadget nagagnum = {
    &nagagokay, 280, 24, 64, 8, GFLG_STRINGEXTEND | GFLG_TABCYCLE,
    GACT_RELVERIFY | GACT_STRINGLEFT | GACT_LONGINT, GTYP_STRGADGET,
    &naboxnum, null, &nalabelnum, 0, &nastrnum, 1101, null
};

struct StringInfo nastrname = {
    naname, null, 0, LONCOLEN, 0, 0, 0, 0, 0, 0, &stringex, 0, null
};

struct IntuiText nalabelname = {
    LABELCOLOR, 0, JAM1, -92, 0, null, "Area name:", null
};

STRINGBORDER(naboxname)

struct Gadget nagagname = {
    &nagagnum, 104, 46, 240, 8, GFLG_STRINGEXTEND | GFLG_TABCYCLE,
    GACT_RELVERIFY | GACT_STRINGLEFT, GTYP_STRGADGET,
    &naboxname, null, &nalabelname, 0, &nastrname, 1100, null
};

struct ExtNewWindow naneww = {
    138, 68, 364, 94, 0, 1, IDCMP_GADGETUP | IDCMP_CLOSEWINDOW | IDCMP_RAWKEY,
    WFLG_NOCAREREFRESH | WFLG_CLOSEGADGET | WFLG_DRAGBAR | WFLG_DEPTHGADGET
		| WFLG_ACTIVATE | WFLG_NW_EXTENDED,
    null, null, "Describe the message area to add:", null, null,
    0, 0, 0, 0, CUSTOMSCREEN, null
};

struct WhereWin naww = { &naneww };

struct Window *nawin;


char toggletitle[] = "Select areas to add or drop from downloads";
char listitle[80];


/* Version of ScrollRaster that only scrolls vertically, does not RectFill   */
/* vacated area, and splits large vertical areas into stripes so as to avoid */
/* the color flickering that can happen with a slow blitter and 3 bitplanes. */

void MyScrollRaster(struct RastPort *rp, /* short dx, */ short dy,
			short xmin, short ymin, short xmax, short ymax)
{
    short symin = ymin, dymin = ymin, sym, dym;
    short totalh = 1 + ymax - ymin, stripeh, thish, stripes, toplast;
    long xsize = 1 + xmax - xmin;

    /* dx is always zero */
    if (!dy) return;
    if (dy > 0)				/* positive means upward scroll */
	symin += dy, totalh -= dy;
    else
	dymin -= dy, totalh += dy;
    stripes = 1 + totalh / 100;		/* 100 is a rough heuristic value */
    stripeh = 1 + totalh / stripes;     /* -- enough for A1000 but not UAE */
    stripeh += fight - stripeh % fight;	 /* stripes split along text lines */
    toplast = totalh - stripeh;
    if (toplast < 0)
	toplast = 0;
    if (dy < 0)
	sym = symin + toplast, dym = dymin + toplast;
    else
	sym = symin, dym = dymin;
    while (totalh > 0) {
	thish = min(stripeh, totalh);
	ClipBlit(rp, xmin, sym, rp, xmin, dym, xsize, thish, 0xC0);
	/* if used on non-smartrefresh window, do masked-off ScrollRaster too */
	totalh -= thish;
	if (dy > 0)
	    sym += thish, dym += thish;
	else if (totalh >= thish)
	    sym -= thish, dym -= thish;
	else
	    sym = symin, dym = dymin;
    }
}


void Titlist(struct Conf *cc)
{
    str basic, pre, eodc = " -- Enter or double-click";

    pre = cc->unfiltered ? "[Searched] " : "";
    if (addropping)
	basic = toggletitle, eodc = "";
    else if (tagging) {
	if (taintag)
	    basic = "Add, edit, or delete taglines", eodc = "";
	else
	    basic = "Select a tagline";
    } else if (packeting)
	basic = fakery ? "Select BBS file to open" : "Select file to unpack";
    else if (airing)
	basic = "Select message area";
    else
	basic = LONGNAME(cc), eodc = "";
    sprintf(listitle, "%s%s%s", pre, basic, eodc);
    if (lawin)
	SetWindowTitles(lawin, listitle, (adr) -1);
}


/* The idea of this function is that we create a simulated SetAPen/SetBPen */
/* for list windows which optimizes the number of actual Set?Pen calls by  */
/* (1) doing nothing when there's no change, and (2) reserving an extra    */
/* rastport just for the most frequently used color combination.           */

void SetLPens(short fg, short bg)	/* pass -1 to leave a color unchanged */
{
    if ((fg < 0 || fg == larp->FgPen) && (bg < 0 || bg == larp->BgPen))
	return;
    if ((fg < 0 ? larp->FgPen : fg) == larpspare.FgPen &&
			      (bg < 0 ? larp->BgPen : bg) == larpspare.BgPen) {
	ASSERT(larp != &larpspare);  /* or the first if would have returned */
	Move(&larpspare, larp->cp_x, larp->cp_y);
	larp = &larpspare;		/* most frequently used combination */
	return;
    }
    if (larp == &larpspare) {
	Move(larp = lawin->RPort, larpspare.cp_x, larpspare.cp_y);
	if (fg < 0) fg = larpspare.FgPen;
	if (bg < 0) bg = larpspare.BgPen;	/* where we're going */
    }
    if (fg >= 0 && fg != larp->FgPen)
	SetAPen(larp, fg);
    if (bg >= 0 && bg != larp->BgPen)
	SetBPen(larp, bg);
}


void MakeLarp(void)
{
    larp = lawin->RPort;
    larpspare = *larp;
    SetDrMd(&larpspare, JAM2);
    SetAPen(&larpspare, tagging | fourcolors ? 1 : NORMCOLOR);
    SetBPen(&larpspare, backcolor);
}


void FixListSlider(short mct, bool force)
{
    long newpot, newbod;

    if (bgustifle || (!force && lastlistpot != lprops.VertPot))
	return;
    newbod = mct ? ((linez - (mct >= 2 * linez)) * MAXBODY
				+ (MAXBODY / 2)) / mct : MAXBODY;
    if (newbod >= 0x10000)
	newbod = MAXBODY;
    if (mct <= linez)
	newpot = 0;
    else
	newpot = (topmess * MAXPOT) / (mct - linez);
    NewModifyProp(&lscroll, lawin, null, AUTOKNOB | FREEVERT | PROPNEWLOOK,
				MAXPOT, newpot, MAXBODY, newbod, 1);
}


void ListAllLines(struct Conf *cc)
{
    short i, n;

    FixListSlider(cc->messct, true);
    for (i = 0; i < linez; i++)
	if ((n = topmess + i) < cc->messct)
	    ListLine(i, cc->messes[n], n == pick);
	else {
	    SetLPens(backcolor, -1);
	    RectFill(larp, (long) lepix, topix + i * fight, pxswide + lepix - 1,
				topix + (i + 1) * fight - 1);
	}
}


void FixTopmess(struct Conf *cc)
{
    topmess = pick - ((linez - 1) >> 1);
    if (topmess > cc->messct - linez)
	topmess = cc->messct - linez;
    if (topmess < 0 || cc->messct <= linez)
	topmess = 0;
}


void RedisplayPickLine(struct Conf *cc)
{
    ASSERT(pick >= 0 && pick < cc->messct);
    if (pick < topmess || pick >= topmess + linez) {
	FixTopmess(cc);
	ListAllLines(cc);
    } else
	ListLine(pick - topmess, cc->messes[pick], true);
}


void BGupdate(struct Conf *cc)
{
    bool bogus = airing | bgustifle | packeting;
    if (deferline || tagging || (bgupdate && !bogus)) {
	if (tagging) {
	    if ( /* !Irped() && */ !(tgagstr.Activation & GACT_ACTIVEGADGET)
			     && (str) tagfaconf.messes[pick] != lasttagrealized)
		RealizeTagline();
	} else {
	    deferrable = true;
	    if (bogus)
		ShowNewMessage(null, onscreen);
	    else
		ShowNewMessage(cc, cc->messes[whichm = pick]);
	    deferrable = false;
	}
    }
    if (!tagging && !bogus && !rupted && cc && waste)
	PreloadConf(cc, pick);
}


void UpListPage(struct Conf *cc)
{
    if (!topmess && !pick)
	return;
    topmess -= linez - 1;
    if (topmess < 0)
	topmess = 0;
    pick -= linez - 1;
    if (pick >= topmess + linez)
	pick = topmess + linez - 1;
    else if (pick < topmess)
	pick = topmess;
    ListAllLines(cc);
}


void DownListPage(struct Conf *cc)
{
    if (topmess >= cc->messct - linez && pick == cc->messct - 1)
	return;
    topmess += linez - 1;
    if (topmess > cc->messct - linez) {
	topmess = cc->messct - linez;
	if (topmess < 0)
	    topmess = 0;
    }
    pick += linez - 1;
    if (pick < topmess)
	pick = topmess;
    else if (pick >= topmess + linez)
	pick = topmess + linez - 1;
    if (pick >= cc->messct)
	pick = cc->messct - 1;
    ListAllLines(cc);
}


void UpListLine(struct Conf *cc)
{
    ASSERT(pick >= 0 && pick < cc->messct);
    if (!pick) return;
    if (pick-- == topmess) {
	if ((topmess -= linez - 1) < 0)
	    topmess = 0;
	ListAllLines(cc);
    } else if (pick < topmess || pick >= linez + topmess) {
	FixTopmess(cc);
	ListAllLines(cc);
    } else {
	ListLine(pick + 1 - topmess, cc->messes[pick + 1], false);
	ListLine(pick - topmess, cc->messes[pick], true);
    }
}


void DownListLine(struct Conf *cc)
{
    ASSERT(pick >= 0 && pick < cc->messct);
    if (pick >= cc->messct - 1) return;
    if (++pick == topmess + linez) {
	if ((topmess += linez - 1) >= cc->messct - linez)
	    topmess = cc->messct - linez;
	if (topmess < 0)
	    topmess = 0;
	ListAllLines(cc);
    } else if (pick >= linez + topmess || pick < topmess) {
	FixTopmess(cc);
	ListAllLines(cc);
    } else {
	ListLine(pick - topmess - 1, cc->messes[pick - 1], false);
	ListLine(pick - topmess, cc->messes[pick], true);
    }
}


void ListTop(struct Conf *cc)
{
    if (!topmess && !pick)
	return;
    pick = topmess = 0;
    ListAllLines(cc);
}


void ListBottom(struct Conf *cc)
{
    pick = cc->messct - 1;
    topmess = pick - (linez - 1);
    if (topmess < 0)
	topmess = 0;
    ListAllLines(cc);
}


bool LScrollUp(struct Conf *cc, short dist)
{
    short l;
    if (topmess <= 0)
	return false;
    if (dist > topmess)
	dist = topmess;
    topmess -= dist;
    MyScrollRaster(larp, /* 0, */ -fight * dist, lepix, topix,
			lepix + pxswide, topix + fight * linez - 1);
    for (l = 0; l < dist; l++)
	ListLine(l, cc->messes[topmess + l], pick == topmess + l);
    FixListSlider(cc->messct, true);
    return true;
}


bool LScrollDown(struct Conf *cc, short dist)
{
    short newbot, l;

    l = cc->messct - linez;
    if (topmess >= l)
	return false;
    if (dist > l - topmess)
	dist = l - topmess;
    topmess += dist;
    newbot = topmess + linez - 1;
    MyScrollRaster(larp, /* 0, */ fight * dist, lepix, topix,
				lepix + pxswide, topix + fight * linez - 1);
    for (l = dist - 1; l >= 0; l--)
	ListLine(linez - 1 - l, cc->messes[newbot - l], pick == newbot - l);
    FixListSlider(cc->messct, true);
    return true;
}


void CenterList(struct Conf *cc)
{
    FixTopmess(cc);
    ListAllLines(cc);
    /* Someday, optimize this to call LScrollDown/Up. */
}


void SearchInList(struct Conf *cc)
{
    bool happenin, oldinuse = inuseified;
    whatsearch whats = packeting ? filezwin : (airing
			? (emptyairing | addropping ? areazwin : readareazwin)
			: (tagging ? taglinewin : listwin));

    readareaz.current = whicha;		/* ignore current pick */
/*  if (whats <= listwin)
	LeaveListArea(cc); */
    happenin = Soich(whats);
    if (oldinuse != inuseified)		/* search called ToggleInUseify */
	cc = listcc;
    wasescaped |= poseclosed;
    if (wasescaped || !happenin || alswitch) {
	if (alswitch)
	    pick = readareaz.confs[whicha]->current;
	else
	    cc->current = pick;		/* two noids */
	return;
    }
    pick = cc->current;			/* == whicha for readareaz */
    FixTopmess(cc);
    ListAllLines(cc);
    Titlist(cc);
}


void UnSearchList(struct Conf *cc, ushort alted)
{
    if (packeting | tagging) {
	ASSERT(pick >= 0 && pick < cc->messct);
	cc->current = pick;
	UnSearch(cc, true);
	pick = cc->current;
    } else if (emptyairing | addropping) {
	struct Conf *zz = inuseified ? &inuseareaz : &areaz;
	ASSERT(pick >= 0 && pick < zz->messct);
	zz->current = pick;
	UnSearch(zz, true);	/* doing one also magically does the other */
	pick = zz->current;
    } else {
	if (airing) {
	    struct Conf *lcc = readareaz.confs[pick];
	    ASSERT(pick >= 0 && pick < readareaz.messct);
	    if (pick == whicha)
		lcc->current = whichm;
	    if (alted) {
		readareaz.current = pick;
		UnSearch(&readareaz, true);
		pick = readareaz.current;
	    } else
		UnSearch(lcc, true);
	} else {
	    ASSERT(pick >= 0 && pick < cc->messct);
	    cc->current = pick;
	    UnSearch(alted ? &readareaz : cc, true);
	    pick = cc->current;
	}
    }
    if (anyunsearched) {
	FixTopmess(cc);
	ListAllLines(cc);
    }
    Titlist(cc);
}


local void ClickNewPick(short oldpick, struct Conf *cc)
{
    ListLine(oldpick - topmess, cc->messes[oldpick], false);
    ListLine(pick - topmess, cc->messes[pick], true);
    BGupdate(cc);
}


bool ListMouseClick(struct Conf *cc, struct IntuiMessage *im)
{
    short line, oldpick;

    if (im->IDCMPWindow != lawin || im->MouseY < topix
				|| im->Code & IECODE_UP_PREFIX)
	return false;
    if (im->Code == IECODE_MBUTTON)		/* by user request */
	return true;
    line = (im->MouseY - topix) / fight;
    if (line >= linez || line + topmess >= cc->messct)
	return false;
    oldpick = pick;
    pick = topmess + line;
    if (pick != oldpick)
	ClickNewPick(oldpick, cc);
    else if (!addropping && !taintag) {
	if (DoubleClick(lastclick.tv_secs,
				lastclick.tv_micro, im->Seconds, im->Micros))
	    return true;
	else if (tagging)
	    ClickNewPick(oldpick, cc);
    }
    lastclick.tv_secs = im->Seconds;
    lastclick.tv_micro = im->Micros;
    return false;
}


void TwiddleSlider(void)
{
    short toppad = scr->WBorTop + 2, *bigns = tagging ? tagbignums : bignums;

    lscroll.LeftEdge = -13;
    lscroll.TopEdge = tifight + toppad;
    lscroll.Width = 10;
    lscroll.Height = -31 - toppad - tifight;
    lastbutton = null;
    bigns[2] = scr->Width;
    bigns[3] = scr->Height - 3;
}


bool BasicListIDCMP(struct IntuiMessage *im)
{
    short gid, line;
    char k;

    switch (im->Class) {
      case IDCMP_CLOSEWINDOW:
	if (im->IDCMPWindow == cbitwin) {
	    CloseBitsWin();
	    break;
	} else if (im->IDCMPWindow == bkfwin) {
	    CloseKeyFilWin();
	    break;
	}
	poseclosed = im->IDCMPWindow == cwin || im->IDCMPWindow == bwdwin
				    || im->IDCMPWindow == llwin;
	return wasescaped = im->IDCMPWindow == lawin || poseclosed;
      case IDCMP_RAWKEY:
	k = KeyToAscii(im->Code, im->Qualifier);
	if (k & 0x80)
	    switch (k & 0x7F) {
	      case 0x5f:				/* Help key */
		/* put up a help window */
		break;
	      case 0x4C: case 0x3E: case 0x4F: case 0x2D:   /* Up, Left */
		if (im->Qualifier & IEQUALIFIER_CONTROL)
		    ListTop(listcc);
		if (im->Qualifier & (SHIFTKEYS | ALTKEYS))
		    UpListPage(listcc);
		else
		    UpListLine(listcc);
		break;
	      case 0x4D: case 0x1E: case 0x4E: case 0x2F:   /* Down, Right */
		if (im->Qualifier & IEQUALIFIER_CONTROL)
		    ListBottom(listcc);
		if (im->Qualifier & (SHIFTKEYS | ALTKEYS))
		    DownListPage(listcc);
		else
		    DownListLine(listcc);
		break;
	      case 0x3f:				/* kp9 / PgUp */
		UpListPage(listcc);
		break;
	      case 0x1F:				/* kp3 / PgDn */
		DownListPage(listcc);
		break;
	      case 0x3D:				/* kp7 / Home */
		ListTop(listcc);
		break;
	      case 0x1D:				/* kp1 / End */
		ListBottom(listcc);
		break;
	      case 0x2E:				/* kp5 (center) */
		CenterList(listcc);
		break;
	    }
	else if (k == 'U' || !(im->Qualifier & ALTKEYS))
	    switch (k) {
	      case ESC:					/* Esc key */
		wasescaped = true;
		return true;
	      case '\r': case '\n':  case ' ':		/* Return, space bar */
		return true;
	      case '`': case '~':			/* (center) */
		CenterList(listcc);
		break;
	    }
	break;
      case IDCMP_MOUSEBUTTONS:
	return ListMouseClick(listcc, im);
      case IDCMP_NEWSIZE:
	if (tagging)
	    PaintTaglineWin();
	else
	    PaintListWin(listcc);
	break;
      case IDCMP_GADGETDOWN:
      case IDCMP_GADGETUP:
      case IDCMP_MOUSEMOVE:
	buttoning = false;
	lastbgbutton = null;
	if (im->Class != IDCMP_MOUSEMOVE)
	    gid = ((struct Gadget *) im->IAddress)->GadgetID;
	if (im->Class == IDCMP_MOUSEMOVE || gid == 202) {	/* slider */
	    bgustifle = im->Class != IDCMP_GADGETUP;
	    lastlistpot = lprops.VertPot;
	    line = (lprops.VertPot * (listcc->messct - linez)
					    + (MAXPOT >> 1)) >> 16;
	    if (line < 0) line = 0;
	    if (line != topmess) {
		if (line > topmess && line - topmess < linez)
		    LScrollDown(listcc, line - topmess);
		else if (line < topmess && topmess - line < linez)
		    LScrollUp(listcc, topmess - line);
		else {
		    topmess = line;
		    ListAllLines(listcc);
		}
	    }
	    if (!bgustifle)
		FixListSlider(listcc->messct, false);
	    bgustifle = false;
	} else if (gid == 200 && im->Class == IDCMP_GADGETDOWN) {
	    lastbutton = &ldownarrow;
	    SetAlarm(SCROLLWAIT);
	    LScrollDown(listcc, 1);
	} else if (gid == 201 && im->Class == IDCMP_GADGETDOWN) {
	    lastbutton = &luparrow;
	    SetAlarm(SCROLLWAIT);
	    LScrollUp(listcc, 1);
	}
	break;
    }
    return false;
}


void ListAreaIDCMP(struct Conf *cc)	/* this used to be complex... */
{
    listcc = (cc == &areaz && inuseified ? &inuseareaz : cc);
    EventLoop(tagging ? &Do1TaglineIDCMP : &Do1ListAreaIDCMP);
}


/* ------------------------------------------------------ */


bool CreateNewArea(str cnum, str shortname, str lname, ushort *initialarea)
{
    struct Conf *newc, **newconfs;

    if (inuseified)
	ToggleInUseify();	/* simplify hassles */
    if (areaz.messct >= areazroom) {
	if (areazroom + INCREMENT > 32767)
	    return false;
	if (!(newconfs = Valloc((areazroom + INCREMENT) << 2)))
	    return false;
	memcpy(newconfs, areaz.confs, areaz.messct << 2);
	Vfree(areaz.confs);
	areaz.confs = newconfs;
	areazroom += INCREMENT;
    } else
	newconfs = areaz.confs;
    if (!(newc = NewPoolConf()) ||
			!(newc->longname = NewPoolString(strlen(lname))))
	return false;
    newconfs[*initialarea = areaz.messct++] = newc;
    strcpy(newc->confnum, cnum);
    strcpy(newc->shortname, shortname);
    strcpy(newc->longname, lname);
    newc->areabits = INF_POST | INF_ANY_NAME;
    newc->morebits = ALLREAD;
    return true;	/* ALLREAD & !INF_SCANNING marks this as a fake area */
}


bool ValidNewArea(ushort *initialarea)
{
    long zork = nastrnum.LongInt;
    char splork[8];
    struct Conf *cc;
    ustr nnp;

    if (!isdigit(nanumber[0]) || (ulong) zork > 9999999)
	Err("The area number must be in\nthe range from 0 to 9999999.");
    else if (!naname[0]) {
	Err("Please enter some sort of\nname for the area.");
	ActivateGag(&nagagname, nawin);
	return false;
    } else {
	sprintf(splork, "%lu", zork);
	if (cc = Confind(splork))
	    if (cc->morebits & ALLREAD && !(cc->areabits & INF_SCANNING)) {
		if (nnp = NewPoolString(strlen(naname))) {
		    strcpy(cc->longname = nnp, naname);
		    return true;
		} else
		    Err("Can't rename fake area\n%ld; no memory.", zork);
	    } else
		Err("The area number %ld is already in use.", zork);
	else {
	    if (!CreateNewArea(splork, "", naname, initialarea))
		Err("Can't add area; no memory or\ntoo many exist already.");
	    return true;
	}
    }
    ActivateGag(&nagagnum, nawin);
    return false;
}


local ushort ginitialarea;

local bool DoAddUnknownAreaIDCMP(struct IntuiMessage *im)
{
    char k;
    bool done = false;

    switch (im->Class) {
      case IDCMP_CLOSEWINDOW:
	poseclosed = im->IDCMPWindow == cwin || im->IDCMPWindow == bwdwin
				|| im->IDCMPWindow == llwin;
	done = im->IDCMPWindow == nawin || poseclosed;
	break;
      case IDCMP_RAWKEY:
	if (im->Qualifier & ALTKEYS)
	    break;
	k = KeyToAscii(im->Code, im->Qualifier);
	if (k == ESC || k == 'C')
	    done = true;
	else if (k == '\t')
	    ActivateGag((nanumber[0] && !naname[0]
			    ? &nagagname : &nagagnum), nawin);
	else if (k == '\n' || k == '\r' || k == 'O')
	    done = ValidNewArea(&ginitialarea);
	break;
      case IDCMP_GADGETUP:
	switch (((struct Gadget *) im->IAddress)->GadgetID) {
	  case 1100:
	    if (!nanumber[0] && GAGCHAIN(im))
		ActivateGag(&nagagnum, nawin);
	    break;
	  case 1101:
	    if (!naname[0] && GAGCHAIN(im))
		ActivateGag(&nagagname, nawin);
	    break;
	  case 1102:			/* Okay */
	    done = ValidNewArea(&ginitialarea);
	    break;
	  case 1103:			/* Cancel */
	    done = true;
	    break;
	}
	break;
    }
    return done;
}


short AddUnknownArea(ushort initialarea)
{
    newareawin = false;
    naname[0] = nanumber[0] = 0;
    nagagnum.TopEdge = fakefight + 12;
    nagagname.TopEdge = 2 * nagagnum.TopEdge;
    nagagokay.TopEdge = nagagcancel.TopEdge = nagagname.TopEdge + fakefight + 9;
    naneww.Height = nagagokay.TopEdge + fakefight + 13;
    if (!(nawin = OpenBlueGagWin(&naww, &nagagname))) {
	WindowErr("adding a new area.");
	return initialarea;
    }
    FixDefarrow();
    defarrow.LeftEdge = nagagokay.LeftEdge;
    defarrow.TopEdge = nagagokay.TopEdge;
    DrawBorder(nawin->RPort, &defarrow, ARROWLEFT, 0);
    ActivateGag(&nagagnum, nawin);
    ginitialarea = initialarea;
    EventLoop(&DoAddUnknownAreaIDCMP);
    CloseBlueGagWin(nawin);
    nawin = null;
    CreateBBSFile(true);
    return ginitialarea;
}
