/* create a new message with your editor -- this handles the intuition part of
it; the actual message massaging is in write.c */

#include <exec/memory.h>
#include <intuition/sghooks.h>
#include <intuition/intuition.h>
#include <graphics/gfxmacros.h>
#include <stdlib.h>
#include "qblue.h"
#include "pigment.h"
#include "semaphore.h"

#define CWINWIDTH  592
#define CWINHEIGHT 153
#define BUTTONTOP  -24
#define ARROWLEFT  -16

#define CONFCOLOR  TEXTCOLOR

#define __nETbITS   UPL_NETCRASH | UPL_NETFILE | UPL_NETKILL | UPL_NETHOLD
#define USERNETBITS (__nETbITS | UPL_NETIMMEDIATE | UPL_NETFRQ | UPL_NETDIRECT)

#define _SELd(gg) (gg).Flags & GFLG_SELECTED
#define NETCHECK(gg, bit) (_SELd(gg) ? (netbits |= bit) : (netbits &= ~bit))


short Conf2ix(struct Conf *cc);
void FixDefarrow(void);
void ScaleGadget(struct Gadget *gg);
void UnScaleGadget(struct Gadget *gg);
void StripIntuiMessages(struct MsgPort *mp, struct Window *win);
bool AllBlank(ustr line);
ustr NewPoolString(ushort length);

long CheckSemaphoreOverlap(bool dodirs);
short AskReedit(void);
void DoSetupIDCMP(char k);
void ConfigEditor(void), ConfigDirs(void);
void FixStringGad(struct Gadget *gg, struct Window *ww);
short AddUnknownArea(short initialarea);
short ListAreas(short initialarea, bool replies, bool addrop, bool minusone);
bool DoFileRequest(bool saving, bool special, str result,
				str deflt, str patter, str hail);

void DoEdit(struct Mess *thing, struct Mess *quotethis);
bool SaveReply(struct Mess *thing);
void FlushEdit(struct Mess *thing);
void Pontificate(struct Mess *mm);
bool ObjectToLocal(void);
void Privatoggle(void);
bool ParseNetAddress(str a);
void FillNetAddress(void);
void CopyNetAddress(struct Mess *dest, struct Mess *src);
void FormatNetAddress(str buf, ushort zone,
			ushort net, ushort node, ushort point);
bool ValidInternet(str where);
bool ExtractFromAddress(struct Mess *mm, str space, short len, bool backwards);
bool NormalizeInternet(ustr raw, ustr cooked, short len);

void TaglineWindow(bool timid, bool maintenance);
bool InitializeTagStuff(void);
void GetBWReplyFilename(str fame, struct Mess *mm);
bool ReplyChoke(ushort current, bool reloading);
str Capitalism(str name);
void GhostGagsRemember(struct Window *ww, bool off);
void GetMessageID(struct Mess *mm, short whichreply);

void IemailFlip(void);		/* forward */


import struct Menu mainu;
import struct QSemNode ourqsemnode;

import ustr *messageID, *newsgroups;
import char edit1command[], qwkid[];

import short netmailareanum, localmailareanum, iemailareanum, allow_noecho;
import ushort quotedefault, hostnetmailflags;
import ushort gatezone, gatenet, gatenode, gatepoint;
import bool addRe, uppitynames, uppitytitle, dubedit, newareawin, iemail_mksubj;
import bool allow_inverse_netkluge, deferflipb, notoowarned, no_forward;
import bool inverse_netkluge, official_iemail, pcboard_ie, pcboard_net, version3;
import bool qwkE_subs, pcbkluge, force_pcbkluge, searchlight_ie, ie_is_gated;


struct Mess *thing, *replyeee, *fixee, *carb;

/****  struct Mess *(replyptrs[REPLYLIMIT + 1]);  ****/

struct Conf *curconf, replies = {
    { /*  &replyptrs[0] */ null }, null, " (your replies and new messages)",
    0, 0, 0, 0, 0, 0, " \xFEr:\xFE", "", 0
};

struct Window *cwin, *cbitwin;

char oldflame[13], composetitle[70], iemailto[MAXIETOLEN];
str anynym;
local ustr lastbufnet = null;

bool firstedit, fixingreply, backwards;
bool lamecurconf, /* tagspossible, */ selfcarbon;

ushort ngflipbufpos, ngflipbufstart, which_reedit;
short defaultarea, fromend, quotype, publicareanum, oldnewarea, maxielen;
#ifdef REPLYPICKS
short edrepix;
#endif

ushort netbits, czone, cnet, cnode, cpoint;
char subchopchar = 0;


struct IntuiText cgtiemail = {
    TEXTCOLOR, FILLCOLOR, JAM2, 12, 2, null, "@-email", null
};

struct Gadget cgagiemail = {
    null, 15, 106, 80, 12, GFLG_GADGHCOMP | GFLG_DISABLED,
    GACT_RELVERIFY, GTYP_BOOLGADGET, &upborder, null,
    &cgtiemail, 0, null, 115, null
};


struct IntuiText cgtsave = {
    TEXTCOLOR, FILLCOLOR, JAM2, 12, 2, null, "Save", null
};

struct Gadget cgagsave = {
    &cgagiemail, CWINWIDTH - 527, BUTTONTOP, 80, 12,
    GFLG_RELBOTTOM | GFLG_GADGHCOMP | GFLG_DISABLED, GACT_RELVERIFY,
    GTYP_BOOLGADGET, &upborder, null, &cgtsave, 0, null, 110, null
};


struct IntuiText cgtattach = {
    TEXTCOLOR, FILLCOLOR, JAM2, 12, 2, null, "FAttach", null
};

struct Gadget cgagattach = {
    &cgagsave, CWINWIDTH - 95, 84, 80, 12, GFLG_GADGHCOMP, GACT_RELVERIFY,
    GTYP_BOOLGADGET, &upborder, null, &cgtattach, 0, null, 116, null
};


struct IntuiText cgtnetmail = {
    TEXTCOLOR, FILLCOLOR, JAM2, 12, 2, null, "Netmail", null
};

struct Gadget cgagnetmail = {
    &cgagattach, 140, 106, 80, 12, GFLG_GADGHCOMP | GFLG_DISABLED,
    GACT_RELVERIFY, GTYP_BOOLGADGET, &upborder, null,
    &cgtnetmail, 0, null, 113, null
};


struct IntuiText cgthandle = {
    TEXTCOLOR, FILLCOLOR, JAM2, 12, 2, null, "Handle", null
};

struct Gadget cgaghandle = {
    &cgagnetmail, CWINWIDTH - 95, 84, 80, 12, GFLG_GADGHCOMP, GACT_RELVERIFY,
    GTYP_BOOLGADGET, &upborder, null, &cgthandle, 0, null, 111, null
};


struct IntuiText cgtquoteverb = {
    TEXTCOLOR, FILLCOLOR, JAM2, 31, 3, null, "Verbatim", null
};

struct IntuiText cgtquotewrapxx = {
    TEXTCOLOR, FILLCOLOR, JAM2, 31, 3, null, "Wrap XX>", null
};

struct IntuiText cgtquoteaddxx = {
    TEXTCOLOR, FILLCOLOR, JAM2, 27, 3, null, " Add XX> ", null
};

struct IntuiText cgtquoteadd = {
    TEXTCOLOR, FILLCOLOR, JAM2, 27, 3, null, "  Add >  ", null
};

struct IntuiText cgtquotenone = {
    TEXTCOLOR, FILLCOLOR, JAM2, 31, 3, null, "  None  ", null
};

struct IntuiText *(cgtquotes[5]) = {
    &cgtquotenone, &cgtquoteadd, &cgtquoteaddxx, &cgtquotewrapxx, &cgtquoteverb
};

struct Gadget cgagquote = {
    &cgaghandle, 471, 22, 106, 12, GFLG_GADGHCOMP, GACT_RELVERIFY,
    GTYP_BOOLGADGET, &upcyclebor, null /* &downcyclebor */, null,
    0, null, 106, null
};


/* for PCBoard -- option "Password"? */

/* The no-echo option should be a separate gadget */

struct IntuiText cgtprivate = {
    TEXTCOLOR, FILLCOLOR, JAM2, 27, 3, null, " Private ", null
};

struct IntuiText cgtpublic = {
    TEXTCOLOR, FILLCOLOR, JAM2, 31, 3, null, " Public ", null
};

struct Gadget cgagpriv = {
    &cgagquote, 471, 44, 106, 12, GFLG_GADGHCOMP, GACT_RELVERIFY,
    GTYP_BOOLGADGET, &upcyclebor, null /* &downcyclebor */, &cgtpublic,
    0, null, 107, null
};


ubyte cbuffrom[NAMELEN + 2];

struct StringInfo cstrfrom = {
    cbuffrom, undospace, 0, NAMELEN, 0, 0, 0, 0, 0, 0, &stringex, 0, null
};

struct IntuiText clabelfrom = {
    LABELCOLOR, 0, JAM1, -60, 0, null, " From:", null
};

STRINGBORDER(cshortbox)

struct Gadget cgagfrom = {
    &cgagpriv, 75, 24, NAMELEN * 8 /* fontwid */ , 8,
    GFLG_STRINGEXTEND | GFLG_DISABLED | GFLG_TABCYCLE,
    GACT_RELVERIFY | GACT_STRINGLEFT, GTYP_STRGADGET,
    &cshortbox, null, &clabelfrom, 0, &cstrfrom, 100, null
};


ubyte cbufnet[40], standbycbufnet[40], gatecbufnet[40];

struct StringInfo cstrnet = {
    cbufnet, undospace, 0, 24, 0, 0, 0, 0, 0, 0, &stringex, 0, null
};

struct IntuiText clabelnet = {
    LABELCOLOR, 0, JAM1, -78, 0, null, "Address:", null
};

STRINGBORDER(cnetbox)

struct Gadget cgagnet = {
    &cgagfrom, 312, 108, 144, 8,
    GFLG_STRINGEXTEND | GFLG_DISABLED | GFLG_TABCYCLE,
    GACT_RELVERIFY | GACT_STRINGLEFT, GTYP_STRGADGET,
    &cnetbox, null, &clabelnet, 0, &cstrnet, 112, null
};


struct IntuiText cgtcancel = {
    TEXTCOLOR, FILLCOLOR, JAM2, 12, 2, null, "Cancel", null
};

struct Gadget cgagcancel = {
    &cgagnet, CWINWIDTH - 95, BUTTONTOP, 80, 12,
    GFLG_RELBOTTOM | GFLG_GADGHCOMP, GACT_RELVERIFY, GTYP_BOOLGADGET,
    &upborder, null, &cgtcancel, 0, null, 109, null
};

struct IntuiText cgttag = {
    TEXTCOLOR, FILLCOLOR, JAM2, 12, 2, null, "Tagline", null
};

struct Gadget cgagtag = {
    &cgagcancel, CWINWIDTH - 236, BUTTONTOP, 80, 12,
    GFLG_RELBOTTOM | GFLG_GADGHCOMP, GACT_RELVERIFY,
    GTYP_BOOLGADGET, &upborder, null, &cgttag, 0, null, 114, null
};

struct IntuiText cgtedit = {
    TEXTCOLOR, FILLCOLOR, JAM2, 12, 2, null, "Edit", null
};

