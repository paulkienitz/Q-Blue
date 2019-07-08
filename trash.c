/* lotta miscellaneous stuff used at packet opening time. */

#include <exec/memory.h>
#include <dos/dosextens.h>
#include <dos/stdio.h>
#include <stdlib.h>
#include "qblue.h"


#ifdef TEST13

#  include <devices/timer.h>
#  include <stdarg.h>

long FPuts(BPTR fh, STRPTR s)
{
    return Write(fh, s, strlen(s)) > 0 ? 0 : -1;
}

int vsprintf(char *_s, const char *_format, va_list _arg);

long FPrintf(BPTR fh, STRPTR fmt, ...)
{
    char space[1024];
    va_list args;
    va_start(args, fmt);
    vsprintf(space, fmt, args);
    va_end(args);
    return Write(fh, space, strlen(space));
}

long SetVBuf(BPTR fh, STRPTR buff, long type, long size)
{
    return 0;
}

long SetIoErr(long code)
{
    import struct Process *me;
    long r = me->pr_Result2;
    me->pr_Result2 = code;
    return r;
}

BPTR OpenFromLock(BPTR lok)
{
    BPTR bar, foo, pee;
    Examine(lok, fib);
    bar = CurrentDir(pee = ParentDir(lok));
    foo = OOpen(fib->fib_FileName);
    UnLock(lok);
    CurrentDir(bar);
    if (pee)
	UnLock(pee);
    return foo;
}

long SameLock(BPTR a, BPTR b)
{
    struct FileLock *fa = gbip(a), *fb = gbip(b);
    if (fa->fl_Volume != fb->fl_Volume)
	return LOCK_DIFFERENT;
    else
	return fa->fl_Key == fb->fl_Key ? LOCK_SAME : LOCK_SAME_VOLUME;
}

long NameFromLock(BPTR lok, STRPTR buffer, long len)
{
    return 0; /* fail */
}

#  asm
	PUBLIC	_Close,_DOSBase
_Close:	move.l	a6,-(sp)
	move.l	8(sp),d1
	move.l	_DOSBase,a6
	jsr	-36(a6)
	move.l	(sp)+,a6
	moveq	#-1,d0
	rts
#  endasm

STRPTR PathPart(STRPTR name)
{
    STRPTR p = strrchr(name, '/');
    if (!p) p = strrchr(name, ':');
    return p ? p + (*p == ':') : name;
}

STRPTR FilePart(STRPTR name)
{
    STRPTR p = PathPart(name);
    return p + (*p == '/');
}

BOOL /* <- error in clib/dos_protos.h -- it's really LONG */
     Fault(long code, STRPTR header, STRPTR buffer, long len)
{
    return sprintf(buffer, "%s: (error %ld)", header, code);
}

void GetSysTime(struct timeval *tm)
{
    void CurrentTime(ULONG *, ULONG *);		/* intuition.library */
    CurrentTime(&tm->tv_secs, &tm->tv_micro);
}

#endif


struct Mess *NewPoolMess(void);
ustr NewPoolString(ushort length);
ulong DS2ux(struct DateStamp *d);
void DateString(struct DateStamp *d, str s);
/* bool LoadMessage(struct Mess *mm, BPTR hand, short qeol, bool trim); */
bool AllBlank(ustr line);
bool ParseNetAddress(str a);
bool RemoveTaglineFromReply(struct Mess *mm, short whichreply, str dump);
void JoinName(str result, str dir, str file, str tail);
void ux2DS(struct DateStamp *d, ulong inn);
void ReadWriteErr(str action, str file, ushort dir);


import char tempacketname[], doorconn[], CONTNAME[], iedummyto[];
import char bbsname[], bbscity[], bbsphone[], sysopname[], myloginame[];
import ustr *oldfilenames, *messageID, *newsgroups;

import ulong downloaddate;
import ushort downernames, downertitles, totalmesses, oldracount;
import ushort czone, cnet, cnode, cpoint;
import short confounded;

import bool personalityflaw, myother26, mylogin26, any26, surepacketname;
import bool lookslikeqwk, replylike, qwkreplike, zerosizefile, useUPL;
import bool onlydirsinthere, twits, uppitynames, uppitytitle, dos3;
import bool qwkE, qwkE_subs, official_iemail, searchlight_ie, pcboard_ie;
import bool dooraddrop, doorreset, wouldahaddrop, searchlight;


str fibupt, fibuf;
long fibize;
ulong temptime, megachexum;


struct Conf personals = {
    { /* &personalptrs[0] */ null } , null, " (messages addressed to you)",
    0, 0, 0, 0, 0, 0, " \xFEp!\xFE", "", 0
};

struct Mess *(bullptrs[TRASHLIMIT]);

struct Conf bullstuff = {
    { &bullptrs[0] }, null, " (news, bulletins, other special files)",
    0, 0, 0, 0, 0, 0, " \xFEs?\xFE", "", 0
};

struct Mess placeholder = {
    null, null, null, null, null, 0, 0, 0, 0, 0, 0,
    0, 0, LOADED | DONTSTRIP, 0, 0, 0, 0, 0, 0, null, "(none)",
    "placeholder -- no mail to read", null, "", " \xFEs?\xFE"
};

struct Conf *(placeholderareaz[2]) = { &bullstuff, &replies };

struct Conf **readareazfree;
short *tempcurrents;

/****  struct trash trashfiles[TRASHLIMIT];  ****/
struct trash *trashfiles = null;		/* allocated at startup */

/* const */ char NEWFNAME[] = "NEWFILES.DAT", CONTNAME[] = "CONTROL.DAT";
char thefilenote[80];


struct twit {
    struct twit *next;
    char packet[9], area[SHOCOLEN];
    union {
	char name[NAMELEN];
	struct {
	    short zone, net, node, point;
	} b;
    } u;
    ushort bits;
} *twitlist = null, *antitwits = null;


#define TBIT_FROM  0x01
#define TBIT_TO    0x02
#define TBIT_SUBJ  0x04
#define TBIT_BBS   0x08
#define TBIT_PRIV  0x10
#define TBIT_PUB   0x20
#define TBIT_YOURS 0x40
#define TBIT_ELSES 0x80


str strncpy0(str dest, str src, ushort lim);
void Stranks(str src);
void utoa(ushort i, str a);
/* void itoa(short i, str a), ultoa(ulong i, str a) */ ;
str strnchr(str in, char what, ushort lim), strend(str s), streol(str s);
/* void strac(str dest, short limit); */

/* DO NOT IMPORT strncpy0() or strend() or strac() without these pragmas! */
#pragma regcall(strncpy0(a0,a1,d0))
#pragma regcall(strend(a1))
/* #pragma regcall(strac(a0,d0)) */

/* no pragma for strnchr(), Stranks(), utoa(), or streol() */


void Personalize(struct Mess *mm)
{
    struct Mess *mmm;
    str f, t, s;

    if (!(mm->bits & TOME) || personals.messct >= PERSONALIMIT)
	return;
    if (!(mmm = NewPoolMess()) || !(f = NewPoolString(strlen(mm->from)))
			|| !(t = NewPoolString(strlen(mm->too)))
			|| !(s = NewPoolString(strlen(mm->subject)))) {
	if (!personalityflaw)
	    Err("Can't copy message(s) into\npersonal area; no memory.");
	personalityflaw = true;
	return;
    }
    *mmm = *mm;
    strcpy(f, mm->from);
    strcpy(t, mm->too);
    strcpy(s, mm->subject);
    mmm->from = f;
    mmm->too = t;
    mmm->subject = s;
    mmm->lines = (adr) mmm->linetypes = null;
    mmm->bits &= ~LOADED;
    mmm->bits |= PERSONALCOPY;
    mmm->personalink = mm;
    mm->personalink = mmm;
    personals.messes[personals.messct++] = mmm;
    personals.tomect = personals.messct;
}


local bool FTCheck(str name, str try, bool m26)
{
    str atpt;
    long leaf;

    if (!*try)
	return false;
    if (!stricmp(try, name) || (m26 && !strnicmp(try, name, 25)))
	return true;
    if (_toupper(*try) == _toupper(*name)
			&& strlen(name) > (leaf = strlen(try))) {
	for (atpt = name + leaf; *atpt; atpt++)
	    if (isalpha(*atpt))
		return false;
	return !strnicmp(name, try, leaf);
    } else
	return false;
}


