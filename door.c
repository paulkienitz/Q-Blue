/* Blue Wave door configuration and, for starters, file download requesting. */

#include <exec/memory.h>
#include <intuition/sghooks.h>
#include <intuition/intuition.h>
#include "qblue.h"
#include "pigment.h"


bool WriteFileREQ(void);
void WritePDQ(bool anydifference);
void WriteUPL(void);
void DecideIfAnythingToSave(void);
bool AskResetDoorFig(bool areas);
void WarnNoDoorIdAddDrop(bool maximus);
short AddUnknownArea(short initialarea);

short ListAreas(short initialarea, bool empties, bool addrop, bool minusone);
void ListAllLines(struct Conf *cc);
void RedisplayPickLine(struct Conf *cc);
void FixListSlider(short mct, bool force);


import struct Image radio_off, radio_on;

import struct Conf *listcc;

import bool pdq_exists, pdq_difference, forbidREQ, forbidOLC;
import bool addropping, newareawin, dooraddrop, doorreset, version3;
import ushort checktall;
import short pick;
import char doorconn[];


/* This is used to keep track of custom QWK door commands: */

struct Doorcommand {
    struct Doorcommand *dcnext, **dcparent;
    struct Conf *dcconf;
    char dctext[25];
} *doorcommandlist = null;

local struct Conf *aaaconf;

char requestedfiles[10][15], unrequestedfiles[10][15];
char bwpassword[23], oldpassword[21];
char bwkeywords[10][23], bwfilters[10][23];
char oldkeywords[10][21], oldfilters[10][21];
char macro1[80], macro2[80], macro3[80];
char oldmacro1[80], oldmacro2[80], oldmacro3[80];

local char pksizenumspace[12], aaextra[28];

short passwordtype, oldpasswordtype;
ushort requestedfiles_limit, addropmask;
ushort bwflistype, oldflistype, bwpksize, oldpksize;

bool filerequests_exist, addsdrops, addropchanges, doorwarnedonce = false;
bool bwexpert, bwhotkeys, bwgraphics, bwownmail, bwnoxlines, bwnumericext;
bool oldexpert, oldhotkeys, oldgraphics, oldownmail, oldnoxlines, oldnumericext;
bool bwauto1, bwauto2, bwauto3, oldauto1, oldauto2, oldauto3;
local bool now_difference;


struct IntuiText aalabelextra = {
    LABELCOLOR, 0, JAM1, 0, -20, null, "Extra command arguments:", null
};

struct StringInfo aastrextra = STRINF(aaextra, 26);

STRINGBORDER(aagagextrabox)

struct Gadget aagagextra = {
    null, 20, 27, 208, 8, GFLG_STRINGEXTEND,
    GACT_RELVERIFY | GACT_STRINGLEFT, GTYP_STRGADGET,
    &aagagextrabox, null, &aalabelextra, 0, &aastrextra, 1754, null
};

struct Gadget aagagcheckreset = {
    &aagagextra, 16, 42, CHECKWID, 13, CHECKHILITE, CHECKACTION,
    GTYP_BOOLGADGET, &checkmarkno, &checkmarkyes, null, 0, null, 1753, null
};

struct Gadget aagagcheckall = {
    &aagagcheckreset, 16, 22, CHECKWID, 13, CHECKHILITE, CHECKACTION,
    GTYP_BOOLGADGET, &checkmarkno, &checkmarkyes, null, 0, null, 1752, null
};

/* ====== Note that aagagcheckall and aagagradioall use the same ID number! */

struct Gadget aagagradioall = {
    null, 16, 22, 19, 11, GFLG_GADGIMAGE | GFLG_GADGHIMAGE,
    GACT_IMMEDIATE | GACT_TOGGLESELECT,
    GTYP_BOOLGADGET, &radio_off, &radio_on, null, 0, null, 1752, null
};

struct Gadget aagagradiopers = {
    &aagagradioall, 16, 42, 19, 11, GFLG_GADGIMAGE | GFLG_GADGHIMAGE,
    GACT_IMMEDIATE | GACT_TOGGLESELECT,
    GTYP_BOOLGADGET, &radio_off, &radio_on, null, 0, null, 1751, null
};

struct Gadget aagagradiopa = {
    &aagagradiopers, 16, 62, 19, 11, GFLG_GADGIMAGE | GFLG_GADGHIMAGE,
    GACT_IMMEDIATE | GACT_TOGGLESELECT,
    GTYP_BOOLGADGET, &radio_off, &radio_on, null, 0, null, 1750, null
};


struct ExtNewWindow aaneww = {
    181, 30, 248, 80, 0, 1,
    IDCMP_GADGETDOWN | IDCMP_GADGETUP | IDCMP_CLOSEWINDOW | IDCMP_RAWKEY,
    WFLG_SMART_REFRESH | WFLG_CLOSEGADGET | WFLG_DEPTHGADGET
		| WFLG_DRAGBAR | WFLG_ACTIVATE | WFLG_NW_EXTENDED,
    null, null, "Selecting an area",
    null, null, 0, 0, 0, 0, CUSTOMSCREEN, null
};

struct WhereWin aaww = { &aaneww };

struct Window *aawin;

local bool addall, addpers, addpa, addreset;


/* ----------------------------------------------------------------- */


STRINGBORDER(drbox)

struct StringInfo drstr[10] = {
    STRINF(requestedfiles[0], 13),
    STRINF(requestedfiles[1], 13),
    STRINF(requestedfiles[2], 13),
    STRINF(requestedfiles[3], 13),
    STRINF(requestedfiles[4], 13),
    STRINF(requestedfiles[5], 13),
    STRINF(requestedfiles[6], 13),
    STRINF(requestedfiles[7], 13),
    STRINF(requestedfiles[8], 13),
    STRINF(requestedfiles[9], 13)
};

struct Gadget drgag[10] = {
    {
	&drgag[1], 22, 24, 104, 8, GFLG_STRINGEXTEND | GFLG_TABCYCLE,
	GACT_RELVERIFY | GACT_STRINGLEFT, GTYP_STRGADGET,
	&drbox, null, null, 0, &drstr[0], 2000, null
    }, {
	&drgag[2], 22, 45, 104, 8, GFLG_STRINGEXTEND | GFLG_TABCYCLE,
	GACT_RELVERIFY | GACT_STRINGLEFT, GTYP_STRGADGET,
	&drbox, null, null, 0, &drstr[1], 2001, null
    }, {
	&drgag[3], 22, 66, 104, 8, GFLG_STRINGEXTEND | GFLG_TABCYCLE,
	GACT_RELVERIFY | GACT_STRINGLEFT, GTYP_STRGADGET,
	&drbox, null, null, 0, &drstr[2], 2002, null
    }, {
	&drgag[4], 22, 87, 104, 8, GFLG_STRINGEXTEND | GFLG_TABCYCLE,
	GACT_RELVERIFY | GACT_STRINGLEFT, GTYP_STRGADGET,
	&drbox, null, null, 0, &drstr[3], 2003, null
    }, {
	&drgag[5], 22, 108, 104, 8, GFLG_STRINGEXTEND | GFLG_TABCYCLE,
	GACT_RELVERIFY | GACT_STRINGLEFT, GTYP_STRGADGET,
	&drbox, null, null, 0, &drstr[4], 2004, null
    }, {
	&drgag[6], 151, 24, 104, 8, GFLG_STRINGEXTEND | GFLG_TABCYCLE,
	GACT_RELVERIFY | GACT_STRINGLEFT, GTYP_STRGADGET,
	&drbox, null, null, 0, &drstr[5], 2005, null
    }, {
	&drgag[7], 151, 45, 104, 8, GFLG_STRINGEXTEND | GFLG_TABCYCLE,
	GACT_RELVERIFY | GACT_STRINGLEFT, GTYP_STRGADGET,
	&drbox, null, null, 0, &drstr[6], 2006, null
    }, {
	&drgag[8], 151, 66, 104, 8, GFLG_STRINGEXTEND | GFLG_TABCYCLE,
	GACT_RELVERIFY | GACT_STRINGLEFT, GTYP_STRGADGET,
	&drbox, null, null, 0, &drstr[7], 2007, null
    }, {
	&drgag[9], 151, 87, 104, 8, GFLG_STRINGEXTEND | GFLG_TABCYCLE,
	GACT_RELVERIFY | GACT_STRINGLEFT, GTYP_STRGADGET,
	&drbox, null, null, 0, &drstr[8], 2008, null
    }, {
	null,      151, 108, 104, 8, GFLG_STRINGEXTEND | GFLG_TABCYCLE,
	GACT_RELVERIFY | GACT_STRINGLEFT, GTYP_STRGADGET,
	&drbox, null, null, 0, &drstr[9], 2009, null
    }
};


