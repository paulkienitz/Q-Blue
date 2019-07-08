/* the compressors setup window */

#include <intuition/sghooks.h>
#include <intuition/intuition.h>
#include <graphics/displayinfo.h>
#include <graphics/gfxmacros.h>
#include <libraries/asl.h>
#include "qblue.h"
#include "pigment.h"


#define PLONGGAG     496
#define PWINWIDTH    536
#define OWINWIDTH    547
#define OWINHEIGHT   178


void FullPresentPackers(void);
bool MakeLotsaRoom(void);
void MakeBotGadgets(void);
bool ClipboardStringPaste(struct IntuiMessage *im);


import struct IntuiText *(cgtquotes[5]);
import struct Gadget llgagtagstyle, llgagtagleadin;
import struct Border elongbox, dpathbox;
import short under[], upleft[], downright[];

import char packernames[GREENBAY][PACKNAMELEN], spoors[GREENBAY][SPOORLEN];
import char packommands[GREENBAY][COMMANDLEN], taglinesfile[];
import char unpackommands[GREENBAY][COMMANDLEN];

import long undercolor;
import ushort wrapmargin, quotedefault, packers, currentpacker;
import ushort tagstyle, tagleadin, localtagstyle, localtagleadin, askdelete;
import bool bgupdate, beepu, showlistwin, formfeed, showareawin;
import bool addRe, showsizes, flushreplies, popupscroller;


char signature[SIGNATURELEN + 2] = "";
char quoteheader[COMMANDLEN + 4] = "At @T on @D, @A said to @R:@N";
char carbonheader[COMMANDLEN + 4] =
		" ** Message forwarded by Q-Blue @V@N"
		" ** Posted @T on @D in area \"@Z@C\"@N"
		" ** From @A to @R@N ** Subject \"@Z@S\"@N";

bool noactivatedammit = false;

short yndownright[10] = { 1, 12, CHECKWID - 2, 12, CHECKWID - 2, 1,
				CHECKWID - 1, 0, CHECKWID - 1, 12 };
short ynupleft[10] = { 0, 0, 0, 12, 1, 11, 1, 0, CHECKWID - 2, 0 };


struct Border upynbor2 = {
    0, 0, SHADOWCOLOR, 0, JAM2, 5, yndownright, null
};

struct Border upynborder = {
    0, 0, SHINECOLOR, 0, JAM2, 5, ynupleft, &upynbor2
};


short checkthicken[4] = { 7, 5, 10, 8 };
short checkmark[14] = { 19, 2, 17, 2, 11, 8, 8, 5, 9, 5, 12, 8, 17, 3 };

struct Border checkthickenyes = {
    0, 1, CHECKCOLOR, 0, JAM2, 2, checkthicken, &upynborder
};

struct Border checkthickenno = {
    0, 1, FILLCOLOR, 0, JAM2, 2, checkthicken, &upynborder
};

struct Border checkmarkyes = {
    0, 1, CHECKCOLOR, 0, JAM2, 7, checkmark, &checkthickenyes
};

struct Border checkmarkno = {
    0, 1, FILLCOLOR, 0, JAM2, 7, checkmark, &checkthickenno
};


struct Gadget ogagpopupscroller = {
    null, 16, 133, CHECKWID, 13, CHECKHILITE, CHECKACTION,
    GTYP_BOOLGADGET, &checkmarkno, &checkmarkyes, null, 0, null, 322, null
};

struct Gadget ogagflushreplies = {
    &ogagpopupscroller, 282, 45, CHECKWID, 13, CHECKHILITE, CHECKACTION,
    GTYP_BOOLGADGET, &checkmarkno, &checkmarkyes, null, 0, null, 321, null
};

struct Gadget ogagshowsizes = {
    &ogagflushreplies, 16, 45, CHECKWID, 13, CHECKHILITE, CHECKACTION,
    GTYP_BOOLGADGET, &checkmarkno, &checkmarkyes, null, 0, null, 320, null
};

struct Gadget ogagaskdelete = {
    &ogagshowsizes, 282, 45, CHECKWID, 13, CHECKHILITE, CHECKACTION,
    GTYP_BOOLGADGET, &checkmarkno, &checkmarkyes, null, 0, null, 319, null
};

struct Gadget ogagstrchain = {
    &ogagaskdelete, 16, 45, CHECKWID, 13, CHECKHILITE, CHECKACTION,
    GTYP_BOOLGADGET, &checkmarkno, &checkmarkyes, null, 0, null, 314, null
};

struct Gadget ogagffeed = {
    &ogagstrchain, 16, 45, CHECKWID, 13, CHECKHILITE, CHECKACTION,
    GTYP_BOOLGADGET, &checkmarkno, &checkmarkyes, null, 0, null, 310, null
};

struct Gadget ogagbeepu = {
    &ogagffeed, 282, 67, CHECKWID, 13, CHECKHILITE, CHECKACTION,
    GTYP_BOOLGADGET, &checkmarkno, &checkmarkyes, null, 0, null, 307, null
};

struct Gadget ogagarea = {
    &ogagbeepu, 16, 89, CHECKWID, 13, CHECKHILITE, CHECKACTION,
    GTYP_BOOLGADGET, &checkmarkno, &checkmarkyes, null, 0, null, 312, null
};

struct Gadget ogagaddRe = {
    &ogagarea, 282, 111, CHECKWID, 13, CHECKHILITE, CHECKACTION,
    GTYP_BOOLGADGET, &checkmarkno, &checkmarkyes, null, 0, null, 313, null
};

struct Gadget ogaglist = {
    &ogagaddRe, 16, 67, CHECKWID, 13, CHECKHILITE, CHECKACTION,
    GTYP_BOOLGADGET, &checkmarkno, &checkmarkyes, null, 0, null, 306, null
};

struct Gadget ogagbgup = {
    &ogaglist, 282, 89, CHECKWID, 13, CHECKHILITE, CHECKACTION,
    GTYP_BOOLGADGET, &checkmarkno, &checkmarkyes, null, 0, null, 305, null
};

struct Gadget ogagwaste = {
    &ogagbgup, 282, 45, CHECKWID, 13, CHECKHILITE, CHECKACTION,
    GTYP_BOOLGADGET, &checkmarkno, &checkmarkyes, null, 0, null, 304, null
};