bool AnyLower(register str s)
{
    while (*s)
	if (islower(*(s++)))
	    return true;
    return false;
}


void FromTo(struct Mess *mm)
{
    str anyn = localanyname[0] ? localanyname : anyname;

    if (FTCheck(mm->from, myloginame, mylogin26)
			|| FTCheck(mm->from, myothername, myother26)
			|| FTCheck(mm->from, anyn, any26))
	mm->bits |= FROMME;
    if (FTCheck(mm->too, myloginame, mylogin26)
			|| FTCheck(mm->too, myothername, myother26)
			|| FTCheck(mm->too, anyn, any26))
	mm->bits |= TOME;
    if (!qwk)
	return;
    if (AnyLower(mm->from) || AnyLower(mm->too))
	downernames++;
/************
    if (AnyLower(mm->subject))
	downertitles++;
************/
    totalmesses++;
}


void Bogotify(short lastbogon)
{
    short i;
    struct Mess *mm, *mmr;

    for (i = 0; i < lastbogon; i++) {
	mm = replies.messes[i];
	if (mmr = mm->mreplyee) {
	    mmr->mreplyee = mm->mreplyee = null;
	    mmr->bits &= ~MEREPLIED;
	    mm->bits &= ~MEREPLIED;
	    if (mmr->personalink) {
		mmr->personalink->mreplyee = null;
		mmr->personalink->bits &= ~MEREPLIED;
	    }
	}
    }
}


/* Dual purpose function ... if passed a file lock, it returns its size.  If
passed a directory lock it returns -1 if the directory is not empty, zero if it
is.  In the nonempty directory case it has side effects: if it finds a BW-like
filename it attempts to derive a packet name from it, and copies it into the var
tempacketname.  If it finds a QWK-like name it sets tempacketname to "" and sets
lookslikeqwk to true.  If it finds an apparent reply file it sets replylike
true, and qwkreplike if its name is *.MSG.  It sets temptime to the datestamp of
whatever file it finds that indicates that either a download or an upload packet
is present.  If it finds only subdirectories it sets onlydirsinthere.  If the
thingy exists, is a file, and has no length, it sets zerosizefile.  If it's a
file, its filenote is copied to thefilenote. */

/* Yes, I know that's a terrible mess.  It just sort of accumulated... */

long FileSize(BPTR lock)
{
    long size = 0;
    char tail[32];
    str p;
    bool gotanondir = false, gotadir = false;
    struct FileInfoBlock *fb;

    lookslikeqwk = replylike = qwkreplike = zerosizefile = false;
    if (!NEWP(fb)) {		/* don't clobber the global fib */
	Err("Could not check file or\ndirectory; no memory.");
	return 0;
    }
    if (Examine(lock, fb))
	if (fb->fib_DirEntryType < 0) {
	    size = fb->fib_Size;			/* file */
	    zerosizefile = !size;
	    strcpy(thefilenote, fb->fib_Comment);
	} else while (ExNext(lock, fb)) {
	    size = -1;
	    p = &fb->fib_FileName[0];
	    if (fb->fib_DirEntryType >= 0)
		gotadir = true;
	    else {
		gotanondir = true;
		if (strlen(p) > 3 && (!stricmp(p, CONTNAME)
				|| !stricmp(p, "MESSAGES.DAT")
				|| !stricmp(strend(p) - 4, ".NDX"))) {
		    tempacketname[0] = 0;
		    lookslikeqwk = true;
		    temptime = DS2ux(&fb->fib_Date);
		    break;
		}
		strcpy(tail, p);
		if ((p = strchr(tail, '.')) && p != &tail[0]) {
		    *(p++) = 0;
		    if (!stricmp(p, "INF") || !stricmp(p, "MIX")
					    || !stricmp(p, "FTI")) {
			strcpy(tempacketname, tail);
			strupr(tempacketname);
			temptime = DS2ux(&fb->fib_Date);
			break;
		    }
		    if ((!surepacketname || !stricmp(packetname, tail))
				&& (!stricmp(p, "UPL") || !stricmp(p, "UPI")
				    || !stricmp(p, "REQ") || !stricmp(p, "PDQ")
				    || (qwkreplike = !stricmp(p, "MSG")))) {
			replylike = true;
			temptime = DS2ux(&fb->fib_Date);
		    }
		}
	    }
	}
    FREE(fb);
    onlydirsinthere = gotadir && !gotanondir;
    return size;
}


local char lonfycd[] = "List of new files you can download";

void PickUpTheTrash(void)		/* must be CD'd to work dir */
{
    short t;
    struct Mess *mm;
    char subj[SUBJLEN];
    BPTR hand;
    str tail, name;

    for (t = 0; t < TRASHLIMIT; t++)
	if (*(name = &trashfiles[t].n[0]))
	    if ((hand = OOpen(name)) && (mm = NewPoolMess())
				&& (mm->too = NewPoolString(strlen(name)))) {
		strcpy(mm->too, name);
		subj[0] = 0;
		if (qwk) {
		    if (t == 0)
			strcpy(subj, "Welcome message (logon announcement)");
		    else if (t == 1)
			strcpy(subj, "Current news of this BBS");
		    else if (t == 2)
			strcpy(subj, "Goodbye message (logoff announcement)");
		    else if (!strncmp(name, NEWFNAME, 8))
			strcpy(subj, lonfycd);
		    else if (!strncmp(name, "BLT-", 4))
			strcpy(subj, "A new bulletin");
		/*  else if (!strcmp(name, "ATTACHED.LST"))
			strcpy(subj, "List of attached files");
		*/  else if (!strcmp(name, "SESSION.TXT"))
			strcpy(subj, "Your mail door download session");
		} else if ((tail = strchr(name, '.')) && !strcmp(tail, ".NWS")
					|| !strncmp(name, "WELCOME", 7))
		    strcpy(subj, "Welcome message");
		else if (!strncmp(name, NEWFNAME, 8))
		    strcpy(subj, lonfycd);
		else if (!strncmp(name, "BLT", 3) || !strncmp(name, "BULLET", 6))
		    strcpy(subj, "A new bulletin");
		if (subj[0] && (mm->subject = NewPoolString(strlen(subj))))
		    strcpy(mm->subject, subj);
		mm->ixinbase = bullstuff.messct + 1;
		mm->datfseek = -1;
		mm->datflen = trashfiles[t].filesize;
		mm->unixdate = DS2ux(&trashfiles[t].d);
		mm->bits = /* DONTSTRIP; */ ISBULLETIN;
		DateString(&trashfiles[t].d, mm->date);
/*		if (LoadMessage(mm, hand, 0, true)) */
		bullstuff.messes[bullstuff.messct++] = mm;
/*		Close(hand); */
		mm->from = (str) hand;
	    } else if (hand) {
		Close(hand);
		Err("Could not store list of\nbulletin files -- no memory.");
	    }
    if (bullstuff.messct) {
	--readareaz.confs;
	++readareaz.messct;
	readareaz.confs[0] = &bullstuff;
	ASSERT(readareaz.confs >= readareazfree);
    }
}


local int Trest(struct trash *a, struct trash *b)	/* uppercase names */
{
    register str aa = &a->n[0], bb = &b->n[0];
    str pa, pb;
    short fa, fb, ea, eb;

    if (strncmp(aa, "BLT-", 4) || strncmp(bb, "BLT-", 4))
	return strcmp(aa, bb);
    if (pa = strchr(aa, '.')) {
	*pa = 0;
	fa = atoi(aa + 4);
	*pa = '.';
	ea = atoi(pa + 1);
    } else {
	fa = atoi(aa + 4);
	ea = 0;
    }
    if (pb = strchr(bb, '.')) {
	*pb = 0;
	fb = atoi(bb + 4);
	*pb = '.';
	eb = atoi(pb + 1);
    } else {
	fb = atoi(bb + 4);
	eb = 0;
    }
    return (fa == fb ? ea - eb : fa - fb);
}


local short KnownTrash(const str name, short sofar)
{
    short i;
    for (i = 0; i < sofar; i++)
	if (!strcmp(name, trashfiles[i].n))
	    return i;
    return -1;
}


/* maybe we should cache the attachment list somewhere to make this efficient... */
local bool IsAttached(str name)
{
    int i, j;
    if (!stricmp(name, "ATTACHED.LST"))
	return true;
    for (i = 0; i < readareaz.messct; i++) {
	struct Conf *cc = readareaz.confs[i];
	for (j = 0; j < cc->messct; j++)
	    if (cc->messes[j]->attached &&
			!stricmp(name, cc->messes[j]->attached->tempname))
		return true;
    }
    return false;
}