struct ExtNewWindow drneww = {
    181, 20, 278, 130, 0, 1,
    IDCMP_GADGETUP | IDCMP_CLOSEWINDOW | IDCMP_RAWKEY,
    WFLG_SMART_REFRESH | WFLG_CLOSEGADGET | WFLG_DEPTHGADGET
		| WFLG_DRAGBAR | WFLG_ACTIVATE | WFLG_NW_EXTENDED,
    null, null, "File download requests",
    null, null, 0, 0, 0, 0, CUSTOMSCREEN, null
};

struct WhereWin drww = { &drneww };

struct Window *drwin;


/* ----------------------------------------------------------------- */


struct IntuiText bwgtansi = {
    TEXTCOLOR, FILLCOLOR, JAM2, 39, 3, null, " ANSI ", null
};

struct IntuiText bwgttext = {
    TEXTCOLOR, FILLCOLOR, JAM2, 39, 3, null, " Text ", null
};

struct IntuiText bwgtnone = {
    TEXTCOLOR, FILLCOLOR, JAM2, 39, 3, null, " None ", null
};

struct IntuiText *(bwgtnewfiles[3]) = {
    &bwgtnone, &bwgttext, &bwgtansi
};

struct Gadget bwgagnewfiles = {
    null, 448, 65, 106, 12, GFLG_GADGHCOMP, GACT_RELVERIFY,
    GTYP_BOOLGADGET, &upcyclebor, null, null, 0, null, 1719, null
};


struct IntuiText bwlabelpksize = {
    LABELCOLOR, 0, JAM1, -147, 0, null, "Max pkt size (K):", null
};

struct StringInfo bwstrpksize = STRINF(pksizenumspace, 6);

STRINGBORDER(bwpksizebox)

struct Gadget bwgagpksize = {
    &bwgagnewfiles, 163, 68, 48, 8, GFLG_STRINGEXTEND | GFLG_TABCYCLE,
    GACT_RELVERIFY | GACT_STRINGLEFT | GACT_LONGINT, GTYP_STRGADGET,
    &bwpksizebox, null, &bwlabelpksize, 0, &bwstrpksize, 1718, null
};


struct Gadget bwgagauto3 = {
    &bwgagpksize, 332, 22, CHECKWID, 13, CHECKHILITE, CHECKACTION,
    GTYP_BOOLGADGET, &checkmarkno, &checkmarkyes, null, 0, null, 1717, null
};

struct Gadget bwgagauto2 = {
    &bwgagauto3, 332, 22, CHECKWID, 13, CHECKHILITE, CHECKACTION,
    GTYP_BOOLGADGET, &checkmarkno, &checkmarkyes, null, 0, null, 1716, null
};

struct Gadget bwgagauto1 = {
    &bwgagauto2, 332, 22, CHECKWID, 13, CHECKHILITE, CHECKACTION,
    GTYP_BOOLGADGET, &checkmarkno, &checkmarkyes, null, 0, null, 1715, null
};

struct Gadget bwgagnumeric = {
    &bwgagauto1, 318, 22, CHECKWID, 13, CHECKHILITE, CHECKACTION,
    GTYP_BOOLGADGET, &checkmarkno, &checkmarkyes, null, 0, null, 1714, null
};

struct Gadget bwgagnoxlines = {
    &bwgagnumeric, 15, 22, CHECKWID, 13, CHECKHILITE, CHECKACTION,
    GTYP_BOOLGADGET, &checkmarkno, &checkmarkyes, null, 0, null, 1713, null
};


#define LAST_V2GAG bwgagpasswhen

struct IntuiText bwgtboth = {
    TEXTCOLOR, FILLCOLOR, JAM2, 39, 3, null, " Both ", null
};

struct IntuiText bwgtreader = {
    TEXTCOLOR, FILLCOLOR, JAM2, 39, 3, null, "Reader", null
};

struct IntuiText bwgtdoor = {
    TEXTCOLOR, FILLCOLOR, JAM2, 39, 3, null, " Door ", null
};

struct IntuiText bwgtneither = {
    TEXTCOLOR, FILLCOLOR, JAM2, 35, 3, null, " Never ", null
};

struct IntuiText *(bwgtpasswhen[4]) = {
    &bwgtneither, &bwgtdoor, &bwgtreader, &bwgtboth
};

struct Gadget bwgagpasswhen = {
    &bwgagnoxlines, 448, 65, 106, 12, GFLG_GADGHCOMP, GACT_RELVERIFY,
    GTYP_BOOLGADGET, &upcyclebor, null, null, 0, null, 1711, null
};


struct IntuiText bwlabelmacro3 = {
    LABELCOLOR, 0, JAM1, -83, 0, null, "Macro #3:", null
};

struct IntuiText bwlabelmacro2 = {
    LABELCOLOR, 0, JAM1, -83, 0, null, "Macro #2:", null
};

struct IntuiText bwlabelmacro1 = {
    LABELCOLOR, 0, JAM1, -83, 0, null, "Macro #1:", null
};

struct StringInfo bwstrmacro3 = STRINF(macro3, 78);

struct StringInfo bwstrmacro2 = STRINF(macro2, 78);

struct StringInfo bwstrmacro1 = STRINF(macro1, 78);

STRINGBORDER(bwmacrobox)

struct Gadget bwgagmacro3 = {
    &bwgagpasswhen, 99, 137, 208, 8, GFLG_STRINGEXTEND | GFLG_TABCYCLE,
    GACT_RELVERIFY | GACT_STRINGLEFT, GTYP_STRGADGET, &bwmacrobox,
    null, &bwlabelmacro3, 0, &bwstrmacro3, 1710, null
};

struct Gadget bwgagmacro2 = {
    &bwgagmacro3, 99, 114, 208, 8, GFLG_STRINGEXTEND | GFLG_TABCYCLE,
    GACT_RELVERIFY | GACT_STRINGLEFT, GTYP_STRGADGET, &bwmacrobox,
    null, &bwlabelmacro2, 0, &bwstrmacro2, 1709, null
};

struct Gadget bwgagmacro1 = {
    &bwgagmacro2, 99, 91, 208, 8, GFLG_STRINGEXTEND | GFLG_TABCYCLE,
    GACT_RELVERIFY | GACT_STRINGLEFT, GTYP_STRGADGET, &bwmacrobox,
    null, &bwlabelmacro1, 0, &bwstrmacro1, 1708, null
};


struct IntuiText bwlabelpass = {
    LABELCOLOR, 0, JAM1, -83, 0, null, "Password:", null
};

struct StringInfo bwstrpass = STRINF(bwpassword, 21);

STRINGBORDER(bwpassbox)

