/* stuff for reading and writing the Blue Wave format */

#include <exec/memory.h>
#include <dos/dosextens.h>
#include "qblue.h"
#include "version.h"

#define RELOAD_UPI
/* turning this off saves only about 500 bytes. */

/* take care of several items misdefined in older versions of bluewave.h: */
typedef struct
{
    tBYTE keywords[10][21];   /* User's entire set of door keywords  */
    tBYTE filters[10][21];    /* User's entire set of door filters   */
    tBYTE macros[3][78];      /* User's door bundling command macros */
    tBYTE password[21];       /* Password, each byte + 10            */
    tBYTE passtype;           /* Password type                       */
                              /*   0=none 1=door 2=reader 3=both     */
    tWORD flags;              /* Bit-mapped flags                    */
} REAL_PDQ_HEADER;

#if sizeof(PDQ_HEADER) != sizeof(REAL_PDQ_HEADER)
#  ifdef PDQ_HEADER
#    undef  PDQ_HEADER
#  endif
#  define PDQ_HEADER        REAL_PDQ_HEADER
#endif

#if INF_NOT_MY_MAIL == 0x0008
#  undef  INF_NOT_MY_MAIL
#  define INF_NOT_MY_MAIL   0x0010
#  undef  INF_GRAPHICS
#  define INF_GRAPHICS      0x0008
#endif

#if INF_NET_INTERNET == 2
#  undef  INF_NET_INTERNET
#  define INF_NET_INTERNET  1
#endif
#if INF_NET_INTERNET != UPL_NET_INTERNET
#  error Incompatible NET_INTERNET values!
#endif

#ifndef MSG_NET_PRIVATE
#  define MSG_NET_PRIVATE   MSG_PRIVATE
#endif

#define STUNTED_UPL_REC_LEN offsetof(UPL_REC, net_dest)

/* blue wave flags used in the bits field: */
#define __fTiOv_ FTI_MSGCRASH | FTI_MSGFILE | FTI_MSGKILL | FTI_MSGIMMEDIATE
#define FTI_OVERLAPMASK (__fTiOv_ | FTI_MSGHOLD | FTI_MSGFRQ | FTI_MSGDIRECT)

/* (We avoid continuing lines with "\" because when that's used the compiler */
/* gives error messages with incorrect line numbers, confusing QuikFix.)     */


bool AskPassword(str answer);
bool AskTruncatedText(ulong bad, ulong total, str packetname);
bool DoPack(str fullname, bool un, bool rep);
void DateString(struct DateStamp *d, str s);
ulong Text2ux(str date);
void ux2DS(struct DateStamp *d, ulong inn);
bool CreateNewArea(str num, str shortname, str longname, ushort *initialarea);
void JoinName(str result, str dir, str file, str tail);
void SeekErr(str what);
void ReadWriteErr(str action, str file, ushort dir);
BPTR CreateLock(str dirname);
bool SafeExamineFH(BPTR hand, struct FileInfoBlock *fib, str flename);

void ReloadQWKreplies(bool nounpack);
bool WriteREP(void);
bool LoadQuacket(str controlfile);
bool LoadQWKmess(BPTR hand, bool replypak);
void FlushPacket(void);
bool LoadMessage(struct Mess *mm, BPTR hand, short qeol, bool trim);
struct Conf *NewPoolConf(void);
struct Mess *NewPoolMess(void);
void ReturnToPool(struct Mess *mm);
ustr NewPoolString(ushort length);
void ContextName(str filename);
void ReadBBSlocalSetup(bool askfile);
bool PDQstateDiffers(void);

void ScanForTrash(void), PickUpTheTrash(void);
void CreateBBSFile(bool artificial);
void FromTo(struct Mess *mm);
void Personalize(struct Mess *mm);
void EmptinessGripe(void);
bool TwitTest(struct Mess *mm);
void ReadOrigin(struct Mess *mm);
bool NeedToCheckOrigins(void);
bool MakeReadAreaz(void);
void UsePlaceholder(void);
bool ReplyChoke(ushort current, bool reloading);
void SetRegistration(str registration);
void SaveBookmarks(void);
bool ValidInternet(str s);
ustr RetrieveReferences(ustr *groups);


import struct trash *trashfiles;
import struct Conf **readareazfree;

import char bwpassword[], bwkeywords[10][23], bwfilters[10][23];
import char oldpassword[], oldkeywords[10][21], oldfilters[10][21];
import char macro1[], macro2[], macro3[], oldmacro1[], oldmacro2[], oldmacro3[];
import char requestedfiles[10][15], tempacketname[], fakeryname[], CONTNAME[];
import char bversionL[], bversionI[], iedummyto[];

import str fibuf;
import short confounded;
import ushort passwordtype, oldpasswordtype, bwflistype, oldflistype;
import ushort areazroom, lastsorder, requestedfiles_limit, bwpksize, oldpksize;
import ulong megachexum, loaded_size;

import bool uppitynames, uppitytitle, addsdrops, filerequests_exist;
import bool opensincelasttagload, bwhotkeys, bwgraphics, bwexpert, bwownmail;
import bool oldhotkeys, oldgraphics, oldexpert, oldownmail, stealthiness;
import bool bwnoxlines, bwnumericext, oldnoxlines, oldnumericext;
import bool bwauto1, bwauto2, bwauto3, oldauto1, oldauto2, oldauto3;


struct Conf areaz, readareaz, inuseareaz;

ustr *oldfilenames, *messageID, *newsgroups;
char myloginame[NAMELEN], myothername[NAMELEN], sysopname[42], bbsname[65];
char registration[9] = "00000000";

/* ARRRGH!  BW2.20 writes "[Global Mail Host Configuration]", but */
/* BWdev300 says you should use "[Global Mail Host Information]". */

const static char OLChead[] = "[Global Mail Host Configuration]";
#define OH_ENUFF 18

local char uppacketname[9];

short tempmessct, gathering, localmailareanum, netmailareanum, unetareanum;
short hostcredits, hostdebits, iemailareanum = -1;
ushort hostzone, hostnet, hostnode, hostpoint, hostnetmailflags;
ushort fromtolen, subjectlen;

local ushort infheaderpad = 0, areainfopad = 0, mixrecpad = 0, ftirecpad = 0;
local short uplrecskip;

bool anythingtosave = false, pdq_exists = false, version3;
bool no_forward, forbidOLC, forbidREQ;
bool useUPL, pdq_difference, official_iemail, bad_upl = false, over32k = false;
bool mylogin26, myother26, any26, personalityflaw, twits, nowreloading = false;


ushort i2(ubyte *intel);
ulong i4(ubyte *intel);
void Un2(ubyte *intel, ushort moto);
void Un4(ubyte *intel, ulong moto);

		/* DON'T IMPORT without these pragmas! */
#pragma regcall(i2(a0))
#pragma regcall(i4(a0))
#pragma regcall(Un2(a0,d0))
#pragma regcall(Un4(a0,d0))

#ifdef C_I24

short i2(ubyte *intel)
{
    short ret;
    register ubyte *pret = (adr) &ret;
    pret[0] = intel[1];
    pret[1] = intel[0];
    return ret;
}


void Un2(ubyte *intel, ushort moto)
{
    register ubyte *mot = (adr) &moto;
    intel[0] = mot[1];
    intel[1] = mot[0];
}


long i4(ubyte *intel)
{
    long ret;
    register ubyte *pret = (adr) &ret;
    pret[0] = intel[3];
    pret[1] = intel[2];
    pret[2] = intel[1];
    pret[3] = intel[0];
    return ret;
}

void Un4(ubyte *intel, ulong moto)
{
    register ubyte *mot = (adr) &moto;
    intel[0] = mot[3];
    intel[1] = mot[2];
    intel[2] = mot[1];
    intel[3] = mot[0];
}

#else
#  asm

	PUBLIC	_i2
_i2:	moveq	#0,d0
	move.b	1(a0),d0
	asl.w	#8,d0
	move.b	(a0),d0
	rts

	PUBLIC	_i4
_i4:	move.b	3(a0),d0
	asl.w	#8,d0
	move.b	2(a0),d0
	asl.l	#8,d0
	move.b	1(a0),d0
	asl.l	#8,d0
	move.b	(a0),d0
	rts

	PUBLIC	_Un2,_Un4
_Un4:	move.b	d0,(a0)+
	asr.l	#8,d0
	move.b	d0,(a0)+
	asr.l	#8,d0
_Un2:	move.b	d0,(a0)+
	asr.w	#8,d0
	move.b	d0,(a0)
	rts

#  endasm
#endif C_I24


ushort Chextr(str s, ushort initial);
#pragma regcall(Chextr(a0, d0))


#ifdef C_CHEXTR

ushort Chextr(str s, ushort initial)
{
    while (*s)
	initial = (initial << 1) + (initial >> 15) + *s++;
}

#else
#  asm
	public	_Chextr
_Chextr:
	moveq	#0,d1
chx1:	move.b	(a0)+,d1
	beq	chx0
	rol.w	#1,d0
	add.w	d1,d0
	bra	chx1
chx0:	rts
#  endasm
#endif


ushort Chex(struct Mess *mm)
{				  /* vvv  Paranoia */
    ushort r = mm->bits & ISBULLETIN ? 0 : Chextr(mm->from, 0);
    r = Chextr(mm->too, r);
    r = Chextr(mm->subject, r);
    return Chextr(mm->date, r);
}


