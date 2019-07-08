/* buncha putrid shit */

#include <exec/execbase.h>
#include <intuition/intuition.h>
#include <intuition/sghooks.h>
#include <stdlib.h>
#include "qblue.h"
#include "pigment.h"


#define TLMGAP     0
#define TWINHEIGHT 100
#define TLONGEST   75
#define TAGEXCESS  100
#define USEDMEMORY 100
#define HALFENOUGH 250 /* thousandths */

#define TLIST_IDCMP (IDCMP_MOUSEBUTTONS | IDCMP_CLOSEWINDOW | IDCMP_RAWKEY \
			| IDCMP_NEWSIZE | IDCMP_ACTIVEWINDOW | IDCMP_GADGETUP \
			| IDCMP_GADGETDOWN | IDCMP_INTUITICKS | IDCMP_MOUSEMOVE)


void TwiddleSlider(void);
void FiddleHeight(struct WhereWin *ww);
void FixTopmess(struct Conf *cc);
void ListAllLines(struct Conf *cc);
void SetLPens(short fg, short bg);
void ListAreaIDCMP(struct Conf *cc);
bool BasicListIDCMP(struct IntuiMessage *im, struct Conf *cc);
void SearchInList(struct Conf *cc);
void UnSearchList(struct Conf *cc, ushort alted);
void Titlist(struct Conf *cc);
void MakeLarp(void);
void BGupdate(struct Conf *cc);
bool AskDeleteTagline(str tag, bool nonmanual /*, str file */ );
short AskUnsavedTaglines(ushort count);

bool ClipboardStringPaste(struct IntuiMessage *im);
void FixStringGad(struct Gadget *gg, struct Window *ww);
void FixGreenGadMaybe(struct Gadget *gg, struct Window *ww, bool rend);
ustr NewPermPoolString(ushort length);
void UnSearch(struct Conf *where, bool freshen);
bool DoFileRequest(bool saving, bool special, str result,
				str deflt, str pattern, str hail);
bool ForeignersPresent(void);
bool IsInFrontOf(struct Window *wa, struct Window *wb);
bool AllBlank(ustr line);
bool AllBlankN(ustr line, ushort len);
void UnMask(str s);
short XNominal(short x);
str streol(str s);
ustr NewMessPoolLine(struct Mess *mm, ubyte len);

short iindexn(ustr s, ushort l, char c);
#pragma regcall(iindexn(a0, d2, d1))		/* required! */


import ListLineFType ListLine;

import struct Gadget ldownarrow;
import struct IntuiText bgt3random, bgt2s, bgt1none, bgt4load, bgt5saVe;
import struct IntuiText bgt4del, itrep2R;
import struct Border upborderV;
import struct MenuItem mirep2;
import struct RastPort *larp;

import struct Mess *thing, *fixee;
import char pblanks[], listitle[], teargline[], thefilenote[];
import char taglinesfile[], localtaglinesfile[];
#ifdef REPLYPICKS
import ushort replypicks[];	/* in who.c */
import short edrepix;
#endif
import str fibuf;

import long hilicolor, undercolor;
import ushort topix, lepix, pxstall, pxswide, charz;
import ushort tagstyle, tagleadin, localtagstyle, localtagleadin;
import short linez, pick, topmess, countoff, bgustifle;
import bool packeting, airing, emptyairing, addropping, fixingreply;


struct Conf tagfaconf = { 0 };

char currentag[TLONGEST + 4];
char loadedtaglinesfile[COMMANDLEN];
str lasttagrealized = null;
bool tagging, taintag = false, opensincelasttagload = false;
long sequence, lastdeleted;

local str taglinebuffer;
local long taglinebulk, totaltagroom;
local short twbotgap = 0, rightgap, storedpick, tstyle, seqoffset;

local char originaltag[81];
local ushort lasttaglines[USEDMEMORY], changedlines;
local short lasttlco, firstappended, bumpedby;
local bool tagwinopened, yeahuseit, nunnery;


struct StringInfo tstrstr = STRINF(currentag, TLONGEST + 1);

STRINGBORDER(manualtagbox)

struct Gadget tgagstr = {
    null, 11, -20, -35, 8, GFLG_STRINGEXTEND | GFLG_RELBOTTOM | GFLG_RELWIDTH,
    GACT_RELVERIFY | GACT_STRINGLEFT, GTYP_STRGADGET,
    /* &manualtagbox */ null, null, null, 0, &tstrstr, 2100, null
};


short tagbignums[4] = { 0, 3, 640, 197 };

struct {
    long t;
    short *a;
    long e, epad;
} tagtags = { WA_Zoom, tagbignums, TAG_DONE, 0 };

