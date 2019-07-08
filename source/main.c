/* Main program for Q-Blue.  By Paul Kienitz.  Begun 28 Jan 1992. */

#include <exec/types.h>
#include <exec/memory.h>
#include <dos/dos.h>
#include <dos/dostags.h>
#include <intuition/sghooks.h>
#include <intuition/intuition.h>
#include <workbench/workbench.h>
#include <workbench/startup.h>
#include <stdlib.h>
#include "qblue.h"


#ifndef ALPHA
#  define DETACH
#endif

#define FILESLIMIT     400
#define FIGLEN         129
#define TRYOPENLEN     290


typedef struct CommandLineInterface SCLI;


void Aboutness(void);
void VersionError(void);
short AskAttemptUnpacked(void);
bool AskAttemptReplies(short newer);
bool AskReopen(short newer);
bool AskReopen3(short newer, bool *reopen);
bool AskAboutUsedDirs(bool replies, bool looksokay, bool ask);
void InitUserNameNum(void);
void WorkbenchRequesterArgs(str body, str options, ...);
void ConfigHints(void);
adr MakeReplySpace(void);
void FreeReplySpace(adr replymesses);
bool FreeTaglines(void);

void LoadTwitList(void), FlushTwitList(void);
void FreePool(adr where);
void FreeReply(struct Mess *mm);
bool OpenDisplay(void);
bool LoadPacket(void);
void MakeBoxTop(void);
void ViewMessages(ushort initialarea, bool listem);
void CloseDisplay(void), FlushPacket(void);
short ReadConfig(void);
void Iconify(bool instant);
void DoSetupIDCMP(char k);
short ListFiles(short start);
void ConfigDirs(void);

ulong DS2ux(struct DateStamp *d);
bool Wavy(str p);
void FlushConf(struct Conf *cc);
void DateString(struct DateStamp *d, str s);
void Resort(bool movement);
void ReloadReplies(bool nounpack);
void EmptyDir(str path, bool quiet);
bool DoPack(str fullname, bool un, bool rep);
bool DoFileRequest(bool saving, bool special, str result,
				str deflt, str pattern, str hail);
void JoinName(str result, str dir, str file, str tail);
void SetMessGags(bool on);

bool ConnectToSemaphore(void);
void DisconnectFromSemaphore(void);
void OkayWithEverybodyToOpen(void);
void TellEverybodyWeClosed(void);
void RestoreBookmarks(void);
bool SwitchToLocalPacker(void);
bool IdenticalPaths(str a, str b);

void SetUpPopKey(void);
void SetDownPopKey(void);
void TryAPopKey(str keyname);
bool ReportFigError(short rcer);
void NukePath(void);
void AddPath(struct Process *parent);
void SeedRandom(void);
void AdjustBGslider(struct Mess *vis, bool force);
void StopTitleBarClock(void);


import const ulong toksize;

import struct RastPort bgrpblack;
import struct IntuiText bgt1asl, bgt2s, bgt2i, bgt1q, bgt4, bgt4del;

import adr stringPermPool, tok;		/* types not exported */
import struct trash *trashfiles;
import ustr *templines, templinetypes;
import str xcommand, functionkeys[20];

import char bbsname[], sysopname[], edit1command[];
import char title[], opendir[], fakendir[], rawtitle[], fakeryname[];
import ushort penzez[], *WaitPointer;

import ushort textop, nomwidth;
import BPTR tryunpackcd, fakepath;
import bool realrequester, lookslikeqwk, replylike, qwkreplike;
import bool datesverted, packed_once, zerosizefile, abbandonement;
import ulong temptime;
import short ourinstance;


ushort whichtitle = 0;		/* regular, personal, replies, bullstuff */
char screentitle[4][80] = { "", "", "", "" };
char tempacketname[31], packetname[32], fakerypath[256], tryomesh[258] = "";
char darchivename[290];
/* const */ char defigfile[] = FIGFILE;

BPTR replylock = 0;
ulong downloaddate, uploaddate;
long endtag = TAG_DONE;			/* universal empty taglist */
short reloadflag;

bool oldreplies, quittitude, alreadyready, surepacketname = false;
bool qwk = false, onefigloaded = false, useopenfakery;
bool onlydirsinthere, dos3, fakery = false;
bool needtoemptywork, needtoemptyreply, dirwasfile, stealthiness;
local bool dopackerswitch, semaphored = false;

/****  struct Mess *(filezlist[FILESLIMIT]);  ****/

struct Conf filez = {
    /* &filezlist[0] */ null, null, "", 0, 0, 0, 0, 0, 0, "#$%@!", "", 0
};

adr IconBase, SysBase;

BPTR originalcd;
struct FileInfoBlock *fib;
struct Process *me;

/* ==== The following crap all gets used both before and after detaching, */
/* so we have to make sure it does not get ZEROED when the background     */
/* process starts up and clears BSS... so it all has to be initialized.   */

bool autopenlist = true;
char userfigfile[FIGLEN] = "\0?", configfilename[FIGLEN] = "\0?";
char tryopename[TRYOPENLEN] = "\0?";

int _argc = -1, _arg_len = -1;
str _arg_lin = (adr) -1, *_argv = (adr) -1;

#ifdef DETACH
BPTR *detseg = (adr) -1;
struct MemList *mem = (adr) -1;
long memsz = -1;
#endif
bool detested = true;		/* false if we have detached */

/* ==== End of crap used both before and after detaching.  Initializing with */
/* zero actually works, but I wanted to make extra-sure it goes in non-BSS.  */


/* The following is stuff that's stuck in main.c just because we can't make  */
/* separate alpha/beta/try versions in the other source files where they     */
/* would naturally fit in...                                                 */

#ifdef TRY

void SetRegistration(str registration)  { }

#else

