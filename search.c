/* Doin' the search and sort functions in Q-Blue */

#include <intuition/sghooks.h>
#include <intuition/intuitionbase.h>
#include <exec/memory.h>
#include <stdlib.h>
#include "qblue.h"
#include "pigment.h"


#define STRINGS     4
#define BODYMARKS   4

#define ZWINWIDTH   560
#define ZWINHEIGHT  112
#define ZWORDWID    248
#define ZZLEN       256
#define ARROWLEFT   (-16)


void FixDefarrow(void);
void ShowHeader(struct Conf *cc, struct Mess *mm);
/* bool LoadMessage(struct Mess *mm, BPTR hand, short qeol, bool trim); */
bool LoadPktMessage(struct Mess *mm);
void StripMessage(struct Mess *mm);
void DoList(struct Conf *cc);
void PickArea(bool force);
void ActMenu(short menu, short item, bool enable);
bool Do1ListAreaIDCMP(struct IntuiMessage *im, struct Conf *cc);
void CloseBitsWin(void);
ulong Text2ux(str date);
void DisplayMsgRegion(ushort x1, ushort y1, ushort x2, ushort y2, short look);
void ClearTempCurrents(short which);
void LeaveListArea(struct Conf *lcc);
void CheckAnyAllRead(struct Conf *cc);
void ToggleInUseify(void);


import struct IntuitionBase *IntuitionBase;
import struct Gadget ldownarrow, luparrow;

import struct Conf tagfaconf;

import short pick, oldwhicha, oldwhichm;
import ushort sorder, tifight;

import bool showlistwin, lastfirst, bgupdate, showareawin;
import bool alswitch, airing, inuseified;


struct toker {
    ubyte bmtab[256];
    ubyte lens[ZZLEN];		/* offset 256 */
    ubyte enos[ZZLEN];		/* offset 512 */
    ustr base;			/* offset 768 */
    ushort c;
    bool front, back;
} *tok;		/**** tok[STRINGS]; AllocMem'd at startup ****/
/* Note! ZZLEN cannot be more than 256 or this structure must be changed! */
/* enos[0] is always zero; that is, base always points to first token. */

ubyte zbufz[STRINGS][ZZLEN + 2];	/* too much hassle to AllocMem */

const ulong toksize = STRINGS * sizeof(struct toker);


struct Gadget zgagnotrash = {
    null, 278, 65, CHECKWID, 13, CHECKHILITE, CHECKACTION,
    GTYP_BOOLGADGET, &checkmarkno, &checkmarkyes, null, 0, null, 718, null
};


struct Gadget zgagresearch = {
    &zgagnotrash, 16, 65, CHECKWID, 13, CHECKHILITE, CHECKACTION,
    GTYP_BOOLGADGET, &checkmarkno, &checkmarkyes, null, 0, null, 717, null
};


struct Gadget zgagbody = {
    &zgagresearch, ZWINWIDTH - 132, 65, CHECKWID, 13, CHECKHILITE, CHECKACTION,
    GTYP_BOOLGADGET, &checkmarkno, &checkmarkyes, null, 0, null, 716, null
};


struct Gadget zgagsubj = {
    &zgagbody, ZWINWIDTH - 236, 65, CHECKWID, 13, CHECKHILITE, CHECKACTION,
    GTYP_BOOLGADGET, &checkmarkno, &checkmarkyes, null, 0, null, 715, null
};


struct Gadget zgagtoo = {
    &zgagsubj, ZWINWIDTH - 340, 65, CHECKWID, 13, CHECKHILITE, CHECKACTION,
    GTYP_BOOLGADGET, &checkmarkno, &checkmarkyes, null, 0, null, 714, null
};


struct Gadget zgagfrom = {
    &zgagtoo, ZWINWIDTH - 444, 65, CHECKWID, 13, CHECKHILITE, CHECKACTION,
    GTYP_BOOLGADGET, &checkmarkno, &checkmarkyes, null, 0, null, 713, null
};


struct IntuiText zgtnowhere = {
    TEXTCOLOR, FILLCOLOR, JAM2, 12, 2, null, "Nowhere", null
};

struct Gadget zgagnowhere = {
    &zgagfrom, ZWINWIDTH - 96, 86, 80, 12, GFLG_GADGHCOMP,
    GACT_RELVERIFY, GTYP_BOOLGADGET,
    &upborder, null, &zgtnowhere, 0, null, 712, null
};


struct IntuiText zgtall = {
    TEXTCOLOR, FILLCOLOR, JAM2, 12, 2, null, "All", null
};

struct Gadget zgagall = {
    &zgagnowhere, ZWINWIDTH - 222, 86, 80, 12,
    GFLG_GADGHCOMP, GACT_RELVERIFY, GTYP_BOOLGADGET,
    &upborder, null, &zgtall, 0, null, 711, null
};


struct IntuiText zgtwindow = {
    TEXTCOLOR, FILLCOLOR, JAM2, 12, 2, null, "Window", null
};

struct IntuiText zgthere = {
    TEXTCOLOR, FILLCOLOR, JAM2, 12, 2, null, "Here", null
};

struct Gadget zgaghere = {
    &zgagall, ZWINWIDTH - 348, 86, 80, 12, GFLG_GADGHCOMP,
    GACT_RELVERIFY, GTYP_BOOLGADGET,
    &upborder, null, &zgthere, 0, null, 710, null
};


struct StringInfo zstrz[STRINGS] = {
    STRINF(zbufz[0], ZZLEN),
    STRINF(zbufz[1], ZZLEN),
    STRINF(zbufz[2], ZZLEN),
    STRINF(zbufz[3], ZZLEN)
};

/***
struct IntuiText zlabelz2 = {
    LABELCOLOR, 0, JAM1, 0, -20, null, "What else to find:", null
};
struct IntuiText zlabelz1 = {
    LABELCOLOR, 0, JAM1, 0, -20, null, "What to find:", null
};
***/

STRINGBORDER(zbox)


struct Gadget zgagz[STRINGS] = {
    {
	&zgagz[1], 16, 41, ZWORDWID, 8, GFLG_STRINGEXTEND | GFLG_TABCYCLE,
	GACT_RELVERIFY | GACT_STRINGLEFT, GTYP_STRGADGET,
	&zbox, null, null, 0, &zstrz[0], 700, null
    }, {
	&zgagz[2], 16, 41, ZWORDWID, 8,
	GFLG_STRINGEXTEND | GFLG_TABCYCLE,
	GACT_RELVERIFY | GACT_STRINGLEFT, GTYP_STRGADGET,
	&zbox, null, null, 0, &zstrz[1], 701, null
    }, {
	&zgagz[3],  48 + ZWORDWID, 61, ZWORDWID, 8,
	GFLG_STRINGEXTEND | GFLG_TABCYCLE,
	GACT_RELVERIFY | GACT_STRINGLEFT, GTYP_STRGADGET,
	&zbox, null, null, 0, &zstrz[2], 702, null
    }, { 
	&zgaghere, 48 + ZWORDWID, 61, ZWORDWID, 8,
	GFLG_STRINGEXTEND | GFLG_TABCYCLE,
	GACT_RELVERIFY | GACT_STRINGLEFT, GTYP_STRGADGET,
	&zbox, null, null, 0, &zstrz[3], 703, null
    }
};


