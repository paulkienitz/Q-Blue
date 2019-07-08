/* load and save setup for Q-Blue. */

#include <exec/memory.h>
#include <dos/dos.h>
#include <dos/dostags.h>
#include <graphics/displayinfo.h>
#include <devices/input.h>
#include <intuition/sghooks.h>
#include <intuition/intuition.h>
#include <libraries/commodities.h>
#include <clib/commodities_protos.h>
#ifndef __NO_PRAGMAS
#  include <pragmas/commodities_lib.h>
#endif
#include "qblue.h"
#include "pigment.h"


#ifdef BETA
long pOpkeylen = 0;
#  undef local
#  define local
#endif

#define COOKIE      0x94DEC2DC
#define STRINGSLIM  150
#define ENDPAD      32

#define POPKEYLEN   50


typedef struct CommandLineInterface SCLI;


bool DoFileRequest(bool saving, bool special, str result,
				str deflt, str pattern, str hail);
bool AskSaveFigOverwrite(str figfile);
void FullPresentPackers(void);
void ScaleGadget(struct Gadget *gg);
void UnScaleGadget(struct Gadget *gg);
void MakeInstanceNames(ushort instouse);
void Resort(bool movement);
void MakeBotGadgets(void);
bool MakeLotsaRoom(void);
void NewSort(short nso);
ulong MagicIdentifierNumber(void);
long sputCryptName(long p);


import struct WhereWin cww, cbitww, zww, eww, dww, oww, sww, pww, lww, fww, yww;
import struct WhereWin tww, drww, llww, arww, awww, atww, naww, ffrww, aslfakww;
import struct WhereWin bwdww, pwww, bkfww, fontfakww, modefakww, modepropfakww;
import struct Window *cwin;
#ifdef WINDOW_ICONIFY
import struct ExtNewWindow newiconw;
import bool iconmoved;
#endif

import struct IntuiText *(cgtquotes[4]);
import struct Border elongbox, elongboxA;
import ushort palette[8], palette4[4];
#ifdef SOMEDAY
import ushort lb_palt[8], lb_palt4[4];
#endif

import str aboutlines[], functionkeys[20];
import char newfontname[], configfilename[], userfigfile[], signature[];
import char tryopename[], taglinesfile[], qsemname[], quoteheader[];
import char carbonheader[], savename[], defsavename[], bversionL[];

import bool autopenlist, customode, latercustomode, dos3, onefigloaded;
import bool customautoscroll, custom4color, customshowprops, lightbg;
import long modeID;
import short ourinstance;
import ushort reg_number;


char workdir[PATHLEN + 8] = "", replydir[PATHLEN + 8] = "",
		uploaddir[PATHLEN + 2] = "", downloaddir[PATHLEN + 2] = "",
		bbsesdir[PATHLEN + 2] = "", anyname[NAMELEN + 2];

char packernames[GREENBAY][PACKNAMELEN] = {
    "Zip 2.x", "Zip 1.x", "LHA", "LZX", "Zoo", "Arc"
};

char unpackommands[GREENBAY][COMMANDLEN] = {
    "UnZip -jo @A", "UnZip -jo @A", "LhA -m -x0 x @A", "LZX -m -X0 x @A",
    "Zoo xSO @A", "Arc xw @A"
};

char packommands[GREENBAY][COMMANDLEN] = {
    "Zip -k @A *", "Zip -0k @A *", "LhA -0 a @A #?", "LZX -X0 a @A #?",
    "Zoo a @A *", "Arc a @A #?"
};

char spoors[GREENBAY][SPOORLEN] = {
    "50 4B 03 04", "50 4B 03 04 0A", "? ? 2D 6C 68 ? 2D", "4C 5A 58",
    "5A 4F 4F 20", "1A"
};


BPTR fakepath = 0;

ushort packers = 6, currentpacker = 0, wrapmargin = 76, sorder = 0;
ushort quotedefault = 3, taheight = 8, fight = 8;
ushort tagstyle = 0 /* none */, tagleadin = 1 /* "* Q-Blue" */ ;

char edit2command[COMMANDLEN + 2] = "";
char edit1command[COMMANDLEN + 2] = "Ed @F";
/*  "Ed @F WINDOW \"CON:0/12//188/Edit reply/CLOSE/SCREEN @P\""; */

char editinfile[PATHLEN + 2] = "", editoutfile[PATHLEN + 2] = "RAM:Reply";

bool conman = false, beepu = false, showlistwin = false, bgupdate = true,
		backbefore = true, frontafter = true, waste = false,
		formfeed = true, lastfirst = true, addRe = true,
		showareawin = true, laterlace = false, laterfourcolors,
		dubedit, gagrow = true, strchain = true, askdelete = true,
		showsizes = false, flushreplies = true, editstrip = true,
		popupscroller = true;

char printerpath[PATHLEN] = "PRT:";
char fontname[PATHLEN + 5], popkeyname[POPKEYLEN] = "";

local BHandle shand;
local bool failage;

struct Library *CxBase;

local bool poptrunc = false, hotinstalled = false;
local IX popexpression;
local struct Interrupt hotrupt;
local struct IOStdReq hotio;

struct TextAttr ta = { fontname, 8, 0, 0 };


struct FigHeader {
    long cookie;			/* constant magic value */
    short version;
    short packertypes;			/* how many compressors: 1 to 8 */
    short stringses;			/* total number of text strings */
};
/* version values: 0 = v0.7, 1 = v0.8 beta, 2 = v0.93 public beta, 3 = v1.0,
   4 = v1.9 public beta, 5 = v2.0, 2.1, & 2.2, 6 = v2.3, 7 = 2.4 (final?) */