struct Gadget cgagedit = {
    &cgagtag, CWINWIDTH - 377, BUTTONTOP, 80, 12,
    GFLG_RELBOTTOM | GFLG_GADGHCOMP, GACT_RELVERIFY, GTYP_BOOLGADGET,
    &upborder, null, &cgtedit, 0, null, 108, null
};


struct IntuiText cgtarea = {
    TEXTCOLOR, FILLCOLOR, JAM2, 12, 2, null, "Area:", null
};

struct Gadget cgagarea = {
    &cgagedit, 15, 84, 80, 12, GFLG_GADGHCOMP, GACT_RELVERIFY, GTYP_BOOLGADGET,
    &upborder, null, &cgtarea, 0, null, 103, null
};


ubyte cbufsubj[SUBJLEN + 5];

struct StringInfo cstrsubj = {
    cbufsubj, undospace, 0, SUBJLEN, 0, 0, 0, 0, 0, 0, &stringex, 0, null
};

struct IntuiText clabelsubj = {
    LABELCOLOR, 0, JAM2, -60, 0, null, "Subj.:", null
};

STRINGBORDER(clongbox)

struct Gadget cgagsubj = {
    &cgagarea, 75, 66, 448, 8, GFLG_STRINGEXTEND | GFLG_TABCYCLE,
    GACT_RELVERIFY | GACT_STRINGLEFT, GTYP_STRGADGET,
    &clongbox, null, &clabelsubj, 0, &cstrsubj, 102, null
};


ubyte cbuftoo[MAXIETOLEN + 3];

struct StringInfo cstrtoo = {
    cbuftoo, undospace, 0, NAMELEN, 0, 0, 0, 0, 0, 0, &stringex, 0, null
};

struct IntuiText clabeltoo = {
    LABELCOLOR, 0, JAM2, -60, 0, null, "   To:", null
};

struct IntuiText clabelgroups = {
    LABELCOLOR, 0, JAM2, -60, 0, null, "NGrps:", null
};

struct Gadget cgagtoo = {
    &cgagsubj, 75, 45, NAMELEN * 8 /* fontwid */ , 8,
    GFLG_STRINGEXTEND | GFLG_TABCYCLE,
    GACT_RELVERIFY | GACT_STRINGLEFT, GTYP_STRGADGET,
    &cshortbox, null, &clabeltoo, 0, &cstrtoo, 101, null
};


struct ExtNewWindow cneww = {
    24, 20, CWINWIDTH, CWINHEIGHT, 0, 1, IDCMP_GADGETUP | IDCMP_CLOSEWINDOW
		| IDCMP_RAWKEY | IDCMP_MENUPICK | IDCMP_INTUITICKS,
    WFLG_SMART_REFRESH | WFLG_CLOSEGADGET | WFLG_DEPTHGADGET
		| WFLG_DRAGBAR | WFLG_ACTIVATE | WFLG_NW_EXTENDED,
    null, null, composetitle, null, null, 0, 0, 0, 0, CUSTOMSCREEN, null
};

struct WhereWin cww = { &cneww };


struct Gadget cbgagimmed = {
    null, 275, 46, CHECKWID, 13, CHECKHILITE, CHECKACTION,
    GTYP_BOOLGADGET, &checkmarkno, &checkmarkyes, null, 0, null, 126, null
};

struct Gadget cbgagcrash = {
    &cbgagimmed, 145, 46, CHECKWID, 13, CHECKHILITE, CHECKACTION,
    GTYP_BOOLGADGET, &checkmarkno, &checkmarkyes, null, 0, null, 125, null
};

struct Gadget cbgagdirect = {
    &cbgagcrash, 15, 46, CHECKWID, 13, CHECKHILITE, CHECKACTION,
    GTYP_BOOLGADGET, &checkmarkno, &checkmarkyes, null, 0, null, 124, null
};

struct Gadget cbgagfatt = {
    &cbgagdirect, 405, 24, CHECKWID, 13, CHECKHILITE, CHECKACTION,
    GTYP_BOOLGADGET, &checkmarkno, &checkmarkyes, null, 0, null, 123, null
};

struct Gadget cbgagfreq = {
    &cbgagfatt, 275, 24, CHECKWID, 13, CHECKHILITE, CHECKACTION,
    GTYP_BOOLGADGET, &checkmarkno, &checkmarkyes, null, 0, null, 122, null
};

struct Gadget cbgagkill = {
    &cbgagfreq, 145, 24, CHECKWID, 13, CHECKHILITE, CHECKACTION,
    GTYP_BOOLGADGET, &checkmarkno, &checkmarkyes, null, 0, null, 121, null
};

struct Gadget cbgaghold = {
    &cbgagkill, 15, 24, CHECKWID, 13, CHECKHILITE, CHECKACTION,
    GTYP_BOOLGADGET, &checkmarkno, &checkmarkyes, null, 0, null, 120, null
};


struct ExtNewWindow cbitneww = {
    56, 69, CWINWIDTH - 64, 68, 0, 1,
    IDCMP_GADGETUP | IDCMP_CLOSEWINDOW | IDCMP_RAWKEY | IDCMP_INTUITICKS,
    WFLG_SMART_REFRESH | WFLG_CLOSEGADGET /* no ACTIVATE */
		| WFLG_DEPTHGADGET | WFLG_DRAGBAR | WFLG_NW_EXTENDED,
    null, null, "Netmail flags", null, null, 0, 0, 0, 0, CUSTOMSCREEN, null
};

struct WhereWin cbitww = { &cbitneww };


char fabname[32], fafile[COMMANDLEN], lastfafile[COMMANDLEN];
bool exceed_dos = false, falocal;

struct Gadget faexceeddosgag = {
    null, 16, 70, CHECKWID, 13, CHECKHILITE, CHECKACTION,
    GTYP_BOOLGADGET, &checkmarkno, &checkmarkyes, null, 0, null, 134, null
};


struct IntuiText fatokay = {
    TEXTCOLOR, FILLCOLOR, JAM2, 12, 2, null, "Okay", null
};

struct Gadget faokaygag = {
    &faexceeddosgag, 436, 68, 80, 12, GFLG_GADGHCOMP, GACT_RELVERIFY,
    GTYP_BOOLGADGET, &upborder, null, &fatokay, 0, null, 135, null
};

struct IntuiText fatclear = {
    TEXTCOLOR, FILLCOLOR, JAM2, 12, 2, null, "Clear", null
};

struct Gadget facleargag = {
    &faokaygag, 436, 48, 80, 12, GFLG_GADGHCOMP, GACT_RELVERIFY,
    GTYP_BOOLGADGET, &upborder, null, &fatclear, 0, null, 133, null
};

struct StringInfo fabstr = STRINF(fabname, 13);

STRINGBORDER(fabbox)

struct IntuiText fablabel = {
    LABELCOLOR, 0, JAM1, -140, 0, null, "Name to send as:", null
};

struct Gadget fabgag = {
    &facleargag, 158, 50, 240, 8, GFLG_STRINGEXTEND | GFLG_TABCYCLE,
    GACT_RELVERIFY | GACT_STRINGLEFT,
    GTYP_STRGADGET, &fabbox, null, &fablabel, 0, &fabstr, 132, null
};

struct StringInfo fafilestr = STRINF(fafile, 256);

STRINGBORDER(fafbox)

struct Gadget fafilegag = {
    &fabgag, 118, 36, 392, 8, GFLG_STRINGEXTEND | GFLG_TABCYCLE,
    GACT_RELVERIFY | GACT_STRINGLEFT,
    GTYP_STRGADGET, &fafbox, null, null, 0, &fafilestr, 131, null
};

struct IntuiText fata = {
    TEXTCOLOR, FILLCOLOR, JAM2, 12, 2, null, "File:", null
};

struct Gadget faagag = {
    &fafilegag, 16, 34, 80, 12, GFLG_GADGHCOMP, GACT_RELVERIFY, GTYP_BOOLGADGET,
    &upborder, null, &fata, 0, null, 130, null
};

struct ExtNewWindow faneww = {
    54, 60, 532, 100, 0, 1, IDCMP_GADGETUP | IDCMP_CLOSEWINDOW | IDCMP_RAWKEY,
    WFLG_NOCAREREFRESH | WFLG_CLOSEGADGET | WFLG_DRAGBAR | WFLG_DEPTHGADGET
		| WFLG_ACTIVATE | WFLG_NW_EXTENDED,
    null, null, "Select file to attach to message", null, null,
    0, 0, 0, 0, CUSTOMSCREEN, null
};

struct WhereWin faww = { &faneww };

struct Window *fawin;


/* ---------------------------------------------------------------------- */


void AbleGad(struct Gadget *gg, bool ability)
{
    if (ability)
	gg->Flags &= ~GFLG_DISABLED;
    else
	gg->Flags |= GFLG_DISABLED;
}


void AbleAddGad(struct Gadget *gg, struct Window *ww, bool ability)
{
    long gp;

    if (!!(gg->Flags & GFLG_DISABLED) == !ability)
	return;
    if (!ww) {
	AbleGad(gg, ability);
	return;
    }
    gp = RemoveGadget(ww, gg);
    AbleGad(gg, ability);
    if (gg->GadgetType & GTYP_STRGADGET)
	FixStringGad(gg, ww);
    else
	FixGreenGad(gg, ww);
    AddGadget(ww, gg, gp);
    if (!(gg->GadgetType & GTYP_STRGADGET)) {
	GhostOn(ww);
	RefreshGList(gg, ww, null, 1);
	GhostOff(ww);
    }
}


void ActivateGag(struct Gadget *gg, struct Window *ww)
{
    struct StringInfo *si = (adr) gg->SpecialInfo;
    short osbp = si->BufferPos;

    if (!(ww->Flags & WFLG_WINDOWACTIVE)) {
	ActivateWindow(ww);	/* DO NOT!! wait for verified activation */
	Delay(7);		/* value 7 selected by trial and error */
    }
    if (!osbp)
	si->BufferPos = strlen(si->Buffer);
    if (!ActivateGadget(gg, ww, null))
	si->BufferPos = osbp;
}


void Privatoggle(void)
{
    if (cgagpriv.Flags & GFLG_DISABLED)
	return;
    if (cgagpriv.GadgetText == &cgtpublic)
	ChangeGagText(&cgagpriv, cwin, &cgtprivate);
    else
	ChangeGagText(&cgagpriv, cwin, &cgtpublic);
    RefreshGList(&cgagpriv, cwin, null, 1);
    if (cgagpriv.GadgetText == &cgtprivate && !stricmp(cbuftoo, "ALL")) {
	cbuftoo[0] = 0;
	RefreshGList(&cgagtoo, cwin, null, 1);
	ActivateGag(&cgagtoo, cwin);
    }
}



void GhostCompose(bool off)
{
    if (cwin)
	GhostGagsRemember(cwin, off);
    if (cbitwin)
	GhostGagsRemember(cbitwin, off);
}


short PickMessArea(short initialarea)
{
    GhostCompose(true);
    defaultarea = initialarea;
    do {
	if (newareawin)
	    defaultarea = AddUnknownArea(defaultarea);
	if (!poseclosed)
	    defaultarea = ListAreas(defaultarea, true, false, false);
    } while (newareawin);
    if (wasescaped | poseclosed)
	defaultarea = initialarea;
    GhostCompose(false);
    return defaultarea;
}