local ubyte normaltitle[] = "Search messages for a word or phrase";
local ubyte areaztitle[] =  "Search message area descriptions";
local ubyte fileztitle[] =  "Search mail packet filenames";
local ubyte tagstitle[] =   "Search taglines for a word or phrase";
local ubyte progresstitle[80], *oldtitle;

local struct ExtNewWindow zneww = {
    40, 20, ZWINWIDTH, ZWINHEIGHT, 0, 1,
    IDCMP_GADGETUP | IDCMP_CLOSEWINDOW | IDCMP_RAWKEY | IDCMP_INTUITICKS,
    WFLG_SMART_REFRESH | WFLG_CLOSEGADGET | WFLG_DEPTHGADGET
		| WFLG_DRAGBAR | WFLG_ACTIVATE | WFLG_NW_EXTENDED,
    null, null, normaltitle, null, null,
    0, 0, 0, 0, CUSTOMSCREEN, null
};

struct WhereWin zww = { &zneww };


struct Window *zwin;
struct RastPort *zrp;

struct Span {
    short sline, eline, scol, ecol;
};

local struct Span bodymarks[STRINGS][BODYMARKS], zh;

bool zfrom = true, zto = true, zsubj = true, zbody = false, znotrash = false;
bool lastfirstlast = false, insearch, fairy, datesverted, reSearch = false;
bool unsearchulated, funkyunsearched, anyunsearched, anysortclobbered;

local bool unfunky, showingprogress = false, tagforresearch = false;
local bool restorationated, gork;

ushort oldracount, lastsorder;
short readarea;
local ushort zendoffset, zbeginline, zbegincol, bmarcount[STRINGS];

local bool (*Chunk)(struct Mess *mm);
local struct Mess *firstfound;
struct Conf *listcc;

/* size_t zlen[STRINGS]; */
long totaltosearch, sofarsearched;

local whatsearch whats;


str LastStart(str name)
{
    str sp = strend(name), pp;
    char provo[NAMELEN];
    short i;
    static str nonname[] = {
	"JR", "SR", "II", "III", "IV", "2ND", "3RD", "4TH",
	"MD", "PHD", "DDS", "ESQ", "LLD", null		/* others? */
    };

    while (sp[-1] == ' ' && sp > name) sp--;
    while (sp[-1] != ' ' && sp > name) sp--;
    if (sp <= name)
	return name;
    strcpy(pp = provo, sp);
    while (*pp)
	if (*pp == ' ' || *pp == '.')
	    strcpy(pp, pp + 1);
	else
	    *pp = toupper(*pp), pp++;
    for (i = 0; nonname[i]; i++)
	if (!strcmp(provo, nonname[i])) {
	    while (sp[-1] == ' ' && sp > name) sp--;
	    while (sp[-1] != ' ' && sp > name) sp--;
	    break;
	}
    while (*sp == ' ') sp++;  
    return sp;
}


#ifdef OLD_LASTSORT

local void flipperooney(str f, str o)
{
    short n = strlen(o), l = n;

    while (n && o[n] != ' ')
	n--;
    if (o[n] == ' ') {
	n++;
	strcpy(f, o + n);
	l -= n;
	f[l++] = ' ';
	strncpy(f + l, o, n - 1);
	Stranks(f);
    } else
	strcpy(f, o);
}

#else

local void flipperooney(str f, str o)
{
    str l = LastStart(o);
    short off = l - o;
    strcpy(f, l);
    l = strend(f);
    if (off > 0) {
	*l++ = ' ';
	strncpy0(l, o, off - 1);
    }
}

#endif


local int agetest(struct Mess **m1, struct Mess **m2)
{
#ifdef _INT32
    int flarp = (*m1)->unixdate - (*m2)->unixdate;
    return flarp ? flarp : (*m1)->ixinbase - (*m2)->ixinbase;
#else
/* with 16 bit ints, we can't trust the sign to stay unchanged if  */
/* we cast the difference of two unsigned longs as a signed short! */
/* Furthermore, this can even happen with signed long if the diff- */
/* erence between the two is large enough, though it's unlikely.   */
    register ulong a = (*m1)->unixdate, b = (*m2)->unixdate;
    if (a == b)
	a = (*m1)->ixinbase, b = (*m2)->ixinbase;
    if (a == b)
	return 0;
    else
	return a > b ? 1 : -1;
#endif
}

local int ltnametest(struct Mess **m1, struct Mess **m2)
{
    char f1[NAMELEN + 1], f2[NAMELEN + 1];
    int flarp;

    flipperooney(f1, (*m1)->too);
    flipperooney(f2, (*m2)->too);
    flarp = stricmp(f1, f2);
    return flarp ? flarp : agetest(m1, m2);
}

local int lfnametest(struct Mess **m1, struct Mess **m2)
{
    char f1[NAMELEN + 1], f2[NAMELEN + 1];
    int flarp;

    flipperooney(f1, (*m1)->from);
    flipperooney(f2, (*m2)->from);
    flarp = stricmp(f1, f2);
    return flarp ? flarp : agetest(m1, m2);
}

local int tnametest(struct Mess **m1, struct Mess **m2)
{
    str t1 = (*m1)->too, t2 = (*m2)->too;
    int flarp;
    while (*t1 == ' ') t1++;
    while (*t2 == ' ') t2++;
    return (flarp = stricmp(t1, t2)) ? flarp : agetest(m1, m2);
}

local int fnametest(struct Mess **m1, struct Mess **m2)
{
    str t1 = (*m1)->from, t2 = (*m2)->from;
    int flarp;
    while (*t1 == ' ') t1++;
    while (*t2 == ' ') t2++;
    return (flarp = stricmp(t1, t2)) ? flarp : agetest(m1, m2);
}

local int titletest(struct Mess **m1, struct Mess **m2)
{
    str t1 = (*m1)->subject, t2 = (*m2)->subject;
    int flarp, o1 = strlen(t1), o2 = strlen(t2), l1, l2, d1, d2, lm = 25, x;

    if (!strnicmp(t1, "Re:", 3))
	t1 += 3;
    while (*t1 == ' ') t1++;
    l1 = o1 - (d1 = t1 - (*m1)->subject);
    if (!strnicmp(t2, "Re:", 3))
	t2 += 3;
    while (*t2 == ' ') t2++;
    l2 = o2 - (d2 = t2 - (*m2)->subject);
    if (l1 < l2 && o1 < 25) {
	for (x = l1; x + d1 < 25 && isspace(t2[x]); x++) ;
	if (x + d1 >= 25) lm = o1;
    }
    if (l2 < l1 && o2 < 25) {
	for (x = l2; x + d2 < 25 && isspace(t1[x]); x++) ;
	if (x + d2 >= 25) lm = o2;
    }
    if ((o1 >= lm && l1 < l2) || (o2 >= lm && l2 < l1))
	flarp = strnicmp(t1, t2, min(l1, l2));
    else
	flarp = stricmp(t1, t2);
    return flarp ? flarp : agetest(m1, m2);
}

local int numtest(struct Mess **m1, struct Mess **m2)
{
    return (*m1)->ixinbase - (*m2)->ixinbase;
}


/* ugh, do this in assembly?  Nah, too easy to fuck up the offsets. */

