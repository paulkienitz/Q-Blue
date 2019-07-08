/* list the messages in a conference, pick one to read first ... or similarly
list files in the download directory, or message areas */


#include <intuition/intuition.h>
#include <intuition/sghooks.h>
#include "qblue.h"
#include "pigment.h"


#define __Li1_     IDCMP_MOUSEBUTTONS | IDCMP_CLOSEWINDOW | IDCMP_RAWKEY
#define __Li2_     IDCMP_NEWSIZE | IDCMP_GADGETDOWN | IDCMP_GADGETUP
#define LIST_IDCMP (__Li1_ | __Li2_ | IDCMP_INTUITICKS | IDCMP_MOUSEMOVE)


#define SCROLLWAIT  500000

#define LWINWIDTH   616
#define LWINHEIGHT  100

#define LMGAP       2

#define NORMCOLOR   7	/* keep lcore.c in sync */
#define TOOCOLOR    5
#define HEADCOLOR   1
#define FARCOLOR    1
#define BARCOLOR    2


void ResetAddsAndDrops(struct Conf *cc);
void AreaAddDrop(struct Conf *cc, bool add);
void UnSearch(struct Conf *where, bool freshen);
short AddUnknownArea(ushort initialarea);
void CheckAnyAllRead(struct Conf *cc);
void FixGreenGadMaybe(struct Gadget *gg, struct Window *ww, bool rend);
void JoinName(str result, str dir, str file, str tail);
bool AskDeleteListedFile(str path, str date, ulong size, ushort dirtok);
void FreeReply(struct Mess *mm);

void SetLPens(short fg, short bg);
void MakeLarp(void);
void ListAllLines(struct Conf *cc);
void RedisplayPickLine(struct Conf *cc);
void UpListPage(struct Conf *cc, bool force);
void DownListPage(struct Conf *cc);
void UpListLine(struct Conf *cc);
void DownListLine(struct Conf *cc);
void ListTop(struct Conf *cc);
void ListBottom(struct Conf *cc);
void LScrollUp(struct Conf *cc, short dist);
void LScrollDown(struct Conf *cc, short dist);
void SearchInList(struct Conf *cc);
void UnSearchList(struct Conf *cc, ushort alted);
bool ListMouse(struct Conf *cc, struct IntuiMessage *im);
void ListAreaIDCMP(struct Conf *cc);
bool BasicListIDCMP(struct IntuiMessage *im);
void FixTopmess(struct Conf *cc);
void BGupdate(struct Conf *cc);
void Titlist(struct Conf *cc);
void TwiddleSlider(void);
void FixListSlider(short mct, bool force);
void PreloadConf(struct Conf *cc, ushort which);


import struct Window *bwdwin, *bkfwin;
import struct Gadget ldownarrow, luparrow;
import struct IntuiText bgt1active, bgt1all, bgt1add, bgt1none, bgt5drop;
import struct IntuiText bgt4new, bgt2s, bgt3reset, itrep2R;
import struct MenuItem mirep2;
import struct PropInfo lprops;

import struct Conf *curconf, *listcc;
import short *tempcurrents;

import char toggletitle[], listitle[];
import long undercolor;
import ushort oldracount, tifight, addropmask;
import bool bgustifle, tagging, addsdrops, showsizes;


ListLineFType ListLine;

ushort charz, topix, lepix, pxstall, pxswide, prelistbits = 0;
short pick, topmess, linez, oldwhicha, oldwhichm;
long hilicolor;

bool airing, emptyairing, packeting, repling, bulling, addropping, personing;
bool newareawin, realrequester, alswitch = false, inuseified = false;
bool areachanging, wasescaped;
local bool nonegag, localnone, delayispast = false, repainting = false;
local bool enterized = false;

local short normcolor, fromcolor, bpcolor, toocolor,
		zingacolor, zingbcolor, blatcolor;


short bignums[4] = { 0, 3, 640, 197 };

struct {
    long t;
    short *a;
    long e;
} ltags = { WA_Zoom, bignums, TAG_DONE };


struct ExtNewWindow lneww = {
    12, 80, LWINWIDTH, LWINHEIGHT, 0, 1,
    IDCMP_MOUSEBUTTONS | IDCMP_CLOSEWINDOW | IDCMP_RAWKEY | IDCMP_NEWSIZE
		| IDCMP_GADGETDOWN | IDCMP_GADGETUP | IDCMP_ACTIVEWINDOW
		| IDCMP_INTUITICKS | IDCMP_MOUSEMOVE,
    WFLG_SMART_REFRESH | WFLG_CLOSEGADGET | WFLG_DEPTHGADGET | WFLG_SIZEGADGET
		| WFLG_DRAGBAR | WFLG_ACTIVATE | WFLG_NW_EXTENDED,
    &ldownarrow, null, listitle, null, null, 430, 64, 9999, 9999,
    CUSTOMSCREEN, (adr) &ltags
};