void UnMask(str s)
{
    while (*s)
	*s++ ^= INITMASK;
}


local bool IsItText(str buf, short len)
{
    if (len <= 0) return false;
    buf[min(32, len - 1)] = 0;
    return *buf > 5 && strlen(buf) > 13
			&& (strchr(buf, '\n') || strchr(buf, '\r'));
}


local bool ReadInfHeader(BHandle hand, str filename)
{
    INF_HEADER ihead;
    INF_AREA_INFO *iai = (adr) &ihead;		/* fake union */
    struct Conf *cc, *clist = null;
    short len, a;
    long iaisize = sizeof(INF_AREA_INFO);

    if ((len = BRead(hand, &ihead, 1, sizeof(ihead))) < sizeof(ihead)) {
	if (!(qwk = IsItText((str) &ihead, len)))
	    ReadWriteErr(null, filename, DWORK);
	return false;
    }
    if (ihead.ver < 2 || ihead.ver > 5) {
	if (ihead.ver < 2 || !(qwk = IsItText((str) &ihead, sizeof(ihead))))
	    Err("This program cannot understand\n"
			"version %ld Blue Wave packets.", (long) ihead.ver);
	return false;
    }
    version3 = ihead.ver > 2;
    a = i2(ihead.ctrl_flags);
    forbidOLC = !!(a & INF_NO_CONFIG);
    forbidREQ = !!(a & INF_NO_FREQ);
    pdq_difference = official_iemail = false;
    oldpasswordtype = passwordtype = ihead.passtype;
    for (a = 0; ihead.password[a] && a < 21; a++)
	bwpassword[a] = ihead.password[a] - 10;
    bwpassword[a] = 0;
    strcpy(oldpassword, bwpassword);
    if (passwordtype & 2 && ihead.password[0])
	if (!AskPassword(bwpassword))
	    return false;
    for (a = 0; a < 10; a++) {
	register str s;		/* avoid bogus "expression too complex" error */
	s = strncpy0(bwkeywords[a], ihead.keywords[a], 20);
	strcpy(oldkeywords[a], s);
	s = strncpy0(bwfilters[a], ihead.filters[a], 20);
	strcpy(oldfilters[a], s);
    }
    strncpy0(macro1, ihead.macros[0], 79);
    strncpy0(macro2, ihead.macros[1], 79);
    strncpy0(macro3, ihead.macros[2], 79);
    strcpy(oldmacro1, macro1);
    strcpy(oldmacro2, macro2);
    strcpy(oldmacro3, macro3);
    a = i2(ihead.uflags);
    oldhotkeys = bwhotkeys = !!(a & INF_HOTKEYS);
    oldgraphics = bwgraphics = !!(a & INF_GRAPHICS);
    oldexpert = bwexpert = !!(a & INF_XPERT);
    oldownmail = bwownmail = !(a & INF_NOT_MY_MAIL);
    oldnoxlines = bwnoxlines = !version3 || !(a & INF_EXT_INFO);
    oldnumericext = bwnumericext = version3 && (a & INF_NUMERIC_EXT);
    oldauto1 = bwauto1 = version3 && ihead.auto_macro[0];
    oldauto2 = bwauto2 = version3 && ihead.auto_macro[1];
    oldauto3 = bwauto3 = version3 && ihead.auto_macro[2];
    oldpksize = bwpksize = i2(ihead.max_packet_size);
    no_forward = !ihead.can_forward;
    oldflistype = bwflistype = (a = ihead.file_list_type) <= 2 ? a : 0;
    strncpy0(myloginame, ihead.loginname, NAMELEN - 1);
    strncpy0(myothername, ihead.aliasname, NAMELEN - 1);
    hostzone = i2(ihead.zone);
    hostnet = i2(ihead.net);
    hostnode = i2(ihead.node);
    hostpoint = i2(ihead.point);
    hostcredits = i2(ihead.credits);
    hostdebits = i2(ihead.debits);
    strncpy0(sysopname, ihead.sysop, 40);
    strncpy0(bbsname, ihead.systemname, 64);
    if ((requestedfiles_limit = ihead.maxfreqs) > 10)
	requestedfiles_limit = 10;
    hostnetmailflags = i2(ihead.netmail_flags);
    a = i2(ihead.inf_header_len) - sizeof(INF_HEADER);
    if (a > 0)
	infheaderpad = a;
    a = i2(ihead.inf_areainfo_len) - sizeof(INF_AREA_INFO);
    if (a > 0) {
	areainfopad = a - 1;
	iaisize++;
    }
    a = i2(ihead.mix_structlen) - sizeof(MIX_REC);
    if (a > 0)
	mixrecpad = a;
    a = i2(ihead.fti_structlen) - sizeof(FTI_REC);
    if (a > 0)
	ftirecpad = a;
    useUPL = ihead.uses_upl_file || version3;
    for (a = 0; a < 5; a++)
	strncpy0(trashfiles[a].n, ihead.readerfiles[a], 12);
    while (a < TRASHLIMIT)
	trashfiles[a++].n[0] = 0;
    subjectlen = ihead.subject_len;
    if (subjectlen <= 1 || subjectlen >= SUBJLEN)
	subjectlen = SUBJLEN - 1;
    fromtolen = ihead.from_to_len;
    if (fromtolen <= 1 || fromtolen >= NAMELEN)
	fromtolen = NAMELEN - 1;
    myloginame[fromtolen] = myothername[fromtolen] = 0;
    strncpy0(uppacketname, ihead.packet_id, 8);
    Stranks(uppacketname);
    Stranks(myloginame);
    Stranks(myothername);
    if (!myothername[0])
	strcpy(myothername, myloginame);
    Stranks(sysopname);
    Stranks(bbsname);
    if (infheaderpad && BSeek(hand, infheaderpad, OFFSET_CURRENT) < 0) {

	SeekErr("reading packet header info");
	BClose(hand);
	return false;
    }
    while ((len = BRead(hand, iai, iaisize, 1)) == 1) {
	a = strlen(iai->title);
	if (a >= LONCOLEN)
	    a = LONCOLEN - 1;
	if (!(cc = NewPoolConf()) || !(cc->longname = NewPoolString(a))) {
	    Err("No memory for loading\nmessage area info.");
	    areaz.messct = 0;
	    return false;
	}
	cc->messes = (adr) clist;		/* store em as list */
	clist = (adr) cc;
	areaz.messct++;
	strncpy0(cc->shortname, iai->echotag, SHOCOLEN - 1);
	strncpy0(cc->longname, iai->title, a);
	strncpy0(cc->confnum, iai->areanum, 5);
	strupr(cc->confnum);
	/* Stranks(cc->shortname); */  /* at least one BBS needs spaces preserved! */
	Stranks(cc->longname);
	Stranks(cc->confnum);
	cc->areabits = i2(iai->area_flags);
	cc->net_type = iai->network_type;
	if (areainfopad && BSeek(hand, areainfopad, OFFSET_CURRENT) < 0) {
	    SeekErr("reading message area info");
	    BClose(hand);
	    areaz.messct = 0;
	    return false;
	}
    }
    if (len < 0 || !areaz.messct) {
	ReadWriteErr(null, filename, DWORK);
	areaz.messct = 0;
	return false;
    }
    if (!(areaz.confs = Valloc(areaz.messct << 2))) {
	Err("No memory for storing\nlist of message areas.");
	areaz.messct = 0;
	return false;
    }
    areazroom = areaz.messct;
    len = localmailareanum = iemailareanum = -1;
    for (a = areaz.messct - 1; a >= 0; a--) {
	areaz.confs[a] = clist;
	if (clist->net_type == INF_NET_INTERNET) {
	    if (clist->areabits & INF_NETMAIL) {
		clist->morebits |= INTERNET_EMAIL;
		iemailareanum = a;
		clist->areabits &= ~INF_NETMAIL;	/* reserve for Fido */
		official_iemail = true;
	    } else if (clist->areabits & INF_ECHO)
		clist->morebits |= NEWSGROUP | MULTI_NEWSGROUP;
	} else if ((clist->areabits & (INF_NETMAIL | INF_POST))
				== (INF_NETMAIL | INF_POST))
	    netmailareanum = a;
	else if (clist->areabits & (INF_NO_PUBLIC | INF_POST)
				== (INF_NO_PUBLIC | INF_POST))
	    localmailareanum = a;
	else if ((clist->areabits & (INF_NO_PRIVATE | INF_POST)) == INF_POST)
	    len = a;
	clist = (adr) clist->messes;
	areaz.confs[a]->messes = null;
    }
    if (localmailareanum < 0 && len >= 0)
	localmailareanum = len;
    ReadBBSlocalSetup(false);
    myother26 = strlen(myothername) > 25;
    mylogin26 = strlen(myloginame) > 25;
    any26 = strlen(localanyname[0] ? localanyname : anyname) > 25;
    return true;
}