local bool ThreadLinked(register struct Mess *m1, register struct Mess *m2)
{
    register ulong x1 = m1->ixinbase, t1 = m1->replyto, a1 = m1->replyat;
    register ulong x2 = m2->ixinbase, t2 = m2->replyto, a2 = m2->replyat;
    /* ASSERT(x1 != x2 && (a1 != a2 || !a1));  nope! */
    return x1 == x2 /* fuggin BW door sometimes bundles one message twice! */
			|| x1 == t2 || x1 == a2 || x2 == t1 || x2 == a1
			|| (t1 && (t1 == t2 || t1 == a2)) || (t2 && a1 == t2);
}


/* call with messes pre-sorted into approximate order clumps should be in */

local void Threadify(struct Mess **mess, ushort mct)
{
    ushort curbase, curend, x, xx;
    
    for (curbase = curend = 0; curbase < mct; curend = curbase = curend + 1) {
	for (xx = curbase; xx <= curend /* increases! */; xx++) {
	    struct Mess *xm = mess[xx];
	    for (x = curend + 1; x < mct; x++)
		if (ThreadLinked(xm, mess[x])) {
		    struct Mess *tm = mess[++curend];
		    mess[curend] = mess[x];
		    mess[x] = tm;
		}
	}
	if (curend > curbase)
	    qsort(mess + curbase, curend + 1 - curbase, 4, agetest);
    }
}
/* This may leave large-scale sorting by subject somewhat APPROXIMATE... */


local int (*(testers[6]))()		/* array of pointers to functions */
    = { numtest, agetest, titletest, fnametest, tnametest, titletest };

void Resort(bool movement /* currently unused */ )
{
    int (*test)();
    struct Conf *cc;
    register struct Mess *mm;
    short c, m;
    short bulloffset = readareaz.confs[0] == &bullstuff;
    bool initial = whicha < 0 || whichm < 0;
    bool namer = sorder == 3 || sorder == 4, chrono = sorder && !namer;
    bool newworldorder = sorder != lastsorder
				|| (namer && lastfirst != lastfirstlast);

    if (cwin || !areaz.messct || !(newworldorder | anysortclobbered))
	return;
    PortOff();
    lastsorder = sorder;
    lastfirstlast = lastfirst;
    if (namer) {
	testers[3] = (lastfirst ? lfnametest : fnametest);
	testers[4] = (lastfirst ? ltnametest : tnametest);
    }
    test = testers[sorder];
    if (initial) {
	readareaz.current = whicha =
		    bulloffset + (readareaz.confs[bulloffset] == &personals);
	whichm = 0;
    }
    for (c = bulloffset; c < readareaz.messct; c++) {
	cc = readareaz.confs[c];
	if (cc == &replies)
	    break;
	if (!datesverted && chrono)
	    for (m = 0; m < cc->messct; m++) {
		mm = cc->messes[m];
		mm->unixdate = Text2ux(mm->date);
	    }
	if (newworldorder || cc->morebits & SORTCLOBBERED) {
	    if (c == whicha)
		cc->current = whichm;
	    mm = cc->messes[cc->current];
	    qsort(cc->messes, (size_t) cc->messct, 4, test);
	    if (sorder == 5)
		Threadify(cc->messes, cc->messct);
	    if (!initial)
#ifdef TWO_WAY_SORTING
		if (movement) {
		    for (m = 0; m < cc->messct; m++)
			if (!(cc->messes[m]->bits & MESEEN))
			    break;
		    if ((short) (cc->current = m - 1) < 0)
			cc->current = 0;
		} else
#endif
		    for (cc->current = 0, m = 1; m < cc->messct; m++)
			if (cc->messes[m]->bits & MESEEN) {
			    for (m = 0; m < cc->messct; m++)
				if (cc->messes[m] == mm) {
				    cc->current = m;
				    break;
				}
			    break;
			}
	    if (c == whicha)
		whichm = cc->current;
	    cc->morebits &= ~SORTCLOBBERED;
	}
    }
    if (chrono)
	datesverted = true;
    anysortclobbered = false;
    if (movement) {
#ifdef TWO_WAY_SORTING
	cc = readareaz.confs[whicha];
	onscreen = null;
	ShowNewMessage(cc, cc->messes[whichm]);
#else
	ShowHeader(readareaz.confs[whicha], onscreen);
#endif
    }
    PortOn();
}


local void StripSearch(struct Conf *cc)
{
    short m;
    struct Mess *mm, *mwowm;	/* Yacatisma != Yacatizma */
    struct Conf *ccf = cc->unfiltered;
    bool funky = cc == &filez || cc == &areaz || cc == &inuseareaz
				|| cc == &tagfaconf;
    bool currented = false, witcha = !funky && readareaz.confs[whicha] == cc;
    bool oldwitch = !funky && lawin && readareaz.confs[oldwhicha] == cc;

    if (ccf) {
	if (witcha | funky | oldwitch) {
	    if (witcha)
		cc->current = whichm;
	    if (oldwitch)
		mwowm = cc->messes[oldwhichm];
	    ccf->current = 0;
	    mm = cc->messes[cc->current];
	    for (m = 0; m < ccf->messct; m++) {
		if (mm == ccf->messes[m])
		    ccf->current = m, currented = true;
		if (oldwitch && mwowm == ccf->messes[m])
		    oldwhichm = m, oldwitch = false;
		if (currented && !oldwitch)
		    break;
	    }
	}
    }
    if (!funky) {
	for (m = 0; m < cc->messct; m++)
	    cc->messes[m]->bits &= ~(BODYMATCH | RESEARCHING);
	if (tagforresearch)
	    for (m = 0; m < cc->messct; m++)
		cc->messes[m]->bits |= RESEARCHING;
    }
    if (ccf) {
	Vfree(cc->messes);
	*cc = *ccf;
	FREE(ccf);
	anyunsearched = true;
	cc->morebits |= SORTCLOBBERED;
	if (witcha)
	    whichm = cc->current;
	if (!funky)
	    CheckAnyAllRead(cc);
    }
}