struct ExtNewWindow arneww = {
    12, 40, LWINWIDTH, LWINHEIGHT, 0, 1, LIST_IDCMP,
    WFLG_SMART_REFRESH | WFLG_CLOSEGADGET | WFLG_DEPTHGADGET | WFLG_SIZEGADGET
		| WFLG_DRAGBAR | WFLG_ACTIVATE | WFLG_NW_EXTENDED,
    &ldownarrow, null, listitle, null, null, 430, 64, 9999, 9999,
    CUSTOMSCREEN, (adr) &ltags
};

struct ExtNewWindow awneww = {
    12, 20, LWINWIDTH, LWINHEIGHT, 0, 1, LIST_IDCMP,
    WFLG_SMART_REFRESH | WFLG_CLOSEGADGET | WFLG_DEPTHGADGET | WFLG_SIZEGADGET
		| WFLG_DRAGBAR | WFLG_ACTIVATE | WFLG_NW_EXTENDED,
    &ldownarrow, null, listitle, null, null, 430, 64, 9999, 9999,
    CUSTOMSCREEN, (adr) &ltags
};


struct ExtNewWindow atneww = {
    12, 80, LWINWIDTH, LWINHEIGHT, 0, 1, LIST_IDCMP,
    WFLG_SMART_REFRESH | WFLG_CLOSEGADGET | WFLG_DEPTHGADGET | WFLG_SIZEGADGET
		| WFLG_DRAGBAR | WFLG_ACTIVATE | WFLG_NW_EXTENDED,
    &ldownarrow, null, toggletitle, null, null, 430, 64, 9999, 9999,
    CUSTOMSCREEN, (adr) &ltags
};

struct ExtNewWindow fneww = {
    52, 60, LWINWIDTH - 80, LWINHEIGHT, 0, 1, LIST_IDCMP,
    WFLG_SMART_REFRESH | WFLG_CLOSEGADGET | WFLG_DEPTHGADGET | WFLG_SIZEGADGET
		| WFLG_DRAGBAR | WFLG_ACTIVATE | WFLG_NW_EXTENDED,
    &ldownarrow, null, listitle, null, null, 430, 64, 9999, 9999,
    CUSTOMSCREEN, (adr) &ltags
};

struct WhereWin lww = { &lneww }, arww = { &arneww },
		awww = { &awneww }, atww = { &atneww}, fww = { &fneww };


struct Window *lawin;
struct RastPort *larp;


void Ftext(str s, long width, short fg, short bg)
{
    char buf[80];
    short i;
    if (!width)
	return;
    if (!s)
	s = "";
    if (width >= 80)
	width = 79;
    SetLPens(s[0] ? fg : -1, bg);
    for (i = 0; i < width; i++)
	buf[i] = *s ? *(s++) : ' ';
    Text(larp, buf, width);
}


void FiddleHeight(struct WhereWin *ww)
{
    short padding = scr->WBorTop + tifight + 3 + (tagging ? 9 : 5);
    struct ExtNewWindow *nw = ww->nw;
    short *h;

    nw->MinHeight = 4 * fight + padding;
    nw->MaxHeight = scr->Height;
    if (!tagging)
	nw->MinWidth = 50 * fontwid + 20 + LMGAP;
    if (ww->moved && ww->scrheight == scr->Height)
	h = &ww->height;
    else {
	nw->Height = (LWINHEIGHT << lace) + fight;
	if (nw == &lneww)
	    nw->TopEdge = texbot + bgwin->TopEdge - (16 << lace) - nw->Height;
	h = &nw->Height;
    }
    *h -= (*h - padding) % fight;
    if (*h < nw->MinHeight)
	*h = nw->MinHeight;
    nw->Width -= (nw->Width - LMGAP - 24) % fontwid;	/* no right margin */
}


void Pickolors(bool hilite, short line)
{
    long top = topix + line * fight, bot = top + fight - 1;

    bpcolor = hilite ? hilicolor : backcolor;
    zingacolor = 1;
    blatcolor = 3;
    toocolor = TOOCOLOR;
    if (fourcolors)
	if (hilite)
	    zingbcolor = fromcolor = blatcolor = normcolor = toocolor = 0;
	else {
	    fromcolor = zingbcolor = 3;
	    normcolor = 1;
	    zingacolor = 0;
	}
    else {
	fromcolor = 6;
	zingbcolor = 3;
	normcolor = hilite ? undercolor : NORMCOLOR;
    }
    if (hilite || !repainting) {
	SetLPens(bpcolor, bpcolor);
	if ((pxswide - LMGAP) % fontwid)
	    RectFill(larp, charz * fontwid + lepix + LMGAP, top,
				pxswide + lepix - 1, bot);
#if LMGAP > 0
	RectFill(larp, (long) lepix, top, lepix + LMGAP - 1, bot);
#endif
    } else
	SetLPens(-1, bpcolor);
    Move(larp, lepix + LMGAP, top + font->tf_Baseline);
}