struct Gadget bwgagpassword = {
    &bwgagmacro1, 99, 68, 168, 8, GFLG_STRINGEXTEND | GFLG_TABCYCLE,
    GACT_RELVERIFY | GACT_STRINGLEFT, GTYP_STRGADGET,
    &bwpassbox, null, &bwlabelpass, 0, &bwstrpass, 1712, null
};


struct Gadget bwgagexpert = {
    &bwgagpassword, 318, 22, CHECKWID, 13, CHECKHILITE, CHECKACTION,
    GTYP_BOOLGADGET, &checkmarkno, &checkmarkyes, null, 0, null, 1707, null
};

struct Gadget bwgaghotkeys = {
    &bwgagexpert, 15, 22, CHECKWID, 13, CHECKHILITE, CHECKACTION,
    GTYP_BOOLGADGET, &checkmarkno, &checkmarkyes, null, 0, null, 1706, null
};

struct Gadget bwgagcolor = {
    &bwgaghotkeys, 15, 44, CHECKWID, 13, CHECKHILITE, CHECKACTION,
    GTYP_BOOLGADGET, &checkmarkno, &checkmarkyes, null, 0, null, 1705, null
};

struct Gadget bwgagownmail = {
    &bwgagcolor, 318, 44, CHECKWID, 13, CHECKHILITE, CHECKACTION,
    GTYP_BOOLGADGET, &checkmarkno, &checkmarkyes, null, 0, null, 1704, null
};


struct IntuiText bwgtreset = {
    TEXTCOLOR, FILLCOLOR, JAM2, 12, 2, null, "Reset", null
};

struct IntuiText bwgtareas = {
    TEXTCOLOR, FILLCOLOR, JAM2, 12, 2, null, "Areas", null
};

struct IntuiText bwgtkeyfil = {
    TEXTCOLOR, FILLCOLOR, JAM2, 12, 2, null, "Key/Fil", null
};

struct Gadget bwgagreset = {
    &bwgagownmail, 462, 135, 80, 12, GFLG_GADGHCOMP, GACT_RELVERIFY,
    GTYP_BOOLGADGET, &upborder, null, &bwgtreset, 0, null, 1701, null
};

struct Gadget bwgagareas = {
    &bwgagreset, 462, 112, 80, 12, GFLG_GADGHCOMP, GACT_RELVERIFY,
    GTYP_BOOLGADGET, &upborder, null, &bwgtareas, 0, null, 1703, null
};

struct Gadget bwgagkeyfil = {
    &bwgagareas, 462, 89, 80, 12, GFLG_GADGHCOMP, GACT_RELVERIFY,
    GTYP_BOOLGADGET, &upborder, null, &bwgtkeyfil, 0, null, 1702, null
};


struct ExtNewWindow bwdneww = {
    35, 30, 570, 161, 0, 1,
    IDCMP_GADGETUP | IDCMP_CLOSEWINDOW | IDCMP_RAWKEY,
    WFLG_SMART_REFRESH | WFLG_CLOSEGADGET | WFLG_DEPTHGADGET
		| WFLG_DRAGBAR | WFLG_ACTIVATE | WFLG_NW_EXTENDED,
    null, null, "Blue Wave mail door configuration",
    null, null, 0, 0, 0, 0, CUSTOMSCREEN, null
};

struct WhereWin bwdww = { &bwdneww };

struct Window *bwdwin;


struct StringInfo bkfstr[20] = {
    STRINF(bwkeywords[0], 21),
    STRINF(bwkeywords[1], 21),
    STRINF(bwkeywords[2], 21),
    STRINF(bwkeywords[3], 21),
    STRINF(bwkeywords[4], 21),
    STRINF(bwkeywords[5], 21),
    STRINF(bwkeywords[6], 21),
    STRINF(bwkeywords[7], 21),
    STRINF(bwkeywords[8], 21),
    STRINF(bwkeywords[9], 21),
    STRINF(bwfilters[0], 21),
    STRINF(bwfilters[1], 21),
    STRINF(bwfilters[2], 21),
    STRINF(bwfilters[3], 21),
    STRINF(bwfilters[4], 21),
    STRINF(bwfilters[5], 21),
    STRINF(bwfilters[6], 21),
    STRINF(bwfilters[7], 21),
    STRINF(bwfilters[8], 21),
    STRINF(bwfilters[9], 21)
};

STRINGBORDER(bkfbox)

struct Gadget bkfgag[20] = {
    {
	&bkfgag[1],  19, 41, 128, 8, GFLG_STRINGEXTEND | GFLG_TABCYCLE,
	GACT_RELVERIFY | GACT_STRINGLEFT, GTYP_STRGADGET,
	&bkfbox, null, null, 0, &bkfstr[0], 1600, null
    }, {
	&bkfgag[2],  19, 62, 128, 8, GFLG_STRINGEXTEND | GFLG_TABCYCLE,
	GACT_RELVERIFY | GACT_STRINGLEFT, GTYP_STRGADGET,
	&bkfbox, null, null, 0, &bkfstr[1], 1601, null
    }, {
	&bkfgag[3],  19, 83, 128, 8, GFLG_STRINGEXTEND | GFLG_TABCYCLE,
	GACT_RELVERIFY | GACT_STRINGLEFT, GTYP_STRGADGET,
	&bkfbox, null, null, 0, &bkfstr[2], 1602, null
    }, {
	&bkfgag[4],  19, 104, 128, 8, GFLG_STRINGEXTEND | GFLG_TABCYCLE,
	GACT_RELVERIFY | GACT_STRINGLEFT, GTYP_STRGADGET,
	&bkfbox, null, null, 0, &bkfstr[3], 1603, null
    }, {
	&bkfgag[5],  19, 125, 128, 8, GFLG_STRINGEXTEND | GFLG_TABCYCLE,
	GACT_RELVERIFY | GACT_STRINGLEFT, GTYP_STRGADGET,
	&bkfbox, null, null, 0, &bkfstr[4], 1604, null
    }, {
	&bkfgag[6],  167, 41, 128, 8, GFLG_STRINGEXTEND | GFLG_TABCYCLE,
	GACT_RELVERIFY | GACT_STRINGLEFT, GTYP_STRGADGET,
	&bkfbox, null, null, 0, &bkfstr[5], 1605, null
    }, {
	&bkfgag[7],  167, 62, 128, 8, GFLG_STRINGEXTEND | GFLG_TABCYCLE,
	GACT_RELVERIFY | GACT_STRINGLEFT, GTYP_STRGADGET,
	&bkfbox, null, null, 0, &bkfstr[6], 1606, null
    }, {
	&bkfgag[8],  167, 83, 128, 8, GFLG_STRINGEXTEND | GFLG_TABCYCLE,
	GACT_RELVERIFY | GACT_STRINGLEFT, GTYP_STRGADGET,
	&bkfbox, null, null, 0, &bkfstr[7], 1607, null
    }, {
	&bkfgag[9],  167, 104, 128, 8, GFLG_STRINGEXTEND | GFLG_TABCYCLE,
	GACT_RELVERIFY | GACT_STRINGLEFT, GTYP_STRGADGET,
	&bkfbox, null, null, 0, &bkfstr[8], 1608, null
    }, {
	&bkfgag[10], 167, 125, 128, 8, GFLG_STRINGEXTEND | GFLG_TABCYCLE,
	GACT_RELVERIFY | GACT_STRINGLEFT, GTYP_STRGADGET,
	&bkfbox, null, null, 0, &bkfstr[9], 1609, null
    }, {
	&bkfgag[11], 328, 41, 128, 8, GFLG_STRINGEXTEND | GFLG_TABCYCLE,
	GACT_RELVERIFY | GACT_STRINGLEFT, GTYP_STRGADGET,
	&bkfbox, null, null, 0, &bkfstr[10], 1610, null
    }, {
	&bkfgag[12], 328, 62, 128, 8, GFLG_STRINGEXTEND | GFLG_TABCYCLE,
	GACT_RELVERIFY | GACT_STRINGLEFT, GTYP_STRGADGET,
	&bkfbox, null, null, 0, &bkfstr[11], 1611, null
    }, {
	&bkfgag[13], 328, 83, 128, 8, GFLG_STRINGEXTEND | GFLG_TABCYCLE,
	GACT_RELVERIFY | GACT_STRINGLEFT, GTYP_STRGADGET,
	&bkfbox, null, null, 0, &bkfstr[12], 1612, null
    }, {
	&bkfgag[14], 328, 104, 128, 8, GFLG_STRINGEXTEND | GFLG_TABCYCLE,
	GACT_RELVERIFY | GACT_STRINGLEFT, GTYP_STRGADGET,
	&bkfbox, null, null, 0, &bkfstr[13], 1613, null
    }, {
	&bkfgag[15], 328, 125, 128, 8, GFLG_STRINGEXTEND | GFLG_TABCYCLE,
	GACT_RELVERIFY | GACT_STRINGLEFT, GTYP_STRGADGET,
	&bkfbox, null, null, 0, &bkfstr[14], 1614, null
    }, {
	&bkfgag[16], 476, 41, 128, 8, GFLG_STRINGEXTEND | GFLG_TABCYCLE,
	GACT_RELVERIFY | GACT_STRINGLEFT, GTYP_STRGADGET,
	&bkfbox, null, null, 0, &bkfstr[15], 1615, null
    }, {
	&bkfgag[17], 476, 62, 128, 8, GFLG_STRINGEXTEND | GFLG_TABCYCLE,
	GACT_RELVERIFY | GACT_STRINGLEFT, GTYP_STRGADGET,
	&bkfbox, null, null, 0, &bkfstr[16], 1616, null
    }, {
	&bkfgag[18], 476, 83, 128, 8, GFLG_STRINGEXTEND | GFLG_TABCYCLE,
	GACT_RELVERIFY | GACT_STRINGLEFT, GTYP_STRGADGET,
	&bkfbox, null, null, 0, &bkfstr[17], 1617, null
    }, {
	&bkfgag[19], 476, 104, 128, 8, GFLG_STRINGEXTEND | GFLG_TABCYCLE,
	GACT_RELVERIFY | GACT_STRINGLEFT, GTYP_STRGADGET,
	&bkfbox, null, null, 0, &bkfstr[18], 1618, null
    }, {
	null,        476, 125, 128, 8, GFLG_STRINGEXTEND | GFLG_TABCYCLE,
	GACT_RELVERIFY | GACT_STRINGLEFT, GTYP_STRGADGET,
	&bkfbox, null, null, 0, &bkfstr[19], 1619, null
    }
};

