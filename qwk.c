/* Q-Blue functions for loading and saving QWK format files */

#include <exec/memory.h>
#include <dos/dosextens.h>
#include "qblue.h"


#define NAMELESS_NAME " (nameless area"
#define NAMELESS_TAIL ", #%lu)"
#define NAMELESS_NLEN 15   /* strlen(NAMELESS_NAME) */


bool LoadMessage(struct Mess *mm, BPTR hand, short qeol, bool trim);
void DateString(struct DateStamp *d, str s);
void FlushPacket(void);
bool MakeReadAreaz(void);
bool DoPack(str fullname, bool un, bool rep);
void ReadBBSlocalSetup(bool askfile);
void SeekErr(str what);
void ReadWriteErr(str action, str file, ushort dir);
bool CopyFile(BPTR inh, str outpath);		/* returns false for success! */
bool SafeExamineFH(BPTR hand, struct FileInfoBlock *fib, str flename);

ulong Text2ux(str date);
bool CreateNewArea(str cnum, str shortname, str longname, ushort *initialarea);
short HexByte(str *try);
void JoinName(str result, str dir, str file, str tail);
bool TwitTest(struct Mess *mm);
void ReadOrigin(struct Mess *mm);
bool NeedToCheckOrigins(void);
struct Mess *NewPoolMess(void);
void ReturnToPool(struct Mess *mm);
struct Conf *NewPoolConf(void);
ustr NewPoolString(ushort length);
ustr RetrieveReferences(ustr *groups);

void Personalize(struct Mess *mm);
void PickUpTheTrash(void);
void ScanForTrash(void);
void Bogotify(short lastbogon);
void FromTo(struct Mess *mm);
void EmptinessGripe(void);
void CreateBBSFile(bool artificial);
void UsePlaceholder(void);
ushort Chex(struct Mess *mm);
short Conf2ix(struct Conf *cc);
bool ReplyChoke(ushort current, bool reloading);
void UnMask(str s);
bool ValidInternet(str s);
bool CheckAAAextra(struct Conf *cc, str command, ushort add, ushort reset);
str GetExtraDoorCommand(struct Conf *cc);


typedef struct {
    char qstatus;	/* one of: space - + * ~ ` % ^ ! # $ */
    char qmessnum[7];	/* decimal - NOT nul terminated */
    char qday[8];	/* mm-dd-yy */
    char qtime[5];	/* hh:mm */
    char qto[25];	/* not nul terminated */
    char qfrom[25];	/* nor this or the next few */
    char qsubject[25];	/* kinda on the short side, aint it? */
    char qpassword[12];	/* ignored by most bbses */
    char qreplyto[8];	/* decimal */
    char qblox[6];	/* # of 128-byte blocks used by msg, including this */
    ubyte qkilt;	/* value is either 0xE1 or 0xE2 (go thou and figure) */
    tWORD qconfnum;	/* Intel order -- high (2nd) byte sometimes a space */
    tWORD qordinal;	/* typically not used */
    ubyte qtagflag;	/* typically not used; ' ' for uploads */
    ubyte qkluge[72];	/* offset 128: kluge line at top of message text */
} qwk_rec;


import char CONTNAME[], sysopname[], bbsname[], iedummyto[];
import char teargline[], qnetkluge[], thefilenote[];
import ustr *messageID, *newsgroups;
import str fibuf, fibupt;

import struct trash *trashfiles;

import ulong megachexum;
import short netmailareanum, unetareanum, localmailareanum;
import ushort hostzone, hostnet, hostnode, hostpoint, hostnetmailflags;
import ushort requestedfiles_limit, lastsorder;

import bool mylogin26, myother26, any26, useopenfakery, ie_is_gated;
import bool twits, addsdrops, stealthiness, over32k, pcboard_net, postlinkage;


char bbscity[65], bbsphone[65], qwkid[32], qobuf[128], qofame[32];
char doorconn[26];
#ifdef HECTOR_HACK
/* const */ char XPRESSNAME[] = "XPAREAS.NDX"; /* !! Hector changed the name! */
local bool copied_xpress = false
#endif

local short c128;
local long sticktotal;
local bool donotstick = false;

ushort downernames, downertitles, totalmesses, disorderly, areazroom;
ushort orig_subjectlen;
bool uppitynames, uppitytitle, hexig, galactiwarn, wouldahaddrop;
bool zerosizefile, lookslikeqwk, replylike, qwkreplike, iemail_mksubj;
bool allow_qpassword, dooraddrop, doorreset, pcbkluge, inverse_netkluge;
bool pcboard_ie, qwkE, qwkE_subs, searchlight, searchlight_ie, pcboard_hood;
ulong hiqwkarea;

BHandle qohand;


bool MKQwkStyle(str doorname)
{
    bool olms = !stricmp(doorname, "OLMS"), val = !stricmp(doorname, "VALENCE");
    bool true_mkqwk = !stricmp(doorname, "MKQWK");
    bool mklike = true_mkqwk || !stricmp(doorname, "JC-QWK");

    if (val)
	subjectlen = 40;	/* the other SL door does not support this */
    else if (olms)
	subjectlen = SUBJLEN - 1;
    qwkE_subs = olms | val;
    /* OLMS and Valence use "Subject:", MKQWK and JC-QWK use "Subj:" */
    if (true_mkqwk)
	hostnetmailflags = INF_CAN_CRASH | INF_CAN_DIRECT | INF_CAN_IMM;
    iemail_mksubj = mklike && !true_mkqwk;
    orig_subjectlen = subjectlen;
    return olms | mklike;
}


void cpy25(str dest, str src)
{
    Stranks(strncpy0(dest, src, 25));
}