struct ExtNewWindow tneww = {
    0, 20, 640, TWINHEIGHT, 0, 1, TLIST_IDCMP,
    WFLG_SMART_REFRESH | WFLG_CLOSEGADGET | WFLG_DEPTHGADGET | WFLG_SIZEGADGET
		| WFLG_DRAGBAR | WFLG_ACTIVATE | WFLG_NW_EXTENDED,
    &ldownarrow, null, listitle, null, null,
    640, 64, 640, 9999, CUSTOMSCREEN, (adr) &tagtags
};

struct WhereWin tww = { &tneww };


int strcmp_dent(register str a, register str b)
{
    while (*a == ' ') a++;
    while (*b == ' ') b++;
    return strcmp(a, b);
}


local short LengthFoundIn(str wheretolook, str whattofind)
{
    short len = strlen(wheretolook), span, maxspan = 0;

    while (len > 0) {
	if ((span = iindexn(wheretolook, len, *whattofind)) < 0)
	    return maxspan;
	wheretolook += span;
	len -= span;
	for (span = 1; toupper(wheretolook[span]) == toupper(whattofind[span])
					&& span < len; span++) ;
	if (span > maxspan)
	    maxspan = span;
	wheretolook++;
	len--;
    }
    return maxspan;
}


bool ALotInCommon(str oa, str ob)
{
    register str a = oa, b = ob;
    bool halfenough = false;
    short goalen = strlen(a), foundlen = strlen(b);

    if (foundlen > goalen) {
	goalen = foundlen;
	a = ob;
	b = oa;
    }
    while (*b) {
	if (!(foundlen = LengthFoundIn(a, b)))
	    b++;
	else {
	    short milage = 1000 * foundlen / goalen;
	    if (milage >= HALFENOUGH * 2)
		return true;
	    else if (milage >= HALFENOUGH) {
		if (halfenough)
		    return true;
		else
		    halfenough = true;
	    }
	    b += foundlen;
	}
    }
    return false;
}
/* Yes this could be way more efficient... if I spent 20x as much time on it. */


void SeedRandom(void)
{
    import struct ExecBase *SysBase;
    struct DateStamp d;

    DateStamp(&d);
    srand(d.ds_Days ^ (d.ds_Minute << 8) ^ (d.ds_Tick << 4)
				^ (long) SysBase->ThisTask ^ SysBase->Elapsed);
}


ulong Random(ushort choices)	/* choices should be > 1 */
{
    ulong raw = ((ulong) rand() << 15) + rand();
    return choices ? raw % choices : 0;
}


local str ExpandTagline(str raw)   /* returns pointer to static temp area! */
{
    static char buf[TLONGEST + 9];
    str p, end;
    short i = 0;

    end = streol(raw);
    for (p = raw; p < end && i <= TLONGEST; p++)
	if (*p == '\t')
	    do
		buf[i++] = ' ';
	    while (i & 7);
	else
	    buf[i++] = *p;
    buf[i] = buf[TLONGEST] = 0;
    Stranks(buf);
    return buf;
}


bool ExtractTagline(short p, str line)
{
    str lin = (str) tagfaconf.messes[p], ttp;

    if (!lin || p < 0 || p >= tagfaconf.messct) {
	line[0] = 0;
	return true;
    }
    ttp = ExpandTagline(lin);
    if (strcmp_dent(ttp, line)) {
	strcpy(line, ttp);
	return true;
    } else
	return false;
}


void RealizeTagline(void)
{
    short w = (lawin->Width + tgagstr.Width) / fontwid - 1;
    long p;

    if (!ExtractTagline(pick, currentag))	/* avoid wasted display work */
	return;
    p = RemoveGadget(lawin, &tgagstr);
    tstrstr.DispPos = 0;
    if ((tstrstr.BufferPos = strlen(currentag)) > w)
	tstrstr.BufferPos = w;
    AddGadget(lawin, &tgagstr, p);
    RefreshGList(&tgagstr, lawin, null, 1);
    lasttagrealized = (str) tagfaconf.messes[pick];
}


void List1Tagline(short line, struct Mess *mm, bool hilite)
{
    str xl = ExpandTagline((str) mm);
    short l = strlen(xl);
    long top = topix + line * fight, back = hilite ? hilicolor : backcolor;

    if (line < 0 || line >= linez)
	return;
    if (l > charz)
	l = charz;
    SetLPens(fourcolors & hilite ? 0 : undercolor, back);
    Move(larp, lepix + TLMGAP, top + font->tf_Baseline);
    Text(larp, xl, l);
    if (l < charz)
	Text(larp, pblanks, charz - l);
    if (rightgap) {
	SetLPens(back, -1);
	RectFill(larp, pxswide + lepix - rightgap, top,
				pxswide + lepix - 1, top + fight - 1);
    }
}