local bool ReadBWBody(short apppt)
{
    char tfrom[NAMELEN], ttoo[NAMELEN], tsub[SUBJLEN], filename[13];
    BHandle hand;
    ulong totalcount = 0, slbluebadcount = 0, overs = 0, unders = 0;
    ulong lostcount = 0, textsize = ~0;
    short a, i;
    bool anytext = false, suck = false, scanorigins;
    struct Conf *cc;
    struct Mess *mm;
    MIX_REC *mrrr, *mrrend;
    FTI_REC ftr;

    strcpy(filename, packetname);
    strcpy(filename + apppt, ".MIX");
    if (!InhaleFile(filename)) {
	apppt = strlen(strcpy(filename, uppacketname));
	strcpy(filename + apppt, ".MIX");
	if (!apppt || !InhaleFile(filename) || !fibize) {
	    ReadWriteErr(null, filename, DWORK);
	    goto cleanup1;
	}
    }
    mrrend = (adr) (fibuf + fibize - sizeof(MIX_REC));
    for (mrrr = (adr) fibuf; mrrr <= mrrend;
			mrrr = (adr) ((str) (mrrr + 1) + mixrecpad)) {
	ushort mekt = i2(mrrr->totmsgs);
	if (mekt > 32767) {
	    mekt = 32767;
	    over32k = true;
	}
	if (!(cc = Confind(mrrr->areanum))
			|| (mekt && !(cc->messes = Valloc(mekt << 2)))) {
	    if (mekt)
		Err("%s.\nAny messages in area %s will be lost.", cc ?
				"No memory to store list of messages":
				"This packet contains invalid data",
				mrrr->areanum);
#ifdef BETA
	    else
		Err("Packet contains data for nonexistent area %s.\n"
				"This probably will not cause trouble, though.",
				mrrr->areanum);
#endif
	} else {
	    memset(cc->messes, 0, mekt << 2);
	    cc->messct = mekt;
/*	    cc->tomect = i2(mrrr->numpers);   */
	    cc->unfiltered = (adr) i4(mrrr->msghptr);  /* temp use as offset */
	}
    }
    if ((str) mrrend - (str) mrrr > mixrecpad) {
	Err("Expected data not present in\nfile %s in work directory.",
				filename);
	goto cleanup1;
    }
    ExhaleFile();

    scanorigins = NeedToCheckOrigins();
    strcpy(filename + apppt, ".DAT");
    texthand = OOpen(filename);
    if (SafeExamineFH(texthand, fib, filename))
	textsize = fib->fib_Size;
    strcpy(filename + apppt, ".FTI");
    if (!(hand = BOpen(filename, false))) {
	ReadWriteErr(null, filename, DWORK);
	goto cleanup1;
    }
    megachexum = 0;
    for (a = 0; a < areaz.messct; a++) {
	cc = areaz.confs[a];
	if (BSeek(hand, (ulong) cc->unfiltered, OFFSET_BEGINNING) < 0) {
	    DosErr("Seek failed while reading message headers.  The\n.FTI file"
				"may be incomplete, or the .MIX file invalid.");
	    goto cleanup1;
	}
	while (cc->sofar < cc->messct
			&& BRead(hand, &ftr, sizeof(ftr), 1) == 1) {
	    if (!(mm = NewPoolMess()))
		break;
	    strncpy0(tfrom, ftr.from, NAMELEN - 1);
	    strncpy0(ttoo, ftr.to, NAMELEN - 1);
	    strncpy0(tsub, ftr.subject, SUBJLEN - 1);
	    Stranks(tfrom);
	    Stranks(ttoo);
	    Stranks(tsub);
	    if (!(mm->from = NewPoolString(strlen(tfrom)))
				|| !(mm->too = NewPoolString(strlen(ttoo)))
				|| !(mm->subject = NewPoolString(strlen(tsub))))
		break;
	    strcpy(mm->from, tfrom);
	    strcpy(mm->too, ttoo);
	    strcpy(mm->subject, tsub);
	    FromTo(mm);
	    strncpy0(mm->date, ftr.date, 19);
	    strcpy(mm->confnum, cc->confnum);
	    mm->ixinbase = (ulong) i2(ftr.msgnum);
	    mm->replyto = (ulong) i2(ftr.replyto);
	    mm->replyat = (ulong) i2(ftr.replyat);
	    totalcount++;
	    if (mm->replyto >= mm->ixinbase)
		slbluebadcount++;
	    mm->attribits = i2(ftr.flags);
	    if (mm->attribits & FTI_MSGREAD)
		mm->bits |= SEENINBASE;
	    if (mm->attribits & FTI_MSGPRIVATE)
		mm->bluebits |= UPL_PRIVATE;
	    if (cc->areabits & INF_NETMAIL)
		mm->bluebits |= UPL_NETMAIL;
	    /* these FTI_MSG* definitions match the UPL_NET* ones: */
	    mm->attribits &= FTI_OVERLAPMASK;
	    mm->datfseek = i4(ftr.msgptr);
	    if (mm->datflen = i4(ftr.msglength))
		anytext = true;
	    mm->zone = i2(ftr.orig_zone);
	    mm->net = i2(ftr.orig_net);
	    mm->node = i2(ftr.orig_node);
	    megachexum = ((megachexum << 3) | (megachexum >> 29)) ^ Chex(mm);
	    if (scanorigins)
		ReadOrigin(mm);
	    if (TwitTest(mm)) {
		cc->messct--;
		twits = true;
		if (mm->attached)
		    FREE(mm->attached);
		ReturnToPool(mm);	/* reuse memory */
		continue;
	    }
	    if (mm->bits & TOME)
		cc->tomect++;
	    Personalize(mm);
	    if (cc->sofar >= cc->messct)
		lostcount++;
	    else
		cc->messes[cc->sofar++] = mm;
	    if (ftirecpad && BSeek(hand, ftirecpad, OFFSET_CURRENT) < 0) {
		SeekErr("reading message headers");
		BClose(hand);
		goto cleanup1;
	    }
	    unders++;
	    if (mm->datflen && mm->datfseek + mm->datflen > textsize)
		overs++;
	}
	if (cc->sofar < cc->messct) {
	    ReadWriteErr(null, filename, DWORK);
	    BClose(hand);
	    goto cleanup1;
	}
	cc->sofar = 0;
    }
    /* SLBlue gives garbage values in replyto and replyat... detect and fix: */
    if (totalcount && slbluebadcount * 5 / totalcount)		/* 20% */
	for (a = 0; a < areaz.messct; a++) {
	    cc = areaz.confs[a];
	    for (i = 0; i < cc->messct; i++) {
		mm = cc->messes[i];
		mm->replyto = mm->replyat = 0;
	    }
	}
    if (overs)
	if (textsize ? !AskTruncatedText(overs, unders, packetname)
			: (Err("The message text file %s.DAT is empty!",
					packetname), true)) {
	    BClose(hand);
	    goto cleanup1;
	}
    if (lostcount)
	Err("%lu messages were lost because of\n"
			"low memory or invalid packet data.", lostcount);

    suck = (!anytext || texthand) && (lostcount < totalcount || !totalcount);
    if (!suck) {
	ReadWriteErr(null, filename, DWORK);
	BClose(hand);
    } else {
	BClose(hand);
	if (!(suck = MakeReadAreaz()))
	    goto cleanup1;
	ScanForTrash();			/* just to get datestamps */
	PickUpTheTrash();
	CreateBBSFile(false);
    }
    if (!totalcount) {
	EmptinessGripe();
	if (bullstuff.messct)
	    readareaz.current = whicha = 0;
	else
	    UsePlaceholder();
    }
cleanup1:
    for (a = 0; a < areaz.messct; a++)
	areaz.confs[a]->unfiltered = null;
    ExhaleFile();
    if (over32k)
	Err("More than 32767 messages were found in one area.\n"
			"Messages beyond that point are not displayed.");
    return suck;
}


/* expects current directory to be set where the unpacked packet is */

bool LoadPacket(void)
{
    char filename[32];
    short apppt = strlen(packetname);
    BHandle hand = null;
    bool suck = false, qwaklok = false;

    PortOff();
    opensincelasttagload = true;
    lastsorder = 0;
    netmailareanum = -1;
    pcbkluge = twits = anythingtosave = false;
    areaz.confs = readareaz.confs = readareazfree = null;
    personalityflaw = uppitynames = uppitytitle = over32k = false;
    areaz.messct = readareaz.messct = replies.messct
			= personals.messct = bullstuff.messct = 0;
    personals.current = replies.current = bullstuff.current = 0;
    if (fakery) {
	strcpy(filename, fakeryname);	/* allow for misnamed context file */
	if (qwk) {
	    bool ret = LoadQuacket(filename);
	    if (qwk) {
		PortOn();
		return ret;
	    } /* else LoadQuacket detected a *.INF file */
	}
	if (!(hand = BOpen(filename, false))) {
	    PortOn();
	    ReadWriteErr(null, filename, DBBS);
	    return false;
	}
    } else {
	BPTR lk;
	if (lk = RLock(CONTNAME)) {
	    UnLock(lk);
	    qwaklok = true;
	}
	if (qwk && qwaklok) {
	    bool ret = LoadQuacket(CONTNAME);
	    PortOn();
	    return ret;
	} else
	    qwk = false;
	strcpy(filename, packetname);
	strcpy(filename + apppt, ".INF");
	while (!(hand = BOpen(filename, false))) {
	    long e = IoErr();
	    if (qwaklok) {
		BPTR foot;
		bool ret;
		if (foot = RLock("MESSAGES.DAT")) {
		    UnLock(foot);
		    qwk = true;
		    ret = LoadQuacket(CONTNAME);
		    PortOn();
		    return ret;
		}
	    } else if (!tempacketname[0] && (FileSize(me->pr_CurrentDir),
						tempacketname[0])
					&& stricmp(tempacketname, packetname)) {
		strcpy(packetname, tempacketname);
		apppt = strlen(packetname);
		strcpy(filename, packetname);
		strcpy(filename + apppt, ".INF");
		continue;
	    }
	    PortOn();
	    SetIoErr(e);
	    DosErr("Could not open expected file in work directory for\n"
				"reading -- found neither CONTROL.DAT nor %s",
				filename);
	    return false;
	}
    }

    suck = ReadInfHeader(hand, filename);
    BClose(hand);
    if (qwk)		/* ReadInfHeader detected a text file */
	suck = LoadQuacket(filename);
    else {
	if (suck)
	    if (fakery)
		UsePlaceholder();
	    else
		suck = ReadBWBody(apppt);
	if (suck) {
	    if (uppacketname[0])
		strcpy(packetname, uppacketname);
	} else
	    FlushPacket();
    }
    PortOn();
    return suck;
}


