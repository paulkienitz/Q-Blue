/* shit for loading messages from disk in Q-Blue */

/* NOTE:  isspace() is true for ^I ^J ^K ^L ^M and ' ', but not '\0' */
/* all isXXXX() are false for args >= 128 */

#include <exec/memory.h>
#include <dos/dosextens.h>
#include "qblue.h"

#define HAM_HANDED_BW_DELETE

#define PRELOADSPACE   50000
#define PRELOADMAX     20

#define GUESSEDGE      60
#define NOWRAPINDENT   8
#define TAGLINEPAD     4


#pragma regcall(iindexn(a0, d2, d1))		/* required! */
short iindexn(ustr s, ushort l, char c);

void FlushExtraDoorCommands(void);
void UnSearch(struct Conf *where, bool freshen);
bool ParseNetAddress(str a);
void EmptyDir(str path, bool quiet);
void SaveBookmarks(void);
void SeekErr(str what);
void ZeroOldfilenamesEtc(void);
#ifdef SEE_PRELOAD_COUNT
void RenderUR(str what);
#endif
str strnstr(register str outer, str inner, ushort len);
str strnistr(register str outer, str inner, ushort len);


import struct Conf **readareazfree, *curconf;
import struct Mess placeholder;
import struct trash *trashfiles;
import BPTR tryunpackcd;
import ustr *oldfilenames, *messageID, *newsgroups;
import short *tempcurrents;

import char qnetkluge[], bwpassword[], macro1[], macro2[], macro3[];
import char requestedfiles[10][15], bwkeywords[10][23], bwfilters[10][23];

#ifdef WARN_LONG_REPLY
import short lastlongwarn;
#endif
import ushort czone, cnet, cnode, cpoint, wrapmargin;
import bool filerequests_exist, personalityflaw, doorwarnedonce, flushreplies;
import bool galactiwarn, nowreloading, inverse_netkluge, pdq_exists, qwkE, dos3;
import bool searchlight_ie, qwkE_subs, official_iemail, pcboard_ie, pcboard_net;
import bool searchlight, pcboard_hood, ie_is_gated, no_forward, exceed_dos;


BPTR texthand;

/****  str templines[LINELIMIT + 1];        ****/
/****  ubyte templinetypes[LINELIMIT + 1];  ****/
ustr *templines = null, templinetypes = null;
local ustr *otemplines, otemplinetypes, tempRFCs, tempgroups;

bool rupted = false, lstifle = false, quitleaveopen = false;
short confounded;
ulong loaded_size;

local long llim;
local short linct, prevlen, shortening;
local BPTR filerr;
local ubyte prevline[80];

#ifdef BETA
short quotifycount = 0, loadmesscount = 0, forceloadcount = 0,
	    stripmesscount = 0, forcestripcount = 0;
#endif


local struct Conf *confPool;
local struct Mess *messPool, *messReturnees = null;
local str stringPool;
str stringPermPool;

#define CONFPUDDLE   20
#define MESSPUDDLE   20
#define STRINGPUDDLE 1016
#define MLINEPUDDLE  1016

local short confIndex = CONFPUDDLE, messIndex = MESSPUDDLE;
local short stringIndex = STRINGPUDDLE, stringPermIndex = STRINGPUDDLE;

short Lanum, Lanum2, Lanst, Ljumpcol, Llx, Lix, Llen;
short /* bool */ Ljumpedup, Leol, Lpseudoeol, Lanyansi;
local bool Lnoshovel, Lnotfirst, gothalf, gotksub;
ubyte Lech, Ligch;


bool MakeRoom(bool here);		/* for forward reference */


void Vfree(adr where)
{
    if (where)
	if ((ulong) where & 1)		/* allocated by BValloc */
	    FreeMem((ubyte *) where - 1, ((ubyte *) where)[-1] + 1L);
	else
	    FreeMem((ulong *) where - 1, ((ulong *) where)[-1] + 4);
}


adr Valloc(ulong size)		/* longword aligned, any size */
{
    ulong *pt;
    if (!size)
	return null;
    while (!(pt = AllocMem(size + 4, 0)) && MakeRoom(true)) ;
    if (pt) {
	*pt = size;
	return pt + 1;
    }
    return null;
}


str BValloc(ushort size)	/* odd-byte aligned, 255 bytes max */
{
    ubyte *pt;
    if (!size || size > 255)
	return null;
    while (!(pt = AllocMem(size + 1, 0)) && MakeRoom(true)) ;
    if (pt) {
	*pt = size;
	return pt + 1;
    }
    return null;
}


void FreePool(adr where)
{
    register ulong *more, *ww = where;
    while (ww) {
	more = (adr) ww[-1];
	FreeMem(ww - 2, ww[-2] + 8);
	ww = more;
    }
}


local bool AddPuddle(ulong size, adr *list)
{
    ulong *pt;

    if (size && list) {
	while (!(pt = AllocMem(size + 8, MEMF_CLEAR)) && MakeRoom(true)) ;
	if (pt) {
	    *pt = size;
	    pt[1] = (ulong) *list;
	    *list = pt + 2;
	    return true;
	}
    }
    return false;
}


struct Conf *NewPoolConf(void)		/* return a new zeroed struct Conf */
{
    if (confIndex >= CONFPUDDLE) {
	if (!AddPuddle(CONFPUDDLE * sizeof(struct Conf), &confPool))
	    return null;
	confIndex = 0;
    }
    return &confPool[confIndex++];
}


struct Mess *NewPoolMess(void)		/* return a new zeroed struct Mess */
{
    if (messReturnees) {
	register struct Mess *mm = messReturnees;
	messReturnees = mm->personalink;
	memset(mm, 0, sizeof(*mm));
	return mm;
    }
    if (messIndex >= MESSPUDDLE) {
	if (!AddPuddle(MESSPUDDLE * sizeof(struct Mess), &messPool))
	    return null;
	messIndex = 0;
    }
    return &messPool[messIndex++];
}


void ReturnToPool(struct Mess *mm)
{
    mm->personalink = messReturnees;
    messReturnees = mm;
}


/* The funny list pointer dancing in here allows us to allocate both very    */
/* small and quite large strings efficiently.  Anything over half the size   */
/* of the puddle is allocated separately, so it does not cause fragmentation */
/* of the puddles and can exceed the maximum puddle size (up to 64K - 9).    */

ustr NewPoolString(ushort length)	/* new string of length + 1 bytes */
{
    ustr result;

    if (stringIndex + length >= STRINGPUDDLE) {		/* length 0 allowed */
	ustr oldpool = stringPool;
	if (!AddPuddle(length >= STRINGPUDDLE / 2 ? length : STRINGPUDDLE,
					&stringPool))
	    return null;
	if (length >= STRINGPUDDLE / 2) {       /* gets a puddle of its own */
	    result = stringPool;
	    if (oldpool) {		  /* move new pool behind previous! */
		((adr *) result)[-1] = ((adr *) oldpool)[-1];
		((adr *) oldpool)[-1] = result;
		stringPool = oldpool;
	    } else
		stringIndex = length + 1;
	    return result;
	}
	stringIndex = 0;
    }
    result = stringPool + stringIndex;
    stringIndex += length + 1;
    return result;
}


/* This is the same idea except it is not freed at closing time --  */
/* this is for strings that stay valid until the program is exited. */
/* Large sizes (over 1K) are not supported in this case.            */

ustr NewPermPoolString(ushort length)
{
    ustr result;

    if (stringPermIndex + length >= STRINGPUDDLE) {	/* length 0 allowed */
	if (!AddPuddle(STRINGPUDDLE, &stringPermPool))
	    return null;
	stringPermIndex = 0;
    }
    result = stringPermPool + stringPermIndex;
    stringPermIndex += length + 1;
    return result;
}