void FilesListLine(short line, struct Mess *mm, bool hilite)
{
    char buf[12];
    short i;

    if (line < 0 || line >= linez)
	return;
    Pickolors(hilite, line);
    if (mm->bits & MEREPLIED)
	Ftext(mm->bits & SEENINBASE ? "old" : "old?", 4, fromcolor, -1);
    else if (mm->bits & MESEEN)
	Ftext(mm->bits & SEENINBASE ? "NEW" : "NEW?", 4, blatcolor, -1);
    else
	Ftext(null, 4, -1, -1);
    sprintf(buf, "%8ld", mm->datflen);
    Ftext(buf, 10, normcolor, -1);
    Ftext(mm->date, 17, toocolor, -1);
    i = max(14, strlen(mm->from));
    if (charz - 31 <= i)
	Ftext(mm->from, charz - 31, normcolor, -1);
    else {
	Ftext(mm->from, i, normcolor, -1);
	Ftext(null, 1, 0, -1);
	Ftext(mm->subject, charz - 32 - i, fromcolor, -1);
    }
}


void MessesListLine(short line, struct Mess *mm, bool hilite)
{
    char buf[12];
    struct Conf *cc;

    if (line < 0 || line >= linez)
	return;
    Pickolors(hilite, line);
    if (repling && mm->bluebits & UPL_INACTIVE)
	Ftext("D", 1, zingacolor, zingbcolor);
    else if (mm->bits & MEREPLIED)
	Ftext("R", 1, zingbcolor, bpcolor);
    else if (mm->bits & MESEEN)
	Ftext(">", 1, fromcolor, bpcolor);
    else
	Ftext(null, 1, 0, bpcolor);
    if (showsizes) {
	if (mm->datflen > 99999)
	    sprintf(buf, "%5luk ", (mm->datflen + 512) / 1024);
	else
	    sprintf(buf, "%7lu", mm->datflen);
    } else if (!personing && !repling && !bulling)
	sprintf(buf, "%7lu", mm->ixinbase);
    else
	sprintf(buf, "%7lu", line + topmess + 1L);
    Ftext(buf, 8, fourcolors ? normcolor : (bulling ? 5 : 1), bpcolor);
    if (repling) {
	Ftext((cc = Confind(mm->confnum)) ? LONGNAME(cc) : null,
				18, toocolor, -1);
	Ftext(null, 1, 0, -1);
    } else if (!bulling) {
	Ftext(mm->from, 18, (mm->bits & FROMME ? (fourcolors ? normcolor : 6)
				: normcolor), -1);
	Ftext(null, 1, 0, -1);
    }
    if (personing)
	Ftext((cc = Confind(mm->confnum)) ? LONGNAME(cc) : null,
				18, toocolor, -1);
    else
	Ftext(mm->too, 18, (mm->bits & TOME && !repling ? toocolor
				: (fourcolors ? normcolor : 1)), -1);
    Ftext(null, 1, 0, -1);
    Ftext(mm->subject, charz - (bulling ? 28 : 47), normcolor, -1);
}