void ParseDoorId(void)
{
    ubyte line[90];
    ustr text, split;
    bool add = false, drop = false;
    BHandle hand;

    strcpy(doorconn, "QMAIL");
    dooraddrop = doorreset = inverse_netkluge = false;
    if (!(hand = BOpen("DOOR.ID", false)))
	return;
    wouldahaddrop = true;
    while (BGetline(hand, line, 89) >= 0) {
	if (!(split = strchr(line, '=')))
	    /* handle keywords without "=" here, like TBBS/TomCat "RECEIPT" */
	    continue;
	for (text = split + 1; isspace(*text); text++) ;
	while (split >= line && isspace(*--split))
	    *split = 0;
	if (!line[0])
	    continue;
	split = strend(text);
	while (split >= text && isspace(*--split))
	    *split = 0;
	if (!*text)
	    continue;
	/* We now have a valid keyword/text pair. */
	if (!stricmp(line, "SYSTEM")) {
	    if (!strnicmp(text, "PCBoard", 7)) {
		for (split = text + 7; *split && !isdigit(*split); split++) ;
		if (isdigit(split[1]) && (*split > '1' || split[1] >= '5'))
		    pcboard_hood = pcbkluge = true;
		allow_qpassword = true;
	    } else if (!strnicmp(text, "Searchlight", 11))
		searchlight = searchlight_ie = true;
	} else if (!stricmp(line, "CONTROLNAME")) {
	    strncpy0(doorconn, text, 25);
	    inverse_netkluge = MKQwkStyle(doorconn);
	} else if (!stricmp(line, "CONTROLTYPE")) {
	    str t = strchr(text, ' ');
	    if (t) *t = 0;
	    if (!stricmp(text, "ADD"))
		add = true;
	    else if (!stricmp(text, "DROP"))
		drop = true;
	    else if (!stricmp(text, "RESET"))
		doorreset = true;
	}
    }
    BClose(hand);
    dooraddrop = add | drop;
}


struct Conf *MakeAFake(str confnum)
{
    char fakename[60];
    ushort arix;

    sprintf(fakename, " (area %s, not included in packet)", confnum);
    if (CreateNewArea(confnum, "", fakename, &arix))
	return areaz.confs[arix];
    else
	return null;
}


ulong msr2i(ustr msr)
{
    ulong mant = (i4(msr) & 0x00FFFFFF) | 0x00800000;
    ulong expo = msr[3] & 0x7F;
    return mant >> (24 - expo);
}


/* mlist points to Messes chained by their unixdate fields, with Conf */
/* pointers stuck in their personalink fields, which need correcting. */

local void CorrectAreasFromNDXs(struct Mess *mlist)
{
    str fn, fp;
    ushort a, xi;
    char anum[10];
    register struct Conf *cc;
    register struct Mess *mm, *lastmm = mlist;
    long dseek;
    bool badwarn = false;

    if (!Examine(me->pr_CurrentDir, fib))
	goto bail;
    while (ExNext(me->pr_CurrentDir, fib)) {
	fn = &fib->fib_FileName[0];
	for (a = 0, fp = fn; isdigit(*fp); fp++)
	    a = 10 * a + *fp - '0';
	if (fp == fn || stricmp(fp, ".NDX"))
	    continue;
	if (!InhaleFile(fn))
	    goto bail;
	utoa(a, anum);
	if (!(cc = Confind(anum)) && !(cc = MakeAFake(anum)))
	    goto bail;
	for (xi = 0; xi + 4 < fibize; xi += 5) {
	    dseek = msr2i(fibuf + xi) * 128 - 1;
	    mm = (adr) lastmm->unixdate;
	    if (!mm || mm->datfseek != dseek) {
		for (mm = mlist; mm; mm = (adr) mm->unixdate)
		    if (mm->datfseek == dseek)
			break;
		if (!mm) {
		    badwarn = true;
		    continue;
		}
	    }
	    lastmm = mm;
	    strcpy(mm->confnum, anum);
	    if (mm->personalink)
		if (((struct Conf *) mm->personalink)->messct < 65535)
		    ((struct Conf *) mm->personalink)->messct--;
	    mm->personalink = (adr) cc;
	    if (cc->messct < 65535)
		cc->messct++;
	}
    }
    over32k = false;
    for (a = 0; a < areaz.messct; a++) {
	cc = areaz.confs[a];
	if (cc->messct >= 32767) {
	    /* Recount from scratch: */
	    for (a = 0; a < areaz.messct; a++)
		areaz.confs[a]->messct = 0;
	    for (mm = mlist; mm; mm = (adr) mm->unixdate)
		if ((cc = (adr) mm->personalink) && cc->messct < 32767)
		    if (++cc->messct == 32767)
			over32k = true;
	    break;
	}
    }
    if (!badwarn) {
	ExhaleFile();
	return;
    }
  bail:
    ExhaleFile();
    Err("Some messages may be in the wrong areas, because this packet\nhas"
			" dubious area information in the messages which"
			" ought\nto be supplemented by the data in the .NDX"
			" files, and\n%s.", badwarn
			? "at least one .NDX file contained invalid information"
			: "lack of memory has prevented processing them all");
}


void SoFarize(void)	/* make freeing of unfinished packet load safe */
{
    short i;
    struct Conf *cc;
    for (i = 0; i < areaz.messct; i++)
	if ((cc = areaz.confs[i]) && cc->sofar < cc->messct)
	    cc->messct = cc->sofar;
}


local bool EatAddDropMsg(qwk_rec *qead, long *areas, bool *real)
{
    struct Conf *cc;
    ubyte dittany[26];
    str dp;

    if (qead->qblox[0] == '1' && qead->qblox[1] == ' '
		    && qead->qstatus == '*' && qead->qreplyto[0] == ' '
		    && !strnicmp(qead->qto, doorconn, strlen(doorconn))) {
	*areas += 128;
	strncpy0(dittany, qead->qmessnum, 7);
	Stranks(dittany);
	if (!(cc = Confind(dittany)) && !(cc = MakeAFake(dittany))) {
	    long z = atol(dittany);
	    cpy25(dittany, qead->qsubject);
	    Err("Found mail door control message for area not\n"
			"included in mail packet, and could not create\n"
			"area: %s in area %ld.", dittany, z);
	    return false;
	}
	cpy25(dittany, qead->qsubject);
	if (!stricmp(dittany, "DROP")) {
	    cc->morebits &= ~DOOR_AREAPICKS;
	    cc->morebits |= DOOR_DROPPING;
	    CheckAAAextra(cc, "", false, false);	/* for consistency */
	    return addsdrops = true;
	}
	if (!strnicmp(dittany, "ADD", 3) && dittany[3] <= ' ') {
	    cc->morebits |= DOOR_ADDING;
	    cc->morebits &= ~DOOR_DROPPING;
	    for (dp = dittany + 3; isspace(*dp); dp++) ;
	    if (*dp)
		CheckAAAextra(cc, dp, true, false);
	    return addsdrops = true;
	}
	if (!strnicmp(dittany, "RESET", 5) && dittany[5] <= ' ') {
	    cc->morebits |= DOOR_RESETTING;
	    for (dp = dittany + 5; isspace(*dp); dp++) ;
	    if (*dp)
		CheckAAAextra(cc, dp, false, true);
	    return addsdrops = true;
	}
	for (dp = dittany; isspace(*dp); dp++) ;
	if (*dp) {
	    cc->morebits |= DOOR_ADDING_YOURS;
	    CheckAAAextra(cc, dp, false, false);
	    return addsdrops = true;
	}
    }
    return *real = true;
}