local bool TrashEnding(register str ext, str whole)
{
    static ulong trashexts[] = {
	'.MIX', '.FTI', '.INF', '.PTN', '.PTO', '.PTX', '.XTI',
	'.NDX', '.PTR', '.PNT', '.QM4', '.QPT', '.DAT', 0
    };
#   define MUST_BE_PACKETNAMED 7
    ulong lext;
    register str txel = (str) &lext;

    if (strlen(ext) != 4)
	return false;
    *txel++ = *ext++;		/* so we can compare bytes as a longword */
    *txel++ = *ext++;
    *txel++ = *ext++;
    *txel = *ext;
    for (ext = (str) &trashexts[0]; *(ulong *) ext; ++(ulong *) ext)
	if (*(ulong *) ext == lext)
	    return ext >= (str) &trashexts[MUST_BE_PACKETNAMED]
			    || (lext = strlen(packetname),
				!strncmp(whole, packetname, lext)
				&& whole[lext] == '.');
    return false;
}


void ScanForTrash(void)
{
    str p, q;
    BPTR lk;
    short t = 3, v = (qwk ? 3 : 5), x = v, i;
    bool dotty;

    while (x < TRASHLIMIT)
	trashfiles[x++].n[0] = 0;
    for (x = 0; x < v; x++) {
	p = &trashfiles[x].n[0];
	strupr(p);
	t = strlen(p) - 1;
	if (t >= 0 && t <= 9 && p[t] == '.')
	    p[t] = 0;
	if (p[0] && (lk = RLock(p))) {
	    Examine(lk, fib);
	    UnLock(lk);
	    trashfiles[x].d = fib->fib_Date;
	    trashfiles[x].filesize = fib->fib_Size;
	} else
	    trashfiles[x].filesize = maxlong;
    }
    if (qwk && KnownTrash(NEWFNAME, 3) < 0 && (lk = RLock(NEWFNAME))) {
	Examine(lk, fib);
	UnLock(lk);
	trashfiles[v].d = fib->fib_Date;
	trashfiles[v].filesize = fib->fib_Size;
	strcpy(trashfiles[v++].n, NEWFNAME);
    }
    lk = me->pr_CurrentDir;
    t = v;
    if (Examine(lk, fib)) {
	while (ExNext(lk, fib)) {
	    if (t >= TRASHLIMIT) {
		Err("Too many extra files in the\npacket (bulletins or"
				    "whatever)\nto load them all in.");
		break;
	    }
	    p = &fib->fib_FileName[0];
	    if ((x = strlen(p)) <= 12) {    /* <- fix this limit someday... */
		strupr(p);
		q = p + x - 4;
		if (dotty = x <= 9 && p[x - 1] == '.')
		    p[--x] = 0;
		if (q < p) q = null;
		if ((i = KnownTrash(p, t)) >= 0) {
		    if (dotty)
			strcpy(trashfiles[i].n + x, ".");
		} else if (!strcmp(p, "TOREADER.EXT"))
		    qwkE = true;
		else if (((qwk ? (strcmp(p, "DOOR.ID") && !TrashEnding(q, p))
			    : !strncmp(p, "BLT", 3) || !strncmp(p, "BULLET", 6))
			  && !IsAttached(p)) || !strncmp(p, NEWFNAME, 8)) {
		    if (dotty) p[x] = '.';
		    trashfiles[t].d = fib->fib_Date;
		    trashfiles[t].filesize = fib->fib_Size;
		    strcpy(trashfiles[t++].n, p);
		}
	    }
	}
	qsort(trashfiles + v, TRASHLIMIT - v, sizeof(struct trash), Trest);
    }
}


void ExhaleFile(void)
{
    Vfree(fibuf);
    fibuf = null;
    fibize = 0;
}


bool IsRomFilesystem(struct MsgPort *handler)
{
#ifdef TEST13
    return false;
#else
    struct DosList *dol;
    bool romitude = false;
    signed long seg;

    if (!TypeOfMem(handler))
	return false;
    dol = LockDosList(LDF_DEVICES | LDF_READ);
    while (dol = NextDosEntry(dol, LDF_DEVICES))
	if (dol && dol->dol_Task == handler) {
	    seg = dol->dol_misc.dol_handler.dol_SegList;
	    if (dol->dol_misc.dol_handler.dol_Startup) {
		romitude = (seg > 1024 || seg < -1024) && !TypeOfMem(gbip(seg));
	    } else if (!dol->dol_misc.dol_handler.dol_Handler && !seg)
		romitude = true;	/* this is probably Ram-Handler */
	    break;
	}
    UnLockDosList(LDF_DEVICES | LDF_READ);
    return romitude;
#endif
}


BPTR SafeOpenFromLock(register BPTR lok)
{
    register BPTR bar, foo;
    long ack;

    if (IsRomFilesystem(bip(struct FileLock, lok)->fl_Task)
					&& (foo = OpenFromLock(lok)))
	return foo;
    if (!Examine(lok, fib)) return 0;
    bar = CurrentDir(ParentDir(lok));		/* funky filesystem... */
    ack = bip(struct FileLock, lok)->fl_Access;
    if (ack == EXCLUSIVE_LOCK) {		/* probably never happens */
	UnLock(lok);
	foo = NOpen(fib->fib_FileName);
    } else {
	foo = OOpen(fib->fib_FileName);
	UnLock(lok);
    }
    UnLock(CurrentDir(bar));
    return foo;
}


/* Pass this the name of a SMALLISH file.  Free with ExhaleFile().       */
/* Side effect: copies filenote to thefilenote, due to calling FileSize. */

str InhaleFile(str filename)
{
    BPTR hand, foot;

    ExhaleFile();
    if (foot = RLock(filename))
	fibize = FileSize(foot);
    else
	fibize = 0;
    if (fibize <= 0) {
	if (foot)
	    UnLock(foot);
	return foot && !fibize ? "" : null;
    }
    if (!(fibuf = Valloc(fibize + 2 /* safety pad */ ))) {
	UnLock(foot);
	SetIoErr(ERROR_NO_FREE_STORE);
	return null;
    }
    if (hand = SafeOpenFromLock(foot)) {
	if ((fibize = RRead(hand, fibuf, fibize + 2)) > 0)
	    fibuf[fibize] = 0;
	else
	    ExhaleFile();
    } else {
	UnLock(foot);
	ExhaleFile();
    }
    if (hand) Close(hand);
    return fibuf;
}


void EmptinessGripe(void)
{
    if (bullstuff.messct && !twits)
	Err("This packet contains no messages, other\n"
				"than files in the news and bulletins area.");
    else if (twits)
	Err("Every message in this packet was\n",
			    "rejected by the twit list!");
    else
	Err("This packet contains no messages.");
}


void UsePlaceholder(void)
{
    struct DateStamp d;
    short a, n;
    struct Conf *cc;

    readareaz.confs = placeholderareaz;		/* &bullstuff, &replies */
    readareaz.messct = bullstuff.messct = 1;
    bullstuff.messes[0] = &placeholder;
    ux2DS(&d, downloaddate);
    DateString(&d, placeholder.date);
    readareazfree = inuseareaz.confs = null;
    inuseareaz.messct = 0;
    if (!fakery || qwk)
	return;
    for (n = a = 0; a < areaz.messct; a++)
	if ((cc = areaz.confs[a])->areabits & INF_SCANNING)
	    n++;
    if (!(inuseareaz.confs = Valloc(n << 2)))
	return;
    for (a = 0; a < areaz.messct; a++)
	if ((cc = areaz.confs[a])->areabits & INF_SCANNING)
	    inuseareaz.confs[inuseareaz.messct++] = cc;
}


