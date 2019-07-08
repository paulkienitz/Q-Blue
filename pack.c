/* Q-Blue stuff for file requesters and running compressors */

#define ASL_V38_NAMES_ONLY

#include <exec/types.h>
#include <exec/memory.h>
#include <exec/execbase.h>
#include <dos/dos.h>
#include <dos/dostags.h>
#include <libraries/asl.h>
#include <intuition/sghooks.h>
#include <intuition/intuitionbase.h>
#include "qblue.h"
#include "pigment.h"
#include "semaphore.h"


short AskDifferentPacker(short whichpacker);
bool AskNotSamePacker(short whichpacker);
bool AskUnpackAnyway(str fullname);
bool AskOverwriteAppend(str filename, bool *appendage);
bool AskOverrideNoRename(bool renaming, str backname);

void EmptyDir(str path, bool quiet);
void SetLocalSetupName(void);
void TweakCloseGag(void);
bool WriteUPL(void);
bool MakeLotsaRoom(void);
void ConfigDirs(void);
BPTR CreateLock(str dirname);
bool OneOfOurNotes(str fn);
long ShellExecute(str command, BPTR in, BPTR out);
void SetAllSharedWindowTitles(str title);
void InitASLfake(struct WhereWin *fakww,
		 ushort left, ushort top, ushort width, ushort height);
void UpdateClock(void);
void DecideIfAnythingToSave(void);


import struct DosLibrary *DOSBase;
import struct IntuitionBase *IntuitionBase;
import BPTR fakepath;
import struct MenuItem passum[];
import ushort penzez[];
import char /* bool */ attachment_hidden[];

import char spoors[][SPOORLEN], packernames[GREENBAY][PACKNAMELEN];
import char packommands[][COMMANDLEN], unpackommands[][COMMANDLEN];
import char pubscrname[], edit1command[], edit2command[];
import char taglinesfile[], localtaglinesfile[], loadedtaglinesfile[];
import char title[], localfigname[], r2ename[];

import str oldtaskname;
import ulong downloaddate, uploaddate, oldcentsize, oldcenttime;
import long endtag;
import ushort packers, currentpacker, nomwidth, localpacker, tifight;
import short ourinstance, reloadflag, oldcentread;

import bool conman, appendage, oldreplies, useopenfakery;
import bool needtoemptywork, bad_upl;


struct ExecBase *SysBase;

str xcommand;

char opendir[256], fakendir[256], reqdir[256], reqfile[256], ffrname[290];
char workdirinst[PATHLEN + 10], replydirinst[PATHLEN + 10];
char editininst[PATHLEN + 6], editoutinst[PATHLEN + 6], fakeryname[31];

struct Window *ffrwin;


struct StringInfo ffrstr = STRINF(ffrname, 256);

STRINGBORDER(ffrbox)

struct Gadget ffrgag = {
    null, 20, 27, 360, 8, GFLG_STRINGEXTEND, GACT_RELVERIFY | GACT_STRINGLEFT,
    GTYP_STRGADGET, &ffrbox, null, null, 0, &ffrstr, 1000, null
};

struct ExtNewWindow ffrneww = {
    100, 60, 400, 100, 0, 1, IDCMP_GADGETUP | IDCMP_CLOSEWINDOW | IDCMP_RAWKEY,
    WFLG_NOCAREREFRESH | WFLG_CLOSEGADGET | WFLG_DRAGBAR | WFLG_DEPTHGADGET
		| WFLG_ACTIVATE | WFLG_NW_EXTENDED,
    null, null, "Enter full name of file:", null, null,
    0, 0, 0, 0, CUSTOMSCREEN, null
};

struct WhereWin ffrww = { &ffrneww };


adr AslBase;

bool fkdoquoted, fkdoquotedX, fkdoquotedW, fkdoraw, fkdonormal;
bool wrongpacker, packed_once, abbandonement = false;
BPTR tryunpackcd;

struct WhereWin aslfakww = { null };

struct QSemNode ourqsemnode = {
    { null, null}, workdirinst, replydirinst,
    editoutinst, editininst, pubscrname,
    false, false, false, false, false,
    QBVERSION, -1, false, workdir, replydir
};

char qsemname[] = ">}] Q-Blue [{<";

struct QSemaphore {
    struct SignalSemaphore sem;
    char semname[16];
    struct MinList lst;
    short runningct;
} *qsem;		/* DO NOT CHANGE SIZE! */



str FixWRpath(str what)
{
    register str when = strend(what);
    if (when > what && when[-1] == ':') {
	if (what == workdir || what == workdirinst)
	    strcpy(when, "Q-work");
	if (what == replydir || what == replydirinst)
	    strcpy(when, "Q-rep");
    }
    return when;
}