void PaintTaglineWin(void)
{
    ulong redge, bedge, gedge, p;

    topix = lawin->BorderTop + 1;
    lepix = 5;
    pxstall = lawin->Height - topix - twbotgap - 2;
    pxswide = lawin->Width - lawin->BorderRight - lepix - 1;
    redge = pxswide + lepix;
    linez = pxstall / fight;
    charz = (pxswide - TLMGAP) / fontwid;
    rightgap = (pxswide - TLMGAP) - charz * fontwid;
    SetLPens(0, 0);
    Move(larp, lepix, lawin->Height - 3);
    Draw(larp, redge, lawin->Height - 3);
    Draw(larp, redge, topix);
    gedge = lawin->Height + tgagstr.TopEdge - 5;
    Move(larp, lepix, gedge);
    Draw(larp, redge, gedge);
    bedge = topix + fight * linez;
    gedge--;
    if (gedge >= bedge) {
	SetLPens(backcolor, -1);
	RectFill(larp, lepix, bedge, redge - 1, gedge);
    }
    FixTopmess(&tagfaconf);
    p = RemoveGadget(lawin, &tgagstr);
    tgagstr.GadgetRender = &manualtagbox;	/* temporarily only! */
    FixStringGad(&tgagstr, lawin);
    tgagstr.GadgetRender = null;	/* prevent bordersniff refresh! */
    AddGadget(lawin, &tgagstr, p);
/*    DrawBorder(larp, tgagstr.GadgetRender, tgagstr.LeftEdge,
					lawin->Height + tgagstr.TopEdge - 1); */
    ListAllLines(&tagfaconf);
}


bool SaveLongerTaglineFile(bool urban_renewal)
{
    BPTR hand;
    long i;
    str piece = taglinebuffer, end = taglinebuffer + taglinebulk, atlf;
    ushort tc = tagfaconf.messct;
    ustr *tfl = (ustr *) tagfaconf.messes;

    if (tagfaconf.unfiltered) {
	tc = tagfaconf.unfiltered->messct;
	tfl = (ustr *) tagfaconf.unfiltered->messes;
    }
    if (!tagfaconf.messct || !DoFileRequest(true, true, loadedtaglinesfile,
				null, null, "Select where to save taglines"))
	return false;
    if (!(hand = NOpen(loadedtaglinesfile))) {
	DosErr("Could not create taglines\nfile %s", loadedtaglinesfile);
	return false;
    }
    while (piece < end) {
	i = strlen(piece);
	if (Write(hand, piece, i) < i) {
	    DosErr("Could not write taglines to\nfile %s", loadedtaglinesfile);
	    Close(hand);
	    return false;
	}
	piece += i;
	while (piece < end && !*piece)		/* skip deleted lines */
	    piece++;
    }
    for (i = firstappended; i < tc; i++) {      /* added lines */
	str lin = ExpandTagline(tfl[i]);
	long ll = strlen(lin);
	strcpy(lin + ll++, "\n");
	if (Write(hand, lin, ll) < ll) {
	    DosErr("Could not write added taglines\nto file %s",
					loadedtaglinesfile);
	    Close(hand);
	    return false;
	}
    }
    if (!Close(hand)) {
	DosErr("Apparent failure writing taglines\nfile %s",
					loadedtaglinesfile);
	return false;
    }
    atlf = localtaglinesfile[0] ? localtaglinesfile : taglinesfile;
    if (!urban_renewal && stricmp(loadedtaglinesfile, atlf))
	strcpy( /* atlf */ localtaglinesfile, loadedtaglinesfile);
    seqoffset = changedlines = 0;
    if (tstyle == 2 /*Sequence*/) {
	sprintf(thefilenote, "#%ld", sequence);
	SetComment(loadedtaglinesfile, thefilenote);
    }
    bumpedby = -1;
    return true;
}


bool FreeTaglines(void)
{
    if (changedlines && tagfaconf.messct) {
	short r = AskUnsavedTaglines(changedlines);
	if (!r || (r > 1 && !SaveLongerTaglineFile(true)))
	    return false;
    }
    UnSearch(&tagfaconf, false);
    Vfree(taglinebuffer);
    /* added lines get freed at program exit by FreePool(stringPermPool) */
    tagfaconf.messes = (adr) taglinebuffer = null;
    loadedtaglinesfile[0] = tagfaconf.messct = firstappended = 0;
    bumpedby = -1;
    return true;
}