bool MakeReadAreaz(void)
{
    ushort a, n, nx;
    struct Conf *cc;

    for (n = nx = a = 0; a < areaz.messct; a++)
	if (areaz.confs[a]->messct) {
	    n++;
	    if (qwk)
		areaz.confs[a]->areabits |= INF_SCANNING;
	} else if (areaz.confs[a]->areabits & INF_SCANNING)
	    nx++;
    if (!(readareazfree = Valloc((n + 3) * 4))) {
	Err("No memory to create list\nof areas with messages.");
	return false;
    }
    readareaz.confs = readareazfree + 2;
    inuseareaz.confs = Valloc((n + nx) * 4);
    inuseareaz.messct = 0;
    for (a = 0; a < areaz.messct; a++) {
	cc = areaz.confs[a];
	cc->sofar = 0;
	if (cc->messct)
	    readareaz.confs[readareaz.messct++] = cc;
	if (cc->messct || cc->areabits & INF_SCANNING) {
	    if (inuseareaz.confs)
		inuseareaz.confs[inuseareaz.messct++] = cc;
	}
    }
    readareaz.confs[readareaz.messct] = &replies;
    if (personals.messct) {
	--readareaz.confs;
	readareaz.messct++;
	readareaz.confs[0] = &personals;
    }
    tempcurrents = Valloc((readareaz.messct + 2) * 2);
    return true;
}


bool SaveToFile(struct Mess *savee, str filename, short whichreply)
{
    BHandle hand;
    short i, l, w, g, top;
    bool suck = false, crlf = (whichreply >= 0), taggd = false, hascr;
    long ellen = 1 + crlf;
    static char chairs[2] = { '\r', '\n' };
    str eol = chairs + 1 - crlf;
    ustr p;

    if (qwk & crlf)
	return true;				/* pretend */
    PortOff();
    if (!(hand = BOpen(filename, true))) {
	PortOn();
	DosErr("Could not open file\n%s\nto save message in.", filename);
	return false;
    }
    if (whichreply < 0)		/* does not actually remove it */
	taggd = RemoveTaglineFromReply(savee, -1, null);
    if (crlf && savee->bits & EMAIL_REPLY && !official_iemail &&
			    (iedummyto[0] || strlen(savee->too) > fromtolen)) {
	char kluge[160];
	if (pcboard_ie) {	/* unlikely with BW, but it could happen */
	    ubyte c = savee->too[60];
	    savee->too[60] = 0;
	    sprintf(kluge, "\xFF@TO     :%-60sN\r\n\r\n", savee->too);
	    savee->too[60] = c;
	    if (strlen(savee->too) > 60)
		sprintf(kluge + 73, "\xFF@TO2    :%-60sN\r\n\r\n",
					savee->too + 60);
	    /* do "@ATTACH : realname.ext (size) tempname.000" at some point */
	} else
	    sprintf(kluge, "%s: %s\r\n\r\n", searchlight_ie ? "Internet"
					: "To", savee->too);
	if (BWrite(hand, kluge, strlen(kluge), 1) < 1) {
	    DosErr("Could not write internet address\nto file %s", filename);
	    goto ohfuckit;
	}
    }
    top = savee->linect - (taggd ? 3 : 1);
    for (i = 0; i <= top; i++) {
	p = savee->lines[i];
	if (savee->linetypes && i < top)
	    w = savee->linetypes[i + 1], g = w & GAP_MASK, w &= TYPE_MASK;
	else
	    w = BODYTYPE, g = 0;
	l = (p ? p[-1] : 0);
	while (l && !p[l - 1])
	    l--;
	if (l && p[l - 1] == '\r')
	    l--;
	if (l && BWrite(hand, p, (long) l, 1) < 1) {
	    DosErr("Could not save message text in file %s.", filename);
	    goto ohfuckit;
	}
	if (hascr = (i < top || !crlf || !lch(p, "... ")) && (w < WRAPTYPE || g))
	    if ((w >= WRAPTYPE && g ? BWrite(hand, GAP_SPACES, g, 1)
					: BWrite(hand, eol, ellen, 1)) < 1) {
		DosErr("Could not save message text in file %s.", filename);
		goto ohfuckit;
	    }
    }
    suck = true;
    if (crlf) {
	struct Conf *cc = Confind(savee->confnum);
	if (cc && cc->morebits & MULTI_NEWSGROUP) {
	    if ((p = messageID[whichreply]) && (i = strlen(p)) > 11
					&& *p == '\x01') {
		if (!hascr)	/* && !strncmp(p, "\x01References:", 12) */
		    BWrite(hand, eol, ellen, 1);
		BWrite(hand, p, i, 1);		/* permit failure */
		hascr = true;
	    }
	    if (p = newsgroups[whichreply]) {
		while (*p == ' ') p++;
		Stranks(p);
		if (*p) {
		    if (!hascr)
			BWrite(hand, eol, ellen, 1);
		    if (BWrite(hand, "\x01Newsgroups: ", 13, 1) > 0) {
			BWrite(hand, p, strlen(p), 1);
			BWrite(hand, eol, ellen, 1);
		    }
		}
	    }
	}
	if (oldfilenames[whichreply]) {
	    if (stricmp(oldfilenames[whichreply], filename))
		DeleteFile(oldfilenames[whichreply]);
#ifdef OLD_OLDFILENAMES
	    Vfree(oldfilenames[whichreply]);
	    oldfilenames[whichreply] = null;
	}
#else
	} else {
	    str foo = oldfilenames[whichreply] = BValloc(15);
	    if (foo) {
		foo[13] = savee->ixinbase >> 8;
		foo[14] = savee->ixinbase & 0xFF;
	    }
	}
	if (oldfilenames[whichreply])
	    strcpy(oldfilenames[whichreply], filename);
#endif
    }
ohfuckit:
    ellen = BClose(hand);
    if (suck && !ellen) {
	suck = false;
	DosErr("Attempt to save message text in file\n"
				"%s has apparently failed.", filename);
    }
    PortOn();
    return suck;
}


void EmptyDir(str path, bool quiet)
{
    char filename[32];
    BPTR oldcd, dlok;

    if (!(dlok = RLock(path)) || !Examine(dlok, fib)) {
	if (!quiet) {
	    DosErr("Could not delete old files\nfrom your %s directory.",
			(path == workdirinst ? "work" : "replies"));
		      /* ^^^^^^^^^^^^^^^^^^^  MAKE SURE THIS STAYS VALID... */
	}
	if (dlok)
	    UnLock(dlok);
	return;
    }
    oldcd = CurrentDir(dlok);
    filename[0] = 0;
    while (ExNext(dlok, fib)) {
	if (filename[0])
	    DeleteFile(filename);	     /* lag behind ExNext by one name */
	/* ***** SHOULD CHECK for errors?  Note: we do NOT consider it an */
	/*       error if a subdirectory won't delete because not empty.  */
	if (fib->fib_DirEntryType < 0)
	    strcpy(filename, fib->fib_FileName);
	else
	    filename[0] = 0;
    }
    if (filename[0])
	DeleteFile(filename);
    UnLock(CurrentDir(oldcd));
}


void LoadTwitList(void)
{
    register str twt, lend, split;
    register ushort flags;
    register char c;
    struct twit *tt;
    bool antitwit;
    str ma, pa;

    if (twitlist)
	return;
    if (!(twt = InhaleFile("S:Q-Blue.twits")) || !fibize)
	return;
    while (*twt) {
	antitwit = false;
	flags = 0;
	lend = streol(twt);
	if (*lend) *lend++ = 0;
	if (*twt == ';') {		/* comment line */
	    twt = lend;
	    continue;
	}
	split = twt;
	ma = pa = "";
	if (twt = strchr(split, ':')) {
	    *(twt++) = 0;
	    while ((c = *split++) && c != ',')
		switch (_tolower(c)) {
		  case 'f':
		    flags |= TBIT_FROM;
		    break;
		  case 't':
		    flags |= TBIT_TO;
		    break;
		  case 's':
		    flags |= TBIT_SUBJ;
		    break;
		  case 'o':
		    flags |= TBIT_BBS;
		    break;
		  case 'p':
		    flags |= TBIT_PUB;
		    break;
		  case 'm':
		    flags |= TBIT_PRIV;
		    break;
		  case 'y':
		    flags |= TBIT_YOURS;
		    break;
		  case 'e':
		    flags |= TBIT_ELSES;
		    break;
		  case '!':
		    antitwit = true;
		    break;
		}
	    if (c == ',') {			/* 1.1-style packet ID */
		while (isspace(*split))
		    split++;
		ma = split;
		if (split = strchr(split, ','))
		    *(split++) = 0;
		Stranks(ma);
		if (strlen(ma) > 8)
		    ma[0] = 0;
		if (split) {			/* area name/number */
		    while (isspace(*split))
			split++;
		    Stranks(split);
		    pa = split;
		}
	    }
	} else
	    twt = split;
	while (isspace(*twt))
	    twt++;
	Stranks(twt);
	if (flags & TBIT_PRIV && flags & TBIT_PUB)
	    flags &= ~(TBIT_PRIV | TBIT_PUB);
	if (flags & TBIT_YOURS && flags & TBIT_ELSES)
	    flags &= ~(TBIT_YOURS | TBIT_ELSES);
	if (!(flags & (TBIT_FROM | TBIT_TO | TBIT_SUBJ | TBIT_BBS)))
	    flags |= TBIT_FROM;
	if (flags & TBIT_BBS) {
	    str com = strchr(twt, ',');
	    flags = TBIT_BBS;
	    if (com) {			/* packet ID -- 1.0 compatibility */
		*com = 0;
		ma = twt;
		twt = com + 1;
		while (isspace(*twt))
		    twt++;
		Stranks(ma);
		if (strlen(ma) > 8)
		    ma = "";
	    }
	    if (!ParseNetAddress(twt))
		continue;
	}
	if (!*twt || !flags || !NEW(tt))
	    continue;
	strcpy(tt->packet, ma);
	strcpy(tt->area, pa);
	if (flags & TBIT_BBS) {
	    tt->u.b.zone = czone;
	    tt->u.b.net = cnet;
	    tt->u.b.node = cnode;
	    tt->u.b.point = cpoint;
	} else
	    strncpy0(tt->u.name, twt, NAMELEN - 1);
	tt->bits = flags;
	if (antitwit)
	    tt->next = antitwits, antitwits = tt;
	else
	    tt->next = twitlist, twitlist = tt;
	twt = lend;
    }
    ExhaleFile();
}