void UnSearch(register struct Conf *where, bool freshen)
{
    short a;
    struct Conf *cc, *occ, *cccp, **facc, *tc;
    struct Mess *mm;
    bool wasbm = onscreen && onscreen->bits & BODYMATCH, funky;

    if (!where) {
#ifdef BETA
	Err("INTERNAL ERROR -- Attempted to unsearch a\n"
				"null conference; doing global unsearch.");
#endif
	where = &readareaz;
    }
    if (!(funky = where == &filez || where == &tagfaconf
				|| where == &areaz || where == &inuseareaz))
	unsearchulated = false;
    if (!zwin)
	anyunsearched = false;
    if (where == &readareaz) {
	if (readareaz.unfiltered) {
	    facc = (adr) readareaz.confs;
	    cc = facc[whicha];
	    occ = lawin ? facc[oldwhicha] : null;
	    cccp = facc[readareaz.current];
	    Vfree(facc);
	    readareaz.confs = (adr) readareaz.unfiltered;
	    readareaz.unfiltered = null;
	    readareaz.messct = oldracount;
	    readareaz.current = whicha = 0;
	    onscreen = null;
	    for (a = 0; a < oldracount; a++) {
		tc = readareaz.confs[a];
		if (tc == cc)
		    whicha = a;
		if (tc == occ)
		    oldwhicha = a;
		if (tc == cccp)
		    readareaz.current = a;
	    }
	    anyunsearched = unsearchulated = true;
	    ClearTempCurrents(-1);
	}
	for (a = 0; a < readareaz.messct; a++)
	    if ((cc = readareaz.confs[a])->unfiltered) {
		StripSearch(cc);
		if (a == whicha && freshen)
		    gork = true;
		unsearchulated = true;
	    }
	insearch = false;
    } else {
	StripSearch(where);
	if (where == &areaz)
	    StripSearch(&inuseareaz);
	else if (where == &inuseareaz)
	    StripSearch(&areaz);
	if (!funky)
	    ClearTempCurrents(airing ? readareaz.current : whicha);
    }
    if (anyunsearched) {
	anysortclobbered = true;
	if (funky)
	    funkyunsearched = true;
	else {
	    Resort(false);
	    if (freshen) {
		cc = readareaz.confs[whicha];
		mm = cc->messes[whichm];
		if (mm == onscreen && (!airing || pick == whicha) && !wasbm)
		    ShowHeader(cc, mm);
		else {
		    onscreen = null;
		    ShowNewMessage(cc, mm);
		}
	    }
	    for (a = 0; a < readareaz.messct; a++)
		if (readareaz.confs[a]->unfiltered)
		    break;
	    if (a >= readareaz.messct)
		ActMenu(1, 3, false);		/* ghost "Undo search" */

	}
    }
}


/* Finds offset from start of s, within l characters, of first instance of
char c, case insensitive, as fast as possible.  Returns -1 if not found. */

#pragma regcall(iindexn(a0, d2, d1))

short iindexn(ustr s, ushort l, char c);

#asm
		public	_iindexn	; a0 = s, d2.w = l, d1.b = c

_iindexn:	movem.l	d2/d3,-(sp)
		move.b	d1,d3
		cmp.b	#'z',d3
		bgt.s	iic2
		cmp.b	#'a',d3
		blt.s	iic2
		and.b	#$DF,d3
iic2:		cmp.b	#'Z',d1		; c2 is now uppercase
		bgt.s	iichoo
		cmp.b	#'A',d1
		blt.s	iichoo
		or.b	#$20,d1
iichoo:		cmp.b	d1,d3		; c is now lowercase
		beq	iidb1		; c == c2, only test one
		bra.s	iidb2		; c != c2, test both

iiloo2:		move.b	(a0)+,d0	; *s++
		cmp.b	d0,d1
		beq.s	iiyes		; == c
		cmp.b	d0,d3
		beq.s	iiyes		; || == c2
iidb2:		dbra	d2,iiloo2
		bra.s	iino

iiloo1:		move.b	(a0)+,d0	; *s++
		cmp.b	d0,d1
		beq.s	iiyes		; == c
iidb1:		dbra	d2,iiloo1

iino:		moveq	#-1,d0		; no match, return -1
		movem.l	(sp)+,d2/d3
		rts
iiyes:		move.w	d2,d0		; yes match, return offset
		movem.l	(sp)+,d2/d3
		sub.w	d2,d0
		not.w	d0
		rts
#endasm


#pragma regcall(PossibleWord0(a0, d2, a1))

/* Boyer-Moore pre-search: find offset in s, within l chars, of first possible
instance of first token word in tk, case insensitive.  Check only the first and
last letters of token word for a match; let wordsin() do the full check. */


#ifdef C_BOWEL

short PossibleWord0(ustr s, ushort l, struct toker *tk)
{
    register short w = 1 - tk->lens[0];
    ubyte ec = tolower(tk->base[-w]);
    ubyte fc = tolower(tk->base[0]), oc;
    register ustr ep = s - w, endp = s + l;

    if (!w)
	return iindexn(s, l, fc);
    while (ep < endp) {
	if (tolower(oc = *ep) == ec && tolower(ep[w]) == fc)
	    return ep + w - s;
	oc = tk->bmtab[oc];
	ep += oc;
    }
    return -1;
}

#else

short PossibleWord0(ustr s, ushort l, struct toker *tk);
#  asm
		public	_PossibleWord0		; a0 = s, d2.w = l, a1 = tk
; d1.b = fc, d3.b = ec, d4.w = w, d5.w = oc, a2 = ep, a3 = endp, a5 = tk->base
pw0regs		reg	a2/a3/a5/d2-d5

_PossibleWord0:	movem.l	pw0regs,-(sp)
		moveq	#0,d5
		moveq	#0,d4
		move.b	256(a1),d4		; tk->lens[0]
		subq	#1,d4			; w gets negated later
		move.l	768(a1),a5		; tk->base
		move.b	(a5),d1			; first char of first word
		cmp.b	#'A',d1
		blt.s	nofclower
		cmp.b	#'Z',d1
		bgt.s	nofclower
		or.b	#$20,d1			; lowercase fc
nofclower:	move.b	(a5,d4.w),d3		; last char of word
		cmp.b	#'A',d3
		blt.s	noeclower
		cmp.b	#'Z',d3
		bgt.s	noeclower
		or.b	#$20,d3			; lowercase ec
noeclower:	move.l	a0,a3			; endp...
		add.w	d2,a3			; (d2 is always small, 1..80)
		move.l	a0,a2
		add.w	d4,a2			; ep

		neg.w	d4			; w is now backwards offset
		bne	doboyer
		bsr.s	_iindexn
		bra.s	pwex

doboyer:	cmp.l	a3,a2			; ep < endp?
		bhs.s	pwfail			; no, so no match
		move.b	(a2),d5			; oc = *ep
		move.b	d5,d0
		cmp.b	#'A',d0
		blt.s	noeplower
		cmp.b	#'Z',d0
		bgt.s	noeplower
		or.b	#$20,d0
noeplower:	cmp.b	d0,d3			; tolower(*ep) == ec?
		bne.s	pwnext			; no, move along
		move.b	(a2,d4.w),d0		; ep[w]
		cmp.b	#'A',d0
		blt.s	noepwlower
		cmp.b	#'Z',d0
		bgt.s	noepwlower
		or.b	#$20,d0
noepwlower:	cmp.b	d0,d1			; tolower(ep[w]) == fc?
		bne.s	pwnext			; no, move along
		add.w	d4,a2			; yes, return ep + w - s
		sub.l	a0,a2
		move.l	a2,d0
		bra.s	pwex

pwnext:		move.b	(a1,d5.w),d5		; oc = tk->bmtab[oc]
		add.w	d5,a2
		bra.s	doboyer
pwfail:		moveq	#-1,d0
pwex:		movem.l	(sp)+,pw0regs
		rts

#  endasm
#endif C_BOWEL


/* like strstr(), but case insensitive and with explicit length: */

str strnistr(register str outer, str inner, ushort len)
{
    register short i, ll = strlen(inner);
    char f = *inner;

    while (~(i = iindexn(outer, len, f))) {
	outer += i;
	len -= i;
	if (len < ll)
	    return null;
	if (!strnicmp(outer, inner, (size_t) ll))
	    return outer;
	outer++, len--;
    }
    return null;
}


/* like strstr(), but with explicit length, case sensitive: */