struct FigTail {
    long cookie2;			/* repeat of magic value */
    short fontsize;			/* height of font */
    bool slace;				/* non-custom screen is interlace */
    short qdefault;			/* still stores 0.7-compatible number */
    bool sconman, bgup, slistwin, swelcome;	/* user option flags */
    ushort sregnum;			/* disguised user serial number */
    bool ubleep;			/* flash if message is personal */
    short wrapmar;			/* right margin for quoting */
    bool sfront, sback, swaste;		/* editor screen front/back; waste */
    short ssorder, curpacker;		/* ssorder meaning differs w/ version */
    ushort spalette[8];			/* dammit -- v0.7 wrote garbage here! */

/* This is where the fields new after 0.7 start: */
    bool slastfirst, snoformfeed, snoaddRe, snoareawin;	/* more option flags */
    bool qdefaultadd;			/* fudge-factor added to qdefault */
    ushort spalette4[4];		/* palette for four color screens */
    bool sfourcolor;			/* all 20 bytes of 0.7 zero pad used */
/* Do not read anything after this point from a 0.7 file! */

/* This is where the fields new after 0.8 beta start: */
    bool snogagrow;			/* true if hide bottom row of gadgets */
    short slfcoords[4];			/* file list window left/top/wid/hi */
    short sllcoords[4];			/* message list window coords */
    short srlcoords[4];			/* read areas list coords */
    short salcoords[4];			/* compose areas list coords */
    short sfreqcoords[4];		/* ASL requester coords */
    short scomposecor[2];		/* compose window left/top corner */
    short snetbitcor[2];		/* netmail flags window corner */
    short ssearchcor[2];		/* search window corner */
    short sdirscor[2];			/* directories setup window corner */
    short seditcor[2];			/* editor setup window corner */
    short scompresscor[2];		/* compressors setup window corner */
    short soptioncor[2];		/* options setup window corner */
    short siconcor[2];			/* AppIcon position; not used */

/* Start of fields new after v0.93 public beta: */
    short satcoords[4];			/* add/drop areas list coords */
    short srequestcor[2];		/* file download request corner */
    short sbwdoorcor[2];		/* BW door options window corner */
    short skeyfilcor[2];		/* keywords & filters window corner */
    short sfakefreqcor[2];		/* fake file requester corner */
    short snewareacor[2];		/* qwk new area creation corner */
    short spasswordcor[2];		/* password prompt window corner */

/* Start of fields new after release 1.0: */
    short sfonscor[2];			/* font&screen setup window corner */
    short stagcoords[4];		/* tagline window coords */
    bool scustomode;			/* using custom screen mode? */
    ushort stagstyle, stagleadin;	/* none/random/manual, "..."/*Q-Blue */
    ushort slastscrheight;		/* screen height of last mode used */
    long smodeid;			/* display ID for custom screen mode */
    short sfontcoords[4];		/* position of font requester */
    short smodecoords[4];		/* position of screenmode requester */
    short smodepropcoords[2];		/* position of properties list window */
    ulong magicality;			/* identifies registered configs */
    bool scustomfourcolor, scustomautoscroll, scustomshowprops;
    short sbbslocalcor[2];		/* position of BBS local setup window */
    bool snostrchain;			/* don't activate next string gadget */
    short sreplying[2];			/* position of Replying setup window */
    bool snoaskdelete;			/* do not ask if delete packet */

/* Start of fields new after v1.9 public beta:   (242 bytes down to here) */
    short stagstyleextra;		/* add to stagstyle */

/* Start of fields new after v2.0/2.1: */
    bool sshowsizes;			/* the Show Message Sizes option */
    bool sflushreplies;			/* the Flush Reply Dir option */
    bool seditnostrip;			/* don't strip funny ascii for editor */
    
/* Start of fields new after v2.3: */
    bool snopopupscroller;		/* turn off hidden popup scroller */

#ifdef SOMEDAY
    bool slightbg;			/* light background; not used yet */
    ushort slb_palette[8], slb_palette4[4];	/* light background palettes */
#endif
};

#define MINTAILSIZE offsetof(struct FigTail, slastfirst)


/* -------------------------------------------------------------------------- */


local void FixAWindow(struct WhereWin *ww, short *ltwh, ushort height)
{
    bool sizer = ww != &modepropfakww &&
			(!ww->nw || ww->nw->Flags & WFLG_SIZEGADGET);
    if (ltwh[0] < 0)
	return;
    ww->left = ltwh[0];
    ww->top = ltwh[1];
    ww->moved = true;
    ww->scrheight = height;
    if (sizer && ltwh[3] > 0) {
	ww->width = ltwh[2];
	ww->height = ltwh[3];
    }
}


local void SaveAWindow(short *ltwh, struct WhereWin *ww)
{
    bool sizer = ww != &modepropfakww &&
			(!ww->nw || ww->nw->Flags & WFLG_SIZEGADGET);
    if (ww->moved && ww->scrheight == scr->Height) {
	ltwh[0] = ww->left;
	ltwh[1] = ww->top;
	if (sizer) {
	    ltwh[2] = ww->width;
	    ltwh[3] = ww->height;
	}
    } else {
	ltwh[0] = ltwh[1] = -1;
	if (sizer)
	    ltwh[2] = ltwh[3] = -1;
    }
}