struct ExtNewWindow bkfneww = {
    16, 26, 624, 147, 0, 1,
    IDCMP_GADGETUP | IDCMP_CLOSEWINDOW | IDCMP_RAWKEY,
    WFLG_SMART_REFRESH | WFLG_CLOSEGADGET | WFLG_DEPTHGADGET
		| WFLG_DRAGBAR | WFLG_ACTIVATE | WFLG_NW_EXTENDED,
    null, null, "Keywords and filters used by Blue Wave mail door",
    null, null, 0, 0, 0, 0, CUSTOMSCREEN, null
};

struct WhereWin bkfww = { &bkfneww };

struct Window *bkfwin;


/* ----------------------------------------------------------------- */


local void ActFirstEmptyFile(void)
{
    short i;
    for (i = 0; i < requestedfiles_limit; i++)
	if (!requestedfiles[i][0]) {
	    ActivateGag(&drgag[i], drwin);
	    return;
	}
    ActivateGag(&drgag[0], drwin);
}


bool PickDLRequestsIDCMP(struct IntuiMessage *im)
{
    bool ret = false;

    if (im->Class == IDCMP_RAWKEY && !(im->Qualifier & ALTKEYS)) {
	char k = KeyToAscii(im->Code, im->Qualifier);
	if (k == '\t')
	    ActFirstEmptyFile();
	else if (k == ESC)
	    ret = true;
    } else if (im->Class == IDCMP_CLOSEWINDOW)
	ret = true;
    else if (im->Class == IDCMP_GADGETUP && GAGFINISH(im)) {
	register short i = ((struct Gadget *) im->IAddress)->GadgetID - 2000;
	StripString(&drgag[i], drwin);
	if (!(im->Qualifier & SHIFTKEYS))
	    i++;
	else if (--i < 0)
	    i = requestedfiles_limit - 1;
	if (i < requestedfiles_limit && GAGCHAIN(im))
	    ActivateGadget(&drgag[i], drwin, null);
    }
    return ret;
}


void PickDLRequests(void)
{
    short i, j = fakefight + 12, k;

    if (!areaz.messct || qwk)
	return;
    if (!requestedfiles_limit || forbidREQ) {
	Err("This BBS does not support\noffline download requests.");
	return;
    }
    for (i = 0; i < 10; i++) {
	if (i == 0 || i == 5)
	    k = 0;
	k += j;
	drgag[i].TopEdge = k;
	StripString(&drgag[i], null);
	AbleGad(&drgag[i], i < requestedfiles_limit);
	strcpy(unrequestedfiles[i], requestedfiles[i]);
    }
    drneww.Height = k + j - 1;
    /* height when fakefight = 12: 143 */
    if (!(drwin = OpenBlueGagWin(&drww, &drgag[0]))) {
	WindowErr("requesting files to download.");
	return;
    }
    ActFirstEmptyFile();
    EventLoop(&PickDLRequestsIDCMP);
    for (i = 0; i < requestedfiles_limit - 1; i++)
	if (!requestedfiles[i][0]) {
	    for (j = i + 1; j < requestedfiles_limit; j++)
		if (requestedfiles[j][0]) {
		    for (k = i; j < requestedfiles_limit; k++, j++)
			strcpy(requestedfiles[k], requestedfiles[j]);
		    for ( ; k < requestedfiles_limit; k++)
			requestedfiles[k][0] = 0;
		    break;
		}
	}
    CloseBlueGagWin(drwin);
    drwin = null;
    for (i = 0; i < requestedfiles_limit; i++) {
	StripString(&drgag[i], null);
	if (stricmp(requestedfiles[i], unrequestedfiles[i])) {
	    repchanges = true;
	    WriteFileREQ();
	    goto doodoo;
	}
    }
doodoo:
    DecideIfAnythingToSave();
}


void FreshFListType(short increment)
{
    if (!version3)
	return;
    bwflistype = (bwflistype + increment + 3) % 3;
    ChangeGagText(&bwgagnewfiles, bwdwin, bwgtnewfiles[bwflistype]);
    RefreshGList(&bwgagnewfiles, bwdwin, null, 1);
}


void FreshPassType(short increment)
{
    passwordtype = (passwordtype + increment + 4) % 4;
    ChangeGagText(&bwgagpasswhen, bwdwin, bwgtpasswhen[passwordtype]);
    RefreshGList(&bwgagpasswhen, bwdwin, null, 1);
}


void StripKFandM(void)
{
    short a;
    strupr(macro1);
    strupr(macro2);
    strupr(macro3);
    strupr(bwpassword);
    macro1[78] = macro2[78] = macro3[78] = 0;
    for (a = 0; a < 10; a++) {
	strupr(bwkeywords[a]);
	strupr(bwfilters[a]);
    }
}