str strnstr(register str outer, str inner, ushort len)
{
    register short i, ll = strlen(inner);
    char f = *inner;

    /* This should use a case-sensitive version of iindexn(), but fuck it. */
    while (~(i = iindexn(outer, len, f))) {
	outer += i;
	len -= i;
	if (len < ll)
	    return null;
	if (!strncmp(outer, inner, (size_t) ll))
	    return outer;
	outer++;
    }
    return null;
}


local short wordsin(ustr line, short len, struct toker *tk, short t)
{
    register short wc = 0, ll = len, tl;
    register ustr lin = line;

    while ((tl = tk->lens[t + wc]) <= ll && !strnicmp(lin,
				tk->base + tk->enos[t + wc], (size_t) tl)) {
	lin += tl;
	zendoffset = lin - line;
	ll -= tl;
	if (++wc + t >= tk->c)
	    if (!tk->back || !ll || !isalnum(*lin)
				|| !isalnum(tk->base[tk->enos[wc + t - 1]]))
		return wc + t;
	    else
		return 0;
	while (ll && isspace(*lin))
	    lin++, ll--;
	if (!ll)
	    return wc + t;
    }
    return 0;
}


local bool Spot(str line, short tix)
{
    short ll, i, j;
    struct toker *tk = &tok[tix];
    char fc;

    if (!tk->lens[0])
	return false;
    if (!line)
	return false;
#ifdef BETA
    if (!TypeOfMem(line)) {
	Err("INTERNAL ERROR -- attempted word search\n"
				"of string at invalid address 0x%lx.", line);
	return false;
    }
#endif
    ll = strlen(line);
    fc = tk->base[0];
    for (j = 0; ~(i = PossibleWord0(line + j, ll - j, tk)); j++) {
	j += i;
	if (tk->front && j && isalnum(line[j - 1]) && isalnum(fc))
	    continue;
	if (wordsin(line + j, ll - j, tk, 0) >= tk->c)
	    return true;
    }
    return false;
}


bool Hockey(struct Mess *mm, short tix)		/* do a body check */
{
    short i, j, k, l, m, n, p, r, s, t, u;
    struct toker *tk = &tok[tix];
    char fc;

    if (!tk->lens[0] || (mm->bits & LOADED && !mm->linect))
	return false;
    fc = tk->base[0];
    if ((!(mm->bits & LOADED) && !LoadPktMessage(mm)) || !mm->linect)
	return false;
    for (m = zbeginline; m < mm->linect; m++) {
	ustr line = mm->lines[m], lin = line;
	if (!line || (l = line[-1]) - zbegincol < tk->lens[0]) {
	    zbegincol = 0;
	    continue;
	}
	j = mm->linetypes[m] & TYPE_MASK;
	if (znotrash && (j == TRASHTYPE || j == TRASHWRAPTYPE))
	    continue;	/* assert no trash lines in MIDDLE of text body */
	for (j = zbegincol; ~(i = PossibleWord0(line + j, l - j, tk)); j++) {
	    zh.scol = (j += i);
	    zh.sline = m;
	    if (tk->front && j && isalnum(line[j - 1]) && isalnum(fc))
		continue;
	    n = m; k = j;
	    if (r = wordsin(line + k, l - k, tk, 0))
		for (;;) {
		    if (r >= tk->c) {
			zh.ecol = zendoffset + k;
			zh.eline = n;
			return true;
		    }
		    if (++n >= mm->linect)
			return false;
		    if (lin = mm->lines[n]) {
			s = lin[-1];
			for (k = 0; isspace(lin[k]); k++)
			    if (k >= s)
				goto skipline;
			u = mm->linetypes[n];
			u = u == QUOTETYPE || u == QUOTEWRAPTYPE;
			while (!(t = wordsin(lin + k, s - k, tk, r)) && u) {
			    if (lin[k] != '|' && lin[k] != ':') {
				p = 0;
				while (lin[k] != '>')
				    if (++p >= 4)
					goto mismatch;
				    else if (++k >= s)
					goto skipline;
			    }
			    do {
				if (++k >= s)
				    goto skipline;
			    } while (isspace(lin[k]));
			}
			if (!(r = t))
			    goto mismatch;		/* is this right?? */
		    }
		  skipline:
		    ;
		}
	  mismatch:
	    ;
	}
	zbegincol = 0;
    }
    return false;
}


local short CompareSpots(short l, short c, short ll, short cc)
{
    register short ld = l - ll;
    return ld ? ld : c - cc;
}


#ifdef OVERCOMPLICATED

local void ReconciliateMatches(short t, short i, short tt, short ii)
{
    struct Span *s = &bodymarks[t][i], *ss = &bodymarks[tt][ii];
    short et, ei, xi;

    if (CompareSpots(s->eline, s->ecol, ss->sline, ss->scol) > 0 &&
		     CompareSpots(s->sline, s->scol, ss->eline, ss->ecol) < 0) {
/* overlapping spans!  Destroy the one that starts later (or ends sooner): */
	xi = CompareSpots(s->sline, s->scol, ss->sline, ss->scol);
	if (xi < 0 || (!xi && CompareSpots(s->eline, s->ecol,
					   ss->eline, ss->ecol) >= 0))
	    et = t, ei = i;
	else
	    et = tt, ei = ii;
	for (xi = ei + 1; xi < bmarcount[et]; xi++)
	    bodymarks[et][xi - 1] = bodymarks[et][xi];
	bmarcount[et]--;
    }
}

#endif


void TabulateBodyMatches(struct Mess *mm)
{
    register short t, i;
#ifdef OVERCOMPLICATED
    register short tt, ii;
#endif

    for (t = 0; t < STRINGS; t++) {
	bmarcount[t] = zbeginline = zbegincol = 0;
	for (i = 0; i < BODYMARKS && Hockey(mm, t); bmarcount[t] = ++i) {
	    bodymarks[t][i] = zh;
#ifdef OVERCOMPLICATED
	    zbeginline = zh.sline;
	    zbegincol = zh.scol + 1;	/* ASSERT scol is not end of line */
	    ASSERT(mm->lines[zbeginline][-1] >= zbegincol);
#else
	    zbeginline = zh.eline;
	    zbegincol = zh.ecol;	/* okay to be at end of line */
#endif
	}
    }
#ifdef OVERCOMPLICATED
    /* make sure no one char is inside more than one span: */
    for (t = 0; t < STRINGS; t++)
	for (i = 0; i < bmarcount[t]; i++) {
	    for (ii = i + 1; ii < bmarcount[t]; ii++)
		ReconciliateMatches(t, i, t, ii);
	    for (tt = t + 1; tt < STRINGS; tt++)
		for (ii = 0; ii < bmarcount[tt]; ii++)
		    ReconciliateMatches(t, i, tt, ii);
	}
#endif
}



#ifdef FANCY_INVERSION

/* call only if: mm->bits & BODYMATCH && mm->lines[line], and you have */
/* called TabulateBodyMatches(mm) previously. */

ushort HowManyInverted(register ushort line, register ushort col, bool *verse)
{
    register short t, i;
    ushort trans = 9999;
    bool lverse = *verse;

    for (t = 0; t < STRINGS; t++)
	for (i = 0; i < bmarcount[t]; i++) {
	    register struct Span *s = &bodymarks[t][i];
	    if (s->sline == line && s->scol >= col + lverse && s->scol < trans)
		trans = s->scol, *verse = true;
	    if (s->eline == line && s->ecol > col && s->ecol < trans)
		trans = s->ecol, *verse = false;
	}
    return trans;
}