local void AccomodateQWKreply(struct Conf *cc, struct Mess *mm,
				short *lastbogon, str qreplyto)
{
    ushort aa;
    if (!replies.messct)
	readareaz.messct++;
    if (!messageID[replies.messct])
	messageID[replies.messct] =
			RetrieveReferences(&newsgroups[replies.messct]);
    replies.messes[mm->replyat = replies.messct++] = mm;
    if (cc && qreplyto[0] != ' ' && mm->replyto) {
	for (aa = 0; aa < cc->messct; aa++)
	    if (cc->messes[aa]->ixinbase == mm->replyto)
		break;
	if (aa < cc->messct)
	    if (Text2ux(cc->messes[aa]->date) < Text2ux(mm->date)) {
		mm->mreplyee = cc->messes[aa];
		mm->mreplyee->mreplyee = mm;
		mm->bits |= MEREPLIED;
		mm->mreplyee->bits |= MEREPLIED /* | MESEEN */ ;
		if (mm->mreplyee->personalink) {
		    mm->mreplyee->personalink->mreplyee = mm;
		    mm->mreplyee->personalink->bits |= MEREPLIED;
		}
	    } else
		*lastbogon = replies.messct;
    }
    if (cc->sofar < mm->ixinbase)
	cc->sofar = mm->ixinbase;
    if (cc->morebits & INTERNET_EMAIL && ValidInternet(mm->too))
	mm->bits |= EMAIL_REPLY;
}


local bool StickMessagesInConfs(struct Mess *messlist)
{
    ushort a;
    register struct Conf *cc;
    register struct Mess *mm;

    for (a = 0; a < areaz.messct; a++) {
	cc = areaz.confs[a];
	if (cc->messct > 32767) {
	    cc->messct = 32767;
	    over32k = true;
	}
    }
    for (a = areaz.messct - 1; a >= 0; a--) {
	cc = areaz.confs[a];
	if (!(cc->morebits & ALLREAD))		/* no more fake areas */
	    break;
	if (!cc->messct && !(cc->morebits & DOOR_AREAPICKS))
	    memmove(areaz.confs + a, areaz.confs + a + 1,
					(--areaz.messct - a) << 2);
    }
    while (mm = messlist) {
	messlist = (adr) messlist->unixdate;
	if (!(cc = (adr) mm->personalink)) {
	    Err("Can't collect messages; area\n"
			    "numbers in packet are invalid.");
	    return false;
	}
	if (!cc->messes && !(cc->messes = Valloc(cc->messct << 2))) {
	    Err("Can't collect messages; no memory.");
	    return false;
	}
	if (cc->sofar >= cc->messct) {  /* have we room?  If not: */
	    if (mm->attached)
		FREE(mm->attached);
	    ReturnToPool(mm);           /* reuse memory. Should announce error? */
	} else {
	    if (cc->sofar && mm->ixinbase < cc->messes[cc->sofar - 1]->ixinbase)
		disorderly++;
	    cc->messes[cc->sofar++] = mm;
	    mm->personalink = null;
	    mm->unixdate = 0;
	    Personalize(mm);
	    if (mm->bits & TOME)
		cc->tomect++;
	}
    }
    return true;
}