/* This works the same way, except the pool belongs to a particular     */
/* message, and because we are dealing with message body lines, we      */
/* create a leading length byte instead of allowing for a trailing nul. */

str NewMessPoolLine(struct Mess *mm, ubyte length)
{
    ustr result;

    if (!mm->poolindex || mm->poolindex + length > MLINEPUDDLE) {
	if (!AddPuddle(MLINEPUDDLE, &mm->linepool))
	    return null;
	mm->poolindex = 1;	/* zero means full, one means zero */
    }
    result = mm->linepool + mm->poolindex;   /* one byte past physical start */
    result[-1] = length;
    mm->poolindex += length + 1;
    return result;
}


long SSeek(BPTR file, long position, long mode)
{
    long r = Seek(file, position, mode);
    return !dos3 && IoErr() ? -1 : r;
}


/* This frees the body text from a message while leaving the header intact.  It
causes any future reference to this message to go back to disk for the text. */

void StripMessage(struct Mess *mm)
{
#ifdef BETA
    if (!TypeOfMem(mm)) {
	Err("Internal error; BOGUS StripMessage 0x%lx", mm);
	return;
    }
    if (mm->lines)
	if (mm->bits & DONTSTRIP)
	    forcestripcount++;
	else
	    stripmesscount++;
#endif
    mm->bits &= ~LOADED;
#ifndef DONT_USE_LINEPOOL
    FreePool(mm->linepool);
#else
    if (mm->lines) {
	register short i;
	for (i = 0; i < mm->linect; i++)
	    Vfree(mm->lines[i]);
    }
#endif
    Vfree(mm->lines);
    Vfree(mm->linetypes);
    mm->lines = (adr) mm->linetypes = mm->linepool = null;
    mm->linect = mm->poolindex = 0;
}


bool StripConf(short a)
{
    short b;
    bool suck = false;
    struct Conf *cc = readareaz.confs[a];

    ASSERT(a >= 0 && a < readareaz.messct);
    if (cc == &replies /* || cc == &bullstuff */ )
	return false;
    for (b = 0; b < cc->messct; b++)
	if (cc->messes[b]->bits & LOADED && (b != whichm || a != whicha)) {
	    suck = true;
	    StripMessage(cc->messes[b]);
	}
    return suck;
}


bool MakeRoom(bool here)
{
    short a;
    struct Conf *cc;
    bool r;

    for (a = 0; a < readareaz.messct && a < whicha; a++)
	if (StripConf(a)) return true;
    for (a = readareaz.messct - 1; a >= 0 && a >= (whicha + !here); a--)
	if (StripConf(a)) return true;
    if (here)
	return false;
    ASSERT(whicha >= 0 && whicha < readareaz.messct);
    cc = readareaz.confs[whicha];
    r = false;
    if (cc != &replies && cc != &bullstuff)
	for (a = 0; a < whichm; a++)
	    if (cc->messes[a]->bits & LOADED) {
		r = true;
		StripMessage(cc->messes[a]);
	    }
    return r;
}


bool MakeLotsaRoom(void)
{
    short a;
    bool ret = false;

    for (a = 0; a < readareaz.messct; a++)
	ret |= StripConf(a);
    return ret;
}


		/* DO NOT import without this pragma! */
#pragma regcall(SameConfNum(a0,a1))

		/* DO NOT call with parms not word aligned! */
bool SameConfNum(str c1, ulong *c2);
#asm
	PUBLIC	_SameConfNum
_SameConfNum:
	cmp.l	(a0)+,(a1)+	; compare first four bytes of string
	bne	cdiff
	cmp.l	(a0)+,(a1)+	; compare last four bytes of string
	bne	cdiff
	moveq	#1,d0
	rts
cdiff:	moveq	#0,d0
	rts
#endasm


struct Conf *Confind(str cnum)
{
    register struct Conf *cc;
    ulong snum[2];		/* must be word aligned */
    register ushort i;

    confounded = -1;
    if (!cnum[0] || strlen(cnum) > 7)
	return null;
    snum[0] = snum[1] = 0;
    strcpy((str) snum, cnum);
    if (SameConfNum(bullstuff.confnum, snum))
	return &bullstuff;
    else if (SameConfNum(personals.confnum, snum))
	return &personals;
    else if (SameConfNum(replies.confnum, snum))
	return &replies;
    strupr((str) snum);		 /* replies etc. use lowercase */
    for (i = 0; i < areaz.messct; i++) {
	cc = areaz.confs[i];
	if (SameConfNum(cc->confnum, snum)) {
	    confounded = i;		/* save a call to Conf2ix */
	    return cc;
	}
    }
    return null;
}


short Conf2ix(struct Conf *cc)
{
    short i;
    if (cc)
	for (i = 0; i < areaz.messct; i++)
	    if (cc == areaz.confs[i])
		return i;
    return -1;
}


#define BLANCTL (bit('\t') | bit('\n') | bit('\r') | bit('\f') | bit(0))

bool AllBlankN(ustr line, ushort len)
{
    ustr lend = line + len;
    while (line < lend) {
	register ubyte c = *line;
	if (c < ' ' ? !(bit(c) & BLANCTL) : (c != ' ' && c != 0xFF))
	    return false;
	line++;
    }
    return true;
}


bool AllBlank(ustr line)
{
    if (!line)
	return true;
    return AllBlankN(line, line[-1]);
}


bool lch(ustr line, ustr s)
{
    size_t sl = strlen(s);
    if (!line || line[-1] < sl)
	return false;
    return !strncmp(line, s, sl);
}


local str knowngarbages[] = {
    " * ", " \xFE ", " ? Origin:", "SEEN?BY:", "?SEEN-BY:", "?PATH:",
    " Blue Wave/", " -- SPEED", " ** MS?QWiK", " ** MS QWiK", "   RoseReader",
    "<ORIGINAL MESSAGE OVER", " | AmiQWK", "                          VQWK ",
    "WWIVMail/", "\\-\\ ", " -- SRP", " -- DLG", " -> Alice4Mac", "?X-Reader:",
    "?Via ", "\xFA\xF9\xFA T", " -- SRW", " ? NFX v", null
};
    
#define isSplotch(c) (!isalnum(c) && !isspace(c))  /* revised several times */

bool KnownGarbage(ustr line)
{
    register short i, l;
    register ustr s, lin;

    if (line[-1] > 7 && !strncmp(line, " X ", 3) && isalnum(line[3])
			&& strncmp(line + 3, "X ", 2)
			&& (strnstr(line + 4, " X ", line[1] - 4)
				|| !strncmp(line + line[-1] - 2, " X", 2)))
	return true;
    i = *line == ' ';
    if (line[-1] >= 8 + i && (((isSplotch(line[i]) || line[i] == 'X')
			&& !strnicmp(line + i + 1, " CMPQWK", 7)) ||
		    (!i && !strnicmp(line, "CMPQWK ", 7) && isdigit(line[7]))))
	return true;		/* what an annoying program... */
    for (s = knowngarbages[i = 0]; s; s = knowngarbages[++i]) {
	l = line[-1];
	lin = line;
	while (l && (*lin == *s || (*s == '?' && isSplotch(*lin))))
	    lin++, s++, l--;
	if (!*s) return true;
    }
    return false;
}


bool IsTearLine(ustr line)
{
    register ubyte c = *line;
    if (line[-1] < 3 || c != line[2] || (c != line[1] && c != '-'))
	return false;
    return (line[-1] == 3 || line[3] == ' ') && ispunct(line[1]) &&
		    (c == '-' || c == '=' || c == '_' || c == '.' || c == '~');
}