void SetRegistration(str registration)        /* from code by George Hatchew */
{
    long rnum = 1;
    char lowname[40];
    str from = myloginame;
    short i, l = strlen(from);

    for (i = 0; i < l; i++)
	rnum += (lowname[i] = tolower(from[i])) + 696;
    lowname[l] = 0;
    if (l > 5)
	rnum += 17 * lowname[4];
    if (l > 16)
	rnum += 13 * lowname[14];
    rnum *= 34;
    if (l > 13)
	rnum *= 59 * lowname[11];
    if (l > 8)
	rnum += 28 * lowname[6];
    rnum += 3788680;
    if (l > 5)
	rnum *= lowname[3];
    if (l > 10)
	rnum += 7 * lowname[8];
    if (l > 19)
	rnum += 12 * lowname[17];
    if (rnum < 0)
	rnum = -rnum;
    do
	rnum /= 10;
    while (rnum > 99999999);
    if (rnum <= 9999999 && rnum > 999999)
	rnum += 70000000;
    if (rnum <= 999999 && rnum > 99999)
	rnum += 77000000;
    sprintf(registration, "%8ld", rnum);
}

#endif


void DetectStealthiness(ushort lead, bool quiet)
{
#ifdef TRY
    if (lead == 3 && !quiet)
	Err("You have to buy the registered version of Q-Blue\n"
		    "for the \"Stealth\" option to work.  In this version\n"
		    "it simply works the same as the \"...\" tagline option.");
#else
    stealthiness = lead == 3;
#endif
}


#ifdef ALPHA
str Strcpy(str dest, str src)      /* for use in SDB: "e Strcpy(var, str)" */
{
    return strcpy(dest, src);
}
#endif

#ifdef BETA
local char assertformat[] = "ASSERTION FAILED, file %s line %lu!\n\"%s\"";

void _assert(char *what, const char *who, unsigned int where)
{
    if (scr)
	Err(assertformat, who, (ulong) where, what);
    else
	WorkbenchRequesterArgs(assertformat, "Okay", who, (ulong) where, what);
}
#endif

/* ==== End of special alpha/beta/try functions */


local str nerd(register str t)
{
    while (isalnum(*t)) t++;
    while (*t && !isalnum(*t)) t++;
    return t;
}