bool PDQstateDiffers(void)
{
    short a;
    if (oldauto1 != bwauto1 || oldauto2 != bwauto2 || oldauto3 != bwauto3
		|| oldhotkeys != bwhotkeys || oldexpert != bwexpert
		|| oldgraphics != bwgraphics || oldownmail != bwownmail
		|| oldnumericext != bwnumericext || oldnoxlines != bwnoxlines
		|| oldpasswordtype != passwordtype || oldflistype != bwflistype
		|| oldpksize != bwpksize || stricmp(bwpassword, oldpassword)
		|| stricmp(macro1, oldmacro1) || stricmp(macro2, oldmacro2)
		|| stricmp(macro3, oldmacro3))
	return true;
    for (a = 0; a < 10; a++)
	if (stricmp(bwkeywords[a], oldkeywords[a])
				|| stricmp(bwfilters[a], oldfilters[a]))
	    return true;
    return false;
}


void ResetDoorOptions(void)
{
    short a;
    long p, q, r, s;

    if (!PDQstateDiffers() || !AskResetDoorFig(false))
	return;
    if (bkfwin)
	p = RemoveGList(bkfwin, &bkfgag[0], 20);
    q = RemoveGadget(bwdwin, &bwgagpassword);
    r = RemoveGList(bwdwin, &bwgagmacro1, 3);
    if (version3)
	s = RemoveGadget(bwdwin, &bwgagpksize);
    strcpy(bwpassword, oldpassword);
    strcpy(macro1, oldmacro1);
    strcpy(macro2, oldmacro2);
    strcpy(macro3, oldmacro3);
    passwordtype = oldpasswordtype;
    bwhotkeys = oldhotkeys;
    bwexpert = oldexpert;
    bwgraphics = oldgraphics;
    bwownmail = oldownmail;
    bwnoxlines = oldnoxlines;
    bwnumericext = oldnumericext;
    bwauto1 = oldauto1;
    bwauto2 = oldauto2;
    bwauto3 = oldauto3;
    if (bwstrpksize.LongInt = bwpksize = oldpksize)
	utoa(bwpksize, pksizenumspace);
    else
	pksizenumspace[0] = 0;
    bwflistype = oldflistype;
    for (a = 0; a < 10; a++) {
	strcpy(bwkeywords[a], oldkeywords[a]);
	strcpy(bwfilters[a], oldfilters[a]);
    }
    StripKFandM();
    if (bkfwin) {
	AddGList(bkfwin, &bkfgag[0], p, 20, null);
	RefreshGList(&bkfgag[0], bkfwin, null, 20);
    }
    AddGList(bwdwin, &bwgagmacro1, r, 3, null);
    AddGadget(bwdwin, &bwgagpassword, q);
    RefreshGList(&bwgagmacro1, bwdwin, null, 3);
    RefreshGList(&bwgagpassword, bwdwin, null, 1);
    Seyn(&bwgaghotkeys, bwhotkeys);
    Seyn(&bwgagexpert, bwexpert);
    Seyn(&bwgagcolor, bwgraphics);
    Seyn(&bwgagownmail, bwownmail);
    FreshPassType(0);
    if (version3) {
	AddGadget(bwdwin, &bwgagpksize, s);
	RefreshGList(&bwgagpksize, bwdwin, null, 1);
	Seyn(&bwgagnoxlines, bwnoxlines);
	Seyn(&bwgagnumeric, bwnumericext);
	Seyn(&bwgagauto1, bwauto1);
	Seyn(&bwgagauto2, bwauto2);
	Seyn(&bwgagauto3, bwauto3);
	FreshFListType(0);
    }
    WritePDQ(false);
    repchanges = true;
    addropchanges = now_difference = false;
    AbleAddGad(&bwgagreset, bwdwin, false);
}


bool CheckAAAextra(struct Conf *cc, str command, ushort add, ushort reset)
{
    register struct Doorcommand *dc;
    long diff;

    for (dc = doorcommandlist; dc; dc = dc->dcnext)
	if (dc->dcconf == cc)
	    break;
    if (command[0]) {
	if ((diff = strlen(command) - (reset ? 19 : (add ? 21 : 25))) > 0) {
	    Err("The command argument \"%s\" is %ld\ncharacter%s too long to"
				" fit after the %s keyword.", command, diff,
				diff == 1 ? "" : "s", reset ? "RESET" : "ADD");
	    return false;	/* can't happen during reload */
	}
	if (!dc) {
	    if (NEW(dc)) {
		dc->dcnext = doorcommandlist;
		dc->dcparent = &doorcommandlist;
		doorcommandlist = dc;
		dc->dcconf = cc;
	    } else if (aawin)
		Err("No memory for storing the\nextra door command string!");
	    else
		return false;
	}
	strcpy(dc->dctext, command);
    } else {
	if (dc) {
	    *dc->dcparent = dc->dcnext;
	    FREE(dc);
	}
	/* cc->morebits &= ~DOOR_ADDING_YOURS; */
    }
    return true;
}


str GetExtraDoorCommand(struct Conf *cc)
{
    register struct Doorcommand *dc;
    for (dc = doorcommandlist; dc; dc = dc->dcnext)
	if (dc->dcconf == cc)
	    return dc->dctext;
    return null;
}


void FlushExtraDoorCommands(void)
{
    while (doorcommandlist) {
	register struct Doorcommand *dc = doorcommandlist;
	doorcommandlist = dc->dcnext;
	FREE(dc);
    }
}


local bool DoAreaAddAdjustIDCMP(struct IntuiMessage *im)
{
    char k;
    bool done = false;

    switch (im->Class) {
      case IDCMP_CLOSEWINDOW:
	poseclosed = im->IDCMPWindow == bwdwin;
	wasescaped = (im->IDCMPWindow == lawin) || poseclosed;
	done = (im->IDCMPWindow == aawin) || wasescaped;
	break;
      case IDCMP_RAWKEY:
	if (im->Qualifier & ALTKEYS)
	    break;
	k = KeyToAscii(im->Code, im->Qualifier);
	switch(k) {
	  case '\r':
	  case ESC:
	    done = true;
	    break;
	  case '\t':
	    if (qwk)
		ActivateGag(&aagagextra, aawin);
	    break;
	  case 'P':
	    if (qwk)
		break;
	    if (!addpa)
		Seyn(&aagagradiopa, addpa = true);
	    if (addpers)
		Seyn(&aagagradiopers, addpers = false);
	    if (addall)
		Seyn(&aagagradioall, addall = false);
	    break;
	  case 'O':
	    if (qwk)
		break;
	    if (!addpers)
		Seyn(&aagagradiopers, addpers = true);
	    if (addpa)
		Seyn(&aagagradiopa, addpa = false);
	    if (addall)
		Seyn(&aagagradioall, addall = false);
	    break;
	  case 'A':
	    if (qwk)
		Seyn(&aagagcheckall, addall = !addall);
	    else {
		if (!addall)
		    Seyn(&aagagradioall, addall = true);
		if (addpers)
		    Seyn(&aagagradiopers, addpers = false);
		if (addpa)
		    Seyn(&aagagradiopa, addpa = false);
	    }
	    break;
	  case 'R':
	    if (doorreset)
		Seyn(&aagagcheckreset, addreset = !addreset);
	    break;
	}
	break;
      case IDCMP_GADGETUP:
        if (!qwk)
	    break;
	/* else fall through: */
      case IDCMP_GADGETDOWN:
	switch (((struct Gadget *) im->IAddress)->GadgetID) {
	  case 1750:			/* P+"All" */
	    if (!CHECK(aagagradiopa, addpa))
		Seyn(&aagagradiopa, addpa = true);
	    if (addpers)
		Seyn(&aagagradiopers, addpers = false);
	    if (addall)
		Seyn(&aagagradioall, addall = false);
	    break;
	  case 1751:			/* Personal */
	    if (!CHECK(aagagradiopers, addpers))
		Seyn(&aagagradiopers, addpers = true);
	    if (addpa)
		Seyn(&aagagradiopa, addpa = false);
	    if (addall)
		Seyn(&aagagradioall, addall = false);
	    break;
	  case 1752:			/* All */
	    if (qwk)
		CHECK(aagagcheckall, addall);
	    else {
		if (!CHECK(aagagradioall, addall))
		    Seyn(&aagagradioall, addall = true);
		if (addpers)
		    Seyn(&aagagradiopers, addpers = false);
		if (addpa)
		    Seyn(&aagagradiopa, addpa = false);
	    }
	    break;
	  case 1753:			/* Reset */
	    CHECK(aagagcheckreset, addreset);
	    break;
	  case 1754:			/* extra */
	    if (GAGFINISH(im)) {
		StripString(&aagagextra, aawin);
		if (!CheckAAAextra(aaaconf, aaextra, addall, addreset))
		    ActivateGadget(&aagagextra, aawin, null);
	    }
	    break;
	}
	break;
    }
    if (done && qwk && aaextra[0]) {
	StripString(&aagagextra, aawin);
	if (!CheckAAAextra(aaaconf, aaextra, addall, addreset)) {
	    ActivateGag(&aagagextra, aawin);
	    return false;
	}
    }
    return done;
}