struct Gadget ogaggagrow = {
    &ogagwaste, 16, 111, CHECKWID, 13, CHECKHILITE, CHECKACTION,
    GTYP_BOOLGADGET, &checkmarkno, &checkmarkyes, null, 0, null, 303, null
};


struct ExtNewWindow oneww = {
    46, 7, OWINWIDTH, OWINHEIGHT, 0, 1,
    IDCMP_GADGETUP | IDCMP_CLOSEWINDOW | IDCMP_RAWKEY,
    WFLG_SMART_REFRESH | WFLG_CLOSEGADGET | WFLG_DEPTHGADGET
		| WFLG_DRAGBAR | WFLG_ACTIVATE | WFLG_NW_EXTENDED,
    null, null, "Miscellaneous preference options",
    null, null, 0, 0, 0, 0, CUSTOMSCREEN, null
};

struct WhereWin oww = { &oneww };


struct StringInfo ystranyname = STRINF(anyname, NAMELEN);

struct IntuiText ylabelanyname = {
    LABELCOLOR, 0, JAM1, -220, 0, null, "Default alias name:", null
};

STRINGBORDER(yanynamebox)

struct Gadget ygaganyname = {
    null, 237, 169, 288, 8, GFLG_STRINGEXTEND | GFLG_TABCYCLE,
    GACT_RELVERIFY | GACT_STRINGLEFT, GTYP_STRGADGET,
    &yanynamebox, null, &ylabelanyname, 0, &ystranyname, 302, null
};


struct StringInfo ystrtagfile = STRINF(taglinesfile, COMMANDLEN);

struct IntuiText ylabeltagfile = {
    LABELCOLOR, 0, JAM1, -220, 0, null, "File containing taglines:", null
};

struct Gadget ygagtagfile = {
    &ygaganyname, 237, 169, 288, 8, GFLG_STRINGEXTEND | GFLG_TABCYCLE,
    GACT_RELVERIFY | GACT_STRINGLEFT, GTYP_STRGADGET,
    &yanynamebox, null, &ylabeltagfile, 0, &ystrtagfile, 315, null
};


STRINGBORDER(yqheadbox)

struct StringInfo ystrsignature = STRINF(signature, SIGNATURELEN);

struct IntuiText ylabelsignature = {
    LABELCOLOR, 0, JAM1, -116, 0, null, "Signature:", null
};

struct Gadget ygagsignature = {
    &ygagtagfile, 133, 169, 392, 8, GFLG_STRINGEXTEND | GFLG_TABCYCLE,
    GACT_RELVERIFY | GACT_STRINGLEFT, GTYP_STRGADGET,
    &yqheadbox, null, &ylabelsignature, 0, &ystrsignature, 317, null
};


struct StringInfo ystrcchead = STRINF(carbonheader, COMMANDLEN);

struct IntuiText ylabelcchead = {
    LABELCOLOR, 0, JAM1, -116, 0, null, "C.C. header:", null
};

struct Gadget ygagcchead = {
    &ygagsignature, 133, 169, 392, 8, GFLG_STRINGEXTEND | GFLG_TABCYCLE,
    GACT_RELVERIFY | GACT_STRINGLEFT, GTYP_STRGADGET,
    &yqheadbox, null, &ylabelcchead, 0, &ystrcchead, 316, null
};


struct StringInfo ystrqhead = STRINF(quoteheader, COMMANDLEN);

struct IntuiText ylabelqhead = {
    LABELCOLOR, 0, JAM1, -116, 0, null, "Quote header:", null
};

struct Gadget ygagqhead = {
    &ygagcchead, 133, 169, 392, 8, GFLG_STRINGEXTEND | GFLG_TABCYCLE,
    GACT_RELVERIFY | GACT_STRINGLEFT, GTYP_STRGADGET,
    &yqheadbox, null, &ylabelqhead, 0, &ystrqhead, 311, null
};


struct IntuiText ygtleadstealth = {
    TEXTCOLOR, FILLCOLOR, JAM2, 27, 3, null, " Stealth ", null
};

struct IntuiText ygtglobal = {
    TEXTCOLOR, FILLCOLOR, JAM2, 31, 3, null, "(global)", null
};

struct IntuiText ygtleadqblue = {
    TEXTCOLOR, FILLCOLOR, JAM2, 31, 3, null, "* Q-Blue", null
};

struct IntuiText ygtleaddots = {
    TEXTCOLOR, FILLCOLOR, JAM2, 31, 3, null, "  ...   ", null
};

struct IntuiText *(ygttagleadins[4]) = {
    &ygtleaddots, &ygtleadqblue, &ygtglobal, &ygtleadstealth
};

struct Gadget ygagtagleadin = {
    &ygagqhead, 425, 173, 106, 12, GFLG_GADGHCOMP, GACT_RELVERIFY,
    GTYP_BOOLGADGET, &upcyclebor, null, null, 0, null, 301, null
};


struct IntuiText ygttagmanual = {
    TEXTCOLOR, FILLCOLOR, JAM2, 31, 3, null, " Manual ", null
};

struct IntuiText ygttagsequence = {
    TEXTCOLOR, FILLCOLOR, JAM2, 31, 3, null, "Sequence", null
};

struct IntuiText ygttagrandom = {
    TEXTCOLOR, FILLCOLOR, JAM2, 31, 3, null, " Random ", null
};

struct IntuiText ygttagnone = {
    TEXTCOLOR, FILLCOLOR, JAM2, 31, 3, null, "  None  ", null
};

struct IntuiText *(ygttagstyles[5]) = {
    &ygttagnone, &ygttagrandom, &ygttagsequence, &ygttagmanual, &ygtglobal
};

struct Gadget ygagtagstyle = {
    &ygagtagleadin, 183, 153, 106, 12, GFLG_GADGHCOMP, GACT_RELVERIFY,
    GTYP_BOOLGADGET, &upcyclebor, null, null, 0, null, 300, null
};


ubyte margin[14];

struct StringInfo ystrmargin = STRINF(margin, 3);

struct IntuiText ylabelmargin = {
    LABELCOLOR, 0, JAM1, -164, 0, null, "Quote right margin:", null
};

STRINGBORDER(yfsizebox)