void FixWindowCoords(struct FigTail *t, short ver, ushort height)
{
    if (ver < 4)	/* must be 1.9 or newer; window sizes are different */
	return;
    FixAWindow(&fww, t->slfcoords, height);
    FixAWindow(&lww, t->sllcoords, height);
    FixAWindow(&arww, t->srlcoords, height);
    FixAWindow(&awww, t->salcoords, height);
    FixAWindow(&aslfakww, t->sfreqcoords, height);
    FixAWindow(&cww, t->scomposecor, height);
    FixAWindow(&cbitww, t->snetbitcor, height);
    FixAWindow(&zww, t->ssearchcor, height);
    FixAWindow(&dww, t->sdirscor, height);
    FixAWindow(&eww, t->seditcor, height);
    FixAWindow(&pww, t->scompresscor, height);
    FixAWindow(&oww, t->soptioncor, height);
#ifdef WINDOW_ICONIFY
    if (t->siconcor[0] >= 0 && t->siconcor[1] >= 0)
	newiconw.LeftEdge = t->siconcor[0], newiconw.TopEdge = t->siconcor[1];
#endif
/*  if (ver >= 3) {  */
	FixAWindow(&atww, t->satcoords, height);
	FixAWindow(&drww, t->srequestcor, height);
	FixAWindow(&bwdww, t->sbwdoorcor, height);
	FixAWindow(&bkfww, t->skeyfilcor, height);
	FixAWindow(&ffrww, t->sfakefreqcor, height);
	FixAWindow(&naww, t->snewareacor, height);
	FixAWindow(&pwww, t->spasswordcor, height);
/*	if (ver >= 4) {  */
	    FixAWindow(&sww, t->sfonscor, height);
	    FixAWindow(&tww, t->stagcoords, height);
	    FixAWindow(&fontfakww, t->sfontcoords, height);
	    FixAWindow(&modefakww, t->smodecoords, height);
	    FixAWindow(&modepropfakww, t->smodepropcoords, height);
	    FixAWindow(&llww, t->sbbslocalcor, height);
	    FixAWindow(&yww, t->sreplying, height);
/*	}
    } */
}


void SaveWindowCoords(struct FigTail *t)
{
    SaveAWindow(t->slfcoords, &fww);
    SaveAWindow(t->sllcoords, &lww);
    SaveAWindow(t->srlcoords, &arww);
    SaveAWindow(t->salcoords, &awww);
    SaveAWindow(t->sfreqcoords, &aslfakww);
    SaveAWindow(t->scomposecor, &cww);
    SaveAWindow(t->snetbitcor, &cbitww);
    SaveAWindow(t->ssearchcor, &zww);
    SaveAWindow(t->sdirscor, &dww);
    SaveAWindow(t->seditcor, &eww);
    SaveAWindow(t->scompresscor, &pww);
    SaveAWindow(t->soptioncor, &oww);
#ifdef WINDOW_ICONIFY
    if (iconmoved)
	t->siconcor[0] = newiconw.LeftEdge, t->siconcor[1] = newiconw.TopEdge;
    else
#endif
	t->siconcor[0] = t->siconcor[1] = -1;
    SaveAWindow(t->satcoords, &atww);
    SaveAWindow(t->srequestcor, &drww);
    SaveAWindow(t->sbwdoorcor, &bwdww);
    SaveAWindow(t->skeyfilcor, &bkfww);
    SaveAWindow(t->sfakefreqcor, &ffrww);
    SaveAWindow(t->snewareacor, &naww);
    SaveAWindow(t->spasswordcor, &pwww);
    SaveAWindow(t->sfonscor, &sww);
    SaveAWindow(t->stagcoords, &tww);
    SaveAWindow(t->sfontcoords, &fontfakww);
    SaveAWindow(t->smodecoords, &modefakww);
    SaveAWindow(t->smodepropcoords, &modepropfakww);
    SaveAWindow(t->sbbslocalcor, &llww);
    SaveAWindow(t->sreplying, &yww);
}