bool MakeTagRoom(void)
{
    struct Mess **new;
    short t = totaltagroom + TAGEXCESS;

    if (tagfaconf.messct < totaltagroom)
	return true;
    if (totaltagroom >= 32767)
	return false;
    if (t > 32767)
	t = 32767;
    if (!(new = Valloc(t * 4)))
	return false;
    memcpy(new, tagfaconf.messes, 4 * totaltagroom);
    Vfree(tagfaconf.messes);
    tagfaconf.messes = new;
    totaltagroom = t;
    return true;
}


local void GatherTaglines(void)
{
    str p, lp, tp;
    short ll, lls, added = tagfaconf.messct - firstappended, newfa;

    lp = taglinebuffer;
    tagfaconf.messct = 0;
    while (tagfaconf.messct < totaltagroom - added || MakeTagRoom()) {
	p = streol(lp);
	ll = lls = 0;
	if (*lp != ';')
	    for (tp = lp; tp < p; tp++)
		if (*tp == '\t')
		    lls += 8 - ((ll + lls) & 7);   /* allow for tab expansion */
		else if (*tp == ' ')
		    lls++;			   /* defer adding blanks */
		else
		    ll += lls + 1, lls = 0;
	if (ll <= TLONGEST && ll > 0 /* && !AllBlankN(lp, ll) */ ) {
	    if (tagfaconf.messct == firstappended && added) {
		newfa = totaltagroom - added;
		memcpy(tagfaconf.messes + newfa,
				tagfaconf.messes + firstappended, added * 4);
		firstappended = newfa;
	    }
	    tagfaconf.messes[tagfaconf.messct++] = (adr) lp;
	}
	lp = p + 1;
	tp = taglinebuffer + taglinebulk;
	while (!*p && p < tp) p++;		/* skip erased lines */
	if (p >= tp)
	    break;
    }
    if (firstappended > tagfaconf.messct && added)
	memcpy(tagfaconf.messes + tagfaconf.messct,
				tagfaconf.messes + firstappended, added * 4);
    firstappended = tagfaconf.messct;
    tagfaconf.messct += added;
}


void DelTagline(str nonmanualtag)
{
    str zilch = (str) tagfaconf.messes[pick], zotch;
    struct Conf *tuf;
    ushort tfp, m;

    if (!AskDeleteTagline(nonmanualtag ? nonmanualtag : currentag,
				!!nonmanualtag /*, loadedtaglinesfile */ ))
	return;
    if (tuf = tagfaconf.unfiltered) {
	tfp = tuf->messct;
	for (m = 0; m < tuf->messct; m++)
	    if ((str) tuf->messes[m] == zilch) {
		tfp = m;
		break;
	    }
	if (tfp < tuf->messct) {
	    if (tfp < --tuf->messct)
		memmove(tuf->messes + tfp, tuf->messes + tfp + 1,
						4 * (tuf->messct - tfp));
	    /* ASSERT if !tfp [list emptied] then taglines freed below */
	}
    } else
	tfp = pick;
    if (pick < --tagfaconf.messct)
	memmove(tagfaconf.messes + pick, tagfaconf.messes + pick + 1,
						4 * (tagfaconf.messct - pick));
    else if (pick)
	pick--;
    else {
	if (tuf)			/* list has been made empty */
	    UnSearch(&tagfaconf, false);
	if (!tagfaconf.messct)
	    FreeTaglines();
	wasescaped = true;		/* close window */
	return;
    }
    if (tfp < sequence)
	sequence--;
    seqoffset++;
    lastdeleted = tfp;
    if (tfp < firstappended) {
	firstappended--;
	zotch = streol(zilch);
	while (zilch <= zotch)
	    *(zilch++) = 0;		/* ooooooooo-hoo-hoo-hoo-hoo wipeout! */
    }
    if (!nonmanualtag) {
	if (pick < topmess || tagfaconf.messct - topmess < linez)
	    FixTopmess(&tagfaconf);
	ListAllLines(&tagfaconf);
	RealizeTagline();
    }
    changedlines++;
    bumpedby = -1;
}


bool LoadTaglineFile(str path, bool silent)
{
    str p;

    if (taglinebuffer && !FreeTaglines())
	return false;
    seqoffset = changedlines = 0;
    if (!(taglinebuffer = InhaleFile(path))) {
	if (!silent)
	    Err("Could not load tagline\nfile %s", path);
	return false;
    }
    taglinebulk = fibize;
    fibuf = null;		/* pretend we ExhaleFile()d */
    sequence = thefilenote[0] == '#' ? atol(thefilenote + 1) : -1;
    totaltagroom = TAGEXCESS;
    for (p = taglinebuffer; p; p = strchr(p + 1, '\n'))
	totaltagroom++;
    if (totaltagroom > 32767)
	totaltagroom = 32767;
    if (!(tagfaconf.messes = Valloc(totaltagroom << 2))) {
	FreeTaglines();
	Err("Out of memory -- could not load\ntagline file %s", path);
	return false;
    }
    lasttlco = 0;
    GatherTaglines();
    if (!tagfaconf.messct) {
	Err("No valid taglines found\nin file %s", path);
	FreeTaglines();
	return false;
    }
    opensincelasttagload = false;
    strcpy(loadedtaglinesfile, path);
    bumpedby = -1;
    return true;
}