struct Gadget ygagmargin = {
    &ygagtagstyle, 501, 156, 24, 8, GFLG_STRINGEXTEND | GFLG_TABCYCLE,
    GACT_RELVERIFY | GACT_STRINGLEFT | GACT_LONGINT, GTYP_STRGADGET,
    &yfsizebox, null, &ylabelmargin, 0, &ystrmargin, 309, null
};


struct Gadget ygagquote = {
    &ygagmargin, 183, 153, 106, 12, GFLG_GADGHCOMP, GACT_RELVERIFY,
    GTYP_BOOLGADGET, &upcyclebor, null, null, 0, null, 308, null
};


struct ExtNewWindow yneww = {
    46, 7, OWINWIDTH, OWINHEIGHT, 0, 1,
    IDCMP_GADGETUP | IDCMP_CLOSEWINDOW | IDCMP_RAWKEY,
    WFLG_SMART_REFRESH | WFLG_CLOSEGADGET | WFLG_DEPTHGADGET
		| WFLG_DRAGBAR | WFLG_ACTIVATE | WFLG_NW_EXTENDED,
    null, null, "Options for writing messages and replies",
    null, null, 0, 0, 0, 0, CUSTOMSCREEN, null
};

struct WhereWin yww = { &yneww };


/* ------------------------------------------------------------------------ */


ubyte pblanks[COMMANDLEN] = "                                      "
				"                                         ";
struct StringInfo pstrpack = STRINF(pblanks, COMMANDLEN);

struct IntuiText plabelpack = {
    LABELCOLOR, 0, JAM1, 0, -20, null,
    "Compression command; \"@A\" = archive name, \"@F\" = file list:", null
};

struct Gadget pgagpack = {
    null, 20, 141, PLONGGAG, 8, GFLG_STRINGEXTEND
				| GFLG_DISABLED | GFLG_TABCYCLE,
    GACT_RELVERIFY | GACT_STRINGLEFT, GTYP_STRGADGET,
    &elongbox, null, &plabelpack, 0, &pstrpack, 612, null
};


struct StringInfo pstrunpack = STRINF(pblanks, COMMANDLEN);

struct IntuiText plabelunpack = {
    LABELCOLOR, 0, JAM1, 0, -20, null,
    "Decompression command; use \"@A\" in place of archive name:", null
};

struct Gadget pgagunpack = {
    &pgagpack, 20, 105, PLONGGAG, 8, GFLG_STRINGEXTEND
				| GFLG_DISABLED | GFLG_TABCYCLE,
    GACT_RELVERIFY | GACT_STRINGLEFT, GTYP_STRGADGET,
    &elongbox, null, &plabelunpack, 0, &pstrunpack, 611, null
};


ubyte pnameblanks[10] = "        ";

struct StringInfo pstrcname = STRINF(pnameblanks, 8);

struct IntuiText plabelcname = {
    LABELCOLOR, 0, JAM1, -52, 0, null, "Name:", null
};

STRINGBORDER(peightball)

struct Gadget pgagcname = {
    &pgagunpack, 72, 67, 64, 8,
    GFLG_STRINGEXTEND | GFLG_DISABLED | GFLG_TABCYCLE,
    GACT_RELVERIFY | GACT_STRINGLEFT, GTYP_STRGADGET,
    &peightball, null, &plabelcname, 0, &pstrcname, 610, null
};


struct StringInfo pstrspoor = STRINF(pblanks, SPOORLEN);

struct IntuiText plabelspoor = {
    LABELCOLOR, 0, JAM1, -76, 0, null, "Pattern:", null
};

STRINGBORDER(pspoorcrust)

struct Gadget pgagspoor = {
    &pgagcname, 239, 67, 168, 8,
    GFLG_STRINGEXTEND | GFLG_DISABLED | GFLG_TABCYCLE,
    GACT_RELVERIFY | GACT_STRINGLEFT, GTYP_STRGADGET,
    &pspoorcrust, null, &plabelspoor, 0, &pstrspoor, 615, null
};


struct IntuiText plabeladd = {
    TEXTCOLOR, FILLCOLOR, JAM2, 12, 2, null, "Add", null
};

struct Gadget pgagadd = {
    &pgagspoor, 434, 65, 80, 13, GFLG_GADGHCOMP, GACT_RELVERIFY,
    GTYP_BOOLGADGET, &upborder, null, &plabeladd, 0, null, 613, null
};


struct Border punderborder = {
    -34, 1, UNDERCOLOR, 0, JAM2, 2, under, null
};

struct Border pupborder2 = {
    0, 0, SHADOWCOLOR, 0, JAM2, 5, downright, &punderborder
};

struct Border pupborder = {
    0, 0, SHINECOLOR, 0, JAM2, 5, upleft, &pupborder2
};

struct IntuiText plabelc8 = {
    TEXTCOLOR, FILLCOLOR, JAM2, 12, 3, null, (ustr) &packernames[7], null
};

struct Gadget pgagc8 = {
    &pgagadd, 434, 43, 80, 13, GFLG_GADGHCOMP | GFLG_DISABLED,
    GACT_RELVERIFY | GACT_TOGGLESELECT, GTYP_BOOLGADGET,
    &pupborder, null, &plabelc8, 0, null, 607, null
};

struct IntuiText plabelc7 = {
    TEXTCOLOR, FILLCOLOR, JAM2, 12, 3, null, (ustr) &packernames[6], null
};

struct Gadget pgagc7 = {
    &pgagc8, 304, 43, 80, 13, GFLG_GADGHCOMP | GFLG_DISABLED,
    GACT_RELVERIFY | GACT_TOGGLESELECT, GTYP_BOOLGADGET,
    &pupborder, null, &plabelc7, 0, null, 606, null
};

struct IntuiText plabelc6 = {
    TEXTCOLOR, FILLCOLOR, JAM2, 12, 3, null, (ustr) &packernames[5], null
};

struct Gadget pgagc6 = {
    &pgagc7, 174, 43, 80, 13, GFLG_GADGHCOMP | GFLG_DISABLED,
    GACT_RELVERIFY | GACT_TOGGLESELECT, GTYP_BOOLGADGET,
    &pupborder, null, &plabelc6, 0, null, 605, null
};