bool InvertedHere(ushort line, ushort col)
{
    register short t, i;
    
    for (t = 0; t < STRINGS; t++)
	for (i = 0; i < bmarcount[t]; i++) {
	    register struct Span *s = &bodymarks[t][i];
	    if (CompareSpots(s->sline, s->scol, line, col) <= 0 &&
				CompareSpots(s->eline, s->ecol, line, col) > 0)
		return true;
	}
    return false;
}

#endif


void ShowHits(short look)
{
    register short t, i;

    for (t = 0; t < STRINGS; t++)
	for (i = 0; i < bmarcount[t]; i++) {
	    register struct Span *s = &bodymarks[t][i];
	    DisplayMsgRegion(s->scol, s->sline, s->ecol, s->eline, look);
	}
}


local bool Check1(register struct Mess *mm)
{
    short r;

    if (zbody)
	for (r = 0; r < STRINGS; r++)
	    if (Hockey(mm, r)) {
		mm->bits |= BODYMATCH;
		return true;
	    }
    for (r = 0; r < STRINGS; r++)
	if ((zfrom && Spot(mm->bits & ISBULLETIN ? mm->too : mm->from, r))
				|| (zto && Spot(mm->too, r))
				|| (zsubj && Spot(mm->subject, r)))
	    return true;
    return false;
}


local bool FCheck1(struct Mess *mm)
{
    short r;

    for (r = 0; r < STRINGS; r++)
	if (Spot(mm->from, r) || Spot(mm->subject, r))
	    return true;
    return false;
}


local bool TCheck1(struct Mess *mm)
{
    str ss = (adr) mm, sse = strchr(ss, '\n');
    short r;

    if (sse)
	*sse = 0;
    for (r = 0; r < STRINGS; r++)
	if (Spot(ss, r)) {
	    if (sse)
		*sse = '\n';
	    return true;
	}
    if (sse)
	*sse = '\n';
    return false;
}


local bool ACheck1(struct Mess *mm)
{
    register struct Conf *cc = (adr) mm;
    short r;

    for (r = 0; r < STRINGS; r++)
	if (Spot(cc->shortname, r) || Spot(cc->longname, r)
				|| Spot(cc->confnum, r))
	    return true;
    return false;
}


local void Tokize(short tix)
{
    register struct toker *tk = &tok[tix];
    short t, i;
    ustr line = zbufz[tix];
    bool word = false;

    tk->front = !!isspace(*line);
    tk->back = !!isspace(line[strlen(line) - 1]);
    while (isspace(*line))
	line++;
    tk->lens[0] = t = 0;
    tk->base = line;
    while (*line) {
	if (isalnum(*line)) {
	    if (!word) {
		tk->enos[t] = line - tk->base;
		tk->lens[t++] = 1;
		word = true;
	    } else
		tk->lens[t - 1]++;
	} else {
	    tk->enos[t] = line - tk->base;
	    tk->lens[t++] = 1;
	    word = false;
	}
	line++;
	while (isspace(*line))
	    word = false, line++;
    }
    ASSERT(!t || tk->enos[0] == 0);
    tk->c = i = t;
    /* and now... Mr. Boyer and Mr. Moore! */
    memset(tk->bmtab, tk->lens[0], 256);
    t = tk->lens[0] - 1;
    for (i = 0; i < t; i++) {
	register ubyte c = tk->base[i], d = t - i;
	tk->bmtab[c] = d;
	if (islower(c))
	    tk->bmtab[_toupper(c)] = d;
	else if (isupper(c))
	    tk->bmtab[_tolower(c)] = d;
    }
}


void ShowSearchProgress(long sofar)
{
    static ulong sex = 0;
/*  if (firstfound && firstfound != onscreen)
	ShowNewMessage(Confind(firstfound->confnum), firstfound);
*/ /* This causes problems by unexpectedly changing whicha/whichm */
    if (showingprogress && (!sofar || IntuitionBase->Seconds != sex)) {
	long centage = 100 * sofar / totaltosearch;
	sex = IntuitionBase->Seconds;
	sprintf(progresstitle, "SEARCHING: %2ld%% complete ", centage);
	SetWindowTitles(zwin, progresstitle, (APTR) -1);
    }
}


void StartShowingProgress(void)
{
    if (showingprogress = (unfunky & zbody)) {
	oldtitle = zwin->Title;
	ShowSearchProgress(sofarsearched = 0);
    }
}


void StopShowingProgress(void)
{
    if (showingprogress) {
	showingprogress = false;
	SetWindowTitles(zwin, oldtitle, (APTR) -1);
    }
}


local short Check1Area(register struct Conf *real, short area)
{
    struct Conf *old;
    struct Mess **zes;
    register struct Mess *mm;
    short m, owm;
    bool preed = false;

    if (!NEW(old) || !(zes = Valloc(real->messct << 2))) {
	if (old)
	    FREE(old);
	Err("Could not search -- no memory.");
	return -1;
    } else {
	*old = *real;
	real->messes = zes;
	real->messct = zbeginline = zbegincol = 0;
	real->unfiltered = old;
	real->current = real->tomect = 0;
	if (unfunky) {
	    if (area == whicha)
		old->current = whichm;
	    if (lawin && area == oldwhicha)
		owm = oldwhichm;
	}
	for (m = 0; m < old->messct; m++) {
	    mm = old->messes[m];
	    if (reSearch && unfunky)
		if (mm->bits & RESEARCHING)
		    preed = true;
		else
		    continue;
	    if (Chunk(mm)) {
		real->messes[real->messct++] = mm;
		if (unfunky && mm->bits & TOME)
		    real->tomect++;
		if (!firstfound)
		    firstfound = mm;
	    }
	    ShowSearchProgress(++sofarsearched);
	    if (m == old->current)
		real->current = real->messct ? real->messct - 1 : 0;
	    if (unfunky && m == owm && area == oldwhicha && lawin)
		oldwhichm = real->messct ? real->messct - 1 : 0;
	    if (zbody && unfunky && !(area == whicha && m == whichm)
						&& !(mm->bits & DONTSTRIP))
		StripMessage(mm);	/* maybe loaded by Check1 */
	} /* and we don't want to cache non-hit messages anyway */
	if (!real->messct) {
	    if (preed) {
		ASSERT(unfunky);
		real->current = 0;
		for (m = 0; m < old->messct; m++) {
		    if ((mm = old->messes[m])->bits & RESEARCHING)
			real->messes[real->messct++] = mm;
		    if (m == old->current) {
			real->current = real->messct ? real->messct - 1 : 0;
			if (area == whicha)
			    whichm = m;
			if (area == oldwhicha)		/*****  lossage */
			    oldwhichm = m;
		    }
		}
		restorationated = true;
	    } else {
		Vfree(zes);
		*real = *old;
		FREE(old);
	    }
	    if (unfunky) {
		for (m = 0; m < real->messct; m++)
		    real->messes[m]->bits &= ~RESEARCHING;
		if (area == whicha)
		    LoadPktMessage(real->messes[whichm]);
	    }
	    return 0;
	}
    }
    if (unfunky) {
	for (m = 0; m < old->messct; m++)
	    old->messes[m]->bits &= ~RESEARCHING;
	CheckAnyAllRead(real);
    }
    anysortclobbered = true;
    real->morebits |= SORTCLOBBERED;
    return 1;
}


