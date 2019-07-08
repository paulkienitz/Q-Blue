/* shit for one/two/three-gadget standardized requesters, for asking questions
and reporting errors or nagging about nonregistration. */

#include <devices/inputevent.h>
#include <intuition/intuitionbase.h>
#include <intuition/sghooks.h>
#include <stdarg.h>
#include "qblue.h"
#include "pigment.h"
#include "semaphore.h"
#include "version.h"
/* from stdio.h: */
int vsprintf(char *_s, const char *_format, va_list _arg);


#define YELLCOLOR   1
#define EDGECOLOR   7
#define GAGTCOLOR   5
#define GAGBCOLOR   2
#define ARROWCOLOR  7

#define MAXLINES    20
#define WINWIDTH3   490
#define MINWIDTH2   340
#define MINWIDTH1   180
#define ARROWLEFT   (-16)

#define REJECTQUAL  (IEQUALIFIER_LCOMMAND | IEQUALIFIER_RCOMMAND) /* others? */


void DamnClock(struct Window *exceptfor);
void NoDamnClock(struct Window *exceptfor);
void StripIntuiMessages(struct MsgPort *mp, struct Window *win);
void ScaleGadget(struct Gadget *gg);
void UnScaleGadget(struct Gadget *gg);
void FormatNetAddress(str buf, ushort zone,
			ushort net, ushort node, ushort point);
void DateString(struct DateStamp *d, str s);
void ux2DS(struct DateStamp *d, ulong inn);
str FixWRpath(str what);


import struct IntuitionBase *IntuitionBase;
import struct Window *owin;
import struct TextAttr ta;
import ushort penzez[];

import struct QSemNode ourqsemnode;
import struct Conf readareaz;

import str aboutlines[];
import char packernames[][PACKNAMELEN], bbsphone[], sysopname[], doorconn[];
import char bbsname[], bbscity[], myloginame[], myothername[], darchivename[];

import ulong downloaddate, totalunread, totaltoread;
import short countoff;
import ushort hostcredits, hostdebits, currentpacker, tifight, nomwidth;
import ushort hostzone, hostnet, hostnode, hostpoint;
import bool quitleaveopen, wouldahaddrop, qed, version3;


struct TextAttr topaz9 = { "topaz.font", 9, 0, FPF_ROMFONT };

short greenies[18] = {
    2, 1, 77, 1, 77, 11, 2, 11, 2, 2, 3, 2, 3, 10, 76, 10, 76, 2
};

/**
struct Border dgreenfill = {
    0, 0, GAGBCOLOR, GAGBCOLOR, JAM2, 9, &greenies[0], &downborder
};
**/

struct Border ugreenfill = {
    0, 0, GAGBCOLOR, GAGBCOLOR, JAM2, 9, &greenies[0], &upborder
};


char strthird[10], strno[10], stryes[10];

struct IntuiText rgtthird = {
    GAGTCOLOR, GAGBCOLOR, JAM2, 4, 2, null, strthird, null
};

struct Gadget rgagthird = {
    null, -410, -23, 80, 13,
    GFLG_GADGHCOMP | GFLG_RELBOTTOM | GFLG_RELRIGHT,
    GACT_RELVERIFY, GTYP_BOOLGADGET | GTYP_REQGADGET,
    &ugreenfill, null /* &dgreenfill */, &rgtthird, 0, null, 902, null
};

struct IntuiText rgtyes = {
    GAGTCOLOR, GAGBCOLOR, JAM2, 4, 2, null, stryes, null
};

struct Gadget rgagyes = {
    null, -260, -23, 80, 13,
    GFLG_GADGHCOMP | GFLG_RELBOTTOM | GFLG_RELRIGHT,
    GACT_RELVERIFY, GTYP_BOOLGADGET | GTYP_REQGADGET,
    &ugreenfill, null /* &dgreenfill */, &rgtyes, 0, null, 901, null
};

struct IntuiText rgtno = {
    GAGTCOLOR, GAGBCOLOR, JAM2, 4, 2, null, strno, null
};

struct Gadget rgagno = {
    &rgagyes, -110, -23, 80, 13,
    GFLG_GADGHCOMP | GFLG_RELBOTTOM | GFLG_RELRIGHT,
    GACT_RELVERIFY, GTYP_BOOLGADGET | GTYP_REQGADGET,
    &ugreenfill, null /* &dgreenfill */, &rgtno, 0, null, 900, null
};


short rarrows[16] = {
    0, 1, 0, 10, -24, 10, -22, 12, -22, 8, -24, 10, -1, 10, -1, 1
};

short redges[18] = {
    2, 0, 399, 0, 399, 99, 0, 99, 0, 0, 1, 0, 1, 99, 398, 99, 398, 1
};

struct Border defarrow = {
    ARROWLEFT, 0, ARROWCOLOR, 0, JAM2, 8, &rarrows[0], null
};

struct Border redge = {
    0, 0, EDGECOLOR, 0, JAM2, 9, &redges[0], &defarrow
};


struct IntuiText rlines[MAXLINES];


struct Requester quest = {
    null, 0, 0, MINWIDTH2, 100, 0, 0, &rgagno, &redge, &rlines[0],
    SIMPLEREQ | NOISYREQ, 0, null, { 0 }, null, null, null, { 0 }
};