void AreasListLine(short line, struct Mess *mm, bool hilite)
{
    char buf[12];
    struct Conf *cc = (adr) mm;
    ushort abits, mbits, i;

    if (line < 0 || line >= linez)
	return;
    abits = cc->areabits, mbits = cc->morebits;
    Pickolors(hilite, line);
    if (emptyairing | addropping) {
	if (addropping) {
	    i = 1;
	    if (!qwk && mbits & DOOR_ADDING_YOURS)
		if (mbits & DOOR_ADDING)
		    Ftext("PAll", 4, zingacolor, zingbcolor);
		else
		    Ftext("Pers", 4, zingacolor, zingbcolor);
	    else if (mbits & DOOR_RESETTING)
		if (mbits & DOOR_ADDING)
		    Ftext("AddR", 4, zingacolor, zingbcolor);
		else
		    Ftext("Rset", 4, zingacolor, zingbcolor);
	    else if (mbits & DOOR_ADDING)
		Ftext("Add", 3, zingacolor, zingbcolor), i = 2;
	    else if (mbits & DOOR_DROPPING)
		Ftext("Drop", 4, zingacolor, zingbcolor);
	    else if (mbits & DOOR_ADDING_YOURS)
		Ftext("misc", 4, zingacolor, zingbcolor);
	    else if (abits & INF_TO_ALL)
		Ftext("PAll", 4, toocolor, bpcolor);
	    else if (abits & INF_PERSONAL)
		Ftext("Pers", 4, toocolor, bpcolor);
	    else if (abits & INF_SCANNING)
		Ftext(" Yes", 4, toocolor, bpcolor);
	    else if (qwk)
		Ftext(" ?", 4, fromcolor, bpcolor);
	    else
		Ftext(" no", 4, fromcolor, bpcolor);
	    Ftext(null, i, 0, bpcolor);
	}
	if (!(abits & INF_POST))
	    Ftext("R", 1, zingacolor, zingbcolor);
	else if (abits & INF_NETMAIL)
	    Ftext("N", 1, zingacolor, zingbcolor);
	else if (mbits & INTERNET_EMAIL)
	    Ftext("@", 1, zingacolor, zingbcolor);
	else if (mbits & NEWSGROUP)
	    Ftext("U", 1, zingbcolor, bpcolor);
	else if (abits & INF_ECHO)
	    Ftext("E", 1, fromcolor, bpcolor);
	else
	    Ftext(null, 1, 0, bpcolor);
    } else {
	if (cc->unfiltered) {
	    Ftext("S", 1, zingacolor, zingbcolor);
	} else if (mbits & ALLREAD)
	    Ftext("* ", 1, fromcolor, bpcolor);
	else if (mbits & ANYREAD)
	    Ftext("> ", 1, blatcolor, bpcolor);
	else
	    Ftext(null, 1, 0, bpcolor);
    }
    if (cc != &replies && cc != &personals && cc != &bullstuff)
	sprintf(buf, qwk ? "%7s" : "%6s", cc->confnum);
    else
	buf[0] = 0;
    Ftext(buf, 8 + qwk, normcolor, bpcolor);
    if (emptyairing | addropping) {
	if (qwk)
	    Ftext(LONGNAME(cc), charz - (addropping ? 15 : 10), normcolor, -1);
	else {
	    Ftext(cc->shortname, 21, normcolor, -1);
	    Ftext(cc->longname, charz - (addropping ? 35 : 30), normcolor, -1);
	}
    } else {
	sprintf(buf, "%4lu", (ulong) cc->messct);
	Ftext(buf, 5, normcolor, -1);
	sprintf(buf, "%4lu", (ulong) cc->tomect);
	Ftext(buf, 7, toocolor, -1);
	Ftext(LONGNAME(cc), charz - 21 - qwk, normcolor, -1);
    }
}


void PaintListWin(struct Conf *cc)
{
    ulong redge;

    topix = lawin->BorderTop + fight + 4;
    lepix = lawin->BorderLeft + 1;
    pxstall = lawin->Height - topix - lawin->BorderBottom - 1;
    pxswide = lawin->Width - lawin->BorderRight - lepix - 1;
    redge = pxswide + lepix;
    linez = pxstall / fight;
    charz = (pxswide - LMGAP) / fontwid;
    FixTopmess(cc);
    FixListSlider(cc->messct, true);	/* otherwise visible glitch later */
    SetLPens(0, 0);
/*    Move(larp, redge - 1, (long) topix - 2); */
/*    Draw(larp, redge - 1, pxstall + topix);  */
    Move(larp, redge, pxstall + topix);
    Draw(larp, (long) lepix, pxstall + topix);
    Move(larp, redge, (long) topix - 2);
    Draw(larp, redge, pxstall + topix);
    RectFill(larp, 13 /* shortest column string */ * fontwid + lepix + LMGAP,
					tifight + 3, redge, topix - 1);
    SetLPens(3, 0);
    Move(larp, (long) lepix, topix - 2);
    Draw(larp, redge - 1, topix - 2);
    Move(larp, lepix + LMGAP, topix + font->tf_Baseline - fight - 3);
    SetLPens(HEADCOLOR, 0);
    ASSERT(charz >= 49);
    if (addropping) {
	if (qwk)
	    Text(larp, "Read?   Area#  Name", 19);
	else
	    Text(larp, "Read?  Area#  Area-tag             Full name", 44);
    } else if (emptyairing) {
	if (qwk)
	    Text(larp, "   Area#  Name", 14);
	else
	    Text(larp, "  Area#  Area-tag             Full name", 39);
    } else if (airing)
	if (qwk)
	    Text(larp, "   Area#  Msgs  You   Area name", 31);
	else
	    Text(larp, "  Area#  Msgs  You   Area name", 30);
    else if (packeting) {
	if (fakery)
	    Text(larp, "Rep?    Size  Date             Filename", 39);
	else {
	    Text(larp, "Rep?    Size  Date             Filename       Note",
					min(50, charz));
	}
    } else {
	Text(larp, showsizes ? "    Size " : "     Num ", 9);
	if (bulling)
	    Text(larp, "Filename           Subject", 26);
	else {
	    Text(larp, repling ? "Area               "
					: "From               ", 19);
	    Text(larp, personing ? "Area" : "To  ", 4);
	    Text(larp, "               Subject", min(22, charz - 32));
	}
    }
    SetLPens(backcolor, -1);
    if (pxstall % fight)
	RectFill(larp, (long) lepix, topix + linez * fight,
				redge - 1, topix + pxstall - 1);
    redge = topix + pxstall - 1;		/* bottom edge */
    if ((pxswide - LMGAP) % fontwid)
	RectFill(larp, charz * fontwid + lepix + LMGAP, topix,
				pxswide + lepix - 1, redge);
#if LMGAP > 0
    RectFill(larp, (long) lepix, topix, lepix + LMGAP - 1, redge);
#endif
    repainting = true;
    ListAllLines(cc);
    repainting = false;
}