bool LoadQWKmess(BPTR hand, bool replypak)
{
    char dittany[52], tsub[130], c;
    bool suck = false, skippity, scanorigins, twits = false, tqls;
    bool real = !replypak, havereadorigin;
    struct Conf *cc;
    struct Mess *mm, *messlist = null, **messtail = &messlist;
    qwk_rec qead;
    long msgstart;
    short len, a, aa, aaa, lastbogon = 0;

    scanorigins = !replypak && NeedToCheckOrigins();
    msgstart = 128;
    for (;;) {
	if (SSeek(hand, msgstart, OFFSET_BEGINNING) < 0) {
	    SeekErr(replypak ? "reading reply packet" : "reading MESSAGES.DAT");
	    return false;
	}
	if (!(len = RRead(hand, &qead, sizeof(qwk_rec))))
	    break;
	if (len == 128)
	    qead.qkluge[0] = 0, len = sizeof(qwk_rec);
	if (len < sizeof(qwk_rec)) {
	    if (replypak)
		ReadWriteErr(null, "reply file", DREP);
	    else
		ReadWriteErr(null, "MESSAGES.DAT", DWORK);
	    return false;
	}
	if (!real && !EatAddDropMsg(&qead, &msgstart, &real))
	    return false;
	if (!real)		/* was it a control message? */
	    continue;
	if (replypak && ReplyChoke(replies.messct, true))
	    break;
	/* Deal with doors that pad the packet with 0x00 and 0xFF bytes... */
	if ( /* qead.qblox[0] == qead.qblox[5] && */
			(!qead.qblox[0] || (ubyte) qead.qblox[0] == 0xFF)) {
	    msgstart += 128;
	    continue;
	}
	if (!(replypak ? NEWZ(mm) : (mm = NewPoolMess()))) {
	    Err("Can't read messages -- no memory.");
	    return false;
	}
	c = qead.qstatus;
	if (c == '+' || c == '*')
	    mm->bluebits |= UPL_PRIVATE;
	if (!replypak && (c == '-' || c == '*'
				|| c == '`' || c == '^' || c == '#'))
	    mm->bits |= SEENINBASE;
	if (qead.qkilt == 0xE2 && replypak)
	    mm->bluebits |= UPL_INACTIVE;	/* user can undelete */
	strncpy0(dittany, qead.qmessnum, 7);
	if (replypak) {
	    mm->bits |= DONTSTRIP;
	    mm->ixinbase = replies.messct;
	    a = atol(dittany);
	    utoa(a, mm->confnum);
	    cc = Confind(mm->confnum);
	} else {
	    mm->ixinbase = atol(dittany);
	    a = i2(qead.qconfnum);
	    if (hiqwkarea < (ushort) a) {
		a = (ubyte) qead.qconfnum[0];	/* low order byte */
		galactiwarn = true;
	    }
	    utoa(a, mm->confnum);
	    cc = Confind(mm->confnum);
	    if (!cc || !strncmp(LONGNAME(cc), NAMELESS_NAME, NAMELESS_NLEN))
		galactiwarn = true;
	}
	strncpy(mm->date, qead.qday, 8);
	mm->date[8] = ' ';
	strncpy0(&mm->date[9], qead.qtime, 5);
	if ((ubyte) mm->date[5] == 0xC4)	/* PCBoard bogosity */
	    mm->date[5] = '-';
	strncpy0(dittany, qead.qreplyto, 8);
	mm->replyto = atol(dittany);
	strncpy0(dittany, qead.qblox, 6);
	a = atol(dittany);
	if (a <= 1) {
	    /* Some error report if 0 or less?  Nah, cascade errors likely */
	    a = 1;
	    qead.qkluge[0] = 0;
	}
	mm->datflen = (a - 1) << 7;
	mm->datfseek = msgstart + 127;
	msgstart += a << 7;
	skippity = tqls = false;
	while (qead.qkluge[0] == 0xFF && qead.qkluge[1] == '@'
			&& isupper(qead.qkluge[2]) && qead.qkluge[9] == ':'
			&& isupper(qead.qkluge[70]) && qead.qkluge[71] == QEOL
			&& strncmp(qead.qkluge + 2, "SUBJECT", 7)) {
	    if (RRead(hand, qead.qkluge, 72) < 72) {
		qead.qkluge[0] = 0;
		break;
	    }
	    skippity = true;
	}
	havereadorigin = false;
	if (!strncmp(qead.qkluge, "\xFF@SUBJECT:", 10)) {
	    strncpy0(tsub, qead.qkluge + 10, 60);
	    if (!strchr(tsub, QEOL)) {
		Stranks(tsub);
		ReadOrigin(mm);
		havereadorigin = true;
		pcbkluge |= tqls = !mm->net && !postlinkage;
		if (!skippity) {
		    mm->datfseek += 72;
		    mm->datflen -= 72;
		}
	    } else
		cpy25(tsub, qead.qsubject);
	} else
	    cpy25(tsub, qead.qsubject);
	cpy25(dittany, qead.qfrom);
	cpy25(dittany + 26, qead.qto);
	a = strlen(tsub);
	aa = strlen(dittany);
	aaa = strlen(dittany + 26);
	if (replypak) {
	    mm->subject = BValloc(a + 1);
	    mm->from = BValloc(aa + 1);
	    mm->too = BValloc(aaa + 1);
	} else {
	    mm->subject = NewPoolString(a);
	    mm->from = NewPoolString(aa);
	    mm->too = NewPoolString(aaa);
	}
	if (!mm->from || !mm->too || !mm->subject) {
	    if (replypak)
		FreeReply(mm);
	    Err("Can't load message header\ninformation -- no memory.");
	    return false;
	}
	strcpy(mm->from, dittany);
	strcpy(mm->too, dittany + 26);
	strcpy(mm->subject, tsub);
	if (replypak) {
	    if (!LoadMessage(mm, hand, 1, false)) {
		FreeReply(mm);
		Err("Could not load text\nof reply #%lu.", replies.messct + 1L);
		return false;
	    }
	} else {
	    FromTo(mm);
	    megachexum = ((megachexum << 3) | (megachexum >> 29)) ^ Chex(mm);
	    if (scanorigins && !havereadorigin)
		ReadOrigin(mm);
	    if (TwitTest(mm)) {
		twits = true;
		if (mm->attached)
		    FREE(mm->attached);
		ReturnToPool(mm);
		continue;
	    }
	}
	if (!cc) {
	    if (!(cc = MakeAFake(mm->confnum)))
		cc = areaz.confs[0];
	    galactiwarn = true;
	    strncpy(mm->confnum, cc->confnum, 5);
	} else if (tqls)
	    cc->morebits |= HASLONGSUBKLUGE;	/* not used any more... */
	if (replypak)
	    AccomodateQWKreply(cc, mm, &lastbogon, qead.qreplyto);
	else {
	    if (cc->messct < 65535)
		cc->messct++;
	    if (cc->messct > 32767)
		galactiwarn = true;
	    *messtail = (adr) mm;
	    messtail = (adr) &mm->unixdate;
	    mm->personalink = (adr) cc;
	}
    }
    if (!replypak) {
	*messtail = null;		/* paranoia */
	if (galactiwarn)
	    CorrectAreasFromNDXs(messlist);
	if (!StickMessagesInConfs(messlist))
	    return false;
	if (over32k)
	    Err("More than 32767 messages were found in one area.\n"
			"Messages beyond that point cannot be displayed.");
    }
    Bogotify(lastbogon);
    return true;
}


local void AttachAttachments(void)
{
    BHandle hand;
    BPTR knuckle;
    char buf[256], biff[16];
    str mnum, iname, ename;
    struct Conf *cc;
    struct Mess *mm;
    ulong an;
    short i;

    if (!(hand = BOpen("ATTACHED.LST", false)))
	return;
    while (BGetline(hand, buf, 256) >= 0) {
	if (!(mnum = strchr(buf, ',')))
	    continue;
	*mnum++= 0;
	if (!(iname = strchr(mnum, ',')))
	    continue;
	*iname++ =0;
	if (!(ename = strchr(iname, ',')))
	    continue;
	*ename++ = 0;
	an = atol(buf);
	sprintf(biff, "%lu", an);
	if (!(cc = Confind(biff)) || !cc->messes)
	    continue;
	an = atol(mnum);
	mm = null;
	for (i = 0; i < cc->messct; i++)
	    if (cc->messes[i]->ixinbase == an) {
		mm = cc->messes[i];
		break;
	    }
	if (!mm)
	    continue;
	Stranks(iname);
	Stranks(ename);
	if (knuckle = RLock(iname)) {
	    UnLock(knuckle);
	    if (NEW(mm->attached)) {
		strcpy(mm->attached->tempname, iname);
		strcpy(mm->attached->arrivename, ename);
		mm->attached->localpath[0] = '\0';
		mm->attached->localsource = false;
	    } /* else fail silently */
	}
    }
    BClose(hand);
}