void OpenBitsWin(void)
{
    if (cbitwin || !(hostnetmailflags & USERNETBITS))
	return;
    cbgaghold.TopEdge = cbgagkill.TopEdge = cbgagfreq.TopEdge
				= cbgagfatt.TopEdge = fakefight + 9 + checkoff;
    cbgagdirect.TopEdge = cbgagcrash.TopEdge = cbgagimmed.TopEdge
				= cbgaghold.TopEdge + checkspace;
    cbitneww.Height = cbgagdirect.TopEdge + checkspace + 2 - checkoff;
    cbitneww.TopEdge = cwin->TopEdge - cbitneww.Height - 3;
    if (cbitneww.TopEdge < 0)
	cbitneww.TopEdge = 0;
    cbitneww.LeftEdge = cwin->LeftEdge + 32;
    if (!(cbitwin = OpenBlueGagWin(&cbitww, &cbgaghold))) {
	WindowErr("setting netmail flags.");
	return;
    }
    SetMenuStrip(cbitwin, &mainu);
    GhostOn(cbitwin);
    ynwin = cbitwin;
    AbleGad(&cbgaghold,   hostnetmailflags & INF_CAN_HOLD);
    AbleGad(&cbgagkill,   hostnetmailflags & INF_CAN_KSENT);
    AbleGad(&cbgagfreq,   hostnetmailflags & INF_CAN_FREQ);
    AbleGad(&cbgagfatt,   hostnetmailflags & INF_CAN_ATTACH);
    AbleGad(&cbgagdirect, hostnetmailflags & INF_CAN_DIRECT);
    AbleGad(&cbgagcrash,  hostnetmailflags & INF_CAN_CRASH);
    AbleGad(&cbgagimmed,  hostnetmailflags & INF_CAN_IMM);
    /* this works because the INF_CAN_ definitions all match the UPL_NET ones: */
    netbits &= hostnetmailflags;				/* 2 noids */
    SeynLabel(&cbgaghold,   netbits & UPL_NETHOLD,      "1. Hold");
    SeynLabel(&cbgagkill,   netbits & UPL_NETKILL,      "2. Kill");
    SeynLabel(&cbgagfreq,   netbits & UPL_NETFRQ,       "3. F'Req.");
    SeynLabel(&cbgagfatt,   netbits & UPL_NETFILE,      "4. F'Att.");
    SeynLabel(&cbgagdirect, netbits & UPL_NETDIRECT,    "5. Direct");
    SeynLabel(&cbgagcrash,  netbits & UPL_NETCRASH,     "6. Crash");
    SeynLabel(&cbgagimmed,  netbits & UPL_NETIMMEDIATE, "7. Immediate");
    GhostOff(cbitwin);
}				/* OpenBitsWin */


void CloseBitsWin(void)
{
    if (!cbitwin)
	return;
    ClearMenuStrip(cbitwin);
    CloseBlueGagWin(cbitwin);
    cbitwin = null;
}


void PadRight(str s, short len)
{
    short l = strlen(s);

    for (s += l; l < len; l++)
	*s++ = ' ';
    *s = 0;
}


void WriteAreaName(void)
{
    char buf[80];

    sprintf(buf, "(%s) %s", curconf->confnum, LONGNAME(curconf));
    PadRight(buf, 47);
    MoveNominal(cwin->RPort, 111, cgagarea.TopEdge + font->tf_Baseline + 2);
    SetAPen(cwin->RPort, CONFCOLOR);
    SetBPen(cwin->RPort, backcolor);
    Text(cwin->RPort, buf, 47);		/* truncate long names */
}


void DeNewsgroupize(void)
{
    long p;
    ushort bpos = cstrtoo.BufferPos, dpos = cstrtoo.DispPos;
    if (cstrtoo.Buffer != cbuftoo) {
	p = RemoveGadget(cwin, &cgagtoo);
	cstrtoo.Buffer = cbuftoo;
	cstrtoo.BufferPos = ngflipbufpos;
	cstrtoo.DispPos = ngflipbufstart;
	cstrtoo.MaxChars = qwk ? 26 : fromtolen + 1;
	ngflipbufpos = bpos;
	ngflipbufstart = dpos;
	cgagtoo.Flags &= ~GFLG_DISABLED;
	cgagtoo.GadgetText = &clabeltoo;
	FixStringGad(&cgagtoo, cwin);
	AddGadget(cwin, &cgagtoo, p);
    } else if (cgagtoo.Flags & GFLG_DISABLED)
	AbleAddGad(&cgagtoo, cwin, true);
}


void EnterNewsgroup(void)
{
    short whichreply = fixingreply ? which_reedit : replies.messct;
    ushort bpos = cstrtoo.BufferPos, dpos = cstrtoo.DispPos;
    long p;

    if (newsgroups[whichreply] || (newsgroups[whichreply]
					= NewPoolString(SIGNATURELEN + 2))) {
	if (cwin)
	    p = RemoveGadget(cwin, &cgagtoo);
	cstrtoo.Buffer = newsgroups[whichreply];
	cstrtoo.BufferPos = ngflipbufpos;
	cstrtoo.DispPos = ngflipbufstart;
	cstrtoo.MaxChars = SIGNATURELEN;
	ngflipbufpos = bpos;
	ngflipbufstart = dpos;
	cgagtoo.Flags &= ~GFLG_DISABLED;	/* just in case */
	cgagtoo.GadgetText = &clabelgroups;
	if (cwin) {
	    FixStringGad(&cgagtoo, cwin);
	    AddGadget(cwin, &cgagtoo, p);
	}
    } else
	AbleAddGad(&cgagtoo, cwin, false);
}


void DeInternetize(void)
{
    char tempto[MAXIETOLEN];
    str e = strchr(composetitle, '[');
    long p;

    if (e) {
	e[-1] = '\0';
	SetWindowTitles(cwin, composetitle, (APTR) -1);
    }
    if (cstrtoo.Buffer == cbuftoo && cstrtoo.MaxChars > 61) {
	p = RemoveGadget(cwin, &cgagtoo);
	cstrtoo.MaxChars = qwk ? 26 : fromtolen + 1;
	strcpy(tempto, cbuftoo);
	strcpy(cbuftoo, iemailto);
	strcpy(iemailto, tempto);
	cbuftoo[cstrtoo.MaxChars - 1] = 0;
	FixStringGad(&cgagtoo, cwin);
	AddGadget(cwin, &cgagtoo, p);
    }
    if (thing->bits & EMAIL_REPLY && ie_is_gated) {
	strcpy(gatecbufnet, cbufnet);
	cbufnet[0] = 0;			/* !! risk clearing before remove */
	czone = cnet = cnode = cpoint = 0;
	AbleAddGad(&cgagnet, cwin, false);
    }
    thing->bits &= ~EMAIL_REPLY;
}				/* DeInternetize */


bool ReallyInNetmail(void)
{
    return qwk && thing->bits & EMAIL_REPLY ? ie_is_gated
				: !!(curconf->areabits & INF_NETMAIL);
}


local short SubjectLength(bool reassurance)
{
    /* QWK long subject kluges are okay under these conditions:       */
    /* Valence: always.  PCBoard15: not with kluge line netmail, but  */
    /* but always with native (PCBFido) netmail; not with iemail if   */
    /* not with netmail, risk it if no netmail.  OLMS or other QWKE:  */
    /* always.  MKQWK: fido netmail only.  JC-QWK: netmail & probably */
    /* iemail.  For those last two subjectlen is 25.                  */
    if (qwk) {
	ushort isem = thing->bits & EMAIL_REPLY, isnet = ReallyInNetmail();
	if (inverse_netkluge && (isnet || (iemail_mksubj && isem)))
	    return SUBJLEN - 1;		/* MKQWK/JC-QWK netmail subj kluge */
	else if (pcbkluge && !pcboard_net && (!reassurance || (isem &&
				!pcboard_ie && netmailareanum >= 0) || isnet))
	    return 25;			/* absence of PCBoard subj kluge */
    }
    return subjectlen;
}


local void CorrectSubGad(void)
{
    long p;
    short nl = SubjectLength(true);

    if (nl != cstrsubj.MaxChars - 1) {
	if (cwin)
	    p = RemoveGadget(cwin, &cgagsubj);
	cbufsubj[cstrsubj.MaxChars - 1] = subchopchar;
	cstrsubj.MaxChars = nl + 1;
	subchopchar = cbufsubj[nl];
	cbufsubj[nl] = 0;
	if (cwin) {
	    FixStringGad(&cgagsubj, cwin);
	    AddGadget(cwin, &cgagsubj, p);
	    RefreshGList(&cgagsubj, cwin, null, 1);
	}
    }
}


void JamNewSubject(ustr what, short len)	/* used by ExtractFromAddress */
{
    long p, z = cstrsubj.MaxChars - 1;
    if (cwin)
	p = RemoveGadget(cwin, &cgagsubj);
    strncpy0(cbufsubj, what, min(len, z));
    subchopchar = cbufsubj[z];
    if (cwin) {
	FixStringGad(&cgagsubj, cwin);
	AddGadget(cwin, &cgagsubj, p);
	RefreshGList(&cgagsubj, cwin, null, 1);
    }
}