struct ExtNewWindow quineww = {
    140, 50, MINWIDTH2, 100, 0, 1,
    IDCMP_RAWKEY | IDCMP_GADGETUP | IDCMP_INTUITICKS,
    WFLG_ACTIVATE | WFLG_BORDERLESS | WFLG_SIMPLE_REFRESH | WFLG_NW_EXTENDED,
    null, null, null, null, null, 0, 0, 0, 0, CUSTOMSCREEN, null
};

struct WhereWin quiww = { &quineww };


struct Window *quin;

bool questionating = false, lastkeywasshifted = false;

local bool justone, justwo, escapee;
local short answer;
local short rbackcolor, gagdefault;


char KeyToAscii(ushort code, ushort qual)
{
    static struct InputEvent iev = { 0 };
    char buf[2];

    if (code & IECODE_UP_PREFIX || qual & REJECTQUAL ||
		    (qual & IEQUALIFIER_REPEAT && (code < 0x4C || code > 0x4F)
			&& (code | 0x20) != 0x3E && (code | 2) != 0x2F))
	return 0xFF;			/* value means "no key" */
    iev.ie_Class = IECLASS_RAWKEY;
    iev.ie_Code = code;
    iev.ie_Qualifier = qual & (IEQUALIFIER_CONTROL | SHIFTKEYS);
    lastkeywasshifted = !!(qual & SHIFTKEYS);
    if ((qual & IEQUALIFIER_NUMERICPAD && code != 0x43)
			|| RawKeyConvert(&iev, buf, 1, null) <= 0)
	return code | 0x80;		/* high bit means raw code, not ascii */
    else
	return toupper(buf[0]);		/* it translated to a single char */
}


bool DoQuestIDCMP(struct IntuiMessage *im)
{
    short giddy;
    char k;

    if (im->Class == IDCMP_GADGETUP) {
	giddy = ((struct Gadget *) im->IAddress)->GadgetID;
	answer = giddy - 900;
	return !(answer & ~3);
    } else if (im->Class == IDCMP_RAWKEY && !(im->Qualifier & ALTKEYS)) {
	k = KeyToAscii(im->Code, im->Qualifier);
	if (k == ESC) {
	    answer = escapee;		/* usually same as gagdefault */
	    return true;
	} else if (k == ' ' || k == '\r' || k == '\n') {
	    answer = gagdefault;
	    return true;
	} else if (k == toupper(rgtno.IText[1])) {
	    answer = 0;
	    return true;
	} else if (!justone && k == toupper(rgtyes.IText[1])) {
	    answer = 1;
	    return true;
	} else if (!justwo && k == toupper(rgtthird.IText[1])) {
	    answer = 2;
	    return true;
	}
    }
    if (!(im->Qualifier & IEQUALIFIER_RBUTTON)
			&& scr->LayerInfo.top_layer != quest.ReqLayer)
	WindowToFront(quin);
    return false;
}


void FixDefarrow(void)
{
    rarrows[3] = rarrows[5] = fight + 2;
    rarrows[11] = rarrows[13] = fight + 2 - lace;
    rarrows[7] = fight + 4;
    rarrows[9] = fight - lace;
}


void LabelAskGag(str teu, str frum)
{
    str u = teu + 1;
    teu[0] = ' ';
    while (u - teu <= 8)
	*(u++) = (*frum ? *(frum++) : ' ');
    teu[9] = 0;
}


/* when using all three gadgets, the text should be 60 or more columns wide */

