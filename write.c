/* stuff for actually producing a new Q-Blue message */

#include <dos/dosextens.h>
#include <dos/dostags.h>
#include <intuition/intuition.h>
#include "qblue.h"
#include "version.h"


#define STEALEN     10
#define ARROWLEFT   -16


bool MakeLotsaRoom(void);
BPTR CreateLock(str dirname);
short Conf2ix(struct Conf *cc);
ulong DS2ux(struct DateStamp *d);
long ShellExecute(str command, BPTR in, BPTR out);
bool AskStripHighBits(ushort count);
bool CreateReplyLock(void);
ustr NewPoolString(ushort length);
ustr NewMessPoolLine(struct Mess *mm, ubyte len);

void StripMessage(struct Mess *thing);
bool LoadMessage(struct Mess *mm, BPTR hand, short qeol, bool trim);
bool LoadPktMessage(struct Mess *mm);
bool SaveToFile(struct Mess *thing, str filename, short whichreply);
bool WriteUPL(void);
bool CNewArea(bool ask, bool netflippin);
void OpenBitsWin(void), CloseBitsWin(void);
void AbleAddGad(struct Gadget *gg, struct Window *ww, bool ability);
void ActivateGag(struct Gadget *gg, struct Window *ww);
void FormatNetAddress(str buf, ushort zone,
			ushort net, ushort node, ushort point);
void JamNewSubject(ustr ll, short lx);
bool NewsgroupSyntaxError(ustr start);
bool ReallyInNetmail(void);

void AdjustToNewArea(bool muffle);
void ShowHeader(struct Conf *cc, struct Mess *mm);
void SwitchRGag(bool replies, bool deleted);
void TweakCloseGag(void);
void FixStringGad(struct Gadget *gg, struct Window *ww);
#ifdef WARN_LONG_REPLY
void WarnLongMess(short n);
#endif
bool AllBlank(ustr line);
bool AllBlankN(ustr line, ushort len);
bool Substitutions(str after, str fullname, str filesdir, str before,
			bool editing, bool un, bool rep);
void GiveItATaglineMaybe(void);
bool FinalizeTagline(struct Mess *who, short whichreply);
bool RemoveTaglineFromReply(struct Mess *mm, short whichreply, str dump);
ushort UniqueRix(struct Mess *self);
void GetBWReplyFilename(str fame, struct Mess *mm);

ulong Text2ux(str date);
bool AnyLower(str s);
void SetAllSharedWindowTitles(str title);
bool AskRetryWriteUPL(void);
str LastStart(str name);
void UnMask(str s);
bool CopyFile(BPTR inh, str outpath);
bool IdenticalPaths(str a, str b);


import struct Gadget cbgaghold, cbgagkill, cbgagfreq, cbgagfatt, cbgagdirect;
import struct Gadget cbgagcrash, cbgagimmed, cgagedit, cgagsave, cgagquote;
import struct Gadget cgagpriv, cgagnet, cgagfrom, cgagtoo, cgagsubj;
import struct StringInfo cstrsubj, cstrtoo;
import struct IntuiText cgtpublic, cgtprivate;

import struct Mess *fixee, *replyeee, *carb;
import struct Conf *curconf;
import BPTR fakepath;
import ustr *oldfilenames, *messageID, *newsgroups;

import ubyte cbufnet[], cbuffrom[], cbuftoo[], cbufsubj[];
import char edit2command[], edit1command[], signature[], localsignature[];
import char r2ename[], quoteheader[], localquoteheader[], carbonheader[];
import char fidogate[], title[], oldflame[], x_version[];
import str anynym, xcommand;

import ulong loaded_size;
import short fromend, defaultarea;
import short localmailareanum, netmailareanum, localwrapmargin;
import ushort netbits, hostnetmailflags, czone, cnet, cnode, cpoint;
import ushort hostzone, hostnet, hostnode, hostpoint;
import ushort oldracount, quotype, wrapmargin, which_reedit;

import bool firstedit, fixingreply, uppitynames, uppitytitle, deferflipb;
import bool dubedit, backbefore, frontafter, selfcarbon, ie_is_gated;
import bool pcboard_ie, searchlight_ie, allowblanksubj, indent_XX, editstrip;

#ifdef BETA
import short quotifycount;
#endif


bool qed = false, repchanges, complained, notoowarned, postlinkage;
#ifdef WARN_LONG_REPLY
short lastlongwarn = -1;
#endif

local struct Mess *qsrc, *qdest;
local char xQbuf[82], xSpare[SUBJLEN];
local short d_x, xQbc, truncstart, truncend, qheader_size;
local bool truncate, filter_qheader;