bool Shovelline(struct Mess *mm, short linc /*, bool trim */ )
{
    str lcopy;

    if (prevlen > 80)
	prevlen = 80;
/*****
    while (trim && prevlen && prevline[prevlen - 1] == ' ')
	prevlen--;
*****/
    if (Lech == '\n' && prevline[prevlen - 1] == '\r')
	prevlen--;
    if (prevlen) {
#ifndef DONT_USE_LINEPOOL
	if (!(lcopy = NewMessPoolLine(mm, prevlen))) {
	    FreePool(mm->linepool);
	    mm->linepool = null;
	    mm->poolindex = 0;
	    return false;
	}
#else
	if (!(lcopy = BValloc(prevlen))) {
	    register short lx;
	    for (lx = 0; lx < linc; lx++)
		Vfree(templines[lx]);
	    return false;
	}
#endif
	memcpy(lcopy, prevline, (size_t) prevlen);
	templines[linc] = lcopy;
    } else
	templines[linc] = null, templinetypes[linc] = UNKNOWNTYPE;
    return true;
}


/* This wrapper for Read is used in case of dos handlers such as     */
/* CrossDOS which may sometimes return a length > 0 but < requested. */

long RRead(BPTR hand, adr buf, long len)
{
    long done = 0, res;
    do {
	if ((res = Read(hand, (ustr) buf + done, len - done)) > 0)
	    done += res;
	else
	    return (done ? done : res);
    } while (done < len);
    return done;
}


ustr RetrieveReferences(ustr *groups)
{
    ustr foo = tempRFCs;
    *groups = tempgroups;
    tempRFCs = tempgroups = null;
    return foo;
}