short Question(str *lines, str gthird, str gyes, str gno, short deflat)
{
    short lct, i, co, hai = fight + 20, wid = 0;
    struct IntuiText *it;
    struct IntuiMessage *im;
    import struct Requester *lastifle;

    if ((justwo = !gthird) && deflat & 0xFFFE)
	deflat = 1;
    if ((justone = !gyes) && deflat)
	deflat = 0;
    if (!*lines)
	return deflat;
    if (!stricmp(gno, "CANCEL") || !stricmp(gno, "LATER"))
	escapee = 0;
    else
	escapee = deflat;
    quest.BackFill = rbackcolor = fourcolors ? 2 : 3;
    LabelAskGag(strno, gno);
    if (!justone)
	LabelAskGag(stryes, gyes);
    if (!justwo)
	LabelAskGag(strthird, gthird);
    rgagno.NextGadget = (justone ? null : &rgagyes);
    rgagyes.NextGadget = (justwo ? null : &rgagthird);

    for (lct = 0; lines[lct] && lct < MAXLINES; lct++) {
	i = strlen(lines[lct]);
	if (lines[lct][0] == '!')	/* this line in topaz 9 */
	    i = 10 * (i - 1), co = 9;
	else
	    i *= fontwid, co = fight;
	if (wid < i) wid = i;
	if (hai + co > scr->Height) {
	    lct++;			/* correct total line count */
	    break;
	} else
	    hai += co;
    }
    if ((wid += 20) > scr->Width)
	wid = scr->Width;
    if (!justwo && wid < (co = XActual(WINWIDTH3)))
	wid = co;
    if (!justone && wid < (co = XActual(MINWIDTH2)))
	wid = co;
    else if (wid < (co = XActual(MINWIDTH1)))
	wid = co;
    co = 4;				/* TopEdge of first text line */
    for (i = 0; i < lct; ) {
	it = &rlines[i];
	it->BackPen = rbackcolor;
	it->TopEdge = co;
	if (lines[i][0] == '!') {
	    co += 9;
	    it->ITextFont = &topaz9;
	    it->IText = lines[i] + 1;
	    it->FrontPen = YELLCOLOR;
	    it->LeftEdge = (wid - 10 * strlen(it->IText)) >> 1;
	    if (it->LeftEdge < 10)
		it->LeftEdge = 10;
	} else {
	    co += fight;
	    it->ITextFont = &ta;
	    it->IText = lines[i];
	    it->FrontPen = TEXTCOLOR;
	    it->LeftEdge = 10;
	}
	it->NextText = &rlines[++i];
    }
    it->NextText = null;
    quineww.Width = quest.Width = wid;
    if (IntuitionBase->ActiveScreen != scr)
	quineww.Flags &= ~WFLG_ACTIVATE;
    else
	quineww.Flags |= WFLG_ACTIVATE;
    quineww.LeftEdge = ((wid > nomwidth ? scr->Width : nomwidth) - wid) >> 1;
    quineww.Height = quest.Height = hai;
    rgagno.TopEdge = rgagyes.TopEdge = rgagthird.TopEdge = -10 - fight;
    quineww.TopEdge = (scr->Height - hai) >> 1;
    quiww.moved = false;
    redges[2] = redges[4] = wid - 1;
    redges[14] = redges[16] = wid - 2;
    redges[5] = redges[7] = redges[13] = redges[15] = hai - 1;
    rgagno.Height = rgagyes.Height = rgagthird.Height = fight + 5;
    quineww.Screen = scr;
    ScaleGadget(&rgagno);
    ScaleGadget(&rgagyes);
    ScaleGadget(&rgagthird);
    greenies[5] = greenies[7] = fight + 3;
    greenies[13] = greenies[15] = fight + 2;
    greenies[2] = greenies[4] = rgagno.Width - 3;
    greenies[14] = greenies[16] = rgagno.Width - 4;
    FixDefarrow();
    defarrow.TopEdge = quest.Height + rgagno.TopEdge - 1;
    defarrow.LeftEdge = (deflat == 2 ? rgagthird.LeftEdge :
				(deflat ? rgagyes.LeftEdge : rgagno.LeftEdge))
				+ quest.Width + ARROWLEFT;

    co = countoff;
    NoMenus();
    while (countoff > 0)
	PortOn();
    wid = fontwid;	/* fool OpenShareWin into not scaling */
    fontwid = 8;
    if (!(quin = OpenShareWin(&quiww)) || !Request(&quest, quin)) {
	/* The following must work with basically NO spare ram available: */
	str owt = bgwin->ScreenTitle;
	fontwid = wid;
	if (quin)
	    CloseShareWin(quin);
	DisplayBeep(scr);
	i = scr->BlockPen;
	lct = scr->DetailPen;
	penzez[0] = scr->DetailPen = fourcolors ? 0 : 7;    /* yellow */
	penzez[1] = scr->BlockPen = 3;			    /* like, RED eh */
	SetWindowTitles(bgwin, null,
" *** ERROR ***    NO MEMORY!!  Other errors likely.    PRESS ANY KEY");
	do {
	    WaitPort(idcort);
	    if (im = (adr) GetMsg(idcort)) {
		wid = im->Class == IDCMP_RAWKEY && !(im->Code & 0x80)
				&& !(im->Qualifier & IEQUALIFIER_REPEAT);
		ReplyMsg((adr) im);
	    }
	} while (!im || !wid);
	penzez[0] = scr->DetailPen = lct;
	penzez[1] = scr->BlockPen = i;
	SetWindowTitles(bgwin, null, owt);
	while (countoff < co)
	    PortOff();
	YesMenus();
	return escapee;
    } else
	fontwid = wid;
    DamnClock(quin);
/*    ScreenToFront(scr);  */		/* No. */
    gagdefault = deflat;
    questionating = true;
    EventLoop(DoQuestIDCMP);
    questionating = false;
/*    StripIntuiMessages(idcort, null);  */
    EndRequest(&quest, quin);
    CloseShareWin(quin);
    UnScaleGadget(&rgagno);
    UnScaleGadget(&rgagyes);
    UnScaleGadget(&rgagthird);
    NoDamnClock(null);
    while (countoff < co)
	PortOff();
    YesMenus();
    return answer;
}


void Aboutness(void)
{
    Question(aboutlines, null, null, "Okay", 0);
}


local str reopenlines[] = {
    "A reply packet exists in your upload directory corresponding",
    "to the packet you just opened.  The download packet seems to",
    null,
    "",
    "Do you want to IGNORE (and presumably discard) that packet,",
    "or UNPACK and reload it so you can add more replies to it?",
    null
};

bool AskReopen(short newer)
{
    reopenlines[2] = (!newer ?
		"be NEWER than the reply packet, which may be outdated." :
		"be OLDER than the reply packet, which may be unfinished.");
    return Question(reopenlines, null, "Unpack", "Ignore", newer);
}