bool AppendNewTagline(str line, bool updating)
{
    short l, p;
    str new;
    char temptag[80];

    for (new = line; *new; new++)
	if (isspace((ubyte) *new))
	    *new = ' ';			/* no tab expansion here */
    Stranks(line);
    l = strlen(new = line);
    if (AllBlankN(line, l))
	return false;
    while (*new == ' ') new++;
    for (p = 0; p < tagfaconf.messct; p++) {
	register str tl = (str) tagfaconf.messes[p];
	while (*tl == ' ') tl++;
	if (*tl == *new && tl[1] == new[1]) {
	    temptag[0] = 0;
	    ExtractTagline(p, tl = temptag);
	    if (!strcmp_dent(tl, new)) {
		pick = p;
		if (line != currentag)
		    strcpy(currentag, line);
		return true;
	    }
	}
    }
    if (tagfaconf.unfiltered) {		/* if not in searched set, unsearch */
	UnSearch(&tagfaconf, false);
	return AppendNewTagline(line, updating);
    }
    if (updating) {		/* new line is not an exact match anywhere */
	temptag[0] = 0;
	ExtractTagline(pick, temptag);
	if (ALotInCommon(line, temptag))
	    DelTagline(temptag);
/* !!!! Someday replace the tagline IN PLACE instead of at the end! ****/
    }
    new = NewPermPoolString(l);
    if (!new || !MakeTagRoom()) {
	Err("No memory for storing\ntagline in list.");
	RealizeTagline();
	return false;
    }
    strcpy(new, line);
    /*****  ANY SPECIAL HANDLING OF ADDING A ";[...]" LINE GOES HERE */
    pick = tagfaconf.messct;
    tagfaconf.messes[tagfaconf.messct++] = (adr) new;
    if (updating) {		/* user edited string gadget  */
	bumpedby = pick;
	changedlines++;
    }
    return true;
}


void NoteUsedTagline(short p)
{
    short tlcolim = tagfaconf.messct / 2;

    if (lasttlco && lasttaglines[lasttlco - 1] == p)
	return;
    if (tlcolim >= USEDMEMORY)
	tlcolim = USEDMEMORY - 1;
    if (lasttlco >= tlcolim) {
	memmove(lasttaglines, lasttaglines + 2, tlcolim * 2);
	lasttlco--;
    }
    lasttaglines[lasttlco++] = p;
}


short PickRandomTagline(bool really)	/* returns index in tagfaconf.messes */
{
    short i, p, c = 0;
    if (really || tstyle != 2 /* Sequence */) {
	if (tagfaconf.unfiltered)
	    return Random(tagfaconf.messct);
	else
	    do {
		p = Random(tagfaconf.messct);
		for (i = 0; i < lasttlco; i++)
		    if (lasttaglines[i] == p)
			break;
	    } while (i < lasttlco && ++c < 50);
    } else {
	ASSERT(!lawin && !tagfaconf.unfiltered);
	p = (sequence + 1) % /* tagfaconf.messct */ firstappended;
    }
    NoteUsedTagline(p);
    return p;
}


bool PickTheRightPick(str existing)
{
    static str copyexisting;

    if (existing)
	copyexisting = existing;
    else
	existing = copyexisting;
    if (existing[0]) {
	if (!AppendNewTagline(existing, false))		/* sets pick */
	    return false;
    } else
	pick = PickRandomTagline(false);
    return true;
}


bool LoadSomeTaglines(str loadthis, bool *cancellate)
{
    bool dummy;
    str tlf = loadthis ? loadthis : (localtaglinesfile[0]
					? localtaglinesfile : taglinesfile);

    if (cancellate)
	*cancellate = false;
    else
	cancellate = &dummy;	/* VVV react to change of BBS local setup */
    if (opensincelasttagload && (loadthis || stricmp(loadedtaglinesfile, tlf)))
	if (*cancellate = !FreeTaglines())
	    return false;
    if (taglinebuffer)
	return true;
    return tlf[0] && LoadTaglineFile(tlf, !loadthis);
}