bool LoadQuacket(str controlname)
{
    BHandle chand;
    struct Conf *cc;
    char cretin[130], f[2], clongname[LONCOLEN];
    bool suck = false;
    long areas;
    short a;
    ulong anum;
    ustr p, ap;

    PortOff();
    uppitynames = uppitytitle = dooraddrop = wouldahaddrop = addsdrops = false;
    pcboard_hood = pcbkluge = qwkE = qwkE_subs = galactiwarn = false;
    searchlight = searchlight_ie = iemail_mksubj = false;
    downernames = downertitles = disorderly = totalmesses = hiqwkarea = 0;
    fromtolen = subjectlen = orig_subjectlen = 25;
    requestedfiles_limit = 0;
    if (!(chand = BOpen(controlname, false))
				|| BGetline(chand, bbsname, 64) <= 0) {
	anum = IoErr();
	ReadWriteErr(IoErr() == ERROR_NO_FREE_STORE ? "allocate memory to read"
				: null, controlname, fakery ?
				(useopenfakery ? DASL : DBBS) : DWORK);
	goto vlorf;
    }
    if (bbsname[0] >= 2 && bbsname[0] <= 8) {	/* actually a BW *.INF file?? */
	strncpy0(cretin, bbsname + 1, 31);
	if (strlen(cretin) <= 12) {
	    qwk = false;			/* yes!  probably. */
	    goto vlorf;
	}
    }
    BGetline(chand, bbscity, 64);
    BGetline(chand, bbsphone, 64);
    BGetline(chand, sysopname, 41);
    if ((p = strrchr(sysopname, ',')))
	*p = 0;
    BGetline(chand, qwkid, 31);
    strupr(qwkid);
    if ((p = strchr(qwkid, ','))) {
	while (p[1] == ' ') p++;
	strcpy(qwkid, p + 1);
	a = strlen(qwkid);
    } else
	qwkid[a = 0] = 0;
    if (a <= 0 || a > 8) {
	Err("Invalid packet name in file %s.",  controlname);
	goto vlorf;
    }
    strcpy(packetname, qwkid);
    BGetline(chand, f, 0);		/* creation date */
    if (BGetline(chand, myothername, 25) <= 0) {
	Err("Unreadable user name in file %s.", controlname);
	goto vlorf;
    }
    Stranks(myothername);
    strcpy(myloginame, myothername);
    any26 = myother26 = mylogin26 = false;
    BGetline(chand, f, 0);
#ifndef LINE_9_NET_AREA
    BGetline(chand, f, 0);
#else
    BGetline(chand, cretin, 31);
/* line 9 = netmail area number with DCQWK door under T.A.G. */
/* supported by OFFLINE? apparently not ... supported by GRAYQWK for QuickBBS */
    Stranks(cretin);
    if (cc = Confind(cretin)) {        /* can't use; don't know address kluge */
	if ((netmailareanum = Conf2ix(cc)) >= 0)
	    areaz.confs[netmailareanum]->areabits |= INF_NETMAIL;
    } else
#endif
	netmailareanum = -1;
    hostzone = hostnet = hostnode = hostpoint = hostnetmailflags = 0;
    localmailareanum = 0;
    BGetline(chand, f, 0);
    BGetline(chand, cretin, 31);
    areas = atol(cretin);
    if ((areas || cretin[0] == '0') && areas >= 0 && areas < 32744)
	areas++;
    else {
	Err("File %s has an invalid\nvalue for the number of message\nareas, "
				"or more than 32000 areas.", controlname);
	goto vlorf;
    }
    if (!(areaz.confs = Valloc((areazroom = areas + 20) << 2))) {
	Err("No memory for reading\nlist of message areas.");
	goto vlorf;
    }
    for (a = 0; a < areas; a++) {
	BGetline(chand, cretin, 31);
	for (ap = cretin; *ap > ' ' && !isdigit(*ap); ap++)	/* paranoia */
	    galactiwarn = true;
	if ((anum = atol(ap)) > hiqwkarea)	/* may have space padding */
	    hiqwkarea = anum;
	if (anum > 9999999 /* strlen(cretin) > 5 */ ) {
	    Err("Invalid area list in %s; area\nnumber has more than"
					" seven digits.", controlname);
	    goto vlorf;
	}
	if (anum > 32767)	/* could use 65535, but let's be overcautious */
	    galactiwarn = true;
	BGetline(chand, clongname, LONCOLEN - 1);
	if (!cretin[0])
	    break;				/* continue with what we have */
	if (!clongname[0])
	    sprintf(clongname, NAMELESS_NAME NAMELESS_TAIL, (ulong) anum);
	if (!(cc = NewPoolConf())) {
	    Err("Ran out of memory while\nreading file %s", controlname);
	    goto vlorf;
	}
	if (strlen(clongname) >= SHOCOLEN) {
	    if (cc->longname = NewPoolString(strlen(clongname)))
		strcpy(cc->longname, clongname);
	    else
		strncpy0(cc->shortname, clongname, SHOCOLEN - 1);
	} else
	    strcpy(cc->shortname, clongname);
/*	for (p = &cretin[0]; *p == ' '; p++) ;  */
/*	strcpy(cc->confnum, p);			*/
	sprintf(cc->confnum, "%lu", anum);
	cc->areabits = INF_ANY_NAME | INF_POST | INF_HASFILE;
	areaz.confs[areaz.messct++] = cc;
    }
    BGetline(chand, trashfiles[0].n, 12);		/* welcome */
    BGetline(chand, trashfiles[1].n, 12);		/* news */
    BGetline(chand, trashfiles[2].n, 12);		/* goodbye */
    if (SafeExamineFH(DOSHANDLE(chand), fib, controlname))
	strcpy(thefilenote, fib->fib_Comment);
    else
	thefilenote[0] = 0;
    BClose(chand);
    chand = null;
    if (fakery) {
	str fnc = strchr(thefilenote, '>'), dnc = strchr(thefilenote, '/');
	char c;
	/* additional information can be included before the '>'. */
	if (dnc) {
	    *dnc = 0;				/* null-terminate fnc */
	    strcpy(doorconn, dnc + 1);
	    inverse_netkluge = MKQwkStyle(doorconn);
	} else
	    doorconn[0] = 0;
	if (fnc)
	    while (c = *++fnc) {		/* skip leading '>' */
		if (c == 'H')
		    wouldahaddrop = true;
		if (c == 'M')
		    dooraddrop = true;
		if (c == 'R')
		    doorreset = true;
		if (c == 'U')
		    uppitynames = true;
		if (c == 'L')
		    pcboard_hood = pcbkluge = true;
		if (c == 'E')
		    qwkE = true;
		if (c == 'Z')
		    qwkE_subs = true;	/* qwkE may be false */
		if (c == 'S')
		    searchlight = searchlight_ie = true;
	    }
	/* new flags can be added after HMULEZS and before /DOORNAME */
	UsePlaceholder();
	ReadBBSlocalSetup(false);
	if (pcbkluge)
	    subjectlen = 60;
	PortOn();
	return suck = true;
    }
    ASSERT(!fakery);
    ParseDoorId();
    if (!(texthand = OOpen("MESSAGES.DAT"))) {
	ReadWriteErr(null, "MESSAGES.DAT", DWORK);
	goto vlorf;
    }
    megachexum = 0;
    if (!wouldahaddrop && !pcbkluge && RRead(texthand, cretin, 128) == 128) {
	str s = strnistr(cretin, "PCBoard", 128);
	if (s) {
	    cretin[128] = 0;
	    s += 8;
	    if (_toupper(*s) == 'V') s++;
	    pcbkluge = atol(s) >= 15;
	    pcboard_hood = true;
	}
    }		/* this detects PCBoard 15's built-in QWK packer */
    /* HERE because it needs to know pcboard_hood and door.id stuff: */
    ReadBBSlocalSetup(false);
    /* and LoadQWKmess needs the local settings. */
    if (!(suck = LoadQWKmess(texthand, false))) {
	SoFarize();
	goto vlorf;
    }
    if (pcbkluge)
	subjectlen = 60;
    uppitynames = downernames <= (totalmesses >> 4);
/************
    uppitytitle = downertitles <= (totalmesses >> 4);
************/
    if (disorderly > (totalmesses >> 4))
	lastsorder = 9999;
    if (!MakeReadAreaz())		/**** SHOULD SET SUCK FALSE? */
	goto vlorf;
    AttachAttachments();
    ScanForTrash();
    PickUpTheTrash();
    CreateBBSFile(false);
    if (!readareaz.messct || (readareaz.messct == 1 && bullstuff.messct)) {
	EmptinessGripe();
	if (readareaz.messct)
	    readareaz.current = 0;
	else
	    UsePlaceholder();
    }
vlorf:
    if (chand)
	BClose(chand);
    if (!suck)
	FlushPacket();
    PortOn();
    return suck;
}


