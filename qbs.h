#ifndef QB_STABLE_H
#define QB_STABLE_H

/* Header for Q-Blue (c) 1992-1997 by Paul Kienitz, ALL RIGHTS RESERVED. */
/* This is the main part with definitions that do not depend on which version */
/* or which source file is being compiled. */

#include <string.h>
#include <ctype.h>
#include <Paul.h>

#define BIG_ENDIAN
#define PAD_SIZES_EVEN
#include "bluewave.h"

/* just to avoid needing stdio.h and stdlib.h for these only: */
int sprintf(char *_s, const char *_format, ...);
long atol(const char *_nptr);


// #define HECTOR_HACK
// extern /* const */ char XPRESSNAME[];


/* ============================  general stuff  ============================= */

#define FIGFILE        "S:Q-Blue.config"
#define TEARGHEAD      " * Q-Blue "

#define COMMANDLEN     256
#define XCOMMANDLEN    2047
#define PATHLEN        80
#define PACKNAMELEN    8
#define SPOORLEN       256
#define GREENBAY       8
#define NAMELEN        36
#define SUBJLEN        72
#define DATELEN        20
#define SHOCOLEN       21
#define LONCOLEN       50
/**** IMPORTANT!!  No string gadget may have a size larger than SIGNATURELEN! */
#define SIGNATURELEN   401
#define BASICLINELIMIT 499
#define SUPERLINELIMIT 8000
#define LOADBLOCK      2048
#define MAXIETOLEN     121
#define TRASHLIMIT     100
#define PERSONALIMIT   400
#define BGSCROLLSPEED  20000
#define QBVERSION      7

#define QEOL           0xE3
#define ESC            27
#define HELPKEY        0x5F


/* assorted conveniences: */

#define SHIFTKEYS     (IEQUALIFIER_LSHIFT | IEQUALIFIER_RSHIFT)
#define ALTKEYS       (IEQUALIFIER_LALT | IEQUALIFIER_RALT)
#define ALTCTLKEYS    (ALTKEYS | IEQUALIFIER_CONTROL)
#define GAGFINISH(I)  ((I)->Code != '\t' && (I)->Code != ESC)
#define GAGCHAIN(I)   (strchain && !((I)->Qualifier & ALTKEYS))
/* Only use GAGCHAIN after first testing for GAGFINISH */

#define SEARCHLITE(c)  (fourcolors ? (((c) & 3) == 3 ? 1 : 3) : 5)


typedef enum {
    normal, readareazwin, listwin, areazwin, filezwin, taglinewin
} whatsearch;


/* this is passed to OpenShareWin to open windows that remember their */
/* coordinates when closed, in a smart way: */

struct WhereWin {
    struct ExtNewWindow *nw;
    short left, top, width, height;
    ushort oldgagmask, gacount, scrheight;
    bool moved;
    struct Gadget *gfirst, *glast;
};
/* Note: top and height are in pixels, but left and width are in units */
/* of fontwid / 8, so that all original coordinates are given in terms */
/* of a 640 pixel wide screen, translated on the fly for other widths. */

struct trash {
    char n[14];
    struct DateStamp d;
    long filesize;
};

struct Attachment {
/*  struct Attachment *another;      -- not used */
    char tempname[32];
    char arrivename[32];
    char localpath[COMMANDLEN];
    bool localsource;
};
/* File attachment rules:  Replies are always limited to one attachment,   */
/* but in QWK, incoming messages might support more in theory.  Outgoing   */
/* tempname values are always in 8.3 form; incoming ones could conceivably */
/* not be, but in practice they are likely to always be.  In Blue Wave,    */
/* tempname always has to be identical to arrivename, and is always 8.3.   */
/* The localpath part is not used for incoming attachments, except         */
/* possibly to remember where the user has saved it permanently.           */

#define ATTACHED(mm)   (((mm)->bits & PERSONALCOPY && (mm)->personalink) \
			? (mm)->personalink->attached : (mm)->attached)

/* ==============================  Messages  ================================ */

struct Mess {
    ustr *lines;		   /* Valloc'd array of linect pooled strings */
    ubyte *linetypes;			    /* Valloc'd array of linect bytes */
    ustr linepool;		    /* chain of puddles pointed into by lines */
    struct Mess *mreplyee, *personalink;
    long datfseek, datflen;
    ulong unixdate, ixinbase, replyto, replyat;
    ushort linect, poolindex, bits, bluebits, attribits;
    ushort zone, net, node, point;
    ustr from, too, subject;               /* either BValloc or NewPoolString */
    struct Attachment *attached;
    char date[DATELEN], confnum[8];
};