struct IntuiText plabelc5 = {
    TEXTCOLOR, FILLCOLOR, JAM2, 12, 3, null, (ustr) &packernames[4], null
};

struct Gadget pgagc5 = {
    &pgagc6, 44, 43, 80, 13, GFLG_GADGHCOMP | GFLG_DISABLED,
    GACT_RELVERIFY | GACT_TOGGLESELECT, GTYP_BOOLGADGET,
    &pupborder, null, &plabelc5, 0, null, 604, null
};

struct IntuiText plabelc4 = {
    TEXTCOLOR, FILLCOLOR, JAM2, 12, 3, null, (ustr) &packernames[3], null
};

struct Gadget pgagc4 = {
    &pgagc5, 434, 21, 80, 13, GFLG_GADGHCOMP | GFLG_DISABLED,
    GACT_RELVERIFY | GACT_TOGGLESELECT, GTYP_BOOLGADGET,
    &pupborder, null, &plabelc4, 0, null, 603, null
};

struct IntuiText plabelc3 = {
    TEXTCOLOR, FILLCOLOR, JAM2, 12, 3, null, (ustr) &packernames[2], null
};

struct Gadget pgagc3 = {
    &pgagc4, 304, 21, 80, 13, GFLG_GADGHCOMP | GFLG_DISABLED,
    GACT_RELVERIFY | GACT_TOGGLESELECT, GTYP_BOOLGADGET,
    &pupborder, null, &plabelc3, 0, null, 602, null
};

struct IntuiText plabelc2 = {
    TEXTCOLOR, FILLCOLOR, JAM2, 12, 3, null, (ustr) &packernames[1], null
};

struct Gadget pgagc2 = {
    &pgagc3, 174, 21, 80, 13, GFLG_GADGHCOMP | GFLG_DISABLED,
    GACT_RELVERIFY | GACT_TOGGLESELECT, GTYP_BOOLGADGET,
    &pupborder, null, &plabelc2, 0, null, 601, null
};

struct IntuiText plabelc1 = {
    TEXTCOLOR, FILLCOLOR, JAM2, 12, 3, null, (ustr) &packernames[0], null
};

struct Gadget pgagc1 = {
    &pgagc2, 44, 21, 80, 13, GFLG_GADGHCOMP | GFLG_DISABLED,
    GACT_RELVERIFY | GACT_TOGGLESELECT, GTYP_BOOLGADGET,
    &pupborder, null, &plabelc1, 0, null, 600, null
};


struct Gadget *nameg[GREENBAY];


struct ExtNewWindow pneww = {
    52, 20, PWINWIDTH, 162, 0, 1,
    IDCMP_GADGETUP | IDCMP_CLOSEWINDOW | IDCMP_RAWKEY | IDCMP_INTUITICKS,
    WFLG_SMART_REFRESH | WFLG_CLOSEGADGET | WFLG_DEPTHGADGET
		| WFLG_DRAGBAR | WFLG_ACTIVATE | WFLG_NW_EXTENDED,
    null, null, "Compression methods for packing and unpacking files",
    null, null, 0, 0, 0, 0, CUSTOMSCREEN, null
};

struct WhereWin pww = { &pneww };


struct Window *owin;
struct RastPort *orp;


/* --------------------------------------------------------------------- */


void NukePacker(short k)
{
    short i;
    void UnSelecticate(short k);

    for (i = 0; i < packers; i++)
	if (nameg[i]->Flags & GFLG_SELECTED) {
	    nameg[i]->Flags &= ~GFLG_SELECTED;
	    UnSelecticate((short) (i - 100));
	}
    for (i = k + 1; i < packers; i++) {
	strcpy(packernames[i - 1], packernames[i]);
	strcpy(packommands[i - 1], packommands[i]);
	strcpy(unpackommands[i - 1], unpackommands[i]);
	strcpy(spoors[i - 1], spoors[i]);
	GhostOn(owin);
	RefreshGList(nameg[i - 1], owin, null, 1);
	GhostOff(owin);
    }
    --packers;
    if (currentpacker >= packers)
	currentpacker = 0;
    if (pgagadd.Flags & GFLG_DISABLED) {
	i = RemoveGadget(owin, &pgagadd);
	pgagadd.Flags &= ~GFLG_DISABLED;
	FixGreenGad(&pgagadd, owin);
	AddGadget(owin, &pgagadd, (long) i);
	RefreshGList(&pgagadd, owin, null, 1);
    }
    packernames[packers][0] = packommands[packers][0]
				= unpackommands[packers][0] = 0;
    i = RemoveGadget(owin, nameg[packers]);
    nameg[packers]->Flags |= GFLG_DISABLED;
    nameg[packers]->Flags &= ~GFLG_SELECTED;
    FixGreenGad(nameg[packers], owin);
    AddGadget(owin, nameg[packers], (long) i);
    GhostOn(owin);
    RefreshGList(nameg[packers], owin, null, 1);
    GhostOff(owin);
}


void Neutralize(void)
{
    pstrcname.Buffer = pnameblanks;
    pstrpack.Buffer = pstrunpack.Buffer = pstrspoor.Buffer = pblanks;
    pstrcname.MaxChars = 9;
    pstrpack.MaxChars = pstrunpack.MaxChars = pstrspoor.MaxChars = 80;
    pstrcname.BufferPos = pstrpack.BufferPos
		= pstrunpack.BufferPos = pstrspoor.BufferPos = 0;
    pgagspoor.Flags |= GFLG_DISABLED;
    pgagcname.Flags |= GFLG_DISABLED;
    pgagpack.Flags |= GFLG_DISABLED;
    pgagunpack.Flags |= GFLG_DISABLED;
}


void UnSelecticate(short k)
{
    struct Gadget *gg;
    long n;
    short i;
    bool flat = k < 0;

    if (flat) k += 100;
    gg = nameg[k];
    GhostOn(owin);
    FixGreenGad(gg, owin);
    RefreshGList(gg, owin, null, 1);
    n = RemoveGList(owin, &pgagspoor, 4);
    Neutralize();
    AddGList(owin, &pgagspoor, n, 4, null);
    RefreshGList(&pgagspoor, owin, null, 4);
    GhostOff(owin);
    if (!flat)
	for (i = 0; i < packers; i++)
	    if (!packernames[i][0] && !packommands[i][0]
					&& !unpackommands[i][0])
		NukePacker(i--);
}