void DateString(struct DateStamp *d, str s)
{
    static char months[12][4] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun",
				  "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
    static short smods[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    short year, mday, month, yell, hour, minute, second;

    year = 78 + ((d->ds_Days / 1461) << 2);
    mday = d->ds_Days % 1461;
    while (mday >= (yell = (year & 3 ? 365 : 366))) {
	mday -= yell;
	year++;
    }
    /* 2000 is a leap year?, 1900 and 2100 are <invalid> */
    smods[1] = (year & 3 ? 28 : 29);
    month = 0;
    while (mday >= smods[month])
	mday -= smods[month++];
    year %= 100;
    hour = d->ds_Minute / 60;
    minute = d->ds_Minute % 60;
    second = d->ds_Tick / TICKS_PER_SECOND;
    if (qed) {
	char am;
	if (hour > 12) hour -= 12, am = 'P';
	else am = 'A';
	if (!hour) hour = 12;
	sprintf(s, "%ld %s %02ld", mday + 1L, months[month], (long) year);
	sprintf(strend(s) + 1, "%ld:%02ld %lcM", (long) hour,
					(long) minute, (long) am);
    } else if (qwk)
	sprintf(s, "%02ld-%02ld-%02ld %02ld:%02ld", month + 1L, mday + 1L,
			(long) year, (long) hour, (long) minute);
    else
	sprintf(s, "%02ld %s %02ld %02ld:%02ld:%02ld",  /* Fido format */
			mday + 1L, months[month], (long) year,
			(long) hour, (long) minute, (long) second);
}


void ux2DS(struct DateStamp *d, ulong inn)
{
    long secs = inn % 86400;
    d->ds_Days = (inn / 86400) - 2922;
    d->ds_Minute = secs / 60;
    d->ds_Tick = (secs % 60) * TICKS_PER_SECOND;
}


/* dammit, I hate doing these fucking ascii state machines... */

bool ValidInternet(str where)
{
    ushort parendepth = 0;
    bool isquoted = false, isslashed = false, isbracketed = false;
    bool wasspecial = true, spaced = true, atted = false;

    while (*where && isspace(*where))
	where++;
    while (*where) {
	register ubyte c = *where++;
	if (parendepth > 0 || isbracketed || isquoted) {
	    if (isslashed)
		isslashed = false;
	    else if (c == '\\')
		isslashed = true;
	    else if (parendepth > 0 && c == '(')
		parendepth++;
	    else if (parendepth > 0 && c == ')')
		parendepth--;
	    else if (isbracketed && c == '[')
		return false;
	    else if (isbracketed && c == ']')
		isbracketed = false;
	    else if (isquoted && c == '"')
		isquoted = false;
	} else if (c == ' ' || c == '\t')
	    spaced = true;
	else if (!isprint(c))		/* no control chars or upper ascii */
	    return false;
	else {
	    if (c == '\\' || c == '<' || c == '>' || c == ')' || c == ']')
		return false;		/* not allowed in atoms */
	    else if (c == '(') {
		parendepth++;
		spaced = true;		/* wasspecial preserved */
	    } else if (c == '[') {
		if (!wasspecial)
		    return false;
		wasspecial = false;
		isbracketed = true;
	    } else if (c == '"') {
		if (!wasspecial)
		    return false;
		wasspecial = false;
		isquoted = true;
	    } else if (c == '@') {
		if (atted | wasspecial)
		    return false;
		wasspecial = atted = true;
	    } else if (c == ',')
		return !wasspecial && atted;	/* "foo@bar, zot@quux" */
	    else if (c == '.' || c == ':' || c == ';') {
		if (wasspecial)
		    return false;
		wasspecial = true;
	    } else {			/* other punct, and alphanumeric */
		if (spaced && !wasspecial)
		    return false;
		wasspecial = false;
	    }
	    spaced = false;
	}
    }
    return atted && !parendepth && !isquoted && !isbracketed && !wasspecial;
}


bool NormalizeInternet(ustr raw, ustr cooked, short len)
{
    bool isquoted = false, isslashed = false, isbracketed = false;
    short parendepth = 0;
    ustr go = cooked, rawstart = raw;
    register ubyte c;

    while (c = *raw++) {
	if (parendepth > 0 || isbracketed || isquoted) {
	    if (!parendepth)		/* copy all except comments */
		*go++ = c, len--;
	    if (isslashed)
		isslashed = false;
	    else if (c == '\\')
		isslashed = true;
	    else if (parendepth > 0 && c == '(')
		parendepth++;
	    else if (parendepth > 0 && c == ')')
		parendepth--;
	    else if (isbracketed && c == ']')
		isbracketed = false;
	    else if (isquoted && c == '"')
		isquoted = false;
	} else {
	    if (c == '(')
		parendepth++;
	    else if (c == '"')
		isquoted = true;
	    else if (c == '[')
		isbracketed = true;
	    else if (c == ',') {		/* discard remainder */
		raw++;				/* force return true */
		break;
	    }
	    if (!isspace(c) && c != '(')	/* all but comments & spaces */
		*go++ = c, len--;
	}
	if (len <= 0) {				/* overflow: fail */
	    *cooked = 0;
	    return true;
	}
    }
    *go = 0;
    return raw - rawstart != go - cooked;
}


bool ExtractFromAddress(struct Mess *mm, str space, short len, bool backwards)
{
    bool frommed = false, subjected = cstrsubj.MaxChars <= 26;
    bool isrt, suck = false, oneblank = false;
    short m, lx, headerlen, subheaderlen, hlen;
    ustr ll, gre, header, header2, subheader = "Subject: ";
    char bluff[160];

    if (pcboard_ie) {
	if (backwards)
	    header = "\xFF@TO     :", header2 = "\xFF@TO2    :";
	else
	    header = "\xFF@FROM   :", header2 = "\xFF@FROM2  :";
	subheader = "\xFF@SUBJECT:";
    } else if (backwards)
	header = "To: ";
    else
	header = searchlight_ie ? "Internet: " : "From: ";
    headerlen = strlen(header);
    subheaderlen = strlen(subheader);
    for (m = 0; m < mm->linect; m++) {
	if ((lx = mm->linetypes[m] & TYPE_MASK) == RFCWRAPTYPE
				|| lx == TRASHTYPE || lx == TRASHWRAPTYPE)
	    continue;
#ifdef TRUST_RFCNESS
	if (!(ll = mm->lines[m]) || lx !=  RFCHEADERTYPE)
	    return suck;		/* fail, if space not filled yet */
	lx = ll[-1];
#else
	if (!oneblank && (AllBlank(ll = mm->lines[m]) || (!isspace(ll[0]) &&
				!strnchr(ll, ':', ll[-1]))) && lx <= WRAPTYPE)
	    return suck;
	lx = ll[-1];
	if (ll && lx && *ll == '\x01' && !qwk)
	    ll++, lx--;
	if (AllBlankN(ll, lx)) {
	    oneblank = true;
	    continue;
	}
#endif
	if (!subjected && lx > 9 && !strnicmp(ll, subheader, subheaderlen)) {
	    for (ll += subheaderlen, lx -= subheaderlen;
					lx > 0 && isspace(*ll); ll++, lx--) ;
	    if (lx > strlen(cbufsubj))
		JamNewSubject(ll, lx);
	    if (frommed)
		return true;
	    subjected = true;
	    continue;
	}
	if (frommed || ((lx <= headerlen || strnicmp(ll, header, headerlen))
			    && (lx <= 10 || strnicmp(ll, "Reply-to: ", 10))))
	    continue;
	isrt = _toupper(ll[0]) == 'R';
	hlen = isrt ? 10 : headerlen;
	if (pcboard_ie && !isrt && lx > 70) lx = 70;
	strncpy0(bluff, ll + hlen, lx - hlen);
	if (m < mm->linect - 1 && !AllBlank(ll = mm->lines[m + 1]) &&
			((pcboard_ie && !isrt) || (isspace(ll[0]) &&
			 ((mm->linetypes[m + 1] & TYPE_MASK) == RFCHEADERTYPE ||
			  (mm->linetypes[m + 1] & TYPE_MASK) == RFCWRAPTYPE)))) {
	    if (pcboard_ie && !isrt) {
		if ((lx = ll[-1]) > 10 && !strnicmp(ll, header2, 10)) {
		    if (lx > 70) lx = 70;
		    strncpy0(strend(bluff), ll + 10, lx - 10);
		}  /* We have assumed FROM and FROM2 are adjacent.  Valid? */
	    } else
		strncpy0(bluff + lx - headerlen, ll + 1, ll[-1] - 1);
	}
	if (!ValidInternet(ll = bluff))
	    if (ll = strchr(bluff, '<')) {
		if (!(gre = strrchr(++ll, '>')))
		    continue;
		*gre = 0;
		if (!ValidInternet(ll))
		    continue;
	    } else
		continue;
	suck = true;
	if (space && len)	/* pass null to just test for headers */
	    NormalizeInternet(ll, space, len);
	if (isrt) {
	    frommed = true;
	    if (subjected)
		return true;
	} /* else give it another chance to find something better */
    }
    return suck;
}


local char Glom(str word)
{
    static str magicwords[] = {
	"one", "two", "three", "four", "five", "six", "seven", "eight", "nine",
	"zero", "number", "at", "dollar", "dollars", "percent", "percenter",
	"percentage", "and", "star", "asterisk", "plus", "add", "equal",
	"equals", "equality", "slash", "slasher", "ii", "iii", "iv", "vi",
	"vii", "viii", "ix", "first", "second", "third", "fourth", "fifth",
	"sixth", "seventh", "eighth", "ninth", null
    };
    static char magichars[] = "1234567890#@$$%%%&**++===//2346789123456789";
    int i, j;

    for (i = 0; magicwords[i]; i++)
	if (!strnicmp(word, magicwords[i], j = strlen(magicwords[i]))
				&& !isalpha(word[j]))
	    return magichars[i];
    return toupper(word[0]);
}


void GetInitials(str leadin, str fromme)
{
    str space[5], hyphen[3], p;
    short spaces, hyphs, x;
    bool spaced = true, hyped = false;

    if ((x = strlen(fromme)) <= 3)
	strcpy(leadin, fromme);
    else {
	spaces = hyphs = x = 0;
	space[0] = space[1] = space[2] = hyphen[0] = hyphen[1] = null;
	for (p = &fromme[0]; *p; p++) {
	    if (isalnum(*p)) {
		if (spaced && spaces < 4)
		    space[spaces++] = p;
		else if (hyped && hyphs < 3)
		    hyphen[hyphs++] = p;
		spaced = hyped = false;
	    } else if (*p == ' ')
		spaced = true, hyped = false;
	    else if (*p == '-' || *p == '_')
		hyped = true;
	}
	if (spaces > 3) {	/* discard the shortest word */
	    register short v, u = 32767, t = 0;
	    space[spaces] = p;
	    for (x = 0; x < spaces; x++)
		if ((v = space[x + 1] - space[x]) <= u)
		    t = x, u = v;
	    p = space[t];
	    for (x = t; x < spaces; x++)
		space[x] = space[x + 1];
	    spaces--;
	    for (x = 0; x < hyphs; x++)
		while (hyphen[x] >= p && hyphen[x] < space[t]) {
		    for (u = x; u < hyphs; u++)
			hyphen[u] = hyphen[u + 1];
		    hyphs--;
		}
	}
	if (spaces) {
	    x = spaces;
	    leadin[0] = Glom(space[0]);
	    if (spaces >= 3) {
		leadin[1] = Glom(space[1]);
		leadin[2] = Glom(space[2]);
	     /*	if (spaces == 4)
		    leadin[3] = Glom(space[3]); */
		x = 3;
	    } else if (spaces == 2 && !hyphs)
		leadin[1] = Glom(space[1]);
	    else if (spaces == 2) {
		hyped = space[1] > hyphen[0];
		spaced = hyphs > 1 && space[1] > hyphen[1];
		leadin[1 + hyped + spaced] = Glom(space[1]);
		leadin[2 - hyped] = Glom(hyphen[0]);
	   /*	if (hyphs > 1) {
		    leadin[3 - spaced] = Glom(hyphen[1]);
		    x = 4;
		} else     */
		    x = 3;
	    } else if (hyphs) {
		leadin[1] = Glom(hyphen[0]);
		if (hyphs > 1) {
		    leadin[2] = Glom(hyphen[1]);
		 /* if (hyphs > 2)
			leadin[hyphs = 3] = Glom(hyphen[2]); */
		    hyphs = 2;
		}
		x = hyphs + 1;
	    } else
		x = 1;
	}
	if (x == 1 && leadin[0] == '/' && _toupper(space[0][0]) == 'S')
	    leadin[0] = 'S';		     /* band-aid: do not allow "/> " */
	if (x == 1 && isalpha(leadin[0])) {
	    short s, t = space[0] + 1 - fromme;
	    char c;
	    for (s = t; (c = fromme[s]) && !isalpha(c); s++) ;
	    if (c) t = s;
	    if (fromme[t])
		leadin[x++] = tolower(fromme[t]);
	}
    }
    strcpy(leadin + x, "> ");
}


str Capitalism(str nam)    /* 35 char limit!  may return static! */
{
    static char nn[NAMELEN];
    char c = '!';
    str ns, ws = nam;

    if (AnyLower(nam))
	return nam;
    for (ns = nn; *nam; ns++, nam++)	/* exceptions for "McFoo", "MacBar" */
	if (isalnum(c)) {
	    if (*ws == 'M' && ((ws[1] == 'A' && nam - ws == 3 && *nam != 'H' &&
				*nam != 'K') || nam - ws == 2) && nam[-1] == 'C')
		*ns = c = *nam;		/* meta-exceptions "Mach-", "Mack-" */
	    else
		*ns = tolower(c = *nam);
	} else
	    *ns = c = *nam, ws = nam;
    *ns = 0;
    return nn;
}


local str QFirstName(str nam, short part)	/* returns pointer to static! */
{
    char fist[NAMELEN], *sp;

    if (part) {
	while (*nam == ' ') nam++;
	sp = strchr(nam, ' ');
	if (!sp || AllBlankN(sp, strlen(sp)))
	    strcpy(fist, nam);
	else if (sp - nam == 3 && !strnicmp(nam, "THE", 3))
	    strcpy(fist, nam + 4);
	else if (part > 0) {
	    strcpy(fist, LastStart(nam));
	    if (sp = strchr(fist, ' ')) {
		*sp = 0;
		if (*--sp == ',')
		    *sp = 0;
	    }
	} else
	    strncpy0(fist, nam, sp - nam);
	nam = fist;
	while (*nam == ' ') nam++;
	Stranks(nam);
    }
    return Capitalism(nam);
}
/* part > 0 means last name, < 0 means first name, = 0 means full name. */


local str QSplitDT(struct Mess *src, bool time)
{					/* returns pointer to static! */
    struct DateStamp d;
    char ds[24];

    if (src) {
	if (!src->unixdate)
	    src->unixdate = Text2ux(src->date);
	ux2DS(&d, src->unixdate);
    } else
	DateStamp(&d);			/* for signatures */
    qed = true;
    DateString(&d, ds);
    qed = false;
    return time ? strend(ds) + 1 : ds;
}


local bool FinishLine(struct Mess *dest)
{
    if ((ulong) dest->lines[-1] <= dest->linect * 4)
	return false;
    while (xQbc && xQbuf[xQbc - 1] == ' ')
	xQbc--;
    if (xQbc) {
#ifndef DONT_USE_LINEPOOL
	if (!(dest->lines[dest->linect] = NewMessPoolLine(dest, xQbc)))
#else
	if (!(dest->lines[dest->linect] = BValloc(xQbc)))
#endif
	    return false;
	strncpy(dest->lines[dest->linect], xQbuf, xQbc);
	if (filter_qheader) {
	    register short c;
	    register ustr p = dest->lines[dest->linect];
	    for (c = xQbc; c > 0; c--, p++)
		if (*p < ' ' || *p > '~')
		    *p = '*';
	}
    } else
	dest->lines[dest->linect] = null;
    if (dest->linetypes)
	dest->linetypes[dest->linect] = BODYTYPE;
    dest->linect++;
    qheader_size += xQbc + 1;	/* <= watch out if ever called by any */
    xQbc = 0;			/* function other than ExpandQHeader  */
    truncstart = truncend = -1;
    return true;
}


/******* fuck around with WRAPTYPE when wrapmargin == 0? */

local bool EatQ(struct Mess *dest, str werd)
{
    char c, rap[81];
    short wmar = localwrapmargin < 0 ? wrapmargin : localwrapmargin;
    short rl, wl = strlen(werd), marge = wmar ? wmar : 80;
    short tl = truncend - truncstart;

    if (xQbc + wl > 80) {
	if (tl < 0) {
	    strncpy(xQbuf + xQbc, werd, 80 - xQbc);
	    truncend = 80;
	    return true;
	} else if (truncend > 0 && !AllBlankN(xQbuf + truncstart, tl)
						&& tl > (rl = xQbc + wl - 80)) {
	    while (truncend > truncstart && xQbuf[truncend - rl - 1] == ' ')
		rl++;
	    strncpy(xQbuf + truncend - rl, xQbuf + truncend, xQbc - truncend);
	    xQbc -= rl;
	    truncend -= rl;
	    return EatQ(dest, werd);
	} else if (truncend > 0) {
	    strncpy0(rap, xQbuf + truncend, xQbc - truncend);
	    xQbc = truncend;
	    truncend = truncstart = -1;
	    return EatQ(dest, xSpare) && EatQ(dest, rap) && EatQ(dest, werd);
	} else {
	    strncpy(xQbuf + xQbc, werd, rl = 80 - xQbc);
	    werd += rl, wl -= rl, xQbc = 80;
/***** Check this VERY carefully if wrapmargin can ever be > 80... */
	    for (rl = 81 - marge; rl < xQbc &&
			    (c = xQbuf[xQbc - rl]) != ' ' && c != '-'; rl++) ;
	    if (rl >= xQbc)
		rl = 0;
	    else {
		rl--;
		strncpy(rap, xQbuf + xQbc - rl, rl);
	    }
	    xQbc -= rl;
	    if (xQbuf[xQbc - 1] == ' ')
		xQbc--;
	    if (!FinishLine(dest))
		return false;
	    strncpy(xQbuf, rap, xQbc = rl);
	    return EatQ(dest, werd);
	}
    }
    strcpy(xQbuf + xQbc, werd);
    xQbc += wl;
    if (truncstart > truncend) {
	truncend = xQbc;
	truncate = false;
    }
    return true;
}


/* caution: assumes dest->lines points to plenty of line slots -- no test */

local void ExpandQHeader(struct Mess *dest, struct Mess *src, str head)
{
    char tmp[2], frusp[40];
    short startx = dest->linect, namepart;
    struct Conf *cc;
    str frum, tuu, subj, ccn;

    if (!*head) return;
    xQbc = tmp[1] = 0;
    if (!src /* && dest->linect */)
	FinishLine(dest);		/* simulate initial @N */
    truncate = false;
    truncstart = truncend = -1;
    while (*head) {
	if (*head == '@') {
	    head += 2;
	    namepart = 0;
	    if (src) {
		subj = src->subject;
		if (src->bits & ISBULLETIN)
		    sprintf(frum = frusp, "Bulletin %s", src->too), tuu = "All";
		else
		    frum = src->from, tuu = src->too;
	    }
	  finish_name:
	    switch (_toupper(head[-1])) {
	      case 'A':
		if (!src) goto literal;
		if (!EatQ(dest, QFirstName(frum, namepart)))
		    goto bomb;
		continue;
	      case 'R':
		if (!src) goto literal;
		if (!EatQ(dest, QFirstName(tuu, namepart)))
		    goto bomb;
		continue;
	      case 'Y':
		if (!src) goto literal;
		if (!EatQ(dest, QFirstName(cbuftoo[0] ? (str) cbuftoo
							: frum, namepart)))
		    goto bomb;
		continue;
	      case 'F':
		if (!src) goto literal;
		if (!(src->bits & ISBULLETIN))
		    namepart = -1;
		head++;
		goto finish_name;
	      case 'L':
		if (!src) goto literal;
		if (!(src->bits & ISBULLETIN))
		    namepart = 1;
		head++;
		goto finish_name;
	      case 'J':
		if (!strnicmp(subj, "Re:", 3)) {
		    subj += 3;
		    while (*subj == ' ')
			subj++;
		}
		/* FALL THROUGH: */
	      case 'S':
		if (!src) goto literal;
		strcpy(xSpare, subj);
		if (truncate)
		    truncstart = xQbc;
		if (!EatQ(dest, subj))
		    goto bomb;
		continue;
	      case 'C':
		if (!src) goto literal;
		if (cc = Confind(src->confnum))
		    ccn = LONGNAME(cc);
		else if (src->bits & ISBULLETIN)
		    ccn = "BBS bulletins";
		else
		    ccn = "(unknown)";
		strcpy(xSpare, ccn);
		if (truncate)
		    truncstart = xQbc;
		if (!EatQ(dest, ccn))
		    goto bomb;
		continue;
	      case 'D':
		if (!EatQ(dest, QSplitDT(src, false)))
		    goto bomb;
		continue;
	      case 'T':
		if (!EatQ(dest, QSplitDT(src, true)))
		    goto bomb;
		continue;
	      case 'V':
		if (!EatQ(dest, VERSION))
		    goto bomb;
		continue;
	      case 'N':
		if (!FinishLine(dest))
		    goto bomb;
		continue;
	      case 'Z':
		if (!src) goto literal;
		truncate = true;
		continue;
	      default:
		if (isdigit(head[-1])) {
		    short t = head[-1] - '0';
		    if (isdigit(*head))
			t = 10 * t + *(head++) - '0';
		    if (t >= 80)
			EatQ(dest, " ");
		    else if (t > xQbc) {
			memset(xQbuf + xQbc, ' ', t - xQbc);
			xQbc = t;
		    }
		    continue;
		}
		/* else fall out to literal: */
	    }
	  literal:
	    head--;			/* char after @ taken literally */
	}
	if (tmp[0] = *head)
	    head++;
	else
	    tmp[0] = '@';		/* final @ is not stripped */
	if (!EatQ(dest, tmp))
	    goto bomb;
    }
    if (!FinishLine(dest))
	goto bomb;
    return;
  bomb:
    if (*head) {
	while (dest->linect > startx) {
	    Vfree(dest->lines[--dest->linect]);
	    dest->lines[dest->linect] = null;
	}
	Err("Could not add header to top\nof quoted text; no memory.");
    }
}


local bool Extrude(str line, ushort ll, short loff)
{
    if (!ll)
	qdest->lines[d_x] = null;
#ifndef DONT_USE_LINEPOOL
    else if (qdest->lines[d_x] = NewMessPoolLine(qdest, ll)) {
#else
    else if (qdest->lines[d_x] = BValloc(ll)) {
#endif
	memcpy(qdest->lines[d_x], line, (size_t) ll);
#ifndef CARBON_FORBID_ONLY_SOME_FIDO_INTERFERENCE
	if (!loff)
	    if ((ll >= 10 && !strncmp(line, " * Origin:", 10))
				|| (ll >= 3 && !strncmp(line, "---", 3))
				    && (ll == 3 || line[3] == ' '))
		qdest->lines[d_x][1] = '+';
	    else if (ll >= 8 && !strncmp(line, "SEEN-BY:", 8))
		qdest->lines[d_x][4] = '+';
#else
#  ifndef FIDO_ANYTHING_GOES
	if (!loff && ll >= 3)
	    if (!strncmp(line, "---", 3) && (ll == 3 || line[3] == ' '))
		qdest->lines[d_x][1] = '+';
#  endif
#endif
	if (filter_qheader) {
	    register short c;
	    register ustr p = qdest->lines[d_x];
	    for (c = ll; c > 0; c--, p++)
		if (*p < ' ' || *p > '~')
		    *p = '*';
	}
    } else {
	qdest->lines[d_x] = null;
	if (!complained) {
	    complained = true;
	    Err("Couldn't quote it all;\nran out of memory.");
	}
	return false;
    }
    d_x++;
    return true;
}


local void Stealead(str dest, str src, ulong ll)
{
    short i = STEALEN - 1, f = 0;

    if (i >= ll) i = ll - 1;
    for ( ; i >= 0; i--)
	if (src[i] == '>') {
	    f = i + 1;
	    break;
	}
    if (f) {
	if (f < ll - 1 && src[f] == ' ')
	    f++;
	strncpy(dest, src, (size_t) f);
    }
    dest[f] = 0;
}


local short CountNs(str qh)
{
    short c = 1;
    if (!qh || !*qh)
	return 0;
    while (*qh) {
	if (*qh == '@' && qh[1]) {
	    qh++;
	    if (_toupper(*qh) == 'N')
		c++;
	}
	qh++;
    }
    return c;
}


local bool Carbonize(struct Mess *dest, struct Mess *src,
			bool noheader, bool verbatimquote)
{
    str header = !verbatimquote ? carbonheader : (localquoteheader[0]
			? localquoteheader : quoteheader);
    str siggy = dubedit || !verbatimquote ? "" :
			(localsignature[0] ? localsignature : signature);
    short i, lcl, extral = 3 * (CountNs(header) + CountNs(siggy) + 1);
    ustr ln;

    filter_qheader = verbatimquote && editstrip;
    StripMessage(qdest = dest);
    lcl = src->linect + (noheader ? 0 : extral);
    if (!(dest->lines = Valloc(lcl * 4)) || !(dest->linetypes = Valloc(lcl)))
	goto nomem;
    dest->bits |= LOADED;
    dest->linect = qheader_size = 0;
#ifdef BETA
    quotifycount++;
#endif
    complained = true;
    if (!noheader) {
	ExpandQHeader(dest, src, header);
	for (i = 0; i < dest->linect; i++)
	    dest->linetypes[i] = BODYTYPE;
    }
    d_x = dest->linect;
    for (i = 0; i < src->linect; i++) {
	dest->linetypes[d_x] = src->linetypes[i];
	if (ln = src->lines[i]) {
	    if (!Extrude(ln, ln[-1], 0))
		goto nomem;
	} else
	    dest->lines[d_x++] = null;
	if (d_x >= lcl)
	    break;
    }
    dest->linect = d_x;
    dest->datflen = src->datflen + qheader_size;
    if (siggy[0]) {
	ExpandQHeader(dest, null, siggy);
	dest->datflen += qheader_size;
    }
    return true;
  nomem:
    dest->linect = d_x;
    Err(verbatimquote ? "Couldn't quote -- no memory."
			: "Couldn't make carbon\ncopy; no memory.");
    StripMessage(dest);
    return false;
}


local short CheckIndent(struct Mess *src)
{
    short i, slti, dd, vv, dent = 256;
    ustr p;

    if (quotype == 4)
	return 0;
    for (i = 0; i < (short) src->linect; i++) {
	p = src->lines[i];
	slti = src->linetypes[i] & TYPE_MASK;
	if (p && slti != QUOTETYPE && slti != QUOTEWRAPTYPE
			    && slti != TRASHTYPE && slti != TRASHWRAPTYPE) {
	    vv = p[-1];
	    for (dd = 0; dd < vv && *p == ' '; dd++, p++) ;
	    if (dd < dent)
		dent = dd;
	    if (!dent)
		break;
	}
    }
    return dent;
}


local short HowFarToIndentWrap(short srclix, short itsawrap)
{
    short d0 = 0, d1 = 32000, ll, ll1, slti, slt1, barge = 0;
    ustr p, p1;

    for (ll1 = srclix; ll1 >= 0 && (slt1 = qsrc->linetypes[ll1]
			    & TYPE_MASK) == WRAPTYPE; ll1--) ;
    if (ll1 < 0 || !(p = qsrc->lines[ll1])) {
	p = qsrc->lines[srclix];
	slt1 = qsrc->linetypes[srclix] & TYPE_MASK;
    }
    ll = p[-1];
    for (barge = srclix + 1; barge < qsrc->linect
		    && (slti = qsrc->linetypes[barge] & TYPE_MASK)
		       == WRAPTYPE; barge++) ;
    if (barge >= qsrc->linect || !(p1 = qsrc->lines[barge])
			    || AllBlank(p1) || slti < itsawrap)
	p1 = p, slti = slt1;
/*******  Have some test of how paragraphs are generally indented? */
    if (p && slti >= itsawrap && *p == ' ') {
	if (p1 != p) {
	    d1 = 0;
	    ll1 = p1[-1];
	    while (*p1++ == ' ' && --ll1 >= 0)
		d1++;
	}
	p1 = p, ll1 = ll;
	while (*p1++ == ' ' && --ll1 >= 0)
	    d0++;
	return min(d0, d1);
    }
    return 0;
}


local short DigestNonBlankLine(ustr p, ulong ll, short *wlp, ulong linez,
			str toad, short *nwtp, short lelen, short dent,
			short slti, str leadin, ustr dlt, short marge,
			short srclix, short itsawrap)
{
    register short wl = *wlp;		/* cache */
    short ddd, nwt = *nwtp, vv, vvv, ov, xmarge, dv;
    bool fakesplit;
    static char k, kk, leadcopy[STEALEN + 2];
    static short dd, sk = 0;

    if (wl) {
	k = toad[wl - 1];
	kk = (wl == 1 ? '-' : toad[wl - 2]);
    }
    /* k value is never referenced on the first time through */
    if (wl || nwt != BODYTYPE) {		/* add space(s) for joining */
	if (k != '-' || !isalnum(kk)) {
	    toad[wl++] = ' ';
	    if (k == '.' || k == '?' || k == '!')
		toad[wl++] = ' ';
	}
    }
    if ((quotype == 2 || quotype == 3) && (slti == QUOTETYPE
						|| slti == QUOTEWRAPTYPE)) {
	lelen = 0;
	if (slti == QUOTETYPE) {
	    Stealead(leadcopy, p, ll);
	    for (vv = 0; leadcopy[vv] == ' '; vv++) ;
	    if (leadcopy[vv] != '>')
		dd = strlen(leadcopy);
	    else
		dd = 0;
	} /* else use existing leadcopy and dd */
    } else
	dd = 0;
    if (!dd)
	lelen = (quotype >= 1 && quotype <= 3 ? strlen(leadin) : 0), dd = 0;
    vvv = lelen && dent && slti != TRASHTYPE && slti != TRASHWRAPTYPE;
    /* vvv = "text is indented so space after leadin is redundant" */
    xmarge = (dd && !dlt ? 80 : marge);
    if (!wl && nwt == BODYTYPE) {
	if (lelen)
	    strcpy(toad, leadin);
	wl = sk = lelen - vvv;
    } else	/* if start of line, leadin; else trim spaces */
	while (ll && *p == ' ')
	    p++, ll--;
    memcpy(&toad[wl], p, (size_t) ll);
    vv = wl;
    wl += ll;			/* this line is joined to remnant of previous */
    do {			/* now word-wrap until not past margin... */
	fakesplit = false;
	if (wl > xmarge) {
	    short oldov = wl - marge;
	    ov = oldov;
	    wl = marge;
	    while (wl > sk && (k = toad[wl]) != ' ' && (k != '-' || wl == marge))
		wl--, ov++;	/* find point between words */
    /* What was I   vvvvvvvvvvvvvvvvvvvvvvvvvv   thinking??  I have no idea! */
	    if ( /* ov + lelen + dd > marge || */ wl == sk) {
		wl = marge;
		ov = oldov;
		k = toad[wl];
	    } else
		wl++, ov--;	/* backed up one too far; compensate */
	} else if ((localwrapmargin < 0 ? wrapmargin : localwrapmargin) == 0
					&& dlt && wl > 80) {
/* split in mid-word is OK, because it will be rejoined when written */
	    ov = wl - 80;
	    wl = 80;
	    fakesplit = true;
	    k = '\0';
	} else
	    k = ov = 0;
	if (dlt)
	    dlt[d_x] = nwt;
	if (!ov && wl <= xmarge /* && !dd */ )
	    break;			/* delay decision on flushing line */
	/* send out the part of the line that fits: */
	if (!Extrude(toad, wl - (k == ' '), vv) || d_x >= linez)
	    return lelen;		/* caller will bail out */
	sk = 0;
	if (fakesplit)
	    nwt = WRAPTYPE;
	else {
	    k = toad[wl - 1];		/* make remnant into new line */
	    kk = (wl == 1 ? '-' : toad[wl - 2]);
	    while (ov && toad[wl + ov - 1] == ' ')
		ov--;
	    while (ov && toad[wl] == ' ')
		wl++, ov--;
	    nwt = BODYTYPE;
	}
	if (ov) {			/* any remnant to deal with? */
	    if (fakesplit)
		ddd = 0;
	    else if (ddd = lelen)	/* pick proper leadin */
		strcpy(toad, leadin);
	    else if ( /* lelen = */ ddd = dd)
		strcpy(toad, leadcopy);
	    if (ddd) {			/* any leadin used? */
		short dx = dv = HowFarToIndentWrap(srclix, itsawrap);
		while (dx--)
		    toad[ddd++] = ' ';
	    } else
		dv = 0;
	    sk = (ddd -= vvv && (isspace(toad[wl]) || dv));
	    memcpy(&toad[ddd], &toad[wl], (size_t) ov);
	    ov += ddd;
	} else if (dlt && srclix + 1 < qsrc->linect &&
		     (qsrc->linetypes[srclix + 1] & TYPE_MASK) >= GUESSWRAPTYPE)
	    nwt = WRAPTYPE;
	wl = ov;
    } while (wl > xmarge);
    *wlp = wl;
    *nwtp = nwt;
    return lelen;
}


/* quotype values: 0 = none, 1 = add, 2 = add XX, 3 = wrap XX, 4 = verbatim */

/* I hate this goddamn function...  what a pain in the ass. */

void Quotify(struct Mess *dest, struct Mess *src)
{
    str qhead = localquoteheader[0] ? localquoteheader : quoteheader;
    str siggy = dubedit ? "" : (localsignature[0] ? localsignature : signature);
    short i, wl, marge, slti, dent, lelen = 0;
    short nwt = BODYTYPE, qxtra = (CountNs(qhead) + CountNs(siggy)) * 3 + 1;
    short wmarg = localwrapmargin < 0 ? wrapmargin : localwrapmargin;
    short itsawrap = (quotype == 3 || !wmarg ? GUESSWRAPTYPE : WRAPTYPE);
    ulong ll, linez = (!wmarg ? src->linect : 3 * src->linect) + qxtra;
    char toad[270], leadin[10];
    ustr p, dlt;
    bool thisquoted, lastquoted = false;

    ASSERT(quotype != 4 || !wmarg);
    if (!src->linect)
	return;
    filter_qheader = editstrip;
    qdest = dest, qsrc = src;
    marge = (!wmarg ? 255 /* max for a BValloc'd line */ : wmarg);
    dent = CheckIndent(src);
    complained = false;
    if (quotype == 1)
	strcpy(leadin, "> ");
    else if (quotype == 2 || quotype == 3) {
	struct Conf *cc = Confind(src->confnum);
	ustr drain = strstr(src->from, " <");
	if (cc && cc->morebits & INTERNET_EMAIL && drain > src->from + 3)
	    *drain = '\0';
	leadin[0] = ' ';
	GetInitials(leadin + indent_XX, src->from);
	if (drain)
	    *drain = ' ';
    } else
	leadin[0] = 0;
    if (!(dest->lines = Valloc(linez * 4))) {
	Err("Couldn't quote -- no memory.");
	return;
    }
    dest->bits |= LOADED;
#ifdef BETA
    quotifycount++;
#endif
    dest->linetypes = dlt = (!wmarg ? Valloc(linez) : null);
    dest->linect = wl = 0;
    ExpandQHeader(dest, src, qhead);
    d_x = dest->linect;
    for (i = 0; i < (short) src->linect; i++) {
	slti = src->linetypes[i] & TYPE_MASK;
	p = src->lines[i];
	ll = p ? p[-1] : 0;
	if (quotype != 4)
	    while (ll && isspace(p[ll - 1]))	/* trim trailing whitespace */
		ll--;
	thisquoted = slti == QUOTETYPE || slti == QUOTEWRAPTYPE;
	if (wl && (!ll || slti < itsawrap || lastquoted != thisquoted)) {
	    if (dlt)				/* leftover wrapped line */
		dlt[d_x] = nwt;
	    if (!Extrude(toad, wl, lelen) || d_x >= linez)
		break;
	    wl = 0;
	    nwt = BODYTYPE;
	}
	lastquoted = thisquoted;
	if (!ll) {				/* blank line in src */
	    nwt = BODYTYPE;
	    if (dlt)
		dlt[d_x] = BODYTYPE;
	    if (quotype == 1) {
#ifndef DONT_USE_LINEPOOL
		if (dest->lines[d_x] = NewMessPoolLine(dest, d_x))
#else
		if (dest->lines[d_x] = BValloc(1))
#endif
		    dest->lines[d_x][0] = '>';
	    } else
		dest->lines[d_x] = null;
	    d_x++;
	} else
	    lelen = DigestNonBlankLine(p, ll, &wl, linez, toad, &nwt, lelen,
					dent, slti, leadin, dlt, marge, i,
					itsawrap);
	if (d_x >= linez)
	    break;
    }
    if (wl && d_x < linez) {
	if (dlt)
	    dlt[d_x] = nwt;
	Extrude(toad, wl, lelen);
    }
    dest->linect = d_x;
    if (siggy[0])
	ExpandQHeader(dest, null, siggy);
}


local void Reload(struct Mess *thing)
{
    BPTR hand = OOpen(editoutinst);
    if (!hand) {
	Err("Could not open the file \"%s\"\n"
			"which is supposed to contain the\n"
			"message created by the editor.", editoutinst);
	return;
    }
    StripMessage(thing);
    thing->datfseek = -1;		/* LoadMessage adds 1 to this */
    thing->datflen = maxlong;
    LoadMessage(thing, hand, 2, false);
    thing->datflen = loaded_size;	/* for list display */
    Close(hand);
}


local void MakeBareReplyFile(str siggy, struct Mess *sp)
{
    bool quuck = false;
    if (siggy[0] && (sp->lines = Valloc(CountNs(siggy) * 12))) {
	sp->bits |= LOADED;
#ifdef BETA
	quotifycount++;
#endif
	filter_qheader = editstrip;
	ExpandQHeader(sp, null, siggy);
	quuck = SaveToFile(sp, editoutinst, -1);
	StripMessage(sp);
    }
    if (!quuck) {
#ifndef NO_CREATE_EMPTY
	BPTR th = NOpen(editoutinst);
	if (th && !Close(th))			/* create empty file */
#endif
	    DeleteFile(editoutinst);
    }
}


void DoEdit(struct Mess *thing, struct Mess *quotethis)
{
    BPTR nullhand = 0;
    bool quuck, doquote = quotethis && quotype > 0;
    str fname = (doquote && dubedit ? &editininst[0] : &editoutinst[0]);
    str command = (doquote && edit2command[0] && (dubedit || (firstedit
		    && !fixingreply)) ? &edit2command[0] : &edit1command[0]);
    str xcm = xcommand, siggy = localsignature[0] ? localsignature : signature;
    APTR oldwptr, oldcont;
    static struct Mess spare = { null /* ... */ };
    struct Mess *quolder;
    ustr obgwt = bgwin->ScreenTitle;
    short odpen = scr->DetailPen, obpen = scr->BlockPen;
    long rc = -1;
    bool once;

    PortOff();
    if (doquote && !(quotethis->bits & LOADED))
	if (!LoadPktMessage(quotethis)) {
	    PortOn();
	    return;
	}
    quolder = (dubedit ? &spare : thing);
    if (doquote && (dubedit || (firstedit && !carb))) {
	if (quotype == 4 && (localwrapmargin < 0 ? wrapmargin : localwrapmargin))
	    Carbonize(quolder, quotethis, false, true);
	else
	    Quotify(quolder, quotethis);
	quuck = SaveToFile(quolder, fname, -1);
	if (quolder == &spare)
	    StripMessage(quolder);
	if (!quuck)
	    command = &edit1command[0];
	else if (dubedit & firstedit && !fixingreply && !carb)
	    MakeBareReplyFile(siggy, &spare);
    } else if (firstedit && !fixingreply)
	MakeBareReplyFile(siggy, &spare);
    if (!Substitutions(xcm, "", "", command, true, false, false)) {
	PortOn();
	return;
    }
    if (firstedit)
	if (carb) {
	    if (!(carb->bits & LOADED) && !LoadPktMessage(carb)) {
		PortOn();
		return;
	    }
	    if (!Carbonize(&spare, carb, selfcarbon, false)
				|| !SaveToFile(&spare, editoutinst, -1)) {
		StripMessage(&spare);
		PortOn();
		return;
	    }
	    StripMessage(&spare);
	} else if (fixingreply)
	    if (!SaveToFile(fixee, editoutinst, -1)) {
		PortOn();
		return;
	    }
    oldwptr = me->pr_WindowPtr;
    oldcont = me->pr_ConsoleTask;
    me->pr_WindowPtr = (APTR) -1;
    nullhand = OOpen("NIL:");
    if (backbefore) {
	me->pr_WindowPtr = null;	/* requesters to Workbench */
	ScreenToBack(scr);
    } else
	me->pr_WindowPtr = oldwptr;
    GhostCompose(true);
    NoMenus();
    for (once = false; !once; once = true) {
	SetAllSharedWindowTitles(title);
	scr->DetailPen = 0;
	scr->BlockPen = 1;
	rc = ShellExecute(xcommand, 0L, nullhand);
	scr->DetailPen = odpen;
	scr->BlockPen = obpen;
	SetAllSharedWindowTitles(obgwt);
	if (rc >= 0 || IoErr() != ERROR_NO_FREE_STORE)
	    break;
	if (once)
	    Err("Couldn't run editor?\n(Not enough memory.)");
	else
	    MakeLotsaRoom();
    }
    me->pr_ConsoleTask = oldcont;
    if (nullhand)
	Close(nullhand);
    YesMenus();
    GhostCompose(false);
    if (cgagsave.Flags & GFLG_DISABLED)
	AbleAddGad(&cgagsave, cwin, true);
    if (firstedit) {
	short c = defarrow.FrontPen;
	defarrow.FrontPen = backcolor;
	defarrow.LeftEdge = cgagedit.LeftEdge;
	defarrow.TopEdge = cgagedit.TopEdge + cwin->Height - 1;
	DrawBorder(cwin->RPort, &defarrow, ARROWLEFT, 0);
	defarrow.FrontPen = c;
	defarrow.LeftEdge = cgagsave.LeftEdge;
	defarrow.TopEdge = cgagsave.TopEdge + cwin->Height - 1;
	DrawBorder(cwin->RPort, &defarrow, ARROWLEFT, 0);
	if (!dubedit)
	    AbleAddGad(&cgagquote, cwin, false);
	firstedit = false;
    }
    PortOn();
    if (frontafter) {
	me->pr_WindowPtr = oldwptr;
	ScreenToFront(scr);
	ActivateWindow(cwin);
    }
    if (!(curconf->areabits & INF_NO_TAGLINE))
	GiveItATaglineMaybe();
}


void FillNetAddress(void)
{
    if (cnet)
	FormatNetAddress(cbufnet, czone, cnet, cnode, cpoint);
    else
	cbufnet[0] = 0;
}


void CopyNetAddress(struct Mess *dest, struct Mess *src)
{
    czone = dest->zone = src->zone;
    cnet = dest->net = src->net;
    cnode = dest->node = src->node;
    cpoint = dest->point = src->point;
}


bool ObjectToLocal(void)
{
    bool ject = (czone == hostzone && cnet == hostnet
			&& cnode == hostnode && cpoint == hostpoint);
    if (ject)
	Err("You should not send netmail to the address of\n"
			"the BBS you downloaded the mail packet from.\n"
			"Use a local private mail area instead.");
    return ject;
}


bool ParseNetAddress(str a)
{
    ushort tzone, tnet, tnode = 0, tpoint = 0;
    ulong tt = 0;
    bool slashed = false, loud = a == (str) &cbufnet[0] || a == &fidogate[0];

    if (qwk && !*a)
	return false;		/* silently */
    tzone = tnet = 0;
    if (!isdigit(*a))
	goto puke;
    while (isdigit(*a))
	if ((tt = 10 * tt + *(a++) - '0') > 65535) goto barf;
    if (*a == ':')
	tzone = tt;
    else if (*a == '/' && tt)
	tnet = tt;
    else goto puke;
    tt = 0;
    if (!isdigit(*++a)) goto puke;
    while (isdigit(*a))
	if ((tt = 10 * tt + *(a++) - '0') > 65535) goto barf;
    if (tnet && *a && *a != '.') goto puke;
    if (tnet) {
	tnode = tt;
	slashed = true;
	if (!*a) goto swallow;
    } else if (*a == '/' && tt)
	tnet = tt;
    else goto puke;
    tt = 0;
    if (!isdigit(*++a)) goto puke;
    while (isdigit(*a))
	if ((tt = 10 * tt + *(a++) - '0') > 65535) goto barf;
    if (slashed) {
	tpoint = tt;
	if (*a) goto puke;
	else goto swallow;
    } else
	tnode = tt;
    if (!*a) goto swallow;
    else if (*a != '.') goto puke;
    tt = 0;
    if (!isdigit(*++a)) goto puke;
    while (isdigit(*a))
	if ((tt = 10 * tt + *(a++) - '0') > 65535) goto barf;
    tpoint = tt;
    if (*a) goto puke;
  swallow:
    czone = tzone; cnet = tnet; cnode = tnode; cpoint = tpoint;
/* #ifdef BETA
    if (loud) {
	FillNetAddress();
	RefreshGList(&cgagnet, cwin, null, 1);
    }
   #endif */
    return true;
  puke:
    if (loud)
	Err("The %s gadget does not contain a\n"
			"valid Zone:Net/Node.Point address.  Examples of\n"
			"addresses:  123/456  1:23/4567  12:345/67.8",
			a == &fidogate[0] ? "Fido gate address" :
			"Netmail Address");
    return false;
  barf:
    if (loud)
	Err("One of the numbers in the \"Net\naddress\" gadget is too large.");
    return false;
}


ushort CountWrappedLength(register struct Mess *mm, register short start)
{
    register ushort len = 0, lx;
    do {
	len += mm->lines[start][-1] + 1;
	lx = mm->linetypes[++start];
    } while (lx == RFCWRAPTYPE || (lx == RFCHEADERTYPE && mm->lines[start]
					&& isspace(mm->lines[start][0])));
    return len;
}


local ustr StitchWrappedLine(ustr dest, struct Mess *mm,
				register ushort start, ushort skippy)
{
    register ustr lll;
    register ushort lx;
    do {
	lll = mm->lines[start];
	lx = lll[-1];
	lll += skippy;
	lx -= skippy;
	while (isspace(*lll) && lx > 0)
	    lll++, lx--;
	strncpy(dest, lll, lx);
	dest += lx;
	if (!isspace(dest[-1]))
	    *dest++ = ' ';
	skippy = 0;	/* it affects only the first line */
    } while ((lx = mm->linetypes[++start]) == RFCWRAPTYPE ||
				(lx == RFCHEADERTYPE && mm->lines[start]
				    && isspace(mm->lines[start][0])));
/*  while (dest[-1] == ' ')
	dest--;  */
    return dest;
}


void GetMessageID(struct Mess *mm, short whichreply)
{
    short i, rln, iln, sln, fln, nln;
    register ushort lx;
    ushort reflen, nglen;
    ustr ll;
    struct Conf *cc;
    char y_version[32];

    if (!(mm->bits & LOADED))
	return;
    if ((cc = Confind(mm->confnum)) && cc->net_type == INF_NET_FIDONET)
	for (i = 0; i < mm->linect; i++) {
	    if ((lx = mm->linetypes[i]) == TRASHWRAPTYPE)
		continue;
	    if (lx != TRASHTYPE)
		break;
	    if (!(ll = mm->lines[i]) || (lx = ll[-1]) < 8)
		continue;
	    if (strncmp(ll, "\x01MSGID:", 7))
		continue;
	    ll += 7, lx -= 7;
	    while (lx > 0 && isspace(*ll))
		ll++, lx--;
	    if (lx < 3 || !(messageID[whichreply] = NewPoolString(lx)))
		continue;
	    strncpy0(messageID[whichreply], ll, lx);
	    return;
	}
    if (cc && cc->net_type == INF_NET_INTERNET) {
	rln = iln = sln = fln = nln = -1;
	for (i = 0; i < mm->linect; i++) {
	    if (mm->linetypes[i] != RFCHEADERTYPE || !(ll = mm->lines[i]))
		continue;
	    lx = ll[-1];
	    if (lx >= 11 && !strnicmp(ll, "Message-ID:", 11))
		iln = i;
	    else if (lx >= 11 && !strnicmp(ll, "References:", 11))
		rln = i;
	    else if (lx >= 8 && !strnicmp(ll, "Subject:", 8))
		sln = i;
	    else if (lx >= 12 && !strnicmp(ll, "Followup-to:", 12))
		fln = i;
	    else if (lx >= 11 && !strnicmp(ll, "Newsgroups:", 11))
		nln = i;
	}
	if (iln < 0)
	    return;
	reflen = CountWrappedLength(mm, iln) + strlen(x_version) + 20;
	if (rln >= 0)
	    reflen += CountWrappedLength(mm, rln);
	if (sln >= 0)
	    reflen += CountWrappedLength(mm, sln) + 2;
	if (fln >= 0)
	    nglen = CountWrappedLength(mm, fln) - 12;
	else if (nln >= 0)
	    nglen = CountWrappedLength(mm, nln) - 11;
	if (reflen && !messageID[whichreply] && (ll = messageID[whichreply]
					= NewPoolString(reflen))) {
	    if (rln < 0) {
		strcpy(ll, "\x01References: ");
		ll += 12;
	    } else {
		*ll++ = '\x01';
		ll = StitchWrappedLine(ll, mm, rln, 0);
	    }
	    ll = StitchWrappedLine(ll, mm, iln, 11);
	    while (isspace(ll[-1]))
		ll--;
	    if (sln >= 0) {
		strcpy(ll, "\r\n\x01");
		ll += 3;
		ll = StitchWrappedLine(ll, mm, sln, 0);
	    }
	    while (isspace(ll[-1]))
		ll--;
	    strcpy(y_version, x_version);
	    UnMask(y_version);
	    sprintf(ll, "\r\n\x01X-Newsreader: %s\r\n", y_version);
	}		/* vvvv  No spam replies to spam posts! */
	if (nglen && nglen < SIGNATURELEN && !newsgroups[whichreply]
					&& (ll = newsgroups[whichreply]
					   = NewPoolString(SIGNATURELEN + 2))) {
	    if (fln >= 0)
		ll = StitchWrappedLine(ll, mm, fln, 12);
	    else if (nln >= 0)
		ll = StitchWrappedLine(ll, mm, nln, 11);
	    while (isspace(ll[-1]))
		ll--;
	    *ll = 0;
	}
    }
}


void SwapLines(struct Mess *m1, struct Mess *m2)
{
    adr t;
    ushort s, changemask;
    long n;

    t = m1->lines;     m1->lines = m2->lines;         m2->lines = t;
    t = m1->linetypes; m1->linetypes = m2->linetypes; m2->linetypes = t;
    t = m1->linepool;  m1->linepool = m2->linepool;   m2->linepool = t;
    s = m1->poolindex; m1->poolindex = m2->poolindex; m2->poolindex = s;
    s = m1->linect;    m1->linect = m2->linect;       m2->linect = s;
    n = m1->datflen;   m1->datflen = m2->datflen;     m2->datflen = n;
    changemask = (m1->bits ^ m2->bits) & (LOADED | REPLY_HAS_TAG);
    m1->bits ^= changemask;
    m2->bits ^= changemask;
}


bool SaveReply(struct Mess *thing)
{
    char fame[14];
    BPTR oldcd;
    bool suck = false, gottaunswap = false;
    struct DateStamp d;
    short whichreply = fixingreply ? which_reedit : replies.messct;

    while (!(curconf->areabits & INF_POST)) {
#ifdef ABORT_SAVE_AT_AREAPICK
	short dfa = defaultarea;
	if (!CNewArea(false, false) || defaultarea == dfa)
	    return false;
#else
	CNewArea(false, false);
#endif
    }
    if (!cbuffrom[0]) {
	DisplayBeep(scr);
	ActivateGag(&cgagfrom, cwin);
	return false;
    } else if (cstrtoo.Buffer != cbuftoo) {
	ustr p = cstrtoo.Buffer;
	if (NewsgroupSyntaxError(p))
	    return false;
	while (*p)
	    if (isspace(*p))
		strcpy(p, p + 1);   /* a tad inefficient for multiple spaces */
	    else
		p++;
    } else if (!cbuftoo[0] && !notoowarned) {
	if (allowblanksubj)
	    notoowarned = true;
	/* else */
	DisplayBeep(scr);
	ActivateGag(&cgagtoo, cwin);
	return false;
    } else if (!cbufsubj[0] && !notoowarned) {
	if (allowblanksubj)
	    notoowarned = true;
	/* else */
	DisplayBeep(scr);
	ActivateGag(&cgagsubj, cwin);
	return false;
    } else if (thing->bits & EMAIL_REPLY) {
	StripString(&cgagtoo, cwin);
	if (cbuftoo[0] && !ValidInternet(cbuftoo)) {
	    Err("Internet email must be sent to a\nvalid address"
				" in \"name@site\" form.");
	    ActivateGag(&cgagtoo, cwin);
	    return false;
	}
    }
    StripString(&cgagnet, cwin);
    if (ReallyInNetmail() && (!cbufnet[0] || !ParseNetAddress(cbufnet)
				    || ObjectToLocal())) {
	if (!qwk || thing->bits & EMAIL_REPLY) {
	    if (!cbufnet[0])
		DisplayBeep(scr);
	    ActivateGag(&cgagnet, cwin);
	    return false;
	}
    }
    if (!CreateReplyLock())
	return false;
    PortOff();
    if (firstedit && carb) {
	if (!(carb->bits & LOADED))
	    if (!LoadPktMessage(carb)) {
		PortOn();
		return false;
	    }
	Carbonize(thing, carb, selfcarbon, false);
    } else if (firstedit && fixee) {
	SwapLines(thing, fixee);
	gottaunswap = true;
    } else
	Reload(thing);
    if (curconf->areabits & INF_NO_HIGHBIT) {
	register ushort i, l, c;
	register ustr p;
	for (c = i = 0; i < thing->linect; i++)
	    if (p = thing->lines[i])
		for (l = p[-1]; l-- > 0; p++)
		    if (*p & 0x80)
			c++;
	if (c) {
	    if (!AskStripHighBits(c))
		StripMessage(thing);	/* we will bail due to !LOADED */
	    else
		for (i = 0; i < thing->linect; i++)
		    if (p = thing->lines[i])
			for (l = p[-1]; l-- > 0; p++)
			    if (*p & 0x80)
				*p = '*';		/* no finesse */
	}
    }
    if (!(thing->bits & LOADED)) {
	PortOn();
	return false;		/* error reported by Reload or LoadMessage */
    }
    if (cgagpriv.GadgetText == &cgtprivate)
	thing->bluebits |= UPL_PRIVATE;
    else
	thing->bluebits &= ~UPL_PRIVATE;
    if (cnet && (curconf->areabits & INF_NETMAIL || (qwk && ie_is_gated
				&& thing->bits & EMAIL_REPLY)) && cbufnet[0]) {
	if (curconf->areabits & INF_NETMAIL)
	    netmailareanum = Conf2ix(curconf);
	thing->bluebits |= UPL_NETMAIL;
	thing->zone = czone;
	thing->net = cnet;
	thing->node = cnode;
	thing->point = cpoint;
	thing->attribits = netbits;
	thing->bits |= POINTLESS;
    } else {
	thing->bluebits &= ~UPL_NETMAIL;
	if (thing->bluebits & UPL_PRIVATE)
	    localmailareanum = Conf2ix(curconf);
	czone = cnet = cnode = cpoint = thing->zone = thing->net = thing->node
				= thing->point = netbits = thing->attribits = 0;
    }
    oldcd = CurrentDir(replylock);
    DateStamp((adr) &d);
    thing->unixdate = DS2ux(&d);
    DateString(&d, thing->date);

    strcpy(thing->confnum, curconf->confnum);
    thing->ixinbase = UniqueRix(fixingreply ? fixee : null);
    thing->bits |= DONTSTRIP | MESEEN;
    if (uppitynames)
	strupr(cbuffrom);
    if (thing->bits & EMAIL_REPLY) {
	char ttoo[MAXIETOLEN];
	if (NormalizeInternet(cbuftoo, ttoo, MAXIETOLEN))
	    strcpy(cbuftoo, ttoo);
    } else if (uppitynames)
	strupr(cbuftoo);
/****
    if (uppitytitle)
	strupr(cbufsubj);
****/
    if (cgagfrom.Flags & GFLG_DISABLED)
	strncpy0(thing->from, cbuffrom, (long) fromend);
    else
	strcpy(thing->from, cbuffrom);
    if (cstrtoo.Buffer == cbuftoo)
	strcpy(thing->too, cbuftoo);
    else
	thing->too[0] = 0;
    strcpy(thing->subject, cbufsubj);
    if (curconf->areabits & INF_NO_TAGLINE)
	RemoveTaglineFromReply(thing, whichreply, null);
    else
	FinalizeTagline(thing, whichreply);
/*  if (fixingreply && !qwk)
	DeleteFile(oldflame);	*/	/* redundant; SaveToFile() does this */
    thing->replyat = whichreply;	/* GetBWReplyFilename() needs this */
    GetBWReplyFilename(fame, thing);
    if (!fixingreply)
	oldfilenames[replies.messct] = null;	/* probably not needed */

    if (suck = SaveToFile(thing, fame, whichreply)) {
	bool alreadythere = false;
	curconf->sofar++;
	if (fixingreply) {
	    struct Attachment *old = replies.messes[whichreply]->attached;
	    if (old) {
		if (old->localsource) {
		    if (thing->attached && IdenticalPaths(old->localpath,
					    thing->attached->localpath)) {
			if (qwk) {
			    strcpy(thing->attached->tempname, old->tempname);
			    alreadythere = true;
			} else
			    alreadythere = !stricmp(old->arrivename,
						thing->attached->arrivename);
		    }
		} else {
		    alreadythere = !thing->attached->localsource;
		    if (!qwk && alreadythere && stricmp(old->arrivename,
						thing->attached->arrivename)) {
			BPTR xcd = CurrentDir(replylock);
			Rename(old->arrivename, thing->attached->arrivename);
			CurrentDir(xcd);
		    }
		}
		if (!alreadythere) {
		    BPTR ocd = CurrentDir(replylock);
		    DeleteFile(old->tempname);
		    CurrentDir(ocd);
		}
	    }
	    FreeReply(replies.messes[whichreply]);
	    replies.messes[whichreply] = thing;
	    gottaunswap = false;
	} else {
	    if (!replies.messct) {
		if (readareaz.unfiltered) {
		    readareaz.confs[readareaz.messct] = &replies;
		    oldracount++;
		}
		readareaz.messct++;		/* make replies readable */
	    }
	    replies.messes[replies.messct++] = thing;
	}
	if (!alreadythere && thing->attached && thing->attached->localsource) {
	    BPTR fah = OOpen(thing->attached->localpath);
	    BPTR ocd = CurrentDir(replylock);
	    if (!fah || CopyFile(fah, thing->attached->tempname)) {
		DosErr("Could not copy attached file\n%s to replies dir",
		       thing->attached->localpath);
		DeleteFile(thing->attached->tempname);
		FREE(thing->attached);
		thing->attached = null;
	    }
	    CurrentDir(ocd);
	    if (fah) Close(fah);
	}
	if (thing->mreplyee && !carb) {
	    thing->mreplyee->mreplyee = thing;
	    if (thing->mreplyee->personalink)
		thing->mreplyee->personalink->mreplyee = thing;
	} else if (!selfcarbon)
	    thing->mreplyee = null;
	while (!WriteUPL() && AskRetryWriteUPL()) ;
	anythingtosave = repchanges = true;

	if (fakery && readareaz.confs[whicha] == &bullstuff) {
	    whicha++;
	    ASSERT(readareaz.confs[whicha] == &replies);
	    replies.current = whichreply;	/* this sets whichm */
	    deferflipb = true;
	    AdjustToNewArea(true);
	    deferflipb = false;
	} else if (readareaz.confs[whicha] == &replies) {
	    onscreen = null;
	    whichm = whichreply;
	    ShowNewMessage(&replies, thing);
	} else if (!fixingreply) {
	    struct Conf *cc = readareaz.confs[whicha];
	    if (replyeee && !carb) {
		replyeee->bits |= MEREPLIED;
		if (replyeee->personalink)
		    replyeee->personalink->bits |= MEREPLIED;
	    }
	    SwitchRGag( /* cc == &replies */ false, false);
	    ShowHeader(cc, cc->messes[whichm]);
	}
	TweakCloseGag();
    }
    if (!suck) {				/* never happens? */
/*	if (!qwk) DeleteFile(fame); */
	if (gottaunswap)
	    SwapLines(thing, fixee);
    }
    CurrentDir(oldcd);
    PortOn();
#ifdef WARN_LONG_REPLY
    if (suck && thing->linect >= 98 && whichreply != lastlongwarn) {
	WarnLongMess(thing->linect);
	lastlongwarn = whichreply;
    }
#endif
    return suck;	/* if bad_upl, may be true in spite of failure */
}


void FlushEdit(struct Mess *thing)
{
    PortOff();
    FreeReply(thing);
    DeleteFile(editoutinst);
    if (editininst[0])
	DeleteFile(editininst);
    PortOn();
}


local bool FindOrigin(str mese, struct Mess *mm, bool allownocolon)
{
    short i, j, k, tl, ll = strlen(mese);
    char lookee[24], c;

    for (j = ll - 2; j > 0; j--)
	if (mese[j] == '/' && isdigit(mese[j + 1]) & isdigit(mese[j - 1])) {
	    i = k = j;
	    while (i && (c = mese[i - 1], isdigit(c) || c == ':'))
		i--;
	    while (k < ll && (c = mese[k + 1], isdigit(c) || c == '.'))
		k++;
	    if ((tl = k + 1 - i) > 23)
		continue;
	    strncpy0(lookee, mese + i, (size_t) tl);
	    if (!allownocolon && !strchr(lookee, ':'))
		return false;
	    if (ParseNetAddress(lookee)) {
		mm->zone = czone;
		mm->net = cnet;
		mm->node = cnode;
		mm->point = cpoint;
		return true;
	    }
	}
    return false;
}


void ReadOrigin(struct Mess *mm)
{
    char bloc[258];
    short bl, i, x;
    str foo;

    do {
	if (!mm->datflen || !texthand)
	    return;
	bl = min(256, mm->datflen);
	if (SSeek(texthand, mm->datfseek + 1 + mm->datflen - bl,
						OFFSET_BEGINNING) < 0)
	    return;
	if ((bl = RRead(texthand, bloc, (long) bl)) <= 0)
	    return;
	for (x = i = 0; i < bl; i++)
	    if (!bloc[i])
		bloc[i] = ' ';
	    else if (bloc[i] != ' ')
		x = i;
	bloc[bl] = 0;
/* PCBoard internal QWK gives some messages extra trailing blocks of spaces: */
	if (qwk)
	    mm->datflen -= 128 * ((bl - x) / 128);
    } while (bl - x >= 128);
    FindOrigin(bloc, mm, true);
    postlinkage = (foo = strstr(bloc, " PostLink(tm) ")) &&
			(foo[-3] == '*' || !isprint(foo[-3]));
}


void Pontificate(struct Mess *mm)
{
    ushort pt = 0;
    ustr line, lin2;
    short i, tl;
    bool liveone;
#ifdef AW_FUCK_THIS
    char lookee[24];
    short j, k, ll, kl;
#endif
    char bowl[162];
    static char blanket[] = "\0\0";

    if (mm->bits & POINTLESS)
	return;
    if (!(mm->bits & LOADED))
	LoadPktMessage(mm);
    if (!mm->linect)
	return;
    mm->bits |= POINTLESS;
#ifdef AW_FUCK_THIS
    if (mm->net) {
	sprintf(lookee, "%lu/%lu", (ulong) mm->net, (ulong) mm->node);
	kl = strlen(lookee);
	for (i = mm->linect - 1; i >= 0; i--) {
	    if (!(line = mm->lines[i]))
		continue;
	    ll = line[-1];
	    tl = ll - kl;
	    for (j = 0; j < tl; j++)
		if (!strncmp(line + j, lookee, kl)) {
		    k = j + kl;
		    if (ll > k && line[k] == '.')
			while (isdigit(line[++k]) && ll > k)
			    pt = 10 * pt + line[k] - '0';
		    mm->point = pt;
		    return;
		}
	}
    }
    /* Okay, the origin numbers we got from the network are not found in the   */
    /* message, as is often the case ... in a lot of cases the numbers we have */
    /* downloaded are simply those of the nearest link in the network.  So:    */
#endif
    tl = -1;
    for (i = mm->linect - 1; i >= 0; i--) {
	if (!(line = mm->lines[i]))
	    continue;
	if (!strncmp(line, " * Origin:", 10)) {
	    tl = i;
	    break;
	}
    }
    if (tl < 0)
	for (i = mm->linect - 1; i >= 0; i--) {
	    if (!(line = mm->lines[i]))
		continue;
	    if (line[0] == ' ' && !strncmp(line + 2, " Origin:", 8)) {
		tl = i;
		break;
	    }
	}
    if (tl >= 0) {
	if (tl == mm->linect - 1 || !(lin2 = mm->lines[tl + 1]))
	    lin2 = blanket + 1;
/*	sprintf(bowl, "%.*s\n%.*s", line[-1], line, lin2[-1], lin2); */
	strncpy0(bowl, line, i = line[-1]);
	if (FindOrigin(bowl, mm, true))
	    return;
	bowl[i++] = '\n';
	strncpy0(bowl + i, lin2, lin2[-1]);
	if (FindOrigin(bowl, mm, true))
	    return;

    }
#ifdef OLD_COCONINO_SCAN
    if ((line = mm->lines[0]) && strnistr(line, "from", line[-1]))
	lin2 = line;
    else if (mm->linect > 1 && (line = mm->lines[1])
				&& strnistr(line, "from", line[-1]))
	lin2 = line;
    else if (mm->linect > 2 && (line = mm->lines[2])
				&& strnistr(line, "from", line[-1]))
	lin2 = line;
    else
	lin2 = null;
    if (lin2) {
	strncpy0(bowl, lin2, lin2[-1]);
	if (FindOrigin(bowl, mm, false))
	    return;
    }
#else
    liveone = false;
    for (i = 0; i < mm->linect; i++) {
	if (AllBlank(line = mm->lines[i])) {
	    if (liveone)
		break;
	} else {
	    tl = line[-1];
	    liveone = true;
	    if (strnistr(line, "from", tl) /* || strnistr(line, "FMPT", tl)
				|| strnistr(line, "INTL", tl) */
				|| strnistr(line, "MSGID", tl)) {
		strncpy0(bowl, line, tl);
		if (FindOrigin(bowl, mm, false))
		    return;
	    }
	}
    }
#endif
    /* scan entire message for anything that looks like an origin? */
    /* if so, call FindOrigin with false in last arg               */
}