bool AreaAddAdjustment(struct Conf *cc)		/* true if window opened */
{
    ushort radiospace, radioff, cab = cc->areabits;
    struct Window *oynw = ynwin;
    str dc;

    addall = addpers = addpa = addreset = false;
    if (qwk) {
	aagagcheckall.TopEdge = fakefight + checkoff + 9;
	aagagcheckreset.TopEdge = aagagcheckall.TopEdge + checkspace;
	aagagextra.TopEdge = aagagcheckreset.TopEdge
					+ fight + checkspace + checkoff + 6;
	aaneww.Height = aagagextra.TopEdge + 22;
	if (dc = GetExtraDoorCommand(cc))
	    strcpy(aaextra, dc);
	else
	    aaextra[0] = 0;
	if (cc->morebits & (DOOR_ADDING | DOOR_RESETTING)) {
	    addall = !!(cc->morebits & DOOR_ADDING);
	    addreset = !!(cc->morebits & DOOR_RESETTING);
	} else if (cc->areabits & INF_SCANNING && doorreset)
	    addreset = true;
	else if (!dc)
	    addall = true;
	AbleGad(&aagagcheckreset, doorreset);
    } else {
	if (!version3)
	    return false;
	radiospace = fakefight + 8;
	radioff = (fakefight - radiospace) / 2;
	if (radioff < 0)
	    radioff = 0;
	aagagradioall.TopEdge = fakefight + radioff + 9;
	aagagradiopers.TopEdge = aagagradioall.TopEdge + radiospace;
	aagagradiopa.TopEdge = aagagradiopers.TopEdge + radiospace;
	aaneww.Height = aagagradiopa.TopEdge + radioff + 22;
	if (cc->morebits & DOOR_ADDING_YOURS) {
	    if (cc->morebits & DOOR_ADDING)
		addpa = true;
	    else
		addpers = true;
	} else if (cc->areabits & INF_TO_ALL)
	    addpa = true;
	else if (cc->areabits & INF_PERSONAL)
	    addpers = true;
	else
	    addall = true;
    }
    FixListSlider(0, true);			/* disable scroll gadget */
    if (!(aawin = OpenBlueGagWin(&aaww, qwk ? &aagagcheckall
						: &aagagradiopa))) {
	WindowErr("adding an area to download.");
	FixListSlider(listcc->messct, true);
	return false;
    }
    ynwin = aawin;
    if (qwk) {
	SeynLabel(&aagagcheckall, addall, "ADD: read this area");
	SeynLabel(&aagagcheckreset, addreset, "RESET: ignore old msgs");
    } else {
	SeynLabel(&aagagradiopa, addpa, "Personal + to \"All\"");
	SeynLabel(&aagagradiopers, addpers, "Only personal msgs");
	SeynLabel(&aagagradioall, addall, "All messages");
    }
    aaaconf = cc;
    EventLoop(&DoAreaAddAdjustIDCMP);
    ynwin = oynw;
    CloseBlueGagWin(aawin);
    FixListSlider(listcc->messct, true);	/* restore scroll gadget */
    aawin = null;
    cc->morebits &= ~DOOR_AREAPICKS;
    if (addpa)
	if (!(cab & INF_SCANNING) || !(cab & INF_TO_ALL))
	    cc->morebits |= DOOR_ADDING | DOOR_ADDING_YOURS;
    if (addpers)
	if (!(cab & INF_SCANNING) || !(cab & INF_PERSONAL) || cab & INF_TO_ALL)
	    cc->morebits |= DOOR_ADDING_YOURS;
    if (addall)
	if (qwk || !(cab & INF_SCANNING) || cab & (INF_PERSONAL | INF_TO_ALL))
	    cc->morebits |= DOOR_ADDING;
    if (addreset)
	cc->morebits |= DOOR_RESETTING;
    if (aaextra[0])
	cc->morebits |= DOOR_ADDING_YOURS;
    return true;
}


void AreaAddDrop(struct Conf *reaz, bool add)
{
    struct Conf *cc = reaz->confs[pick];
    short a;

    ASSERT(pick >= 0 && pick < reaz->messct);
    if (!add || !AreaAddAdjustment(cc)) {
	/* Currently, ADD and DROP both cancel themselves and each other: */
	if (cc->morebits & DOOR_AREAPICKS) {
	    cc->morebits &= ~DOOR_AREAPICKS;
	    CheckAAAextra(cc, "", false, false);
	} else if (cc->areabits & INF_SCANNING) {
	    if (!add)
		cc->morebits |= DOOR_DROPPING;
	    else if (qwk)	/* allow adding an area with a message in it */
		cc->morebits |= DOOR_ADDING;
	    else
		return;
	} else if (add)
	    cc->morebits |= DOOR_ADDING;
	else if (qwk)
	    cc->morebits |= DOOR_DROPPING;
	else
	    return;
    }
    RedisplayPickLine(reaz);
    addropchanges = true;
    for (a = 0; a < reaz->messct; a++)
	if (reaz->confs[a]->morebits & DOOR_AREAPICKS) {
	    if (!addsdrops) {
		addsdrops = true;
		FlipBGadgets(addropmask | 8);
	    }
	    return;
	}
    addsdrops = false;
    FlipBGadgets(addropmask);
}


void ResetAddsAndDrops(struct Conf *reaz)
{
    short a;

    if (!addsdrops || !AskResetDoorFig(true))
	return;
    for (a = 0; a < reaz->messct; a++)
	reaz->confs[a]->morebits &= ~DOOR_AREAPICKS;
    FlushExtraDoorCommands();
    ListAllLines(reaz);
    addsdrops = false;
    addropchanges = true;
    FlipBGadgets(addropmask);
}


void GhostBWDoor(bool un)
{
    struct Gadget *gg = &bwgagkeyfil;
    short a, gact = bwdww.gacount;

    if (bwdwin)
	for (a = 0; a < gact; a++, gg = gg->NextGadget)
	    AbleAddGad(gg, bwdwin, un &&
				(gg != &bwgagreset || PDQstateDiffers()));
    if (bkfwin)
	for (a = 0; a < 20; a++)
	    AbleAddGad(&bkfgag[a], bkfwin, un);
}