local str attemptleftoverlines[] = {
    "Your replies directory seems to already contain some",
    "for this packet.  The download packet seems to be",
    null,
    "",
    "Do you want to RELOAD those replies, or DELETE them?",
    null
};

bool AskAttemptReplies(short newer)
{
    attemptleftoverlines[2] = (!newer ?
		"NEWER than those replies, which may be outdated." :
		"OLDER than those replies, which may be unfinished.");
    return Question(attemptleftoverlines, null, "Reload", "Delete", 1);
}


local str reopen3lines[] = {
    "Your replies directory seems to already contain replies for this",
    "message packet.  Also, a reply packet corresponding to this message",
    "packet exists in your upload directory.  This message packet seems",
    null,
    "",
    "Do you want to try to RELOAD the messages already in the replies",
    "directory so you can add to them, UNPACK the replies in the upload",
    "directory instead, or IGNORE (and presumably discard) them all?",
    null
};

bool AskReopen3(short newer, bool *reopen)
{
    static short def[3] = { 0, 2, 1 };
    static str nline[3] = {
	    "to be NEWER than the leftover replies, which may be outdated.",
	    "to be OLDER than the leftover replies, which may be unfinished.",
	    "to be OLDER than the upload packet, which may be unfinished."
    };
    short foo;

    reopen3lines[3] = nline[newer];
    foo = Question(reopen3lines, "Reload", "Unpack", "Ignore", def[newer]);
    *reopen = foo == 1;
    return foo == 2;
}


void sprintjam(str dest, str format /* must contain "%s%s" only */, str path)
{
    ushort i = strlen(path), j = strlen(format) - 4;
    ushort k = i > 77 - j ? i - (74 - j) : 0;
    sprintf(dest, format, k ? "..." : "", path + k);
}


local str unpackanywaylines[] = {
    null,
    "type.  Do you want to TRY unpacking it anyway (which",
    "probably won't work), or CANCEL the attempt?",
    null
};

bool AskUnpackAnyway(str fullname)
{
    char buf[80];

    unpackanywaylines[0] = buf;
    sprintjam(buf, "Unable to read file \"%s%s\" to check", fullname);
    return Question(unpackanywaylines, null, "Try it", "Cancel", 0);
}


local str overwritelines[] = {
    null,
    "",
    "Do you want to APPEND to it, REPLACE its current contents, or",
    "leave it untouched (CANCEL this save)?",
    null
};

bool AskOverwriteAppend(str filename, bool *appendage)
{
    short foo;
    char buf[80];

    sprintjam(buf, "A file named \"%s%s\" already exists.", filename);
    overwritelines[0] = buf;
    foo = Question(overwritelines, "Append", "Replace", "Cancel", 2);
    *appendage = foo == 2;
    return !!foo;
}


/* limited to twelve lines including first and last, 800 chars between */
local void RawErr(str first, str last, str format, va_list args)
{
    char space[800];
    str brake, lineys[13];
    short i, mex = 12 - !!last;

    StripIntuiMessages(idcort, null);
    vsprintf(space, format, args);
    lineys[0] = first;
    for (i = 1, brake = space; i < mex && brake; i++) {
	lineys[i] = brake;
	if (brake = strchr(brake, '\n'))
	    *brake++ = 0;
    }
    if (last)
	lineys[i++] = last;
    lineys[i] = null;
    Question(lineys, null, null, "Okay", 0);
}


void Err(str format, ...)
{
    va_list args;
    va_start(args, format);
    if (scr)
	RawErr("!**  ERROR  **", null, format, args);
    va_end(args);
}


void DosErr(str format, ...)
{
    long err = IoErr();
    char foo[24], bar[120];
    va_list args;

    sprintf(foo, "DOS error = %ld", err);
    if (err)
	Fault(err, foo, bar, 120);
    else
	sprintf(bar, "%s (unset)", foo);
    va_start(args, format);
    if (scr)
	RawErr("!**  ERROR  **", bar, format, args);
    va_end(args);
}


/* a widespread kind of error function: */

void SeekErr(str what)
{
    DosErr("Seek failed while %s.\nPossibly the file is truncated.", what);
}


/* more widespread than SeekErr: */

void ReadWriteErr(str action, str file, ushort dirtok)
{
    if (!action) action = "read expected data from";
    DosErr("Could not %s\nfile %s in %s directory.",
				action, file, dirname[dirtok]);
}


/* also widespread: */

void WindowErr(str what)
{
    Err("Could not open dialog window for\n%s", what);
}


local str hintlines[] = {
    "You do not have a complete configuration set up for this copy of Q-Blue.",
    "Before trying to read any e-mail, please use the rightmost pull-down",
    "menu, labeled \"Setup\", to configure the directories, compressors, text",
    "editor, and preference options that you wish to use with Q-Blue, and use",
    "the \"Save setup\" option to make the configuration permanent.  Many",
    "features of the program will not work until these setup steps are",
    "completed.  DO consult the documentation for details on configuring the",
    "program, even if you ignore it on other topics.",
    "",
    "!IMPORTANT: the directories you select as your \"work\" and",
    "!\"replies\" directories must not be used for any other    ",
    "!purpose, because Q-Blue will DELETE any existing files  ",
    "!it finds in either of them.  NAME DIRECTORIES THAT ARE  ",
    "!NOT IN USE FOR OTHER FILES, or your data may be lost.   ",
    null
};