bool NeedToCheckOrigins(void)
{
    register struct twit *tt;
    for (tt = antitwits; tt; tt = tt->next)
	if (tt->bits & TBIT_BBS && (!tt->packet[0]
				|| !stricmp(tt->packet, packetname)))
	    return true;
    for (tt = twitlist; tt; tt = tt->next)
	if (tt->bits & TBIT_BBS && (!tt->packet[0]
				|| !stricmp(tt->packet, packetname)))
	    return true;
    return false;
}


void FlushTwitList(void)
{
    register struct twit *tt;
    for (tt = twitlist; tt; tt = twitlist) {
	twitlist = tt->next;
	FREE(tt);
    }
    for (tt = antitwits; tt; tt = antitwits) {
	antitwits = tt->next;
	FREE(tt);
    }
    twitlist = antitwits = null;
}


local bool TwitSubTest(struct twit *tt, struct Mess *mm)
{
    register ushort flags = tt->bits, tome = mm->bits & TOME;
    register ushort privates = mm->bluebits & UPL_PRIVATE;
    register struct Conf *cc;
    if ((flags & TBIT_YOURS && !tome) || (flags & TBIT_ELSES && tome)
			|| (flags & TBIT_PUB && privates)
			|| (flags & TBIT_PRIV && !privates)
			|| (tt->packet[0] && stricmp(packetname, tt->packet)))
	return false;
    if (!tt->area[0])
	return true;
    if (!(cc = Confind(mm->confnum)))
	return false;				/* should never happen */
    return !stricmp(tt->area, cc->confnum) || !stricmp(tt->area,
			qwk ? LONGNAME(cc) : cc->shortname);
}


local bool TwitTest1(struct twit *list, register struct Mess *mm)
{
    register struct twit *tt;

    for (tt = list; tt; tt = tt->next) {
	if (tt->bits & TBIT_FROM && !stricmp(mm->from, tt->u.name)
				&& TwitSubTest(tt, mm))
	    return true;
	if (tt->bits & TBIT_TO && !stricmp(mm->too, tt->u.name)
				&& TwitSubTest(tt, mm))
	    return true;
	if (tt->bits & TBIT_SUBJ && strnistr(mm->subject, tt->u.name,
				strlen(mm->subject)) && TwitSubTest(tt, mm))
	    return true;
	if (tt->bits & TBIT_BBS && mm->zone == tt->u.b.zone
			&& mm->net == tt->u.b.net && mm->node == tt->u.b.node
			&& mm->point == tt->u.b.point && TwitSubTest(tt, mm))
	    return true;
    }
    return false;
}


bool TwitTest(struct Mess *mm)
{
    return !TwitTest1(antitwits, mm) && TwitTest1(twitlist, mm);
}


#define BLOXIZE   16384
#define SMALLSIZE 512

bool CopyFile(BPTR inh, str outpath)       /* returns false for success! */
{
    char smallbuf[SMALLSIZE], *blonk;
    long rs, sz = BLOXIZE;
    BPTR ouh = NOpen(outpath);

    if (!ouh)
	return true;
    if (!(blonk = AllocP(BLOXIZE))) {      /* if no big buffer, use small */
	blonk = smallbuf;
	sz = SMALLSIZE;
    }
    Seek(inh, 0, OFFSET_BEGINNING);
    while ((rs = Read(inh, blonk, sz)) > 0)
	if (Write(ouh, blonk, rs) < rs)
	    break;
    if (blonk != smallbuf)
	FreeMem(blonk, sz);
    return !Close(ouh) || rs;     /* if true, probably should delete outpath */
}


bool WriteControlDat(str outpath)	/* returns false for success! */
{
    BPTR ouh = NOpen(outpath);
    struct DateStamp dd;
    char date[32];
    struct Conf *cc;
    short i;

    if (!ouh)
	return true;
    SetVBuf(ouh, null, BUF_FULL, 1024);	/* better performance under 3.x */
    ux2DS(&dd, downloaddate);
    DateString(&dd, date + 2);		/* "mm-dd-yy hh:mm" */
    date[10] = ',';
    strncpy(date, date + 2, 6);		/* convert year to four digits */
    if (date[8] >= '7')
	date[6] = '1', date[7] = '9';
    else
	date[6] = '2', date[7] = '0';
    if (FPrintf(ouh, "%s\r\n%s\r\n%s\r\n%s, Sysop\r\n0,%s\r\n%s:00\r\n"
			"%s\r\n\r\n0\r\n0\r\n%ld\r\n", bbsname, bbscity,
			bbsphone, sysopname, packetname, date, myloginame,
			(ulong) areaz.messct - 1) <= 0) {
	Close(ouh);
	return true;
    }
    for (i = 0; i < areaz.messct; i++) {
	cc = areaz.confs[i];
	if (FPrintf(ouh, "%s\r\n%s\r\n", cc->confnum, LONGNAME(cc)) <= 0) {
	    Close(ouh);
	    return true;
	}
    }
    FPrintf(ouh, "%s\r\n%s\r\n%s\r\n", trashfiles[0].n,
			trashfiles[1].n, trashfiles[2].n);	/* failure OK */
    Close(ouh);
    return false;
}


bool SafeExamineFH(BPTR hand, struct FileInfoBlock *fib, str filename)
{
#ifndef TEST13
    if (IsRomFilesystem(bip(struct FileHandle, hand)->fh_Type)
				&& ExamineFH(hand, fib))
	return true;
    else
#endif
    {
	BPTR lk = RLock(filename);
	if (lk) {
	    long r = Examine(lk, fib);
	    UnLock(lk);
	    return !!r;
	}
    }
    return false;
}


void ContextName(str where)
{
    strcpy(where, packetname);
/*  strlwr(where); */
    strcat(where, qwk ? ".bbs-QWK" : ".bbs-BW");
}