void FormatNetAddress(str buf, ushort zone,
			ushort net, ushort node, ushort point)
{
    if (zone)
    	sprintf(buf, (point ? "%lu:%lu/%lu.%lu" : "%lu:%lu/%lu"), (long) zone,
				(long) net, (long) node, (long) point);
    else
	sprintf(buf, (point ? "%lu/%lu.%lu" : "%lu/%lu"),
				(long) net, (long) node, (long) point);
}


void CreateQNetKluge(str buf, struct Mess *mm, str shortto)
{
    register str k, b = buf;
    if (pcboard_net) {
	char buf2[60];
#ifndef USE_AT_ROUTE
	strncpy0(buf2, shortto, 25);
	Stranks(buf2);
	k = strend(buf2);
	*k++ = '@';
	FormatNetAddress(k, mm->zone, mm->net, mm->node, mm->point);
	if (mm->attribits & UPL_NETCRASH)
	    strcat(buf2, " +C");
	if (mm->attribits & UPL_NETDIRECT)
	    strcat(buf2, " +D");
	sprintf(b, "\xFF@TO     :%-60sN", buf2);
#else
	FormatNetAddress(buf2, mm->zone, mm->net, mm->node, mm->point);
	if (mm->attribits & UPL_NETCRASH)
	    strcat(buf2, " +C");
	if (mm->attribits & UPL_NETDIRECT)
	    strcat(buf2, " +D");
	sprintf(b, "\xFF@ROUTE  :%-60sN", buf2);
#endif
	return;
    }
    for (k = inverse_netkluge ? "@/" : qnetkluge; *k; k++)
	if (*k == '/') {
	    if (k[1] == '/')
		*b++ = '/', k++;
	    else {
		FormatNetAddress(b, mm->zone, mm->net, mm->node, mm->point);
		b += strlen(b);
	    }
	} else
	    *b++ = *k;
    *b = 0;
    if (inverse_netkluge) {
	if (mm->attribits & UPL_NETCRASH && strlen(buf) <= 19)
	    strcpy(b, " CRASH");
	if (mm->attribits & UPL_NETIMMEDIATE && strlen(buf) <= 21)
	    strcat(b, " IMM");
	if (mm->attribits & UPL_NETDIRECT && strlen(buf) <= 21)
	    strcat(b, " DIR");
    }
}


void splat(str dest, str source, short len)
{
    while (len--)
	*(dest++) = *source ? *(source++) : ' ';
}


bool flush128(void)
{
    while (c128 <= 127)
	qobuf[c128++] = ' ';
    if (BWrite(qohand, qobuf, 128, 1) < 1) {
	ReadWriteErr("write text to reply", qofame, DREP);
	return false;
    }
    c128 = 0;
    return true;
}


bool stick128(str src, short len)
{
    sticktotal += len;
    if (donotstick)
	return true;
    while (len-- > 0) {
	if (c128 > 127 && !flush128())
	    return false;
	qobuf[c128++] = *(src++);
    }
    return true;
}


bool WriteQWKbody(struct Mess *mm)
{
    short l, e, w, g;
    ustr p;
    static char eol = QEOL;

    for (l = 0; l < mm->linect; l++) {
	p = mm->lines[l];
	if (l < mm->linect - 1)
	    w = mm->linetypes[l + 1], g = w & GAP_MASK, w &= TYPE_MASK;
	else
	    w = BODYTYPE, g = 0;
	e = p ? p[-1] : 0;
	while (e && !p[e - 1])
	    e--;
	if (e && p[e - 1] == '\r')
	    e--;
	if (!stick128(p, e))
	    return false;
	if (w >= WRAPTYPE) {
	    if (!stick128(GAP_SPACES, g))
		return false;
	} else if (l < mm->linect - 1)		/* no EOL on final line */
	    if (!stick128(&eol, 1))
		return false;
    }
    return true;
}