bool CheckForFuckups(void)
{
    short i;
    str p;
    void Selecticate(short n);

    for (i = 0; i < packers; i++)
	if (!packernames[i][0] && !packommands[i][0]
				&& !unpackommands[i][0] && !spoors[i][0])
	    NukePacker(i--);
    if (!packers) {
	Err("You have erased every single one of the\n"
			"compressor definitions.  You must have at\n"
			"least one valid definition in there.");
	return false;
    }
    for (i = 0; i < packers; i++)
	if (!packernames[i][0] || !packommands[i][0] || !unpackommands[i][0]) {
	    Err("Compressor #%ld, named \"%s\", has an incomplete\n"
				"definition.  Either finish the name and both",
				"commands, or erase everything.",
				i + 1L, &packernames[i][0]);
	    return false;
	}
    for (i = 0; i < packers; i++)
	for (p = &spoors[i][0]; *p; p++)
	    if (*p != ' ' && *p != '?' && (!isxdigit(*p) || !isxdigit(*++p))) {
		Err("Compressor #%ld, named \"%s\", has an invalid\n"
				"pattern.  Only spaces, question marks, and\n"
				"pairs of hexadecimal digits are valid there.",
				i + 1L, &packernames[i][0]);
		if (!(nameg[i]->Flags & GFLG_SELECTED)) {
		    nameg[i]->Flags |= GFLG_SELECTED;
		    noactivatedammit = true;
		    Selecticate(i);
		    noactivatedammit = false;
		}
		pstrspoor.BufferPos = p - &spoors[i][0];
		pstrspoor.DispPos = pstrspoor.BufferPos - 12;
		if ((signed) pstrspoor.DispPos < 0)
		    pstrspoor.DispPos = 0;
		ActivateGag(&pgagspoor, owin);
		return false;
	    }
    FullPresentPackers();
    return true;
}


void Selecticate(short k)
{
    short i;
    struct Gadget *gg = nameg[k];
    static char buttwipe[10] = "         ";

    GhostOn(owin);			/* unnecessary? */
    for (i = 0; i < GREENBAY; i++) {
	if (nameg[i] != gg && nameg[i]->Flags & GFLG_SELECTED) {
	    nameg[i]->Flags &= ~GFLG_SELECTED;
	    FixGreenGad(nameg[i], owin);
	    RefreshGList(nameg[i], owin, null, 1);
	}
    }
    i = RemoveGList(owin, &pgagspoor, 4);
    pstrpack.Buffer = packommands[k];
    pstrunpack.Buffer = unpackommands[k];
    pstrspoor.Buffer = spoors[k];
    pstrpack.MaxChars = pstrunpack.MaxChars = COMMANDLEN;
    pstrspoor.MaxChars = SPOORLEN;
    pstrpack.BufferPos = strlen(pstrpack.Buffer);
    pstrunpack.BufferPos = strlen(pstrunpack.Buffer);
    pstrspoor.BufferPos = strlen(pstrspoor.Buffer);
    pstrcname.DispPos = pstrpack.DispPos
			= pstrunpack.DispPos = pstrspoor.DispPos = 0;
    pgagcname.Flags &= ~GFLG_DISABLED;
    pgagpack.Flags &= ~GFLG_DISABLED;
    pgagunpack.Flags &= ~GFLG_DISABLED;
    pgagspoor.Flags &= ~GFLG_DISABLED;
    AddGList(owin, &pgagspoor, (long) i, 4, null);
    pstrcname.Buffer = packernames[k];
    pstrcname.BufferPos = strlen(pstrcname.Buffer);
    pstrcname.MaxChars = 8;
    RefreshGList(&pgagspoor, owin, null, 4);
    GhostOff(owin);
    if (!noactivatedammit)
	ActivateGadget(&pgagcname, owin, null);
}


bool DoCompressorsIDCMP(struct IntuiMessage *im)
{
    ushort shifty = im->Qualifier & SHIFTKEYS;
    short k, j;
    long i;

    if (im->Class == IDCMP_CLOSEWINDOW && im->IDCMPWindow == owin)
	return CheckForFuckups();
    if (im->Class == IDCMP_RAWKEY && !(im->Qualifier & ALTKEYS)) {
	k = KeyToAscii(im->Code,
			(short) (im->Qualifier & ~IEQUALIFIER_NUMERICPAD));
	if (k == ESC)
	    return CheckForFuckups();
	if (k == '\t' && !(pgagcname.Flags & GFLG_DISABLED))
	    ActivateGag(&pgagcname, owin);
	else if (k >= '1' && k <= '8') {
	    k -= '1';
	    if (k >= packers)
		return false;
	    nameg[k]->Flags ^= GFLG_SELECTED;
	    RefreshGList(nameg[k], owin, null, 1);
	} else if (k == 'A' && packers < GREENBAY)
	    k = 13;
	else
	    return false;
    } else if (im->Class == IDCMP_GADGETUP)
	k = ((struct Gadget *) im->IAddress)->GadgetID - 600;
    else
	return false;
    switch (k) {
      case 0: case 1: case 2: case 3:			/* compressor type */
      case 4: case 5: case 6: case 7:
	if (!(nameg[k]->Flags & GFLG_SELECTED))
	    UnSelecticate(k);
	else
	    Selecticate(k);
	break;
      case 13:						/* Add */
	for (j = 0; j < packers; j++)
	    if (!packernames[j][0] && !packommands[j][0]
					&& !unpackommands[j][0])
		NukePacker(j--);
	if (packers >= 8) return false;	/* paranoia */
	if (packers == 7) {
	    i = RemoveGadget(owin, &pgagadd);
	    pgagadd.Flags |= GFLG_DISABLED;
	    AddGadget(owin, &pgagadd, i);
	    GhostOn(owin);
	    RefreshGList(&pgagadd, owin, null, 1);
	    GhostOff(owin);
	}
	i = RemoveGadget(owin, nameg[packers]);
	nameg[packers]->Flags &= ~GFLG_DISABLED;
	nameg[packers]->Flags |= GFLG_SELECTED;
	FixGreenGad(nameg[packers], owin);
	AddGadget(owin, nameg[packers], i);
	RefreshGList(nameg[packers], owin, null, 1);
	packernames[packers][0] = packommands[packers][0]
		    = unpackommands[packers][0] = spoors[packers][0] = 0;
	Selecticate(packers++);
	break;
      case 10:						/* Name */
	for (k = 0; k < packers; k++)
	    if (nameg[k]->Flags & GFLG_SELECTED)
		break;
	if (k < packers) {
	    FixGreenGad(nameg[k], owin);
	    GhostOn(owin);
	    RefreshGList(nameg[k], owin, null, 1);
	    GhostOn(owin);
	}
	if (GAGFINISH(im) && GAGCHAIN(im))
	    ActivateGadget(shifty ? &pgagspoor : &pgagunpack, owin, null);
	break;
      case 11:						/* unpack command */
	if (GAGFINISH(im) && GAGCHAIN(im))
	    ActivateGadget(shifty ? &pgagcname : &pgagpack, owin, null);
	break;
      case 12:						/* pack command */
	if (GAGFINISH(im) && GAGCHAIN(im))
	    ActivateGadget(shifty ? &pgagunpack : &pgagspoor, owin, null);
	break;
      case 15:						/* pattern */
	if (GAGFINISH(im) && shifty && GAGCHAIN(im))
	    ActivateGadget(&pgagpack, owin, null);
	break;
    }
    return false;
}