bool SetPickAt(struct Conf *in, adr which)
{
    register short a;
    for (a = 0; a < in->messct; a++)
	if (in->confs[a] == which) {
	    pick = a;
	    return true;
	}
    return false;
}

/* this could probably be used a couple places in search.c ... */


void ToggleInUseify(void)
{
    struct Conf *cc;
    struct IntuiText *nbgt;

    if (!inuseareaz.confs)
	return;
    if (inuseified = !inuseified) {
	nbgt = &bgt1all;
	ASSERT(listcc == &areaz);
	ASSERT(pick >= 0 && pick < areaz.messct);
	listcc = &inuseareaz;
	cc = areaz.confs[areaz.current = pick];
    } else {
	nbgt = &bgt1active;
	ASSERT(listcc == &inuseareaz);
	ASSERT(pick >= 0 && pick < inuseareaz.messct);
	listcc = &areaz;
	cc = inuseareaz.confs[inuseareaz.current = pick];
    }
    if (!SetPickAt(listcc, cc) /* && !SetPickAt(listcc, curconf) */ )
	pick = listcc->current;
    if (!lawin)
	return;
    ChangeBotGagText(&readgag1, nbgt, true);
    FixTopmess(listcc);
    Titlist(listcc);
    ListAllLines(listcc);
}


/* tempcurrents[x] is the temporary cruise-through value of ->current when
x != whicha, but is the permanent ->current value when x == whicha (temp in
current itself). */

local struct Conf *lastleft;


void ClearTempCurrents(short which)
{
    if (!tempcurrents)
	return;
    if (which < 0)
	memset(tempcurrents, -1, ((long *) tempcurrents)[-1]);
    else {
	ASSERT(which < readareaz.messct);
	tempcurrents[which] = -1;
    }
    lastleft = null;
}


void LeaveListArea(struct Conf *lcc)
{
    short t = lcc->current, lw = whicha, i;
    if (lcc != readareaz.confs[lw])
	for (i = 0; i < readareaz.messct; i++)
	    if (lcc = readareaz.confs[i]) {
		lw = i;
		break;
	    }
/* Note: as far as I can tell in empirical trials, the final value of lw    */
/* always equals whicha, which means that the for loop above is never used. */
    if (lcc != lastleft && tempcurrents && tempcurrents[lw] >= 0)
	lcc->current = tempcurrents[lw], tempcurrents[lw] = t;
    lastleft = lcc;
}


void EnterListArea(struct Conf *lcc)
{
    short t = lcc->current;
    ASSERT(whicha >= 0 && whicha < readareaz.messct);
    ASSERT(lcc == readareaz.confs[whicha]);
    if (!tempcurrents)
	return;
    if (tempcurrents[whicha] >= 0)
	lcc->current = tempcurrents[whicha];
    tempcurrents[whicha] = t;
    if (lastleft == lcc)
	lastleft = null;
}


local bool MsgListNewArea(struct Conf *lcc, bool decrease)
{
    ASSERT(whicha >= 0 && whicha < readareaz.messct);
    if (decrease ? !whicha : whicha >= readareaz.messct - 1)
	return false;
    ASSERT(pick >= 0 && pick < lcc->messct);
    lcc->current = pick;	/* temporary value! */
    LeaveListArea(lcc);
    if (decrease)
	whicha--;
    else
	whicha++;
    listcc = readareaz.confs[whicha];
    EnterListArea(listcc);
    pick = whichm = listcc->current;
    ASSERT(pick < listcc->messct);
    repling = listcc == &replies;
    bulling = listcc == &bullstuff;
    personing = listcc == &personals;
    Titlist(listcc);
    PaintListWin(listcc);
    return areachanging = true;
}