bool WriteREP(void)
{
    BPTR oldcd;
    char fluf[20], st[160], kluge[160], brag[32];
    qwk_rec qead;
    bool suck = false, did1 = false, pclongsub, ie_kluge, istag, stretchsub;
    bool mickey_net = inverse_netkluge && !pcboard_net;
    short i, klen, elen;
    long n;
    struct Mess *mm;
    struct Conf *cc;
    ustr lastl;
    static char eol[1] = { QEOL };

    qohand = null;
    oldcd = CurrentDir(replylock);
    PortOff();
    strcpy(qofame, qwkid);
    strcat(qofame, ".MSG");
    /* *********  WE SHOULD set a flag if any message already written */
    /* out has been reedited, and otherwise just append to this file. */
    if (!addsdrops) {
	for (i = 0; i < replies.messct; i++)
	    if (!(replies.messes[i]->bluebits & UPL_INACTIVE))
		goto gottadoit;
	DeleteFile(qofame);
#ifdef HECTOR_HACK
	if (copied_xpress)
	    copied_xpress = !DeleteFile(XPRESSNAME);
#endif
	suck = true;
	goto ralph;
    }
  gottadoit:
#ifdef HECTOR_HACK
    if (!copied_xpress && !fakery) {
	BPTR xh, ocd, wd = RLock(workdirinst);
	if (wd) {
	    ocd = CurrentDir(wd);
	    xh = OOpen(XPRESSNAME);
	    UnLock(CurrentDir(ocd));
	    if (xh) {
		CopyFile(xh, XPRESSNAME);	/* work dir -> replies dir */
		Close(xh);			/* ignore failures */
		copied_xpress = true;
	    }
	}
    }
#endif
    if (!(qohand = BOpen(qofame, true))) {
	ReadWriteErr("create upload message", qofame, DREP);
	goto ralph;
    }
    memset(&qead, ' ', 128);
    strncpy((str) &qead, qwkid, strlen(qwkid));
    if (BWrite(qohand, &qead, 128, 1) < 1) {
	ReadWriteErr("write packet header", qofame, DREP);
	goto ralph;
    }
    if (addsdrops) {
	struct DateStamp now;
	DateStamp(&now);
	DateString(&now, fluf);
	splat(qead.qday, fluf, 8);
	splat(qead.qtime, fluf + 9, 5);
	qead.qstatus = '*';
	splat(qead.qfrom, myothername, 25);
	splat(qead.qto, doorconn, 25);
	splat(qead.qblox, "1", 6);
	qead.qkilt = 0xE1;
	for (i = 0; i < areaz.messct; i++) {
	    register ushort b;
	    cc = areaz.confs[i];
	    b = cc->morebits;
	    if (b & DOOR_AREAPICKS) {
		splat(qead.qmessnum, cc->confnum, 7);
		if (b & DOOR_ADDING && !did1) {
		    splat(qead.qsubject, "ADD", 25);
		    if (b & DOOR_RESETTING)
			did1 = true, i--;	  /* redo this conference! */
		    else if (lastl = GetExtraDoorCommand(cc))
			splat(qead.qsubject + 4, lastl, 21);
		} else if (b & DOOR_RESETTING) {
		    splat(qead.qsubject, "RESET", 25);
		    if (lastl = GetExtraDoorCommand(cc))
			splat(qead.qsubject + 6, lastl, 19);
		    did1 = false;
		} else if (b & DOOR_ADDING_YOURS) {
		    if (lastl = GetExtraDoorCommand(cc))
			splat(qead.qsubject, lastl, 25);
		} else if (b & DOOR_DROPPING)
		    splat(qead.qsubject, "DROP", 25);
		n = atol(cc->confnum);
		qead.qconfnum[0] = n & 0xff;
		qead.qconfnum[1] = n >> 8;
		if (BWrite(qohand, &qead, 128, 1) < 1) {
		    ReadWriteErr("write mail door commands to", qofame, DREP);
		    goto ralph;
		}
	    }
	}
    }
    for (i = 0; i < replies.messct; i++) {
	mm = replies.messes[i];
	if (mm->bluebits & UPL_INACTIVE)		 /* marked as deleted */
	    continue;
	ie_kluge = mm->bits & EMAIL_REPLY && (searchlight_ie || iedummyto[0]
						|| strlen(mm->too) > 25);
	/* SHOULD use '+' but some doors only work right with '*': */
	qead.qstatus = (mm->bluebits & UPL_PRIVATE ? '*' : ' ');
	splat(qead.qmessnum, mm->confnum, 7);
	splat(qead.qday, mm->date, 8);
	splat(qead.qtime, mm->date + 9, 5);
	splat(qead.qfrom, mm->from, 25);
	if (ie_kluge)
	    splat(qead.qto, iedummyto[0] ? iedummyto : "ALL", 25);
	else
	    splat(qead.qto, mm->too, 25);
	splat(qead.qsubject, mm->subject, 25);
/*	splat(qead.qpassword, "", 12);  */
	if (mm->replyto)
	    sprintf(fluf, "%lu", mm->replyto);
	else
	    fluf[0] = 0;
	splat(qead.qreplyto, fluf, 8);
	qead.qkilt = 0xE1;
	n = atol(mm->confnum);
	qead.qconfnum[0] = n & 0xff;
	qead.qconfnum[1] = n >> 8;
/*	splat(qead.qpad, "", 3);  */
	lastl = mm->linect ? mm->lines[mm->linect - 1] : (ustr) "\0" + 1;
	istag = lch(lastl, TEARGHEAD);
	brag[0] = QEOL, brag[1] = 0;
	if (!stealthiness && !(mm->bluebits & UPL_PRIVATE)
			&& !strnistr(lastl, "Q-Blue", lastl[-1])) {
	    /* istag is false, otherwise strnistr would return success */
	    if (istag = lch(lastl, "... "))
		for (n = mm->linect - 2; n >= 0; n--)
		    if (lastl = mm->lines[n]) {
			if (strnistr(lastl, "Q-Blue", lastl[-1]))
			    goto nobrag;
			else if (n == mm->linect - 2)
			    istag = false;	/* preceding line not blank */
			break;			/* add brag */
		    }
	    strcpy(brag + 1, teargline + istag);   /* no 2nd newline if tag */
	    UnMask(brag + 1);
	  nobrag: ;
	}
	if (istag && !brag[1])
	    brag[0] = 0;	/* message ends with no final newline */
	/* We must now predict exactly how many bytes will   */
	/* be written when the message body is output below. */
	stretchsub = strlen(mm->subject) > 25;
	sticktotal = strlen(brag);
	st[klen = 0] = 0;
	if ((mm->bluebits & UPL_NETMAIL || ( /* qwk && */ mm->bits & EMAIL_REPLY
				&& ie_is_gated)) && mm->net && (inverse_netkluge
				|| pcboard_net || strchr(qnetkluge, '/'))) {
	    CreateQNetKluge(st, mm, qead.qto);
	    if (mickey_net) {
		splat(qead.qsubject, st, 25);	/* MKQWK address in subject */
		if (qwkE_subs) {
		    strcpy(st, "Subject: ");
		    strcpy(st + 9, mm->subject);
		} else {
		    strcpy(st, "Subj: ");
		    strcpy(st + 6, mm->subject);
		}
	    }
	    klen = strlen(st);
	}
	if (stretchsub && (mickey_net ? !klen : qwkE_subs) && !pcbkluge) {
	    char foo[80];
	    strcpy(foo, st);
	    if (qwkE_subs) {
		strcpy(st, "Subject: ");
		strcpy(st + 9, mm->subject);
	    } else {
		strcpy(st, "Subj: ");		/* basically for JC-QWK */
		strcpy(st + 6, mm->subject);
	    }
	    if (foo[0]) {
		lastl = strend(st);
		*lastl++ = QEOL;
		strcpy(lastl, foo);    /* Valence requires sub BEFORE netmail! */
	    }
	    klen = strlen(st);
	}
	if (klen)
	    sticktotal += klen + 1 + (!ie_kluge &&
				(!(mm->bluebits & UPL_NETMAIL) || mickey_net));
	if (pclongsub = (pcbkluge /* && !mickey_net */ && stretchsub))
	    sticktotal += 72;
	if (ie_kluge) {
	    short smt = strlen(mm->too);
	    if (pcboard_ie)
		elen = smt > 60 ? 144 : (smt > 25 ? 72 : 0);
	    else
		elen = smt + (searchlight_ie ? 12 : 6);
	    sticktotal += elen;
	}
	donotstick = true;			/* PRETEND to write body */
	WriteQWKbody(mm);
	donotstick = false;			/* done pretending */
	utoa((ushort) ((sticktotal + 255) >> 7), fluf);
	splat(qead.qblox, fluf, 6);
	if (BWrite(qohand, &qead, 128, 1) < 1) {
	    ReadWriteErr("write reply header to", qofame, DREP);
	    goto ralph;
	}
	c128 = 0;
	if (!pcboard_net && klen && !(stick128(st, klen) && stick128(eol, 1)))
	    goto ralph;
	if (pclongsub) {
	    sprintf(kluge, "\xFF@SUBJECT:%-60sN%lc", mm->subject, (ulong) QEOL);
	    if (!stick128(kluge, 72))
		goto ralph;
	}
	if (pcboard_net && klen && !(stick128(st, klen) && stick128(eol, 1)))
	    goto ralph;
	if (ie_kluge && elen) {
	    if (pcboard_ie) {
		ubyte c = mm->too[60];
		mm->too[60] = 0;
		sprintf(kluge, "\xFF@TO     :%-60sN%lc", mm->too, (ulong) QEOL);
		mm->too[60] = c;
		if (elen > 72)
		    sprintf(kluge + 72, "\xFF@TO2    :%-60sN%lc",
						mm->too + 60, (ulong) QEOL);
	    } else
		sprintf(kluge, "%s: %s%lc%lc", searchlight_ie ? "Internet" :
				    "To", mm->too, (ulong) QEOL, (ulong) QEOL);
	    if (!stick128(kluge, elen))
		goto ralph;
	} else if (klen && (!(mm->bluebits & UPL_NETMAIL) || mickey_net))
	    if (!stick128(eol, 1))
		goto ralph;
	if (!WriteQWKbody(mm) || !stick128(brag, strlen(brag)) || !flush128())
	    goto ralph;
    }
    suck = anythingtosave = true;
    did1 = false;
    for (i = 0; i < replies.messct; i++)
	if (replies.messes[i]->attached &&
			!(replies.messes[i]->bluebits & UPL_INACTIVE)) {
	    did1 = true;
	    break;
	}
    if (!did1)
	DeleteFile("ATTXREF.DAT");
    else {
	BPTR thumb = NOpen("ATTXREF.DAT");
	if (!thumb) {
	    DosErr("Could not create file ATTXREF.DAT\n"
		    "to tell door about attached files.");
	    suck = false;
	} else {
	    for (i = 0; i < replies.messct; i++)
		if (!(replies.messes[i]->bluebits & UPL_INACTIVE)) {
		    struct Attachment *at = replies.messes[i]->attached;
		    if (at)
			n = FPrintf(thumb, "%s,%s\r\n", at->tempname, at->arrivename);
		    else
			n = FPuts(thumb, "\r\n");
		    if (n < 0)
			break;
		}
	    if (n < 0)
		DosErr("Could not write file ATTXREF.DAT\n"
			"to tell door about attached files.");
	    Close(thumb);
	    if (n < 0)
		DeleteFile("ATTXREF.DAT");
	}
    }
  ralph:
    if (qohand) {
	long ellen = BClose(qohand);
	if (suck && !ellen) {
	    DosErr("Attempt to save replies in file\n%s has apparently failed.",
					qofame);
	    suck = false;
	}
    }
    CurrentDir(oldcd);
    PortOn();
    return suck;
}