void ConfigHints(void)
{
    Question(hintlines, null, null, "Okay", 0);
}


local str differentpackerlines[] = {
    "This packet appears to be compressed with a different packer",
    "than the one you have selected.  Do you want to switch from",
    null,
    null
};

short AskDifferentPacker(short whichpacker)
{
    char buf[80];
    sprintf(differentpackerlines[2] = buf, "%s to %s before trying to unpack?",
		packernames[currentpacker], packernames[whichpacker]);
    return Question(differentpackerlines, "Yes", "No", "Cancel", 2);
}


local str notsamepackerlines[] = {
    "This packet appears to be compressed with a different packer",
    "than the one you have selected, and it does not match any of",
    "the recognizable types currently defined.  Do you want to",
    null,
    null
};

bool AskNotSamePacker(short whichpacker)
{
    char buf[80];
    sprintf(buf, "try unpacking it with %s, or cancel the attempt?",
				packernames[currentpacker]);
    notsamepackerlines[3] = buf;
    return Question(notsamepackerlines, null, "Try it", "Cancel", 1);
}


/*
local str noattacharealines[] = {
    "You have moved this message into an area which does not",
    "support attached files.  Do you want to PICK a different",
    "area, RISK leaving the attachment here (the BBS will",
    "probably ignore it), or CANCEL the area change?",
    null
};

short AskNoAttachmentArea(void)
{
    return Question(noattacharealines, "Pick", "Risk", "Cancel", 2);
}
*/


#ifdef WARN_LONG_REPLY
local str warnlongmesslines[] = {
    null,
    "This may be too long for some BBS or network software",
    "to handle without problems.  This version of Q-Blue",
    "does not yet have a feature for automatically splitting",
    "long messages, but you may want to separate this one",
    "manually into two or more shorter replies.",
    null
};

void WarnLongMess(short n)
{
    char buf[80];
    sprintf(warnlongmesslines[0] = buf,
		"The message you just saved is %lu lines long.", (long) n);
    Question(warnlongmesslines, null, null, "Okay", 0);
}
#endif


local str attemptpreunpackedlines[] = {
    "Your work directory apparently already has a mail packet",
    "unpacked in it.  Do you want to try to LOAD the files",
    "that are there, or SELECT a new packet to decompress,",
    "or not open any mail after all (CANCEL)?",
    null
};

short AskAttemptUnpacked(void)
{
    return Question(attemptpreunpackedlines, "Load", "Select", "Cancel", 2) - 1;
}


local str frquitlines[] = {
    "Do you really want to quit?",
    null
};

local str reallyquitlines[] = {
    "If you wish, you can leave the packet open for reloading later.",
    "Do you want to LEAVE the work directory intact, EMPTY it as is",
    "normally done when closing, or CANCEL (don't quit Q-Blue)?",
    null
};

bool AskReallyQuit(void)
{
    short r;
    r = fakery ? Question(frquitlines, null, "Okay", "Cancel", 0)
		: Question(reallyquitlines, "Leave", "Empty", "Cancel", 0);
    quitleaveopen = r == 2;
    return !!r;
}


local str packemlines[] = {
    "You have written some replies for this mail packet.  Do you want",
    "to PACK them for uploading (or later reloading into Q-Blue) or",
    "IGNORE them so they can be forgotten, or CANCEL (don't close)?",
    null
};

bool AskPackReplies(bool *cancellation)
{
    short r = Question(packemlines, "Pack", "Ignore", "Cancel", 2);
    *cancellation = r > 0;
    return r == 2;
}


local str askreeditlines[] = {
    "You have already written a reply to this message.  Do you want to",
    "RE-EDIT that existing reply, or ADD another one, or neither (CANCEL)?",
    null
};

short AskReedit(void)
{
    return Question(askreeditlines, "Re-edit", "Add new", "Cancel", 2);
}


local str askovnorenlines[] = {
    "!** ERROR **",
    null,
    "in order to create the new upload archive; the Dos",
    null,
    "Do you want to DELETE the old archive instead of",
    "keeping a backup copy of it, or CANCEL?",
    null
};


bool AskOverrideNoRename(bool renaming, str filename)
{
    char line[80], ee[40], line2[120];
    long ie = IoErr();

    sprintjam(line, renaming ? "Could not rename archive file \"%s%s\"" :
			"Could not delete backup file \"%s%s.OLD\"", filename);
    sprintf(ee, "error code was %ld", ie);
    if (ie) {
	Fault(ie, ee, line2, 120);
	strcat(line2, ".");
    } else
	sprintf(line2, "%s (unset).", ee);
    askovnorenlines[1] = line;
    askovnorenlines[3] = line2;
    if (!Question(askovnorenlines, null, "Delete", "Cancel", 1))
	return false;
    if (!DeleteFile(filename)) {
	DosErr("Could not delete archive file\n%s", filename);
	return false;
    }
    return true;
}


local str useddirlines[] = {
    null,
    "already has at least one file present in it.",
    null,
    null,
    null,
    null,
    "Do you want to go ahead and USE this directory",
    "setting, discarding the files there, or do you want",
    "to CHANGE your choice of what directory to use?",
    null
};