void ConfigCompressors(void)
{
    struct Gadget *gg;
    long gact = 0;
    char buf[8];
    short i, j;

    StopScroll();
    pneww.Screen = scr;
    for (i = 0, gg = &pgagc1; i < GREENBAY; i++, gg = gg->NextGadget) {
	AbleGad(gg, i < packers);
	gg->Flags &= ~GFLG_SELECTED;
	nameg[i] = gg;
    }
    punderborder.LeftEdge = XActual(-22) - under[0];
    punderborder.FrontPen = undercolor;
    AbleGad(&pgagadd, packers < GREENBAY);
    Neutralize();

    pgagc1.TopEdge = pgagc2.TopEdge = pgagc3.TopEdge
			= pgagc4.TopEdge = fakefight + 9;
    pgagc5.TopEdge = pgagc6.TopEdge = pgagc7.TopEdge
			= pgagc8.TopEdge = pgagc1.TopEdge + fakefight + 11;
    pgagcname.TopEdge = pgagspoor.TopEdge = pgagc5.TopEdge + fakefight + 14;
    pgagadd.TopEdge = pgagcname.TopEdge - 2;
    pgagunpack.TopEdge = pgagcname.TopEdge + 2 * fakefight + 13;
    pgagpack.TopEdge = pgagunpack.TopEdge + 2 * fakefight + 13;
    pneww.Height = pgagpack.TopEdge + fakefight + 11;
    /* height for fakefight = 11: 159 */
    pgagunpack.GadgetText->TopEdge = pgagpack.GadgetText->TopEdge = -4 - fight;
    if (!(owin = OpenBlueGagWin(&pww, &pgagc1))) {
	WindowErr("configuring compressors.");
	return;
    }

    orp = owin->RPort;
    SetAPen(orp, LABELCOLOR);
    SetBPen(orp, backcolor);
    j = XActual(22);
    for (i = 0, gg = &pgagc1; i < GREENBAY; i++, gg = gg->NextGadget) {
	Move(orp, gg->LeftEdge - j, gg->TopEdge + 3 + font->tf_Baseline);
	sprintf(buf, "%ld:", i + 1L);
	Text(orp, buf, 2);
    }
    GhostCompose(true);
    EventLoop(DoCompressorsIDCMP);
    CloseBlueGagWin(owin);
    owin = null;
    GhostCompose(false);
}


void Seyn(struct Gadget *gyn, bool yes)		/* checkmark or radio button */
{
    ushort p = RemoveGadget(ynwin, gyn);
    if (yes)
	gyn->Flags |= GFLG_SELECTED;
    else
	gyn->Flags &= ~GFLG_SELECTED;
    GhostOn(ynwin);
    AddGadget(ynwin, gyn, p);
    RefreshGList(gyn, ynwin, null, 1);
    GhostOff(ynwin);
}


local bool FixWrapMargin(void)
{
    if (margin[0] && (!ystrmargin.LongInt || (ystrmargin.LongInt >= 30
					&& ystrmargin.LongInt <= 80))) {
	wrapmargin = ystrmargin.LongInt;
	return true;
    }
    ystrmargin.LongInt = wrapmargin;
    utoa((ushort) ystrmargin.LongInt, margin);
    RefreshGList(&ygagmargin, owin, null, 1);
    ActivateGag(&ygagmargin, owin);
    return false;
}


local void ToggleQuoteDefault(short increment)
{
    quotedefault = (quotedefault + increment + 5) % 5;
    ChangeGagText(&ygagquote, owin, cgtquotes[quotedefault]);
    RefreshGList(&ygagquote, owin, null, 1);
}


void ToggleTagStyle(short increment, struct Window *where)
{
    ushort howmany, *what;
    struct Gadget *who;

    if (where == owin)
	what = &tagstyle, who = &ygagtagstyle, howmany = 4;
    else
	what = &localtagstyle, who = &llgagtagstyle, howmany = 5;
    *what = (*what + increment + howmany) % howmany;
    ChangeGagText(who, where, ygttagstyles[*what]);
    RefreshGList(who, where, null, 1);
}


void ToggleTagLeadin(short increment, struct Window *where)
{
    ushort howmany, *what, t;
    struct Gadget *who;

    if (where == owin)
	what = &tagleadin, who = &ygagtagleadin, howmany = 2;
    else
	what = &localtagleadin, who = &llgagtagleadin, howmany = 4;
    t = (*what + increment + howmany) % howmany;
    if (!qwk && where != owin && t && t < 3)
	t = increment < 0 ? 0 : 3;
    ChangeGagText(who, where, ygttagleadins[*what = t]);
    GhostOn(where);
    RefreshGList(who, where, null, 1);
    GhostOff(where);
}