/* flags in the bits field: */
#define RESEARCHING     0x0001
#define FROMME          0x0002
#define TOME            0x0004
#define LOADED          0x0008
#define PERSONALCOPY    0x0010
#define SEENINBASE      0x0020
#define BODYMATCH       0x0040
#define MESEEN          0x0080
#define MEREPLIED       0x0100
#define ANSI            0x0200
#define POINTLESS       0x0400
#define DONTSTRIP       0x0800
#define ISBULLETIN      0x1000
#define LASTREALLYREAD  0x2000

#define REPLY_HAS_TAG   TOME
#define EMAIL_REPLY     FROMME

/* The MEREPLIED bit is used in the replies area to distinguish replies there
from original messages.  RESEARCHING in replies means it was generated with the
"Reply to addressee" command.  Bulletins have ISBULLETIN set, which means the
from field contains a filehandle BPTR casted as a pointer.  LASTREALLYREAD is
only used for messages that have a copy in the personal area. */

/* The bluebits field contains UPL_xxxx flags.  UPL_INACTIVE is used as a flag
to mark deleted messages, in the replies area.  The attribits field uses the
Fido defines; the bits redundant with UPL_PRIVATE and UPL_NETMAIL are mostly
ignored.  */

/* datfseek and datflen give byte offsets in the bbsname.DAT or bbsname.MSG file
where the text can be found.  The text itself may or may not be present in
memory, the LOADED bit is set if it is.  The text is pointed to by the array of
string pointers that the lines field points to.  Note that the strings pointed
to are NOT null terminated -- their lengths are stored in the byte before the
start of the string.  Empty lines generally have null pointers.  */

/* linetypes values:  (EXTENSIONTYPE is not used yet) */

#define UNKNOWNTYPE   0x00
#define BODYTYPE      0x10
#define QUOTETYPE     0x20
#define TRASHTYPE     0x30
#define RFCHEADERTYPE 0x40
#define GUESSWRAPTYPE 0x50
#define WRAPTYPE      0x60
#define QUOTEWRAPTYPE 0x70
#define TRASHWRAPTYPE 0x80
#define RFCWRAPTYPE   0x90
#define EXTENSIONTYPE 0xA0
/* rule: gap should always be zero if type is less than WRAPTYPE */
#define GAP_MASK      0x0F
#define GAP_SPACES    "               "
#define TYPE_MASK     0xF0
#define TYPE_SHIFT    4
/* GAP_SPACES must consist of exactly GAP_MASK space characters. */


/* =============================  Conferences  ============================== */

struct Conf {
    union {
	struct Mess **mray;		/* Valloc'd array of messct ptrs */
	struct Conf **cray;		/* likewise */
    } m;
    struct Conf *unfiltered;		/* for word searches */
    str longname;			/* NewPoolString pointer, or null */
    ushort areabits, morebits, messct, tomect, sofar, current;
    char confnum[8], shortname[SHOCOLEN];
    ubyte net_type;
};
/* The BW confnum string must be kept aligned to an even address, all letters in
it must be uppercase except with fake areas like replies and personals, and the
bytes after the terminating nul MUST all be zero.  Otherwise the optimized
version of Confind() will fail. */

/* the "messes" fieldname predates the use of a union: */
#define messes m.mray
#define confs  m.cray

/* with QWK, the name goes in shortname if it fits, longname otherwise. */
#define LONGNAME(cc) ((cc)->longname ? (cc)->longname : &(cc)->shortname[0])

/* we define an extra value for net_type besides the ones in bluewave.h: */
#define INF_NET_POSTLINK  8

/* areabits are Bluewave INF_* flags; morebits flag values are: */
#define ANYREAD           0x0001
#define ALLREAD           0x0002
#define SORTCLOBBERED     0x0004
#define HASLONGSUBKLUGE   0x0008
#define DOOR_ADDING       0x0010
#define DOOR_DROPPING     0x0020
#define INTERNET_EMAIL    0x0040
#define NEWSGROUP         0x0080
#define DOOR_ADDING_YOURS 0x0100
#define DOOR_RESETTING    0x0200
#define MULTI_NEWSGROUP   0x0400