bool AskAboutUsedDirs(bool replies, bool looksokay, bool ask)
{
    char buf[80], frm[80];
    str dirt, dire;

    if (ourqsemnode.editingdirs) {
	dirt = replies ? replydir : workdir;
	dire = FixWRpath(dirt);
    } else
	dirt = null;
    StripIntuiMessages(idcort, null);
    sprintf(frm, "%sYour %s directory, \"%%s%%s\",",
				looksokay ? "Note: " : "WARNING!  ",
				dirname[replies ? DREP : DWORK]);
    sprintjam(useddirlines[0] = buf, frm, dirt ? dirt :
				(replies ? replydirinst : workdirinst));
    if (dirt) *dire = 0;
    if (looksokay) {
	useddirlines[2] = "However, it looks like the file(s) are part of a";
	useddirlines[3] = "mail packet such as would normally be put there, so";
	useddirlines[4] = "you may not be concerned about them being deleted.";
    } else {
	useddirlines[2] = "The file(s) present DO NOT appear to be part of any";
	useddirlines[3] = owin ?
			  "mail packet!  Q-Blue may DELETE these files if you" :
			  "mail packet!  Q-Blue WILL DELETE these files if you";
	useddirlines[4] = "do not move them, or use a different directory.";
    }
    if (ask) {
	useddirlines[5] = "";
	return Question(useddirlines, null, "Use", "Change", 0);
    } else {
	useddirlines[5] = null;
	return Question(useddirlines, null, null, "Okay", 0);
    }
}


local str askresetdoorlines[] = {
    "Are you sure you want to discard all changes",
    "you have made to the mail door's current",
    null,
    null
};

bool AskResetDoorFig(bool areas)
{
    askresetdoorlines[2] = areas ?
			"list of which message areas to download?" :
			"keywords, filters, and option settings?";
    return Question(askresetdoorlines, null, "Yes", "Cancel", 0);
}


local str warnnodoorid[] = {
    "WARNING: this mail packet does not contain a DOOR.ID file.",
    "This may mean that ADD and DROP commands don't work, or",
    "that Q-Blue won't be able to address them to the correct",
    "door control name.  The name \"QMAIL\" is used if you try it.",
    null
};

local str warnnoaddrop[] = {
    "WARNING: this mail packet contains a DOOR.ID file which",
    "does not specify ADD and DROP commands.  This means that",
    "ADD and DROP commands probably will not work.",
    null
};

local str warnmaximus[] = {
    "WARNING: this mail packet's DOOR.ID file indicates that",
    "it came from a Maximus BBS's built-in QWK packer.  This",
    "means that ADD and DROP commands probably will not work.",
    null
};

bool WarnNoDoorIdAddDrop(bool maximus)
{
    return Question(wouldahaddrop ? (maximus ? warnmaximus : warnnoaddrop)
				: warnnodoorid, null, null, "Okay", 0);
}


local str asksavefigoverlines[] = {
    null,
    "is not a valid Q-Blue configuration file.  If you",
    "save your setup in this file, you may lose data",
    "you want to keep.  Do you want to go ahead and",
    "save your Q-Blue setup in this file?",
    null
};

bool AskSaveFigOverwrite(str figfile)
{
    char buf[80];
    sprintjam(asksavefigoverlines[0] = buf, "The file \"%s%s\" exists, and",
				figfile);
    return Question(asksavefigoverlines, null, "Okay", "Cancel", 0);
}


local str asksavestriphighlines[] = {
    "This message area does not allow the use of special characters",
    "with ASCII values above 127, such as accented letters, and",
    null,
    "",
    "Do you want the illegal characters to be REPLACED with \"*\"",
    "characters, or do you want a chance to FIX the message",
    "manually before saving this reply?",
    null
};

bool AskStripHighBits(ushort count)
{
    char dracula[32], buf[80];
    if (count == 1)
	strcpy(dracula, "one character");
    else
	sprintf(dracula, "%lu characters", (ulong) count);
    sprintf(buf, "this message contains %s of that type.", dracula);
    asksavestriphighlines[2] = buf;
    return Question(asksavestriphighlines, null, "Replace", "Fix", 0);
}


local str askdeletearchivelines[] = {
    "Do you want to DELETE the original mail archive,",
    null, null, null, null
};

bool AskDeleteArchive(bool quitting, bool *cancelflag)
{
    char aname[80], report[80], minder[80];
    ushort i;

    sprintjam(aname, "\"%s%s\"?", darchivename);
    askdeletearchivelines[1] = aname;
    if (totalunread)
	sprintf(report, "%lu messages out of %lu have not been read.",
					totalunread, totaltoread);
    else
	strcpy(report, "All messages in the packet have been read.");
    askdeletearchivelines[2] = report;
    sprintf(minder, "Select \"Cancel\" if you don't want to %s after all.",
					quitting ? "quit" : "close");
    askdeletearchivelines[3] = minder;
    i = Question(askdeletearchivelines, "Yes", "No", "Cancel", 1);
    *cancelflag = !!i;
    return i == 2;
}


local str asknonreplydeletelines[] = {
    "You have no replies to upload, but a reply packet for",
    "this BBS exists in your uploads directory.  Do you want",
    null,
    "so that it won't be unintentionally uploaded?",
    null
};