local void MakeBWReplyFilename(str fame, str confnum, ulong index)
{
    BPTR ocd = CurrentDir(replylock), tl = 0;
    char cum[10];
    short i;

    for (i = 0; *confnum && i <= 8; i++, confnum++)
	if (isalnum(*confnum) || strchr("$%'-_@~`!()^&", *confnum))
	    cum[i] = *confnum;
	else
	    cum[i] = '_';
    cum[i] = 0;
    if (i > 7)
	i = 7;
    else
	cum[i + 1] = 0;
    do {
	sprintf(fame, "%s.%03lu", cum, index);
	if (tl) {
	    UnLock(tl);
	    if (++cum[i] >= 'Z') {
		cum[i] = '$';
		if (i) i--;
		else break;
	    }
	} else  /* this is the first time through the loop */
	    cum[i] = 'A';
    } while (tl = RLock(fame));
    CurrentDir(ocd);
}


void GetBWReplyFilename(str fame, struct Mess *mm)
{
    if (oldfilenames[mm->replyat])
	strcpy(fame, oldfilenames[mm->replyat]);
    else
	MakeBWReplyFilename(fame, mm->confnum, (ulong) mm->ixinbase);
}


local bool TakenRix(ushort ix, struct Mess *self)
{
    register short i;
    register struct Mess *mm;

    for (i = 0; i < replies.messct; i++) {
	mm = replies.messes[i];
	if (mm != self && (mm->ixinbase == ix || (oldfilenames[i]
				&& ((oldfilenames[i][13] << 8)
				    + oldfilenames[i][14]) == ix)))
	    return true;
    }
    return false;
}


ushort UniqueRix(struct Mess *self)
{
    ushort r;
    if (self && self->ixinbase && !TakenRix(self->ixinbase, self))
	return self->ixinbase;
    for (r = 1; TakenRix(r, self); r++) ;
    return r;
}


/* kind > 0 means .UPL, = 0 means .UPI, < 0 means .NET */

bool AnyShadeOfBlue(short kind, str fame)
{
    union {
	UPI_HEADER b_uph;
	UPI_REC b_ur;
	NET_REC b_ner;
	UPL_HEADER b_ulh;
	UPL_REC b_ul;
    } both;
#define uph both.b_uph
#define ur  both.b_ur
#define ner both.b_ner
#define ulh both.b_ulh
#define ul  both.b_ul
    BHandle hand;
    short i, j;
    long ribbon;
    struct Mess *mm;
    struct Conf *cc;
    bool suck = false;
    ubyte st[80];
    ustr toot, p;
    size_t bsize = kind > 0 ? /* STUNTED_UPL_REC_LEN */ sizeof(UPL_REC) :
			    (kind < 0 ? sizeof(NET_REC) : sizeof(UPI_REC));

    if (!(hand = BOpen(fame, true))) {
	DosErr("Could not create upload header file\n"
				"%s in your replies directory.", fame);
	goto ralph;
    }
    memset(&both, 0, sizeof(both));
    if (kind >= 0) {
	if (kind > 0) {
	    strcpy(ulh.regnum, registration);
	    strcpy(ulh.vernum, stealthiness ? CLOAK_GENERIC_VER : bversionL);
	    UnMask(ulh.vernum);
	    for (i = 0; ulh.vernum[i]; i++)
		ulh.vernum[i] -= 10;
	    ulh.reader_major = VERSION_MAJOR;
	    ulh.reader_minor = VERSION_MINOR;
	    strcpy(ulh.reader_name, "Q-Blue offline mail reader for Amiga");
	    strcpy(ulh.reader_tear, stealthiness ? CLOAK_GENERIC : CLOAK_NAME);
	    UnMask(ulh.reader_tear);
	    ribbon = i = sizeof(UPL_HEADER);
	    Un2(ulh.upl_header_len, i);
	    i = bsize;
	    Un2(ulh.upl_rec_len, i);
	    strcpy(ulh.loginname, myloginame);
	    strcpy(ulh.aliasname, myothername);
	} else {
	    strcpy(uph.regnum, registration);
	    strcpy(uph.vernum, stealthiness ? CLOAK_GENERIC_VER : bversionI);
	    UnMask(uph.vernum);
	    for (i = 0; uph.vernum[i]; i++)
		uph.vernum[i] -= 10;
	    ribbon = sizeof(UPI_HEADER) - 1;	/* remove pad byte */
	}
	if (BWrite(hand, &both, ribbon, 1) < 1) {
	    DosErr("Could not save data in upload header file\n"
				"%s in your replies directory.", fame);
	    goto ralph;
	}
    }
    for (i = 0; i < replies.messct; i++)
	if ((mm = replies.messes[i]) && !(mm->bluebits & UPL_INACTIVE)) {
	    if (!(cc = Confind(mm->confnum))) {      /* should never happen */
		Err("INTERNAL ERROR:  I've lost track of what area\n"
				"reply number %ld belongs in!  Cannot save it.",
				i + 1L);
		continue;
	    }
	    memset(&both, 0, bsize);
	    GetBWReplyFilename(st, mm);
	    ribbon = (mm->mreplyee && cc != Confind(mm->mreplyee->confnum)
					? 0 : mm->replyto);
	    if (mm->bits & EMAIL_REPLY && (official_iemail || iedummyto[0]
					|| strlen(mm->too) > fromtolen))
		toot = iedummyto[0] ? iedummyto : "All";
	    else
		toot = mm->too;
	    if (kind > 0) {
		strcpy(ul.filename, st);
		Un4(ul.replyto, ribbon);
		strcpy(ul.from, mm->from);
		if (!toot[0] && cc->morebits & MULTI_NEWSGROUP)
		    strcpy(ul.to, "All");
		else
		    strcpy(ul.to, toot);
		strcpy(ul.subj, mm->subject);
		strcpy(ul.echotag, cc->shortname);
		Un4(ul.unix_date, mm->unixdate);
		if (mm->attached)
		    strcpy(ul.f_attach, mm->attached->tempname);
		j = cc->areabits;				/* no mask */
		Un2(ul.area_flags, j);
		if (mm->bluebits & UPL_NETMAIL) {
		    Un2(ul.netmail_attr, mm->attribits);	/* no mask */
		    Un2(ul.destzone, mm->zone);
		    Un2(ul.destnet, mm->net);
		    Un2(ul.destnode, mm->node);
		    Un2(ul.destpoint, mm->point);
		}
#ifdef USER_AREA_REPLY_XREF
		if (mm->mreplyee) {
		    if (stricmp(mm->confnum, mm->mreplyee->confnum))
			Confind(mm->mreplyee->confnum), j = confounded;
		    else
			j = -1;
		    ribbon = (Chex(mm->mreplyee) << 16) | (j + 1);
		} else
		    ribbon = (long) mm->personalink;
		Un4(&ul.user_area[3], ribbon);
		j = (ushort) mm->replyto;
		Un2(&ul.user_area[1], j);
		ul.user_area[0] = mm->replyto >> 16;	/* 24 bits! */
#else
		j = mm->mreplyee ? Chex(mm->mreplyee)
					: (ulong) mm->personalink >> 16;
		Un2(&ul.user_area[0], j);
#endif
		j = mm->bluebits;
		if (mm->attached)
		    j |= UPL_HAS_FILE;
		if ((ul.network_type = cc->net_type) == UPL_NET_INTERNET) {
		    if (toot != mm->too) {
			strcpy(ul.net_dest, mm->too);
			j |= UPL_NETMAIL;
		    }
		} else if ((p = messageID[i]) && p[0] && p[0] != '\x01'
				&& !(mm->bluebits & (UPL_PRIVATE | UPL_NETMAIL))
				&& cc->net_type == INF_NET_FIDONET) {
		    strcpy(ul.net_dest, "REPLY: ");
		    strcpy(ul.net_dest + 7, p);
		}
		Un2(ul.msg_attr, j);
	    } else if (!kind) {
		if (mm->bluebits & UPL_NETMAIL)
		    continue;
		strcpy(ur.fname, st);
		strcpy(ur.from, mm->from);
		strcpy(ur.to, toot);
		strcpy(ur.subj, mm->subject);
		strcpy(ur.echotag, cc->shortname);
		Un4(ur.unix_date, mm->unixdate);
		ur.flags = mm->bluebits & UPL_PRIVATE ? UPI_PRIVATE : 0;
		if (mm->bluebits & UPL_NO_ECHO)		/* unlikely */
		    ur.flags |= UPI_NO_ECHO;
	    } else {
		if (!(mm->bluebits & UPL_NETMAIL))
		    continue;
		strcpy(ner.fname, st);
		j = ribbon;
		Un2(ner.msg.reply, j);
		strcpy(ner.msg.from, mm->from);
		strcpy(ner.msg.to, toot);
		strcpy(ner.msg.subj, mm->subject);
		strcpy(ner.msg.date, mm->date);
		strcpy(ner.echotag, cc->shortname);
		Un4(ner.unix_date, mm->unixdate);
		Un2(ner.zone, mm->zone);
		Un2(ner.point, mm->point);
		Un2(ner.msg.dest, mm->node);
		Un2(ner.msg.destnet, mm->net);
		Un2(ner.msg.orig, hostnode);
		Un2(ner.msg.orig_net, hostnet);
/* DIRECT and IMMEDIATE bits don't match, but we'll just take our chances: */
		j = mm->attribits;
		if (mm->bluebits & UPL_PRIVATE)
		    j |= MSG_NET_PRIVATE;
		Un2(ner.msg.attr, j);
	    }
	    if (BWrite(hand, &both, bsize, 1) < 1) {
		DosErr("Could not write data in upload header file\n"
					"%s in your replies directory.", fame);
		goto ralph;
	    }
	}
    suck = anythingtosave = true;
ralph:
    if (hand) {
	ribbon = BClose(hand);
	if (suck && !ribbon) {
	    DosErr("Attempt to write upload header file\n"
					"%s has apparently failed.", fame);
	    suck = false;
	}
    }
    return suck;
#undef uph
#undef ur
#undef ner
#undef ulh
#undef ul
}