local void UseValidConfig(str *stringys, struct FigHeader *head,
				struct FigTail *tail, bool spoorulate,
				short afterpacks)
{
    short i, len = head->packertypes + 15, headversion = head->version;

/*  if (fakery || !readareaz.messct) */
	strcpy(workdir, stringys[0]);
/*  if (!replylock) */
	strcpy(replydir, stringys[1]);
    strcpy(uploaddir, stringys[2]);
    strcpy(downloaddir, stringys[3]);
    strcpy(bbsesdir, stringys[4]);
    if (!cwin) {	/* MakeInstanceNames can't protect commands! */
	strcpy(editinfile, stringys[5]);
	strcpy(editoutfile, stringys[6]);
	strcpy(edit2command, stringys[8]);
	strcpy(edit1command, stringys[9]);
    }
    strcpy(newfontname, stringys[7]);
    strcpy(anyname, stringys[10]);
    strcpy(printerpath, stringys[11]);
    strcpy(taglinesfile, stringys[12]);
    strcpy(quoteheader, stringys[13]);
    strcpy(defsavename, stringys[14]);
    dubedit = edit2command[0] && editinfile[0];
    packers = head->packertypes;
    for (i = 0; i < GREENBAY; i++)
	if (i < packers) {
	    strcpy(packernames[i], stringys[i + 15]);
	    strcpy(packommands[i], stringys[i + len]);
	    strcpy(unpackommands[i], stringys[i + len + packers]);
	    if (spoorulate)
		strcpy(spoors[i], stringys[i + len + (packers << 1)]);
	    else
		spoors[i][0] = 0;
	} else
	    packernames[i][0] = packommands[i][0]
				= unpackommands[i][0] = spoors[i][0] = 0;
    if (head->stringses > afterpacks) {
	strcpy(carbonheader, stringys[afterpacks]);
	/* afterpacks + 1 = registered user name, encrypted */
	strcpy(signature, stringys[afterpacks + 2]);
	/* 7 more strings are reserved */
	if (head->stringses >= afterpacks + 30)
	    for (i = 0; i < 20; i++)
		strcpy(functionkeys[i], stringys[i + afterpacks + 10]);
    }
    taheight = tail->fontsize;
    laterlace = tail->slace;
    if (!scr) {
	lace = laterlace;
	ta.ta_YSize = taheight;
	strcpy(fontname, newfontname);
    }
    conman = tail->sconman;	/* ignored; we set actual value when needed */
    bgupdate = tail->bgup;
    showlistwin = tail->slistwin;
    beepu = tail->ubleep;
    wrapmargin = tail->wrapmar;
    if (wrapmargin < 30 || wrapmargin > 80)
	wrapmargin = 76;
    if (!cwin) {
	backbefore = tail->sback;
	frontafter = tail->sfront;
    }
    waste = tail->swaste;
    if (!cwin)
	sorder = tail->ssorder;
    formfeed = !tail->snoformfeed;
    addRe = !tail->snoaddRe;
    showareawin = !tail->snoareawin;
    currentpacker = tail->curpacker;
    if (headversion == 0) {
	quotedefault = tail->qdefault + (tail->qdefault == 3);
	if (!cwin && (lastfirst = (sorder == 4)))
	    sorder = 3;				/* convert 0.7 sorder num */
    } else {					/* v0.8 beta 1 or newer */
	quotedefault = tail->qdefault + tail->qdefaultadd;
	lastfirst = !!tail->slastfirst;
	for (i = 0; i < 8; i++)
	    palette[i] = tail->spalette[i];
	if (tail->spalette4[0] != tail->spalette4[1]
				&& tail->spalette4[1] != tail->spalette4[2])
	    for (i = 0; i < 4; i++)
		palette4[i] = tail->spalette4[i];
	laterfourcolors = tail->sfourcolor;
	if (scr)
	    if (fourcolors)
		LoadRGB4(&scr->ViewPort, palette4, 4);
	    else
		LoadRGB4(&scr->ViewPort, palette, 8);
	else
	    fourcolors = laterfourcolors;
	gagrow = !tail->snogagrow;
	if (headversion >= 4) {			/* 1.0x beta or newer */
	    latercustomode = tail->scustomode;
	    if (!scr)
		customode = latercustomode;
	    modeID = tail->smodeid;
	    tagstyle = tail->stagstyle;
	    tagleadin = tail->stagleadin;
	    custom4color = tail->scustomfourcolor;
	    customautoscroll = tail->scustomautoscroll;
	    customshowprops = tail->scustomshowprops;
	    strchain = !tail->snostrchain;
	    askdelete = !tail->snoaskdelete;
	    showsizes = flushreplies = false;
	    if (headversion >= 5) {
		tagstyle += tail->stagstyleextra;
		if (headversion >= 6) {
		    showsizes = tail->sshowsizes;
		    flushreplies = tail->sflushreplies;
		    editstrip = !tail->seditnostrip;
		    if (headversion >= 7) {
			popupscroller = !tail->snopopupscroller;
#ifdef SOMEDAY
			lightbg = !!tail->slightbg;
			if (tail->slb_palette[0] != tail->slb_palette[1] &&
				    tail->slb_palette[1] != tail->slb_palette[2]) {
			    for (i = 0; i < 8; i++)
				lb_palt[i] = tail->slb_palette[i];
			    for (i = 0; i < 4; i++)
				lb_palt4[i] = tail->slb_palette4[i];
			}
#endif

		    }
		}
	    }
	}
    }
    if (headversion < 4) {
	latercustomode = custom4color = customautoscroll = showsizes
				= flushreplies = customshowprops = false;
	modeID = HIRES_KEY;
	tagstyle = tagleadin = 0;
    }
    if (headversion < 6)
	editstrip = false;		/* with no config it defaults true */
    if (sorder > 5 || sorder < 0)
	sorder = 0;
    if (currentpacker > packers || currentpacker < 0)
	currentpacker = 0;
    FixWindowCoords(tail, headversion, tail->slastscrheight);
    FullPresentPackers();
    onefigloaded = true;
}