/* The INTERNET_EMAIL flag is set when an area either has flag INF_NETMAIL */
/* and type INF_NET_INTERNET, or it's selected as such in the local setup. */
/* Note that we unset the INF_NETMAIL bit and use it only for Fido areas,  */
/* except of course when email and netmail share one area (i.e. when email */
/* is gated through FidoNet).  The NEWSGROUP bit is set either by INF_ECHO */
/* and INF_NET_INTERNET, or by guesswork.  MULTI_NEWSGROUP means that we   */
/* can edit a crossposting group list.  The DOOR_ADDING_YOURS bit means    */
/* that we are asking for personal messages only, or if DOOR_ADDING is set */
/* also, for personal messages and messages to "All".  DOOR_RESETTING is   */
/* for QWK only so far: it means set the high message pointer to maximum.  */
/* With QWK, DOOR_ADDING_YOURS is used as a time-saving indicator that an  */
/* extra command argument has been specified.                              */

#define DOOR_AREAPICKS   (DOOR_ADDING | DOOR_ADDING_YOURS \
				| DOOR_DROPPING | DOOR_RESETTING)


/* ========================  widely used variables  ========================= */

import struct MsgPort *idcort;

import struct Conf areaz, readareaz, inuseareaz, filez;
import struct Conf bullstuff, personals, replies;
import struct Mess *onscreen;

import char workdir[], replydir[], bbsesdir[], downloaddir[], uploaddir[];
import char myloginame[], myothername[], anyname[], localanyname[];
import char editinfile[], editoutfile[], packetname[], undospace[];
import char workdirinst[], replydirinst[], editininst[], editoutinst[];

import ushort fromtolen, subjectlen;
import ushort fight, fakefight, fontwid, checkspace, checkoff, texbot;

import bool qwk, lace, anythingtosave, fakery, pcbkluge, fourcolors;
import bool repchanges, wasescaped, poseclosed, gagrow, waste, strchain;

import short whicha, whichm;

import long backcolor, fibize;

import str dirname[6];

/* These constants are used to index into the dirname[] array: */

#define DDOWN 0
#define DUP   1
#define DBBS  2
#define DWORK 3
#define DREP  4
#define DASL  5


/* ========================  widely used functions  ========================= */

struct Conf *Confind(str cnum);
void ShowNewMessage(struct Conf *cc, struct Mess *mm);
str InhaleFile(str filename);
void ExhaleFile(void);
void Err(str format, ...);
void DosErr(str format, ...);
void WindowErr(str what);

void Seyn(struct Gadget *gyn, bool yes);
void SeynLabel(struct Gadget *gyn, bool yes, str label);
void UnderstartedText(long x, long y, struct RastPort *rp, str label);

void MoveNominal(struct RastPort *rp, short x, short y);

void Stranks(str s);
bool lch(ustr line, ustr s);
void /* itoa(short i, str a), */ utoa(ushort i, str a);
str strnistr(str outer, str inner, ushort len);
str strnchr(str in, char what, ushort lim);

typedef bool (*IntuiHandler)(struct IntuiMessage *);
void EventLoop(IntuiHandler handler);

struct Window *OpenShareWin(struct WhereWin *ww);
void CloseShareWin(struct Window *w);
struct Window *OpenBlueGagWin(struct WhereWin *ww, struct Gadget *first);
void CloseBlueGagWin(struct Window *w);
char KeyToAscii(ushort code, ushort qual);
void PortOff(void), PortOn(void);
void NoMenus(void), YesMenus(void);
void GhostCompose(bool off);
void GhostOn(struct Window *ww), GhostOff(struct Window *ww);
void StopScroll(void);

ushort FlipBGadgets(ushort mask);
void AbleGad(struct Gadget *gg, bool ability);
void AbleAddGad(struct Gadget *gg, struct Window *ww, bool ability);
void ActivateGag(struct Gadget *gg, struct Window *ww);
void ChangeGagText(struct Gadget *gg, struct Window *ww, struct IntuiText *ii);
void ChangeBotGagText(struct Gadget *gg, struct IntuiText *ii, bool fresh);
void FixGreenGad(struct Gadget *gg, struct Window *ww);
void StripString(struct Gadget *gg, struct Window *ww);

adr Valloc(ulong size);
str BValloc(ushort size);
void Vfree(adr where);
void FreeReply(struct Mess *mm);

		/* Functions that cannot be used without a pragma: */

short XActual(short x);
#pragma regcall(XActual(d0))

str strncpy0(str dest, str src, ushort lim);
#pragma regcall(strncpy0(a0,a1,d0))
str strend(str s);
#pragma regcall(strend(a1))

ushort i2(ubyte *intel);
#pragma regcall(i2(a0))
ulong i4(ubyte *intel);
#pragma regcall(i4(a0))

#endif QB_STABLE_H