bool CreateReplyLock(void)
{
    if (!replylock && !(replylock = CreateLock(replydirinst))) {
	DosErr("Could not find or create replies\ndirectory %s;"
				" press %s\nto select a valid directory.",
				replydirinst, "Alt-D");
	return false;
    }
    return true;
}


bool WriteUPL(void)
{
    short i;
    char fame[13], nfame[13], ufame[13];
    BPTR hand = 0, oldcd;
    bool suck = false, somenormal = false, somenetmail = false, goodnet = true;

    SaveBookmarks();
    if (!CreateReplyLock()) {
	bad_upl = anythingtosave;
	return false;
    }
    if (qwk) {
	bad_upl = !(suck = WriteREP());
	return suck;
    }
    PortOff();
    oldcd = CurrentDir(replylock);
    strcpy(fame, packetname);
    strcat(fame, ".UPI");
    strcpy(nfame, packetname);
    strcat(nfame, ".NET");
    strcpy(ufame, packetname);
    strcat(ufame, ".UPL");
    for (i = 0; i < replies.messct; i++)
	if (!(replies.messes[i]->bluebits & UPL_INACTIVE)) {
	    if (replies.messes[i]->bluebits & UPL_NETMAIL)
		somenetmail = true;
	    else
		somenormal = true;
	}
    if (!somenormal && !somenetmail && !(pdq_exists | filerequests_exist)) {
	DeleteFile(fame);
	DeleteFile(ufame);
	DeleteFile(nfame);
	suck = true;
	bad_upl = false;
	goto ralph;
    }
    SetRegistration(registration);
    if (!useUPL) {
	if (suck = AnyShadeOfBlue(0, fame)) {
	    if (somenetmail)
		goodnet = AnyShadeOfBlue(-1, nfame);
	    else
		DeleteFile(nfame);
	}
    } else {
	DeleteFile(fame);
	DeleteFile(nfame);
	suck = true;
    }
    if (suck)
	suck = AnyShadeOfBlue(1, ufame);
  ralph:
    bad_upl = !suck;
    CurrentDir(oldcd);
    PortOn();
    return suck & goodnet;
}