void FileDelOption(struct Mess *mm)
{
    char filename[406];
    struct Conf *luf;
    short m, lfp;

    ASSERT(listcc->messes[pick] == mm);
    JoinName(filename, fakery ? bbsesdir : downloaddir, mm->from, null);
    if (!AskDeleteListedFile(filename, mm->date, mm->datflen,
						fakery ? DBBS : DDOWN))
	return;
    if (!DeleteFile(filename)) {
	DosErr("Deletion of %s failed.", filename);
	return;
    }
    FreeReply(mm);
    if (luf = listcc->unfiltered) {
	lfp = luf->messct;
	for (m = 0; m < luf->messct; m++)
	    if (luf->messes[m] == mm) {
		lfp = m;
		break;
	    }
	if (lfp < luf->messct) {
	    if (lfp < --luf->messct)
		memmove(luf->messes + lfp, luf->messes + lfp + 1,
					sizeof(adr) * (luf->messct - lfp));
	    ASSERT(lfp || listcc->messct > 1);
    	    /* in other words, if list emptied, then wasescaped set below */
	}
    }
    if (pick < --listcc->messct)
	memmove(listcc->messes + pick, listcc->messes + pick + 1,
					sizeof(adr) * (listcc->messct - pick));
    else if (pick)
	pick--;
    else {
	wasescaped = true;
	return;
    }
    if (pick < topmess || listcc->messct - topmess < linez)
	FixTopmess(listcc);
    ListAllLines(listcc);
    strcat(filename, ".info");
    DeleteFile(filename);
}


local short LookForString(struct Conf *reaz, short start, str word, str inw)
{
    short i;
    for (i = start + 1; i < reaz->messct; i++) {
	register struct Conf *cc = reaz->confs[i];
	if (strnistr(cc->shortname, word, strlen(cc->shortname))) {
	    strcpy(inw, cc->shortname);
	    return i;
	}
	if (cc->longname && strnistr(cc->longname, word,
					strlen(cc->longname))) {
	    strcpy(inw, cc->longname);
	    return i;
	}
    }
    return -1;
}


local short LookForNetmailStrings(struct Conf *reaz, bool internet)
{
    char inw[LONCOLEN];
    short p = -1, q;

    if (internet) {
	while ((p = LookForString(reaz, p, "INTERNET", inw)) >= 0) {
	    q = strlen(inw);
	    if (strnistr(inw, "MAIL", q))
		return p;
	}
	p = -1;
	while ((p = LookForString(reaz, p, "INET", inw)) >= 0) {
	    q = strlen(inw);
	    if (strnistr(inw, "MAIL", q))
		return p;
	}
	p = -1;
	while ((p = LookForString(reaz, p, "MAIL", inw)) >= 0) {
	    q = strlen(inw);
	    /* if (!strnistr(inw, "FIDO", q) && !strnistr(inw, "NETMAIL", q)
				    && !strnistr(inw, "LOCAL", q)) */
		return p;
	}
	return -1;
    }
    if ((p = LookForString(reaz, -1, "NETMAIL", inw)) >= 0)
	return p;
    do {
	if ((p = LookForString(reaz, p, "NET", inw)) >= 0 ||
			(pick = LookForString(reaz, p, "MAIL", inw)) >= 0)
	    q = strlen(inw);
	    if (!strnistr(inw, "INTERNET", q) && !strnistr(inw, "USENET", q))
		return p;
    } while (p >= 0);
    if (p = LookForString(reaz, -1, "MATRIX", inw))
	return p;
    return LookForString(reaz, -1, "MAIL", inw);
}