short ReadConfig(void)
{
    ustr p, figbuffer = InhaleFile(configfilename);
    short len, i, j, afterpacks;
    struct FigHeader *head;
    struct FigTail *tail;
    str stringys[STRINGSLIM];
    short lenz[STRINGSLIM];
    bool spoorulate;

    if (!figbuffer)					/* use defaults */
	return (IoErr() == ERROR_NO_FREE_STORE ? 1 : 3);
    head = (adr) figbuffer;
    if (fibize <= sizeof(*head) + MINTAILSIZE
		|| head->packertypes < 1 || head->packertypes > GREENBAY
		|| head->stringses < 15 + 3 * head->packertypes
		/* || head->stringses > STRINGSLIM */ || head->cookie != COOKIE)
	goto corrupt;
    p = figbuffer + sizeof(struct FigHeader);
    for (i = 0; i < head->stringses; i++) {
	j = strlen(p);
	if (i < STRINGSLIM)			/* don't smash the stack! */
	    stringys[i] = p, lenz[i] = j;
	p += j + 1;
	if (p - figbuffer > fibize - MINTAILSIZE)
	    goto corrupt;
    }
    if (head->stringses > STRINGSLIM) head->stringses  = STRINGSLIM;
    if ((long) p & 1)			/* word align */
	p++;
    tail = (adr) p;
    if (tail->cookie2 != COOKIE)
	goto corrupt;

/* The first fifteen strings are fixed, the ones after are variable.  In all
versions, the first eight strings are workdir, replydir, uploaddir, downloaddir,
bbsesdir, editinfile, editoutfile, and fontname, each limited to PATHLEN - 1
chars.  The bbsesdir space is not actually used until version 4.  Strings 9
and 10 (numbered from 1, not zero) are edit2command and edit1command, of
COMMANDLEN - 1 chars each.  String 11 is anyname, NAMELEN - 1 chars.  String 12
is printerpath (size PATHLEN), unused in version 0.  Strings 13 and 14 are the
taglines filename and the quote header string (PATHLEN - 1 and COMMANDLEN - 1
chars), unused before version 4.  String 15 is default msg save path
(COMMANDLEN), also unused before version 4.  Next come (packertypes) packernames
of length PACKNAMELEN - 1, and then (2 * packertypes) strings of length
COMMANDLEN - 1 giving the packing commands for each packer type, and then the
unpacking commands for each.  If the stringses field at the beginning is large
enough (15 + 4 * packers rather than 15 + 3 * packers), then there are
(packertypes) pattern strings of length SPOORLEN - 1.  Strings beyond this point
are not present in versions 1.0 and older; after that there are now ten strings.
The first is the carbon copy header (COMMANDLEN), the second is the user name
(if registered) in encrypted form padded with five bytes of garbage, otherwise
(if unregistered) garbage that won't decrypt; the third is the end-of-message
signature (SIGNATURELEN), the next seven are reserved.  After that, in versions
after 2.1, are twenty function key strings (COMMANDLEN) -- currently these are
not actually used.  If any of these string limits are lengthened later, the
longer version or the excess WILL HAVE TO BE STORED IN A SEPARATE SLOT so older
versions can read the files without overrunning their buffers. */

    for (i = 0; i < 8; i++)
	if (lenz[i] >= PATHLEN)
	    goto corrupt;
    if (lenz[8] >= COMMANDLEN || lenz[9] >= COMMANDLEN || lenz[10] >= NAMELEN
			|| lenz[11] >= PATHLEN || lenz[12] >= PATHLEN
			|| lenz[13] >= COMMANDLEN)
	goto corrupt;
    len = head->packertypes + 15;
    for (i = 15; i < len; i++)
	if (lenz[i] >= PACKNAMELEN)
	    goto corrupt;
    j = len + (head->packertypes << 1);
    for (i = len; i < j; i += 2)
	if (lenz[i] >= COMMANDLEN || lenz[i + 1] >= COMMANDLEN)
	    goto corrupt;
    if (spoorulate = head->stringses >= j + head->packertypes) {
	for (i = j; i < j + head->packertypes; i++)
	    if (lenz[i] >= SPOORLEN)
		goto corrupt;
	afterpacks = j + head->packertypes;
    } else
	afterpacks = j;
    if (head->stringses > afterpacks && head->stringses < afterpacks + 30
				&& head->stringses != afterpacks + 10)
	goto corrupt;
    if (head->stringses >= afterpacks + 30)
	for (i = afterpacks + 10; i < afterpacks + 30; i++)
	    if (lenz[i] >= COMMANDLEN)
		goto corrupt;
    if (head->stringses <= afterpacks || (lenz[afterpacks] < COMMANDLEN
				&& lenz[afterpacks + 2] < SIGNATURELEN)) {
	UseValidConfig(stringys, head, tail, spoorulate, afterpacks);
	ExhaleFile();
	return 0;
    }
corrupt:
    ExhaleFile();
    return 2;
}


long sput(long p, str in)	/* returns updated total of bytes written */
{
    size_t l = strlen(in) + 1;
    if (!failage && BWrite(shand, in, 1, l) < l)
	failage = true;
    return p + l;
}