ulong Text2ux(str date)
{
    long t;
    short year, month, day, hour, minute, second, i;
    static short men[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
    str d1, d2, d3, t1, t2, t3, tp;
    static char mon[12][4] = { "JAN", "FEB", "MAR", "APR", "MAY", "JUN",
			       "JUL", "AUG", "SEP", "OCT", "NOV", "DEC" };
    static char wee[7][4] = { "SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT" };

    d1 = date;
    while (isspace(*d1))
	d1++;
    for (i = 0; i < 7; i++)
	if (!strnicmp(date, wee[i], 3)) {
	    d1 = nerd(date);
	    break;
	}
    d3 = nerd(d2 = nerd(d1));
    if (d2[-1] == ':') {
	t3 = d3; t2 = d2; t1 = d1;
	d3 = nerd(d2 = nerd(d1 = nerd(t3)));
    } else
	t3 = nerd(t2 = nerd(t1 = nerd(d3)));
    tp = t3;
    while (isdigit(*tp)) tp++;
    while (*tp && !isalpha(*tp)) tp++;
    second = atol(t3);
    minute = atol(t2);
    hour = atol(t1) + (tolower(*tp) == 'p' ? 12 : 0);
    if (isalpha(*d2) && isdigit(*d1))
	tp = d1, d1 = d2, d2 = tp;
    year = atol(d3);
    day = atol(d2) - 1;
    if (isalpha(*d1)) {
	month = 0;
	for (i = 0; i < 12; i++)
	    if (!strnicmp(d1, mon[i], 3)) {
		month = i;
		break;
	    }
    } else
	month = atol(d1) - 1;
    if (year < 72) year += 100;		/* start from a leap year */
    year -= 72;
    year %= 100;
    t = (year >> 2) * 1461 + day;
    if (year &= 3) {
	t += year * 365 + 1;
	men[1] = 28;
    } else
	men[1] = 29;
    for (i = 0; i < month; i++)
	t += men[i];
    t += 730;				/* re-adjust back to 1970 */
    return t * (60 * 60 * 24) + hour * (60 * 60L) + minute * 60 + second;
}


int datesort(struct Mess **a, struct Mess **b)
{
    return (*b)->unixdate - (*a)->unixdate;
}


local bool Scanem(BPTR dir, bool downz, bool upz)
{
    struct Mess *mm;
    str p;
    short i, j;
    bool wavular;
    struct IntuiMessage *im;
    long r;

    if (!dir)
	return false;
    if (!(r = Examine(dir, fib)) || fib->fib_DirEntryType < 0) {
	if (r)
	    dirwasfile = true;
	return false;
    }
    if (fakery)
	downz = false;
    while (ExNext(dir, fib)) {
	mm = null;
	if (!tryomesh[0])
	    while (im = (adr) GetMsg(idcort)) {
		char k = (im->Class == IDCMP_RAWKEY
			    ? KeyToAscii(im->Code, im->Qualifier) : 0);
		short gid = (im->Class == IDCMP_GADGETUP
			    ? ((struct Gadget *) im->IAddress)->GadgetID : 0);
		ReplyMsg((adr) im);
		if (k == 'A' || gid == 1) {
		    realrequester = true;
		    return false;
		} else if (k == ESC) {
		    wasescaped = true;
		    return false;
		}
	    }
	if (filez.messct >= FILESLIMIT)
	    break;		/* silent */
	if (!NEWZ(mm) || !(mm->from = BValloc(strlen(fib->fib_FileName) + 1))) {
	    if (mm)
		FREE(mm);
	    Err("Cannot display all files in dir;\nout of memory.");
	    break;
	}
	strcpy(mm->from, fib->fib_FileName);
	filez.messes[filez.messct++] = mm;
	if (!fakery && (i = strlen(fib->fib_Comment)))
	    if (mm->subject = BValloc(i + 1))
		strcpy(mm->subject, fib->fib_Comment);
	mm->unixdate = DS2ux(&fib->fib_Date);
	DateString(&fib->fib_Date, mm->date);
	if (p = strrchr(mm->date, ':'))		/* presently cannot fail */
	    *p = '\0';				/* truncate seconds */
	mm->datflen = fib->fib_Size;
	if (tryomesh[0]) {
	    if (MatchPatternNoCase(tryomesh, mm->from)) {
		mm->bits |= TOME;
		filez.tomect++;
		filez.current = filez.messct - 1;
		if ((p = strchr(mm->from, '.')) && (!stricmp(p, ".REP")
					|| !stricmp(p, ".NEW")))
		    mm->bits |= FROMME;
	    }
	} else if ((p = strchr(mm->from, '.')) && !strchr(mm->from, ' ')) {
	    mm->ixinbase = i = p - &mm->from[0];
	    wavular = Wavy(p + 1);
	    if (!i || i > 8 || (j = strlen(p)) < 4 || isalpha(p[4]))
		continue;
	    if (fakery && !upz) {
		if (!stricmp(p, ".BBS-QWK"))
		    mm->bits |= TOME | LOADED;
		else if (!stricmp(p, ".BBS-BW"))
		    mm->bits |= TOME;
	    } else if (j == 4 || (wavular || !strnicmp(p, ".QWK", 4))) {
		i = (_toupper(p[1]) << 8) | _toupper(p[2]);
		if (upz && i == 'NE' && _toupper(p[3]) == 'W')
		    mm->bits |= FROMME;
		else if (upz && i == 'RE' && _toupper(p[3]) == 'P')
		    mm->bits |= FROMME | LOADED;
		else if (downz && wavular)
		    mm->bits |= TOME;
		else if (downz && _toupper(p[1]) == 'Q')
		    mm->bits |= TOME | LOADED;
	    }
	}
    }
    return filez.messct;
}


local bool IncedMatch(struct Mess *downf, struct Mess *upf)
{
    register short base = downf->ixinbase, end = upf->ixinbase;
    register str wrong = downf->from;

    if (!(downf->bits & LOADED) || end > base || base - end > 2)
	return false;
    while (base > end)
	if (!isdigit(wrong[--base]))
	    return false;
    while (base > 6 && isdigit(wrong[base - 1]))
	base--;
    return !strnicmp(wrong, upf->from, base);
}
/* true for SOMENA23 = SOMENAME, FOO23 = FOO, but not AAAAAAAB = AAAAAAAA */


bool LikeWhichPacketEh(void)
{
    str oddir, dbdir;
    BPTR dlcd, ulcd;
    bool suck = false, florf = false, botted = true;
    bool samedirs;
    struct Mess *mm, *mmm;
    char packetn[32];
    short i, j;
    ushort ogm;
    size_t k;
    /* files as Messes: name in from[], date in both unixdate and date[],     */
    /* size in datflen, TOME means its name looks packet-like, FROMME means   */
    /* it looks like a reply packet, and LOADED means it's not Blue Wave.  At */
    /* this point, we use MESEEN to mean that a reply packet exists for this  */
    /* download packet, MEREPLIED to mean that the reply packet is newer than */
    /* the download packet, and SEENINBASE to mean it is not a guessed link.  */
    /* datfseek is used for unixdate of corresponding reply packet.  The      */
    /* subject[] field is used to store the filenote if any.                  */

    if (fakery)
	dbdir = bbsesdir, oddir = fakendir;
    else
	dbdir = downloaddir, oddir = opendir;
    dlcd = RLock(dbdir);
    ulcd = RLock(uploaddir);
    samedirs = SameLock(dlcd, ulcd) == LOCK_SAME && !IoErr();
    filez.messct = filez.tomect = 0;
    useopenfakery = wasescaped = dirwasfile = false;
    ChangeBotGagText(&readgag1, &bgt1asl, false);
    ChangeBotGagText(&readgag2, &bgt2s, false);
    ChangeBotGagText(&readgag4, &bgt4del, false);
    ogm = FlipBGadgets(2);
    if (!dlcd || !Scanem(dlcd, true, samedirs))
	goto aargh;
    if (!samedirs && ulcd && !Scanem(ulcd, false, true)) {
	florf = true;
	goto aargh;
    }
    qsort(filez.messes, (size_t) filez.messct, 4, datesort);
    for (i = 0; i < filez.messct; i++) {
	mm = filez.messes[i];
	if (mm->bits & TOME) {		/* VVV include "." in packetn */
	    strncpy0(packetn, mm->from, (k = mm->ixinbase + 1));
	    for (j = 0; j < filez.messct; j++) {
		mmm = filez.messes[j];
		if (i != j && (mmm->bits & LOADED) == (mm->bits & LOADED)
							&& mmm->bits & FROMME)
		    if (!strnicmp(mmm->from, packetn, k)) {
			mm->bits |= MESEEN | SEENINBASE | MEREPLIED;
			if (j > i) mm->bits &= ~MEREPLIED;
			mm->datfseek = mmm->unixdate;
			break;
		    } else if (!(mm->bits & SEENINBASE) && IncedMatch(mm, mmm))
			mm->datfseek = mmm->unixdate,
			mm->bits |= (j > i ? MESEEN : MESEEN | MEREPLIED);
	    }
	}
    }
    j = 0;
    for (i = 0; i < filez.messct; i++) {
	mm = filez.messes[i];
	if (mm->bits & TOME)
	    filez.messes[j++] = filez.messes[i];
	else
	    FreeReply(mm);
    }
    if (!(filez.messct = j))
	goto aargh;
    i = ListFiles(-1);
    ChangeBotGagText(&readgag1, &bgt1q, false);
    ChangeBotGagText(&readgag2, &bgt2i, false);
    ChangeBotGagText(&readgag4, &bgt4, false);
    FlipBGadgets(ogm);
    botted = false;
    UnLock(dlcd);
    if (ulcd)
	UnLock(ulcd);
    dlcd = ulcd = 0;
    if (!(suck = i >= 0) || realrequester || wasescaped)
	goto aargh;
    if (needtoemptywork)
	EmptyDir(workdirinst, true);
    if (needtoemptyreply)
	EmptyDir(replydirinst, true);
    strncpy0(packetname, filez.messes[i]->from, filez.messes[i]->ixinbase);
    strupr(packetname);
    qwk = !!(filez.messes[i]->bits & LOADED);
    downloaddate = filez.messes[i]->unixdate;
    uploaddate = filez.messes[i]->datfseek;
    JoinName(darchivename, dbdir, filez.messes[i]->from, null);
    if (fakery) {
	strcpy(fakeryname, filez.messes[i]->from);
	suck = true;
    } else
	suck = DoPack(darchivename, true, false);
    oldreplies = suck && (filez.messes[i]->bits & MESEEN);
    reloadflag = !!(filez.messes[i]->bits & MEREPLIED);
    wasescaped = !suck;
  aargh:
    if (botted) {
	ChangeBotGagText(&readgag1, &bgt1q, false);
	ChangeBotGagText(&readgag2, &bgt2i, false);
	ChangeBotGagText(&readgag4, &bgt4, false);
	FlipBGadgets(ogm);
    }
    if (dlcd)
	UnLock(dlcd);
    if (ulcd)
	UnLock(ulcd);
    if (dirwasfile) {
	Err("The %s directory you have specified,\n%s,\nis not a directory,"
			"it's a file.", dirname[florf ? DUP : (fakery ? DBBS
				: DDOWN)], florf ? uploaddir : dbdir);
    }
    while ((signed short) filez.messct > 0) {
	mm = filez.messes[--filez.messct];
	FreeReply(mm);
    }
    filez.tomect = 0;
    if (suck | wasescaped)
	return suck;
    if (!oddir[0])
	strcpy(oddir, dbdir);
    return DoFileRequest(false, false, darchivename, oddir, !fakery ? null :
					"(#?.BBS-(QWK|BW)|#?.INF|CONTROL.DAT)",
					"Select a packet to open");
}


bool IsDirEmpty(str path)
{
    BPTR lok = RLock(path);

    lookslikeqwk = replylike = false;
    if (lok) {
	bool ret = !FileSize(lok) && !zerosizefile;
	UnLock(lok);
	return ret;
    }
    return true;
}


local bool CheckUpAgain(void)
{
    char tname[32];
    BPTR ocd, tcd;
    ulong au = 0;
    bool ret = false;

    if (tcd = RLock(uploaddir)) {
	ocd = CurrentDir(tcd);
	strcpy(tname, packetname);
	strcat(tname, (qwk ? ".REP" : ".NEW"));
	if (tcd = RLock(tname)) {
	    if (Examine(tcd, fib)) {
		uploaddate = DS2ux(&fib->fib_Date);
		reloadflag = uploaddate > downloaddate;
	    } else
		reloadflag = 1;
	    UnLock(tcd);
	    ret = true;
	}
	UnLock(CurrentDir(ocd));
    }
/*  if (fakery)
	reloadflag = 0;  */
    return ret;
}


BPTR CreateLock(str dirname)
{
    register BPTR b = RLock(dirname);
    
    if (!b && IoErr() != ERROR_DEVICE_NOT_MOUNTED && (b = CreateDir(dirname))) {
	if (!ChangeMode(CHANGE_LOCK, b, ACCESS_READ)) {
	    UnLock(b);		/* brute force ChangeMode */
	    b = RLock(dirname);
	}
    }
    return b;
}


local void MaybeEmptyDir(str which, bool trying)
{
    if (trying)
	EmptyDir(which, true);
    else if (which == replydirinst)
	needtoemptyreply = true;
    else
	needtoemptywork = true;
}


local bool AlreadyUnpacked(bool trying)
{
    short r;
    if (replydirinst[0] && !IsDirEmpty(replydirinst)
				&& !replylike && !onlydirsinthere) {
	if (AskAboutUsedDirs(true, false, true))
	    MaybeEmptyDir(replydirinst, trying);
	else {
	    ConfigDirs();
	    alreadyready = false;
	    return true;
	}
    }
    if (fakery)
	dopackerswitch = true;
    else if (!IsDirEmpty(workdirinst) && !onlydirsinthere) {
	if (!lookslikeqwk && !tempacketname[0]) {
	    if (AskAboutUsedDirs(false, false, true)) {
		MaybeEmptyDir(workdirinst, trying);
		return alreadyready = false;
	    } else {
		ConfigDirs();
		alreadyready = false;
		return true;
	    }
	} else if (trying)
	    MaybeEmptyDir(workdirinst, true);
	else if ((r = AskAttemptUnpacked()) > 0) {
	    downloaddate = temptime;
	    if (!(qwk = !tempacketname[0]))
		strcpy(packetname, tempacketname);
	    return alreadyready = dopackerswitch = true;
	} else if (r < 0) {
	    alreadyready = false;	/* user selected Cancel */
	    return true;
	} else {
	    downloaddate = temptime;
	    MaybeEmptyDir(workdirinst, false);
	}
    }
    return alreadyready = false;
}


local bool PutSomethingInWorkDir(void)
{
    BPTR tol = 0;
    char tryopat[128];
    struct Mess *mm;
    ulong dt;
    ushort i;
    str p;

    abbandonement = false;
    if (tryopename[0]) {
	if (strchr(tryopename, ':')) {
	    useopenfakery = fakery;
	    strcpy(darchivename, tryopename);
	} else
	    JoinName(darchivename, fakery ? bbsesdir : downloaddir,
					tryopename, null);
	alreadyready = false;
	if (tol = RLock(darchivename)) {
	    if (Examine(tol, fib))
		downloaddate = DS2ux(&fib->fib_Date);
	    UnLock(tol);
	    qwk = true;
	    if (p = strchr(tryopename, '.')) {
		if (Wavy(p + 1))
		    qwk = false;
	    }
	} else {				/* PACKET=pattern */
	    p = FilePart(darchivename);
	    if (strlen(p) < 128) {
		strcpy(tryopat, p);
		*p = 0;
		if (useopenfakery)
		    strcpy(fakendir, darchivename);
		if (tol = RLock(darchivename)) {
		    if (ParsePatternNoCase(tryopat, tryomesh, 258) > 0
						&& Scanem(tol, true, false)) {
			UnLock(tol);
			dt = tol = 0;
			tryopat[0] = 0;
			if (filez.tomect == 1)
			    strcpy(tryopat, filez.messes[filez.current]->from);
			else if (filez.tomect)		/* find newest */
			    for (i = 0; i < filez.messct; i++)
				if ((mm = filez.messes[i])->unixdate > dt
							&& mm->bits == TOME) {
				    dt = mm->unixdate;
				    strcpy(tryopat, mm->from);
			}
			if (tryopat[0]) {
			    if (fakery)
				strcpy(fakeryname, tryopat);
			    strcpy(p, tryopat);   /* appends to darchivename */
			    if (tol = RLock(darchivename)) {
				if (Examine(tol, fib))
				    downloaddate = DS2ux(&fib->fib_Date);
				UnLock(tol);
			    }
			}
		    } else {
			UnLock(tol);
			tol = 0;
		    }
		    while ((signed short) filez.messct > 0)
			FreeReply(filez.messes[--filez.messct]);
		    filez.tomect = 0;
		    tryomesh[0] = 0;
		}
	    }
	}
	if (tol) {	/* remember, tol is already unlocked */
	    if (AlreadyUnpacked(true))
		return false;
	    if (fakery)
		return true;
	    else if (DoPack(darchivename, true, false))
		return alreadyready = true;
	}
	if (!abbandonement)
	    Err("Could not find, or could not\nunpack, the %s named\n%s",
				fakery ? "BBS file" : "mail packet", tryopename);
	return autopenlist && LikeWhichPacketEh();
    } else {
	if (AlreadyUnpacked(false))
	    return alreadyready;
	return LikeWhichPacketEh();
    }
}


local bool PickPacketAndOpen(void)
{
    BPTR oldcd, middlecd = 0;
    bool suck = false, nounpack = false, alreadyasked = false, reopen = false;
    bool oldqwk;
    char oldpacketname[32];

    qwk = dopackerswitch = false;
    oldcd = CurrentDir(0);
    repchanges = needtoemptywork = needtoemptyreply = false;
    tempacketname[0] = 0;
    if (!fakery && ((!tryopename[0] && !downloaddir[0]) || !workdir[0])) {
	Err("Please make sure both a download directory\n"
				"and a work directory are defined before\n"
				"attempting to open a message packet.");
	ConfigDirs();
    }
    if (fakery && !tryopename[0] && !bbsesdir[0]) {
	Err("Please make sure a BBS Context\ndirectory is defined before\n"
				"attempting to open with no packet.");
	ConfigDirs();
    }
    OkayWithEverybodyToOpen();
    if ((fakery || ((tryopename[0] || downloaddir[0]) && workdirinst[0]))
					&& PutSomethingInWorkDir()) {
	PortOff();
	middlecd = fakery ? RLock(useopenfakery ? fakendir : bbsesdir)
					: CreateLock(workdirinst);
	if (!middlecd)
	    if (fakery)
		DosErr("Unexpected failure to CD to context\ndirectory %s",
				bbsesdir);
	    else
		DosErr("Could not find or create work\ndirectory"
				" %s\nfor unpacking messages.", workdirinst);
	else {
	    CurrentDir(middlecd);
	    strcpy(oldpacketname, packetname);
	    oldqwk = qwk;
	    tempacketname[0] = 0;
	    if (!fakery)
		LoadTwitList();
	    if (suck = LoadPacket()) {
		FlushTwitList();
		surepacketname = true;
		packed_once = false;
		if (dopackerswitch)
		    SwitchToLocalPacker();
		if (replydirinst[0]) {
		    if (replylock = RLock(replydirinst)) {
			CurrentDir(replylock);
			if (alreadyready || oldqwk != qwk
					  || stricmp(packetname, oldpacketname))
			    oldreplies = CheckUpAgain();
			if (FileSize(replylock) < 0 && replylike
							&& qwk == qwkreplike) {
		      /***  if (fakery)
				reloadflag = 0;
			    else  ***/		      /* vvvvv  12 hours */
			    if (oldreplies && temptime + 43200 < uploaddate)
				reloadflag *= 2;
			    else {
				uploaddate = temptime;
				reloadflag |= uploaddate > downloaddate;
			    }
			    alreadyasked = oldreplies;
			    if (oldreplies)
				nounpack = AskReopen3(reloadflag, &reopen);
			    else
				nounpack = AskAttemptReplies(reloadflag);
			}
			if (!nounpack)
			    EmptyDir("", false);	/* replydirinst */
			else
			    repchanges = true;
			CurrentDir(middlecd);
		    }
		    if (!alreadyasked && oldreplies)
			reopen = AskReopen(reloadflag);
		    if (nounpack | reopen)
			if (replylock || (replylock
					    = CreateLock(replydirinst))) {
			    CurrentDir(replylock);
			    ReloadReplies(nounpack);
			    CurrentDir(middlecd);
			} else
			    DosErr("Could not find or create Replies\n"
						"directory %s", replydirinst);
		} else if (oldreplies)
		    Err("There appears to be a reply packet in your uploads"
					" directory\ncorresponding to the mail"
					" packet you just opened, but it can't"
					"\nbe reloaded because you have no"
					" replies directory defined.");
		Resort(false);
	    } else if (!fakery)
		EmptyDir("", false);	/* flush unpacked garbage */
	    UnLock(CurrentDir(0) /* == middlecd */ );
	    FlushTwitList();		/* in case LoadPacket fails */
	}
	PortOn();
    }
    CurrentDir(oldcd);
    tryopename[0] = 0;
    return suck;
}


void ReadAPacket(bool fakeit)
{
    ushort i, ogm = FlipBGadgets(0);
    bool offed = false;
    char inform[8];
    static str spesh[4] = {
	"", " -- PERSONAL MAIL", " -- YOUR REPLIES", " -- BULLETINS"
    };

    datesverted = false;
    whicha = -1;
    fakery = fakeit;
    darchivename[0] = 0;
    if (PickPacketAndOpen()) {
	if (ourinstance)
	    sprintf(inform, "(%ld) ", (ulong) ourinstance);
	else
	    inform[0] = 0;
	for (i = 0; i < 4; i++)
	    sprintf(screentitle[i], "%s%s:  reading \"%s\"%s",
				inform, rawtitle, packetname, spesh[i]);
	SetWindowTitles(bgwin, null, screentitle[whichtitle = 0]);
	readareaz.current = !!personals.messct + !!bullstuff.messct;
	RestoreBookmarks();	/* ^^ current >= messct is OK */
	ViewMessages(readareaz.current, !fakery || replies.messct);
	/* NOTE:  ViewMessages() does PortOff() at exit! */
	offed = true;
	FlushPacket();
	if (replylock) {
	    UnLock(replylock);
	    replylock = 0;
	}
	surepacketname = false;
	onscreen = null;
	if (!quittitude) {
	    StopTitleBarClock();
	    SetWindowTitles(bgwin, null, title);
	    RectFill(&bgrpblack, 0, (long) textop, nomwidth - 1, texbot - 1);
	    MakeBoxTop();	/* ^^ bgrpblack's APen is always zero */
	    AdjustBGslider(null, true);
	}
	screentitle[0][0] = screentitle[1][0]
			= screentitle[2][0] = screentitle[3][0] = 0;
    }
    TellEverybodyWeClosed();
    FlipBGadgets(ogm);
    if (offed)
	PortOn();
    alreadyready = fakery = false;
}


bool DoOuterIDCMP(struct IntuiMessage *im)
{
    ushort gid;
    char k;

    switch (im->Class) {
      case IDCMP_RAWKEY:
	k = KeyToAscii(im->Code, im->Qualifier);
	if (im->Qualifier & ALTCTLKEYS && !(k & 0x80)) {
	    if (k == '?' || k == '/')
		Aboutness();
	    else
		DoSetupIDCMP(k);
	} else if (k == 'I') {
	    Iconify(false);
	    if (tryopename[0])		/* file dropped on icon */
		ReadAPacket(false);
	} else if (k == 'O' || k == 'N')
	    ReadAPacket(k == 'N');
	else if (k == 'Q')
	    /* no ask really quit when packet not open */
	    /*  ... except if config changes are outstanding??? */
	    return FreeTaglines();
	break;
      case IDCMP_GADGETUP:
	gid = ((struct Gadget *) im->IAddress)->GadgetID;
	if (!gid || gid == 3) {		/* Open / NoPkt */
	    ReadAPacket(gid == 3);
	} else if (gid == 2) {		/* Iconify */
	    Iconify(false);
	    if (tryopename[0])
		ReadAPacket(false);
	} else if (gid == 1)		/* Quit */
	    return FreeTaglines();
	break;
     case IDCMP_MENUPICK:
	if (MENUNUM(im->Code) == 3)
	    DoSetupIDCMP((char) (ITEMNUM(im->Code) + 0x80));
	else if (!MENUNUM(im->Code))
	    switch (ITEMNUM(im->Code)) {
	      case 0: case 1:		/* Open / Open (no packet) */
		ReadAPacket(ITEMNUM(im->Code));
		break;
	      case 4:			/* Compression type */
		DoSetupIDCMP((char) 20 + 0x80);
		break;
	      case 6:			/* Iconify screen */
		Iconify(false);
		if (tryopename[0])
		    ReadAPacket(false);
		break;
	      case 7:			/* About Q-Blue... */
		Aboutness();
		break;
	      case 9:			/* Quit */
		return FreeTaglines();
	    }
	break;
    }
    return quittitude;		/* set inside ReadAPacket */
}


local ustr GeToolType(struct DiskObject *bob, ustr keyword)
{
    str p = FindToolType((ustr *) bob->do_ToolTypes, keyword);
    if (p)
	while (isspace(*p))
	    p++;
    return p;
}


local void TryFig(str filename)
{
    size_t l;
    if (filename) {
	if (!(l = strlen(filename)))
	    return;
	if (l >= FIGLEN)
	    l = FIGLEN - 1;
	strncpy0(configfilename, filename, l);
	Stranks(configfilename);
	strcpy(userfigfile, configfilename);
    }
}


bool NoteWBArgPacket(struct WBArg *wa)
{
    bool baddrop = false;

    if (readareaz.messct || !wa->wa_Lock)
	baddrop = true;
    else {
	BPTR dllock = RLock(downloaddir);
	bool prepend;
	if (dllock) {
	    prepend = SameLock(dllock, wa->wa_Lock) != LOCK_SAME || IoErr();
	    UnLock(dllock);
	} else
	    prepend = true;
	if (prepend) {
	    BPTR ocd = CurrentDir(wa->wa_Lock);
	    if (!(dllock = RLock(wa->wa_Name)))
		baddrop = true;
	    CurrentDir(ocd);
	    if (dllock) {
		if (!NameFromLock(dllock, tryopename, TRYOPENLEN))
		    baddrop = true;
		UnLock(dllock);
	    }
	} else
	    strcpy(tryopename, wa->wa_Name);
    }
    return baddrop;
}


local short DigestArgs(struct WBStartup *wbm)
{
    struct DiskObject *bob;
    struct WBArg *wa;
    int na;
    str ava, figarg = null, oparg = null;
    BPTR oldcd;

    tryopename[0] = configfilename[0] = userfigfile[0] = 0;
    autopenlist = fakery = false;
    if (wbm) {		/* WORKBENCH: */
	if (me->pr_StackSize < 8000) {
	    WorkbenchRequesterArgs("Q-Blue stack size must be\nset in "
					"icon to 8000 bytes", "Okay");
	    return -1;
	}
	if (IconBase = OpenL("icon")) {
	    for (na = 0; na < wbm->sm_NumArgs; na++) {
		wa = &wbm->sm_ArgList[na];
		oldcd = CurrentDir(wa->wa_Lock);
		if (bob = GetDiskObject(wa->wa_Name)) {
		    TryFig(GeToolType(bob, "CONFIG"));
		    if ((ava = GeToolType(bob, "PACKET"))
					|| (ava = GeToolType(bob, "OPEN"))) {
			strcpy(tryopename, ava);
			Stranks(tryopename);
		    }
		    if (((ava = GeToolType(bob, "OPENLIST"))
					|| (ava = GeToolType(bob, "LIST")))
				   && stricmp(ava, "NO") && stricmp(ava, "OFF"))
			autopenlist = true;
		    if ((ava = GeToolType(bob, "NOPACKET"))
				   && stricmp(ava, "NO") && stricmp(ava, "OFF"))
			fakery = true;
		    TryAPopKey(GeToolType(bob, "POPKEY"));
		    FreeDiskObject(bob);
		}
		CurrentDir(oldcd);
		if (configfilename[0] || tryopename[0] || autopenlist || fakery)
		    break;
	    }
	    if (wbm->sm_NumArgs > 1)
		NoteWBArgPacket(&wbm->sm_ArgList[1]);	/* overrides PACKET= */
	    CloseLibrary(IconBase);
	    IconBase = null;
	    _argv = (adr) _arg_lin = null;
	}
    } else {		/* CLI: */
	long rez[5];	/* a.k.a. Vikki */
	struct RDArgs *ra;
	rez[0] = rez[1] = rez[2] = rez[3] = rez[4] = 0;
#ifndef TEST13
	if (ra = ReadArgs("PACKET,CONFIG/K,POPKEY/K,NOPACKET/S,LIST=OPENLIST/S",
					rez, null)) {
	    if (SetSignal(0, SIGBREAKF_CTRL_C) & SIGBREAKF_CTRL_C) {
		PutStr(" *** BREAK\n");
		return -1;
	    }
	    if (rez[0])
		strcpy(tryopename, (str) rez[0]);
	    if (rez[1])
		TryFig((str) rez[1]);
	    TryAPopKey((str) rez[2]);
	    if (rez[3])
		fakery = true;
	    if (rez[4])
		autopenlist = true;
	    FreeArgs(ra);
	} else {
	    PrintFault(IoErr(), "Q-Blue");
	    return -1;
	}
#else
	ra = null;
#endif
    }
    return 0;
}


#ifdef DETACH

local long WellDetachItAlready(void)
{
    register short c;
    SCLI *cli;
    static char qname[] = "Q-Blue";
# ifdef CREATENEWPROC			/* FUCK this shit!! */
    static struct {
	long t0;
	BPTR tseg;
	long t1;
	long tpri;
	long t1a; BPTR tpath;
	long t2;
	str n2;
	long t3;
	str n3;
	long t4, v4, t5, v5, t6, v6, t7, v7, t8, v8, t9, v9, t10, v10;
	long t11, v11, t12, v12, t13, v13, t14, v14, t15, v15, tend;
    } CNPtags = {
	NP_Seglist, 0,		NP_Priority, 0,		NP_Path, 0,
	NP_CommandName, qname,	NP_Name, qname,		NP_FreeSeglist, 1,
	NP_StackSize, 8000,	NP_CopyVars, 0,		NP_Cli, 0, 	
	NP_Input, 0,		NP_Output, 0,		NP_Error, 0,
	NP_CloseInput, 0,	NP_CloseOutput, 0,	NP_CloseError, 0,
	NP_CurrentDir, 0,	NP_ConsoleTask, 0,	TAG_DONE
    };
# else
    register BPTR *ss;
# endif

    if (me->pr_CLI && detested) {		/* do this to get detached */
	/* in other circumstances, we would DupLock the current directory */
	detested = false;
	cli = gbip(me->pr_CLI);
	detseg = gbip(cli->cli_Module);
# ifndef CREATENEWPROC
	for (c = 0, ss = detseg; ss; c++, ss = gbip(*ss)) ;
	memsz = sizeof(struct MemList) + (c - 1) * sizeof(struct MemEntry);
	if (!(mem = AllocP(memsz)))
	    return 20;
	mem->ml_NumEntries = c;
	mem->ml_Node.ln_Name = null;
	mem->ml_Node.ln_Pri = 0;
	mem->ml_Node.ln_Type = NT_MEMORY;
# endif
	if ((c = me->pr_Task.tc_Node.ln_Pri) > 5)
	    c = 5;
# ifdef CREATENEWPROC
	CNPtags.tseg = cli->cli_Module;
	CNPtags.tpri = c;
	/* CNPtags.tpath = fakepath; */
	if (CreateNewProc((adr) &CNPtags))
	    cli->cli_Module = 0;
	else
	    return 20;
# else
	if (CreateProc(qname, (long) c, cli->cli_Module, 8000))
	    cli->cli_Module = 0;
	else {
	    FreeMem(mem, memsz);
	    return 20;
	}
# endif
	return -1;
    } else if (!detested) {		/* do this to survive detachment */
# ifndef CREATENEWPROC
	for (c = 0, ss = detseg; ss; ss = gbip(*ss)) {
	    mem->ml_me[c].me_Addr = (adr) (ss - 1);
	    mem->ml_me[c++].me_Length = ss[-1];
	}
	AddTail((struct List *) &me->pr_Task.tc_MemEntry, &mem->ml_Node);
# endif
	me->pr_ConsoleTask = null;
    }					/* else WB launch -- do nothing */
    return 0;
}

#endif


void StartupCleanup(struct WBStartup *wbm)
{
    if (trashfiles)
	NFREE(trashfiles, TRASHLIMIT);
    if (personals.messes)
	NFREE(personals.messes, PERSONALIMIT + 1);
    if (filez.messes)
	NFREE(filez.messes, FILESLIMIT);
    if (tok)
	FreeMem(tok, toksize);
    if (templines)
	NFREE(templines, BASICLINELIMIT + 1);
    if (templinetypes)
	NFREE(templinetypes, BASICLINELIMIT + 1);
    if (xcommand)
	NFREE(xcommand, XCOMMANDLEN + 3);
    if (functionkeys[0])
	FreeMem(functionkeys[0], 20 * COMMANDLEN);
    FreeReplySpace(replies.messes);
    NukePath();
    if (semaphored)
	DisconnectFromSemaphore();
    if (fib)
	FREE(fib);
    if (wbm) {
	Forbid();
	ReplyMsg((struct Message *) wbm);
    }
}


bool AllocBigBSSthings(void)
{
    short i;
    if (!(replies.messes = MakeReplySpace()))
	return false;
    if (!NNEW(personals.messes, PERSONALIMIT + 1))
	return false;       /* why + 1 here? ^^^  I forget.  paranoia? */
    if (!NNEW(filez.messes, FILESLIMIT))
	return false;
    if (!NNEWZ(trashfiles, TRASHLIMIT))
	return false;
    if (!NNEW(templines, BASICLINELIMIT + 1))
	return false;
    if (!NNEW(templinetypes, BASICLINELIMIT + 1))
	return false;
    if (!(tok = Alloc(toksize)))	/* the biggest alloc; a bit over 6K */
	return false;
    if (!NNEW(xcommand, XCOMMANDLEN + 3))
	return false;
    if (!(functionkeys[0] = AllocZ(20 * COMMANDLEN)))
	return false;
    for (i = 1; i < 20; i++)
	functionkeys[i] = functionkeys[i - 1] + COMMANDLEN;
    return true;
}


long _main(long alen, str aptr)
{
    struct WBStartup *wbm = null;
    static struct Process *originalme = (adr) -1;
    short rcer, na;
#ifdef DETACH
    long ret;
#endif

    me = ThisProcess();
    if (originalme == (adr) -1)
	originalme = me;
#ifndef TEST13
    if (((struct Library *) SysBase)->lib_Version < 37) {
	VersionError();
	if (!me->pr_CLI) {
	    WaitPort(&me->pr_MsgPort);
	    wbm = (struct WBStartup *) GetMsg(&me->pr_MsgPort);
	    Forbid();
	    ReplyMsg((struct Message *) wbm);
	}
	return 20;
    }
#endif
    dos3 = ((struct Library *) SysBase)->lib_Version >= 39;
    if (me->pr_CLI) {
	rcer = DigestArgs(null);
	if (rcer < 0) {
	    StartupCleanup(wbm);
	    return 10;
	}
    }
#ifdef DETACH
    if (ret = WellDetachItAlready()) {
	if (ret > 0) {
	    StartupCleanup(null);
	    SetIoErr(ERROR_NO_FREE_STORE);
	    return ret;
	}
	SetIoErr(0);
	return 0;
    }
    if (detested) {			/* WB launched */
#else
    if (!me->pr_CLI) {
#endif DETACH
	WaitPort(&me->pr_MsgPort);
	wbm = (struct WBStartup *) GetMsg(&me->pr_MsgPort);
	if (DigestArgs(wbm) < 0) {
	    StartupCleanup(wbm);
	    return 0;
	}
    }
    SeedRandom();
    InitUserNameNum();
    if (!NEWP(fib) || !AllocBigBSSthings()) {
	WorkbenchRequesterArgs("Q-Blue: not enough memory", "Okay");
	StartupCleanup(wbm);
	SetIoErr(ERROR_NO_FREE_STORE);
	return 20;
    }
    if (configfilename[0]) {
        rcer = ReadConfig();
	if (rcer >= 2) {
	    na = (rcer - 1) << 2;
	    strcpy(configfilename, defigfile);
	    rcer = ReadConfig();
	    if (rcer != 1)
		rcer += na;
	}
    } else {
	strcpy(configfilename, defigfile);
	strcpy(userfigfile, defigfile);
	rcer = ReadConfig();
    }
    if (!(semaphored = ConnectToSemaphore())) {
	WorkbenchRequesterArgs("Q-Blue: not enough memory", "Okay");
	StartupCleanup(wbm);
	SetIoErr(ERROR_NO_FREE_STORE);
	return 20;
    }
#if (defined(BETA) && !defined(ALPHA)) || defined(TRY)
    FlipBGadgets(0);
#else
    FlipBGadgets(0x0F);
#endif
    if (!OpenDisplay()) {
	WorkbenchRequesterArgs("Could not open Q-Blue's\n"
				"screen!  No chip ram?", "Okay");
	StartupCleanup(wbm);
	SetIoErr(ERROR_NO_FREE_STORE);
	return 20;
    }
    AddPath(wbm ? wbm->sm_Message.mn_ReplyPort->mp_SigTask : originalme);
#if (defined(BETA) && !defined(ALPHA)) || defined(TRY)
    Aboutness();			/* shareware/beta sufferin */
#endif
    SetMessGags(false);
    if (rcer && rcer != 3)	/* silent for absence of default file */
	ReportFigError(rcer);
    SetUpPopKey();
    if (!editoutfile[0] || !edit1command[0] || !downloaddir[0]
			|| !uploaddir[0] || !workdir[0] || !replydir[0])
	ConfigHints();
    quittitude = false;
    if (autopenlist || tryopename[0])
	ReadAPacket(fakery);
    if (!quittitude)
	EventLoop(&DoOuterIDCMP);
    if (tryunpackcd) {
	BPTR ocd = CurrentDir(tryunpackcd);
	EmptyDir("", false);			/* workdirinst */
	UnLock(CurrentDir(ocd));
    }
    CloseDisplay();
    SetDownPopKey();
    FreePool(stringPermPool);
    NukePath();
    StartupCleanup(wbm);
    SetIoErr(0);
    return 0;
}