void CreateBBSFile(bool artificial)	/* should be CD'd to work dir */
{
    char fame[20], fame2[20], refame[24];
    BPTR ih, oh, ocd, bbd, l2 = 0;
    str sb;
    long theirage = 0, myage = 0, otherage = 0;
    bool recover = false;

    if (!bbsesdir[0])
	return;
    if (!artificial) {
	if (!qwk) {
	    strcpy(fame, packetname);
	    strcat(fame, ".INF");
	    ih = OOpen(fame);
	    strcpy(fame2, packetname);
	    strcat(fame2, ".FTI");
	    l2 = RLock(fame2);
	} else {
	    fame[0] = 0;
	    ih = OOpen(CONTNAME);
	    l2 = RLock("MESSAGES.DAT");
	}
	if (!ih) {
	    DosErr("Cannot reopen %s to copy\ninto your BBS context directory.",
					qwk ? CONTNAME : fame);
	    return;
	}
	if (l2 && Examine(l2, fib)) {
	    otherage = DS2ux(&fib->fib_Date);
	    UnLock(l2);
	}
	if (SafeExamineFH(ih, fib, qwk ? CONTNAME : fame))
	    theirage = DS2ux(&fib->fib_Date);
	if (otherage > theirage)
	    theirage = otherage;
    } else
	ih = 0;
    if (!(bbd = RLock(bbsesdir))) {
	DosErr("Cannot find BBS context\ndirectory %s", bbsesdir);
	if (ih)
	    Close(ih);
	return;
    }
    ocd = CurrentDir(bbd);
    ContextName(fame);
    refame[0] = 0;
    if (oh = RLock(fame)) {
	if (!artificial && Examine(oh, fib))
	    myage = DS2ux(&fib->fib_Date);
	UnLock(oh);
	if (artificial || myage < theirage) {
	    strcpy(refame, fame);
	    strcat(refame, ".old");
	    Rename(fame, refame);		/* if it fails, live with it */
	}
    }
    if (artificial) {
	if (recover = WriteControlDat(fame))
	    DosErr("Failure writing BBS context\nfile %s.", fame);
    } else {
	if (myage < theirage && (recover = CopyFile(ih, fame)))
	    DosErr("Failure copying BBS context\nfile %s.", fame);
	Close(ih);
    }
    if (recover) {
	DeleteFile(fame);
	if (refame[0])
	    Rename(refame, fame);	/* tough mammary if it fails */
    } else if (artificial || myage < theirage) {
	if (refame[0])
	    DeleteFile(refame);
#ifdef COMMENT_HEXDATE
	sprintf(refame, "%08lx ", downloaddate);
#endif
	if (qwk) {
#ifdef COMMENT_HEXDATE
	    sb = refame + /* strlen(refame) */ 9;
#else
	    sb = refame;
#endif
	    *sb++ = '>';
	    if (wouldahaddrop)
		*sb++ = 'H';		/* had door.id */
	    if (dooraddrop)
		*sb++ = 'M';		/* mail door add/drop configurable */
	    if (doorreset)
		*sb++ = 'R';		/* mail door can reset pointers */
	    if (uppitynames)	
		*sb++ = 'U';		/* uppercase names */
	    if (pcbkluge)
		*sb++ = 'L';		/* PCBoard long subjects */
/*	    if (qwkE)
		*sb++ = 'E';	*/	/* packet had QWKE extensions */
	    if (qwkE_subs)
		*sb++ = 'Z';		/* "Subject: blah" kluge line usable */
	    if (searchlight)
		*sb++ = 'S';		/* door.id said it was Searchlight */
	    /* someday: allow_qpassword, uppitytitle? */
	    if (wouldahaddrop)
		sprintf(sb, " /%s", doorconn);
	    else
		*sb = 0;
#ifndef COMMENT_HEXDATE
	    SetComment(fame, refame);	/* BW needs no filenote at all */
#endif
	}
#ifdef COMMENT_HEXDATE
	SetComment(fame, refame);
#endif
    }

    if (qwk) {				/* Now we check for pointer files: */
	ih = 0;
	CurrentDir(ocd);		/* temporarily back to work dir */
	strcpy(fame, packetname);
	otherage = strlen(fame);
	strcpy(fame + otherage, ".PTR");
	if (!(ih = OOpen(fame))) {
	    strcpy(fame + otherage, ".PNT");
	    if (!(ih = OOpen(fame))) {
		strcpy(fame + otherage, ".PTO");  /* ignore Valence's .PTN */
		if (!(ih = OOpen(fame))) {
		    strcpy(fame + otherage, ".LMR");
		    if (!(ih = OOpen(fame))) {
			strcpy(fame + otherage, ".SFP");
			ih = OOpen(fame);
		    }
		}
	    }
	}
	if (ih) {
	    theirage = myage = 0;
	    if (SafeExamineFH(ih, fib, fame))
		theirage = DS2ux(&fib->fib_Date);
	    CurrentDir(bbd);		/* back to context dir */
	    if (oh = RLock(fame)) {
		if (Examine(oh, fib))
		    myage = DS2ux(&fib->fib_Date);
		UnLock(oh);
	    }
	    if (myage < theirage)
		if (CopyFile(ih, fame))
		    DosErr("Failure copying high-message\npointer file %s", fame);
	    Close(ih);
	} else
	    CurrentDir(bbd);
    }
    UnLock(CurrentDir(ocd));
}


void CheckAnyAllRead(struct Conf *cc)
{
    ushort j, n, nx, b;

    if (!(cc->morebits & ALLREAD)) {
	cc->morebits &= ~ANYREAD;
	nx = n = 0;
	for (j = 0; j < cc->messct; j++)
	    if ((b = cc->messes[j]->bits) & MESEEN)
		n++;
	    else if (b & MEREPLIED && cc != &replies)
		nx++;
	if (n + nx == cc->messct)
	    cc->morebits |= ALLREAD;
	if (n | nx)
	    cc->morebits |= ANYREAD;
    }
}


/* The format of a bookmark file.  It is kept in the context directory and is
named <packetname>.marks-(QWK|BW), and its contents are as follows: */

struct BMheader {
    ulong bcookie;		/* magic number -- different from v2.0's */
    ulong bmchexum;		/* calculated by ReadBWBody() / LoadQWKmess() */
    ulong bairz;		/* count of BMarea thingies following */
    ulong bcurmess;		/* ixinbase number for current message there */
    char bcurconf[8];		/* ascii area "number" for current area */
    char bpacketn[9];		/* the 8 char packet name */
    ubyte /* bool */ tryreply;	/* do reply flip to find the real current msg */
    ubyte bpad[16];		/* reserved -- size is multiple of 4 */
};

#define BCOOKIE 0xD85B929B

/* which is followed by BMheader.bairz instances of this: */

struct BMarea {
    char bconfnum[8];		/* ascii message area "number" */
    ulong bcurrent;		/* ixinbase of message last read in area */
    ushort bmixct;		/* count of message range numbers */
};

/* each of which is followed by bmixct instances of this: */

struct BMix {
#ifdef _INT32
    unsigned bmix : 24;		/* the message with this ixinbase */
    unsigned bconsecutive : 8;	/* how many more numbers after */
#else /* goddamn compiler... */
    ubyte bconsecutive, bmix[3]; /* compiler puts second bitfield first */
#endif
};	/* one longword... run-length compression */

/* Finally, there is now an extra blip at the end, consisting of one ushort */
/* giving the number of ushorts following (usually zero or one) which make  */
/* up an array of bit flags, which are consecutively applied to each msg in */
/* the personals area which is marked as read in the middle section.  If    */
/* the bit is on, then the real message, not the personal copy, is the last */
/* one that has been read, for purposes of the reply flip command.  Absence */
/* of this section of data is accepted for compatibility.                   */

/* This format is designed to be immune to changes of the twit list.  The */
/* bmchexum number is computed before twitting is applied, and should be  */
/* unique to each different packet except in rare cases.  When no more    */
/* than one initial message has been read, the file is absent.  The flags */
/* at the end for personal reply flipping are not twit list immune.       */