bool DoOptionIDCMP(struct IntuiMessage *im)
{
    if (im->Class == IDCMP_RAWKEY && !(im->Qualifier & ALTKEYS)) {
	char k = KeyToAscii(im->Code, im->Qualifier);
	switch (k) {
	  case 'W':
	    Seyn(&ogagwaste, waste = !waste);
	    if (!waste)
		MakeLotsaRoom();
	    break;
	  case 'B':
	    Seyn(&ogaggagrow, gagrow = !gagrow);
	    MakeBotGadgets();
	    break;
	  case 'U':
	    Seyn(&ogagbgup, bgupdate = !bgupdate);
	    break;
	  case 'M':
	    Seyn(&ogaglist, showlistwin = !showlistwin);
	    break;
	  case 'Y':
	    Seyn(&ogagbeepu, beepu = !beepu);
	    break;
	  case 'P':
	    Seyn(&ogagffeed, formfeed = !formfeed);
	    break;
	  case 'R':
	    Seyn(&ogagaddRe, addRe = !addRe);
	    break;
	  case 'A':
	    Seyn(&ogagarea, showareawin = !showareawin);
	    break;
	  case 'N':
	    Seyn(&ogagstrchain, strchain = !strchain);
	    break;
	  case 'O':
	    Seyn(&ogagaskdelete, askdelete = !askdelete);
	    break;
	  case 'S':
	    Seyn(&ogagshowsizes, showsizes = !showsizes);
	    break;
	  case 'E':
	    Seyn(&ogagflushreplies, flushreplies = !flushreplies);
	    break;
	  case 'H':
	    Seyn(&ogagpopupscroller, popupscroller = !popupscroller);
	    break;
	  case ESC:
	    return true;
	}
    } else if (im->Class == IDCMP_GADGETUP) {
	switch (((struct Gadget *) im->IAddress)->GadgetID) {
	  case 303:					/* buttons? */
	    CHECK(ogaggagrow, gagrow);
	    MakeBotGadgets();
	    break;
	  case 304:					/* waste? */
	    CHECK(ogagwaste, waste);
	    if (!waste)
		MakeLotsaRoom();
	    break;
	  case 305:					/* bgupdate? */
	    CHECK(ogagbgup, bgupdate);
	    break;
	  case 306:					/* list? */
	    CHECK(ogaglist, showlistwin);
	    break;
	  case 307:					/* beepu? */
	    CHECK(ogagbeepu, beepu);
	    break;
	  case 310:					/* pagebreak? */
	    CHECK(ogagffeed, formfeed);
	    break;
	  case 312:					/* area win? */
	    CHECK(ogagarea, showareawin);
	    break;
	  case 313:					/* add "Re:"? */
	    CHECK(ogagaddRe, addRe);
	    break;
	  case 314:					/* next gad activates */
	    CHECK(ogagstrchain, strchain);
	    break;
	  case 319:					/* delete pkt option */
	    CHECK(ogagaskdelete, askdelete);
	    break;
	  case 320:					/* show sizes */
	    CHECK(ogagshowsizes, showsizes);
	    break;
	  case 321:					/* flush reply dir */
	    CHECK(ogagflushreplies, flushreplies);
	    break;
	  case 322:					/* popup scroller */
	    CHECK(ogagpopupscroller, popupscroller);
	    break;
	}
	return false;
    } else if (im->Class == IDCMP_CLOSEWINDOW)
	return im->IDCMPWindow == owin;
    return false;
}


/* (y) is for top edge of text, as with IntuiText, not baseline; x is actual. */
/* Hack: if the first N characters are ">", it underlines the Nth word.       */

void UnderstartedText(long x, long y, struct RastPort *rp, str label)
{
    ushort second = 0;
    while (*label == '>')
	second++, label++;
    SetAPen(rp, LABELCOLOR);
    SetBPen(rp, backcolor);
    Move(rp, x, y + font->tf_Baseline);
    Text(rp, label, strlen(label));
    SetAPen(rp, undercolor);
    y += fight + 1;
    while (second--) {
	while (*label && !isspace(*label))
	    x += fontwid, label++;
	while (isspace(*label))
	    x += fontwid, label++;
    }
    while (ispunct(*label++))
	x += fontwid;
    Move(rp, --x, y);
    Draw(rp, x + fontwid, y);
}


void SeynLabel(struct Gadget *gyn, bool yes, str label)
{
    short right, top = gyn->TopEdge + 6 - fight / 2;
    if (gyn->Flags & GFLG_GADGIMAGE)
	right = 26;			/* radio button */
    else
	right = 34, top += lace;	/* checkmark */
    UnderstartedText(gyn->LeftEdge + right, top, ynwin->RPort, label);
    Seyn(gyn, yes);
}

/* protrusion of text above/below gadget for various font heights (lace):   */
/* 0-13: 0/0  14: 0/1  15: 1/1  16: 1/2  17: 2/2, etc.  Underscore 2 lower. */


void ConfigOptions(void)
{
    ogagffeed.TopEdge = ogagwaste.TopEdge = fakefight + 9 + checkoff;
    ogaglist.TopEdge = ogagbeepu.TopEdge = ogagffeed.TopEdge + checkspace;
    ogagarea.TopEdge = ogagbgup.TopEdge = ogaglist.TopEdge + checkspace;
    ogaggagrow.TopEdge = ogagaddRe.TopEdge = ogagarea.TopEdge + checkspace;
    ogagstrchain.TopEdge = ogagaskdelete.TopEdge
				= ogaggagrow.TopEdge + checkspace;
    ogagshowsizes.TopEdge = ogagflushreplies.TopEdge
				= ogagstrchain.TopEdge + checkspace;
    ogagpopupscroller.TopEdge = ogagshowsizes.TopEdge + checkspace;
    oneww.Height = ogagpopupscroller.TopEdge + 22 + checkoff;
    /* Height for fakefight = 11: 144 */
    oneww.Width = (fontwid <= 6 ? OWINWIDTH + 3 : OWINWIDTH);
    oneww.LeftEdge = 320 - oneww.Width / 2;
    oneww.TopEdge = (scr->Height - oneww.Height) / 2;
    if (!(owin = OpenBlueGagWin(&oww, &ogaggagrow))) {
	WindowErr("choosing preference options.");
	return;
    }

    ynwin = owin;
    SeynLabel(&ogagwaste, waste, "Waste memory for speed");
    SeynLabel(&ogaggagrow, gagrow, "Buttons at screen bottom");
    SeynLabel(&ogagbgup, bgupdate, "Update behind List window");
    SeynLabel(&ogaglist, showlistwin, "Msgs: list before reading");
    SeynLabel(&ogagbeepu, beepu, "Your msgs flash screen");
    SeynLabel(&ogagffeed, formfeed, "Page break after printing");
    SeynLabel(&ogagarea, showareawin, "Areas: list before reading");
    SeynLabel(&ogagaddRe, addRe, "\"Re:\" before reply subject");
    SeynLabel(&ogagstrchain, strchain, "Next string gad activates");
    SeynLabel(&ogagaskdelete, askdelete, "Option to delete packet");
    SeynLabel(&ogagshowsizes, showsizes, "Sizes in message list");
    SeynLabel(&ogagflushreplies, flushreplies, "Empty reply dir at close");
    SeynLabel(&ogagpopupscroller, popupscroller,
	      "Hidden scroll bar pops up when mouse at right edge of screen");
    GhostCompose(true);
    EventLoop(&DoOptionIDCMP);
    CloseBlueGagWin(owin);
    owin = null;
    GhostCompose(false);
}