void SaveConfig(void)
{
    long p;
    short i;
    struct FigHeader head;
    struct FigTail tail;

    if (!(shand = BOpen(configfilename, true))) {
	Err("Could not create configuration\nfile %s", configfilename);
	return;
    }
    head.cookie = COOKIE;
    head.version = QBVERSION;
    head.packertypes = packers;
    head.stringses = 45 + packers * 4;
    if (BWrite(shand, &head, p = sizeof(head), 1) < 1)
	failage = true;
    p = sput(p, workdir);
    p = sput(p, replydir);
    p = sput(p, uploaddir);
    p = sput(p, downloaddir);
    p = sput(p, bbsesdir);
    p = sput(p, editinfile);
    p = sput(p, editoutfile);
    p = sput(p, newfontname);
    p = sput(p, edit2command);
    p = sput(p, edit1command);
    p = sput(p, anyname);
    p = sput(p, printerpath);
    p = sput(p, taglinesfile);
    p = sput(p, quoteheader);
    p = sput(p, defsavename);
    for (i = 0; i < packers; i++)
	p = sput(p, packernames[i]);
    for (i = 0; i < packers; i++)
	p = sput(p, packommands[i]);
    for (i = 0; i < packers; i++)
	p = sput(p, unpackommands[i]);
    for (i = 0; i < packers; i++)
	p = sput(p, spoors[i]);
    p = sput(p, carbonheader);
    p = sputCryptName(p);
    p = sput(p, signature);
    for (i = 0; i < 7; i++)		/* seven reserved slots */
	p = sput(p, "");
    for (i = 0; i < 20; i++)
	p = sput(p, functionkeys[i]);
    if (p & 1)
	sput(p, "");			/* word align with an extra zero byte */
    tail.cookie2 = COOKIE;
    tail.fontsize = taheight;
    tail.slace = laterlace;
    tail.qdefaultadd = quotedefault >= 3;	/* for 0.7 compatibility */
    tail.qdefault = quotedefault - tail.qdefaultadd;
    tail.sconman = conman;
    tail.bgup = bgupdate;
    tail.slistwin = showlistwin;
    tail.swelcome = false;
    tail.ubleep = beepu;
    tail.wrapmar = wrapmargin;
    tail.sback = backbefore;
    tail.sfront = frontafter;
    tail.swaste = waste;
    tail.ssorder = sorder;
    tail.slastfirst = lastfirst;
    tail.snoformfeed = !formfeed;
    tail.snoaddRe = !addRe;
    tail.curpacker = currentpacker;
    tail.snoareawin = !showareawin;
#ifdef SOMEDAY
    tail.slightbg = lightbg;
    for (i = 0; i < 8; i++)
	tail.spalette[i] = palette[i], tail.slb_palette[i] = lb_palt[i];
    for (i = 0; i < 4; i++)
	tail.spalette4[i] = palette4[i], tail.slb_palette4[i] = lb_palt4[i];
#else
    for (i = 0; i < 8; i++)
	tail.spalette[i] = palette[i];
    for (i = 0; i < 4; i++)
	tail.spalette4[i] = palette4[i];
#endif
    tail.sfourcolor = laterfourcolors;
    tail.snogagrow = !gagrow;
    tail.smodeid = modeID;
    tail.scustomode = latercustomode;
    /* we store tag style "Sequence" as "Random" for compatibility; */
    /* the value 2 formerly meant "Manual" so we store it that way: */
    tail.stagstyle = tagstyle - (tagstyle >= 2);
    tail.stagstyleextra = tagstyle - tail.stagstyle;
    tail.stagleadin = tagleadin;
    tail.slastscrheight = scr->Height;
    tail.scustomfourcolor = custom4color;
    tail.scustomautoscroll = customautoscroll;
    tail.scustomshowprops = customshowprops;
    tail.snostrchain = !strchain;
    tail.snoaskdelete = !askdelete;
    tail.sshowsizes = showsizes;
    tail.sflushreplies = flushreplies;
    tail.seditnostrip = !editstrip;
    tail.magicality = MagicIdentifierNumber();
    tail.sregnum = reg_number;
    tail.snopopupscroller = !popupscroller;
    SaveWindowCoords(&tail);
    if (!failage && BWrite(shand, &tail, sizeof(tail), 1) < 1)
	failage = true;
    memset(&tail, 0, ENDPAD);
    if (!failage && BWrite(shand, &tail, 1, ENDPAD) < ENDPAD)
	failage = true;
    if (!BClose(shand))
	failage = true;
    if (failage) {
	Err("Could not write data to\nconfiguration file %s", configfilename);
	DeleteFile(FIGFILE);
    }
}


bool ReportFigError(short rcer)		/* true if no error */
{
    str using = (!(rcer & 3) ? "file " FIGFILE " instead" : "settings");

    switch (rcer) {
      /* case 0: do nothing;    case 5: case 9:  can't happen */
      case 1:
	Err("Couldn't load configuration -- no memory.", null, null);
	break;
      case 2:
	Err("Configuration file %s\ncontained invalid data.  "
			    "Using %s settings.", userfigfile,
			    (onefigloaded ? "existing" : "default"));
	break;
      case 3: case 8: case 11:
	Err("Could not open configuration file\n\"%s\".  Using "
			    "default\n%s.", userfigfile, using);
	break;
      case 4: case 7:
	Err("Configuration file %s\ncontained invalid data.  "
			    "Using default\n%s.", userfigfile, using);
	break;
      case 6:
	Err("Neither configuration file %s\nnor default file "
			    FIGFILE " contained\nvalid data.  "
			    "Using default settings.", userfigfile);
	break;
      case 10:
	Err("Could not open configuration file \"%s\",\nand default file "
			    FIGFILE " contained\ninvalid data.  "
			    "Using default settings.", userfigfile);
	break;
    }
    if (!workdir[0])
	tryopename[0] = 0, autopenlist = false;
    return !rcer;
}


void FiddleFigFile(bool saving)
{
    BPTR hand;
    long cookie;
    bool clobber = false, oldgagrow = gagrow;

    if (!DoFileRequest(saving, true, configfilename,
				configfilename[0] ? configfilename : FIGFILE,
				null, saving ? "Select file to save setup in"
				: "Select file to load setup from"))
	return;
    if (saving) {
	if (hand = OOpen(configfilename)) {
	    if (Read(hand, (adr) &cookie, 4) == 4 && cookie != COOKIE)
		clobber = true;
	    Close(hand);
	}
	if (clobber && !AskSaveFigOverwrite(configfilename))
	    return;
	SaveConfig();
    } else {
	strcpy(userfigfile, configfilename);
	if (ReportFigError(ReadConfig())) {
	    if (areaz.messct) {
		NewSort(sorder);
		if (!waste)
		    MakeLotsaRoom();
	    }
	    MakeInstanceNames(ourinstance);
	    if (gagrow != oldgagrow)
		MakeBotGadgets();
	}
    }
}


void TryAPopKey(str keyname)
{
    size_t l;
    if (keyname) {
	if (!(l = strlen(keyname)))
	    return;
#ifdef BETA
	pOpkeylen = l;
#endif
	if (l >= POPKEYLEN)
	    l = POPKEYLEN - 1, poptrunc = true;
	strncpy0(popkeyname, keyname, l);
	Stranks(popkeyname);
    }
}