void ReloadQWKreplies(bool nounpack)
{
    BPTR hand, foot = 0;
    char fame[290];

    PortOff();
    JoinName(fame, uploaddir, qwkid, ".REP");
    if (nounpack || DoPack(fame, true, true)) {
	strcpy(fame, qwkid);
	strcat(fame, ".MSG");
	if (!(hand = OOpen(fame))) {
	    ReadWriteErr("find reply message", fame, DREP);
	    PortOn();
	    return;
	}
	LoadQWKmess(hand, true);
	Close(hand);
	if (replies.messct || addsdrops) {
	    BHandle elbow;
	    int i;
	    if (elbow = BOpen("ATTXREF.DAT", false)) {
		char nn[256];
		str p;
		for (i = 0; i < replies.messct; i++)
		    if (BGetline(elbow, nn, 256) > 0 && (p = strchr(nn, ','))) {
			*p++ = '\0';
			while (isspace(*p)) p++;
			if (!*p) p = nn;	/* bogus bandaid */
			if (hand = RLock(nn)) {
			    UnLock(hand);
			    if (NEW(replies.messes[i]->attached)) {
				strcpy(replies.messes[i]->attached->tempname, nn);
				strcpy(replies.messes[i]->attached->arrivename, p);
				replies.messes[i]->attached->localpath[0] = '\0';
				replies.messes[i]->attached->localsource = false;
			    } /* else fail silently */
			}
		    }
		BClose(elbow);
	    }
	    anythingtosave = true;
	}
    }
    PortOn();
}