bool IdenticalPaths(str a, str b)
{
    bool bad;
    BPTR la = 0, lb = 0;
    APTR mwp = me->pr_WindowPtr;
    str oae, obe;

    if (!a[0] || !b[0])
	return /* a[0] == b[0] */ false;
    oae = FixWRpath(a);
    obe = FixWRpath(b);
    if (!(bad = !stricmp(a, b))) {
	me->pr_WindowPtr = (APTR) -1;
	if ((la = RLock(a)) && (lb = RLock(b)))
	    bad = SameLock(la, lb) == LOCK_SAME && !IoErr();
	if (la) UnLock(la);
	if (lb) UnLock(lb);
	me->pr_WindowPtr = mwp;
    }
    *oae = *obe = 0;
    return bad;
}


/* call only when semaphore is obtained! */
local struct QSemNode *DirOverlaps(str path)
{
    struct QSemNode *qs;
    register str w, r;

    for (qs = (adr) qsem->lst.mlh_Head;
			qs->node.mln_Succ; qs = (adr) qs->node.mln_Succ) {
	if (qs != &ourqsemnode) {
	    if (qs->screename[19] && qs->qversion >= 5) {
		r = qs->replydinst[0] ? qs->replydinst : qs->rawreplyd;
		w = qs->workdinst[0] ? qs->workdinst : qs->rawworkd;
	    } else
		r = qs->replydinst, w = qs->workdinst;
	    if (IdenticalPaths(path, w) || IdenticalPaths(path, r))
		return qs;
	}
    }
    return null;
}