typedef BOOL (*MixFunc)(struct InputEvent *e, IX *x);

#define LVOMatchIX -204


#ifdef C_NOT_ASM

#  pragma regcall(MyMatchIX(a0, a1))

/* this is used ONLY with commodities v37, where there is no real MatchIX */

BOOL MyMatchIX(register struct InputEvent *e, register IX *x)
{
    register ushort qual, syms;
    if (e->ie_Class != x->ix_Class)
	return false;
    if ((e->ie_Code ^ x->ix_Code) & x->ix_CodeMask)
	return false;
    qual = e->ie_Qualifier;
    if (syms = x->ix_QualSame) {
	if (syms & IXSYM_SHIFT && qual & SHIFTKEYS)
	    qual |= SHIFTKEYS;
	if (syms & IXSYM_CAPS && qual & (SHIFTKEYS | IEQUALIFIER_CAPSLOCK))
	    qual |= SHIFTKEYS | IEQUALIFIER_CAPSLOCK;
	if (syms & IXSYM_ALT && qual & ALTKEYS)
	    qual |= ALTKEYS;
    }
    return !((qual ^ x->ix_Qualifier) & x->ix_QualMask);
}


#  pragma intfunc(HotHandler(a0, a1))
/* intfunc saves regs and does geta4() so we can see global variables */

/* NOTE!!  This C version does NOT WORK, because you can't tell it to */
/* pass args in registers to a call of a function pointer variable!   */

local struct InputEvent *HotHandler(struct InputEvent *raw, MixFunc Mix)
{
    register struct InputEvent *e;
    for (e = raw; e; e = e->ie_NextEvent)
	if (Mix(e, &popexpression)) {
	    e->ie_Class = IECLASS_NULL;		   /* remove */
	    Signal((adr) me, SIGBREAKF_CTRL_F);    /* makes us pop to front */
	    break;
	}
    return raw;
}

#else

#  pragma regcall(MyMatchIX(a0, a1))
#  pragma regcall(HotHandler(a0, a1))

local BOOL MyMatchIX(register struct InputEvent *e, register IX *x);
struct InputEvent *HotHandler(struct InputEvent *original, MixFunc Mix);

#  asm
	xdef		_HotHandler
	xref		_geta4
hregz	REG		a0/a2-a6

_MyMatchIX:
	move.b		1(a1),d0		; ix_Class
	cmp.b		4(a0),d0		; ie_Class
	bne.s		mixNo			; different?  fail
	move.w		2(a1),d0		; ix_Code
	move.w		6(a0),d1		; ie_Code
	eor.w		d1,d0			; find differing ie_Code bits
	and.w		4(a1),d0		; use ix_CodeMask to ignore some
	bne.s		mixNo			; any different?  fail
	move.w		8(a0),d0		; ie_Qualifier
	move.w		10(a1),d1		; ix_QualSame
	bne.s		simple			; no synonyms?
	  move.l	d2,a0		; we're done with a0, save d2 in it
	  btst		#0,d1
	  bne.s		noSh			; IXSYM_SHIFT present?
	  move.w	d0,d2
	  and.w		#3,d2
	  beq.s		noSh			; _LSHIFT or _RSHIFT present?
	    or.w	#3,d0			; yes, turn on both
noSh:	  btst		#1,d1
	  bne.s		noCap			; IXSYM_CAPS present?
	  move.w	d0,d2
	  and.w		#7,d2
	  beq.s		noCap			; _LSHIFT, _RSHIFT, or _CAPSLOCK?
	    or.w	#7,d0			; yes, turn on all three
noCap:	  btst		#2,d1
	  bne.s		noAlt			; IXSYM_ALT present?
	  move.w	d0,d2
	  and.w		#$30,d2
	  beq.s		noAlt			; _LALT or _RALT present?
	    or.w	#$30,d0			; yes, turn on both
noAlt:	  move.l	a0,d2
simple:	move.w		6(a1),d1		; ix_Qualifier
	eor.w		d1,d0			; find bits that differ
	and.w		8(a1),d0		; ix_QualMask ignore mask
	bne.s		mixNo			; any mismatch?
	moveq		#1,d0			; if not, succeed
	rts
mixNo:	moveq		#0,d0
	rts
; Yes, I did check this for correctness against a disassembly of the ROM
; code (V38).  It is used only with commodities.library V37.


_HotHandler:
	movem.l		hregz,-(sp)
	FAR	CODE
	jsr		_geta4
#  endasm
#  ifndef _LARGE_CODE
#    asm
	NEAR	CODE
#    endasm
#  endif
#  asm
	move.l		a1,a2		; test function pointer
	lea		_popexpression,a3
	move.l		_CxBase,a6	; MatchIX may expect it someday
	move.l		a0,a5		; inputevent pointer
foyer:	move.l		a5,d0		; fake tst.l; out of inputevents yet?
	beq.s		donne
	  move.l	a5,a0
	  move.l	a3,a1
	  jsr		(a2)		; Mix(event, &popexpression)
	  tst.w		d0
	  beq.s		neven
	    clr.b	4(a5)		; event->ie_Class = IECLASS_NULL
	    move.l	_SysBase,a6
	    move.l	_me,a1
	    moveq	#0,d0
	    bset	#15,d0
	    jsr		-324(a6)	; Signal(me, SIGBREAKF_CTRL_F)
	    bra.s	donne