bool ActuallySearch(bool here)
{
    bool ret = false;
    short r;
    struct Conf *real, *real2;
    str erst;
#ifndef TRY_NAH_ALLOW_FULL_SEARCH
    struct Conf **nram;
    short a, b, wa, owa;
#endif

    if (unfunky && !(zfrom | zto | zsubj | zbody)) {
	Err("You must set the checkmark in at least one\n"
				"of the four gadgets for where to look.");
	return false;
    }
    restorationated = false;
    firstfound = null;
    PortOff();
    for (r = 0; r < STRINGS; r++)
	Tokize(r);
    if (lawin)
	listcc->current = pick;
    readarea = (whats == readareazwin ? readareaz.current : whicha);
    if (!here) {
#ifdef TRY_NAH_ALLOW_FULL_SEARCH
	Err("You must buy the registered\nversion to search in all\n"
					"message areas at once.");
#else
	tagforresearch = reSearch;
	UnSearch(&readareaz, true);
	tagforresearch = false;
	if (!(nram = Valloc((readareaz.messct + !replies.messct) << 2)))
	    Err("Could not search -- no memory.");
	else {
	    totaltosearch = 0;
	    for (a = 0; a < readareaz.messct; a++) {
		struct Conf *cc = readareaz.confs[a];
		if (cc != &replies)
		    totaltosearch += cc->messct;
	    }
	    if (!totaltosearch) totaltosearch = 1;
	    StartShowingProgress();
	    Chunk = Check1;
	    wa = owa = -1;
	    for (b = a = 0; a < readareaz.messct; a++) {
		real = readareaz.confs[a];
		if (real != &replies && (r = Check1Area(real, a))) {
		    if (r < 0) {
			b = -1;
			break;
		    }
		    if (a == whicha)
			wa = b;
		    if (lawin && a == oldwhicha)
			owa = b;
		    nram[b++] = real;
		} else {
		    if (a == whicha) {
			LeaveListArea(real);
			wa = b ? b - 1 : 0;
		    }
		    if (lawin && a == oldwhicha)
			owa = b ? b - 1 : 0;
		}
	    }
	    if (b > 0) {
		ret = gork = true;
		if (replies.messct) {
		    if (readareaz.confs[whicha] == &replies)
			wa = b;
		    nram[b++] = (adr) &replies;
		}
		if (lawin) {
		    if (readareaz.confs[oldwhicha] == &replies)
			owa = b - 1;
		    if (owa < 0) owa = 0;
		    oldwhicha = owa;
		}
		oldracount = readareaz.messct;
		readareaz.messct = b;
		readareaz.unfiltered = (adr) readareaz.confs;
		readareaz.confs = nram;
		if (wa < 0) {
		    wa = b > 1 && readareaz.confs[0] == &bullstuff;
		    wa += b > wa + 1 && readareaz.confs[wa] == &personals;
		}
		readareaz.current = whicha = wa;
		onscreen = null;
		real = nram[whicha];
		ShowNewMessage(real, real->messes[whichm = real->current]);
		fairy = true;
		ClearTempCurrents(-1);
	    } else {
		Vfree(nram);
		if (!b) {
		    StopShowingProgress();
		    Err("No matching messages found.");
		}
	    }
	}
#endif
    } else if (unfunky && readareaz.confs[readarea] == &replies)
	Err("This version of Q-Blue cannot\nsearch in the replies area.");
    else {
	real2 = null;
	if (unfunky) {
	    real = readareaz.confs[readarea];
	    Chunk = Check1;
	    erst = "messages";
	} else if (whats == taglinewin) {
	    real = &tagfaconf;
	    Chunk = TCheck1;
	    erst = "taglines";
	} else if (whats == filezwin) {
	    real = &filez;
	    Chunk = FCheck1;
	    erst = "files";
	} else {
	    if (inuseified)
		real = &inuseareaz, real2 = &areaz;
	    else
		real = &areaz, real2 = &inuseareaz;
	    Chunk = ACheck1;
	    erst = "areas";
	}
	tagforresearch = reSearch;
	UnSearch(real, true);	/* "You have confused the true and the real." */
	tagforresearch = false;
	totaltosearch = real->messct;
	StartShowingProgress();
	if (!(r = Check1Area(real, readarea))) {
	    if (real == &inuseareaz && (r = Check1Area(real2, readarea)) > 0) {
		ToggleInUseify();
		real = real2;
		real2 = null;
	    } else {
		StopShowingProgress();
		Err("No matching %s found.", erst);
	    }
	}
	if (r > 0) {
	    if (real2 && real2->confs)
		Check1Area(real2, readarea);	/* ignore failure */
	    ret = gork = true;
	    if (unfunky) {
		onscreen = null;
		if (whats == normal || bgupdate) {
		    whichm = real->current;
		    ShowNewMessage(real, real->messes[whichm]);
		}
		ClearTempCurrents(readarea);
	    }
	}
    }
    ret |= restorationated;
    StopShowingProgress();
    if (unfunky)
	ActMenu(1, 3, ret);			/* Undo search */
    PortOn();
    return ret;
}


local bool ZDing(bool anyway)
{
    ushort i;
    if (!anyway)
	for (i = 0; i < STRINGS; i++)
	    if (zbufz[i][0])
		return false;
    if (!anyway)
	DisplayBeep(scr);
    ActivateGag(&zgagz[0], zwin);
    return true;
}


struct Conf *ConfToSearch(whatsearch wh)
{
    switch (wh) {
      case areazwin:
	return inuseified ? &inuseareaz : &areaz;
      case filezwin:
	return &filez;
      case taglinewin:
	return &tagfaconf;
      default:
	return &readareaz;
    }
    return null;		/* stupid fucking compiler... */
}