local bool Check4Netmail(struct Mess *mm, ustr lin, short lx, bool rapper)
{			/* returns FALSE if given a kluge line */
    ubyte blarch[150], oldsub[26];
    ustr tail, khead, ktail;
    ushort nbits = 0;
    struct Conf *cc = Confind(mm->confnum);
    bool isiemail = cc && (cc->morebits & INTERNET_EMAIL) && !official_iemail;
    bool isnet = cc && ((cc->areabits & INF_NETMAIL) || (qwk && ie_is_gated
				&& isiemail)) && !(mm->bluebits & UPL_NETMAIL
				|| mm->bits & EMAIL_REPLY);
/*  bool isusenet = cc && (cc->areabits & (INF_ECHO | INF_NETMAIL)) == INF_ECHO
				&& cc->net_type == INF_NET_INTERNET; */
    bool subtolineflag = true;

    if ((gotksub || mm->bluebits & UPL_NETMAIL || gothalf)
					&& AllBlankN(lin, lx)) {
	return false;
    }
    if ((!qwk || !(qwkE_subs || isnet)) && !isiemail /* && !isusenet */ )
	return true;
    if (inverse_netkluge)
	strncpy0(oldsub, mm->subject, 25);
    if (qwkE_subs || (inverse_netkluge && isnet | isiemail)) {
	str subhead = qwkE_subs ? "Subject: " : "Subj: ";
	unsigned subheadlen = qwkE_subs ? 9 : 6;
	if (lin && lx >= subheadlen && !strnicmp(lin, subhead, subheadlen)) {
	    if ((lx -= subheadlen) >= SUBJLEN - 1)
		lx = SUBJLEN - 1;
	    strncpy0(blarch, lin + subheadlen, lx);
	    if (tail = BValloc(lx + 1)) {
		Vfree(mm->subject);
		strcpy(mm->subject = tail, blarch);
	    }
	    gotksub = true;
	    if (inverse_netkluge && oldsub[0] == '@') {    /* MKQWK style */
		if (khead = strchr(oldsub + 1, ' ')) {
		    *khead++ = 0;
		    do {
			if (tail = strchr(khead, ' '))
			    *tail++ = 0;
			while (isspace(*khead))
			    khead++;
			if (!stricmp(khead, "CRASH"))
			    nbits |= UPL_NETCRASH;
			if (!stricmp(khead, "IMM"))
			    nbits |= UPL_NETIMMEDIATE;
			if (!stricmp(khead, "DIR"))
			    nbits |= UPL_NETDIRECT;
		    } while (khead = tail);
		}
		if (ParseNetAddress(oldsub + 1)) {
		    mm->bluebits |= UPL_NETMAIL;
		    mm->attribits |= nbits;
		    mm->zone = czone;
		    mm->net = cnet;
		    mm->node = cnode;
		    mm->point = cpoint;
		}
	    }
	    return false;
	}
    }
    if (isiemail) {
	str tohead = pcboard_ie ? (gothalf ? "\xFF@TO2    :" : "\xFF@TO     :")
				: (searchlight_ie ? "Internet: " : "To: ");
	unsigned toheadlen = strlen(tohead);
	if (lin && lx >= toheadlen && !strnicmp(lin, tohead, toheadlen)) {
	    strncpy0(blarch + 60, lin + toheadlen,
					pcboard_ie ? 60 : lx - toheadlen);
	    if (!(tail = strrchr(blarch + 60,'@')))
		tail = blarch + 60;
	    if ((!gothalf || pcboard_ie) && !ParseNetAddress(tail)) {
		if ((lx -= toheadlen) >= MAXIETOLEN - 1)
		    lx = MAXIETOLEN - 1;
		if (gothalf) {
		    strncpy(blarch, mm->too, 60);
		    for (tail = strend(blarch); tail < blarch + 60; tail++)
			*tail = ' ';	/* undo Stranks() */
		} else
		    strcpy(blarch, blarch + 60);
		Stranks(blarch);
		if (tail = BValloc(strlen(blarch) + 1)) {
		    Vfree(mm->too);
		    strcpy(mm->too = tail, blarch);
		} else	/* crude band-aid: */
		    strncpy0(mm->too, blarch,
					min(mm->too[-1] - 1, lx - toheadlen));
		gothalf = true;
		mm->bits |= EMAIL_REPLY;
		/* mm->bluebits &= ~UPL_NETMAIL; */
		return false;
	    }
	}
    }
    if (!isnet || !qwk)
	return true;
    if (pcboard_net) {					/* PCB 15+ style */
#ifndef USE_AT_ROUTE
	if (lx >= 70 && !strncmp(lin, "\xFF@TO     :", 10)) {
	    strncpy0(oldsub, lin + 10, 60);
	    Stranks(oldsub);
	    tail = strrchr(oldsub, '@');
	    if (khead = strstr(tail + 1, " +"));
		*khead++ = 0;
	    if (tail && !strnicmp(oldsub, mm->too, nbits = strlen(mm->too))
					&& tail - oldsub >= nbits
					&& ParseNetAddress(tail + 1)) {
#else
	if (lx >= 70 && !strncmp(lin, "\xFF@ROUTE  :", 10)) {
	    strncpy0(oldsub, lin + 10, 60);
	    Stranks(oldsub);
	    if (ParseNetAddress(oldsub)) {
#endif
		ushort kl = strlen(khead);	/* due to lack of stristr() */
		mm->zone = czone;
		mm->net = cnet;
		mm->node = cnode;
		mm->point = cpoint;
		mm->bluebits |= UPL_NETMAIL;
		if (strnistr(khead, "+C", kl))
		    mm->attribits |= UPL_NETCRASH;
		if (strnistr(khead, "+D", kl))
		    mm->attribits |= UPL_NETDIRECT;
#ifndef USE_AT_ROUTE
		*tail = 0;
		if (tail - oldsub > strlen(mm->too) &&
					(khead = BValloc(1 + tail - oldsub))) {
		    Vfree(mm->too);
		    strcpy(mm->too = khead, oldsub);
		}
#endif
		return false;
	    }
	}
    } else if (lin && lx >= 3) {
	khead = qnetkluge, ktail = strend(qnetkluge) - 1;
	tail = lin + lx - 1;
	while (khead <= ktail && lin <= tail) {
	    if (*khead == '/') {
		if (khead[1] == '/') {
		    khead++;
		    if (*lin != '/')
			break;
		} else
		    goto validhead;
	    } else if (toupper(*khead) != toupper(*lin))
		break;
	    khead++, lin++;
	}
	return true;
      validhead:
	while (ktail > khead && tail > lin) {
	    if (toupper(*ktail) != toupper(*tail))
		break;
	    ktail--, tail--;
	}
	if (ktail != khead || tail <= lin)
	    return subtolineflag;
	strncpy0(blarch, lin, tail + 1 - lin);
	if (ParseNetAddress(blarch)) {
	    mm->zone = czone;
	    mm->net = cnet;
	    mm->node = cnode;
	    mm->point = cpoint;
	    mm->bluebits |= UPL_NETMAIL;
	    return false;
	}
    }
    return true;
}


#pragma regcall(WeirdANSIshit(d2, a0))

local void WeirdANSIshit(ubyte c, ustr linebuf)
{
    if (c == ';')
	Lanst = 3, Lanum2 = 0;
    else if (isdigit(c)) {
	if (Lanst == 2)
	    Lanum = 10 * Lanum + c - '0';
	else
	    Lanum2 = 10 * Lanum2 + c - '0';
    } else {
	if ((c == 'H' || c == 'f') && Lanum2 <= 1)
	    Lpseudoeol = Leol = true;
	else if (c == 'C') {
	    if (!Lanum)
		Lanum = 1;
	    if (Ljumpedup) {
		if (Lnoshovel = !Llx && !Ljumpcol && Lnotfirst)
		    memcpy(linebuf, prevline, Ljumpcol = Llx = prevlen);
		Lanum -= Ljumpcol;
	    }
	    if (Lanum > 0) {
		if (Llx + Lanum >= 80)
		    Lpseudoeol = Leol = true;
		else while (Lanum--)
		    linebuf[Llx++] = ' ';
	    }
	    Ljumpedup = false;
	} else if (c == 'D') {
	    if (!Lanum)
		Lanum = 1;
	    if (Lanum <= Llx)
		Llx -= Lanum;
	} else if (c == 'B' || c == 'f' || c == 'H')
	    linebuf[Llx++] = ' ';
	Lanst = 0;
	if (Ljumpedup = (c == 'A' || c == 's'))
	    Ljumpcol = Llx;
    }
}


#pragma regcall(LoadALine(a0, a1, d0))
#pragma regcall(FilterOddEOLs(a1))

#ifdef C_LOADALINE

local void LoadALine(ustr linebuf, ustr inbuf, bool trim)
{
    register ubyte c;
    for (c = inbuf[Lix]; !Lpseudoeol && Lix < Llen && !(Leol =
				    c == Lech && (!Ljumpedup || !Llx))
				&& (c == Ligch || Llx < 80); c = inbuf[++Lix])
	if (c == Ligch || c == 0x8D || (Ljumpedup && c == Lech))
	    Lanst = 0;
	else {
	    if (!Lanst) {
		if (c >= ' ')
		    linebuf[Llx++] = c;
		else {
		    if (c == ESC && trim) {
			Lanst = 1;
			continue;
		    }
		    if (c == '\t')
			do
			    linebuf[Llx++] = ' ';
			while (Llx & 7);
		    else if (c == '\b' && trim) {
			if (Llx)		/* backspace */
			    Llx--;
		    } else
			linebuf[Llx++] = c;
		}
		Ljumpedup = false;
	    } else if (Lanst == 1) {
		Lanum = Lanum2 = 0;
		if (c == '[')
		    Lanyansi = true, Lanst = 2;
		else
		    Lanst = 0;
	    } else
		WeirdANSIshit(c, linebuf);
	}
}


void FilterOddEOLs(ustr inbuf)
{
    register short ie;
    for (ie = 0; ie < Llen; ie++)
	if (inbuf[ie] == 0x8D && inbuf[ie - 1] != ' ' && inbuf[ie - 1] != '\t')
	    inbuf[ie] = ' ';
	else if (inbuf[ie] == '\f')
	    inbuf[ie] = Lech;
}

#else

void LoadALine(ustr linebuf, ustr inbuf, bool trim);
void FilterOddEOLs(ustr inbuf);

#  asm
	public	_LoadALine,_FilterOddEOLs

	public	_Llx,_Lix,_Lanst,_Lanum,_Lanum2
	public	_Leol,_Lpseudoeol,_Ljumpedup,_Lanyansi
	public	_Lech,_Ligch,_WeirdANSIshit

linebuf	equr	a0	; arg -- passed to WeirdANSIshit
inbuf	equr	a1	; arg
trim	equr	d0	; arg
c	equr	d2	; ubyte value -- passed to WeirdANSIshit
t	equr	d4	; scratch
lx	equr	d1	; cached _Llx -- write back for WeirdANSIshit
anst	equr	d3	; cached _Lanst -- write back for WeirdANSIshit
nix	equr	d5	; Llen - Lix countdown index
ech	equr	d6	; cached _Lech -- constant byte

savreg	reg	d2-d6

_LoadALine:
	movem.l	savreg,-(sp)
	move.w	_Llx,lx
	move.w	_Lanst,anst
	move.b	_Lech,ech
	move.w	_Lix,nix
	add.w	nix,inbuf
	move.b	(inbuf)+,c
	sub.w	_Llen,nix
	beq.s	xitt			; THIS SHOULD BE UNNECESSARY
	not.w	nix			; nix = Llen - Lix - 1

Hweil:	tst.w	_Lpseudoeol		; complicated condition for while loop
	bne.s	xitt
	cmp.b	ech,c
	bne.s	eol0
	tst.w	_Ljumpedup
	beq.s	eol1
	tst.w	lx
	bne.s	eol0
eol1:	move.w	#1,_Leol
	bra.s	xitt
eol0:	clr.w	_Leol
	cmp.b	_Ligch,c
	beq.s	skippy		; fold part of the first if into the while
	cmp.w	#80,lx
	bge.s	xitt

	cmp.b	#$8D,c			; is c a legitimate char?
	beq.s	skippy
	tst.w	_Ljumpedup
	beq.s	legit
	cmp.b	ech,c
	bne.s	legit

skippy:	moveq	#0,anst			; no, ignore c this time
	bra.s	liewh

legit:	tst.w	anst			; yes.  Are we processing an ANSI thing?
	beq.s	anch0

	cmp.w	#1,anst			; yes; a left bracket after an Esc?
	bgt.s	anch2
	clr.w	_Lanum
	clr.w	_Lanum2
	cmp.b	#'[',c
	bne.s	nabr
	moveq	#2,anst
	move.w	#1,_Lanyansi
	bra.s	liewh
nabr:	moveq	#0,anst		; Esc was followed by a non-[
	bra.s	anch0		; unlike C version, don't lose this char

anch2:	move.w	lx,_Llx
	move.w	anst,_Lanst
	movem.l	a0/a1/d0/d1,-(sp)
	bsr	_WeirdANSIshit		; args are already set up
	movem.l	(sp)+,a0/a1/d0/d1
	move.w	_Lanst,anst
	move.w	_Llx,lx
	bra.s	liewh

anch0:	cmp.b	#' ',c			; non-ansi.  Also non-control char?
	bhs.s	chord			; if so, skip following tests
	cmp.b	#27,c			; an Esc char?
	bne.s	chtab
	tst.w	trim
	beq.s	chord
	moveq	#1,anst
	bra.s	liewh

chtab:	cmp.b	#9,c			; tab char?
	bne.s	chbak
ttab:	move.b	#' ',(linebuf,lx.w)
	addq.w	#1,lx
	move.w	lx,t
	and.w	#7,t
	bne.s	ttab
	bra.s	ean0

chbak:	cmp.b	#8,c			; backspace char?
	bne.s	chord
	tst.w	trim
	beq.s	chord
	tst.w	lx
	ble.s	ean0
	subq.w	#1,lx
	bra.s	ean0

chord:	move.b	c,(linebuf,lx.w)	; ordinary char, stick it in
	addq.w	#1,lx
ean0:	moveq	#0,anst			; all non-ansi stuff drops here
	clr.w	_Ljumpedup

liewh:	move.b	(inbuf)+,c
	dbra	nix,Hweil

xitt:	move.w	anst,_Lanst
	move.w	lx,_Llx
	not.w	nix			; convert change of nix back into Lix
	add.w	_Llen,nix
	move.w	nix,_Lix
	movem.l	(sp)+,savreg
	rts


;inbuf	equr	a1	; as before
;c	equr	d2	; as before
ie	equr	d0
k	equr	d1

_FilterOddEOLs:
	move.l	c,-(sp)
	moveq	#0,ie
fower:	cmp.w	_Llen,ie
	bge.s	xfoel
	move.b	(inbuf,ie.w),k
	cmp.b	#$8D,k			; soft EOL used by some systems
	bne.s	fftst
	move.b	-1(inbuf,ie.w),c
	cmp.b	#' ',c			; space
	beq.s	fftst
	cmp.b	#9,c			; tab
	beq.s	fftst
	move.b	#' ',(inbuf,ie.w)
	bra.s	dew4
fftst:	cmp.b	#12,k			; formfeed
	bne.s	dew4
	move.b	_Lech,(inbuf,ie.w)
dew4:	addq.w	#1,ie
	bra.s	fower
xfoel:	move.l	(sp)+,c
	rts

#  endasm
#endif C_LOADALINE


local bool WithinLimits(short linc)
{
    if (linc >= SUPERLINELIMIT)
	return false;
    if (templines == otemplines && linc >= BASICLINELIMIT) {
	if (!(templines = Alloc((SUPERLINELIMIT + 1) * 5)) && MakeRoom(true))
	    templines = Alloc((SUPERLINELIMIT + 1) * 5);
	if (!templines) {
	    templines = otemplines;
	    return false;
	}
	templinetypes = (adr) ((ulong) templines + 4 * (SUPERLINELIMIT + 1));
	memcpy(templines, otemplines, 4 * linc);
	memcpy(templinetypes, otemplinetypes, linc + 1);
    }
    return true;
}


local void FreeTempTempLines(void)
{
    if (templines != otemplines)
	FreeMem(templines, (SUPERLINELIMIT + 1) * 5);
    templinetypes = otemplinetypes;
    templines = otemplines;
}


bool IsEchoful(struct Mess *mm, struct Conf *cc)
{
    if (mm->bits & UPL_PRIVATE)
	return false;
    return !cc || qwk || ((cc->areabits & (INF_ECHO | INF_NETMAIL)) == INF_ECHO
				&& cc->net_type == INF_NET_FIDONET);
}


local bool LoadMessageText(struct Mess *mm, BPTR hand, short qeol, bool trim)
{
    ubyte realinbuf[LOADBLOCK + 1], linebuf[81];
    ustr inbuf = &realinbuf[1];
    bool mainfil = (hand == texthand);
    bool rapt = false, convertnewlines = false, ignore_rapt = false;
    static ubyte eolchs[3] = { '\r', QEOL, '\n' };
    short ie, gap;
    long rlen;

    Lech = eolchs[qeol];
    Ligch = /* qeol ? 0 : */ '\n';
    Lanst = prevlen = linct = Llx = 0;
    Lnotfirst = Ljumpedup = Lnoshovel = Lanyansi = gothalf = gotksub = false;
    Leol = true;
    templinetypes[0] = UNKNOWNTYPE;
    realinbuf[0] = ' ';
    rlen = min(LOADBLOCK, llim);
    loaded_size = 0;
    while (llim >= 0 && (!llim || (Llen = Read(hand, inbuf, rlen)) >= 0)
						&& WithinLimits(linct)) {
	Lix = 0;
	if (Llen > llim)
	    Llen = llim;
	llim -= Llen;
	loaded_size += Llen;
	if (!linct && !qeol) {		/* semi-certainly first time through */
	    for (ie = 0; ie < Llen; ie++) {
		if (inbuf[ie] == '\n')
		    convertnewlines = true;
		else if (inbuf[ie] == '\r') {
		    convertnewlines = false;
		    break;
		}
	    }
	}
	if (convertnewlines)
	    for (ie = 0; ie < Llen; ie++)
		if (inbuf[ie] == '\n')
		    inbuf[ie] = '\r';
	if ((!llim || Llen < rlen) && Llen && inbuf[Llen - 1] == 26)
	    Llen--;
	if (Llen <= 0) {			/* simulate newline at EOF */
	    if (llim > 0 && llim < 10000000)
		filerr = IoErr();
	    if (Llx) {
		Llen = 1;
		inbuf[0] = Lech;
	    }
	    llim = -1;
	}
	if (trim)
	    FilterOddEOLs(inbuf);
	inbuf[-1] = inbuf[Llen - 1];
	while (Lix < Llen && WithinLimits(linct)) {
	    gap = shortening = 0;
	    if (rapt && !ignore_rapt && llim >= 0) {
		register ubyte c = inbuf[Lix];
		if (c == ' ' || c == '\t') {
		    while ((c == ' ' || c == '\t') && gap < GAP_MASK) {
			gap++;
			if (++Lix >= Llen)
			    goto whilebreak;
			c = inbuf[Lix];
		    }
		} else
		    for (ie = 79; ie >= 0; ie--) {
			c = prevline[ie];
			if (isspace(c) || c == '-' || (Lanyansi && c >= 0x80)) {
			    prevlen = ie + 1;
			    shortening = 79 - ie;
			    while (ie < 79)
				linebuf[Llx++] = prevline[++ie];
			    break;
			}
		    }
	    }
	    Lpseudoeol = ignore_rapt = false;
	    LoadALine(linebuf, inbuf, trim);
	    if (Lix < Llen || Llx == 80) {
		bool wasrapt = rapt;
		templinetypes[linct + 1] = rapt ? WRAPTYPE | gap : UNKNOWNTYPE;
		if (!(rapt = !Leol) && !Lpseudoeol && Lix < Llen)
		    Lix++;
		if (!Lnoshovel)
		    if (Lnotfirst) {
			if (!Shovelline(mm, linct++ /*, trim */ ))
			    return false;
			if (!WithinLimits(linct))
			    break;
		    } else
			Lnotfirst = !nowreloading || Check4Netmail(mm,
							linebuf, Llx, wasrapt);
		Lnoshovel = false;
		memcpy(prevline, linebuf, (size_t) Llx);
		prevlen = Llx;
#ifndef REPLY_FORBID_ONLY_SOME_FIDO_INTERFERENCE
		if (qeol == 2 && IsEchoful(mm, curconf))
		    if ((Llx >= 10 && !strncmp(prevline, " * Origin:", 10))
				    || (Llx >= 3 && !strncmp(prevline, "---", 3)
					&& (Llx == 3 || prevline[3] == ' ')))
			prevline[1] = '+';
		    else if (Llx >= 8 && !strncmp(prevline, "SEEN-BY:", 8))
			prevline[4] = '+';
#else
#  ifndef FIDO_ANYTHING_GOES
		if (qeol == 2 && IsEchoful(mm, curconf) && Llx >= 3
					&& !strncmp(prevline, "---", 3)
					&& (Llx == 3 || prevline[3] == ' '))
		    prevline[1] = '+';
#  endif
#endif
		Llx = 0;
	    } else
		ignore_rapt = true;
	}
      whilebreak:
	;
    }
    if (WithinLimits(linct) && prevlen && (!mainfil
			|| prevlen != 1 || prevline[0] != ' '))
	if (!Shovelline(mm, linct++ /*, trim */ ))
	    return false;
    return true;
}


void FigureOutQuoteWrap(struct Conf *cc)		/* what a mess! */
{
    register short spaces, prespaces, lx;
    register ustr l1, l0, l2;
    register ubyte c, d;
    short glyphs, len, len0;
    bool barred, addressed = false, eyedeed = false, frommed = false;
    bool minfrommed = false, minaddressed = false, ayed = false;
    bool bw_inet = cc && cc->net_type == INF_NET_INTERNET;

    if (!bw_inet) {
	for (lx = linct - 1; lx >= 0; lx--) {     /* pick out trailing trash */
	    l0 = templines[lx];
	    if (l0 && templinetypes[lx] == UNKNOWNTYPE)
		if (AllBlank(l0) || KnownGarbage(l0) ||
				(IsTearLine(l0) && (lx >= linct - 2 ||
				 !((templinetypes[lx + 1] & TYPE_MASK)
				   == WRAPTYPE && (templinetypes[lx + 2]
				   & TYPE_MASK) == WRAPTYPE))))
			      
		    templinetypes[lx] = TRASHTYPE;
		else if (lx > 0 && l0[-1] < 25
				&& lch(templines[lx - 1], " * Origin:"))
		    templinetypes[lx] = TRASHWRAPTYPE;   /* a guesswrap */
		else
		    break;
	}
	while (lx < linct) {
	    if (templinetypes[lx] == TRASHTYPE) {
		if (!AllBlank(templines[lx]))
		    break;
		templinetypes[lx] = UNKNOWNTYPE;
	    }
	    lx++;
	}		/* detect fido ^A kluge lines: */
	for (lx = !templines[0]; lx < linct; lx++) {
	    l0 = templines[lx];
	    if (AllBlank(l0) || *l0 != '\x01' || !isalpha(l0[1])
				|| templinetypes[lx] != UNKNOWNTYPE)
		break;
	    if ((len0 = l0[-1]) >= 6 && (!strncmp(l0, "\x01\FMPT ", 6)
				|| !strncmp(l0, "\x01TOPT ", 6)
				|| !strncmp(l0, "\x01INTL ", 6)
				|| !strncmp(l0, "\x01\FLAGS", 6))) {
		templinetypes[lx] = TRASHTYPE;	/* no colon on these guys */
		continue;
	    }
	    while (len0 && (isupper(*++l0) || *l0 == '-'))
		len0--;
	    if (!len0 || *l0 != ':')
		break;
	    templinetypes[lx] = TRASHTYPE;
	}
    } else
	lx = 0;
    for (spaces = /* existing */ lx; lx < linct; lx++) {  /* spot RFC headers */
	l0 = templines[lx];
	if (AllBlank(l0) || templinetypes[lx] != UNKNOWNTYPE)
	    break;
	len0 = l0[-1];
	if ( /* bw_inet */ !qwk && *l0 == '\x01') {
	    l0++, len0--, ayed = true;
	    if (*l0 == '\x01' && len0 > 0)	/* ack, double ^As happen! */
		l0++, len0--;
	} else if (nowreloading)
	    break;
#ifdef GEORGES_GODDAMN_DOOR_ISNT_BROKEN
	else if (ayed)		/* no non-^A lines recognized after one ^A one */
	    break;
#endif						/* ack, absent ^As happen too! */
	if (lx > spaces || len0 < 5 || strnicmp(l0, "From ", 5)) { 
	    if (!(l1 = strnchr(l0, ':', len0)) || l1 - l0 >= len0) {
		if (!addressed && !eyedeed)
		    lx = 0;
		break;
	    }
	    len = l1 - l0;
	    if (!addressed)
		addressed = (len == 10 && !strnicmp(l0, "Newsgroups", 10)) ||
				(len == 11 && !strnicmp(l0, "Followup-to", 11));
	    if (!minaddressed && !nowreloading)
		if ((minaddressed = len == 2 && !strnicmp(l0, "To", 2)) && ayed)
		    addressed = true;
	    if (!eyedeed)
		eyedeed = (len == 10 && !strnicmp(l0, "Message-ID", 10)) ||
				(len == 10 && !strnicmp(l0, "References", 10));
	    if (!frommed && !nowreloading)
		frommed = len == 8 && !strnicmp(l0, "Reply-to", 8);
	    if (!minfrommed && !nowreloading)
		if (searchlight_ie) {
		    if (minfrommed = len == 8 && !strnicmp(l0, "Internet", 8))
			frommed = true;
		} else if ((minfrommed = len == 4 &&
					!strnicmp(l0, "From", 4)) && ayed)
		    frommed = true;
	    while (l0 < l1)
		if (!isgraph(*l0++)) {
		    if (!addressed && !eyedeed)
			lx = 0;
		    goto brake;
		}
	}
	while (!nowreloading && lx + 1 < linct && !AllBlank(l1 = templines[lx + 1])
				&& (!ayed ? isspace(l1[0]) : (l1[0] == '\x01'
				  && isspace(l1[1])) || (templinetypes[lx + 1]
					& TYPE_MASK) == WRAPTYPE))
	    lx++;
    }
  brake:
    if (bw_inet && nowreloading && !eyedeed) {
	frommed = addressed = minfrommed = false;
	spaces = linct;
	for (lx = linct - 1; (signed) lx >= 0; lx--) {
	    l0 = templines[lx];
	    if (!l0 || ((len = templinetypes[lx] & TYPE_MASK) != WRAPTYPE &&
				(*l0 != '\x01' || !strnchr(l0, ':', l0[-1]))))
		break;
	    if (len < WRAPTYPE)
		spaces = lx;		/* earliest valid reply RFC */
	    if (l0[-1] >= 12) {
		if (!eyedeed)
		    eyedeed = !strnicmp(l0, "\x01References:", 12);
		if (!addressed)
		    addressed = !strnicmp(l0, "\x01Newsgroups:", 12);
	    }
	}
	lx = linct;
    }
    prespaces = glyphs = 0;	 /* pull BW ^A-RFC lines back out of replies: */
    tempRFCs = tempgroups = null;
    barred = false;
    if ((frommed || eyedeed || addressed || (minfrommed && minaddressed)) /* &&
				(lx == linct || AllBlank(templines[lx])) */ )
	for (len0 = spaces; len0 < lx; len0++) {
	    c = templinetypes[len0] & GAP_MASK;
	    d = (templinetypes[len0] & TYPE_MASK) == WRAPTYPE
					? RFCWRAPTYPE : RFCHEADERTYPE;
	    templinetypes[len0] = d | c;
	    l0 = templines[len0];
	    if (l0 && nowreloading) {
		if (d == RFCHEADERTYPE)
		    barred = !strnicmp(l0, "\x01Newsgroups:", 12);
		if (barred)
		    glyphs += l0[-1] + 2;
		else
		    prespaces += l0[-1] + 2;
	    } else if (l0 && l0[0] == '\x01')
#ifndef DONT_USE_LINEPOOL
		if (l0[-1] <= 1)
		    templines[len0] = null;
		else {
		    l0[0] = l0[-1] - 1, templines[len0]++;
		    if (l0[1] == '\x01' && l0[0] > 0)	/* double ^A! */
			l0[1] = l0[0] - 1, templines[len0]++;
		}
#else
		if (l0[-1] <= 1) {
		    Vfree(l0);
		    templines[len0] = null;
		} else if (l1 = BValloc(l0[-1] - 1)) {
		    strncpy(l1, l0 + 1, l1[-1]);
		    templines[len0] = l1;
		    Vfree(l0);
		}
#endif
	    }
    barred = false;
    l1 = prespaces ? (tempRFCs = NewPoolString((ushort) prespaces)) : null;
    l2 = glyphs && glyphs < SIGNATURELEN + 14 ? (tempgroups =
					NewPoolString(SIGNATURELEN + 2)) : null;
    if (l1 || l2) {
	for (lx = 0; lx < linct; lx++)
	    if ((l0 = templines[lx]) && ((d = templinetypes[lx] & TYPE_MASK)
					== RFCHEADERTYPE || d == RFCWRAPTYPE)) {
		if (d == RFCHEADERTYPE) {
		    c = !strnicmp(l0, "\x01Newsgroups:", 12);
		    if (c != barred) {
			ustr t;
			t = l1; l1 = l2; l2 = t;
			barred = c;
		    }
		}
		if (!l1)
		    continue;
		if (l1 > (barred ? tempgroups : tempRFCs))
		    if (d != RFCWRAPTYPE)
			*l1++ = '\r', *l1++ = '\n';
		    else if (l1[-1] != ' ' && templinetypes[lx - 1] & GAP_MASK)
			*l1++ = ' ';
		len0 = l0[-1];
		if (d == RFCHEADERTYPE && barred) {
		    l0 += 13;
		    len0 -= 13;
		    while (len0 > 0 && *l0 == ' ')
			len0--, l0++;
		}
		strncpy(l1, l0, len0);
		l1 += len0;
		memmove(templines + lx, templines + lx + 1,
					(linct - lx - 1) * sizeof(ustr));
		memmove(templinetypes + lx, templinetypes + lx + 1,
					linct - lx - 1);
		lx--;		/* nuke the line! */
		linct--;
	    }
	if (barred)
	    l0 = l1, l1 = l2, l2 = l0;
	if (l1) strcpy(l1, "\r\n");
	if (l2) *l2 = '\0';
    }
    len = len0 = -2;
    barred = false;
    for (spaces = lx = 0; lx < linct; lx++)
	if ((l0 = templines[lx]) && (*l0 == ':' || *l0 == '|')) {
	    if (++spaces >= 10) {
		barred = true;
		break;
	    }
	    if (lx == len + 1) {
		if (++len - len0 >= 3) {
		    barred = true;
		    break;
		}
	    } else
		len = len0 = lx;
    }
    for (lx = linct - 1; lx >= 0; lx--) {
	d = templinetypes[lx] & TYPE_MASK;
	if (d >= GUESSWRAPTYPE       /* limited to one wrap for trash lines */
			&& (templinetypes[lx - 1] & TYPE_MASK) == TRASHTYPE)
	    templinetypes[lx] = TRASHWRAPTYPE | (d & GAP_MASK);
	if (!(l0 = templines[lx]) || d != UNKNOWNTYPE)
	    continue;
	len = l0[-1];
	if (len > 12)
	    len = 12;
	if ((*l0 == ':' || *l0 == '|') && barred)
	    goto markit;
	for (prespaces = spaces = glyphs = 0; len && prespaces <= 2
				&& spaces <= 1 && glyphs <= 4; l0++, len--)
	    if ((c = *l0) == '>')
		goto markit;
	    else if (c == ' ') {
		if (glyphs)
		    spaces++;
		else
		    prespaces++;
	    } else if (c == '<')
		break;
	    else
		glyphs++;
	if (d == UNKNOWNTYPE)
	    templinetypes[lx] = BODYTYPE | (d & GAP_MASK);
	continue;
      markit:
	templinetypes[lx] = QUOTETYPE | (d & GAP_MASK);
	for (len0 = lx + 1; len0 < linct; len0++)
	    if ((templinetypes[len0] & TYPE_MASK) == WRAPTYPE)
		templinetypes[len0] = QUOTEWRAPTYPE
					| (templinetypes[len0] & GAP_MASK);
	    else
		break;
    }
    for (lx = linct - 1; lx > 0; lx--) {
	if (!(l1 = templines[lx]) || !(len = l1[-1]))
	    continue;
	if ((templinetypes[lx] & TYPE_MASK) != BODYTYPE)
	    continue;
	for (spaces = lx - 1; ; spaces--) {
	    if (spaces < 0 || !(l0 = templines[spaces]))
		goto notthisline;
	    len0 = l0[-1];
	    prespaces = templinetypes[spaces] & TYPE_MASK;
	    if (prespaces == QUOTETYPE)
		goto notthisline;
	    if (prespaces != WRAPTYPE)
		break;
	}
	if (len0 < GUESSEDGE && spaces == lx - 1)
	    continue;
	for (spaces = 0; spaces < len; spaces++) {
	    if ((c = l1[spaces]) != ' ')
		break;
	    if (spaces >= NOWRAPINDENT)
		goto notthisline;
	}
/***** handle non-trash cases of "..."?  Handle "--"? */
	if (!isalnum(c) && c != '(' && c != '"' && c != '\'' && c != '[')
	    goto notthisline;
	for (prespaces = 0; prespaces < len0; prespaces++) {
	    if (l0[prespaces] != ' ')
		break;
	    if (prespaces > spaces + 8)
		goto notthisline;
	}
	if (prespaces < len0 && prespaces >= spaces)
	    templinetypes[lx] = GUESSWRAPTYPE | (templinetypes[lx] & GAP_MASK);
      notthisline:
	;
    }
}


/* qeol = 0 for CRLF text, = 1 for QEOL newlines, = 2 for Amiga newlines */
/* trim = true when you want to strip ANSI and so on */

bool LoadMessage(struct Mess *mm, BPTR hand, short qeol, bool trim)
{
    short lx;
    bool tagd = false;
    ustr ll;

    if (mm->bits & LOADED || !(llim = mm->datflen))
	return true;
#ifdef BETA
    if (!hand) {
	Err("INTERNAL ERROR!  Attempt to load\nmessage from null file handle.");
	return false;
    }
#endif
    filerr = 0;
    if (AvailMem(0) < 70000 /* semi-random */)
	MakeRoom(false);
/* we seek one char past the spot because BW has an  extra leading      */
/* space character in there, and we just do everything else off by 1... */
    if (SSeek(hand, mm->datfseek + 1, OFFSET_BEGINNING) < 0) {
	SeekErr("reading message text");
	return false;
    }
    otemplines = templines, otemplinetypes = templinetypes;
    if (!LoadMessageText(mm, hand, qeol, trim)) {
	FreeTempTempLines();
	return false;
    }
    if (Lanyansi)
	mm->bits |= ANSI;
    if (llim > 0 && llim < 10000000 && Llen < 0)
	filerr = IoErr();
    if (filerr) {
	SetIoErr(filerr);
	DosErr("Could not read all of the current message.");
    }
    if (qeol == 1) {
	short tl, tx;
	while (linct > 0 && AllBlank(templines[linct - 1])) {
	    if (templines[--linct]) {
#ifdef QWK_REVISE_SIZE
		mm->datflen -= templines[linct][-1];
#endif
#ifdef DONT_USE_LINEPOOL
		Vfree(templines[linct]);
#endif
	    }
#ifdef QWK_REVISE_SIZE
	    mm->datflen--;
#endif
	}
	if (linct) {
	    ll = templines[linct - 1];
	    tx = ll[-1];
	    for (tl = tx; tl && isspace(ll[tl - 1]); tl--) ;
	    if (tl < tx) {
#ifdef DONT_USE_LINEPOOL
		register str temptemp;
		if (temptemp = BValloc(tl)) {
		    strncpy(temptemp, ll, tl);
		    Vfree(templines[linct - 1]);
		    ll = templines[linct - 1] = temptemp;
		} else
		    memset(ll + tl, ' ', tx - tl);
#else
		templines[linct - 1][-1] = tl;
#endif
#ifdef QWK_REVISE_SIZE
		mm->datflen -= tx - tl;
#endif
	    }
	    if (hand != texthand && linct && lch(ll, TEARGHEAD) && tl < 25
				&& tl - iindexn(ll + 2, tl - 2, '*') <= 3) {
#ifdef DONT_USE_LINEPOOL
		Vfree(templines[linct]);
#endif
		if (--linct && !templines[linct - 1])
		    linct--;
	    }
	}
    } else if (linct && AllBlank(templines[linct - 1]))
#ifdef DONT_USE_LINEPOOL
	Vfree(templines[--linct]);
#else
	linct--;
#endif
    if (linct) {
	FigureOutQuoteWrap(Confind(mm->confnum));
	lx = linct;
	if (nowreloading && linct > 1 && AllBlank(templines[linct - 2])
				&& (lch(templines[linct - 1], " * Q-Blue")
				    || lch(templines[linct - 1], "... ")))
	    mm->bits |= REPLY_HAS_TAG;
	else if (nowreloading || qeol == 2)
	    lx += TAGLINEPAD;		/* make space if none there already */
	if (!(mm->lines = Valloc(lx * 4)) || !(mm->linetypes = Valloc(lx))) {
	    Vfree(mm->lines);
#ifdef DONT_USE_LINEPOOL
	    for (lx = 0; lx < linct; lx++)
		Vfree(templines[lx]);
#else
	    FreePool(mm->linepool);
	    mm->linepool = null;
	    mm->poolindex = 0;
#endif
	    if (!lstifle)
		Err("Insufficient memory to\nload text of message.");
	    mm->lines = (adr) mm->linetypes = null;
	    FreeTempTempLines();
	    return false;
	}
	for (lx = 0; lx < linct; lx++) {
	    mm->lines[lx] = templines[lx];
	    mm->linetypes[lx] = templinetypes[lx];
	}
    } else if (nowreloading || qeol == 2)		/* room for tagline */
	if (!(mm->lines = Valloc(TAGLINEPAD * 4))
				|| !(mm->linetypes = Valloc(TAGLINEPAD))) {
	    Vfree(mm->lines);
	    mm->lines = (adr) mm->linetypes = null;
	    /* return false; */		/* nah -- just fail to add tagline */
	}
    mm->linect = linct;
    mm->bits |= LOADED;
    FreeTempTempLines();
#ifdef BETA
    if (mm->bits & DONTSTRIP)
	forceloadcount++;
    else
	loadmesscount++;
#endif
    return true;
}


bool LoadPktMessage(struct Mess *mm)
{
    if (mm->bits & DONTSTRIP)
	return true;			/* replies */
    else if (mm->bits & ISBULLETIN)
	return LoadMessage(mm, (BPTR) mm->from, 0, true);
    else
	return LoadMessage(mm, texthand, qwk, true);
}


void PreloadConf(struct Conf *cc, ushort which)
{
    short i;
    rupted = false;
    lstifle = true;
    for (i = which + 1; i < cc->messct && i - which < PRELOADMAX; i++)
	if (AvailMem(0) < PRELOADSPACE || !LoadPktMessage(cc->messes[i])
			|| (rupted = !!idcort->mp_MsgList.lh_Head->ln_Succ))
	    break;	/* LoadMessage(cc->messes[i], texthand, qwk, true) ^^ */
    lstifle = false;
#ifdef SEE_PRELOAD_COUNT	/* not compatible with title bar clock */
{ char buf[10];
  static short lasti = -1;
  static struct Conf *lastcc = null;
  i -= which;
  if (i <= lasti && cc == lastcc) return;
  sprintf(buf, "%ld+", (long) i);
  RenderUR(buf);
  lasti = i;
  lastcc = cc;
}
#endif
}


void SuperStripConf(struct Conf *cc)
{
    short i;
    register struct Mess *mm;

    for (i = 0; i < cc->messct; i++)
	if (mm = cc->messes[i]) {
	    StripMessage(mm);
	    if (mm->attached)
		FREE(mm->attached);
	}
}


void FreeReply(struct Mess *mm)
{
    if (!mm)
	return;
    StripMessage(mm);
    Vfree(mm->from);
    Vfree(mm->too);
    Vfree(mm->subject);
    if (mm->attached)
	FREE(mm->attached);
    FREE(mm);
}


void FlushPacket(void)
{
    short a, i, ogm;
    struct Conf *cc;
    struct Mess *mm;

    ogm = FlipBGadgets(0);
    PortOff();
    UnSearch(&readareaz, false);
    SaveBookmarks();
    FlushExtraDoorCommands();
    for (i = 0; i < replies.messct; i++) {
	Vfree(oldfilenames[i]);		/* messageID's are freed by FreePool */
	if (mm = replies.messes[i])
	    FreeReply(mm);
    }
    ZeroOldfilenamesEtc();
    SuperStripConf(&personals);
    for (a = 0; a < bullstuff.messct; a++)
	if ((mm = bullstuff.messes[a]) && mm->from) {
	    Close((BPTR) mm->from);
	    mm = null;
	}
    if (bullstuff.messes[0] != &placeholder)
	SuperStripConf(&bullstuff);
    replies.messct = bullstuff.messct = personals.messct = 0;
    for (a = 0; a < areaz.messct; a++)
	if (cc = areaz.confs[a]) {
	    SuperStripConf(cc);
	    Vfree(cc->messes);
	}
    Vfree(readareazfree);
    Vfree(tempcurrents);
    Vfree(areaz.confs);
    Vfree(inuseareaz.confs);
    areaz.confs = inuseareaz.confs = readareaz.confs = readareazfree = null;
    tempcurrents = null;
    areaz.messct = inuseareaz.messct = readareaz.messct = 0;
    replies.morebits = bullstuff.morebits = personals.morebits = 0;
    for (i = 0; i < 10; i++) {
	requestedfiles[i][0] = 0;
	bwkeywords[i][0] = 0;
	bwfilters[i][0] = 0;
    }
    macro1[0] = macro2[0] = macro3[0] = bwpassword[0] = 0;
    filerequests_exist = pdq_exists = doorwarnedonce = searchlight_ie = qwkE
				= galactiwarn = inverse_netkluge = pcboard_ie
				= personalityflaw = official_iemail = qwkE_subs
				= searchlight = pcboard_hood = no_forward
				= exceed_dos = false;
    FreePool(confPool);
    FreePool(messPool);			/* gets the messReturnees ones too */
    FreePool(stringPool);
    /* do NOT free stringPermPool until exit time. */
    confPool = (adr) messPool = (adr) messReturnees = (adr) stringPool = null;
    confIndex = CONFPUDDLE;
    messIndex = MESSPUDDLE;
    stringIndex = STRINGPUDDLE;
    if (texthand) {
	Close(texthand);
	texthand = 0;
    }
#ifdef WARN_LONG_REPLY
    lastlongwarn = -1;
#endif
    if (!quitleaveopen && !fakery)
	EmptyDir(workdirinst, false);
    if (flushreplies && !repchanges && !quitleaveopen)
	EmptyDir(replydirinst, false);
    if (tryunpackcd) {
	UnLock(tryunpackcd);
	tryunpackcd = 0;
    }
    if (editoutinst[0])
	DeleteFile(editoutinst);
    if (editininst[0])
	DeleteFile(editininst);
    PortOn();
    FlipBGadgets(ogm);
#ifdef BETA
    if (i = loadmesscount + forceloadcount + quotifycount
			- stripmesscount - forcestripcount) {
	Err("%ld DONTSTRIP loaded, %ld regular loaded, %ld quotified,\n%ld "
			"DONTSTRIP stripped, %ld other stripped, %ld LEFT OVER.",
			forceloadcount, loadmesscount, quotifycount,
			forcestripcount, stripmesscount, i);
	loadmesscount = forceloadcount = quotifycount
			= stripmesscount = forcestripcount = 0;
    }
#endif
}