local void CSettleIntoArea(void)
{
    long p;
    bool netting = ReallyInNetmail();

    CorrectSubGad();
    if (thing->attached && !(curconf->areabits & INF_HASFILE))
	Err("You have moved a message with an attached file into\n"
	    "a message area that does not support attachments.\n"
	    "Unless you either move the message back to an area\n"
	    "that permits attachments or remove the attachment,\n"
	    "your attachment will probably be discarded by the BBS.");
    if (curconf->areabits & INF_ANY_NAME && cgagfrom.Flags & GFLG_DISABLED) {
	p = RemoveGadget(cwin, &cgagfrom);
	if (curconf->areabits & INF_ANY_NAME && anynym[0])
	    strcpy(cbuffrom, anynym);
	else
	    strcpy(cbuffrom, myloginame);
	cbuffrom[fromtolen] = 0;
	cgagfrom.Flags &= ~GFLG_DISABLED;
	FixStringGad(&cgagfrom, cwin);
	AddGadget(cwin, &cgagfrom, p);
	AbleAddGad(&cgaghandle, cwin, true);
    }
    if (!qwk && !(curconf->areabits & INF_ANY_NAME)) {
	p = RemoveGadget(cwin, &cgagfrom);
	strncpy0(cbuffrom, (curconf->areabits & INF_ALIAS_NAME
					? myothername : myloginame), fromtolen);
	fromend = strlen(cbuffrom);
	cgagfrom.Flags |= GFLG_DISABLED;
	FixStringGad(&cgagfrom, cwin);
	AddGadget(cwin, &cgagfrom, p);
	AbleAddGad(&cgaghandle, cwin, false);
    }
    DeNewsgroupize();	/* <- will be immediately undone if still in a NG: */
    if (curconf->morebits & MULTI_NEWSGROUP)
	EnterNewsgroup();
    else if (curconf->morebits & NEWSGROUP)	/* not possible yet */
	AbleAddGad(&cgagtoo, cwin, false);
    else if (curconf->morebits & INTERNET_EMAIL && official_iemail
					&& !(thing->bits & EMAIL_REPLY))
	IemailFlip();
    else if (netting) {
	struct Conf *old = areaz.confs[oldnewarea];
	bool wasghost = cgagnet.Flags & GFLG_DISABLED;
	ustr otherbufnet = thing->bits & EMAIL_REPLY
					? gatecbufnet : standbycbufnet;
	bool wasempty = !cbufnet[0] && !otherbufnet[0];

	if (!lamecurconf && !(old->areabits & INF_NETMAIL)
					&& !(old->morebits & INTERNET_EMAIL))
	    publicareanum = oldnewarea;
	AbleAddGad(&cgagnet, cwin, true);
	if (thing->bits & EMAIL_REPLY)
	    lastbufnet = gatecbufnet;
	else {
	    if (wasghost && otherbufnet[0])
		strcpy(cbufnet, otherbufnet);
	    else {
		if (!cnet && thing->net && !fixingreply) {
		    czone = thing->zone;
		    cnet = thing->net;
		    cnode = thing->node;
		    cpoint = thing->point;
		}
		FillNetAddress();
	    }
	    lastbufnet = standbycbufnet;
	}
	RefreshGList(&cgagnet, cwin, null, 1);
	if (wasempty)
	    ActivateGag(&cgagnet, cwin);
	OpenBitsWin();
    } else if (!(cgagnet.Flags & GFLG_DISABLED)) {
	if (!lastbufnet)
	    lastbufnet = standbycbufnet;
	strcpy(lastbufnet, cbufnet);
	cbufnet[0] = 0;
	AbleAddGad(&cgagnet, cwin, false);
    }
    if (!netting)
	CloseBitsWin();
    if (curconf->areabits & (INF_NO_PUBLIC | INF_NO_PRIVATE)) {
	struct IntuiText *ii = (curconf->areabits & INF_NO_PRIVATE
						? &cgtpublic : &cgtprivate);
	if (cgagpriv.GadgetText != ii
			    || !(cgagpriv.Flags & GFLG_DISABLED)) {
	    ChangeGagText(&cgagpriv, cwin, ii);
	    p = RemoveGadget(cwin, &cgagpriv);
	    cgagpriv.Flags |= GFLG_DISABLED;
	    AddGadget(cwin, &cgagpriv, p);
	    GhostOn(cwin);
	    RefreshGList(&cgagpriv, cwin, null, 1);
	    GhostOff(cwin);
	}
    } else {
	struct IntuiText *old = cgagpriv.GadgetText;
	if (curconf->areabits & INF_NETMAIL || thing->bits & EMAIL_REPLY)
	    ChangeGagText(&cgagpriv, cwin, &cgtprivate);
	else if (qwk)
	    ChangeGagText(&cgagpriv, cwin, &cgtpublic);
	if (cgagpriv.GadgetText != old || cgagpriv.Flags & GFLG_DISABLED) {
	    p = RemoveGadget(cwin, &cgagpriv);
	    cgagpriv.Flags &= ~GFLG_DISABLED;
	    FixGreenGad(&cgagpriv, cwin);
	    AddGadget(cwin, &cgagpriv, p);
	    RefreshGList(&cgagpriv, cwin, null, 1);
	}
    }
/*  if (tagspossible) */
    AbleAddGad(&cgagattach, cwin,
               curconf->areabits & INF_HASFILE || thing->attached);
    AbleAddGad(&cgagtag, cwin, !(curconf->areabits & INF_NO_TAGLINE));
    lamecurconf = false;
}				/* CSettleIntoArea */


bool CNewArea(bool ask, bool netflippin)
{
    bool ret = true;
    if (ask) {
	defaultarea = PickMessArea(defaultarea);
	curconf = areaz.confs[defaultarea];
	if (poseclosed)
	    return false;
    }
    if (thing->bits & EMAIL_REPLY && !(curconf->morebits & INTERNET_EMAIL))
	DeInternetize();
    while (!(curconf->areabits & INF_POST)) {
	WriteAreaName();
	Err("This area is read-only;\nyou cannot post messages.");
	defaultarea = PickMessArea(oldnewarea);
	ASSERT(defaultarea >= 0 && defaultarea < areaz.messct);
	curconf = areaz.confs[defaultarea];
	if (poseclosed || (!ask && wasescaped)) {
	    ret = false;
	    break;
	}
	if (wasescaped)
	    break;
    }
    WriteAreaName();
    if (oldnewarea != defaultarea || netflippin || ask)
	CSettleIntoArea();
    oldnewarea = defaultarea;
    return ret;
}				/* CNewArea */


void IemailFlip(void)
{
    char tempto[MAXIETOLEN];
    long p;

    if (iemailareanum < 0)
	return;
    if (!(thing->bits & EMAIL_REPLY)) {
	DeNewsgroupize();
	if (!(curconf->areabits & INF_NETMAIL
				|| curconf->morebits & INTERNET_EMAIL))
	    publicareanum = defaultarea;
	defaultarea = iemailareanum;
	thing->bits |= EMAIL_REPLY;
	if (ie_is_gated /* && curconf->areabits & INF_NETMAIL */ ) {
	    if (cwin)
		p = RemoveGadget(cwin, &cgagnet);
	    if (cbufnet[0])
		strcpy(standbycbufnet, cbufnet);
	    if (gatecbufnet[0])
		strcpy(cbufnet, gatecbufnet);
	    else if (gatenet)
		FormatNetAddress(cbufnet, gatezone, gatenet, gatenode, gatepoint);
	    else
		cbufnet[0] = 0;
	    cgagnet.Flags &= ~GFLG_DISABLED;
	    if (cwin) {
		AddGadget(cwin, &cgagnet, p);
		RefreshGList(&cgagnet, cwin, null, 1);
	    }
	}
	if (!strchr(composetitle, '[')) {
	    *strend(composetitle) = ' ';
	    if (cwin)
		SetWindowTitles(cwin, composetitle, (APTR) -1);
	}
	if (!iemailto[0] && replyeee && !carb)
	    ExtractFromAddress(replyeee, iemailto, maxielen, backwards);
	if (cstrtoo.MaxChars <= 61) {
	    if (cwin)
		p = RemoveGadget(cwin, &cgagtoo);
	    cstrtoo.MaxChars = maxielen;
	    if (!ValidInternet(cbuftoo) || iemailto[0]) {
		strcpy(tempto, cbuftoo);
		strcpy(cbuftoo, iemailto);
		strcpy(iemailto, tempto);
	    }
	    if (cwin) {
		FixStringGad(&cgagtoo, cwin);
		AddGadget(cwin, &cgagtoo, p);
	    }
	}
    } else {
	DeInternetize();
	if (defaultarea == publicareanum || publicareanum < 0) {
	    CNewArea(true, true);	/* nowhere to go */
	    return;
	} else
	    defaultarea = publicareanum;
    }
    ASSERT(defaultarea >= 0 && defaultarea < areaz.messct);
    curconf = areaz.confs[defaultarea];
    if (cwin)
	CNewArea(false, true);
    lastbufnet = gatecbufnet;		/* in case CNewArea was not called */
}				/* IemailFlip */


local short WrapFind(short initial, short dodge)
{
    short a, ll = areaz.messct;
    ushort ff;

    for (a = (initial + 1) % ll; a != initial; a = (a + 1) % ll)
	if (a == dodge)
	    return -1;
	else {
	    ff = areaz.confs[a]->areabits;
	    if ((ff & (INF_NETMAIL | INF_POST | INF_NO_PRIVATE))
				== (INF_NETMAIL | INF_POST))
		return a;
	}
    return -1;
}


void NetmailFlip(void)
{
    if (thing->bits & EMAIL_REPLY) {
	DeInternetize();
	/* publicareanum = defaultarea; */
	defaultarea = netmailareanum;
    } else if (!(curconf->areabits & INF_NETMAIL)) {
	DeNewsgroupize();
	publicareanum = defaultarea;
	defaultarea = netmailareanum;
    } else {
	short a = WrapFind(defaultarea, netmailareanum);
	if (a >= 0) {
	    defaultarea = a;
	    OpenBitsWin();
	} else if (defaultarea == publicareanum || publicareanum < 0) {
	    CNewArea(true, true);	/* nowhere to go */
	    return;
	} else
	    defaultarea = publicareanum;
    }
    ASSERT(defaultarea >= 0 && defaultarea < areaz.messct);
    curconf = areaz.confs[defaultarea];
    CNewArea(false, true);
}				/* NetmailFlip */


void Handlize(void)
{
    static str namz[6] = {
	myloginame, myothername, null, myloginame, myothername, null
    };
    short whichname;
    str nn;

    if (!(curconf->areabits & INF_ANY_NAME))
	return;
    namz[2] = anynym;
    if (!stricmp(cbuffrom, myothername))
	whichname = 2;
    else if (anynym[0] && !stricmp(cbuffrom, anynym))
	whichname = 0;
    else
	whichname = 1;
    while (nn = namz[whichname], nn && (!*nn || !stricmp(cbuffrom, nn)))
	whichname++;
    if (nn && *nn) {
	strncpy0(cbuffrom, nn, fromtolen);
	if (uppitynames)
	    strupr(cbuffrom);
	RefreshGList(&cgagfrom, cwin, null, 1);
    }
}				/* Handlize */


local void DosReducePart(str out, str in, str inend, ushort limit)
{
    str startout, p;
    /* XXX  should convert upper ascii from ISO to OEM */
    for (startout = out; in < inend; in++) {
	if (!isspace(*in) && !strchr("[]+,;=\"<>\\.", *in))
	    *out++ = *in;
	else
	    *out++ = '_';
	if (out - startout > limit) {
	    out--;
	    if ((p = strchr(startout, '_')) && p < out)
		strncpy(p, p + 1, out - p);
	    else
		break;
	}
    }
    *out = '\0';
}				/* DosReducePart */


void DosReduce(str out, str in)
{
    str dit = strrchr(in, '.');
    if (!dit)
	dit = strend(in);
    DosReducePart(out, in, dit, 8);
    if (*dit && dit[1]) {
	strcat(out, ".");
	DosReducePart(strend(out), dit + 1, strend(dit), 3);
    }
}				/* DosReduce */


bool NameIsDosful(str name)
{
    str s, dot = null;
    for (s = name; *s; s++) {
	if (isspace(*s) || strchr("[]+,;=\"<>\\", *s))
	    return false;
	if (*s == '.')
	    if (dot || s - name > 8)
		return false;
	    else
		dot = s;
    }
    return dot ? s - dot <= 4 : s - name <= 8;
}				/* NameIsDosful */


local void FixSendFilename(str old)
{
    str dit;
    str name = FilePart(old);
    long q = RemoveGadget(fawin, &fabgag);

    if (!old[0])
	fabname[0] = 0;
    else if (exceed_dos && qwk) {
	char shortemp[32];
	str t = fabname;
	DosReduce(shortemp, name);
	if (!fabname[0] || !stricmp(shortemp, fabname)
			|| !stricmp(FilePart(lastfafile), fabname)) {
	    for (dit = name; *dit; dit++)
	       if (!isspace(*dit) && *dit != '\\')
		  *t++ = *dit;
	    *t = '\0';
	}
    } else
	DosReduce(fabname, name);
    AddGadget(fawin, &fabgag, q);
    RefreshGList(&fabgag, fawin, null, 1);
}				/* FixSendFilename */