local bool DoSoichIDCMP(struct IntuiMessage *im)
{
    char k = 0;
    bool lawful = im->IDCMPWindow == lawin;
    bool shifty = !!(im->Qualifier & SHIFTKEYS);
    ushort id;

    ynwin = zwin;
    if (im->Class == IDCMP_RAWKEY && !(im->Qualifier & ALTKEYS))
	switch (k = KeyToAscii(im->Code, im->Qualifier)) {
	  case '\t':
	    ZDing(true);
	    break;
	  case 'F':
	    if (unfunky)
		Seyn(&zgagfrom, zfrom = !zfrom);
	    break;
	  case 'T':
	    if (unfunky)
		Seyn(&zgagtoo, zto = !zto);
	    break;
	  case 'S':
	    if (unfunky)
		Seyn(&zgagsubj, zsubj = !zsubj);
	    break;
	  case 'B':
	    if (unfunky) {
		Seyn(&zgagbody, zbody = !zbody);
		AbleAddGad(&zgagnotrash, zwin, zbody);
	    }
	    break;
	  case 'R':
	    if (!(zgagresearch.Flags & GFLG_DISABLED))
		Seyn(&zgagresearch, reSearch = !reSearch);
	    break;
	  case 'I':
	    if (!(zgagnotrash.Flags & GFLG_DISABLED))
		Seyn(&zgagnotrash, znotrash = !znotrash);
	    break;
	  case 'N':
	    UnSearch(ConfToSearch(whats), true);
	    return true;
	  case 'A':
	    if (unfunky && !ZDing(false))
		return ActuallySearch(false);
	    break;
	  case 'H':
	  case 'W':
	  case '\n':
	  case '\r':
	    if (k != (unfunky ? 'W' : 'H') && !ZDing(false))
		return ActuallySearch(true);
	  case ESC:
	    break;
	default:
	    Do1ListAreaIDCMP(im, listcc);
	    break;
	}
    else if (im->Class == IDCMP_GADGETUP)
	switch (id = ((struct Gadget *) im->IAddress)->GadgetID) {
	  case 700:						/* strings */
	  case 701:
	  case 702:
	  case 703:
	    if (GAGFINISH(im) && GAGCHAIN(im) && !(id == 703 && !shifty))
		ActivateGadget(&zgagz[(id + (shifty ? STRINGS - 1 : 1) - 700)
					% STRINGS], zwin, null);
	    break;
	  case 710:						/* do here */
	    if (!ZDing(false))
		return ActuallySearch(true);
	  case 711:						/* do all */
	    if (unfunky && !ZDing(false))
		return ActuallySearch(false);
	    break;
	  case 712:						/* nowhere */
	    UnSearch(ConfToSearch(whats), true);
	    return true;
	  case 713:						/* from */
	    if (unfunky)
		CHECK(zgagfrom, zfrom);
	    break;
	  case 714:						/* to */
	    if (unfunky)
		CHECK(zgagtoo, zto);
	    break;
	  case 715:						/* subject */
	    if (unfunky)
		CHECK(zgagsubj, zsubj);
	    break;
	  case 716:						/* body */
	    if (unfunky) {
		CHECK(zgagbody, zbody);
		AbleAddGad(&zgagnotrash, zwin, zbody);
	    }
	    break;
	  case 717:						/* re-search */
	    CHECK(zgagresearch, reSearch);
	    break;
	  case 718:						/* no trash */
	    CHECK(zgagnotrash, znotrash);
	    break;
	default:
	    if (lawful)
		Do1ListAreaIDCMP(im, listcc);
	    break;
	}
    else if (lawful)
	Do1ListAreaIDCMP(im, listcc);
    else if (im->Class == IDCMP_CLOSEWINDOW) {
	if (im->IDCMPWindow == cbitwin) {
	    CloseBitsWin();
	    return false;
	} else if (im->IDCMPWindow == cwin)
	    poseclosed = true;
    }
    return k == ESC || im->Class == IDCMP_CLOSEWINDOW;
}


bool Soich(whatsearch owhats)
{
    struct Conf *cc, *where;
    bool bull, rip, alreadynarrowed, listsaster;
    short i;

    anyunsearched = reSearch = false;
    whats = owhats;
    unfunky = owhats <= listwin;
    if (unfunky)
	zneww.Title = normaltitle;
    else if (owhats == filezwin)
	zneww.Title = fileztitle;
    else if (owhats == taglinewin)
	zneww.Title = tagstitle;
    else /* areazwin */
	zneww.Title = areaztitle;
    where = ConfToSearch(owhats);
    alreadynarrowed = !!where->unfiltered;
    if (where == &readareaz && !alreadynarrowed)
	for (i = 0; i < readareaz.messct; i++)
	    if (readareaz.confs[i]->unfiltered) {
		alreadynarrowed = true;
		break;
	    }
    zgagz[0].TopEdge = zgagz[2].TopEdge = 2 * fakefight + 16;
    zgagz[1].TopEdge = zgagz[3].TopEdge = zgagz[0].TopEdge + fakefight + 12;
    zgagfrom.TopEdge = zgagtoo.TopEdge = zgagsubj.TopEdge = zgagbody.TopEdge
			= zgagz[3].TopEdge + fakefight + 9 + checkoff;
    zgagresearch.TopEdge = zgagnotrash.TopEdge = zgagfrom.TopEdge + checkspace;
    zgaghere.TopEdge = zgagall.TopEdge = zgagnowhere.TopEdge
			= zgagresearch.TopEdge + checkspace - checkoff;
    zneww.Height = zgaghere.TopEdge + fakefight + 13;
    /* height for fakefight = 12: 126 */
    zgaghere.GadgetText = (unfunky ? &zgthere : &zgtwindow);
    AbleGad(&zgagall, unfunky);
    AbleGad(&zgagfrom, unfunky);
    AbleGad(&zgagtoo, unfunky);
    AbleGad(&zgagsubj, unfunky);
    AbleGad(&zgagbody, unfunky);
    AbleGad(&zgagresearch, alreadynarrowed && unfunky);
    AbleGad(&zgagnotrash, unfunky & zbody);
    if (!(zwin = OpenBlueGagWin(&zww, &zgagz[0]))) {
	WindowErr("doing word search.");
	return false;
    }

    if (unfunky) {
	cc = readareaz.confs[whicha];
	bull = cc == &bullstuff;
	rip = cc == &replies;
    }
    FixDefarrow();
    zrp = zwin->RPort;
    ynwin = zwin;
    GhostOn(zwin);
    SeynLabel(&zgagfrom, zfrom, "From");
    SeynLabel(&zgagtoo, zto, "To");
    SeynLabel(&zgagsubj, zsubj, "Subj.");
    SeynLabel(&zgagbody, zbody, "Body text");
    SeynLabel(&zgagresearch, reSearch, "Refine previous search");
    SeynLabel(&zgagnotrash, znotrash, "Ignore end-of-message clutter");
    GhostOff(zwin);
    SetAPen(zrp, LABELCOLOR);
    SetBPen(zrp, backcolor);
    MoveNominal(zrp, 156, tifight + 9 + font->tf_Baseline);
    Text(zrp, "Words or phrases to search for:", 31);
    MoveNominal(zrp, 16, zgagfrom.TopEdge + font->tf_Baseline
						- (fakefight - 15) / 2);
    Text(zrp, "Look in:", 8);
    MoveNominal(zrp, 16, zgaghere.TopEdge + font->tf_Baseline + 2);
    Text(zrp, "Search area(s):", 15);
    defarrow.LeftEdge = zgaghere.LeftEdge;
    defarrow.TopEdge = zgaghere.TopEdge;
    DrawBorder(zrp, &defarrow, ARROWLEFT, 0);
    ActivateGadget(&zgagz[0], zwin, null);
    gork = fairy = false;

    EventLoop(&DoSoichIDCMP);
    CloseBlueGagWin(zwin);
    zwin = null;
    if (gork & unfunky) {
	insearch = true;
	if (!lawin) {
	    if (fairy && owhats != readareazwin)
		PickArea(showareawin);
	    else if (!fairy && showlistwin && owhats == normal)
		DoList(readareaz.confs[whicha]);
	}
    }
    if (owhats == listwin) {
	listsaster = true;
	for (i = 0; i < readareaz.messct; i++)
	    if (readareaz.confs[i] == listcc) {
		listsaster = false;
		break;
	    }
	if (listsaster)
	    alswitch = true;
    }
    return gork | unsearchulated | anyunsearched;
}