bool ToggleDownloadAreas(void)
{
    short initial;
    struct Conf *targ = readareaz.confs[whicha];

    ASSERT(whicha >= 0 && whicha < readareaz.messct);
    if (targ == &replies)
	targ = Confind(replies.messes[whichm]->confnum);
    for (initial = 0; initial < areaz.messct; initial++)
	if (areaz.confs[initial] == targ)
	    goto useit;
    initial = 0;
  useit:
    GhostBWDoor(false);
    poseclosed = false;
    do {
	if (newareawin)
	    initial = AddUnknownArea(initial);
	if (!poseclosed)
	    initial = ListAreas(initial, false, true, false);
    } while (newareawin);
    if (!poseclosed)
	GhostBWDoor(true);
    return poseclosed;
}


void CloseKeyFilWin(void)
{
    if (bkfwin) {
	if (lawin && addropping)
	    bkfww.oldgagmask = qwk ? 0x3E : 0x2E;
	CloseBlueGagWin(bkfwin);
    }
    bkfwin = null;
}


void OpenKeyFilWin(void)
{
    register struct RastPort *rr;
    long labelbase, center;
    short a, h;

    if (bkfwin)
	CloseKeyFilWin();
    else {
	for (a = 0; a < 20; a++) {
	    if (!(a % 5))
		h = fakefight + 4;
	    h += fakefight + 12;
	    bkfgag[a].TopEdge = h;
	    bkfgag[a].Flags &= ~GFLG_DISABLED;
	}
	bkfneww.Height = h + fakefight + 11;
	bkfneww.TopEdge = scr->Height - bkfneww.Height - (10 << lace);
	/* height when fakefight = 12: 159 */
	if (bkfwin = OpenBlueGagWin(&bkfww, &bkfgag[0])) {
	    rr = bkfwin->RPort;
	    SetAPen(rr, LABELCOLOR);
	    SetBPen(rr, backcolor);
	    center = XActual(311) + (fontwid >= 12);
	    Move(rr, center, fight + 6);
	    Draw(rr, center, bkfwin->Height - 6);
	    Move(rr, --center, bkfwin->Height - 6);
	    Draw(rr, center, fight + 6);
	    labelbase = bkfgag[0].TopEdge + font->tf_Baseline - fakefight - 7;
	    MoveNominal(rr, 66, labelbase);
	    Text(rr, "Keywords to look for:", 21);
	    MoveNominal(rr, 366, labelbase);
	    Text(rr, "Filter words to avoid:", 22);
	} else
	    WindowErr("setting keywords and filters.");
    }
}


void UpperStrip(struct Gadget *gg, struct Window *ww)
{
    str word = ((struct StringInfo *) gg->SpecialInfo)->Buffer;

    strcpy(undospace, word);
    Stranks(word);
    strupr(word);
    if (strcmp(word, undospace))
	RefreshGList(gg, ww, null, 1);
}


bool ConfigBWDoorIDCMP(struct IntuiMessage *im)
{
    short a, shifty = im->Qualifier & SHIFTKEYS;
    bool finn = GAGFINISH(im), tweak = true;
    char k;

    switch (im->Class) {
      case IDCMP_CLOSEWINDOW:
	tweak = false;
	if (im->IDCMPWindow == bkfwin)
	    CloseKeyFilWin();
	else
	    return true;
	break;
      case IDCMP_GADGETUP:
	a = ((struct Gadget *) im->IAddress)->GadgetID;
	if (bkfwin && a >= 1600 && a < 1620) {
	    if (finn) {
		UpperStrip(&bkfgag[a -= 1600], bkfwin);
		if (GAGCHAIN(im)) {
		    if ((shifty ? --a : ++a) < 0)
			a = 19;
		    if (a < 20)
			ActivateGadget(&bkfgag[a], bkfwin, null);
		}
	    }
	} else switch (a) {
	  case /* NCC- */ 1701:
	    ResetDoorOptions();
	    tweak = false;
	    break;
	  case 1702:
	    OpenKeyFilWin();
	    tweak = false;
	    break;
	  case 1703:
	    tweak = false;
	    return ToggleDownloadAreas();
	  case 1704:
	    CHECK(bwgagownmail, bwownmail);
	    break;
	  case 1705:
	    CHECK(bwgagcolor, bwgraphics);
	    break;
	  case 1706:
	    CHECK(bwgaghotkeys, bwhotkeys);
	    break;
	  case 1707:
	    CHECK(bwgagexpert, bwexpert);
	    break;
	  case 1708:
	    if (finn) {
		UpperStrip(&bwgagmacro1, bwdwin);
		if (GAGCHAIN(im))
		    ActivateGadget(shifty ? &bwgagpassword
					: &bwgagmacro2, bwdwin, null);
	    }
	    break;
	  case 1709:
	    if (finn) {
		UpperStrip(&bwgagmacro2, bwdwin);
		if (GAGCHAIN(im))
		    ActivateGadget(shifty ? &bwgagmacro1
					: &bwgagmacro3, bwdwin, null);
	    }
	    break;
	  case 1710:
	    if (finn) {
		UpperStrip(&bwgagmacro3, bwdwin);
		if (GAGCHAIN(im) && shifty)
		    ActivateGadget(&bwgagmacro2, bwdwin, null);
	    }
	    break;
	  case 1711:
	    FreshPassType(shifty ? -1 : 1);
	    break;
	  case 1712:
	    if (finn) {
		UpperStrip(&bwgagpassword, bwdwin);
		if (GAGCHAIN(im))
		    ActivateGadget(shifty ? (version3 ? &bwgagpksize :
				    &bwgagmacro3) : &bwgagmacro1, bwdwin, null);
	    }
	    break;
	  case 1713:
	    CHECK(bwgagnoxlines, bwnoxlines);
	    break;
	  case 1714:
	    CHECK(bwgagnumeric, bwnumericext);
	    break;
	  case 1715:
	    CHECK(bwgagauto1, bwauto1);
	    break;
	  case 1716:
	    CHECK(bwgagauto2, bwauto2);
	    break;
	  case 1717:
	    CHECK(bwgagauto3, bwauto3);
	    break;
	  case 1718:
	    if (finn && version3) {
		if (bwstrpksize.LongInt & 0xFFFF8000 || !bwstrpksize.LongInt) {
		    long p = RemoveGadget(bwdwin, &bwgagpksize);
		    pksizenumspace[0] = bwpksize = bwstrpksize.LongInt = 0;
		    AddGadget(bwdwin, &bwgagpksize, p);
		    RefreshGList(&bwgagpksize, bwdwin, null, 1);
		}
		if (GAGCHAIN(im))
		    ActivateGadget(shifty ? &bwgagmacro3
					: &bwgagpassword, bwdwin, null);
	    }
	    break;
	  case 1719:
	    FreshFListType(shifty ? -1 : 1);
	    break;
	  default:
	    tweak = false;
	}
	break;
      case IDCMP_RAWKEY:
	if (im->Qualifier & ALTKEYS)
	    break;
	k = KeyToAscii(im->Code, im->Qualifier);
	switch (k) {
	  case '\t':
	    tweak = false;
	    if (bkfwin && im->IDCMPWindow != bwdwin)
		ActivateGag(&bkfgag[0], bkfwin);
	    else
		ActivateGag(&bwgagmacro1, bwdwin);
	    break;
	  case ESC:
	    tweak = false;
	    if (bkfwin && im->IDCMPWindow != bwdwin)
		CloseKeyFilWin();
	    else
		return true;
	    break;
	  case 'A':
	    tweak = false;
	    return ToggleDownloadAreas();
	  case 'C':
	    Seyn(&bwgagcolor, bwgraphics = !bwgraphics);
	    break;
	  case 'E':
	    Seyn(&bwgagexpert, bwexpert = !bwexpert);
	    break;
	  case 'H':
	    Seyn(&bwgaghotkeys, bwhotkeys = !bwhotkeys);
	    break;
	  case 'I':
	    Seyn(&bwgagownmail, bwownmail = !bwownmail);
	    break;
	  case 'F':
	    Seyn(&bwgagnoxlines, bwnoxlines = !bwnoxlines);
	    break;
	  case 'N':
	    Seyn(&bwgagnumeric, bwnumericext = !bwnumericext);
	    break;
	  case '1':
	    Seyn(&bwgagauto1, bwauto1 = !bwauto1);
	    break;
	  case '2':
	    Seyn(&bwgagauto2, bwauto2 = !bwauto2);
	    break;
	  case '3':
	    Seyn(&bwgagauto3, bwauto3 = !bwauto3);
	    break;
	  case 'K':
	    OpenKeyFilWin();
	    tweak = false;
	    break;
	  case 'R':
	    tweak = false;
	    ResetDoorOptions();
	    break;
	  case 'W':
	    FreshPassType(shifty ? -1 : 1);
	    break;
	  case 'L':
	    FreshFListType(shifty ? -1 : 1);
	    break;
	default:
	    tweak = false;
	}
	break;
      default:
	tweak = false;
    }
    if (tweak) {
	now_difference = true;
	AbleAddGad(&bwgagreset, bwdwin, PDQstateDiffers());
    }
    return false;
}