void CheckSignatureCodeWarnings(void)
{
    str s;
    for (s = signature; *s; s++) {
	if (*s == '@') {
	    register char c = toupper(s[1]);
	    if (!c)
		return;
	    else if (c == 'V' || c == 'D' || c == 'T' || c == 'N'
					|| c == '@' || isdigit(c))
		s++;
	    else {
/*		if (AskFixSuspiciousSignature()) {
		    
		} ***********************/
		return;
	    }
	}
    }
}


bool DoReplyingIDCMP(struct IntuiMessage *im)
{
    char k;
    ushort shifty = im->Qualifier & SHIFTKEYS;
    short cycleinc = shifty ? -1 : 1;
    bool finn = GAGFINISH(im) && GAGCHAIN(im);

    if (im->Class == IDCMP_RAWKEY && !(im->Qualifier & ALTKEYS)) {
	switch (k = KeyToAscii(im->Code, im->Qualifier)) {
	  case '\t':
	    ActivateGag(&ygagmargin, owin);
	    break;
	  case 'Q':
	    ToggleQuoteDefault(cycleinc);
	    break;
	  case 'D':
	    ToggleTagStyle(cycleinc, owin);
	    break;
	  case 'T':
	    ToggleTagLeadin(cycleinc, owin);
	    break;
	  case ESC:
	    return FixWrapMargin();
	}
    } else if (im->Class == IDCMP_GADGETUP) {
	k = 0;
	switch (((struct Gadget *) im->IAddress)->GadgetID) {
	  case 300:					/* tag style */
	    ToggleTagStyle(cycleinc, owin);
	    break;
	  case 301:					/* tag leadin */
	    ToggleTagLeadin(cycleinc, owin);
	    break;
	  case 302:					/* any name */
	    if (finn && shifty)
		ActivateGadget(&ygagtagfile, owin, null);
	    break;
	  case 308:					/* quote type */
	    ToggleQuoteDefault(cycleinc);
	    break;
	  case 309:					/* margin */
	    if (finn && FixWrapMargin())
		ActivateGadget(shifty ? &ygaganyname : &ygagqhead, owin, null);
	    break;
	  case 311:					/* quote header */
	    if (finn)
		ActivateGadget(shifty ? &ygagmargin : &ygagcchead, owin, null);
	    break;
	  case 315:					/* tagline file */
	    if (finn)
		ActivateGadget(shifty ? &ygagsignature : &ygaganyname,
							owin, null);
	    break;
	  case 316:					/* carbon header */
	    if (finn)
		ActivateGadget(shifty ? &ygagqhead : &ygagsignature,
							owin, null);
	    break;
	  case 317:					/* signature */
	    if (finn) {
		CheckSignatureCodeWarnings();
		ActivateGadget(shifty ? &ygagcchead : &ygagtagfile, owin, null);
	    }
	    break;
	}
    } else if (im->Class == IDCMP_CLOSEWINDOW)
	return im->IDCMPWindow == owin && FixWrapMargin();
    return false;
}


void ConfigReplying(void)
{
    long lale = XActual(17);

    ystrmargin.LongInt = wrapmargin;
    utoa(wrapmargin, margin);
    ygagquote.TopEdge = fakefight + 9;
    ygagmargin.TopEdge = ygagquote.TopEdge + 2;
    ygagqhead.TopEdge = ygagmargin.TopEdge + fakefight + 12;
    ygagcchead.TopEdge = ygagqhead.TopEdge + fakefight + 12;
    ygagsignature.TopEdge = ygagcchead.TopEdge + fakefight + 12;
    ygagtagstyle.TopEdge = ygagtagleadin.TopEdge
				 = ygagsignature.TopEdge + fakefight + 9;
    ygagtagstyle.LeftEdge = 183;
    ylabeltagfile.LeftEdge = -220;
    ygagtagfile.TopEdge = ygagtagstyle.TopEdge + fakefight + 14;
    ygaganyname.TopEdge = ygagtagfile.TopEdge + fakefight + 12;
    yneww.Height = ygaganyname.TopEdge + fakefight + 11;
    /* height for fakefight = 11: 181 */
    yneww.TopEdge = (scr->Height - yneww.Height) / 2;
    if (!(owin = OpenBlueGagWin(&yww, &ygagquote))) {
	WindowErr("configuring reply options.");
	return;
    }

    UnderstartedText(lale, ygagquote.TopEdge + 3, owin->RPort,
				"Quote style default:");
    UnderstartedText(lale, ygagtagstyle.TopEdge + 3, owin->RPort,
				"Default tagline:");
    UnderstartedText(ygagtagleadin.LeftEdge - XActual(118),
				ygagtagleadin.TopEdge + 3, owin->RPort,
				"Tagline after:");
    ToggleQuoteDefault(0);
    ToggleTagStyle(0, owin);
    ToggleTagLeadin(0, owin);
    GhostCompose(true);
    EventLoop(&DoReplyingIDCMP);
    CloseBlueGagWin(owin);
    owin = null;
    GhostCompose(false);
}