bool Do1ListAreaIDCMP(struct IntuiMessage *im)
{
    short gid;
    char k;

    bgustifle = false;
    switch (im->Class) {
      case IDCMP_CLOSEWINDOW:
      case IDCMP_MOUSEBUTTONS:
      case IDCMP_NEWSIZE:
      case IDCMP_ACTIVEWINDOW:
      case IDCMP_MOUSEMOVE:
	return BasicListIDCMP(im);
      case IDCMP_RAWKEY:
	k = KeyToAscii(im->Code, im->Qualifier);
	if (k & 0x80 || k <= ' ' || k == '`' || k == '~')
	    return BasicListIDCMP(im);
	else if (k == 'U' || !(im->Qualifier & ALTKEYS))
	    switch (k) {
	      case 'A':
		if (addropping)
		    AreaAddDrop(listcc, true);
		else if (emptyairing)
		    ToggleInUseify();
		else if (packeting)
		    return realrequester = true;
		else if (!airing)
		    return alswitch = true;
		break;
	      case 'L':
		if (!packeting && !emptyairing && airing)
		    return alswitch = true;
		break;
	      case 'D':
	      case 127:  /* Del */
		if (packeting) {
		    FileDelOption(listcc->messes[pick]);
		    if (wasescaped)
			return true;
		} else if (addropping && k == 'D')
		    AreaAddDrop(listcc, false);
		break;
	      case 'C':
		if (qwk && emptyairing | addropping)
		    return newareawin = wasescaped = true;
		break;
	      case 'N':
		if (nonegag)
		    return localnone = true;
		break;
	      case 'R':
		if (addropping)
		    ResetAddsAndDrops(listcc);
		break;
	      case 'S':
		SearchInList(listcc);
		return wasescaped | alswitch;
	      case 'U':
		UnSearchList(listcc, im->Qualifier & ALTKEYS);
		break;
	      case '[':
	      case ']':
		if (airing) {
		    if (k == '[')
			UpListLine(listcc);
		    else
			DownListLine(listcc);
		} else if (!packeting)
		    return MsgListNewArea(listcc, k == '[');
		break;
	    }
	break;
      case IDCMP_GADGETDOWN:
      case IDCMP_GADGETUP:
	gid = ((struct Gadget *) im->IAddress)->GadgetID;
	if (gid >= 10)
	    return BasicListIDCMP(im);
	else if (gid == 4 && airing && !packeting) {
	    if (qwk && emptyairing | addropping)
		newareawin = wasescaped = true;
	    else
		alswitch = true;
	    return true;
	} else if (gid == 1) {
	    if (nonegag)
		localnone = true;
	    else if (packeting)
		realrequester = true;
	    else if (!airing)
		alswitch = true;
	    else {
		if (addropping)
		    AreaAddDrop(listcc, true);
		else if (emptyairing)
		    ToggleInUseify();
		return wasescaped;
	    }
	    return true;
	} else if (gid == 2) {
	    SearchInList(listcc);
	    return wasescaped | alswitch;
	} else if (addropping) {
	    if (gid == 5)
		AreaAddDrop(listcc, false);
	    else if (gid == 3)
		ResetAddsAndDrops(listcc);
	} else if (packeting && gid == 4) 
	    FileDelOption(listcc->messes[pick]);
	break;
      case IDCMP_INTUITICKS:
	break;
    }
    BGupdate(listcc);
    return wasescaped;
}


short ListMessages(struct Conf *cc, short defaultm)
{
    ushort ogm = FlipBGadgets(0);

    StopScroll();
    if (!alswitch) {
	oldwhicha = whicha;
	oldwhichm = whichm;
	ClearTempCurrents(-1);
    }
    EnterListArea(cc);
    pick = alswitch ? cc->current : defaultm;
    airing = emptyairing = packeting = wasescaped = nonegag
				= addropping = alswitch = false;
    ListLine = &MessesListLine;
    lneww.Screen = scr;
    TwiddleSlider();
    FiddleHeight(&lww);
    Titlist(cc);
    if (!(lawin = OpenShareWin(&lww))) {
	Err("Could not open window\nto list messages.");
	FlipBGadgets(ogm);
	return defaultm;
    }
    NoMenus();
    ChangeBotGagText(&readgag2, &bgt2s, false);
    FlipBGadgets(6);
    MakeLarp();
    repling = cc == &replies;
    bulling = cc == &bullstuff;
    personing = cc == &personals;
    PaintListWin(cc);
    do {
	areachanging = false;
	BGupdate(cc);
	if (waste)
	    PreloadConf(cc, pick);
	ListAreaIDCMP(cc);
	ASSERT(whicha >= 0 && whicha < readareaz.messct);
	cc = readareaz.confs[whicha];
    } while (areachanging);
    CloseShareWin(lawin);
    lawin = null;
    ChangeBotGagText(&readgag2, &bgt2w, false);
    if (alswitch) {
	FlipBGadgets(0);
	if (!prelistbits)
	    prelistbits = ogm;
    } else {
	FlipBGadgets(prelistbits ? prelistbits : ogm);
	prelistbits = 0;
    }
    if (wasescaped) {
	LeaveListArea(cc);		/* restore cc->current */
	whicha = oldwhicha;
	ASSERT(whicha < readareaz.messct);
	pick = whichm = oldwhichm;	/* Search/UnSearch keep updated */
	ASSERT(whichm < readareaz.confs[whicha]->messct);
    } else
	cc->current = pick;
    YesMenus();
    return pick;
}