bool PickNewTaglineFile(bool bombifcancel)
{
    char tagfiletoload[COMMANDLEN];
    str orig = localtaglinesfile[0] ? localtaglinesfile : taglinesfile;

    if (!DoFileRequest(false, true, tagfiletoload, loadedtaglinesfile[0]
					? loadedtaglinesfile : orig, null,
					"Select a tagline file to load"))
	return bombifcancel;
    opensincelasttagload = true;
    if (LoadSomeTaglines(tagfiletoload, null)) {
	if (stricmp(tagfiletoload, orig))
	    strcpy(localtaglinesfile, tagfiletoload);
	if (!lawin)
	    return false;
	if (!PickTheRightPick(null))
	    pick = PickRandomTagline(false);
	FixTopmess(&tagfaconf);
	ListAllLines(&tagfaconf);
	RealizeTagline();
	return false;
    } else
	return !tagfaconf.messct;	/* old taglines may still be loaded */
}


bool RemoveTaglineFromReply(struct Mess *mm, short whichreply, str dump)
{
    ustr p, pp;
    short l;

    if (mm == thing && !mm->linect && fixingreply && dump)
	mm = fixee;
    if (!(mm->bits & REPLY_HAS_TAG) || !mm->linect)
	return false;
    p = mm->lines[mm->linect - 1];
    if (lch(p, " * Q-Blue") || lch(p, "... ")) {
	l = p[-1];
	if (p[1] == '*') {
	    if (!(pp = strnchr(p + 2, '*', l - 2)))
		return false;
	    if (*++pp == ' ' && (l -= pp - p) > 0) {
		pp++;
		l--;
		if (l >= 2 && *pp == ' ' && pp[1] != ' ')
		    pp++, l--;
	    }
	} else
	    pp = p + 4, l -= 4;
	if (AllBlankN(pp, l))
	    return false;
	if (dump)
	    strncpy0(dump, pp, l);
	if (whichreply >= 0) {		/* otherwise don't physically remove */
#ifndef DONT_USE_LINEPOOL
	    if (--mm->linect && AllBlank(mm->lines[mm->linect - 1]))
		--mm->linect;	/* memory is wasted until message is freed */
#else
	    Vfree(p);
	    if (--mm->linect && AllBlank(mm->lines[mm->linect - 1]))
		Vfree(mm->lines[--mm->linect]);
#endif
	    mm->bits &= ~REPLY_HAS_TAG;
#ifdef REPLYPICKS
	    replypicks[whichreply] = 0;
#endif
	}
	return true;
    }
    return false;
}