void SaveBookmarks(void)
{
    struct BMheader bh;
    struct BMarea ba;
    struct BMix bi;
    BPTR ocd, ccd;
    BHandle hand = null;
    struct Conf *cc, *c1, **raz;
    struct Mess *mm;
    bool noneread = false;
    char fame[32];
    ushort a, i, sec, ract, perseen = 0;
    long px;
    bool outstanding;

    if (fakery || !bbsesdir[0] || !readareaz.messct || !(ccd = RLock(bbsesdir)))
	return;
    if (readareaz.unfiltered)
	raz = (adr) readareaz.unfiltered, ract = oldracount;
    else
	raz = readareaz.confs, ract = readareaz.messct;
    ocd = CurrentDir(ccd);
    strcpy(fame, packetname);
    strlwr(fame);
    strcat(fame, qwk ? ".marks-QWK" : ".marks-BW");
    bh.bcookie = BCOOKIE;
    bh.bmchexum = megachexum;
    bh.tryreply = false;
    if (readareaz.confs[whicha] == &replies) {
	if (replies.messes[whichm]->mreplyee) {
	    strcpy(bh.bcurconf, replies.messes[whichm]->mreplyee->confnum);
	    bh.bcurmess = replies.messes[whichm]->mreplyee->ixinbase;
	    bh.tryreply = true;
	} else {
	    strcpy(bh.bcurconf, replies.confnum);
	    bh.bcurmess = 0;	/* do NOT try to re-find the same reply */
	}
    } else if (onscreen) {
	/* use the conf's number, not the message's, in case it's personals */
	strcpy(bh.bcurconf, readareaz.confs[whicha]->confnum);
	bh.bcurmess = onscreen->ixinbase;
    } else
	bh.bcurconf[0] = bh.bcurmess = 0;
    strcpy(bh.bpacketn, packetname);
    memset(bh.bpad, 0, sizeof(bh.bpad));
    bh.bairz = 0;
    for (a = 0; a < ract; a++) {
	cc = raz[a];
	if (cc == &replies)
	    continue;
	if (cc->unfiltered)
	    cc = cc->unfiltered;
	CheckAnyAllRead(cc);
	if (cc->morebits & ANYREAD)   /* note actual number read may be zero! */
	    c1 = cc, bh.bairz++;
    }
    if (!bh.bairz)
	noneread = true;
    else if (bh.bairz == 1 && !(c1->morebits & ALLREAD)) {
	for (i = 1; i < c1->messct; i++)
	    if (c1->messes[i]->bits & MESEEN)	/* any read after first? */
		goto notnone;
	noneread = true;	/* only the 1st message in one conf has been  */
      notnone: ;		/* read, and we consider that as "none read". */
    }
    if (!noneread) {
	if (!(hand = BOpen(fame, true))) {
	    DosErr("Could not create marks file\n%s in context directory.",
							fame);
	    UnLock(CurrentDir(ocd));
	    return;
	}
	if (BWrite(hand, &bh, sizeof(bh), 1) < 1) {
	    DosErr("Failure writing bookmark\nfile %s.", fame);
	    noneread = true;
	} else {
	    for (a = 0; a < ract; a++) {
		cc = raz[a];
		if (cc == &replies || !(cc->morebits & ANYREAD))
		    continue;
		if (cc->unfiltered)
		    cc = cc->unfiltered;
		((long *) &ba.bconfnum)[0] = ((long *) &ba.bconfnum)[1] = 0;
		strcpy(ba.bconfnum, cc->confnum);
		ba.bcurrent = cc->messes[cc->current]->ixinbase;
		px = -2;
		ba.bmixct = sec = 0;
		for (i = 0; i < cc->messct; i++) {
		    mm = cc->messes[i];
		    if (mm->bits & MESEEN) {
			if (sec < 255 && (signed long) mm->ixinbase == px + 1)
			    sec++;
			else
			    sec = 0, ba.bmixct++;
			px = mm->ixinbase;
			if (cc == &personals)
			    perseen++;
		    }
		}
		if (BWrite(hand, &ba, sizeof(ba), 1) < 1) {
		    DosErr("Failure writing bookmark\nfile %s.", fame);
		    noneread = true;
		} else {
/* gotta go through it all twice, in order to know bmixct ahead of time... */
		    px = -2;
		    sec = 0;
		    outstanding = false;
		    for (i = 0; i < cc->messct; i++) {
			mm = cc->messes[i];
			if (mm->bits & MESEEN) {
			    if (sec < 255 && (long) mm->ixinbase == px + 1)
				sec++;
			    else {
				/* thank god for buffered IO... */
				bi.bconsecutive = sec;
				if (px >= 0 && BWrite(hand, &bi,
							sizeof(bi), 1) < 1) {
				    DosErr("Failure writing marks\nfile %s.",
							fame);
				    goto fuggit;
				}
				sec = 0;
#ifdef _INT32
				bi.bmix = mm->ixinbase;
#else
				bi.bmix[0] = (ubyte) (mm->ixinbase >> 16);
				bi.bmix[1] = (ubyte) (mm->ixinbase >> 8);
				bi.bmix[2] = (ubyte) mm->ixinbase;
#endif
				outstanding = true;
			    }
			    px = mm->ixinbase;
			}
		    }
		    bi.bconsecutive = sec;
		    if (outstanding && BWrite(hand, &bi, sizeof(bi), 1) < 1) {
			DosErr("Failure writing marks\nfile %s.", fame);
			goto fuggit;
		    }
		}
	    }
	}
	a = (perseen + 15) / 16;
	if (BWrite(hand, &a, 2, 1) > 0) {	/* failure now is ignored */
	    a = sec = 0;
	    for (i = 0; i < personals.messct; i++)
		if ((mm = personals.messes[i])->bits & MESEEN) {
		    if (mm->personalink->bits & LASTREALLYREAD)
			sec |= bit(a);
		    if (++a > 15) {
			BWrite(hand, &sec, 2, 1);
			a = sec = 0;
		    }
		}
	    if (a)
		BWrite(hand, &sec, 2, 1);
	}
      fuggit:
	if (!BClose(hand) && !noneread) {
	    DosErr("Apparent failure writing to\nmarks file %s.", fame);
	    noneread = true;
	}
    }
    if (noneread)
	DeleteFile(fame);
    UnLock(CurrentDir(ocd));
}


void RestoreBookmarks(void)
{
    struct BMheader bh;
    struct BMarea ba;
    struct BMix bi;
    BPTR ocd, ccd;
    BHandle hand;
    struct Conf *cc;
    struct Mess *mm;
    char fame[32];
    ushort a, i;
    ulong mix;
    short sec, cur, tcur;
    bool scrub = false;

    ASSERT(!readareaz.unfiltered);
    if (!bbsesdir[0] || !(ccd = RLock(bbsesdir)))
	return;
    ocd = CurrentDir(ccd);
    strcpy(fame, packetname);
    strlwr(fame);
    strcat(fame, qwk ? ".marks-QWK" : ".marks-BW");
    if (!(hand = BOpen(fame, false))) {
	UnLock(CurrentDir(ocd));
	return;
    }
    a = BRead(hand, &bh, sizeof(bh), 1);
    if (a <= 0 || bh.bcookie != BCOOKIE || bh.bmchexum != megachexum
				|| stricmp(bh.bpacketn, packetname)) {
	if (a < 0)
	    DosErr("Could not read marks\nfile %s.", fame);
	BClose(hand);
	UnLock(CurrentDir(ocd));
	return;
    }
    while (bh.bairz-- > 0) {
	if (BRead(hand, &ba, sizeof(ba), 1) < 1) {
	    DosErr("Could not read data\nin marks file %s.", fame);
	    scrub = true;
	    goto bail;
	}
	if (cc = Confind(ba.bconfnum))
	    for (i = 0; i < cc->messct; i++)
		if (cc->messes[i]->ixinbase == ba.bcurrent)
		    cc->current = i;
	i = 0;
	while (ba.bmixct-- > 0) {
	    if (BRead(hand, &bi, sizeof(bi), 1) < 1) {
		DosErr("Could not read data\nin marks file %s.", fame);
		scrub = true;
		goto bail;
	    }
	    if (!cc || cc == &personals) /* ignore ambiguous personal indexes */
		continue;
#ifdef _INT32
	    mix = bi.bmix;
	    sec = bi.bconsecutive & 0x00FF;    /* COMPILER BUG WORKAROUND */
#else
	    mix = bi.bmix[2] | (bi.bmix[1] << 8) | (bi.bmix[0] << 16);
	    sec = bi.bconsecutive;
#endif
	    cur = tcur = -1;
	    do {
		if (i < cc->messct && (mm = cc->messes[i])->ixinbase == mix) {
		    mm->bits |= MESEEN;
		    if (mm->personalink)
			mm->personalink->bits |= MESEEN;
		} else
		    for (i = 0; i < cc->messct; i++)
			if ((mm = cc->messes[i])->ixinbase == mix) {
			    mm->bits |= MESEEN;
			    if (mm->personalink)
				mm->personalink->bits |= MESEEN;
			    break;
			}
		i++, mix++;
	    } while (--sec >= 0);
	}
	if (cc)
	    CheckAnyAllRead(cc);
    }
    if (BRead(hand, &a, 2, 1) > 0 && a > 0)
	for (cur = 16, i = 0; i < personals.messct; i++)
	    if ((mm = personals.messes[i])->bits & MESEEN) {
		if (++cur > 15) {
		    if (!a-- || BRead(hand, &sec, 2, 1) <= 0)
			break;
		    cur = 0;
		}
		if (sec & bit(cur)) {
		    mm->personalink->bits |= LASTREALLYREAD;
		    mm->bits &= ~LASTREALLYREAD;
		} else {
		    mm->bits |= LASTREALLYREAD;
		    mm->personalink->bits &= ~LASTREALLYREAD;
		}
	    }
  bail:
    BClose(hand);
    if (scrub)
	for (a = 0; a < readareaz.messct; a++) {
	    cc = readareaz.confs[a];
	    for (i = 0; i < cc->messct; i++)
		cc->messes[i]->bits &= ~MESEEN;
	    cc->current = 0;
	    cc->morebits &= ~(ANYREAD | ALLREAD);
	}
    else if (cc = Confind(bh.bcurconf)) {
	for (a = 0; a < readareaz.messct; a++)
	    if (readareaz.confs[a] == cc) {
		readareaz.current = a;
		for (i = 0; i < cc->messct; i++)
		    if (cc->messes[i]->ixinbase == bh.bcurmess) {
			cc->current = i;
			mm = cc->messes[i]->mreplyee;
			if (readareaz.confs[readareaz.messct - 1] == &replies
							&& bh.tryreply && mm) {
			    readareaz.current = readareaz.messct - 1;
			    for (a = 0; a < replies.messct; a++)
				if (replies.messes[a] == mm) {
				    replies.current = a;
				    break;
				}
			}
			break;
		    }
		break;
	    }
    }
    UnLock(CurrentDir(ocd));
}