short ListAreas(short initialarea, bool empties, bool addrop, bool nungag)
{
    struct Conf *reaz;
    struct WhereWin *aww;
    ushort ogm = FlipBGadgets(0), ngm, i;
    bool startalswitch = alswitch;

    StopScroll();
    nonegag = nungag;
    if (addrop) {
	aww = &atww;
	reaz = &areaz;
    } else if (empties) {
	aww = &awww;
	reaz = &areaz;
    } else {
	aww = &arww;
	reaz = &readareaz;
    }
    if (!(empties | addrop)) {
	if (startalswitch)
	    LeaveListArea(reaz->confs[whicha]);
	else {
	    ClearTempCurrents(-1);
	    oldwhicha = whicha;
	    oldwhichm = whichm;
	}
    }
    newareawin = packeting = wasescaped = alswitch
				= inuseified = localnone = false;
    airing = true;
    emptyairing = empties;
    addropping = addrop;
    ListLine = &AreasListLine;
    aww->nw->Screen = scr;
    TwiddleSlider();
    FiddleHeight(aww);
    Titlist(reaz);
    if (!(lawin = OpenShareWin(aww))) {
	Err("Could not open window to\nlist message areas.");
	FlipBGadgets(ogm);
	return initialarea;
    }
    if (empties) {
	ChangeBotGagText(&readgag4, &bgt4new, false);
	ChangeBotGagText(&readgag1, nungag ? &bgt1none : &bgt1active, false);
    } else if (addrop) {
	ChangeBotGagText(&readgag1, &bgt1add, false);
	ChangeBotGagText(&readgag3, &bgt3reset, false);
	ChangeBotGagText(&readgag4, &bgt4new, false);
	readgag5.GadgetRender = &upborder;
	ChangeBotGagText(&readgag5, &bgt5drop, false);
    }
    ChangeBotGagText(&readgag2, &bgt2s, false);
    NoMenus();
    MakeLarp();
    if (!empties)
	for (i = 0; i < readareaz.messct; i++)
	    CheckAnyAllRead(readareaz.confs[i]);
    if ((pick = initialarea) < 0 &&
			(pick = LookForNetmailStrings(reaz, pick < -1)) < 0)
	pick = 0;
    PaintListWin(reaz);
    ngm = 4;				/* "Search" */
    if (qwk || !(empties | addrop))
	ngm |= 0x10;			/* "Create" or "List" */
    if (empties && (inuseareaz.confs || nungag))
	ngm |= 2;			/* "Active"/"All" or "None" */
    if (addrop) {
	ngm |= addsdrops ? 0x2A : 0x22;	/* "Add" & "Drop" & maybe "Reset" */
	addropmask = ngm & ~8;		/* not including Reset */
    }
    FlipBGadgets(ngm);
    ListAreaIDCMP(reaz);
    CloseShareWin(lawin);
    lawin = null;
    ChangeBotGagText(&readgag4, &bgt4, false);
    ChangeBotGagText(&readgag2, &bgt2w, false);
    ChangeBotGagText(&readgag1, &bgt1, false);
    if (addrop) {
	ChangeBotGagText(&readgag5, &bgt5, false);
	readgag5.GadgetRender = &upborderleft;
	FixGreenGadMaybe(&readgag5, bgwin, gagrow);	/* erase mess */
	ChangeBotGagText(&readgag3, (mirep2.ItemFill == &itrep2R
					? &bgt3re : &bgt3r), false);
    }
    if (alswitch) {
	FlipBGadgets(0);
	if (!prelistbits)
	    prelistbits = ogm;
    } else {
	if (startalswitch && !wasescaped && !(empties | addrop)
				&& pick == whicha && whicha == initialarea)
	    EnterListArea(reaz->confs[whicha]);
	FlipBGadgets(prelistbits ? prelistbits : ogm);
	prelistbits = 0;
    }
    YesMenus();
    if (empties | addrop) {
	if (inuseified)
	    ToggleInUseify();
	areaz.current = pick;
	UnSearch(&areaz, false);		/* also gets inuseareaz */
	pick = areaz.current;
    } else if (wasescaped) {
	LeaveListArea(readareaz.confs[pick]);	/* restore cc->current */
	whicha = oldwhicha;
	whichm = oldwhichm;		/* Search/UnSearch keep updated */
    } /* else
	initialarea = whicha; */
    return wasescaped ? initialarea : (localnone ? -1 : pick);
}


short ListFiles(short start)
{
    StopScroll();
    realrequester = wasescaped = airing = emptyairing
				= addropping = nonegag = false;
    packeting = true;
    ListLine = &FilesListLine;
    fneww.Screen = scr;
    TwiddleSlider();
    FiddleHeight(&fww);
    Titlist(&filez);
    if (!(lawin = OpenShareWin((adr) &fww))) {
	Err("Could not open window to list\npackets in the %s directory.",
				dirname[fakery ? DBBS : DDOWN]);
	return start;
    }
    NoMenus();
    MakeLarp();
    if ((pick = start) < 0)
	pick = 0;
    PaintListWin(&filez);
    FlipBGadgets(0x16);
    ListAreaIDCMP(&filez);
    CloseShareWin(lawin);
    lawin = null;
    YesMenus();
    filez.current = pick;
    UnSearch(&filez, false);
    return (wasescaped | realrequester ? start : filez.current);
}