neven:	  move.l	(a5),a5		; event = event->ie_NextEvent
	  bra.s		foyer
donne:	movem.l		(sp)+,hregz
	move.l		a0,d0		; return original InputEvent pointer
	rts

	public		_CxBase,_SysBase
#  endasm
#endif C_NOT_ASM


void SetDownPopKey(void)
{
    if (hotinstalled) {
	hotio.io_Data = (adr) &hotrupt;		/* just in case */
	hotio.io_Command = IND_REMHANDLER;
	hotio.io_Message.mn_Node.ln_Type = NT_MESSAGE;
	DoIO((adr) &hotio);
	Delay(12);		/* at least one event must happen, right? */
	hotinstalled = false;
    }
    if (hotio.io_Device) {
	CloseDevice((adr) &hotio);
	if (hotio.io_Message.mn_ReplyPort)
	    DeleteMsgPort(hotio.io_Message.mn_ReplyPort);
	hotio.io_Message.mn_ReplyPort = (adr) hotio.io_Device = null;
    }
    if (CxBase) {
	CloseLibrary(CxBase);
	CxBase = null;
    }
}


void SetUpPopKey(void)
{
    bool deverror = false;
    long perr;

    if (!popkeyname[0])
	return;
    if (!(CxBase = OpenL("commodities"))) {
	Err("Can't use popkey;\nno commodities.library");
	return;
    }
    if (poptrunc || (perr = ParseIX(popkeyname, &popexpression))) {
#ifdef BETA
	if (poptrunc)
	    Err("Popkey description too long; truncated to\n\"%s\"\n"
			"original length %ld; flag value 0x%04lx",
			popkeyname, (long) pOpkeylen, (ulong) poptrunc);
	else
	    Err("Invalid popkey description \"%s\"\n"
			"error value %ld; CxBase = 0x%lx, version %ld\n"
			"IX is at 0x%lx, contains %08lx %08lx %08lx",
			popkeyname, perr, CxBase, (long) CxBase->lib_Version,
			&popexpression, ((long *) &popexpression)[0],
			((long *) &popexpression)[1],
			((long *) &popexpression)[2]);
#else
	Err(poptrunc ? "Popkey description too long" :
			"Invalid popkey description:\n%s", popkeyname);
#endif
	goto bomb;
    }
/* for something this simple, input handler is less trouble than a commodity */
    hotio.io_Device = null;
    if (!(deverror = !(hotio.io_Message.mn_ReplyPort = CreateMsgPort())
			|| OpenDevice("input.device", 0, (adr) &hotio, 0))) {
	hotrupt.is_Code = HotHandler;
	if (CxBase->lib_Version >= 38)
	    hotrupt.is_Data = (adr) ((long) CxBase + LVOMatchIX);
	else
	    hotrupt.is_Data = (adr) &MyMatchIX;
	hotrupt.is_Node.ln_Pri = 52;
	hotrupt.is_Node.ln_Name = qsemname;
	hotio.io_Command = IND_ADDHANDLER;
	hotio.io_Data = (adr) &hotrupt;
	hotio.io_Message.mn_Node.ln_Type = NT_MESSAGE;
	deverror = DoIO((adr) &hotio);
    }
    if (deverror) {
	Err("Could not install popkey\nin input stream.");
/*	hotio.io_Device = null;	*/
	goto bomb;
    } else
	hotinstalled = true;
    return;
  bomb:
    SetDownPopKey();
}


local BPTR DupPath(BPTR oldpath)
{
    BPTR *wext, *noop, newpath = 0, *lastnoop = &newpath;
    APTR foo = me->pr_WindowPtr;

    me->pr_WindowPtr = (APTR) -1;
    for (wext = gbip(oldpath); wext; wext = gbip(*wext)) {
	if (!(noop = AllocMem(12, MEMF_PUBLIC)))
	    break;
	noop[0] = 12;
	if (noop[2] = DupLock(wext[1])) {
	    noop[1] = 0;
	    *lastnoop = (long) (noop + 1) >> 2;
	    lastnoop = noop + 1;
	} else {
	    FreeMem(noop, 12);
	    break;
	}
    }
    me->pr_WindowPtr = foo;
    return newpath;
}


void AddPath(struct Process *parent)
{
    SCLI *parcli = gbip(parent->pr_CLI);
    short tx = 0;
    str tale;
    static str trials[] = {
	"Workbench", "AmigaShell", "Initial CLI", "New CLI", null
    };
    BPTR path = 0;

    path = parcli ? DupPath(parcli->cli_CommandDir) : 0;
    while (!path) {
	tale = trials[tx++];
	if (!tale)
	    return;
	parent = (adr) FindTask(tale);
	if (parent && (parcli = gbip(parent->pr_CLI)))
	    path = DupPath(parcli->cli_CommandDir);
    }
    fakepath = path;
}


void NukePath(void)
{
    BPTR *pp, *npp;

    if (fakepath)  {
	for (pp = gbip(fakepath); pp; pp = npp) {
	    UnLock(pp[1]);
	    npp = gbip(pp[0]);
	    FreeMem(pp - 1, 12);
	}
	fakepath = 0;
    }
}


long ShellExecute(str command, BPTR in, BPTR out)
{
#ifdef TEST13
    Execute(command, 0L, out);
    return 0;
#else
    return SystemTags(command, SYS_UserShell, 1L, NP_Path, DupPath(fakepath),
			SYS_Input, in, SYS_Output, out, TAG_DONE);
#endif
}
/* The fucking spawned shell frees the path itself -- we have to */
/* make a new valid, DOS-freeable copy every time we pass it in! */