bool AskNonReplyDelete(str arcname)
{
    char space[80];
    asknonreplydeletelines[2] = space;
    sprintf(space, "rename upload packet %s as %s.old,", arcname, arcname);
    return Question(asknonreplydeletelines, null, "Yes", "No", 0);
}


bool AskDeleteListedFile(str path, str date, ulong size, ushort dirtok)
{
    char space0[80], space1[80], space2[80];
    str lines[4];

    lines[0] = space0; lines[1] = space1; lines[2] = space2; lines[3] = null;
    sprintf(space0, "Are you sure you want to delete the %ld byte file", size);
    sprintjam(space1, "%s%s", path);
    sprintf(space2, "dated %s, from your %s directory?", date, dirname[dirtok]);
    return Question(lines, null, "Yes", "No", 0);
}


local str askdeltagline[] = {
    null, null, null
};

bool AskDeleteTagline(str tag, bool nonmanual /*, str file*/ )
{
/*  char space[80]; */
/*  sprintjam(askdeltagline[2] = space, "from the file %s%s?", file); */
    askdeltagline[0] = nonmanual ?
	"Do you want to delete this original version of the tagline you edited?"
	: "Are you sure you want to delete this tagline?";

    askdeltagline[1] = tag;
    return Question(askdeltagline, null, "Yes", "No", 0);
}


local str askretryupllines[] = {
    null,
    "no valid upload packet can be produced at this time,",
    "though all replies exist intact in Q-Blue's memory.",
    "The operation needs to be retried.  If you can correct",
    "the problem, do so and then select \"Now\" to make another",
    "attempt.  If you select \"Later\", another attempt will be",
    "made the next time you write or edit a reply, or when it",
    "is time to pack replies.  Do you want to retry writing",
    "the information to the replies directory NOW or LATER?",
    null
};

bool AskRetryWriteUPL(void)
{
    char buf[100];
    sprintf(askretryupllines[0] = buf, "Because the message %s file "
			"could not be written,", qwk ? "data" : "header");
    return Question(askretryupllines, null, "Now", "Later", 1);
}


local str asktruncatedtextlines[] = {
    null, null,
    "you want to try reading the messages anyway?",
    null
};

bool AskTruncatedText(ulong bad, ulong total, str packetname)
{
    char buf0[100], buf1[100];
    sprintf(asktruncatedtextlines[0] = buf0, "The text portion of %lu out "
				"of %lu messages cannot be found", bad, total);
    sprintf(asktruncatedtextlines[1] = buf1, "in the packet.  Probably the "
				"file %s.DAT is truncated.  Do", packetname);
    return Question(asktruncatedtextlines, null, "Yes", "Cancel",
				bad * 10 < total);
}


local str askunsavedtagstext[] = {
    "The current set of taglines is about to be unloaded from memory.",
    null,
    "loaded or last saved.  Do you want to SAVE the modified set of",
    "lines, DISCARD the changes, or CANCEL this action?",
    null
};

short AskUnsavedTaglines(ushort count)
{
    char buf[100];
    static char common[] = "been added or deleted since the file was";
    if (count == 1)
	sprintf(buf, "One tagline has %s", common);
    else
	sprintf(buf, "%ld taglines have %s", (ulong) count, common);
    askunsavedtagstext[1] = buf;
    return Question(askunsavedtagstext, "Save", "Discard", "Cancel", 2);
}


/* ----------------------------------------------------------------- */


ubyte pword[23];

struct StringInfo pwstr = {
    pword, null, 0, 21, 0, 0, 0, 0, 0, 0, &stringex, 0, null
};

STRINGBORDER(pwbox)

struct Gadget pwgag = {
    null, 46, 27, 168, 8, GFLG_STRINGEXTEND, GACT_RELVERIFY | GACT_STRINGLEFT,
    GTYP_STRGADGET, &pwbox, null, null, 0, &pwstr, 900, null
};

struct ExtNewWindow pwneww = {
    190, 60, 260, 100, 0, 1, IDCMP_GADGETUP | IDCMP_CLOSEWINDOW | IDCMP_RAWKEY,
    WFLG_NOCAREREFRESH | WFLG_CLOSEGADGET | WFLG_DRAGBAR | WFLG_DEPTHGADGET
		| WFLG_ACTIVATE | WFLG_NW_EXTENDED,
    null, null, "Enter the password:", null, null,
    0, 0, 0, 0, CUSTOMSCREEN, null
};

struct WhereWin pwww = { &pwneww };

struct Window *pwwin;

local struct RastPort *pwrp;
local bool right;
local short wrong, labelbase;
local str passanswer;

bool DoAskPasswordIDCMP(struct IntuiMessage *im)
{
    if (im->Class == IDCMP_RAWKEY && !(im->Qualifier & ALTKEYS)) {
	char k = KeyToAscii(im->Code, im->Qualifier);
	if (k == '\t')
	    ActivateGag(&pwgag, pwwin);
	else if (k == ESC)
	    return true;
    } else if (im->Class == IDCMP_CLOSEWINDOW)
	return true;
    else if (im->Class == IDCMP_GADGETUP && GAGFINISH(im)) {
	if (right = !stricmp(pword, passanswer))
	    return true;
	if (++wrong >= 3)
	    return true;
	pword[0] = 0;
	RefreshGadgets(&pwgag, pwwin, null);
	MoveNominal(pwrp, 16, labelbase);
	Text(pwrp, "   INCORRECT -- try again   ", 28);
	MoveNominal(pwrp, 16, labelbase + fight + 1);
	Text(pwrp, "                            ", 28);
	ActivateGadget(&pwgag, pwwin, null);
    }
    return false;
}