local void ToggleExcessiveness(void)
{
    long q;
    char tn[32];

    if (!exceed_dos) {
	if (!NameIsDosful(fabname)) {	/* an empty name is considered dosful */
	    if (fafile[0])
		FixSendFilename(fafile);
	    else {
		strcpy(tn, fabname);
		FixSendFilename(tn);
	    }
	}
    } else if (fafile[0])
	FixSendFilename(fafile);	/* won't change fabname in some cases */
    q = RemoveGadget(fawin, &fabgag);
    fabstr.MaxChars = exceed_dos ? 31 : 13;
    AddGadget(fawin, &fabgag, q);
    SetWrMsk(fawin->RPort, 0);    /* do no physical drawing; FixSendFilename... */
    RefreshGList(&fabgag, fawin, null, 1);	/* already redrew it if needed  */
    SetWrMsk(fawin->RPort, -1);
}				/* ToggleExcessiveness */


local void GetAttachFilename(bool clear)
{
    long p = RemoveGadget(fawin, &fafilegag);
    if (clear || DoFileRequest(false, true, fafile, null, null,
					"Select file to attach")) {
	if (fafile[0] && !strchr(fafile + 1, ':')) {  /* path is not absolute */
	    BPTR lk = RLock(fafile);
	    if (lk) {
		char tf[COMMANDLEN];
		if (NameFromLock(lk, tf, COMMANDLEN))	 /* get absolute path */
		    strcpy(fafile, tf);
		UnLock(lk);
	    }
	}
	if (clear) {
	    if (!falocal) {
		falocal = true;
		AbleAddGad(&faagag, fawin, true);
		AbleGad(&fafilegag, true);
	    }
	    fafile[0] = 0;
	    FixSendFilename(fafile);
	} else
	    FixSendFilename(FilePart(fafile));
	strcpy(lastfafile, fafile);
	AddGadget(fawin, &fafilegag, p);
	RefreshGList(&fafilegag, fawin, null, 1);
    } else
	AddGadget(fawin, &fafilegag, p);
}				/* GetAttachFilename */


bool BadSendFilename(str tempname, bool quiet)
{
    char fame[14];
    BPTR fl, ocd;
    str s;

    if (!tempname) {
	StripString(&fafilegag, fawin);
	tempname = fabname;
    }
    if (!fafile[0] && !tempname[0])
	return false;
    if (!tempname[0] || (!exceed_dos && !NameIsDosful(tempname)))
	goto bad;
    for (s = tempname; *s; s++)
	if (*s == '/' || *s == '\\' || *s == ':' || isspace(*s) || *s == ',')
	    goto bad;
    if (qwk) {
	strncpy0(fame, qwkid, 8);
	strcat(fame, ".MSG");
	if (!stricmp(tempname, fame))		/* unnecessary check? */
	    goto conflict;
	if (!stricmp(tempname, "ATTXREF.DAT"))	/* ditto? */
	    goto conflict;
    } else {
	strncpy0(fame, packetname, 8);
	s = strend(fame);
	strcpy(s, version3 ? ".OLC" : ".PDQ");
	if (!stricmp(tempname, fame))
	    goto conflict;
	strcpy(s, ".REQ");
	if (!stricmp(tempname, fame))
	    goto conflict;
	strcpy(s, ".UPI");
	if (!stricmp(tempname, fame))
	    goto conflict;
	strcpy(s, ".NET");
	if (!stricmp(tempname, fame))
	    goto conflict;
	strcpy(s, ".UPL");
	if (!stricmp(tempname, fame))
	    goto conflict;
    }
    if (thing->attached && !stricmp(tempname, thing->attached->tempname))
	return false;	/* should test upper-ascii case-insensitively? */
    ocd = CurrentDir(replylock);
    fl = RLock(tempname);
    CurrentDir(ocd);
    if (fl) {
	UnLock(fl);
	goto conflict;
    }
    return false;
  bad:
    if (!quiet)
	Err("You must specify a valid name that the file will have when\n"
			"it arrives at its destination.  This must be a plain"
			" filename\nwith no path information; it must not"
			" contain any colon, slash,\nbackslash, comma, or whitespace"
			" characters.  For most BBSes,\nit must not exceed the"
			" MS-DOS 8.3 character limit on size.%s",
			tempname[0] ? "" : "\n\nIf you activate the file string"
			" gadget above this one and\npress return, a name"
			" should be selected automatically.");
    return true;
  conflict:
    if (!quiet)
	Err("The filename currently selected,\n%s\nconflicts with the names of "
			"other\nfiles in use in the reply packet.  You must\n"
			"choose a different filename in the \"Name\n"
			"to send as\" gadget.", tempname);
    return true;
}


local bool FileAttachIDCMP(struct IntuiMessage *im)
{
    ushort gid;
    char *p;

    if (im->Class == IDCMP_GADGETUP) {
	switch (((struct Gadget *) im->IAddress)->GadgetID) {
	  case 130:				/* File: button */
	    GetAttachFilename(false);
	    break;
	  case 131:				/* filename string */
	    if (GAGFINISH(im)) {
		if (falocal && stricmp(lastfafile, fafile))
		    FixSendFilename(FilePart(fafile));
		strcpy(lastfafile, FilePart(fafile));
		if (GAGCHAIN(im))
		    ActivateGadget(&fabgag, fawin, null);
	    }
	    break;
	  case 132:				/* name sent as */
	    for (p = fabname; isspace(*p); p++) ;
	    if (!p[0] && !falocal)
	        GetAttachFilename(true);	/* clear; ungray local name */
	    if (GAGFINISH(im)) {
		StripString(&fabgag, fawin);
		if (!qwk && BadSendFilename(null, false))
		    ActivateGadget(&fabgag, fawin, null);
		else if (GAGCHAIN(im) && im->Qualifier & SHIFTKEYS)
		    ActivateGadget(&fafilegag, fawin, null);
	    }
	    break;
	  case 133:				/* Clear button */
	    GetAttachFilename(true);
	    break;
	  case 134:				/* exceed dos checkmark */
	    CHECK(faexceeddosgag, exceed_dos);
	    ToggleExcessiveness();
	    break;
	  case 135:				/* Okay button */
	    StripString(&fabgag, fawin);
	    return !qwk || !BadSendFilename(null, false);
	    break;
	}
    } else if (im->Class == IDCMP_RAWKEY) {
	gid = KeyToAscii(im->Code, im->Qualifier);
	if (gid == ESC || gid == '\r' || gid == '\n' || gid == 'O') {
	    StripString(&fabgag, fawin);
	    return !qwk || !BadSendFilename(null, false);
	} else if (gid == '\t')
	    ActivateGag(falocal ? &fafilegag : &fabgag, fawin);
	else if (gid == 'C' || gid == 'F')
	    GetAttachFilename(gid == 'C');
	else if (gid == 'A' && !(faexceeddosgag.Flags & GFLG_DISABLED)) {
	    Seyn(&faexceeddosgag, exceed_dos = !exceed_dos);
	    ToggleExcessiveness();
	}
    } else if (im->Class == IDCMP_CLOSEWINDOW && im->IDCMPWindow == fawin) {
	StripString(&fabgag, fawin);
	return !qwk || !BadSendFilename(null, false);
    }
    return false;
}				/* FileAttachIDCMP */


void DoFileAttachment(void)
{
    if (!(curconf->areabits & INF_HASFILE) && !thing->attached)
	return;
    if (thing->attached) {
	strcpy(fabname, thing->attached->arrivename);
	strcpy(fafile, thing->attached->localpath);
	strcpy(lastfafile, fafile);
	falocal = thing->attached->localsource;
	if (qwk && !NameIsDosful(fabname))
	    exceed_dos = true;
    } else {
	fabname[0] = fafile[0] = '\0';
	falocal = true;
    }
    faagag.TopEdge = fakefight + 10;
    fafilegag.TopEdge = faagag.TopEdge + 2;
    fabgag.TopEdge = fafilegag.TopEdge + fakefight + 16;
    facleargag.TopEdge = fabgag.TopEdge - 2;
    faokaygag.TopEdge = facleargag.TopEdge + fakefight + 11;
    faexceeddosgag.TopEdge = faokaygag.TopEdge + 1 + checkoff;
    faneww.Height = faokaygag.TopEdge + fakefight + 13;
    AbleGad(&faexceeddosgag, qwk);
    AbleGad(&faagag, falocal);
    AbleGad(&fafilegag, falocal);
    if (!(fawin = OpenBlueGagWin(&faww, &faagag))) {
	WindowErr("attaching a file to a reply.");
	return;
    }
    ynwin = fawin;
    defarrow.LeftEdge = faokaygag.LeftEdge;
    defarrow.TopEdge = faokaygag.TopEdge;
    DrawBorder(fawin->RPort, &defarrow, ARROWLEFT, 0);
    SeynLabel(&faexceeddosgag, exceed_dos,
	      "Allow names exceeding MS-DOS 8.3 limits");
    GhostCompose(true);
    if (!fafile[0] && falocal)
	GetAttachFilename(false);
    EventLoop(&FileAttachIDCMP);
    if (falocal && fafile[0]) {
	BPTR at = RLock(fafile);
	if (!at)
	    DosErr("File %s not found", fafile);
	else {
	    BPTR ocd = CurrentDir(replylock);
	    UnLock(at);
	    if (!thing->attached)
		NEW(thing->attached);
	    if (thing->attached) {
/*		thing->attached->another = null;  */
		strcpy(thing->attached->arrivename, fabname);
		strcpy(thing->attached->localpath, fafile);
		thing->attached->localsource = falocal;
		if (qwk)
		    do {
			/* ulong rex = (rand() << 16) | rand(); */
			static ulong rex = 0;
			sprintf(thing->attached->tempname, "%08lx.atf", ++rex);
		    } while (BadSendFilename(thing->attached->tempname, true));
		else		/* already filtered by BadSendFilename */
		    strcpy(thing->attached->tempname, fabname);
	    } else
		Err("Could not attach file --\nout of memory");
	    CurrentDir(ocd);
	}
    } else if (!fabname[0]) {
	if (thing->attached)
	    FREE(thing->attached);
	thing->attached = null;
    } else if (!falocal && thing->attached) {		/* possibly renamed */
	strcpy(thing->attached->arrivename, fabname);
	if (!qwk)
	    strcpy(thing->attached->tempname, fabname);
	thing->attached->localpath[0] = 0;
	thing->attached->localsource = false;
    }
    GhostCompose(false);
    CloseBlueGagWin(fawin);
    fawin = null;
}				/* DoFileAttachment */


local void ShowNGerror(ushort offset)
{
    Err("The list of newsgroups in the \"To\" gadget\n"
			    "has a syntax error.  You must either correct\n"
			    "it or erase the contents of the gadget.");
    cstrtoo.BufferPos = offset;
    cstrtoo.DispPos = offset - 18;
    if ((signed) cstrtoo.DispPos < 0)
	cstrtoo.DispPos = 0;
    ActivateGag(&cgagtoo, cwin);
}