local void Make1Instance(str out, str ori, ushort ins, bool shavetail)
{
    char tale[32];
    str shave, tail;
    short a, b;

    strcpy(out, ori);
#ifdef DONT_INSTANTIATE_ONLY_PROC
    if (shavetail ? ins : !!DirOverlaps(ori)) {
#else /* fuck that, we should follow a consistent rule. */
    if (ins) {
#endif
	tail = FilePart(out);
	shave = strrchr(tail, '.');
	b = strlen(tail);
	if (shavetail && shave && shave != tail && shave[1]) {
	    strcpy(tale, shave);
	    *shave = 0;
	} else {
	    tale[0] = 0;
	    FixWRpath(out);
	}
	a = strlen(out);
/**	if (a >= PATHLEN - 4 || b >= 27)
	    a = max(a - 4, a - b);	 */    /* unnecessary; they have room */
	utoa(ins, out + a);
	if (shavetail)
	    strcat(out, tale);
    } else if (!shavetail)
	FixWRpath(out);
}


void MakeInstanceNames(ushort instouse)
{
    if (!ourqsemnode.packetopen || fakery)
	Make1Instance(workdirinst, workdir, instouse, false);
    if (!replylock)
	Make1Instance(replydirinst, replydir, instouse, false);
    if (!ourqsemnode.composingnow) {
	Make1Instance(editininst, editinfile, instouse, true);
	Make1Instance(editoutinst, editoutfile, instouse, true);
    }
}


bool ConnectToSemaphore(void)
{
    Forbid();
    if (!(qsem = (adr) FindSemaphore(qsemname))) {
	if (!NEWP(qsem)) {
	    Permit();
	    return false;
	}
	strcpy(qsem->semname, qsemname);
	qsem->sem.ss_Link.ln_Name = qsem->semname;
	qsem->runningct = 0;
	NewList((struct List *) &qsem->lst);
/*	AddSemaphore(&qsem->sem); */
	InitSemaphore(&qsem->sem);
	qsem->sem.ss_Link.ln_Pri = -1;       /* v37 AddSemaphore zeroes this */
	Enqueue(&SysBase->SemaphoreList, &qsem->sem.ss_Link);
    }
    ObtainSemaphore(&qsem->sem);
    qsem->runningct++;
    MakeInstanceNames(0);		/* TEMPORARY values! */
    AddHead((struct List *) &qsem->lst, (struct Node *) &ourqsemnode.node);
    ReleaseSemaphore(&qsem->sem);
    Permit();	/* permit before release just increases rescheduling overhead */
    return true;
}


void DisconnectFromSemaphore(void)
{
    short rc;
    ObtainSemaphore(&qsem->sem);
    rc = --qsem->runningct;
    Remove((struct Node *) &ourqsemnode);
    if (rc <= 0) {
	RemSemaphore(&qsem->sem);
	ReleaseSemaphore(&qsem->sem);
	/* make sure nobody who found it before RemSemaphore just obtained it */
	ObtainSemaphore(&qsem->sem);	/* all others now guaranteed released */
	ReleaseSemaphore(&qsem->sem);
	FREE(qsem);
    } else
	ReleaseSemaphore(&qsem->sem);
    me->pr_Task.tc_Node.ln_Name = oldtaskname;		/* just in case */
}


/* this returns a set of bits saying how much is overlapping with other */
/* Q-Blue processes; see SOF_ defines in qblue.h */

long CheckSemaphoreOverlap(bool dodirs)
{
    long ret = 0;
    struct QSemNode *qs;

    ObtainSemaphore(&qsem->sem);
    for (qs = (adr) qsem->lst.mlh_Head; qs->node.mln_Succ;
				qs = (adr) qs->node.mln_Succ)
	if (qs != &ourqsemnode) {
	    if (!strcmp(qs->screename, pubscrname))
		ret |= SOF_PUBSCRNAME;
	    if (qs->screename[19] && qs->instancenum == ourinstance)
		ret |= SOF_INSTANCENUM;
	    if (qs->packetopen && !qs->editingeditor && editoutinst[0]
				&& !stricmp(editoutinst, qs->editrepf)) {
		ret |= SOF_EDITOUTFILE;
		if (qs->composingnow)
		    ret |= SOF_EDITORINUSE;
	    }
	    if (qs->packetopen && !qs->editingeditor && editininst[0]
				&& !stricmp(editininst, qs->editquof)) {
		ret |= SOF_EDITINFILE;
		if (qs->composingnow)
		    ret |= SOF_EDITORINUSE;
	    }
	}
    if (dodirs && !qs->editingdirs) {
	if (workdirinst[0] && (qs = DirOverlaps(workdirinst)))
	    ret |= qs->packetopen && !(qs->screename[19] && qs->fakeopen)
					? SOF_INUSEWORK : SOF_MAYBEWORK;
	if (replydirinst[0] && (qs = DirOverlaps(replydirinst)))
	    ret |= qs->anyrepliesyet ? SOF_INUSEREPLY :
			(qs->packetopen ? SOF_EMPTYREPLY : SOF_MAYBEREPLY);
    }
    ReleaseSemaphore(&qsem->sem);
    return ret;
}


void OkayWithEverybodyToOpen(void)
{
    ushort instouse;
    long fbid = fakery ? SOF_INUSEREPLY | SOF_EMPTYREPLY
			: SOF_INUSEWORK | SOF_INUSEREPLY | SOF_EMPTYREPLY;

    workdirinst[0] = replydirinst[0] = 0;
    ObtainSemaphore(&qsem->sem);
    instouse = ourqsemnode.instancenum;
    do {
	MakeInstanceNames(instouse++);
    } while (CheckSemaphoreOverlap(true) & fbid);
    ourqsemnode.packetopen = true;
    ourqsemnode.fakeopen = fakery;
    ReleaseSemaphore(&qsem->sem);
}


void TellEverybodyWeClosed(void)
{
    ourqsemnode.packetopen = ourqsemnode.anyrepliesyet = false;
}


ubyte HexByte(str *try)
{
    ubyte c, cc;

    if (!isxdigit(c = toupper(**try)))
	return -1;
    if (!isxdigit(cc = toupper(*++*try)))
	cc = c, c = '0';
    else
	++*try;
    c -= '0';
    if (c > 9) c -= 'A' - 10 - '0';
    cc -= '0';
    if (cc > 9) cc -= 'A' - 10 - '0';
    return (c << 4) + cc;
}


/* copies filepart to second arg, null-terminates dirpart in place */
void DivideName(str path, str copy)
{
    strcpy(copy, FilePart(path));
    *PathPart(path) = 0;
}


void JoinName(str result, str dir, str file, str tail)
{
    short l = strlen(dir);

    strcpy(result, dir);
    result += l;
    dir = result - 1;
    if (l && *dir != '/' && *dir != ':')
	*result++ = '/';
    strcpy(result, file);
    if (tail)
	strcat(result, tail);
}


short SignatureMatches(ustr spoor, short spen, str hexig)
{
    short hits = 0;
    short i;

    for (i = 0; i < spen; i++) {
	while (*hexig == ' ') hexig++;
	if (!*hexig)
	    return hits;
	else if (*hexig == '?')
	    hexig++, hits++;
	else {
	    if (spoor[i] == HexByte(&hexig))
		hits += 100;
	    else
		return 0;
	}
    }
    return 0;
}


bool SwitchToLocalPacker(void)
{
    BPTR lk;
    short p;

    localpacker = 0;
    SetLocalSetupName();		/******* FAILS IF QWK FLAG WRONG! */
    if (lk = RLock(localfigname)) {
	if (Examine(lk, fib) && fib->fib_Comment[0])
	    for (p = 0; p < packers; p++)
		if (!stricmp(fib->fib_Comment, packernames[p])) {
		    localpacker = p + 1;
		    break;
		}
	UnLock(lk);
    }
    if (!localpacker)
	return false;
    passum[currentpacker].Flags &= ~CHECKED;
    currentpacker = localpacker - 1;
    passum[currentpacker].Flags |= CHECKED;
    return true;
}


bool PickPacker(ustr spoor, short spen)
{
    short p, r, m = -1, ml = 0;

    wrongpacker = false;
    if (SignatureMatches(spoor, spen, spoors[currentpacker]))
	return true;
    if (SwitchToLocalPacker()) {
	if (SignatureMatches(spoor, spen, spoors[currentpacker]))
	    return true;
    }
    for (p = 0; p < packers; p++)
	if ((r = SignatureMatches(spoor, spen, spoors[p])) > ml)
	    m = p, ml = r;	/* find match with the most significant bytes */
	else if (p == currentpacker && spoors[p][0])
	    wrongpacker = true;
    if (m >= 0) {
	if (m != currentpacker) {
	    r = AskDifferentPacker(m);
	    if (!r)
		return false;
	    else if (r == 2) {
		passum[currentpacker].Flags &= ~CHECKED;
		passum[currentpacker = m].Flags |= CHECKED;
	    }
	}
	wrongpacker = false;
	return true;
    }
    return true;
}


local str SkipTo(char mark, str start)
{
    while (*++start)
	if (*start == '@') {
	    if (_toupper(*++start) == mark)
		return start + 1;
	    if (!*start)		/* in case @ at very end */
		break;
	}
    return start;
}


local str Blatt(register str after, str add, str add2, short *tg)
{
    short l1 = strlen(add), len = l1 + strlen(add2);
    short quo = strchr(add, ' ') ? 2 : 0;
    /* if funckey temp files ever can have spaces, test add2 for space also */

    if ((*tg -= len + quo) >= 0) {
	if (quo) *(after++) = '"';
	strcpy(after, add);
	strcpy(after + l1, add2);
	after += len;
	if (quo) *(after++) = '"';
    }
    return after;
}


/* After must point to XCOMMANDLEN available bytes.  If un and editing are */
/* both true, this is for a function key command (which "un-edits" a msg?) */

bool Substitutions(str after, str fullname, str filesdir, str before,
		   bool editing, bool un, bool rep)
{
    str arcdir = (rep ? &uploaddir[0] : &downloaddir[0]);
    str check0, check1, chk, patch, bf = before;
    short tg = XCOMMANDLEN - 1, tcof = 0;
    ushort bfl = strlen(bf);
    char scrna[14], k, tcommand[COMMANDLEN + 1];
    bool isedit2 = before == edit2command, addslash;

    if (isedit2 || before == edit1command) {
	if (isedit2) {
	    if (strlen(editinfile) > strlen(editoutfile))
		check0 = editinfile, check1 = editoutfile;
	    else
		check0 = editoutfile, check1 = editinfile;
	} else
	    check0 = editoutfile, check1 = null;
	strcpy(tcommand, before);
	while ((patch = strnistr(bf, chk = check0, bfl)) || (check1 &&
				(patch = strnistr(bf, chk = check1, bfl)))) {
	    tcof += patch - bf;
	    bf = patch + strlen(chk);
	    tcommand[tcof++] = '@';
	    tcommand[tcof++] = chk == editinfile ? 'O' : 'F';
	    strcpy(tcommand + tcof, bf);
	    bfl = strlen(bf);
	}
	before = tcommand;
    }
    fkdoquoted = fkdoquotedX = fkdoquotedW = fkdoraw = fkdonormal = false;
    k = filesdir[strlen(filesdir) - 1];
    addslash = k != ':' && k != '/';
    while (*before && tg > 0) {
	if (*before == '@') {
	    before += 2;
	    switch (k = _toupper(before[-1])) {
	      case 'A':
		if (editing)
#ifdef FUNCTION_KEYS
		    if (un) {
			fkdoraw = true;
			fullname[0] = '@';	/* always passed as '.' */
			after = Blatt(after, editoutinst, fullname, &tg);
			fullname[0] = '.';
		    } else
#endif
			break;
		else
		    after = Blatt(after, fullname, "", &tg);
		if (tg < 0)
		    goto bombe;
		continue;
	      case 'D':
		if (editing)
		    break;
		after = Blatt(after, filesdir, (addslash ? "/" : ""), &tg);
		if (tg < 0)
		    goto bombe;
		continue;
	      case 'F':
		if (editing) {
		    if (un) {
			fkdonormal = true;
			after = Blatt(after, editoutinst, fullname, &tg);
		    } else
			after = Blatt(after, editoutinst, "", &tg);
		} else {
		    bool notfirst = false;
		    if (un) {
			Err("The @F code cannot be used\n"
					    "in decompression commands.");
			return false;
		    }
		    if (Examine(me->pr_CurrentDir, fib)) {
			bool onebad = false;
			do {
			    while (ExNext(me->pr_CurrentDir, fib)) {
				if (notfirst && tg > 0)
				    *(after++) = ' ', tg--;
				after = Blatt(after, fib->fib_FileName, "", &tg);
			    /*	if (tg < 0)
				    goto bombe; */  /* no, let gap accumulate */
				notfirst = true;
			    }
			    if (IoErr() == ERROR_NO_MORE_ENTRIES)
				goto success;
			    onebad |= IoErr() == ERROR_DEVICE_NOT_MOUNTED;
#ifdef BETA
			    DosErr("Error in @F directory scanning;\n%s scan",
				    onebad ? "abandoning" : "continuing");
#endif
			} while (onebad ^= true);
		    }
		    DosErr("Failure scanning directory while\n"
				  "expanding @F compression command code.");
		    return false;
		}
	    success:
		if (tg < 0)
		    goto bombe;
		continue;
	      case 'O':
		if (!editing)
		    break;
		if ((!isedit2 && !un) || !editinfile[0]) {
#ifdef FUNCTION_KEYS
		    Err("You cannot use the @O substitution code\n"
				"except in the second editor command or a\n"
				"function key command, and only when both\n"
				"editing filenames are defined.  Press\n"
				"%s to correct your editor settings%s",
				"Alt-E", un ? ", or\nAlt-K to correct "
				"your function keys." : ".");
#else
		    Err("You cannot use the @O substitution code\n"
				"except in the second editor command, and\n"
				"both editing filenames must be defined.\n"
				"Press %s to correct your editor settings.",
				"Alt-E");
#endif
		    return false;
		}
		after = Blatt(after, editininst, "", &tg);
		if (tg < 0)
		    goto bombe;
		continue;
	      case 'U':
		if (editing)
		    break;
		after = Blatt(after, arcdir, "", &tg);
		if (tg < 0)
		    goto bombe;
		continue;
#ifdef FUNCTION_KEYS
	      case 'W':
		fkdoquotedW = true;	/* fall through: */
	      case 'X':
		fkdoquotedX = true;	/* fall through: */
	      case 'V':
		fkdoquoted = true;
		if (!editing || !un)
		    break;
		fullname[0] = "+!&"[k - 'V'];	/* always passed as '.' */
		after = Blatt(after, editoutinst, fullname, &tg);
		fullname[0] = '.';
		if (tg < 0)
		    goto bombe;
		continue;
#endif
	      case 'P':
		after = Blatt(after, pubscrname, "", &tg);
		if (tg < 0)
		    goto bombe;
		continue;
	      case 'S':
		sprintf(scrna, "%08lX", scr);
		after = Blatt(after, scrna, "", &tg);
		if (tg < 0)
		    goto bombe;
		continue;
	      case 'Q':
	      case 'B':
		if (qwk != (k == 'Q'))
		    before = SkipTo(k, before - 1);
		continue;
	      case 'N':
		if (tg <= 0)
		    goto bombe;
		*(after++) = '\n';
		tg--;
		continue;
	      default:		/* including '@' */
		before--;
		break;
	    }
	}
	if (*after = *before)
	    before++;
	else
	    *after = '@';		/* trailing @ is not stripped */
	after++;
	tg--;
    }
    if (tg <= 0 && *before)
	goto bombe;
    *(after++) = '\n';
    *after = 0;
    return true;
bombe:
    Err("Command would be too long to execute after substitutions\n"
			"are added: probably %ld characters or more.",
			strlen(before) + XCOMMANDLEN - (long) tg);
    return false;
}


struct DosList *DosNode(ustr name)	/* Call this inside a Forbid */
{
    struct DosList *top, *ret;

#ifndef TEST13
    if (top = LockDosList(LDF_DEVICES | LDF_READ)) {
	ret = FindDosEntry(top, name, LDF_DEVICES);
	UnLockDosList(LDF_DEVICES | LDF_READ);
    } else
#else
    top = null;
#endif
	ret = null;
    return ret;
}



void TestConMan(void)
{
    struct List *libraries = &SysBase->LibList;
    static bool tested = false;

    if (!tested) {
	Forbid();
	conman = !!FindName(libraries, "conhandler.library");
	/* with ConMan at least, lib_OpenCnt may be zero while it's in use */
	if (conman) {
#ifndef TEST13
	    struct DosList *conode;
	    ushort *code;
	    if (conode = DosNode("CON")) {	/* should never fail */
		code = gbip(conode->dol_misc.dol_handler.dol_SegList + 1);
		if ((ulong) code > 1024 && *code == 0x4EF9)	/* JMP */
		    code = *(ushort **) (code + 1);	/* JMP destination */
		if (!TypeOfMem(code))			/* ROM address? */
		    conman = false;
		if (!conode->dol_misc.dol_handler.dol_Handler)
		    conman = false;	/* ConMan always has pathname */
	    }
#endif
	}
	Permit();
	tested = true;
    }
}


void HideLooseAttachedFiles(bool hide)		/* call CDed to replies dir */
{
    int i;
    char tname[32];

    tname[0] = '/';
    for (i = 0; i < replies.messct; i++) {
	struct Attachment *att = replies.messes[i]->attached;
	strcpy(tname + 1, att->tempname);
	if (att && replies.messes[i]->bluebits & UPL_INACTIVE)
	    if (hide) {
		if (Rename(att->tempname, tname))
		    attachment_hidden[i] = true;
	    } else if (attachment_hidden[i]) {
		if (!Rename(tname, att->tempname))
		    DosErr("The file %s was attached to a deleted\n"
			   "message.  It was temporarily moved to the PARENT\n"
			   "of the replies directory %s so as\n"
			   "not to include it in the upload packet.\n"
			   "But the attempt to move it back has failed.",
			   att->tempname, replydirinst);
		/* else */
		attachment_hidden[i] = false;
	    }
	else if (hide)
	    attachment_hidden[i] = false;
    }
}


ulong DS2ux(struct DateStamp *d)
{
    return d->ds_Tick / TICKS_PER_SECOND + d->ds_Minute * 60 
			+ (d->ds_Days + 2922) * 86400;
}


bool DoPack(str fullname, bool un, bool rep)
{
    str filesdir = (rep ? &replydirinst[0] : &workdirinst[0]);
    char realspec[80], k;
    const char wtitle[] = "Packer command output";
    BPTR windowhand = 0, oldcd, fcd, flk;
    APTR oldcask = me->pr_ConsoleTask, oldwptr = me->pr_WindowPtr;
    long rlen = 0, ie;
    ushort ogm;
    long ctop = 48, chi = 150;
    bool ret = false;
    char spoor[256];

    abbandonement = true;
    if (!(fcd = CreateLock(filesdir))) {
	DosErr("Couldn't find or create %s directory\n%s for %s messages.",
				dirname[rep ? DREP : DWORK], filesdir,
				un ? "unpacking" : "packing");
	return false;
    }
    if (un && !rep) {
	oldcentread = oldcentsize = 0;
	if (flk = RLock(fullname)) {
	    if (Examine(flk, fib)) {
		OneOfOurNotes(fib->fib_Comment);	/* sets oldcentread */
		oldcentsize = fib->fib_Size;
		oldcenttime = DS2ux(&fib->fib_Date);
	    }
	    UnLock(flk);
	}
    }
    if (un && (!(windowhand = OOpen(fullname))
			|| (rlen = Read(windowhand, spoor, 256)) <= 0)) {
	if (!AskUnpackAnyway(fullname)) {
	    if (windowhand)
		Close(windowhand);
	    goto borg;
	}
    }
    if (windowhand)
	Close(windowhand);
    if ((rlen && !PickPacker(spoor, rlen)) ||
			(wrongpacker && !AskNotSamePacker(currentpacker)))
	goto borg;
    if (un && !rep)
	tryunpackcd = DupLock(fcd);
    xcommand[0] = '>';
    xcommand[1] = ' ';
    if (lace) {
	ctop = 90;
	chi = 250;
    }
    oldcd = CurrentDir(fcd);	/* for new @F */
    if (!Substitutions(&xcommand[2], fullname, filesdir,
			(un ? &unpackommands[currentpacker][0]
			   : &packommands[currentpacker][0]), false, un, rep))
	goto borg2;
    CurrentDir(oldcd);
    abbandonement = false;
    ogm = FlipBGadgets(0);
    PortOff();
    NoMenus();
    MakeLotsaRoom();
    windowhand = 0;
    TestConMan();
    if (conman) {
	sprintf(realspec, "CON:S%08lX/0/%ld/%ld/%ld/%s", scr, ctop,
			    (long) nomwidth, chi, wtitle);
	if (!(windowhand = OOpen(realspec)))
	    conman = false;
    }
    if (!conman) {
	sprintf(realspec, "CON:0/%ld/%ld/%ld/%s/SCREEN %s", ctop,
			    (long) nomwidth, chi, wtitle, pubscrname);
	windowhand = OOpen(realspec);
    }
    if (!windowhand) {
	sprintf(realspec, "CON:0/20/640/150/%s", wtitle);
	windowhand = OOpen(realspec);
	ie = IoErr();
	me->pr_WindowPtr = null;
	WBenchToFront();
    }
    oldcd = CurrentDir(fcd);
    if (windowhand) {
	long retcode;

	if (!un && rep)
	    HideLooseAttachedFiles(true);
	me->pr_ConsoleTask = bip(struct FileHandle, windowhand)->fh_Type;
	Write(windowhand, xcommand, strlen(xcommand));
	retcode = ShellExecute(xcommand + 2, windowhand, 0L);
	ret = retcode != -1;
	if (!ret)
	    DosErr("Couldn't run compressor?");
	else if (retcode) {
	    Write(windowhand,
		    "\nError detected?  Press return to close window: ", 48);
	    Read(windowhand, &k, 1);
	}
	me->pr_ConsoleTask = oldcask;
	Close(windowhand);
	if (!un && rep)
	    HideLooseAttachedFiles(false);
    } else {
	SetIoErr(ie);
	DosErr("Couldn't open console window\nfor packer command output.", null);
    }
    if (!me->pr_WindowPtr) {
	me->pr_WindowPtr = oldwptr;
	ScreenToFront(scr);
	if (IntuitionBase->ActiveScreen != scr)
	    ActivateWindow(scr->FirstWindow);
    }
    YesMenus();
    PortOn();
    FlipBGadgets(ogm);
  borg2:
    CurrentDir(oldcd);
  borg:
    UnLock(fcd);
    return ret;
}


bool PackReplies(void)
{
    char fullname[400];
    bool movement = false;
    BPTR czech;
    static char flm[] = "One or more of your replies was not successfully\n"
			"saved.  An attempt was made to rewrite the %s\n"
			"file but it did not succeed.  If you can correct\n"
			"the problem, do so now, and the bad file will be\n"
			"rewritten when you select \"Pack\" again.%s";

    if (bad_upl && (DecideIfAnythingToSave(), WriteUPL(), bad_upl)) {
	if (qwk)
	    Err(flm, "message", "");
	else
	    Err(flm, "header",
			"\nOtherwise, you may wish to salvage the individual\n"
			"reply text files from your replies directory (the\n"
			"ones with numeric filenames) for reuse later.");
	return false;
    } else if (!repchanges || !anythingtosave)
	return true;
    if (!uploaddir[0]) {
	Err("You do not have an upload directory defined\n"
			"in your configuration.  Please select one now.");
	ConfigDirs();
	if (!uploaddir[0] || !replydirinst[0]) return false;	/* !@!@! */
    }
    JoinName(fullname, uploaddir, packetname, qwk ? ".REP" : ".NEW");
    if (packed_once) {
	if (!DeleteFile(fullname) && IoErr() == ERROR_DISK_WRITE_PROTECTED)
	    return false;
    } else {
	char backname[260];
	strcpy(backname, fullname);
	strcat(backname, ".old");
	if (czech = RLock(fullname)) {
	    movement = !!FileSize(czech);
	    UnLock(czech);
	}
	if (movement) {
	    if (!DeleteFile(backname) && IoErr() != ERROR_OBJECT_NOT_FOUND) {
		if (IoErr() != ERROR_DISK_WRITE_PROTECTED && !(packed_once
				    = AskOverrideNoRename(false, fullname)))
		    return false;
	    }
	    if (!packed_once && !Rename(fullname, backname)) {
		if (!AskOverrideNoRename(true, fullname))
		    return false;
	    }
	}
    }
    packed_once = true;
    if (DoPack(fullname, false, true)) {
	bool ret = false;
	if (czech = RLock(fullname)) {
	    ret = FileSize(czech) > 0;
	    UnLock(czech);
	}
	if (ret) {
	    repchanges = false;
	    TweakCloseGag();
	} else
	    Err("The upload archive file %s\nwas not successfully created.",
					fullname);
	return ret;
    } else
	return false;
}


bool Wavy(str p)
{
    ushort i;
    bool has2;
    if (fakery)
	return !stricmp(p, "INF") || !stricmp(p, "bbs-BW");
    i = (_tolower(p[0]) << 8) | _tolower(p[1]);
    has2 = isdigit(p[0]) && isdigit(p[1]);
    return (isdigit(p[2]) && (i == 'su' || i == 'mo' || i == 'tu' || i == 'we'
				|| i == 'th' || i == 'fr' || i == 'sa' || has2))
				|| (has2 && !p[2]) || (isdigit(p[0]) && !p[1]);
}


local bool ActOnFileRequest(str result, str dir, str file,
				bool saving, bool special)
{
    str p;
    short i;
    char tname[32], fullname[290];
    ulong ad = 0, au = 0;
    BPTR tcd, oldcd;
    bool ret = false;

    JoinName(fullname, dir, file, null);
    if ((tcd = RLock(fullname)) && Examine(tcd, fib)) {
	UnLock(tcd);
	if (fib->fib_EntryType > 0) {
	    Err("You've selected a directory, not a file.");
	    return false;
	}
	if (!special) {
	    if (saving) {
		if (!AskOverwriteAppend(fullname, &appendage))
		    return false;
	    } else
		ad = DS2ux(&fib->fib_Date);
	}
    }
    if (saving | special) {
	strcpy(result, fullname);
	return true;
    }
    useopenfakery = fakery;
    if (p = strchr(file + 1, '.')) {
	i = p - file;
	p++;
    } else
	p = file + (i = strlen(file));
    strncpy0(packetname, file, min(8, i));
    strupr(packetname);
    qwk = !fakery ? !Wavy(p) : stricmp(p, "bbs-BW") || stricmp(p, "INF");
    if (tcd = RLock(uploaddir)) {
	oldcd = CurrentDir(tcd);
	strcpy(tname, packetname);
	strcat(tname, (qwk ? ".REP" : ".NEW"));
	if (tcd = RLock(tname)) {
	    if (Examine(tcd, fib))
		au = DS2ux(&fib->fib_Date);
	    UnLock(tcd);
	}
	UnLock(CurrentDir(oldcd));
    }
    if (fakery)
	ret = true;
    else {
	if (needtoemptywork)
	   EmptyDir(workdirinst, true);
	ret = DoPack(fullname, true, false);
    }
    if (ret) {
	strcpy(result, fullname);
	if (fakery) {
	    strcpy(fakeryname, file);
	    strcpy(fakendir, dir);
	} else
	    strcpy(opendir, dir);
    }
    oldreplies = ret && au;
    reloadflag = (uploaddate = au) > (downloaddate = ad);
    return ret;
}


local bool fakegagup;

bool DoFakeFReqIDCMP(struct IntuiMessage *im)
{
    if (im->Class == IDCMP_RAWKEY && !(im->Qualifier & ALTKEYS)) {
	char k = KeyToAscii(im->Code, im->Qualifier);
	if (k == '\t')
	    ActivateGag(&ffrgag, ffrwin);
	else if (k == ESC)
	    return true;
    } else if (im->Class == IDCMP_CLOSEWINDOW || (im->Class == IDCMP_GADGETUP
				&& GAGFINISH(im) && ((struct Gadget *)
					im->IAddress)->GadgetID == 1000))
	return fakegagup = true;
    return false;
}


/* "special" is false ONLY for opening packets/BBSes and saving message text. */

bool DoFileRequest(bool saving, bool special, str result,
				str deflt, str patter, str hail)
{
    struct FileRequester *freq;
    BPTR lok;
    ushort ogm;
    bool arf, suck = false, atleastasked = false;
    long actionflags = 0;
    str actioname;

    if (!deflt)
	deflt = result;
    strcpy(reqdir, deflt);	/* for packets, this is a dir name! */
    if (saving | special)
	DivideName(reqdir, reqfile);
    else
	reqfile[0] = 0;
    ogm = FlipBGadgets(0);
    InitASLfake(&aslfakww, 30, 20 << lace, 42 * fontwid + 14,
					scr->Height - (50 << lace));
    if (patter)
	actionflags |= FRF_DOPATTERNS;
    if (saving) {
	actionflags |= FRF_DOSAVEMODE;	/* no ASLFR_DoSaveMode tag in V37 */
	actioname = "SAVE";
    } else if (special)
	actioname = "LOAD";
    else
	actioname = "OPEN";
    NoMenus();
    if (AslBase = OpenL("asl")) {
	if (freq = AllocAslRequestTags(ASL_FileRequest, ASLFR_TitleText, hail,
			ASLFR_InitialLeftEdge, (long) aslfakww.left,
			ASLFR_InitialTopEdge, (long) aslfakww.top,
			ASLFR_InitialWidth, (long) aslfakww.width,
			ASLFR_InitialHeight, (long) aslfakww.height,
			ASLFR_Window, bgwin, ASLFR_Flags1, actionflags,
			ASLFR_InitialDrawer, reqdir, ASLFR_InitialFile, reqfile,
			ASLFR_PositiveText, actioname, ASLFR_RejectIcons, 1L,
			(patter ? ASLFR_InitialPattern : TAG_DONE), patter,
			TAG_DONE)) {
	    short odpen = scr->DetailPen, obpen = scr->BlockPen;
	    ustr obgwt = bgwin->ScreenTitle;
	    SetAllSharedWindowTitles(title);
	    scr->DetailPen = 0;
	    scr->BlockPen = 1;
	    atleastasked = true;
	    arf = AslRequest(freq, (adr) &endtag);
	    /* AARGH -- there's no way to tell if requester failed to open!! */
	    scr->DetailPen = odpen;
	    scr->BlockPen = obpen;
	    SetAllSharedWindowTitles(obgwt);
	    UpdateClock();
	    if (arf && freq->fr_File[0])
		suck = ActOnFileRequest(result, freq->fr_Drawer,
						freq->fr_File, saving, special);
	    aslfakww.left = freq->fr_LeftEdge;
	    aslfakww.top = freq->fr_TopEdge;
	    aslfakww.width = freq->fr_Width;
	    aslfakww.height = freq->fr_Height;
	    FreeAslRequest(freq);
	} else
	    Err("Could not allocate ASL\nrequester.  No memory?");
	CloseLibrary(AslBase);
	AslBase = null;
    } else {
	StopScroll();
	JoinName(ffrname, reqdir, reqfile, null);
	ffrgag.TopEdge = fight + 12;
	ffrneww.Height = 4 * fight + 29;
	if (ffrwin = OpenBlueGagWin(&ffrww, &ffrgag)) {
	    register struct RastPort *ffrrp = ffrwin->RPort;
	    short labelbase = tifight + fight + font->tf_Baseline + 19;
	    atleastasked = true;
	    SetAPen(ffrrp, LABELCOLOR);
	    SetBPen(ffrrp, backcolor);
	    MoveNominal(ffrrp, 20, labelbase);
	    Text(ffrrp, "(If you want a real file requester, put a", 41);
	    MoveNominal(ffrrp, 20, labelbase + fight + 1);
	    Text(ffrrp, "copy of asl.library in your LIBS: directory.)", 45);
	    ActivateGag(&ffrgag, ffrwin);
	    EventLoop(&DoFakeFReqIDCMP);
	    CloseBlueGagWin(ffrwin);
	    ffrwin = null;
	    if (fakegagup && ffrname[0]) {
		lok = 0;
		if (saving || (lok = RLock(ffrname))) {
		    DivideName(ffrname, reqfile);
		    if (lok) UnLock(lok);
		    suck = ActOnFileRequest(result, ffrname, reqfile,
							saving, special);
		} else
		    DosErr("Could not find any file\nnamed %s", ffrname);
	    }
	}
    }
    if (!atleastasked)
	Err("Could not open file requester.");
    FlipBGadgets(ogm);
    YesMenus();
    return suck;
}