bool AskPassword(str answer)
{
    short co = countoff, pi, pa;

    while (countoff > 0)
	PortOn();
    pi = stringex.Pens[0];
    pa = stringex.ActivePens[0];
    stringex.Pens[0] = stringex.Pens[1];
    stringex.ActivePens[0] = stringex.ActivePens[1];
    pwgag.TopEdge = fight + 12;
    pwneww.Height = 4 * fight + 29;
    if (pwwin = OpenBlueGagWin(&pwww, &pwgag)) {
	labelbase = tifight + fight + 19 + font->tf_Baseline;
	right = false;
	wrong = 0;
	passanswer = answer;
	pwrp = pwwin->RPort;
	SetAPen(pwrp, LABELCOLOR);
	SetBPen(pwrp, backcolor);
	MoveNominal(pwrp, 20, labelbase);
	Text(pwrp, "This packet is protected by", 27);
	MoveNominal(pwrp, 20, labelbase + fight + 1);
	Text(pwrp, "a password, for privacy.", 24);
	pword[0] = 0;
	/* RefreshGadgets(&pwgag, pwwin, null); */
	ActivateGag(&pwgag, pwwin);
	EventLoop(&DoAskPasswordIDCMP);
	CloseBlueGagWin(pwwin);
	pwwin = null;
    } else
	WindowErr("asking the packet's password.");
    stringex.Pens[0] = pi;
    stringex.ActivePens[0] = pa;
    while (countoff < co)
	PortOff();
    return right;
}


void ShowSysopEtc(void)
{
    char lz[11][80], dat[DATELEN];
    str linez[11];
    struct DateStamp dd;
    short i;

    if (!areaz.messct) return;
    sprintf(lz[0], "!BBS info for %s packet \"%s\":", qwk ? "QWK" : (version3
				? "v3 Blue Wave" : "v2 Blue Wave"), packetname);
    strcpy(lz[1], " ");
    Stranks(bbsname);
    Stranks(sysopname);
    ux2DS(&dd, downloaddate);
    qed = true;
    DateString(&dd, dat);	/* date & time separated by nul byte! */
    qed = false;
    dat[strlen(dat)] = ' ';
    if (darchivename[0])
	sprintjam(lz[2], "Pkt. file: %s%s", darchivename);
    else
	strcpy(lz[2], "Pkt. file:  (unknown)");
    sprintf(lz[3], "File date: %s", dat);
    sprintf(lz[4], " BBS name: %s", bbsname);
    sprintf(lz[5], "    Sysop: %s", sysopname);
    sprintf(lz[6], "Your name: %s", myloginame);
    if (qwk) {
	sprintf(lz[7], " Location: %s", bbscity);
	sprintf(lz[8], "  Phone #: %s", bbsphone);
	sprintf(lz[9], "Mail door control name: %s",
				wouldahaddrop ? doorconn : " (none)");
	lz[10][0] = 0;
    } else {
	sprintf(lz[7], "Alt. name: %s", myothername);
	if (hostnet) {
	    strcpy(lz[8], "Net addr.: ");
	    FormatNetAddress(lz[8] + 11, hostzone, hostnet, hostnode,hostpoint);
	    if (hostcredits || hostdebits)
		sprintf(lz[9], "Netmail credit: %ld credits - %ld debits",
				(long) hostcredits, (long) hostdebits);
	    else
		lz[9][0] = 0;
	    lz[10][0] = 0;
	} else
	    lz[8][0] = 0;
    }
    for (i = 0; i < 11; i++)
	linez[i] = lz[i][0] ? lz[i] : null;
    Question(linez, null, null, "Okay", 0);
}


void WorkbenchRequesterArgs(str body, str options, ...)
{
    static struct EasyStruct es = { sizeof(es), 0, "Q-Blue", null, null };
    va_list args;

    va_start(args, options);
    if (!(IntuitionBase = (adr) OpenLibrary("intuition.library", 37)))
	return;
    es.es_TextFormat = body;
    es.es_GadgetFormat = options;
    EasyRequestArgs(null, &es, null, (adr) args);
    CloseLibrary((adr) IntuitionBase);
    va_end(args);
}


#define AUTOSTUFF   AUTOFRONTPEN, AUTOBACKPEN, AUTODRAWMODE

local struct IntuiText ailine2 = {
    AUTOSTUFF, 15, 15, null, "AmigaDOS 2.04 or newer", null
}, ailine1 = {
    AUTOSTUFF, 15, 5, null, "Q-Blue " VERSION " requires", &ailine2
}, aineg = {
    AUTOSTUFF, AUTOLEFTEDGE, AUTOTOPEDGE, null, "Okay", null
};


void VersionError(void)
{
    if (!(IntuitionBase = OpenL("intuition")))
	return;
    AutoRequest(null, &ailine1, null, &aineg, 0, 0, 320, 62);
    CloseLibrary((adr) IntuitionBase);
}