#ifdef C_STRS

local short digit;
   
local void x_utoa(ushort i, str a)
{
    ushort q = i / 10;
   
    if (q) x_utoa(q, a);
    a[digit++] = i % 10 + '0';
}

   
void utoa(ushort i, str a)
{
    digit = 0;
    x_utoa(i, a);
    a[digit] = '\0';
}


/* void itoa(short i, str a)
/* {
/*    if (digit = (i < 0)) {
/*	*a = '-';
/*	i = -i;
/*    }
/*    x_utoa((ushort) i, a);
/*    a[digit] = '\0';
/* }
*******/


/* void strac(str dest, short limit)	/* parse a line from memory */
/* {
/*    bool nb = false;
/*
/*    while (*fibupt && *fibupt != '\r' && *fibupt != '\n')
/*	if (limit > 0 && (nb || *fibupt != ' '))
/*	    *(dest++) = *(fibupt++), nb = true, limit--;
/*	else
/*	    fibupt++;
/*    *dest = 0;
/*    if (*fibupt == '\r') fibupt++;
/*    if (*fibupt == '\n') fibupt++;
/* }
*******/


str strncpy0(str dest, str src, ushort lim)
{
    strncpy(dest, src, lim);
    dest[lim] = 0;
    return dest;
}
/* Defined to copy a string UP TO lim chars long and nul terminate it. */
/* The asm version stops if it encounters a nul byte in the middle.    */


void Stranks(str s)
{
    str p = strend(s);

    while (--p >= s && *p == ' ')
	*p = 0;
}


str strnchr(str in, char what, ushort lim)
{
    while (lim && *in != what)
	in++, lim--;
    return lim ? in : null;
}


str streol(str s)
{
    while (*s && *s != '\n') s++;
    return s;
}


str strend(str s)
{
    while (*s) s++;
    return s;
}

#else
#  asm

;;	PUBLIC		_itoa
;;_itoa:
;;	move.l		6(sp),a0
;;	moveq		#0,d1
;;	move.w		4(sp),d1
;;	bge		noneg
;;	  neg.w		d1
;;	  move.b	#'-',(a0)+
;;noneg:	bsr.s		utoar
;;	clr.b		(a0)
;;	rts

	PUBLIC		_utoa
_utoa:
	move.l		6(sp),a0
	moveq		#0,d1
	move.w		4(sp),d1
	bsr.s		utoar
	clr.b		(a0)
	rts

utoar:	moveq		#10,d0
	divu		d0,d1
	tst.w		d1		; redundant?  My bad asm book no say
	beq.s		norcu
	  move.l	d1,-(sp)
	  and.l		#$0000FFFF,d1
	  bsr.s		utoar
	  move.l	(sp)+,d1
norcu:	swap		d1
	add.b		#'0',d1
	move.b		d1,(a0)+
	rts


;;	PUBLIC		_strac
;;_strac:
;;	move.l		d2,-(sp)
;;	moveq		#0,d1
;;	move.l		_fibupt,a1
;;elihu:	  move.b	(a1),d2
;;	  beq		harris
;;	  cmp.b		#13,d2
;;	  beq		harris
;;	  cmp.b		#10,d2
;;	  beq		harris
;;	    cmp.b	#' ',d2
;;	    bne		s0
;;	    tst.w	d1
;;	    beq		coast
;;s0:	    tst.w	d0
;;	    ble		coast
;;	      moveq	#1,d1
;;	      subq.w	#1,d0
;;	      move.b	d2,(a0)+
;;coast:	    addq.l	#1,a1
;;	    bra		elihu
;;harris:	clr.b		(a0)
;;	cmp.b		#13,(a1)
;;	bne		s1
;;	  addq.l	#1,a1
;;s1:	cmp.b		#10,(a1)
;;	bne		s2
;;	  addq.l	#1,a1
;;s2:	move.l		a1,_fibupt
;;	move.l		(sp)+,d2
;;	rts


	PUBLIC		_strncpy0
_strncpy0:
	move.l		a0,d1
	subq.w		#1,d0
	blt.s		dun0
un1:	  move.b	(a1)+,(a0)+
	  dbeq		d0,un1		; exit loop if byte moved was zero
        beq.s		dun1		; and do NOT add a terminating nul then
dun0:	  clr.b		(a0)		;  or it may overrun an allocation
dun1:	move.l		d1,d0		; return start of copied string
	rts


	PUBLIC		_Stranks
_Stranks:
	move.l		4(sp),a0
	move.l		a0,a1
end0:	  tst.b		(a0)+
	  bne.s		end0
	subq		#1,a0
gin:	  cmp.l		a0,a1
	  bhs.s		ning
	  cmp.b		#' ',-(a0)
	  bne		ning
	  clr.b		(a0)
	  bra.s		gin
ning:	rts


	PUBLIC		_strnchr
_strnchr:
	move.l		4(sp),a0
	move.b		8(sp),d0
	move.w		10(sp),d1
oop:	  tst.w		d1
	  beq		pooo
	  cmp.b		(a0),d0
	  beq		pooo
	  addq.l	#1,a0
	  subq.w	#1,d1
	  bra.s		oop
pooo:	tst.w		d1
	beq.s		po0
	move.l		a0,d0
	rts
po0:	moveq		#0,d0
	rts


	PUBLIC		_strend
_strend:
	  tst.b		(a1)+
	  bne.s		_strend
	move.l		a1,d0
	subq.l		#1,d0
	rts


	PUBLIC		_streol
_streol:
	move.l		4(sp),a1
seoloo:
	move.b		(a1)+,d0
	beq.s		seout
	cmp.b		#10,d0
	bne.s		seoloo
seout:	move.l		a1,d0
	subq.l		#1,d0
	rts

#  endasm

#  ifndef TEST13
#    asm

	PUBLIC		_sprintf		; c.lib used if C_STRS defined
	PUBLIC		_vsprintf

	DSEG
chrco:	ds.w	1
	CSEG

	FAR	DATA

_LVORawDoFmt	equ	-522
rawMoveByte:
	move.b		d0,(a3)+
	addq.w		#1,chrco
	rts

#    endasm
#    ifndef _LARGE_DATA
#      asm
	NEAR	DATA
#      endasm
#    endif
#    asm

_sprintf:
	movem.l		a2/a3/a6,-(sp)
	lea		24(sp),a1		; "..." args
	bra.s		bspf(pc)
_vsprintf:
	movem.l		a2/a3/a6,-(sp)
	move.l		24(sp),a1		; va_list
bspf:	move.l		16(sp),a3		; dest buffer
	move.l		20(sp),a0		; format string
	move.w		#-1,chrco		; compensate for counting \0
	lea		rawMoveByte(pc),a2	; the writing function
	move.l		_SysBase,a6
	jsr		_LVORawDoFmt(a6)	; locale.library WEDGES THIS
	moveq		#0,d0
	move.w		chrco,d0		; return # of chars output
	movem.l		(sp)+,a2/a3/a6
	rts

	public		_SysBase

#    endasm
#  endif TEST13
#endif C_STRS