void ReloadAnyKind(short kind, BHandle hand, bool oldstylechex)
{
    union {
	UPI_REC b_ur;
	NET_REC b_ner;
	UPL_REC b_ul;
    } both;
#define ur  both.b_ur
#define ner both.b_ner
#define ul  both.b_ul
    BPTR foot;
    struct Conf *cc, *rc;
    struct Mess *mm = null;
#ifdef RELOAD_UPI
    long rl, bsize = kind > 0 ? sizeof(UPL_REC) :
			    (kind < 0 ? sizeof(NET_REC) : sizeof(UPI_REC));
#else
    long rl, bsize = sizeof(UPL_REC);
#endif
    str from, too, subj;
#ifdef OLD_OLDFILENAMES
    char line[64], flame[14];
    short i, j;
#else
    char line[64];
    short i;
#endif
    ushort chex = 0;
    bool prebogon, chock;

    if (uplrecskip < 100 && kind > 0)
	bsize = STUNTED_UPL_REC_LEN;
    while ((rl = BRead(hand, &both, bsize, 1)) == 1 && !(chock =
				ReplyChoke(replies.messct, true)) && NEWZ(mm)) {
#ifdef RELOAD_UPI
	if (kind < 0)
	    from = ner.msg.from, too = ner.msg.to, subj = ner.msg.subj;
	else if (kind == 0)
	    from = ur.from, too = ur.to, subj = ur.subj;
	else
#endif
	{
	    from = ul.from;
	    subj = ul.subj;
	    too = ul.network_type && ul.net_dest[0] ? ul.net_dest : ul.to;
	}
	if (!(mm->from = BValloc(strlen(from) + 1))
			|| !(mm->too = BValloc(strlen(too) + 1))
			|| !(mm->subject = BValloc(strlen(subj) + 1)))
	    break;
	strcpy(mm->from, from);
	strcpy(mm->too, too);
	strcpy(mm->subject, subj);
	mm->datfseek = -1;
	mm->datflen = maxlong;
	mm->bits = DONTSTRIP;
	rc = null;
#ifdef RELOAD_UPI
	if (kind > 0) {
#endif
	    mm->bluebits = i2(ul.msg_attr);
	    mm->unixdate = i4(ul.unix_date);
	    mm->replyto = i4(ul.replyto);
	    if (mm->bluebits & UPL_NETMAIL) {	/* all zero otherwise */
		mm->zone = i2(ul.destzone);
		mm->net = i2(ul.destnet);
		mm->node = i2(ul.destnode);
		mm->point = i2(ul.destpoint);
		mm->attribits = i2(ul.netmail_attr);
	    }
	    from = ul.echotag;
	    too = ul.filename;
#ifdef USER_AREA_REPLY_XREF
	    mm->personalink = (adr) i4(ul.user_area + 3);
	    chex = (ushort) ((ulong) mm->personalink >> 16);
	    i = ((short) (ulong) mm->personalink) - 1;
	    if (i >= 0 && i < areaz.messct) {
		rc = areaz.confs[i];
		if (!mm->replyto)
		    mm->replyto = (ul.user_area[0] << 16)	/* 24 bits! */
					+ i2(ul.user_area + 1);
	    }
#else
	    chex = i2(ul.user_area + (oldstylechex ? 5 : 0));
	    mm->personalink = (adr) ((ulong) chex << 16);
#endif
	    if (ul.network_type == UPL_NET_FIDONET
				&& !strncmp(ul.net_dest, "REPLY: ", 7)) {
		register ustr id = ul.net_dest + 7;
		if (messageID[replies.messct] = NewPoolString(strlen(id)))
		    strcpy(messageID[replies.messct], id);
	    }
	    if (mm->bluebits & UPL_HAS_FILE && ul.f_attach[0]) {
		BPTR toe = RLock(ul.f_attach);
		if (toe) {
		    UnLock(toe);
		    if (NEW(mm->attached)) {
			strcpy(mm->attached->arrivename, ul.f_attach);
			strcpy(mm->attached->tempname, ul.f_attach);
			mm->attached->localpath[0] = '\0';
			mm->attached->localsource = false;
		    } /* else fail silently */
		}
	    }

#ifdef RELOAD_UPI
	} else if (kind < 0) {
	    mm->replyto = (ulong) i2(ner.msg.reply);
	    mm->attribits = i2(ner.msg.attr);
	    mm->bluebits = mm->attribits & MSG_NET_PRIVATE
				? UPL_NETMAIL | UPL_PRIVATE : UPL_NETMAIL;
	    mm->attribits &= ~MSG_NET_PRIVATE;
	    mm->unixdate = i4(ner.unix_date);
	    mm->zone = i2(ner.zone);
	    mm->net = i2(ner.msg.destnet);
	    mm->node = i2(ner.msg.dest);
	    mm->point = i2(ner.point);
	    from = ner.echotag;
	    too = ner.fname;
	} else {
	    mm->bits = DONTSTRIP;
	    mm->bluebits = ur.flags & UPI_PRIVATE ? UPL_PRIVATE : 0;
	    if (ur.flags & UPI_NO_ECHO)
		mm->bluebits |= UPL_NO_ECHO;
	    mm->unixdate = i4(ur.unix_date);
	    from = ur.echotag;
	    too = ur.fname;
	}
	if (kind < 0)
	    strcpy(mm->date, ner.msg.date);
	else
#endif
	{
	    struct DateStamp d;
	    ux2DS(&d, mm->unixdate);
	    DateString(&d, mm->date);
	}
	for (i = 0; i < areaz.messct; i++) {
	    cc = areaz.confs[i];
	    if (!stricmp(cc->shortname, from)) {
		strcpy(mm->confnum, cc->confnum);
		break;
	    }
	}
	if (i >= areaz.messct)
	    cc = null;
	if (official_iemail && cc && cc->morebits & INTERNET_EMAIL
					&& mm->bluebits & UPL_NETMAIL) {
	    mm->bluebits &= ~UPL_NETMAIL;
	    mm->bits |= EMAIL_REPLY;
	    mm->zone = mm->net = mm->node = mm->point = mm->attribits = 0;
	}
	prebogon = !cc || (cc->morebits & ALLREAD
				&& !(cc->areabits & INF_SCANNING));
	if (!cc || (mm->bluebits & UPL_NETMAIL
				&& !(cc->areabits & INF_NETMAIL))) {
	    cc = null;
	    if (mm->bluebits & UPL_NETMAIL) {
		bool twonets = false;
		for (i = 0; i < areaz.messct; i++)
		    if (areaz.confs[i]->areabits & INF_NETMAIL)
			if (cc) {
			    twonets = true;
			    break;
			} else
			    cc = areaz.confs[i];
		if (cc) {
		    strcpy(mm->confnum, cc->confnum);
		    Err("Found a netmail message that does not belong to\n"
				    "any netmail area.  Putting it into the "
				    "%s\nnetmail area.  Bad area name: %s",
				    (twonets ? "first" : "only"), from);
		}
	    }
	    if (!cc) {
		ubyte cn[8], et[22];
		ushort a = areaz.messct + 30000, aa;
		do
		    utoa(a++, cn);
		while (Confind(cn));
		strcpy(et, from);
		for (a = 0; et[a]; a++)
		    if (et[a] < ' ')
			et[a] = '?';
		sprintf(line, " ## UNKNOWN AREA \"%s\"", et);
		if (CreateNewArea(cn, from, line, &aa)) {
		    cc = areaz.confs[aa];
		    strcpy(mm->confnum, cn);
		    Err("Found a message that belongs to no known area."
				"  REMEMBER to\nmove it to a valid area before"
				" uploading your replies, or the\nBBS will"
				" discard it.  The unknown area name for"
				" message #%ld\nis \"%s\".",
				replies.messct + 1L, et);
		} /* else fail silently -- text probly won't load anyway */
	    }
	}
#ifdef OLD_OLDFILENAMES
	mm->bits |= PERSONALCOPY;	/* means filename is bogus */
	if (subj = strchr(too, '.')) {
	    *subj = 0;
	    mm->ixinbase = atol(subj + 1);
	    if (!prebogon) {
		MakeBWReplyFilename(flame, cc->confnum, mm->ixinbase);
		*subj = '.';
		if (!stricmp(flame, too))
		    mm->bits &= ~PERSONALCOPY;
	    }
	    *subj = '.';
	}
#else
	if (subj = strchr(too, '.'))
	    mm->ixinbase = atol(subj + 1);	/* suggested value only */
	mm->ixinbase = UniqueRix(mm);
#endif
	if (oldfilenames[replies.messct] = subj = BValloc(15)) {
	    strcpy(subj, too);
	    subj[13] = mm->ixinbase >> 8;
	    subj[14] = mm->ixinbase & 0xFF;
	} else
	    break;
#ifdef RELOAD_UPI
	if (kind) {
	    if (!rc) rc = cc;
	    if (rc && mm->replyto && kind > 0) {
#else
	{
	    if (!rc) rc = cc;
	    if (rc && mm->replyto) {
#endif
		for (i = 0; i < rc->messct; i++)
		    if (rc->messes[i]->ixinbase == mm->replyto)
			break;
		if (i < rc->messct && (Chex(rc->messes[i]) == chex || !chex)) {
		    mm->mreplyee = rc->messes[i];	/* for BWave  ^^^^^ */
		    mm->mreplyee->mreplyee = mm;
		    mm->bits |= MEREPLIED;
		    mm->mreplyee->bits |= MEREPLIED /* | MESEEN */;
		    if (mm->mreplyee->personalink) {
			mm->mreplyee->personalink->mreplyee = mm;
			mm->mreplyee->personalink->bits |= MEREPLIED;
		    }
		}
	    }
	}
	if (!(foot = OOpen(too)) || !LoadMessage(mm, foot, 0, false)) {
	    Err("Could not load text for message number\n"
				"%lu in old upload packet from file %s",
				replies.messct + 1L, too);
	    if (foot)
		Close(foot);
	    FreeReply(mm);
	    mm = null;
	    continue;
	}
	mm->datflen = loaded_size;
	if (foot)
	    Close(foot);
	foot = 0;
	if (!messageID[replies.messct])
	    messageID[replies.messct] =
				RetrieveReferences(&newsgroups[replies.messct]);
	if (!replies.messct)
	    readareaz.messct++;
	replies.messes[mm->replyat = replies.messct++] = mm;
	anythingtosave = true;
	if (cc->morebits & INTERNET_EMAIL && !official_iemail
					&& ValidInternet(mm->too))
	    mm->bits |= EMAIL_REPLY;
	mm = null;
	i = uplrecskip + STUNTED_UPL_REC_LEN - bsize;
	if (kind > 0 && i > 0 && BSeek(hand, i, OFFSET_CURRENT) < 0) {
	    SeekErr("reply message info");
	    rl = 0;
	    break;
	}
    }
    BClose(hand);
    if (mm || (rl && !mm && !chock)) {
	FreeReply(mm);
	Err("Could not load all\nreplies; no memory?");
    }
#ifdef OLD_OLDFILENAMES
    for (i = 0; i < replies.messct; i++) {
	mm = replies.messes[i];
	j = mm->ixinbase;
	mm->ixinbase = UniqueRix(mm);
	if (j != mm->ixinbase)
	    mm->bits |= PERSONALCOPY;
	if (!(mm->bits & PERSONALCOPY)) {
	    Vfree(oldfilenames[i]);
	    oldfilenames[i] = null;
	}
	mm->bits &= ~PERSONALCOPY;
    }
#endif

#undef ur
#undef ner
#undef ul
}


void ReloadDownloadReqs(void)
{
    char filename[13];
    short i, fc = 0;

    strcpy(filename, packetname);
    strcat(filename, ".REQ");
    if (!InhaleFile(filename))
	return;
    for (i = 0; i < requestedfiles_limit && i < fibize / 13; i++) {
	strncpy0(requestedfiles[i], fibuf + 13 * i, 13);
	Stranks(requestedfiles[i]);
	if (requestedfiles[i][0])
	    fc++;
    }
    if (!(anythingtosave |= filerequests_exist = fc > 0)) {
	if (fibize > 0)
	    Err("Download request file %s\nis invalid; deleting it.", filename);
	DeleteFile(filename);
    }
    ExhaleFile();
}


local bool CheckBool(str what)
{
    return !stricmp(what, "ON") || !stricmp(what, "YES")
				|| !stricmp(what, "TRUE");
}


local bool ReadOLCstuff(BHandle hand)
{
    char line[100];
    str lp, ep;
    bool areachange = false, otto, suck = false;
    short keyix = 0, filix = 0, macix = 0;
    struct Conf *cc;
    long a, last;

    while (BGetline(hand, line, 99) >= 0) {
	for (lp = line; *lp && isspace(*lp); lp++) ;
	if (*lp == ';')
	    continue;
	if (*lp == '[')
	    break;
	if (!(ep = strchr(lp, '=')))
	    continue;			/* be tolerant */
	for (*ep++ = 0; *ep && isspace(*ep); ep++) ;
	Stranks(lp);
	Stranks(ep);
	if (toupper(*lp) == 'M') {
	    if (!stricmp(lp, "MenuHotKeys"))
		bwhotkeys = CheckBool(ep), suck = true;
	    else if (!stricmp(lp, "MaxPacketSize"))
		bwpksize = (a = atol(ep)) < 0 || a > 32767 ? 0 : a, suck = true;
	    else if (!stricmp(lp, "Macro")) {
		if (macix >= 3)
		    continue;
		if (otto = !strnicmp(ep, "Auto,", 5))
		    for (ep += 5; *ep && isspace(*ep); ep++) ;
		switch(macix++) {
		    case 0: strncpy0(macro1, ep, 77); bwauto1 = otto; break;
		    case 1: strncpy0(macro2, ep, 77); bwauto2 = otto; break;
		    case 2: strncpy0(macro3, ep, 77); bwauto3 = otto; break;
		}
		suck = true;
	    }
	} else if (toupper(*lp) == 'E') {
	    if (!stricmp(lp, "ExpertMenus"))
		bwexpert = CheckBool(ep), suck = true;
	    else if (!stricmp(lp, "ExtendedInfo"))
		bwnoxlines = !CheckBool(ep), suck = true;
	} else if (toupper(*lp) == 'N') {
	    if (!stricmp(lp, "NumericExtensions"))
		bwnumericext = CheckBool(ep), suck = true;
	    else if (!stricmp(lp, "NewFileList")) {
		if (!stricmp(ep, "ANSI"))
		    bwflistype = 2;
		else if (!stricmp(ep, "TEXT"))
		    bwflistype = 1;
		else
		    bwflistype = 0;
		suck |= !!*ep;
	    }
	} else if (!stricmp(lp, "SkipUserMsgs"))
	    bwownmail = !CheckBool(ep), suck = true;
	else if (!stricmp(lp, "DoorGraphics"))
	    bwgraphics = CheckBool(ep), suck = true;
	else if (!stricmp(lp, "AreaChanges"))
	    areachange = CheckBool(ep), suck = true;
	else if (!stricmp(lp, "Keyword")) {
	    if (keyix >= 10)
		continue;
	    strncpy0(bwkeywords[keyix++], ep, 20);
	    suck = true;
	} else if (!stricmp(lp, "Filter")) {
	    if (filix >= 10)
		continue;
	    strncpy(bwfilters[filix++], ep, 20);
	    suck = true;
	} else if (!stricmp(lp, "Password")) {
	    if (!strnicmp(ep, "Both,", 5))
		passwordtype = 3, ep += 5;
	    else if (!strnicmp(ep, "Reader,", 7))
		passwordtype = 2, ep += 7;
	    else if (!strnicmp(ep, "Door,", 5))
		passwordtype = 1, ep += 5;
	    else
		passwordtype = 0;
	    bwpassword[0] = 0;
	    if (passwordtype) {
		for (ep += 5; *ep && isspace(*ep); ep++) ;
		strncpy0(bwpassword, ep, 20);
	    }
	    suck = true;
	}
    }
    pdq_difference = suck && PDQstateDiffers();
    if (suck && areachange) {
	for (a = 0; a < areaz.messct; a++) {
	    cc = areaz.confs[a];
	    cc->morebits &= ~DOOR_AREAPICKS;
	    if (cc->areabits & INF_SCANNING)
		cc->morebits |= DOOR_DROPPING;
	}
	last = areaz.messct - 1;
	while (*lp == '[' && (ep = strend(lp))[-1] == ']' && --ep > ++lp) {
	    *ep = 0;
	    a = last;
	    do {
		if (++a >= areaz.messct)
		    a = 0;
		if (!stricmp(lp, (cc = areaz.confs[a])->shortname)) {
		    last = a;
		    if (cc->areabits & INF_SCANNING)
			cc->morebits &= ~DOOR_DROPPING;
		    else
			cc->morebits |= DOOR_ADDING;
		    goto valid;
		}
	    } while (a != last);
	    Err("Nonexistent area specified in door\n"
				    "configuration file: \"%s\"", lp);
	  valid:
	    do {
		if (BGetline(hand, line, 99) < 0)
		    return true;
		for (lp = line; *lp && isspace(*lp); lp++) ;
		if (*lp != '[' && *lp != ';' && (ep = strchr(lp, '='))) {
		    for (*ep++ = 0; *ep && isspace(*ep); ep++) ;
		    Stranks(lp);
		    Stranks(ep);
		    if (!stricmp(lp, "Scan")) {
			if (!stricmp(ep, "Pers+All"))
			    if (cc->areabits & INF_TO_ALL)
				cc->morebits &= ~DOOR_ADDING;
			    else
				cc->morebits |= DOOR_ADDING_YOURS | DOOR_ADDING;
			else if (!stricmp(ep, "PersOnly"))
			    if (cc->areabits & INF_PERSONAL)
				cc->morebits &= ~DOOR_ADDING;
			    else {
				cc->morebits |= DOOR_ADDING_YOURS;
				cc->morebits &= ~DOOR_ADDING;
			    }
		    }
		}
	    } while (*lp != '[');
	}
	for (a = 0; a < areaz.messct; a++)
	    if (areaz.confs[a]->morebits & DOOR_AREAPICKS) {
		pdq_difference |= addsdrops = true;
		break;
	    }
    }
    return suck;
}


void ReloadPDQ(void)
{
    char filename[13], line[40];
    PDQ_HEADER *pdq;
    str tagspot;
    register struct Conf *cc;
    short a, last;
    BHandle hand;

    strcpy(filename, packetname);
    strcat(filename, ".OLC");
    if (hand = BOpen(filename, false)) {
	pdq_exists = false;	/* just in case */
	if (BGetline(hand, line, 39) < 0 || strnicmp(line, OLChead, OH_ENUFF)) {
	    Err("Door configuration file %s\n"
				"is invalid; deleting it.", filename);
	    DeleteFile(filename);
	} else
	    anythingtosave |= pdq_exists = ReadOLCstuff(hand);
	BClose(hand);
	if (!addsdrops && !PDQstateDiffers())
	    DeleteFile(filename), pdq_exists = false;
	if (pdq_exists)
	    return;
    }
    strcpy(filename, packetname);
    strcat(filename, ".PDQ");
    if (!InhaleFile(filename))
	return;
    pdq = (adr) fibuf;
    if (!(anythingtosave |= pdq_exists = fibize >= sizeof(pdq))) {
	Err("Door configuration file %s\nis invalid; deleting it.", filename);
	DeleteFile(filename);
    } else {
	Stranks(strcpy(macro1, pdq->macros[0]));
	Stranks(strcpy(macro2, pdq->macros[1]));
	Stranks(strcpy(macro3, pdq->macros[2]));
	for (a = 0; pdq->password[a]; a++)
	    bwpassword[a] = pdq->password[a] - 10;
	bwpassword[a] = 0;
	Stranks(bwpassword);
	passwordtype = pdq->passtype & 3;
	for (a = 0; a < 10; a++) {
	    Stranks(strcpy(bwkeywords[a], pdq->keywords[a]));
	    Stranks(strcpy(bwfilters[a], pdq->filters[a]));
	}
	a = i2(pdq->flags);
	bwhotkeys = !!(a & PDQ_HOTKEYS);
	bwexpert = !!(a & PDQ_XPERT);
	bwgraphics = !!(a & PDQ_GRAPHICS);
	bwownmail = !(a & PDQ_NOT_MY_MAIL);
	pdq_difference = PDQstateDiffers();
	if (a & PDQ_AREA_CHANGES) {
	    for (a = 0; a < areaz.messct; a++) {
		cc = areaz.confs[a];
		cc->morebits &= ~DOOR_AREAPICKS;
		if (cc->areabits & INF_SCANNING)
		    cc->morebits |= DOOR_DROPPING;
	    }
	    last = areaz.messct - 1;
	    for (tagspot = fibuf + sizeof(*pdq);
				tagspot < fibuf + fibize; tagspot += 21) {
		tagspot[20] = 0;
		a = last;
		do {
		    if (++a >= areaz.messct)
			a = 0;
		    if (!stricmp(tagspot, (cc = areaz.confs[a])->shortname)) {
			last = a;
			if (cc->areabits & INF_SCANNING)
			    cc->morebits &= ~DOOR_DROPPING;
			else
			    cc->morebits |= DOOR_ADDING;
			goto valid;
		    }
		} while (a != last);
		Err("Nonexistent area specified in door\n"
					"configuration file: %s", tagspot);
	      valid:
		;
	    }
	    for (a = 0; a < areaz.messct; a++)
		if (areaz.confs[a]->morebits & DOOR_AREAPICKS) {
		    pdq_difference |= addsdrops = true;
		    break;
		}
	}
	pdq_exists = true;
    }
    ExhaleFile();
    if (!pdq_difference)
	DeleteFile(filename);
}


void FuckWithEmptyHeaders(void)
{
    short a;
    for (a = 0; a < replies.messct; a++)
	if (!(replies.messes[a]->bluebits & UPL_INACTIVE))
	    return;
    WriteUPL();		/* either rewrites or deletes empty *.UP[IL] file */
}


void ReloadReplies(bool nounpack)	/* call CDed to replylock */
{					/* upload packet exists, or nounpack */
    union {
	UPI_HEADER b_upi;
	UPL_HEADER b_upl;
    } both;
#define uph both.b_upi
#define upl both.b_upl
#define ISIZE sizeof(UPI_HEADER) - 1	/* because of #define PAD_SIZES_EVEN */
#define LSIZE sizeof(UPL_HEADER)
    short i, initial = replies.messct;
#ifdef RELOAD_UPI
    char nfame[13], fame[290];
    BHandle nhand, hand;
#else
    char fame[290];
    BHandle hand;
    BPTR lk;
#endif

    anythingtosave = false;
    nowreloading = true;
    if (qwk) {
	ReloadQWKreplies(nounpack);
	nowreloading = false;
	return;
    }
    PortOff();
    if (!nounpack)
	JoinName(fame, uploaddir, packetname, ".NEW");
    if (nounpack || DoPack(fame, true, true)) {
	ReloadDownloadReqs();
	ReloadPDQ();
	strcpy(fame, packetname);
	strcat(fame, ".UPL");
	if ((hand = BOpen(fame, false)) && BRead(hand, &upl, LSIZE, 1) == 1) {
	    i = i2(upl.upl_header_len) - sizeof(UPL_HEADER);
	    uplrecskip = i2(upl.upl_rec_len) - STUNTED_UPL_REC_LEN;
	    if (i <= 0 || BSeek(hand, (long) i, OFFSET_CURRENT) >= 0) {
		ReloadAnyKind(1, hand, upl.reader_major < 2 ||
			      (upl.reader_major == 2 && upl.reader_minor < 20));
		goto finalexit;
	    }
	}
	strcpy(fame, packetname);
	strcat(fame, ".UPI");
#ifdef RELOAD_UPI
	strcpy(nfame, packetname);
	strcat(nfame, ".NET");
	if (!(hand = BOpen(fame, false)) || BRead(hand, &uph, ISIZE, 1) < 1) {
	    if (hand) {
		ReadWriteErr(null, fame, DREP);
		BClose(hand);
	    } else if (!filerequests_exist && !pdq_exists)
		Err("No valid reply files found in reply packet %s.NEW.",
						packetname);
	    goto finalexit;
	}
	nhand = BOpen(nfame, false);
    } else
	goto finalexit;
    if (nhand)
	ReloadAnyKind(-1, nhand, false);
    ReloadAnyKind(0, hand, false);
#else
	if (lk = RLock(fame)) {
	    UnLock(lk);
	    Err("File %s.UPL not found in reply packet.  This\n"
			"version of Q-Blue no longer supports reloading\n"
			"replies based on the obsolete *.UPI/*.NET format.",
			packetname);
	} else
	    Err("File %s.UPL not found in reply packet.", packetname);
    }
#endif
  finalexit:
    FuckWithEmptyHeaders();	/* in case PDQ/OLC or REQ was rejected */
    PortOn();
    nowreloading = false;
}


bool WriteFileREQ(void)
{
    char filename[13];
    BPTR ocd;
    BHandle hand;
    short i;
    bool ret = false;

    if (!CreateReplyLock())
	return false;
    filerequests_exist = false;
    ocd = CurrentDir(replylock);
    strcpy(filename, packetname);
    strcat(filename, ".REQ");
    for (i = 0; i < requestedfiles_limit; i++)
	if (requestedfiles[i][0])
	    break;
    if (i >= requestedfiles_limit)
	DeleteFile(filename);
    else if (!(hand = BOpen(filename, true)))
	ReadWriteErr("create download request", filename, DREP);
    else {
	ret = true;
	for (i = 0; i < requestedfiles_limit; i++)
	    if (requestedfiles[i][0])
		if (BWrite(hand, requestedfiles[i], 13, 1) < 1) {
		    ReadWriteErr("write download request",
						filename, DREP);
		    ret = false;
		    break;
		}
	BClose(hand);
	if (!(filerequests_exist = ret))
	    DeleteFile(filename);
    }
    FuckWithEmptyHeaders();
    CurrentDir(ocd);
    return ret;
}


bool WriteOLCtext(BPTR hand)
{
    static str no = "OFF", yes = "ON";		/* BWave compatible */
    ushort p = bwpassword[0] ? passwordtype : 0;

    if (FPrintf(hand, "%s\r\n", OLChead) < 0)
	return false;
    if (FPrintf(hand, "MenuHotKeys=%s\r\n", bwhotkeys ? yes : no) < 0)
	return false;
    if (FPrintf(hand, "ExpertMenus=%s\r\n", bwexpert ? yes : no) < 0)
	return false;
    if (FPrintf(hand, "SkipUserMsgs=%s\r\n", bwownmail ? no : yes) < 0)
	return false;
    if (FPrintf(hand, "ExtendedInfo=%s\r\n", bwnoxlines ? no : yes) < 0)
	return false;
    if (FPrintf(hand, "NumericExtensions=%s\r\n", bwnumericext ? yes : no) < 0)
	return false;
    if (FPrintf(hand, "DoorGraphics=%s\r\n", bwgraphics ? yes : no) < 0)
	return false;
    if (FPrintf(hand, "NewFileList=%s\r\n", bwflistype == 2 ? "ANSI"
					: (bwflistype ? "TEXT" : "OFF")) < 0)
	return false;
    if (FPrintf(hand, "MaxPacketSize=%luK\r\n", (ulong) bwpksize) < 0)
	return false;
    if (p && p < 4) {
	if (FPrintf(hand, "Password=%s,%s\r\n", p > 1 ? (p == 3 ? "Both"
					: "Reader") : "Door", bwpassword) < 0)
	    return false;
    } else if (FPrintf(hand, "Password=No\r\n") < 0)
	return false;
    if (macro1[0] && FPrintf(hand, "Macro=%s%s\r\n",
					bwauto1 ? "Auto," : "", macro1) < 0)
	return false;
    if (macro2[0] && FPrintf(hand, "Macro=%s%s\r\n",
					bwauto2 ? "Auto," : "", macro2) < 0)
	return false;
    if (macro3[0] && FPrintf(hand, "Macro=%s%s\r\n",
					bwauto3 ? "Auto," : "", macro3) < 0)
	return false;
    for (p = 0; p < 10; p++)
	if (bwkeywords[p][0] &&
			FPrintf(hand, "Keyword=%s\r\n", bwkeywords[p]) < 0)
	    return false;
    for (p = 0; p < 10; p++)
	if (bwfilters[p][0] && FPrintf(hand, "Filter=%s\r\n", bwfilters[p]) < 0)
	    return false;
    if (FPrintf(hand, "AreaChanges=%s\r\n", addsdrops ? yes : no) < 0)
	return false;
    if (!addsdrops)
	return true;
    for (p = 0; p < areaz.messct; p++) {
	register struct Conf *cc = areaz.confs[p];
	register ushort w, b = cc->morebits;
	if (b & (DOOR_ADDING | DOOR_ADDING_YOURS) || (!(b & DOOR_DROPPING)
					&& cc->areabits & INF_SCANNING)) {
	    if (b & DOOR_ADDING_YOURS && b & DOOR_ADDING)
		w = 2;
	    else if (b & DOOR_ADDING_YOURS)
		w = 1;
	    else if (b & DOOR_ADDING)
		w = 0;
	    else if (cc->areabits & INF_TO_ALL)
		w = 2;
	    else if (cc->areabits & INF_PERSONAL)
		w = 1;
	    else
		w = 0;
	    if (FPrintf(hand, "\r\n[%s]\r\nScan=%s\r\n", cc->shortname,
					w == 2 ? "Pers+All"
					: (w ? "PersOnly" : "All")) < 0)
		return false;
	}
    }
    return true;
}


void WritePDQ(bool anydifference)
{
    char filename[13], foo[40];
    PDQ_HEADER pdq;
    BPTR ocd, hand3;
    BHandle hand;
    short a;
    bool writefail = false;

    pdq_exists = false;
    if (!CreateReplyLock())
	return;
    ocd = CurrentDir(replylock);
    strcpy(filename, packetname);
    if (anydifference | addsdrops) {
	if (version3) {
	    strcat(filename, ".OLC");
	    if (!(hand3 = NOpen(filename)))
		ReadWriteErr("create door configuration", filename, DREP);
	    else {
		pdq_exists = WriteOLCtext(hand3);
		if (!Close(hand3) || !pdq_exists)
		    ReadWriteErr("writing door configuration", filename, DREP);
	    }
	} else {
	    strcat(filename, ".PDQ");
	    memset(&pdq, 0, sizeof(pdq));
	    for (a = 0; a < 10; a++) {
		strcpy(pdq.keywords[a], strupr(bwkeywords[a]));
		strcpy(pdq.filters[a], strupr(bwfilters[a]));
	    }
	    strcpy(pdq.macros[0], strupr(macro1));
	    strcpy(pdq.macros[1], strupr(macro2));
	    strcpy(pdq.macros[2], strupr(macro3));
	    strupr(bwpassword);
	    for (a = 0; bwpassword[a]; a++)
		pdq.password[a] = bwpassword[a] + 10;
	    pdq.password[a] = 0;
	    pdq.passtype = bwpassword[0] ? passwordtype : 0;
	    a = 0;
	    if (bwhotkeys) a |= PDQ_HOTKEYS;
	    if (bwexpert) a |= PDQ_XPERT;
	    if (bwgraphics) a |= PDQ_GRAPHICS;
	    if (!bwownmail) a |= PDQ_NOT_MY_MAIL;
	    if (addsdrops) a |= PDQ_AREA_CHANGES;
	    Un2(pdq.flags, a);
	    if (!(hand = BOpen(filename, true)))
		ReadWriteErr("create door configuration", filename, DREP);
	    else {
		if (BWrite(hand, &pdq, sizeof(pdq), 1) < 1)
		    writefail = true;
		else {
		    pdq_exists = true;
		    if (addsdrops)
			for (a = 0; a < areaz.messct; a++) {
			    register struct Conf *cc = areaz.confs[a];
			    if (cc->morebits & DOOR_ADDING ||
					    (cc->areabits & INF_SCANNING &&
					     !(cc->morebits & DOOR_DROPPING))) {
				memset(foo + 2, 0, 19);
				strcpy(foo, cc->shortname);
				if (BWrite(hand, foo, 21, 1) < 1) {
				    pdq_exists = false;
				    writefail = true;
				    break;
				}
			    }
			}
		}
		if (writefail)
		    ReadWriteErr("write door configuration", filename, DREP);
		BClose(hand);
	    }
	}
    }
    if (!pdq_exists)
	DeleteFile(filename);
    FuckWithEmptyHeaders();
    CurrentDir(ocd);
}