void ConfigBWDoor(void)
{
    struct Gadget *gg;
    short i;
    char prevkeywords[10][21], prevfilters[10][21];
    bool maximus = !stricmp(doorconn, "MAXIMUS");

     if (forbidOLC) {
	Err("This BBS does not support\nany offline configuration of\n"
					"the Blue Wave mail packer.");
	return;
    }
    addropchanges = false;
    if (qwk) {
	if ((!dooraddrop || maximus) && !doorwarnedonce) {
	    WarnNoDoorIdAddDrop(maximus);
	    doorwarnedonce = true;
	}
	ToggleDownloadAreas();
	if (addropchanges) {
	    repchanges = true;
	    WriteUPL();		/* => WriteREP() */
	}
    } else {
	for (i = 0; i < 10; i++) {
	    strcpy(prevkeywords[i], bwkeywords[i]);
	    strcpy(prevfilters[i], bwfilters[i]);
	};
	StripKFandM();
	now_difference = false;
	if (bwstrpksize.LongInt = bwpksize)
	    utoa(bwpksize, pksizenumspace);
	else
	    pksizenumspace[0] = 0;
	for (gg = &bwgagkeyfil; gg; gg = gg->NextGadget)
	    gg->Flags &= ~GFLG_DISABLED;
	bwgaghotkeys.TopEdge = bwgagexpert.TopEdge = fakefight + 9 + checkoff;
	bwgagcolor.TopEdge = bwgagownmail.TopEdge
				= bwgaghotkeys.TopEdge + checkspace;
	if (version3) {
	    bwgagnoxlines.TopEdge = bwgagnumeric.TopEdge
				= bwgagcolor.TopEdge + checkspace;
	    LAST_V2GAG.NextGadget = &bwgagnoxlines;
	    bwgagmacro1.Width = bwgagmacro2.Width = bwgagmacro3.Width = 208;
	    bwgagnewfiles.TopEdge = bwgagnoxlines.TopEdge
					+ checkspace - checkoff;
	    bwgagpksize.TopEdge = bwgagnewfiles.TopEdge + 2;
	    bwgagpassword.TopEdge = bwgagnewfiles.TopEdge + fakefight + 12;
	} else {
	    LAST_V2GAG.NextGadget = null;
	    bwgagmacro1.Width = bwgagmacro2.Width = bwgagmacro3.Width = 248;
	    bwgagpassword.TopEdge = bwgagcolor.TopEdge
					+ 3 + checkspace - checkoff;
	}
	bwgagpasswhen.TopEdge = bwgagpassword.TopEdge - 3;
	bwgagmacro1.TopEdge = bwgagpassword.TopEdge + fakefight + 12;
	bwgagmacro2.TopEdge = bwgagmacro1.TopEdge + fakefight + 12;
	bwgagmacro3.TopEdge = bwgagmacro2.TopEdge + fakefight + 12;
	if (version3) {
	    i = (fight - checktall) / 2;
	    bwgagauto1.TopEdge = bwgagmacro1.TopEdge + i;
	    bwgagauto2.TopEdge = bwgagmacro2.TopEdge + i;
	    bwgagauto3.TopEdge = bwgagmacro3.TopEdge + i;
	}
	bwgagkeyfil.TopEdge = bwgagmacro1.TopEdge - 2;
	bwgagareas.TopEdge = bwgagmacro2.TopEdge - 2;
	bwgagreset.TopEdge = bwgagmacro3.TopEdge - 2;
	bwdneww.Height = bwgagreset.TopEdge + fakefight + 11;
	/* height when fakefight == 11 and lace and version3:  */
	bwgagpasswhen.LeftEdge = bwgagnewfiles.LeftEdge = 444 + XActual(5);
	bwdneww.TopEdge = lace ? 100 : 10;
	AbleGad(&bwgagreset, PDQstateDiffers());
	if (!(bwdwin = OpenBlueGagWin(&bwdww, &bwgagkeyfil))) {
	    WindowErr("configuring Blue Wave door options.");
	    return;
	}

	ynwin = bwdwin;
	FreshPassType(0);
	if (version3)
	    FreshFListType(0);
	UnderstartedText(bwgagpasswhen.LeftEdge - XActual(103),
				bwgagpasswhen.TopEdge + 3, bwdwin->RPort,
				"When to ask:");
	SeynLabel(&bwgaghotkeys, bwhotkeys, "Hotkey menu commands");
	SeynLabel(&bwgagcolor, bwgraphics, "Color graphics in menus");
	SeynLabel(&bwgagexpert, bwexpert, "Expert (brief) menus");
	SeynLabel(&bwgagownmail, bwownmail, "Include my own messages");
	if (version3) {
	    SeynLabel(&bwgagnoxlines, bwnoxlines,
					"Filter out Fido control lines");
	    SeynLabel(&bwgagnumeric, bwnumericext, "Numeric file extensions");
	    SeynLabel(&bwgagauto1, bwauto1, "#1 auto");
	    SeynLabel(&bwgagauto2, bwauto2, "#2 auto");
	    SeynLabel(&bwgagauto3, bwauto3, "#3 auto");
	    UnderstartedText(bwgagnewfiles.LeftEdge - XActual(151),
				 bwgagnewfiles.TopEdge + 3, bwdwin->RPort,
				 "List of new files:");
	}
	EventLoop(&ConfigBWDoorIDCMP);
	CloseKeyFilWin();
	CloseBlueGagWin(bwdwin);
	ynwin = bwdwin = null;
	StripKFandM();
	for (i = 0; i < 10; i++)
	    if (strcmp(prevkeywords[i], bwkeywords[i])
				|| strcmp(prevfilters[i], bwfilters[i])) {
		now_difference = true;
		break;
	    }
	if (now_difference | addropchanges) {
	    if (strcmp(bwpassword, oldpassword))
		Stranks(bwpassword);
	    Stranks(macro1);
	    Stranks(macro2);
	    Stranks(macro3);
	    for (i = 0; i < 10; i++) {
		Stranks(bwkeywords[i]);
		Stranks(bwfilters[i]);
	    }
	    if (version3)
		if (bwstrpksize.LongInt & 0xFFFF8000 || !bwstrpksize.LongInt)
		    bwpksize = 0;
		else
		    bwpksize = bwstrpksize.LongInt;
	    WritePDQ(PDQstateDiffers());
	    repchanges = true;
	}
	pdq_difference |= now_difference;
	now_difference = false;			/* jusfurthellavit */
    }
    DecideIfAnythingToSave();
}