bool NewsgroupSyntaxError(ustr start)
{
    ustr where = start;
    ushort parendepth = 0;
#if 0
    bool isquoted = false, isslashed = false, isbracketed = false;
#endif
    bool wasspecial = true, spaced = true, dotted = false;

    StripString(&cgagtoo, cwin);
    if (!*where)
	return false;
    for (; *where; where++) {
	register ubyte c = *where;
#if 0      /* at least one newsgroup has existed with parens in its name. */
	if (parendepth > 0 || isbracketed || isquoted) {
	    if (isslashed)
		isslashed = false;
	    else if (c == '\\')
		isslashed = true;
	    else if (parendepth > 0 && c == '(')
		parendepth++;
	    else if (parendepth > 0 && c == ')')
		parendepth--;
	    else if (isbracketed && c == '[') {
		ShowNGerror(where - start);
		return true;
	    } else if (isbracketed && c == ']')
		isbracketed = false;
	    else if (isquoted && c == '"')
		isquoted = false;
	} else
#endif
	if (c == ' ' || c == '\t')
	    spaced = true;
	else if (!isprint(c)) {		/* no control chars or upper ascii */
	    ShowNGerror(where - start);
	    return true;
	} else {
	    if (c == '\\' || c == '<' || c == '>' || c == ')' || c == ']'
					|| c == '@' || c == ':' || c == ';') {
		ShowNGerror(where - start);
		return true;
#if 0
	    } else if (c == '(') {
		parendepth++;
		spaced = true;		/* wasspecial preserved */
	    } else if (c == '[') {
		if (!wasspecial) {
		    ShowNGerror(where - start);
		    return true;
		}
		wasspecial = false;
		isbracketed = true;
	    } else if (c == '"') {
		if (!wasspecial) {
		    ShowNGerror(where - start);
		    return true;
		}
		wasspecial = false;
		isquoted = true;
#else
	    } else if (c == '(' || c == '[' || c == '"') {
		ShowNGerror(where - start);
		return true;
#endif
	    } else if (c == '.') {
		if (wasspecial) {
		    ShowNGerror(where - start);
		    return true;
		}
		wasspecial = dotted = true;
	    } else if (c == ',') {
		if (wasspecial || !dotted) {
		    ShowNGerror(where - start);
		    return true;
		}
		wasspecial = true;
		dotted = false;
	    } else {			/* other punct, and alphanumeric */
		if (spaced && !wasspecial) {
		    ShowNGerror(where - start);
		    return true;
		}
		wasspecial = false;
	    }
	    spaced = false;
	}
    }
#if 0
    if (!dotted || wasspecial || parendepth || isquoted || isbracketed) {
#else
    if (!dotted || wasspecial) {
#endif
	ShowNGerror(where - start);
	return true;
    }
    return false;
}				/* NewsgroupSyntaxError */


void ActComposeGag(short fromwhere, struct IntuiMessage *im)
{
    static struct Gadget *(csgs[4]) = {
	&cgagfrom, &cgagtoo, &cgagsubj, &cgagnet
    };
    short shifty = (im->Qualifier & SHIFTKEYS ? -1 : 1);
    short nw = fromwhere + shifty;

    if (!GAGCHAIN(im) || nw > 3)
	return;
    while (csgs[nw &= 3]->Flags & GFLG_DISABLED && nw != fromwhere)
	if ((nw += shifty) > 3)
	    return;
    if (nw != fromwhere)		/* should always be true */
	ActivateGadget(csgs[nw], cwin, null);
}


bool DoComposeIDCMP(struct IntuiMessage *im)
{
    char k;
    ushort shifty = im->Qualifier & SHIFTKEYS;

    ynwin = cbitwin;
    switch (im->Class) {
      case IDCMP_CLOSEWINDOW:
	if (im->IDCMPWindow == cwin) {
	    FlushEdit(thing);
	    return true;
	} else if (im->IDCMPWindow == cbitwin)
	    CloseBitsWin();
	break;
      case IDCMP_RAWKEY:
	k = KeyToAscii(im->Code, im->Qualifier);
	if (im->Qualifier & ALTCTLKEYS) {
	    DoSetupIDCMP(k);
	    AbleAddGad(&cgagnetmail, cwin, netmailareanum >= 0);
	    AbleAddGad(&cgagiemail, cwin, iemailareanum >= 0);
#ifdef MUFFLE_TAG_GAG
	    if (!tagspossible) {
		tagspossible = InitializeTagStuff();
		AbleAddGad(&cgagtag, cwin, tagspossible &&
				!(curconf->areabits & INF_NO_TAGLINE));
	    }
#endif
	} else switch (k) {
	  case '\t':
	    if (!cbuffrom[0])		/* rare */
		ActivateGag(&cgagfrom, cwin);
	    else if (!cbuftoo[0])
		ActivateGag(&cgagtoo, cwin);
	    else if (!cbufnet[0] && !(cgagnet.Flags & GFLG_DISABLED))
		ActivateGag(&cgagnet, cwin);
	    else
		ActivateGag(&cgagsubj, cwin);
	    break;
	  case ESC:
	    if (cbitwin && im->IDCMPWindow != cwin) {
		CloseBitsWin();
		break;
	    } /* else fall through to "Close": */
	  case 'C':
	    FlushEdit(thing);
	    return true;
	  case 'S':
	    if (cgagsave.Flags & GFLG_DISABLED)
		break;
	    return SaveReply(thing);
	  case 'E':
	    DoEdit(thing, replyeee);
	    break;
	  case 'A':
	    CNewArea(true, false);
	    break;
	  case 'H':
	    Handlize();
	    break;
	  case 'N':
	    if (netmailareanum >= 0)
		NetmailFlip();
	    break;
	  case '@':
	    IemailFlip();
	    break;
	  case 'P':
	    Privatoggle();
	    break;
	  case 'T':
	    if (!(curconf->areabits & INF_NO_TAGLINE))
		TaglineWindow(false, false);
	    break;
	  case 'Q':
	    if (!(cgagquote.Flags & GFLG_DISABLED)) {
		quotype = (quotype + (shifty ? 4 : 1)) % 5;
		ChangeGagText(&cgagquote, cwin, cgtquotes[quotype]);
		RefreshGList(&cgagquote, cwin, null, 1);
	    }
	    break;
	  case 'F':
	    DoFileAttachment();
	    break;
	  case '\n': case '\r':
	    if (firstedit && !carb)
		DoEdit(thing, replyeee);
	    else
		return SaveReply(thing);
	    break;
	  case '1':
	    if (cbitwin && hostnetmailflags & INF_CAN_HOLD)
		Seyn(&cbgaghold, (netbits ^= UPL_NETHOLD) & UPL_NETHOLD);
	    break;
	  case '2':
	    if (cbitwin && hostnetmailflags & INF_CAN_KSENT)
		Seyn(&cbgagkill, (netbits ^= UPL_NETKILL) & UPL_NETKILL);
	    break;
	  case '3':
	    if (cbitwin && hostnetmailflags & INF_CAN_FREQ)
		Seyn(&cbgagfreq, (netbits ^= UPL_NETFRQ) & UPL_NETFRQ);
	    break;
	  case '4':
	    if (cbitwin && hostnetmailflags & INF_CAN_ATTACH)
		Seyn(&cbgagfatt, (netbits ^= UPL_NETFILE) & UPL_NETFILE);
	    break;
	  case '5':
	    if (cbitwin && hostnetmailflags & INF_CAN_DIRECT)
		Seyn(&cbgagdirect, (netbits ^= UPL_NETDIRECT) & UPL_NETDIRECT);
	    break;
	  case '6':
	    if (cbitwin && hostnetmailflags & INF_CAN_CRASH)
		Seyn(&cbgagcrash, (netbits ^= UPL_NETCRASH) & UPL_NETCRASH);
	    break;
	  case '7':
	    if (cbitwin && hostnetmailflags & INF_CAN_IMM)
		Seyn(&cbgagimmed, (netbits ^= UPL_NETIMMEDIATE)
					& UPL_NETIMMEDIATE);
	    break;
	}
	break;
      case IDCMP_GADGETUP:
	switch (((struct Gadget *) im->IAddress)->GadgetID) {
	  case 100:				/* From: string */
	    if (!GAGFINISH(im)) break;
	    if (!cbuffrom[0]) {
		strncpy0(cbuffrom, (qwk || curconf->areabits & INF_ALIAS_NAME
				  ? myothername : myloginame), fromtolen);
		if (uppitynames)
		    strupr(cbuffrom);
		GhostOn(cwin);
		RefreshGList(&cgagfrom, cwin, null, 1);
		GhostOff(cwin);
	    } else if (uppitynames) {
		strupr(cbuffrom);
		RefreshGList(&cgagfrom, cwin, null, 1);
	    }
	    ActComposeGag(0, im);
	    break;
	  case 101:				/* To: string */
	    if (!GAGFINISH(im)) break;
	    if (cstrtoo.Buffer != cbuftoo) {		/* newsgroups */
		if (NewsgroupSyntaxError(cstrtoo.Buffer))
		    break;
	    } else if (!cbuftoo[0] && cgagpriv.GadgetText != &cgtprivate) {
		strcpy(cbuftoo, uppitynames ? "ALL" : "All");
		RefreshGList(&cgagtoo, cwin, null, 1);
	    } else if (thing->bits & EMAIL_REPLY) {
		if (cbuftoo[0] && !ValidInternet(cbuftoo)) {
		    Err("Internet email must be sent to a\nvalid address"
					" in \"name@site\" form.");
		    ActivateGag(&cgagtoo, cwin);
		    break;
		} else {
		    char ttoo[MAXIETOLEN];
		    if (NormalizeInternet(cbuftoo, ttoo, MAXIETOLEN)) {
			strcpy(cbuftoo, ttoo);
			RefreshGList(&cgagtoo, cwin, null, 1);
		    }
		}
	    } else if (uppitynames) {
		strupr(cbuftoo);
		RefreshGList(&cgagtoo, cwin, null, 1);
	    }
	    ActComposeGag(1, im);
	    break;
	  case 102:				/* Subject: string */
	    if (!GAGFINISH(im)) break;
	    if (cstrsubj.DispPos || uppitytitle) {
		cstrsubj.DispPos = cstrsubj.BufferPos = 0;
		if (uppitytitle)
		    strupr(cbufsubj);
		RefreshGList(&cgagsubj, cwin, null, 1);
	    }
	    ActComposeGag(2, im);
	    break;
	  case 103:				/* Area: */
	    CNewArea(true, false);
	    break;
	  case 106:				/* Quote cycle */
	    quotype = (quotype + (shifty ? 4 : 1)) % 5;
	    ChangeGagText(&cgagquote, cwin, cgtquotes[quotype]);
	    RefreshGList(&cgagquote, cwin, null, 1);
	    break;
	  case 107:				/* Pub/Private cycle */
	    Privatoggle();
	    break;
	  case 108:				/* Edit button */
	    DoEdit(thing, replyeee);
	    break;
	  case 109:				/* Cancel button */
	    FlushEdit(thing);
	    return true;
	  case 110:				/* SAVE button */
	    return SaveReply(thing);
	  case 111:				/* Handle button */
	    Handlize();
	    break;
	  case 112:				/* Net address */
	    if (!GAGFINISH(im)) break;
	    StripString(&cgagnet, cwin);
	    if (ParseNetAddress(cbufnet)) {
		if (ObjectToLocal())
		    ActivateGag(&cgagnet, cwin);
		else
		    ActComposeGag(3, im);
	    }
	    break;
	  case 113:				/* Netmail button */
	    if (netmailareanum >= 0)
		NetmailFlip();
	    break;
	  case 114:				/* Tagline button */
	    if (!(curconf->areabits & INF_NO_TAGLINE))
		TaglineWindow(false, false);
	    break;
	  case 115:				/* @-email button */
	    IemailFlip();
	    break;
	  case 116:				/* FAttach button */
	    DoFileAttachment();
	    break;
	  case 120:
	    if (hostnetmailflags & INF_CAN_HOLD)
		NETCHECK(cbgaghold, UPL_NETHOLD);
	    break;
	  case 121:
	    if (hostnetmailflags & INF_CAN_KSENT)
		NETCHECK(cbgagkill, UPL_NETKILL);
	    break;
	  case 122:
	    if (hostnetmailflags & INF_CAN_FREQ)
		NETCHECK(cbgagfreq, UPL_NETFRQ);
	    break;
	  case 123:
	    if (hostnetmailflags & INF_CAN_ATTACH)
		NETCHECK(cbgagfatt, UPL_NETFILE);
	    break;
	  case 124:
	    if (hostnetmailflags & INF_CAN_DIRECT)
		NETCHECK(cbgagdirect, UPL_NETDIRECT);
	    break;
	  case 125:
	    if (hostnetmailflags & INF_CAN_CRASH)
		NETCHECK(cbgagcrash, UPL_NETCRASH);
	    break;
	  case 126:
	    if (hostnetmailflags & INF_CAN_IMM)
		NETCHECK(cbgagimmed, UPL_NETIMMEDIATE);
	    break;
	}
	break;
      case IDCMP_MENUPICK:
	if (MENUNUM(im->Code) == 3) {	/* should be always */
	    DoSetupIDCMP((char) (ITEMNUM(im->Code) + 0x80));
	    AbleAddGad(&cgagnetmail, cwin, netmailareanum >= 0);
	    AbleAddGad(&cgagiemail, cwin, iemailareanum >= 0);
#ifdef MUFFLE_TAG_GAG
	    if (!tagspossible) {
		tagspossible = InitializeTagStuff();
		AbleAddGad(&cgagtag, cwin, tagspossible &&
				!(curconf->areabits & INF_NO_TAGLINE));
	    }
#endif
	}
	break;
    }
    if (poseclosed) {
	FlushEdit(thing);
	return true;
    } else
	return false;
}				/* DoComposeIDCMP */


bool SetUpToCompose(struct Mess *editee, bool fixing, struct Mess *carbon)
{
    static short lasta = 0;
    short i;
    struct Conf *cc;

    if (editee && readareaz.confs[whicha] == &bullstuff)
	return false;
    StopScroll();
    if (!edit1command[0] || !editoutfile[0]) {
	Err("You have not configured a text editor for\n"
		"composing messages with.  Please set up\n"
		"the commands and filenames now.");
	ConfigEditor();
	if (!edit1command[0] || !editoutfile[0])
	    return false;
    }
    if ((!replylock && !replydir[0]) || !uploaddir[0]) {
	Err("Your configuration of directories is missing\n"
		"either a reply directory or an upload directory.\n"
		"Please specify the missing one(s) now.");
	ConfigDirs();
	if ((!replylock && !replydir[0]) || !uploaddir[0])
	    return false;
    }
    if (CheckSemaphoreOverlap(false) & SOF_EDITORINUSE) { /* shouldn't happen */
	Err("Another Q-Blue process is currently creating\n"
		"a message using the same temporary filename(s).\n"
		"Either finish that or change your editor setup.");
	return false;		/* can't happen any more? */
    }
    if (editee && editee->bits & PERSONALCOPY)
	editee = editee->personalink;	/* no mreplyees pointing to personals */
    if (carbon && carbon->bits & PERSONALCOPY)
	carbon = carbon->personalink;	/* not even for carbons */
    if (editee && !fixing && editee->mreplyee) {	/* carbon => !editee */
	if (!(i = AskReedit()))
	    return false;		/* cancel */
	if (i == 2) {
	    fixing = true;		/* re-edit */
	    editee = editee->mreplyee;
	}				/* else add second reply */
    }
    if ((!fixing || carbon) && ReplyChoke(replies.messct, false))
	return false;
    if (!NEWZ(thing) || !(thing->from = BValloc(NAMELEN))
			|| !(thing->too = BValloc(MAXIETOLEN))
			|| !(thing->subject = BValloc(SUBJLEN))) {
	FreeReply(thing);
	Err("Can't create message -- no memory.");
	return false;
    }
    iemailto[0] = 0;
    ngflipbufstart = ngflipbufpos = 0;
    lastfafile[0] = 0;

    if (carb = carbon) {
#ifdef REPLYPICKS
	edrepix = -1;
#endif
	if (!(selfcarbon = fixing) && no_forward)
	    Err("Warning: the mail packet indicates that this BBS\ndoes not"
				" approve of forwarding copied messages.");
	replyeee = thing->mreplyee = fixing ? carbon->mreplyee : null;
	fixingreply = false;
	fixee = carbon;
	/* thing->bits |= fixee->bits & EMAIL_REPLY; */
	curconf = Confind(carbon->confnum);
	if (curconf && curconf->morebits & INTERNET_EMAIL && (official_iemail ||
					ExtractFromAddress(carbon, null, 0, 0)))
	    thing->bits |= EMAIL_REPLY;
	netbits = fixee->attribits;
	if (fixing && carbon->attached)
	    Err("If you want the copy to have a file\n"
		    "attached like the original, you have to\n"
		    "reattach it manually.  Sorry about that.%s",
		    qwk ? "" : "\n\nAnd unfortunately, in the Blue Wave\n"
		    "format, there isn't any known safe way for two\n"
		    "messages to have the same filename attached.\n"
		    "You have to use a different name for the copy.");
    } else if (fixingreply = fixing) {
#ifdef REPLYPICKS
	edrepix = whichm;
#endif
	which_reedit = editee->replyat;			/* a small integer */
	replyeee = thing->mreplyee = editee->mreplyee;
	fixee = editee;
	netbits = fixee->attribits;
	thing->bits |= fixee->bits & EMAIL_REPLY;
	thing->personalink = fixee->personalink;	/* magic chex value */
	if (fixee->attached)
	    if (!NEW(thing->attached)) {
		FreeReply(thing);
		Err("Can't re-edit message with\nattached file -- no memory");
		return false;
	    } else
		*thing->attached = *fixee->attached;
    } else {
#ifdef REPLYPICKS
	edrepix = -1;
#endif
	replyeee = fixee = thing->mreplyee = editee;
	netbits = 0;		/* no George-like default bits */
	if (editee)
	    GetMessageID(editee, replies.messct);
	else
	    messageID[replies.messct] = null;	/* just in case */
    }
    if (replyeee)
	Pontificate(replyeee);
    czone = cnet = cnode = cpoint = 0;
    if (fixee) {
	if (replyeee && !fixee->net)
	    CopyNetAddress(fixee, replyeee);
	CopyNetAddress(thing, fixee);
	if (carbon && !fixing)
	    fixee = null;
    }
    if (carbon ? selfcarbon && carbon->bits & MEREPLIED
			: (fixing ? editee->bits & MEREPLIED : !!editee))
	thing->bits |= MEREPLIED;
/*  if (fixing && editee->bits & REPLY_HAS_TAG)
	thing->bits |= REPLY_HAS_TAG;  */
    thing->datflen = maxlong;
    lamecurconf = false;
    if (carbon) {
	curconf = Confind(carbon->confnum);
	thing->replyto = carbon->replyto;
    } else if (editee) {
	curconf = Confind(editee->confnum);
	thing->replyto = (fixing ? editee->replyto : editee->ixinbase);
    } else if (whicha >= 0 && whichm >= 0
				&& readareaz.confs[whicha] != &bullstuff) {
	curconf = Confind(readareaz.confs[whicha]->messes[whichm]->confnum);
	lamecurconf = readareaz.confs[whicha] == &replies;
    } else
	curconf = null;
    if (!curconf) {
	for (lasta = 0; lasta < areaz.messct; lasta++) {
	    curconf = areaz.confs[lasta];
	    if (curconf->areabits & INF_POST && !(curconf->areabits & INF_NETMAIL)
					&& !(curconf->morebits & INTERNET_EMAIL))
		break;
	}
	if (lasta >= areaz.messct) {
	    for (lasta = 0; lasta < areaz.messct; lasta++) {
		curconf = areaz.confs[lasta];
		if (curconf->areabits & INF_POST)
		    break;
	    }
	    if (lasta >= areaz.messct) {
		Err("All message areas are marked Read Only.\n"
				"You cannot post messages on this BBS.");
		FreeReply(thing);
		return false;
	    }
	}
	curconf = areaz.confs[lasta];
	lamecurconf = true;
    }
/*  if (!(curconf->areabits & INF_POST))
	lamecurconf = true; */
    if ((defaultarea = Conf2ix(curconf)) < 0)
	defaultarea = lasta;
    oldnewarea = lasta = defaultarea;
    if (curconf->areabits & (INF_NETMAIL | INF_NO_PUBLIC)) {
	if (replyeee && (cc = Confind(replyeee->confnum))
			    && !(cc->areabits & (INF_NETMAIL | INF_NO_PUBLIC))
			    && (i = Conf2ix(cc)) >= 0)
	    publicareanum = i;
	else
	    publicareanum = -1;
    } else
	publicareanum = lamecurconf ? -1 : defaultarea;
    if (curconf->areabits & (INF_NO_PUBLIC | INF_NO_PRIVATE)) {
	if (!(curconf->areabits & INF_NO_PRIVATE))
	    thing->bluebits |= UPL_PRIVATE;
    } else if (editee)		/* applies to re-edits and replies */
	thing->bluebits |= editee->bluebits & UPL_PRIVATE;
    else if (curconf->areabits & INF_NETMAIL)
	thing->bluebits |= UPL_PRIVATE;
    if (editee && !fixing)
	thing->bluebits |= UPL_IS_REPLY;
    return true;
}				/* SetUpToCompose */


local void FiggerWhoFrom(void)
{
    if (!qwk && curconf->areabits & INF_ANY_NAME && anynym[0])
	strncpy0(cbuffrom, anynym, fromtolen);
    else
	strncpy0(cbuffrom, (!qwk && curconf->areabits & INF_ALIAS_NAME
				    ? myothername : myloginame), fromtolen);
    if (uppitynames)
	strupr(cbuffrom);
}


void PrepCGags(struct Mess *editee, bool fixing, bool addressee, bool newiemail)
{
    short h = fakefight + 12;

    maxielen = official_iemail ? 100 : (pcboard_ie ? 121
					: (searchlight_ie ? 67 : 76));
    cstrfrom.MaxChars = fromtolen + 1;
    cstrtoo.MaxChars = thing->bits & EMAIL_REPLY ? maxielen : cstrfrom.MaxChars;
    cgagfrom.Width = cstrfrom.MaxChars * 8;	/* nominal coordinates */
    if (cgagfrom.Width > 288)
	cgagfrom.Width = 288;
    cgagtoo.Width = cgagfrom.Width;
    cstrtoo.Buffer = cbuftoo;
    cstrsubj.MaxChars = subjectlen + 1;		/* may be revised later */
    cgagsubj.Width = cstrsubj.MaxChars * 8;
    if (cgagsubj.Width > 496)
	cgagsubj.Width = 496;
    cgagfrom.TopEdge = h;
    cgagquote.TopEdge = h - 3;
    cgagtoo.TopEdge = 2 * h;
    cgagpriv.TopEdge = cgagtoo.TopEdge - 3;
    cgagsubj.TopEdge = 3 * h;
    cgagarea.TopEdge = cgaghandle.TopEdge = cgagsubj.TopEdge + fakefight + 9;
    cgagiemail.TopEdge = cgagnetmail.TopEdge
				= cgaghandle.TopEdge + fakefight + 11;
    cgagattach.TopEdge = cgagiemail.TopEdge;
    cgagnet.TopEdge = cgagnetmail.TopEdge + 2;
    cgagcancel.TopEdge = cgagedit.TopEdge = cgagsave.TopEdge
				= cgagtag.TopEdge = -fakefight - 12;
    cneww.Height = cgagnetmail.TopEdge + fakefight + 12 - cgagcancel.TopEdge;
    /* height for fakefight = 11: 158 */
    cneww.Width = (fontwid < 8 ? CWINWIDTH + 4 * (8 - fontwid) : CWINWIDTH);
    cneww.TopEdge = texbot + bgwin->TopEdge - cneww.Height - lace;
    cbufnet[0] = subchopchar = 0;
    memset(cbufsubj, 0, sizeof(cbufsubj));
    if (carb) {
	cbuftoo[0] = 0;
	FiggerWhoFrom();
	strcpy(cbufsubj, carb->subject);
    } else if (fixing) {
	GetBWReplyFilename(oldflame, editee);
	strncpy0(cbuftoo, editee->too, cstrtoo.MaxChars - 1);
	strncpy0(cbuffrom, editee->from, fromtolen);
	subchopchar = editee->subject[subjectlen];
	strncpy0(cbufsubj, editee->subject, subjectlen);
	if ((h = strlen(editee->subject)) > subjectlen) {    /* MKQWK netmail */
	    register ushort o = subjectlen + 1;
	    strncpy0(cbufsubj + o, editee->subject + o, min(SUBJLEN - 1, h) - o);
	}
    } else {
	if (editee) {
	    backwards = addressee;
	    strncpy0(cbuftoo, addressee ? editee->too : editee->from, fromtolen);
	    if (!uppitynames)
		strcpy(cbuftoo, Capitalism(cbuftoo));
	} else
	    cbuftoo[0] = 0;
	FiggerWhoFrom();
	if (editee) {
	    char subuf[SUBJLEN];
	    str su = editee->subject;
	    /* we do NOT expand beyond 25 without reassurance: */
	    short l, foo, i = SubjectLength(force_pcbkluge || strlen(su) > 25);
	    /* !!!!  This is a quickie that will need expanding for real QWKE: */
	    if (qwkE_subs && editee->linect && !strncmp(editee->lines[0],
				"Subject: ", 9) && editee->lines[0][-1] >
				strlen(su) + 9)
		strncpy0(su = subuf, editee->lines[0] + 9,
				editee->lines[0][-1] - 9);
	    if ((addRe || (curconf->morebits & (INTERNET_EMAIL | NEWSGROUP))
				&& subjectlen > 25) && strnicmp(su, "RE:", 3)) {
		strcpy(cbufsubj, uppitytitle ? "RE: " : "Re: ");
		strncpy(&cbufsubj[4], su + (su[0] == ' '), SUBJLEN);
		l = strlen(cbufsubj);
		foo = l - i;
		if (foo > 0) {
		    short bc = -1;
		    str p;
		    for (p = cbufsubj + l - 1; p >= (str) &cbufsubj[0]; p--) {
			if (*p == ' ') {
			    bc++;
			    if (bc > foo) {
				strcpy(p, p + foo);
				break;
			    }
			} else
			    bc = -1;
		    }
		    cbufsubj[i] = 0;
		}
	    } else
		strncpy0(cbufsubj, su, i);
	} else
	    cbufsubj[0] = 0;
	if (newiemail || (curconf->morebits & INTERNET_EMAIL &&
				(official_iemail || (editee &&
	  /* unnecessary? -> */	  !strcmp(editee->confnum, curconf->confnum) &&
				  (ValidInternet(cbuftoo) ||
				   ExtractFromAddress(editee, null, 0, 0)))))) {
	    IemailFlip();
	    if (whicha >= 0 && readareaz.confs[whicha] != &replies)
		lamecurconf = false;
	}
    }
/*  Stranks(cbufsubj); */	/* might fuck up CorrectSubGad() later */
/***
    if (uppitytitle)
	strupr(cbufsubj);
***/
    Stranks(cbuftoo);
    fromend = strlen(cbuffrom);
    AbleGad(&cgagfrom, curconf->areabits & INF_ANY_NAME);
    AbleGad(&cgagnet, ReallyInNetmail());
    AbleGad(&cgagnetmail, netmailareanum >= 0);
    AbleGad(&cgagiemail, iemailareanum >= 0);
    AbleGad(&cgaghandle, curconf->areabits & INF_ANY_NAME);
    AbleGad(&cgagattach, curconf->areabits & INF_HASFILE || thing->attached);
    if (!(cgagnet.Flags & GFLG_DISABLED) && !cbufnet[0])
	FillNetAddress();
    standbycbufnet[0] = gatecbufnet[0] = 0;
    lastbufnet = 0;
    czone = cnet = cnode = cpoint = 0;		/* must be reparsed later */
    if (curconf->morebits & MULTI_NEWSGROUP)
	EnterNewsgroup();
    cgagpriv.GadgetText = thing->bluebits & UPL_PRIVATE
				? &cgtprivate : &cgtpublic;
    AbleGad(&cgagpriv, !(curconf->areabits & (INF_NO_PUBLIC | INF_NO_PRIVATE)));
    quotype = quotedefault;
    cgagquote.GadgetText = cgtquotes[quotype];
    AbleGad(&cgagquote, replyeee && (dubedit || !fixing));
    AbleGad(&cgagsave, fixing || carb);
    /* tagspossible = */ InitializeTagStuff();
    AbleGad(&cgagtag, /* tagspossible && */ !(curconf->areabits & INF_NO_TAGLINE));
}				/* PrepCGags */


void ComposeMsg(struct Mess *editee, bool fixing,
			bool addressee, struct Mess *carbon)
{
    long tele = XActual(401);	/* VVV hidden flag in parms */
    bool newness, newiemail = fixing && addressee && !editee && !carbon;
    struct Gadget *arro = carbon ? &cgagsave : &cgagedit;

    if (newiemail) {
	if (iemailareanum < 0)
	    return;
	fixing = addressee = false;
    }
    if (fixing && editee && editee->bits & RESEARCHING)
	addressee = true;
    if (!SetUpToCompose(editee, fixing, carbon))
	return;
    anynym = localanyname[0] ? &localanyname[0] : &anyname[0];
    if (addressee)
	thing->bits |= RESEARCHING;
    PrepCGags(fixee, fixingreply, addressee, newiemail);
    CorrectSubGad();
    newness = fixingreply ? !replyeee : !fixee;
    sprintf(composetitle, "%sing a %s [Internet]", carbon ? "Carbon-copy" :
				(fixing ? "Re-edit" : "Writ"), newness ? (carbon
				? "message" : "new message") : "reply");
    if (!(thing->bits & EMAIL_REPLY))
	strchr(composetitle, '[')[-1] = '\0';
    if (!(cwin = OpenBlueGagWin(&cww, &cgagtoo))) {
	WindowErr("writing a message.");
	return;
    }
    if (cgagtoo.GadgetText == &clabeltoo) {
	clabelgroups.LeftEdge = clabeltoo.LeftEdge;
	clabelgroups.BackPen = clabeltoo.BackPen;
    } else {
	clabeltoo.LeftEdge = clabelgroups.LeftEdge;
	clabeltoo.BackPen = clabelgroups.BackPen;
    }
    ourqsemnode.composingnow = true;
    firstedit = true;
    poseclosed = false;
    notoowarned = fixing && editee && (!editee->too[0] || !editee->subject[0])
				&& !(curconf->morebits & NEWSGROUP);

    GhostOn(cwin);
    RefreshGList(&cgagpriv, cwin, null, 2);		/* both cycles */
    GhostOff(cwin);
    FixDefarrow();
    defarrow.LeftEdge = arro->LeftEdge;
    defarrow.TopEdge = arro->TopEdge + cwin->Height - 1;
    DrawBorder(cwin->RPort, &defarrow, ARROWLEFT, 0);
    UnderstartedText(tele, cgagquote.TopEdge + 3, cwin->RPort, "Quoting:");
    UnderstartedText(tele, cgagpriv.TopEdge + 3,  cwin->RPort, "Privacy:");
    OffMenu(bgwin, FULLMENUNUM(0, NOITEM, NOSUB));	/* Project */
    OffMenu(bgwin, FULLMENUNUM(1, NOITEM, NOSUB));	/* Messages */
    OffMenu(bgwin, FULLMENUNUM(2, NOITEM, NOSUB));	/* Replies */
    OffMenu(bgwin, FULLMENUNUM(3, 0, NOSUB));		/* Setup / Sort */
    SetMenuStrip(cwin, &mainu);

    if (!CNewArea(lamecurconf, false))
	FlushEdit(thing);
    else {
	if (ReallyInNetmail())
	    OpenBitsWin();
	ActivateGag((cbuftoo[0] ? &cgagsubj : &cgagtoo), cwin);
	EventLoop(&DoComposeIDCMP);
    }
    cstrtoo.Buffer = cbuftoo;
    CloseBitsWin();
    ClearMenuStrip(cwin);
    OnMenu(bgwin, FULLMENUNUM(0, NOITEM, NOSUB));
    OnMenu(bgwin, FULLMENUNUM(1, NOITEM, NOSUB));
    OnMenu(bgwin, FULLMENUNUM(2, NOITEM, NOSUB));
    OnMenu(bgwin, FULLMENUNUM(3, 0, NOSUB));
    deferflipb = true;
    CloseBlueGagWin(cwin);
    deferflipb = false;
    FlipBGadgets(readareaz.confs[whicha] == &bullstuff ? 0x77 : 0x7f);
    ourqsemnode.composingnow = false;
    if (newsgroups[replies.messct])	/* in case of Cancel */
	newsgroups[replies.messct] = null;
    if (messageID[replies.messct])
	messageID[replies.messct] = null;
    cgagtoo.GadgetText = &clabeltoo;
    cwin = null;
}				/* ComposeMsg */