bool AttachTaglineToReply(struct Mess *mm, short whichrep, str line)
{
    short /* lim = TLLimit(), */ l = strlen(line), tl;
    char buf[82];
    ustr pp;

    if (taintag || !(mm->bits & LOADED))
	return false;
    RemoveTaglineFromReply(mm, whichrep, null);
    if (!*line)			/* leave old line in place?  NO. */
	return false;
    if (!mm->lines || (ulong) mm->lines[-1] < 4 * (mm->linect + 2)) {
	Err("Cannot attach tagline; there was no spare\n"
			"memory when the reply was loaded.");
	return false;
    }
/*  if (l > lim)
	l = lim;	*/
    if (qwk && (localtagleadin == 2 ? tagleadin : localtagleadin) == 1) {
	strcpy(buf, teargline + 1);
	UnMask(buf);
	tl = l + strlen(buf);
	if (tl > TLONGEST + 3)
	    strcpy(buf, "... ");
	else if (tl <= TLONGEST + 1 && !isspace(*line))
	    strcat(buf, "  ");
	else
	    strcat(buf, " ");
    } else
	strcpy(buf, "... ");
    pp = strend(buf);
    strncpy(pp, line, l);
    pp[l] = 0;
    l = strlen(buf);
#ifndef DONT_USE_LINEPOOL
    if (!(pp = NewMessPoolLine(mm, l))) {
#else
    if (!(pp = BValloc(l))) {
#endif
	Err("Cannot attach tagline\nto reply; no memory.");
	return false;
    }
    strncpy(pp, buf, l);
    mm->linetypes[mm->linect] = mm->linetypes[mm->linect + 1] = TRASHTYPE;
    mm->lines[mm->linect] = null;
    mm->lines[mm->linect + 1] = pp;
    mm->linect += 2;
    mm->bits |= REPLY_HAS_TAG;
    if (line == currentag && storedpick < firstappended) {
	ASSERT(!tagfaconf.unfiltered);
	sequence = storedpick;
	if (tstyle == 2 /*Sequence*/ && loadedtaglinesfile[0]) {
	    long ss = sequence;
/*** The following is just an approximation of how to compensate for deleted  */
/*** taglines when saving the sequence number.  It works completely when all  */
/*** lines deleted are adjacent, and works often when all are before or all   */
/*** are after the sequence point, and is off when it's between some of them. */
	    if (seqoffset && ss >= lastdeleted)
		ss += seqoffset;
	    sprintf(thefilenote, "#%ld", ss);
	    SetComment(loadedtaglinesfile, thefilenote);
	}
    }
    return true;
}


bool Do1TaglineIDCMP(struct IntuiMessage *im)
{
    short gid;
    char k;
    bool r, rami = !!(im->Qualifier & IEQUALIFIER_RCOMMAND);

    bgustifle = false;
    if (im->Class == IDCMP_RAWKEY && !(im->Qualifier & ALTKEYS)) {
	if (rami)
	    im->Qualifier = 0;		/* for ramiga-V paste */
	k = KeyToAscii(im->Code, im->Qualifier);
	switch (k) {
	  case '\t':
	    ActivateGag(&tgagstr, lawin);
	    return false;
	  case 'R':
	    pick = PickRandomTagline(true);		/* always random */
	    FixTopmess(&tagfaconf);
	    ListAllLines(&tagfaconf);
	    RealizeTagline();
	    return false;
	  case 'N':
	    currentag[0] = 0;
	    wasescaped = false;
	    return nunnery = yeahuseit = true;
	  case 'D':
	  case 127:  /* Del */
	    DelTagline(null);
	    return wasescaped;
	  case 'S':
	    SearchInList(&tagfaconf);
	    return wasescaped;
	  case 'U':
	    UnSearchList(&tagfaconf, false);
	    return wasescaped;
	  case 'L':
	    return PickNewTaglineFile(false);
	  case 'V':
	    if (rami) {
		/* simulate ramiga-V inside string gadget: */
		im->Class = IDCMP_GADGETUP;
		im->Code = 'V';
		im->IAddress = (adr) &tgagstr;
		im->IDCMPWindow = lawin;
		strcpy(undospace, currentag);
		if (!(lawin->Flags & WFLG_WINDOWACTIVE)) {
		    ActivateWindow(lawin);
		    Delay(7);
		}
		ClipboardStringPaste(im);
	    } else
		SaveLongerTaglineFile(false);
	    return false;
	}
    } else if (im->Class == IDCMP_GADGETUP) {
	gid = ((struct Gadget *) im->IAddress)->GadgetID;
	switch (gid) {
	  case 0:					/* Delete */
	    DelTagline(null);
	    return wasescaped;
	  case 1:					/* None */
	    currentag[0] = 0;
	    wasescaped = false;
	    return nunnery = yeahuseit = true;
	  case 2:					/* Search */
	    SearchInList(&tagfaconf);
	    return wasescaped;
	  case 3:					/* Random */
	    pick = PickRandomTagline(true);		/* always random */
	    FixTopmess(&tagfaconf);
	    ListAllLines(&tagfaconf);
	    RealizeTagline();
	    return false;
	  case 4:					/* Load */
	    return PickNewTaglineFile(false);
	  case 5:					/* saVe */
	    SaveLongerTaglineFile(false);
	    return false;
	  case 2100:					/* tagline string */
	    /* StripString(&tgagstr, lawin); */	
	    Stranks(currentag);
	    if (currentag[0]) {
		AppendNewTagline(currentag, true);
		lasttagrealized = (str) tagfaconf.messes[pick];
		FixTopmess(&tagfaconf);
		ListAllLines(&tagfaconf);
	    } else
		return nunnery = yeahuseit = true;
	    return false;
	}
    }
    r = BasicListIDCMP(im, &tagfaconf);
    BGupdate(&tagfaconf);		/* could even use BGupdate(null)... */
    yeahuseit = !wasescaped;
    return r;
}


bool TaglineWindow(bool timid, bool maintenance)
{
    ushort ogm, i, j;
    char existing[80], oldcurrentag[80];
    struct IntuiText *bg0 = readgag0.GadgetText;	/* Close or Pack */
    bool cancellate;

    tagging = true;
    if (taintag = maintenance) {
	currentag[0] = originaltag[0] = 0;
	yeahuseit = nunnery = false;
	tstyle = localtagstyle == 4 ? tagstyle : localtagstyle;
    }
    wasescaped = packeting = airing = emptyairing = addropping = false;
    tgagstr.TopEdge = -5 - fight;
    tstrstr.MaxChars = i = /* TLLimit() */ TLONGEST + 1;
    while ((j = i * fontwid) > scr->Width - 35)
	i--;
    tneww.Width = tneww.MaxWidth = tagbignums[2] = j + 35;
    tneww.MinWidth = 40 * fontwid + 35;
    if ((tneww.LeftEdge = 320 - tneww.Width / 2) < 0)
	tneww.LeftEdge = 0;
    tneww.Screen = scr;
    twbotgap = fight + 8;
    ListLine = &List1Tagline;
    TwiddleSlider();
    FiddleHeight(&tww);
    Titlist(&tagfaconf);

    if (!LoadSomeTaglines(null, &cancellate) && (cancellate
						 || PickNewTaglineFile(true)))
	return tagging = false;
/*  if (lastlimit != TLLimit())
	GatherTaglines();	*/
    if (!tagfaconf.messct)
	return tagging = false;
    strcpy(oldcurrentag, currentag);
    strcpy(existing, yeahuseit ? currentag : originaltag);
    if (!PickTheRightPick(existing))
	return tagging = false;
    ogm = FlipBGadgets(0);
    if (!(lawin = OpenShareWin(&tww))) {
	Err("Could not open tagline\nselection window.");
	FlipBGadgets(ogm);
	return tagging = false;
    }
    if (timid) {
	struct Window *front = cwin;
	if (IsInFrontOf(cbitwin, cwin))
	    front = cbitwin;
	if (front)
	    MoveWindowInFrontOf(lawin, front);
    }
    NoMenus();
    GhostCompose(true);
    MakeLarp();
/*    FixStringGad(&tgagstr, lawin);	*/	/* PaintTaglineWin does this */
    AddGadget(lawin, &tgagstr, 3);
    PaintTaglineWin();
    RealizeTagline();
    ChangeBotGagText(&readgag0, &bgt4del /* misplaced */, false);
    ChangeBotGagText(&readgag1, &bgt1none, false);
    ChangeBotGagText(&readgag2, &bgt2s, false);
    ChangeBotGagText(&readgag3, &bgt3random, false);
    ChangeBotGagText(&readgag4, &bgt4load, false);
    readgag5.GadgetRender = &upborderV;
    ChangeBotGagText(&readgag5, &bgt5saVe, false);
    FlipBGadgets(0x3F);

    ListAreaIDCMP(&tagfaconf);
    RemoveGadget(lawin, &tgagstr);
    CloseShareWin(lawin);
    lawin = null;
    ChangeBotGagText(&readgag5, &bgt5, false);
    readgag5.GadgetRender = &upborderleft;
    FixGreenGadMaybe(&readgag5, bgwin, gagrow);		/* erase mess */
    ChangeBotGagText(&readgag4, &bgt4, false);
    ChangeBotGagText(&readgag3, (mirep2.ItemFill == &itrep2R
					? &bgt3re : &bgt3r), false);
    ChangeBotGagText(&readgag2, &bgt2w, false);
    ChangeBotGagText(&readgag1, &bgt1, false);
    ChangeBotGagText(&readgag0, bg0, false);
    FlipBGadgets(ogm);
    YesMenus();
    GhostCompose(false);
    tagfaconf.current = pick;
    UnSearch(&tagfaconf, false);
    pick = tagfaconf.current;
    twbotgap = 0;
    tagging = taintag = false;
    tagwinopened = true;
    lasttagrealized = null;
    if (wasescaped) {
	strcpy(currentag, oldcurrentag);
	return false;
    }
    if (!maintenance)
	NoteUsedTagline(storedpick = pick);
    return true;
}


void GiveItATaglineMaybe(void)
{
    short p, q = countoff;

    tstyle = localtagstyle == 4 ? tagstyle : localtagstyle;
    if (!tstyle || fixingreply || thing->bits & REPLY_HAS_TAG)
	return;
    if (currentag[0] || nunnery)		/* already picked one */
	return;
    if (tstyle == 3) {				/* Manual */
	while (countoff > 0)
	    PortOn();
	if (LoadSomeTaglines(null, null))	/* avoid forcing load req */
	    TaglineWindow(ForeignersPresent(), false);
	while (countoff < q)
	    PortOff();
    } else if (LoadSomeTaglines(null, null)) {	/* Random / Sequence */
/*	if (lastlimit != TLLimit())
	    GatherTaglines();		*/
	if (tagfaconf.messct) {
	    p = PickRandomTagline(false);	/* random or sequential */
	    ExtractTagline(storedpick = p, currentag);
	    yeahuseit = !!currentag[0];
	}
    }
}


bool InitializeTagStuff(void)
{
    currentag[0] = originaltag[0] = 0;
    if (fixingreply)
	RemoveTaglineFromReply(fixee, -1, originaltag);
    tagwinopened = yeahuseit = nunnery = false;
    bumpedby = -1;
    return taglinesfile[0] || localtaglinesfile[0] || taglinebuffer;
}


bool FinalizeTagline(struct Mess *who, short whichreply)
{
    if (yeahuseit && bumpedby == pick && !strcmp_dent(currentag,
				ExpandTagline((str) tagfaconf.messes[pick])))
	changedlines--;
    return AttachTaglineToReply(who, whichreply,
				yeahuseit ? currentag : originaltag);
}
