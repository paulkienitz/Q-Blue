/* do the directories, editor, and local setup windows for Q-Blue */

#include <graphics/gfxmacros.h>
#include <intuition/sghooks.h>
#include <intuition/intuitionbase.h>
#include <dos/dosextens.h>
#include <dos/stdio.h>
#include "qblue.h"
#include "pigment.h"
#include "semaphore.h"


#define WORKBITS     (SOF_MAYBEWORK | SOF_INUSEWORK)
#define REPLYBITS    (SOF_MAYBEREPLY | SOF_EMPTYREPLY | SOF_INUSEREPLY)

#define DEWINWIDTH   536
#define DEWINHEIGHT  190
#define LLWINWIDTH   546
#define LLWINHEIGHT  170
#define DELONGGAG    496
#define DEFLEFT      282


void JoinName(str result, str dir, str file, str tail);
bool AskAboutUsedDirs(bool replies, bool looksokay, bool ask);
long CheckSemaphoreOverlap(bool dodirs);
void MakeInstanceNames(ushort instouse);
bool IdenticalPaths(str a, str b);
void ToggleTagStyle(short increment, struct Window *where);
void ToggleTagLeadin(short increment, struct Window *where);
bool DoFileRequest(bool saving, bool special, str result,
				str deflt, str pattern, str hail);
bool SafeExamineFH(BPTR hand, struct FileInfoBlock *fib, str pathname);
short ListAreas(short initialarea, bool empties, bool addrop, bool minusone);
short AddUnknownArea(ushort initialarea);
void DetectStealthiness(ushort lead, bool quiet);
str FixWRpath(str what);
void FormatNetAddress(str buf, ushort zone,
			ushort net, ushort node, ushort point);
bool ParseNetAddress(str a);
bool IsInFrontOf(struct Window *wa, struct Window *wb);
void FixStringGad(struct Gadget *gg, struct Window *ww);
void ActMenu(short menu, short item, bool enable);


import struct Border yqheadbox, yfsizebox, yanynamebox, upborderV;
import struct IntuiText ylabelanyname, ylabelmargin, ylabelqhead;
import struct IntuiText ylabelsignature, ylabeltagfile;
import struct Window *owin;
import struct QSemNode ourqsemnode;
import struct TextAttr ta;
import struct IntuitionBase *IntuitionBase;

import ushort ghostpat[2];
import char doorconn[], thefilenote[], packernames[GREENBAY][PACKNAMELEN];
import char printerpath[], tempacketname[], edit2command[], edit1command[];
import str fibupt;

import long modeID;
import ushort taheight, sorder, packers, czone, cnet, cnode, cpoint;
import ushort hostnetmailflags, orig_subjectlen;
import short netmailareanum, iemailareanum, ourinstance, confounded;

import bool backbefore, frontafter, dubedit, zerosizefile, replylike;
import bool laterlace, laterfourcolors, lookslikeqwk, official_iemail;
import bool customode, latercustomode, newareawin, inverse_netkluge;
import bool searchlight, searchlight_ie, pcboard_ie, pcboard_hood, editstrip;


char undospace[SIGNATURELEN + 1], iedummyto[NAMELEN + 2], sparedummy[NAMELEN];
char okayworkdir[PATHLEN + 2], okayreplydir[PATHLEN + 2];
char taglinesfile[COMMANDLEN + 2], localtaglinesfile[COMMANDLEN + 2];
char localanyname[NAMELEN + 2], localquoteheader[COMMANDLEN + 2];
char qnetkluge[NAMELEN + 2], localfigname[COMMANDLEN + 2];
char localsignature[SIGNATURELEN + 2], fidogate[25];
char savename[COMMANDLEN], defsavename[COMMANDLEN + 2];

local /* const */ char LOCALLABEL[] = "Q-Blue BBS local setup file";
local char iemailconfnum[8], qnetconfnum[8];
local bool semnotwarned, was_pcbkluge;

ushort localtagstyle = 4, localtagleadin = 2, localpacker = 0;    /* "global" */
ushort gatezone, gatenet, gatenode, gatepoint, tempgatenet, tempnetflags;
ushort local_sorder = 0, ieklugestyle = 0;
short localwrapmargin = -1;	/* negative means global */
bool allowblanksubj = false, local_firstfirst = false, indent_XX = true;
bool force_pcbkluge, pcboard_net, ie_is_gated;
#ifdef SHORTS_CHECK
bool iemail_shorts = false;
#endif


struct Window *ynwin;


#ifdef ASL_FREQ_FOLDER_GAG

/* this here is a border that creates a little folder button of the sort  */
/* used for summoning the ASL requester to put a directory name into a    */
/* string gadget, in a gadtools-like way... we'll also have the Help key, */
/* when pressed inside the active string gadget, bring up the requester:  */

short folderfolder = { 4, 10, 4, 4, 8, 4, 10, 2, 13, 2, 15, 4,
			15, 10, 5, 10, 5, 5, 9, 5, 10, 6, 14, 6 };
short foldershadow = { 19, 13, 19, 0, 18, 1, 18, 13, 0, 13 };
short foldershine = { 0, 0, 0, 13, 1, 12, 1, 0, 18, 0};

/* normally this would be SHADOWCOLOR, but I'm making it white: */
struct Border folderbor3 = {
    0, 0, CHECKCOLOR, 0, JAM2, 12, folderfolder, null };
};

struct Border folderbor2 = {
    0, 0, SHADOWCOLOR, 0, JAM2, 5, foldershadow, &folderbor3
};

struct Border folderborder = {
    0, 0, SHINECOLOR, 0, JAM2, 5, foldershine, &folderbor2
};

/* The gadget is width 20, height 14, and should be placed with the top edge */
/* equal to the string gadget's plus (fight + 1) / 2 - 7, measured from the  */
/* top of the string space, not the top border... left edge such that there  */
/* is a gap of just two pixels between it and the string's right border.     */

#endif


struct Gadget egagstripbad = {
    null, 20, 168, CHECKWID, 13, CHECKHILITE, CHECKACTION,
    GTYP_BOOLGADGET, &checkmarkno, &checkmarkyes, null, 0, null, 406, null
};

struct Gadget egagsfront = {
    &egagstripbad, 312, 168, CHECKWID, 13, CHECKHILITE, CHECKACTION,
    GTYP_BOOLGADGET, &checkmarkno, &checkmarkyes, null, 0, null, 405, null
};

struct Gadget egagsback = {
    &egagsfront, 20, 168, CHECKWID, 13, CHECKHILITE, CHECKACTION,
    GTYP_BOOLGADGET, &checkmarkno, &checkmarkyes, null, 0, null, 404, null
};


struct StringInfo estrcomm2 = STRINF(edit2command, COMMANDLEN);

struct IntuiText elabelcomm2 = {
    LABELCOLOR, 0, JAM2, 0, -20, null,
    "Optional command for editing a reply using BOTH files:", null
};

STRINGBORDER(elongbox)

struct Gadget egagcomm2 = {
    &egagsback, 20, 147, DELONGGAG, 8, GFLG_STRINGEXTEND | GFLG_TABCYCLE,
    GACT_RELVERIFY | GACT_STRINGLEFT, GTYP_STRGADGET,
    &elongbox, null, &elabelcomm2, 0, &estrcomm2, 403, null
};


struct StringInfo estrcomm1 = STRINF(edit1command, COMMANDLEN);

struct IntuiText elabelcomm1 = {
    LABELCOLOR, 0, JAM2, 0, -20, null,
    "Command for editing message text in the temporary file:", null
};

struct Gadget egagcomm1 = {
    &egagcomm2, 20, 75, DELONGGAG, 8, GFLG_STRINGEXTEND | GFLG_TABCYCLE,
    GACT_RELVERIFY | GACT_STRINGLEFT, GTYP_STRGADGET,
    &elongbox, null, &elabelcomm1, 0, &estrcomm1, 401, null
};


struct StringInfo estrfile2 = STRINF(editinfile, PATHLEN);

struct IntuiText elabelfile2 = {
    LABELCOLOR, 0, JAM2, -DEFLEFT, 0, null,
    "Optional file for quoted message:", null
};

struct Gadget egagfile2 = {
    &egagcomm1, 20 + DEFLEFT, 111, DELONGGAG - DEFLEFT, 8,
    GFLG_STRINGEXTEND | GFLG_TABCYCLE,
    GACT_RELVERIFY | GACT_STRINGLEFT, GTYP_STRGADGET,
    &elongbox, null, &elabelfile2, 0, &estrfile2, 402, null
};


struct StringInfo estrfile1 = STRINF(editoutfile, PATHLEN);

struct IntuiText elabelfile1 = {
    LABELCOLOR, 0, JAM2, -DEFLEFT, 0, null,
    "Temporary file for message text:", null
};

struct Gadget egagfile1 = {
    &egagfile2, 20 + DEFLEFT, 39, DELONGGAG - DEFLEFT, 8,
    GFLG_STRINGEXTEND | GFLG_TABCYCLE,
    GACT_RELVERIFY | GACT_STRINGLEFT, GTYP_STRGADGET,
    &elongbox, null, &elabelfile1, 0, &estrfile1, 400, null
};


struct ExtNewWindow eneww = {
    52, 6, DEWINWIDTH, DEWINHEIGHT, 0, 1,
    IDCMP_GADGETUP | IDCMP_CLOSEWINDOW | IDCMP_RAWKEY,
    WFLG_SMART_REFRESH | WFLG_CLOSEGADGET | WFLG_DEPTHGADGET
		| WFLG_DRAGBAR | WFLG_ACTIVATE | WFLG_NW_EXTENDED,
    null, null, "Configuration of text editor for writing messages",
    null, null, 0, 0, 0, 0, CUSTOMSCREEN, null
};

struct WhereWin eww = { &eneww };


/* ---------------------------------------------------------------------- */


struct StringInfo dstrsavename = STRINF(defsavename, COMMANDLEN);

struct IntuiText dlabelsavename = {
    LABELCOLOR, 0, JAM2, -248, 0, null, "Path for saving messages:", null
};

STRINGBORDER(dpathbox)

struct Gadget dgagsavename = {
    null, 264, 169, 248, 8, GFLG_STRINGEXTEND | GFLG_TABCYCLE,
    GACT_RELVERIFY | GACT_STRINGLEFT, GTYP_STRGADGET,
    &dpathbox, null, &dlabelsavename, 0, &dstrsavename, 506, null
};


struct StringInfo dstrprinter = STRINF(printerpath, PATHLEN);

struct IntuiText dlabelprinter = {
    LABELCOLOR, 0, JAM2, -248, 0, null, "Printer output filename:", null
};

struct Gadget dgagprinter = {
    &dgagsavename, 264, 169, 248, 8, GFLG_STRINGEXTEND | GFLG_TABCYCLE,
    GACT_RELVERIFY | GACT_STRINGLEFT, GTYP_STRGADGET,
    &dpathbox, null, &dlabelprinter, 0, &dstrprinter, 505, null
};


struct StringInfo dstrreply = STRINF(replydir, PATHLEN);

struct IntuiText dlabelreply = {
    LABELCOLOR, 0, JAM2, -248, 0, null, "Reply creation directory:", null
};

struct Gadget dgagreply = {
    &dgagprinter, 264, 147, 248, 8, GFLG_STRINGEXTEND | GFLG_TABCYCLE,
    GACT_RELVERIFY | GACT_STRINGLEFT, GTYP_STRGADGET,
    &dpathbox, null, &dlabelreply, 0, &dstrreply, 504, null
};


struct StringInfo dstrwork = STRINF(workdir, PATHLEN);

struct IntuiText dlabelwork = {
    LABELCOLOR, 0, JAM2, -248, 0, null, "Work dir -- mail being read:", null
};

struct Gadget dgagwork = {
    &dgagreply, 264, 111, 248, 8, GFLG_STRINGEXTEND | GFLG_TABCYCLE,
    GACT_RELVERIFY | GACT_STRINGLEFT, GTYP_STRGADGET,
    &dpathbox, null, &dlabelwork, 0, &dstrwork, 503, null
};


struct StringInfo dstrbbs = STRINF(bbsesdir, PATHLEN);

struct IntuiText dlabelbbs = {
    LABELCOLOR, 0, JAM2, -248, 0, null, "BBS context directory:", null
};

struct Gadget dgagbbs = {
    &dgagwork, 264, 75, 248, 8, GFLG_STRINGEXTEND | GFLG_TABCYCLE,
    GACT_RELVERIFY | GACT_STRINGLEFT, GTYP_STRGADGET,
    &dpathbox, null, &dlabelbbs, 0, &dstrbbs, 502, null
};


struct StringInfo dstrup = STRINF(uploaddir, PATHLEN);

struct IntuiText dlabelup = {
    LABELCOLOR, 0, JAM2, -248, 0, null, "Reply packets to be uploaded:", null
};

struct Gadget dgagup = {
    &dgagbbs, 264, 75, 248, 8, GFLG_STRINGEXTEND | GFLG_TABCYCLE,
    GACT_RELVERIFY | GACT_STRINGLEFT, GTYP_STRGADGET,
    &dpathbox, null, &dlabelup, 0, &dstrup, 501, null
};


struct StringInfo dstrdown = STRINF(downloaddir, PATHLEN);

struct IntuiText dlabeldown = {
    LABELCOLOR, 0, JAM2, -248, 0, null, "Downloaded mail packets:", null
};

struct Gadget dgagdown = {
    &dgagup, 264, 39, 248, 8, GFLG_STRINGEXTEND | GFLG_TABCYCLE,
    GACT_RELVERIFY | GACT_STRINGLEFT, GTYP_STRGADGET,
    &dpathbox, null, &dlabeldown, 0, &dstrdown, 500, null
};


struct ExtNewWindow dneww = {
    52, 6, DEWINWIDTH, DEWINHEIGHT, 0, 1,
    IDCMP_GADGETUP | IDCMP_CLOSEWINDOW | IDCMP_RAWKEY,
    WFLG_SMART_REFRESH | WFLG_CLOSEGADGET | WFLG_DEPTHGADGET
		| WFLG_DRAGBAR | WFLG_ACTIVATE | WFLG_NW_EXTENDED,
    null, null, "Pathnames of essential directories",
    null, null, 0, 0, 0, 0, CUSTOMSCREEN, null
};

struct WhereWin dww = { &dneww };


str dirname[6] = {
    "Downloads", "Uploads", "BBS Context", "Work", "Replies", "ASL selected"
};

local str dirbuf[5] = {
    downloaddir, uploaddir, bbsesdir, workdir, replydir
}, edfibuf[2] = {
    editoutfile, editinfile
};


/* ---------------------------------------------------------------------- */


struct Gadget llgagxxindent = {
    null, 296, 45, CHECKWID, 13, CHECKHILITE, CHECKACTION,
    GTYP_BOOLGADGET, &checkmarkno, &checkmarkyes, null, 0, null, 2320, null
};


struct Gadget llgagblanksubj = {
    &llgagxxindent, 296, 45, CHECKWID, 13, CHECKHILITE, CHECKACTION,
    GTYP_BOOLGADGET, &checkmarkno, &checkmarkyes, null, 0, null, 2316, null
};


struct IntuiText lllabelmailwin = {
    TEXTCOLOR, FILLCOLOR, JAM2, 12, 2, null, "Mail...", null
};

struct Gadget llgagmailwin = {
    &llgagblanksubj, 16, 21, 80, 13, GFLG_GADGHCOMP, GACT_RELVERIFY,
    GTYP_BOOLGADGET, &upborder, null, &lllabelmailwin, 0, null, 2315, null
};


struct IntuiText lllabelsave = {
    TEXTCOLOR, FILLCOLOR, JAM2, 12, 2, null, "Save", null
};

struct Gadget llgagsave = {
    &llgagmailwin, LLWINWIDTH - 96, 21, 80, 13, GFLG_GADGHCOMP, GACT_RELVERIFY,
    GTYP_BOOLGADGET, &upborder, null, &lllabelsave, 0, null, 2310, null
};


struct IntuiText lllabelload = {
    TEXTCOLOR, FILLCOLOR, JAM2, 12, 2, null, "Load", null
};

struct Gadget llgagload = {
    &llgagsave, LLWINWIDTH - 244, 21, 80, 13, GFLG_GADGHCOMP, GACT_RELVERIFY,
    GTYP_BOOLGADGET, &upborder, null, &lllabelload, 0, null, 2309, null
};


struct IntuiText llgtpackers[9] = {
    { TEXTCOLOR, FILLCOLOR, JAM2, 31, 3, null, "(global)", null },
    { TEXTCOLOR, FILLCOLOR, JAM2, 27, 3, null, " 0000000 ", null },
    { TEXTCOLOR, FILLCOLOR, JAM2, 27, 3, null, " 1111111 ", null },
    { TEXTCOLOR, FILLCOLOR, JAM2, 27, 3, null, " 2222222 ", null },
    { TEXTCOLOR, FILLCOLOR, JAM2, 27, 3, null, " 3333333 ", null },
    { TEXTCOLOR, FILLCOLOR, JAM2, 27, 3, null, " 4444444 ", null },
    { TEXTCOLOR, FILLCOLOR, JAM2, 27, 3, null, " 5555555 ", null },
    { TEXTCOLOR, FILLCOLOR, JAM2, 27, 3, null, " 6666666 ", null },
    { TEXTCOLOR, FILLCOLOR, JAM2, 27, 3, null, " 7777777 ", null }
};

/***
struct IntuiText llgtsort[7] = {
    { TEXTCOLOR, FILLCOLOR, JAM2, 31, 3, null, "(global)", null },
    { TEXTCOLOR, FILLCOLOR, JAM2, 31, 3, null, " Number ", null },
    { TEXTCOLOR, FILLCOLOR, JAM2, 31, 3, null, "  Age   ", null },
    { TEXTCOLOR, FILLCOLOR, JAM2, 27, 3, null, " Subject ", null },
    { TEXTCOLOR, FILLCOLOR, JAM2, 31, 3, null, " Thread ", null },
    { TEXTCOLOR, FILLCOLOR, JAM2, 31, 3, null, "  From  ", null },
    { TEXTCOLOR, FILLCOLOR, JAM2, 31, 3, null, "   To   ", null }
}; ***/

struct Gadget llgagpacker = {
    &llgagload, 110, 153, 106, 12, GFLG_GADGHCOMP, GACT_RELVERIFY,
    GTYP_BOOLGADGET, &upcyclebor, null, null, 0, null, 2311, null
};


local ubyte localmargin[14];

struct StringInfo llstrmargin = STRINF(localmargin, 3);

struct Gadget llgagmargin = {
    &llgagpacker, 180, 156, 24, 8, GFLG_STRINGEXTEND | GFLG_TABCYCLE,
    GACT_RELVERIFY | GACT_STRINGLEFT | GACT_LONGINT, GTYP_STRGADGET,
    &yfsizebox, null, &ylabelmargin, 0, &llstrmargin, 2308, null
};


struct StringInfo llstrsignature = STRINF(localsignature, SIGNATURELEN);

struct Gadget llgagsignature = {
    &llgagmargin, 132, 169, 392, 8, GFLG_STRINGEXTEND | GFLG_TABCYCLE,
    GACT_RELVERIFY | GACT_STRINGLEFT, GTYP_STRGADGET,
    &yqheadbox, null, &ylabelsignature, 0, &llstrsignature, 2312, null
};


struct StringInfo llstrqhead = STRINF(localquoteheader, COMMANDLEN);

struct Gadget llgagqhead = {
    &llgagsignature, 132, 169, 392, 8, GFLG_STRINGEXTEND | GFLG_TABCYCLE,
    GACT_RELVERIFY | GACT_STRINGLEFT, GTYP_STRGADGET,
    &yqheadbox, null, &ylabelqhead, 0, &llstrqhead, 2304, null
};


struct StringInfo llstranyname = STRINF(localanyname, NAMELEN);

struct Gadget llgaganyname = {
    &llgagqhead, 236, 169, 288, 8, GFLG_STRINGEXTEND | GFLG_TABCYCLE,
    GACT_RELVERIFY | GACT_STRINGLEFT, GTYP_STRGADGET,
    &yanynamebox, null, &ylabelanyname, 0, &llstranyname, 2302, null
};


struct Gadget llgagtagleadin = {
    &llgaganyname, 424, 173, 106, 12, GFLG_GADGHCOMP, GACT_RELVERIFY,
    GTYP_BOOLGADGET, &upcyclebor, null, null, 0, null, 2301, null
};


struct Gadget llgagtagstyle = {
    &llgagtagleadin, 150, 153, 106, 12, GFLG_GADGHCOMP, GACT_RELVERIFY,
    GTYP_BOOLGADGET, &upcyclebor, null, null, 0, null, 2300, null
};


struct StringInfo llstrtagfile = STRINF(localtaglinesfile, COMMANDLEN);

struct Gadget llgagtagfile = {
    &llgagtagstyle, 276, 169, 248, 8, GFLG_STRINGEXTEND | GFLG_TABCYCLE,
    GACT_RELVERIFY | GACT_STRINGLEFT, GTYP_STRGADGET,
    &dpathbox, null, &ylabeltagfile, 0, &llstrtagfile, 2303, null
};


struct ExtNewWindow llneww = {
    47, 7, LLWINWIDTH, LLWINHEIGHT, 0, 1,
    IDCMP_GADGETUP | IDCMP_CLOSEWINDOW | IDCMP_RAWKEY,
    WFLG_SMART_REFRESH | WFLG_CLOSEGADGET | WFLG_DEPTHGADGET
		| WFLG_DRAGBAR | WFLG_ACTIVATE | WFLG_NW_EXTENDED,
    null, null, "Options specific to this BBS",
    null, null, 0, 0, 0, 0, CUSTOMSCREEN, null
};

struct WhereWin llww = { &llneww };

struct Window *llwin;

				/* ------- */


#ifdef SHORTS_CHECK
struct Gadget l2gagieshortsub = {
    null, 16, 45, CHECKWID, 13, CHECKHILITE, CHECKACTION,
    GTYP_BOOLGADGET, &checkmarkno, &checkmarkyes, null, 0, null, 2321, null
};

struct Gadget l2gagpcbsubj = {
    &l2gagieshortsub, 320, 45, CHECKWID, 13, CHECKHILITE, CHECKACTION,
    GTYP_BOOLGADGET, &checkmarkno, &checkmarkyes, null, 0, null, 2319, null
};
#else
struct Gadget l2gagpcbsubj = {
    null, 320, 45, CHECKWID, 13, CHECKHILITE, CHECKACTION,
    GTYP_BOOLGADGET, &checkmarkno, &checkmarkyes, null, 0, null, 2319, null
};
#endif

struct IntuiText l2gtstylesearchlt = {
    TEXTCOLOR, FILLCOLOR, JAM2, 31, 3, null, "SearchLt", null
};

struct IntuiText l2gtstylepcboard = {
    TEXTCOLOR, FILLCOLOR, JAM2, 27, 3, null, " PCBoard ", null
};

struct IntuiText l2gtstylegeneric = {
    TEXTCOLOR, FILLCOLOR, JAM2, 27, 3, null, " Generic ", null
};

struct IntuiText *(l2gtstyles[3]) = {
    &l2gtstylegeneric, &l2gtstylepcboard, &l2gtstylesearchlt
};

struct Gadget l2gagiestyle = {
    &l2gagpcbsubj, 166, 153, 106, 12, GFLG_GADGHCOMP, GACT_RELVERIFY,
    GTYP_BOOLGADGET, &upcyclebor, null, null, 0, null, 2318, null
};


struct StringInfo l2strfidogate = STRINF(fidogate, 25);

struct IntuiText l2labelfidogate = {
    LABELCOLOR, 0, JAM2, -260, 0, null, "FidoNet address of gateway:    ", null
};

STRINGBORDER(l2fidogatebox)

struct Gadget l2gagfidogate = {
    &l2gagiestyle, 277, 156, 144, 8, GFLG_STRINGEXTEND | GFLG_TABCYCLE,
    GACT_RELVERIFY | GACT_STRINGLEFT, GTYP_STRGADGET,
    &l2fidogatebox, null, &l2labelfidogate, 0, &l2strfidogate, 2317, null
};


struct StringInfo l2striedummy = STRINF(iedummyto, 26);

struct IntuiText l2labeliedummy = {
    LABELCOLOR, 0, JAM2	, -260, 0, null, "Internet email dummy recipient:", null
};

STRINGBORDER(l2iedummybox)

struct Gadget l2gagiedummy = {
    &l2gagfidogate, 277, 156, 96, 8, GFLG_STRINGEXTEND | GFLG_TABCYCLE,
    GACT_RELVERIFY | GACT_STRINGLEFT, GTYP_STRGADGET,
    &l2iedummybox, null, &l2labeliedummy, 0, &l2striedummy, 2314, null
};


struct IntuiText l2labeliemailarea = {
    TEXTCOLOR, FILLCOLOR, JAM2, 12, 2, null, "arEa:", null
};

struct Gadget l2gagiemailarea = {
    &l2gagiedummy, LLWINWIDTH - 152, 21, 80, 13,
    GFLG_GADGHCOMP, GACT_RELVERIFY, GTYP_BOOLGADGET,
    &upborderV, null, &l2labeliemailarea, 0, null, 2313, null
};


struct StringInfo l2strqnetkluge = STRINF(qnetkluge, NAMELEN);

struct IntuiText l2labelqnetkluge = {
    LABELCOLOR, 0, JAM2, -196, 0, null, "QWK netmail kluge line:", null
};

STRINGBORDER(l2qnetklugebox)

struct Gadget l2gagqnetkluge = {
    &l2gagiemailarea, 213, 156, 160, 8, GFLG_STRINGEXTEND | GFLG_TABCYCLE,
    GACT_RELVERIFY | GACT_STRINGLEFT, GTYP_STRGADGET,
    &l2qnetklugebox, null, &l2labelqnetkluge, 0, &l2strqnetkluge, 2306, null
};


struct IntuiText l2labelqnetarea = {
    TEXTCOLOR, FILLCOLOR, JAM2, 12, 2, null, "Area:", null
};

struct Gadget l2gagqnetarea = {
    &l2gagqnetkluge, LLWINWIDTH - 152, 21, 80, 13,
    GFLG_GADGHCOMP, GACT_RELVERIFY, GTYP_BOOLGADGET,
    &upborder, null, &l2labelqnetarea, 0, null, 2305, null
};


struct ExtNewWindow l2neww = {
    43, 7, LLWINWIDTH + 8, LLWINHEIGHT, 0, 1,
    IDCMP_GADGETUP | IDCMP_CLOSEWINDOW | IDCMP_RAWKEY,
    WFLG_SMART_REFRESH | WFLG_CLOSEGADGET | WFLG_DEPTHGADGET
		| WFLG_DRAGBAR | WFLG_ACTIVATE | WFLG_NW_EXTENDED,
    null, null, "Email/Netmail on this BBS",
    null, null, 0, 0, 0, 0, CUSTOMSCREEN, null
};

struct WhereWin l2ww = { &l2neww };

struct Window *l2win;


/* ---------------------------------------------------------------------- */


void StripString(struct Gadget *gg, struct Window *ww)
{
    str gs = ((struct StringInfo *) gg->SpecialInfo)->Buffer, frgs;
    ushort l = strlen(gs);
    for (frgs = gs; *frgs && *frgs == ' '; frgs++) ;
    if (frgs > gs)
	strcpy(gs, frgs);
    Stranks(gs);
    if (ww && strlen(gs) != l)
	RefreshGList(gg, ww, null, 1);
}


local bool EdFileOkay(ushort f, ushort gid)
{
    str path = edfibuf[f];
    short i = strlen(path) - 1;
    bool badness;

    if (i >= 0 && (path[i] == '/' || path[i] == ':')) {
	Err("One of the filenames in the editor setup window\nhas a pathname "
				"ending with a \"%lc\" character.\nThat "
				"makes it a directory, not a file.", path[i]);
	return false;
    }
    badness = (gid == dgagwork.GadgetID || !gid)
				&& IdenticalPaths(path, dirbuf[i = DWORK]);
    if (!badness && (gid == dgagreply.GadgetID || !gid))
	badness = IdenticalPaths(path, dirbuf[i = DREP]);
    if (badness) {
	Err("You have selected the same pathname for\n"
			"your %s directory as for one of the\n"
			"filenames in your editor setup window.", dirname[i]);
	return false;
    }
    return true;
}


bool DoEditorIDCMP(struct IntuiMessage *im)
{
    ushort shifty = im->Qualifier & SHIFTKEYS;
    bool finn = GAGFINISH(im), sein = GAGCHAIN(im);
    struct Gadget *g;
    char k;

    ynwin = owin;
    if (im->Class == IDCMP_RAWKEY && !(im->Qualifier & ALTKEYS)) {
	k = KeyToAscii(im->Code, im->Qualifier);
	if (k == '\t') {
	    if (!editoutfile[0])
		g = &egagfile1;
	    else if (!edit1command[0])
		g = &egagcomm1;
	    else if (!editinfile[0] != !edit2command[0]) {
		if (!editinfile[0])
		    g = &egagfile2;
		else
		    g = &egagcomm2;
	    } else
		g = &egagfile1;
	    ActivateGag(g, owin);
	} else if (k == 'B')
	    Seyn(&egagsback, backbefore = !backbefore);
	else if (k == 'A')
	    Seyn(&egagsfront, frontafter = !frontafter);
	else if (k == 'R')
	    Seyn(&egagstripbad, editstrip = !editstrip);
    } else if (im->Class == IDCMP_GADGETUP) {
	k = 0;
	switch (((struct Gadget *) im->IAddress)->GadgetID) {
	  case 400:						/* file1 */
	    if (!finn) break;
	    StripString(&egagfile1, owin);
	    if (!EdFileOkay(0, 0))
		ActivateGadget(&egagfile1, owin, null);
	    else if (sein)
		ActivateGadget(shifty ? &egagcomm2 : &egagfile2, owin, null);
	    break;
	  case 401:						/* comm1 */
	    if (!finn) break;
	    StripString(&egagcomm1, owin);
	    if (sein)
		ActivateGadget(shifty ? &egagfile2 : &egagcomm2, owin, null);
	    break;
	  case 402:						/* file2 */
	    if (!finn) break;
	    StripString(&egagfile2, owin);
	    if (!EdFileOkay(1, 0))
		ActivateGadget(&egagfile2, owin, null);
	    else if (sein)
		ActivateGadget(shifty ? &egagfile1 : &egagcomm1, owin, null);
	    break;
	  case 403:						/* comm2 */
	    if (!finn) break;
	    StripString(&egagcomm2, owin);
	    if (sein && shifty)
		ActivateGadget(&egagcomm1, owin, null);
	    break;
	  case 404:						/* backbefore */
	    CHECK(egagsback, backbefore);
	    break;
	  case 405:						/* frontafter */
	    CHECK(egagsfront, frontafter);
	    break;
	  case 406:						/* frontafter */
	    CHECK(egagstripbad, editstrip);
	    break;
	}
	return false;
    }
    if ((im->Class != IDCMP_CLOSEWINDOW || im->IDCMPWindow != owin) && k != ESC)
	return false;
    StripString(&egagfile1, owin);
    StripString(&egagcomm1, owin);
    StripString(&egagfile2, owin);
    StripString(&egagcomm2, owin);
    if (!EdFileOkay(0, 0) || !EdFileOkay(1, 0))
	return false;
    MakeInstanceNames((ushort) ourinstance);
    return true;
}


void ConfigEditor(void)
{
    short gap = 2 * fakefight + 13 + lace;

    egagfile1.TopEdge = fakefight + 12;
    egagfile2.TopEdge = egagfile1.TopEdge + fakefight + 12;
    egagcomm1.TopEdge = egagfile2.TopEdge + gap;
    egagcomm2.TopEdge = egagcomm1.TopEdge + gap;
    egagsback.TopEdge = egagsfront.TopEdge
			= egagcomm2.TopEdge + fakefight + 9 + checkoff;
    egagstripbad.TopEdge = egagsfront.TopEdge + checkspace;
    eneww.Height = egagstripbad.TopEdge + 22 + checkoff;
    /* Height when fakefight = 11 (nolace): 171 */
    if (!(owin = OpenBlueGagWin(&eww, &egagfile1))) {
	WindowErr("configuring editor commands.");
	return;
    }
    ynwin = owin;
    ourqsemnode.editingeditor = true;
    SeynLabel(&egagsback, backbefore, "Before edit, screen to back");
    SeynLabel(&egagsfront, frontafter, "After edit, to front");
    SeynLabel(&egagstripbad, editstrip,
				"Remove upper ascii and control characters");
    GhostCompose(true);

    EventLoop(&DoEditorIDCMP);
#ifdef WAS_THAT_NECESSARY_ASKED_LEGOLAM
    if (CheckSemaphoreOverlap(false) & (SOF_EDITINFILE | SOF_EDITOUTFILE))
	Err("The temporary file name(s) you have selected are not usable...");
#endif
    dubedit = edit2command[0] && editinfile[0];
    ourqsemnode.editingeditor = false;
    CloseBlueGagWin(owin);
    owin = null;
    GhostCompose(false);
}


local void CheckAbsolution(bool ask, short gid)
{
    struct Gadget *gg;
    short i;
    str cole;

    for (i = 0, gg = &dgagdown; i < 5; i++, gg = gg->NextGadget)
	if ((ask || gg->GadgetID == gid) && dirbuf[i][0]) {
	    cole = strchr(dirbuf[i], ':');
	    if (!cole || cole == dirbuf[i]) {
		Err("Your %s directory is not specified as an absolute\n"
				"pathname.  For dependable behavior, each"
				" path should include\na disk name or assigned"
				" name, with a colon.", dirname[i]);
		break;
	    }
	}
}


local bool EnsureDiffer1(ushort d1, ushort d2)
{
    bool bad = IdenticalPaths(dirbuf[d1], dirbuf[d2]);

    if (bad) {
	Err("You have set your %s and %s directories\n"
			"both to the same place.  The Work and Replies\n"
			"directories must each be used for no other purpose.",
			dirname[d1], dirname[d2]);
    }
    return bad;
}


local bool EnsureDiffer(bool ask, short gid)    /* return false = trouble */
{
    APTR mwp = me->pr_WindowPtr;
    bool ret = true;
    long fs, overs = 0;
    BPTR ll;

    StripString(&dgagdown, owin);
    StripString(&dgagup, owin);
    StripString(&dgagwork, owin);
    StripString(&dgagreply, owin);
    me->pr_WindowPtr = (APTR) -1;
    if (EnsureDiffer1(0, 3) || EnsureDiffer1(1, 3) || EnsureDiffer1(2, 3)
				|| EnsureDiffer1(0, 4) || EnsureDiffer1(1, 4)
				|| EnsureDiffer1(2, 4) || EnsureDiffer1(3, 4))
	ret = false;			/* 0 == 1 == 2 is okay */
    else {
	MakeInstanceNames((ushort) ourinstance);
#ifdef WAS_THAT_NECESSARY_ASKED_LEGOLAM
	overs = CheckSemaphoreOverlap(true);
	if ((strcmp(workdir, okayworkdir) || strcmp(replydir, okayreplydir)
				|| ask & semnotwarned)
			&& overs & (WORKBITS | REPLYBITS)) {
	    strcpy(okayworkdir, workdir);
	    strcpy(okayreplydir, replydir);
	    semnotwarned = false;
	    if (ask || (gid == dgagwork.GadgetID && overs & WORKBITS)
			    || (gid == dgagreply.GadgetID && overs & REPLYBITS))
		Err("The work and/or replies directory names you have\n"
		    "selected are not usable.  In spite of each Q-Blue\n"
		    "process adding numeric suffixes to the actual directory\n"
		    "names it uses, two or more currently running Q-Blue\n"
		    "processes would end up using the same directory.");
	} else
#endif
	{
	    if (!EdFileOkay(0, gid) || !EdFileOkay(1, gid))
		ret = false;
	    fs = 0;
	    if (!readareaz.messct && (gid == dgagwork.GadgetID || ask)) {
		if (workdir[0] && strcmp(workdir, okayworkdir)) {
		    str trim = FixWRpath(workdir);
		    if (ll = RLock(workdir)) {
			fs = FileSize(ll);
			if (fs > 0 || zerosizefile) {
			    Err("%s is not\na directory, it's a file.", workdir);
			    ret = false;
			} else if (fs < 0)
			    ret = AskAboutUsedDirs(false, lookslikeqwk
						  || tempacketname[0], ask);
			UnLock(ll);
		    }
		    *trim = 0;
		}
		if (fs < 0 && ask && !ret)
		    okayworkdir[0] = 0;
		else
		    strcpy(okayworkdir, workdir);
	    }
	    if ((!readareaz.messct || !anythingtosave)
					&& (gid == dgagreply.GadgetID || ask)) {
		if ((!fs || !ret) && replydir[0]
					&& strcmp(replydir, okayreplydir)) {
		    str trim = FixWRpath(replydir);
		    if (ll = RLock(replydir)) {
			fs = FileSize(ll);
			if (fs > 0 || zerosizefile) {
			    Err("%s is not\na directory, it's a file.",
								replydir);
			    ret = false;
			} else if (fs < 0)
			    ret = AskAboutUsedDirs(true, replylike, ask);
			UnLock(ll);
		    }
		    *trim = 0;
		} else
		    fs = 0;
		if (fs < 0 && ask && !ret)
		    okayreplydir[0] = 0;
		else
		    strcpy(okayreplydir, replydir);
	    }
	    CheckAbsolution(ask, gid);
	}
    }
    me->pr_WindowPtr = mwp;
    return ret;
}


bool DoDirsIDCMP(struct IntuiMessage *im)
{
    static struct Gadget *(gargar[8]) = {
	&dgagsavename, &dgagdown, &dgagup, &dgagbbs,
	&dgagwork, &dgagreply, &dgagprinter, &dgagsavename
    };
    short i, gid, shifty;

    if (im->Class == IDCMP_GADGETUP) {
	if (!GAGFINISH(im))
	    return false;
	gid = ((struct Gadget *) im->IAddress)->GadgetID;
	shifty = (im->Qualifier & SHIFTKEYS ? -1 : 1);
	if (gid == 506) {
	    i = strlen(defsavename);
	    StripString(&dgagsavename, null);
	    if (i != strlen(defsavename))
		RefreshGList(&dgagsavename, owin, null, 1);
	} else if (gid == 505)
	    StripString(&dgagprinter, owin);
	if (gid >= 500 && gid <= 506 && GAGCHAIN(im) && (gid == 506
				? shifty < 0 : EnsureDiffer(false, gid)) ) {
	    i = gid + shifty - 499;
	    while (gargar[i]->Flags & GFLG_DISABLED)
		i += shifty;
	    ActivateGadget(gargar[i], owin, null);		/* next/prev */
	}
	return false;
    }
    if (im->Class == IDCMP_RAWKEY) {
	gid = KeyToAscii(im->Code, im->Qualifier);
	if (gid == ESC)
	    return true;
	else if (gid == '\t') {
	    for (i = 1; i < 4; i++) {			/* first empty dir */
		if (!((struct StringInfo *) gargar[i]->SpecialInfo)->Buffer[0]
				&& !(gargar[i]->Flags & GFLG_DISABLED)) {
		    ActivateGag(gargar[i], owin);
		    return false;
		}
	    }
	    ActivateGag(&dgagdown, owin);
	}
    }
    return im->Class == IDCMP_CLOSEWINDOW && im->IDCMPWindow == owin;
}


void ConfigDirs(void)
{
    short gap = fakefight + 12;

    dgagdown.TopEdge = gap;
    dgagup.TopEdge = dgagdown.TopEdge + gap;
    dgagbbs.TopEdge = dgagup.TopEdge + gap;
    dgagwork.TopEdge = dgagbbs.TopEdge + gap;
    dgagreply.TopEdge = dgagwork.TopEdge + gap;
    dgagprinter.TopEdge = dgagreply.TopEdge + gap + 7 + lace;
    dgagsavename.TopEdge = dgagprinter.TopEdge + gap;
    dneww.Height = dgagsavename.TopEdge + gap + 1;
    /* height when fakefight = 11 (nolace): 192 */
    dneww.TopEdge = min(24, (scr->Height - dneww.Height) / 2);
/*  AbleGad(&dgagwork, !readareaz.messct || fakery); */
/*  AbleGad(&dgagreply, !replylock); */
    if (!(owin = OpenBlueGagWin(&dww, &dgagdown))) {
	WindowErr("configuring directories.");
	return;
    }
    ourqsemnode.editingdirs = true;
    semnotwarned = true;
    SetAPen(owin->RPort, LABELCOLOR);
    gap = dgagprinter.TopEdge - 10;
    Move(owin->RPort, 10, gap);
    Draw(owin->RPort, owin->Width - 11, gap);
    if (lace) {
	Move(owin->RPort, 10, --gap);
	Draw(owin->RPort, owin->Width - 11, gap);
    }
    strcpy(okayworkdir, workdir);
    strcpy(okayreplydir, replydir);
    GhostCompose(true);
    do
	EventLoop(&DoDirsIDCMP);
    while (!EnsureDiffer(true, -1));
    ourqsemnode.editingdirs = false;
    CloseBlueGagWin(owin);
    owin = null;
    GhostCompose(false);
}


/* ------------------------------------------------------------------------ */


void SetLocalSetupName(void)
{
    char pn[32];
    strcpy(pn, packetname);
    strlwr(pn);
    JoinName(localfigname, bbsesdir, pn, qwk ? ".local-QWK" : ".local-BW");
}


/* We specify that we do NOT allow extra spaces around the "=" in     */
/* keyword lines in a local setup file, unlike DOOR.ID and such like. */
/* This way things like the signature can have leading spaces.        */

/* For compatibility with 1.9, localtagstyle is saved as:  0 = none,  */
/* 1 = random, 2 = manual, 3 = global, 4 = sequential.  We translate. */
/* Yes, this is inconsistent with how we handle localtagleadin.       */

#define TSFUDGE(n) (ulong) ((n) >= 3 ? 7 - (n) : (n))

bool WriteBBSlocalSetup(void)
{
    BPTR hand;
    long r;
    char iemcn[8];

    if (!bbsesdir[0]) {
	Err("Cannot save BBS local setup unless\n"
				"a BBS context directory is specified\n"
				"in the Directories setup window.");
	return false;
    }
    SetLocalSetupName();
    if (!(hand = NOpen(localfigname))) {
	DosErr("Could not create BBS local setup\nfile %s", localfigname);
	DeleteFile(localfigname);
	return false;
    }
    SetVBuf(hand, null, BUF_FULL, 512);    /* better performance under 3.x */
    if (FPrintf(hand, "%s v2\n", LOCALLABEL) < 0)
	goto bail;
    if (localtaglinesfile[0])
	if (FPrintf(hand, "TAG_FILENAME=%s\n", localtaglinesfile) < 0)
	    goto bail;
    if (localtagstyle != 4)			/* global */
	if (FPrintf(hand, "TAG_WHEN=%lu\n", TSFUDGE(localtagstyle)) < 0)
	    goto bail; /* V global                 "..." V */
    if (localtagleadin != 2 && (qwk || localtagleadin != 0))
	if (FPrintf(hand, "TAG_LEADIN=%lu\n", (ulong) localtagleadin) < 0)
	    goto bail;
    if (localanyname[0])
	if (FPrintf(hand, "DEFAULT_ALIAS=%s\n", localanyname) < 0)
	    goto bail;
    if (localquoteheader[0])
	if (FPrintf(hand, "QUOTE_HEADER=%s\n", localquoteheader) < 0)
	    goto bail;
    if (localwrapmargin >= 0)
	if (FPrintf(hand, "QUOTE_MARGIN=%lu\n", (ulong) localwrapmargin) < 0)
	    goto bail;
    if (indent_XX != searchlight)
	if (FPrintf(hand, "QUOTES_INDENTED=%s\n", indent_XX ? "Yes" : "No") < 0)
	    goto bail;
    if (qwk && qnetconfnum[0])
	if (FPrintf(hand, "QNET_AREA%s=%s\n", strlen(qnetconfnum) > 5
				? "_7" : "", qnetconfnum) < 0)
	    goto bail;
    if (qwk && qnetkluge[0])
	if (FPrintf(hand, "QNET_KLUGE=%s\n", qnetkluge) < 0)
	    goto bail;
    if (iemailareanum >= 0 && !official_iemail) {
	strcpy(iemcn, areaz.confs[iemailareanum]->confnum);
	if (FPrintf(hand, "IEMAIL_AREA%s=%s\n",
				strlen(iemcn) > 5 ? "_7" : "", iemcn) < 0)
	    goto bail;
    }
    if (iedummyto[0])
	if (FPrintf(hand, "IEMAIL_DUMMYTO=%s\n", iedummyto) < 0)
	    goto bail;
    if ((iemailareanum >= 0 || searchlight || pcboard_hood) && !official_iemail)
	if (FPrintf(hand, "INET_KLUGE_STYLE=%ld\n", ieklugestyle) < 0)
	    goto bail;
    if (gatenet) {
	char foo[40];
	FormatNetAddress(foo, gatezone, gatenet, gatenode, gatepoint);
	if (FPrintf(hand, "INET_FIDO_GATE=%s\n", foo) < 0)
	    goto bail;
    }
#ifdef SHORTS_CHECK
    if (iemail_shorts != (!ieklugestyle && qwk && !inverse_netkluge))
	if (FPrintf(hand, "IEMAIL_LONG_SUBJ_OK=%s\n",
					iemail_shorts ? "No" : "Yes") < 0)
	    goto bail;
#endif
    if (force_pcbkluge | pcbkluge)
	if (FPrintf(hand, "PCBOARD_KLUGES=%s\n", pcbkluge ? "Yes" : "No") < 0)
	    goto bail;
    if (allowblanksubj)
	if (FPrintf(hand, "SUBJECT_BLANK=Yes\n") < 0)
	    goto bail;
    if (localsignature[0])
	if (FPrintf(hand, "SIGNATURE=%s\n", localsignature) < 0)
	    goto bail;
#ifdef LOCAL_SORDER_SOMEDAY
    if (local_sorder)
	if (FPrintf(hand, "SORT=%lu\n", local_sorder) < 0)
	    goto bail;
    if (local_firstfirst)
	if (FPrintf(hand, "SORT_FIRST_FIRST=Yes\n") < 0)
	    goto bail;
#endif
    r = Close(hand);
    hand = 0;
    if (localpacker > 0)
	SetComment(localfigname, packernames[localpacker - 1]);
    else
	SetComment(localfigname, "");
    if (!r)
	goto bail;
    return true;
  bail:
    DosErr("Could not write BBS local setup\nfile %s", localfigname);
    if (hand)
	Close(hand);
    DeleteFile(localfigname);
    return false;
}


void ToggleLocalPacker(short inc)
{
    ushort howmany = packers + 1;
    localpacker = (localpacker + inc + howmany) % howmany;
    ChangeGagText(&llgagpacker, llwin, &llgtpackers[localpacker]);
    RefreshGList(&llgagpacker, llwin, null, 1);
}


#ifdef SHORTS_CHECK
void AbleShortSub(void)
{
    AbleAddGad(&l2gagieshortsub, l2win, qwk && !ieklugestyle &&
				(subjectlen > 25 /* || inverse_netkluge */ ));
}
#endif


void FillFidoGate(void)
{
    long p;
    register bool in = !ieklugestyle && iemailareanum >= 0 &&
			((qwk && (inverse_netkluge || pcbkluge
				|| strchr(qnetkluge, '/')))
			 || areaz.confs[iemailareanum]->areabits & INF_NETMAIL);
    
    if (l2win)			/* vvv  CAUTION: may be already removed! */
	p = RemoveGadget(l2win, &l2gagfidogate);	/* p == 65535 if so */
    if (in && !gatenet)
	gatenet = tempgatenet, tempgatenet = 0;
    else if (gatenet && !in)
	tempgatenet = gatenet, gatenet = 0;
    if (gatenet)
	FormatNetAddress(fidogate, gatezone, gatenet, gatenode, gatepoint);
    else
	fidogate[0] = 0;
    ie_is_gated = in && (gatenet || !qwk);
    AbleGad(&l2gagfidogate, in);
    if (l2win) {
	FixStringGad(&l2gagfidogate, l2win);
	if ((signed short) p != -1)
	    AddGadget(l2win, &l2gagfidogate, p);
    }
}


local void ShowQNKA(bool internet)
{
    struct RastPort *l2rp = l2win->RPort;
    char buf[16];
    char *conum = internet ? iemailconfnum : qnetconfnum;
    struct Gadget *ref = internet ? &l2gagiemailarea : &l2gagqnetarea;
    long left = l2gagqnetarea.LeftEdge + XActual(88);
    long top = ref->TopEdge + 2;

    Move(l2rp, left, top + font->tf_Baseline);
    SetAPen(l2rp, TEXTCOLOR);
    SetBPen(l2rp, backcolor);
    SetDrMd(l2rp, JAM2);
    sprintf(buf, "%s       ", conum);
    Text(l2rp, buf, 7);
    if (!internet && qnetconfnum[0] && netmailareanum < 0) {
	SetAPen(l2rp, backcolor);
	SetAfPt(l2rp, ghostpat, 1);
	SetDrMd(l2rp, JAM1);
	RectFill(l2rp, left, top, left + 7 * fontwid - 1, top + fight - 1);
	SetAfPt(l2rp, null, 0);
	SetDrMd(l2rp, JAM2);
    }
}


void BelieveQWKnetmail(void)
{
    ushort p;
    bool qnk = !!strchr(qnetkluge, '/');

    StripString(&l2gagiedummy, l2win);
    if (qwk) {
	for (p = 0; p < areaz.messct; p++)
	    areaz.confs[p]->areabits &= ~INF_NETMAIL;
	netmailareanum = -1;
	pcboard_net = false;
	if (qnetconfnum[0] && (inverse_netkluge || pcbkluge || qnk))
	    if (Confind(qnetconfnum), (netmailareanum = confounded) >= 0) {
		areaz.confs[netmailareanum]->areabits |= INF_NETMAIL;
		pcboard_net = pcbkluge && !qnk;
	    }
    }
    if (!official_iemail) {
	for (p = 0; p < areaz.messct; p++)
	    areaz.confs[p]->morebits &= ~INTERNET_EMAIL;
	iemailareanum = -1;
	if (iemailconfnum[0])
	    if (Confind(iemailconfnum), (iemailareanum = confounded) >= 0) {
		areaz.confs[iemailareanum]->morebits |= INTERNET_EMAIL;
	    }
    }
    FillFidoGate();
    if (qwk && iemailareanum >= 0 && ie_is_gated)
	pcboard_net |= pcbkluge && !qnk;
    ActMenu(2, 5, iemailareanum >= 0);
}


void ToggleIEstyle(short inc)
{
    bool wasgeneric = !ieklugestyle;
    ieklugestyle = (ieklugestyle + inc + 3) % 3;
    ChangeGagText(&l2gagiestyle, l2win, l2gtstyles[ieklugestyle]);
    RefreshGList(&l2gagiestyle, l2win, null, 1);
    searchlight_ie = ieklugestyle == 2;
    pcboard_ie = ieklugestyle == 1;
    ynwin = l2win;
    if (qwk)
	if (pcbkluge = ieklugestyle ? ieklugestyle <= 1 : was_pcbkluge)
	    subjectlen = 60;
	else
	    subjectlen = orig_subjectlen;
    AbleAddGad(&l2gagpcbsubj, l2win, qwk && !ieklugestyle);
    Seyn(&l2gagpcbsubj, pcbkluge);
    if (ieklugestyle) {
	if (iedummyto[0])
	    strcpy(sparedummy, iedummyto), iedummyto[0] = 0;
	else if (wasgeneric)	/* make erased dummy-to STAY erased */
	    sparedummy[0] = 0;
    } else if (sparedummy[0] && !iedummyto[0])
	strcpy(iedummyto, sparedummy);
    AbleAddGad(&l2gagiedummy, l2win, !ieklugestyle);
#ifdef SHORTS_CHECK
    AbleShortSub();
    Seyn(&l2gagieshortsub, (iemail_shorts /* = !ieklugestyle */ )
				&& !(l2gagieshortsub.Flags & GFLG_DISABLED));
#endif
    BelieveQWKnetmail();
    ShowQNKA(false);
    FillFidoGate();
}


local void SetNetmailFlags(void)
{
    if (pcboard_net)
	hostnetmailflags = INF_CAN_CRASH | INF_CAN_DIRECT;
    else if (!stricmp(doorconn, "MKQWK"))
	hostnetmailflags = INF_CAN_CRASH | INF_CAN_DIRECT | INF_CAN_IMM;
    else
	hostnetmailflags = 0;
}


local void SetLocalFigDefaults(void)
{
    localtaglinesfile[0] = localanyname[0] = localquoteheader[0] = qnetkluge[0]
				= qnetconfnum[0] = sparedummy[0] = iedummyto[0]
				= iemailconfnum[0] = localsignature[0] = 0;
    localtagstyle = 4;			/* (global) */
    localtagleadin = qwk ? 2 : 0;	/* (global) or "..." */
    force_pcbkluge = allowblanksubj = local_firstfirst = ie_is_gated = false;
    indent_XX = searchlight;
    gatezone = gatenet = gatenode = gatepoint = tempgatenet = local_sorder = 0;
    ieklugestyle = searchlight ? 2 : (pcboard_hood ? 1 : 0);
    searchlight_ie = ieklugestyle == 2;
    pcboard_ie = ieklugestyle == 1;
#ifdef SHORTS_CHECK
    iemail_shorts = !ieklugestyle && !inverse_netkluge && qwk;
#endif
    localwrapmargin = -1;		/* blank (global) */
    if (!official_iemail)
	iemailareanum = -1;
    if (qwk)
	netmailareanum = -1;
}


void ReadBBSlocalSetup(bool askfile)
{
    char line[SIGNATURELEN + 20];
    str tail;
    short p, p2, v, namelen = qwk ? 25 : NAMELEN - 1;
    BHandle hand;

    if (askfile) {
	JoinName(localfigname, bbsesdir, packetname,
					qwk ? ".local-QWK" : ".local-BW");
	if (!DoFileRequest(false, true, localfigname, null, "#?.local-(QWK|BW)",
						 "Select BBS local setup file"))
	    return;
    } else
	SetLocalSetupName();
    if (!(hand = BOpen(localfigname, false))) {
	if (askfile)
	    DosErr("Could not read local setup\nfile %s", localfigname);
	else {
	    ASSERT(!llwin);
	    SetLocalFigDefaults();
	    ActMenu(2, 5, iemailareanum >= 0);
	}
	return;
    }
    if (SafeExamineFH(DOSHANDLE(hand), fib, localfigname)
					&& fib->fib_Comment[0]) {
	localpacker = 0;
	for (p = 0; p < packers; p++)
	    if (!stricmp(fib->fib_Comment, packernames[p])) {
		localpacker = p + 1;
		break;
	    }
    }
    BGetline(hand, line, SIGNATURELEN + 19);
    if (strncmp(line, LOCALLABEL, strlen(LOCALLABEL))) {
	Err("%s is not\na valid BBS Local setup file.", localfigname);
	ExhaleFile();
	if (!askfile) {
	    SetLocalFigDefaults();
	    ActMenu(2, 5, iemailareanum >= 0);
	}
	return;
    }
    if (llwin)
	p = RemoveGList(llwin, &llgagtagfile, llww.gacount);
    if (l2win)
	p2 = RemoveGList(l2win, &l2gagqnetarea, l2ww.gacount);
    /* for now, don't check version number after LOCALLABEL */
    SetLocalFigDefaults();
    while (BGetline(hand, line, SIGNATURELEN + 19) >= 0) {
	char first = _toupper(line[0]);
	if (!(tail = strchr(line, '=')))
	    continue;
	*tail++ = 0;
	if (first == 'T') {
	    if (!stricmp(line, "TAG_FILENAME"))
		strncpy0(localtaglinesfile, tail, COMMANDLEN - 1);
	    else if (!stricmp(line, "TAG_WHEN")) {
		if ((v = atol(tail)) < 0 || v > 4)
		    localtagstyle = 4;
		else
		    localtagstyle = TSFUDGE(v);
	    } else if (!stricmp(line, "TAG_LEADIN")) {
		if ((v = atol(tail)) < 0 || v > 3 || (!qwk && v != 3))
		    localtagleadin = qwk ? 2 : 0;
		else
		    localtagleadin = v;
	    }
	} else if (first == 'Q') {
	    if (!stricmp(line, "QUOTE_HEADER"))
		strncpy0(localquoteheader, tail, COMMANDLEN - 1);
	    else if (!stricmp(line, "QUOTES_INDENTED"))
		indent_XX = _toupper(*tail) != 'N';
	    else if (!stricmp(line, "QUOTE_MARGIN")) {
		if ((v = atol(tail)) >= 0 && v <= 255)
		    localwrapmargin = v;
	    } else if (qwk && !stricmp(line, "QNET_AREA_7") && Confind(tail))
		strncpy0(qnetconfnum, tail, 7);
	    else if (qwk && !stricmp(line, "QNET_AREA") && Confind(tail))
		strncpy0(qnetconfnum, tail, 5);
	    else if (qwk && !stricmp(line, "QNET_KLUGE"))
		strncpy0(qnetkluge, tail, NAMELEN - 1);
	} else if (first == 'I' && !official_iemail) {
	    if (!stricmp(line, "IEMAIL_AREA_7") && Confind(tail))
		strncpy0(iemailconfnum, tail, 7);
	    else if (!stricmp(line, "IEMAIL_AREA") && Confind(tail))
		strncpy0(iemailconfnum, tail, 5);
	    else if (!stricmp(line, "IEMAIL_DUMMYTO"))
		strncpy0(iedummyto, tail, namelen);
	    else if (!stricmp(line, "INET_KLUGE_STYLE"))
		if ((v = atol(tail)) < 0 || v > 2)
		    ieklugestyle = 0;
		else
		    ieklugestyle = v;
	    else if (!stricmp(line, "INET_FIDO_GATE")) {
		if (ParseNetAddress(tail))
		    gatezone = czone, gatenet = cnet, gatenode = cnode,
						gatepoint = cpoint;
	    }
#ifdef SHORTS_CHECK
	    else if (!stricmp(line, "IEMAIL_LONG_SUBJ_OK"))
		iemail_shorts = _toupper(*tail) == 'N';
#endif
	} else if (first == 'S') {
	    if (!stricmp(line, "SIGNATURE"))
		strncpy0(localsignature, tail, SIGNATURELEN - 1);
	    else if (!stricmp(line, "SUBJECT_BLANK"))
		allowblanksubj = _toupper(*tail) != 'N';
#ifdef LOCAL_SORDER_SOMEDAY
	    else if (!stricmp(line, "SORT"))
		if ((v = atol(tail)) <= 0 || v > 7)
		    local_sorder = 0;
		else
		    local_sorder = v;
	    else if (!stricmp(line, "SORT_FIRST_FIRST"))
		local_firstfirst = _toupper(*tail) != 'N';
#endif
	} else if (!stricmp(line, "DEFAULT_ALIAS"))
	    strncpy0(localanyname, tail, namelen);
	else if (!stricmp(line, "PCBOARD_KLUGES")) {
	    pcbkluge = was_pcbkluge = _toupper(*tail) != 'N';
	    force_pcbkluge = true;
	}
    }
    BClose(hand);
    if (searchlight_ie = ieklugestyle == 2)
	pcbkluge = false;
    if (pcboard_ie = ieklugestyle == 1)
	pcbkluge = true;
    if (ieklugestyle)
	strcpy(sparedummy, iedummyto), iedummyto[0] = 0;
    BelieveQWKnetmail();
    if (qwk)
	subjectlen = pcbkluge ? 60 : orig_subjectlen;
    if (llwin) {
	AddGList(llwin, &llgagtagfile, p, llww.gacount, null);
	ToggleTagStyle(0, llwin);
	ToggleTagLeadin(0, llwin);
	ToggleLocalPacker(0);
	RefreshGList(&llgagtagfile, llwin, null, 1);
	RefreshGList(&llgaganyname, llwin, null, 2);	/* gets qhead */
	ynwin = llwin;
	Seyn(&llgagblanksubj, allowblanksubj);
	Seyn(&llgagxxindent, indent_XX);
    }
    if (l2win) {
	AddGList(l2win, &l2gagqnetarea, p2, l2ww.gacount, null);
	ToggleIEstyle(0);
	ynwin = l2win;
	AbleAddGad(&l2gagpcbsubj, l2win, qwk && !ieklugestyle);
	Seyn(&l2gagpcbsubj, pcbkluge);
#ifdef SHORTS_CHECK
	Seyn(&l2gagieshortsub, iemail_shorts
				&& !(l2gagieshortsub.Flags & GFLG_DISABLED));
	AbleShortSub();
#endif
	GhostOn(l2win);
	RefreshGList(&l2gagiedummy, l2win, null, 1);
	if (qwk) {
	    RefreshGList(&l2gagqnetkluge, l2win, null, 1);
	    ShowQNKA(false);
	}
	ShowQNKA(true);
	GhostOff(l2win);
    }
    DetectStealthiness(localtagleadin, true);
    SetNetmailFlags();
}


void GhostGagsRemember(struct Window *ww, bool off)
{
    register struct Gadget *gg;
    for (gg = ww->FirstGadget; gg; gg = gg->NextGadget)
	if (!(gg->GadgetType & GTYP_SYSGADGET))
	    if (off) {
		if (!(gg->Flags & GFLG_DISABLED)) {
		    AbleAddGad(gg, ww, false);
		    gg->GadgetID |= 0x8000;
		}
	    } else if (gg->GadgetID & 0x8000) {
		gg->GadgetID &= ~0x8000;
		AbleAddGad(gg, ww, true);
	    }
}


void GhostLocalSetup(bool off)
{
    GhostGagsRemember(llwin, off);
    if (l2win)
	GhostGagsRemember(l2win, off);
}


local void SelectQNKA(bool internet)
{
    short a = internet ? iemailareanum : netmailareanum;
    char *conum = internet ? iemailconfnum : qnetconfnum;

    if (internet ? official_iemail : !qwk)
	return;
    if (a < 0 && conum[0])
	Confind(conum), a = confounded;
    GhostLocalSetup(true);
    do {
	if (newareawin)
	    a = AddUnknownArea(a);
	if (internet && a < 0)
	    a = -2;
	if (!poseclosed)
	    a = ListAreas(a, true, false, true);
    } while (newareawin);
    GhostLocalSetup(false);
    if (wasescaped)
	return;
    if (a >= 0)
	strcpy(conum, areaz.confs[a]->confnum);
    else
	conum[0] = 0;
    BelieveQWKnetmail();
    ShowQNKA(internet);
}


local bool CheckQNKL(void)
{
    if (!strchr(qnetkluge, '/')) {
	StripString(&l2gagqnetkluge, l2win);
	if (qnetkluge[0]) {
	    Err("If you define a QWK netmail kluge line, it\n"
			"must contain a slash character \"/\" to show\n"
			"where the Fido-style network address goes.");
	    return false;
	}
    }
    return true;
}


local bool CheckLocalWrap(void)
{
    if (!localmargin[0]) {
	localwrapmargin = -1;
	return true;
    }
    if (llstrmargin.LongInt < 0 || llstrmargin.LongInt > 80) {
	localwrapmargin = -1;
	localmargin[0] = 0;
	if (llwin) {
	    long p = RemoveGadget(llwin, &llgagmargin);
	    FixStringGad(&llgagmargin, llwin);
	    AddGadget(llwin, &llgagmargin, p);
	}
	return false;
    }
    localwrapmargin = llstrmargin.LongInt;
    return true;
}


local bool CheckFidoGate(void)
{
    StripString(&l2gagfidogate, l2win);
    if (!fidogate[0]) {
	gatezone = gatenet = gatenode = gatepoint = 0;
	return true;
    }
    if (!ParseNetAddress(fidogate))
	return false;
#ifdef NO_GATE_POINT
    if (cpoint) {
	Err("A Fido gateway address may not\nuse a nonzero point number.");
	return false;
    }
#endif
    gatezone = czone;
    gatenet = cnet;
    gatenode = cnode;
    gatepoint = cpoint;
    if (ieklugestyle)				/* currently can't happen */
	ToggleIEstyle(-ieklugestyle);		/* force "Generic" */
    return true;
}


void OpenMailWindow(void)
{
    if (official_iemail || l2win)
	return;
    l2gagqnetkluge.TopEdge = fakefight + 12;
    l2gagqnetarea.TopEdge = l2gagqnetkluge.TopEdge - 2;
    l2gagiedummy.TopEdge = l2gagqnetkluge.TopEdge + fakefight + 12;
    l2gagiemailarea.TopEdge = l2gagiedummy.TopEdge - 2;
    l2gagfidogate.TopEdge = l2gagiedummy.TopEdge + fakefight + 12;
    l2gagiestyle.TopEdge = l2gagfidogate.TopEdge + fakefight + 9;
    l2gagpcbsubj.TopEdge = l2gagiestyle.TopEdge + 1 + checkoff;
#ifdef SHORTS_CHECK
    l2gagieshortsub.TopEdge = l2gagpcbsubj.TopEdge + fakefight + 11 + checkoff;
    l2neww.Height = l2gagieshortsub.TopEdge + 22 + checkoff;
    /* Height with fakefight == 11: 134 */
    AbleShortSub();
#else
    l2neww.Height = l2gagiestyle.TopEdge + fakefight + 13;
#endif
    AbleGad(&l2gagqnetarea, qwk);
    AbleGad(&l2gagqnetkluge, qwk);
    /* AbleGad(&l2gagfidogate, !ieklugestyle && IEinNetmailArea()); */
    FillFidoGate();
    AbleGad(&l2gagpcbsubj, qwk && !ieklugestyle);
    AbleGad(&l2gagiedummy, !ieklugestyle);
    if (!(l2win = OpenBlueGagWin(&l2ww, &l2gagqnetarea))) {
	WindowErr("BBS local configuration options.");
	return;
    }
    ynwin = l2win;
    pcbkluge = ieklugestyle ? ieklugestyle <= 1 : was_pcbkluge;
    SeynLabel(&l2gagpcbsubj, pcbkluge, "Use PCBoard extensions");
#ifdef SHORTS_CHECK
    SeynLabel(&l2gagieshortsub, iemail_shorts &&
				!(l2gagieshortsub.Flags & GFLG_DISABLED),
				">>>>long subject kluge does NOT work"
				" with internet email");
#endif
    ToggleIEstyle(0);
    UnderstartedText(XActual(16), l2gagiestyle.TopEdge + 3,
				l2win->RPort, ">email Kluge style:");
    if (qwk && netmailareanum >= 0)
	strcpy(qnetconfnum, areaz.confs[netmailareanum]->confnum);
    ShowQNKA(false);
    ShowQNKA(true);
}


bool DoBLocalIDCMP(struct IntuiMessage *im)
{
    char k;
    ushort shifty = im->Qualifier & SHIFTKEYS;
    short cycleinc = shifty ? -1 : 1;
    bool finn = GAGFINISH(im) && GAGCHAIN(im);
    bool on2fido = !(l2gagfidogate.Flags & GFLG_DISABLED);
    bool on2ied = !(l2gagiedummy.Flags & GFLG_DISABLED);
    bool on2qkluge = !(l2gagqnetkluge.Flags & GFLG_DISABLED);

    poseclosed = false;
    if (im->Class == IDCMP_RAWKEY && !(im->Qualifier & ALTKEYS)) {
	switch (k = KeyToAscii(im->Code, im->Qualifier)) {
	  case '\t':
	    if (!l2win || IntuitionBase->ActiveWindow == llwin
				|| !(on2fido | on2ied | on2qkluge)
				/* || !IsInFrontOf(l2win, llwin) */ )
		ActivateGag(&llgagtagfile, llwin);
	    else
		ActivateGag(on2qkluge ? &l2gagqnetkluge : (on2ied
				    ? &l2gagiedummy : &l2gagfidogate), l2win);
	    break;
	  case ESC:
	    if (l2win) {
		CloseBlueGagWin(l2win);
		l2win = null;
	    } else
		return true;
	    break;
	  case 'D':
	    ToggleTagStyle(cycleinc, llwin);
	    break;
	  case 'T':
	    ToggleTagLeadin(cycleinc, llwin);
	    break;
	  case 'L':
	    ReadBBSlocalSetup(true);
	    break;
	  case 'S':
	    CheckLocalWrap();
	    return WriteBBSlocalSetup();
	  case 'A':
	  case 'E':
	    SelectQNKA(k == 'E');
	    if (poseclosed)
		return true;
	    break;
	  case 'C':
	    ToggleLocalPacker(cycleinc);
	    break;
	  case 'M':
	    OpenMailWindow();
	    break;
	  case 'P':
	    ynwin = llwin;
	    Seyn(&llgagblanksubj, allowblanksubj = !allowblanksubj);
	    break;
	  case 'I':
	    ynwin = llwin;
	    Seyn(&llgagxxindent, indent_XX = !indent_XX);
	    break;
#ifdef SHORTS_CHECK
	  case 'N':
	    if (!(l2gagieshortsub.Flags & GFLG_DISABLED)) {
		ynwin = l2win;
		Seyn(&l2gagieshortsub, iemail_shorts = !iemail_shorts);
	    }
	    break;
#endif
	  case 'K':
	    if (l2win)
		ToggleIEstyle(cycleinc);
	    break;
	  case 'U':
	    if (l2gagpcbsubj.Flags & GFLG_DISABLED)
		break;
	    ynwin = l2win;
	    if (qwk)
		subjectlen = (was_pcbkluge = pcbkluge = !pcbkluge)
						? 60 : orig_subjectlen;
	    Seyn(&l2gagpcbsubj, pcbkluge);
#ifdef SHORTS_CHECK
	    AbleShortSub();
#endif
	    BelieveQWKnetmail();
	    ShowQNKA(false);
	    force_pcbkluge = true;
	    break;
	}
    } else if (im->Class == IDCMP_GADGETUP) {
	switch (((struct Gadget *) im->IAddress)->GadgetID) {
	  case 2300:						/* tag style */
	    ToggleTagStyle(cycleinc, llwin);
	    break;
	  case 2301:						/* tag leadin */
	    ToggleTagLeadin(cycleinc, llwin);
	    break;
	  case 2303:						/* tag file */
	    if (finn)
		ActivateGadget(shifty ? &llgagmargin
					: &llgaganyname, llwin, null);
	    break;
	  case 2302:						/* any name */
	    if (finn)
		ActivateGadget(shifty ? &llgagtagfile
					: &llgagqhead, llwin, null);
	    break;
	  case 2304:						/* quote hdr */
	    if (finn)
		ActivateGadget(shifty ? &llgaganyname
					: &llgagsignature, llwin, null);
	    break;
	  case 2305:						/* net area */
	    SelectQNKA(false);
	    if (poseclosed)
		return true;
	    break;
	  case 2306:						/* net kluge */
	    if (!CheckQNKL())
		ActivateGadget(&l2gagqnetkluge, l2win, null);
	    else {
		if (GAGFINISH(im)) {
		    BelieveQWKnetmail();
		    ShowQNKA(false);
		}
		if (finn && on2fido | on2ied)
		    ActivateGadget(shifty & on2fido || !on2ied ? &l2gagfidogate
					: &l2gagiedummy, l2win, null);
	    }
	    break;
	  /* case 2307:  quote style */
	  case 2308:						/* margin */
	    if (!CheckLocalWrap())
		ActivateGadget(&llgagmargin, llwin, null);
	    else if (finn && shifty)
		ActivateGadget(&llgagsignature, llwin, null);
	    break;
	  case 2309:						/* load */
	    ReadBBSlocalSetup(true);
	    break;
	  case 2310:						/* save */
	    CheckLocalWrap();
	    return WriteBBSlocalSetup();
	  case 2311:
	    ToggleLocalPacker(cycleinc);
	    break;
	  case 2312:						/* signature */
	    if (finn)
		ActivateGadget(shifty ? &llgagqhead : &llgagmargin, llwin, null);
	    break;
	  case 2313:						/* email area */
	    SelectQNKA(true);
	    if (poseclosed)
		return true;
	    break;
	  case 2314:						/* dummy to */
	    if (GAGFINISH(im))
		BelieveQWKnetmail();
	    if (finn && (on2fido || (shifty && on2qkluge)))
		ActivateGadget(shifty && on2qkluge ? &l2gagqnetkluge
					: &l2gagfidogate, l2win, null);
	    break;
	  case 2315:						/* Mail... */
	    OpenMailWindow();
	    break;
	  case 2316:						/* blank subj */
	    CHECK(llgagblanksubj, allowblanksubj);
	    break;
	  case 2317:						/* fido gate */
	    if (!CheckFidoGate())
		ActivateGadget(&l2gagfidogate, l2win, null);
	    else if (finn && shifty && on2ied | on2qkluge)
		ActivateGadget(on2ied ? &l2gagiedummy
					: &l2gagqnetkluge, l2win, null);
	    break;
	  case 2318:						/* IE style */
	    ToggleIEstyle(cycleinc);
	    break;
	  case 2319:						/* PCB subj */
	    CHECK(l2gagpcbsubj, pcbkluge);
	    if (qwk)
		subjectlen = (was_pcbkluge = pcbkluge) ? 60 : orig_subjectlen;
	    ynwin = l2win;
#ifdef SHORTS_CHECK
	    AbleShortSub();
	    Seyn(&l2gagieshortsub, iemail_shorts
				&& !(l2gagieshortsub.Flags & GFLG_DISABLED));
#endif
	    BelieveQWKnetmail();
	    ShowQNKA(false);
	    force_pcbkluge = true;
	    break;
	  case 2320:						/* indent XX> */
	    CHECK(llgagxxindent, indent_XX);
	    break;
#ifdef SHORTS_CHECK
	  case 2321:					 /* short iemail subj */
	    CHECK(l2gagieshortsub, iemail_shorts);
	    break;
#endif
	}
    } else if (im->Class == IDCMP_CLOSEWINDOW) {
	if (im->IDCMPWindow == l2win) {
	    CloseBlueGagWin(l2win);
	    l2win = null;
	} else if (im->IDCMPWindow == llwin)
	    return true;
    }
    return false;
}


void ConfigBLocal(void)
{
    short i;

    if (!readareaz.messct)
	return;
    l2win = null;
    tempgatenet = 0;
    was_pcbkluge = pcbkluge /* && !pcboard_ie */ ;
    llgagtagfile.TopEdge = fakefight + 12;
    ylabeltagfile.LeftEdge = -260;
    llgagtagstyle.TopEdge = llgagtagleadin.TopEdge
				= llgagtagfile.TopEdge + fakefight + 9;
    llgaganyname.TopEdge = llgagtagstyle.TopEdge + fakefight + 14;
    llgagqhead.TopEdge = llgaganyname.TopEdge + fakefight + 12;
    llgagsignature.TopEdge = llgagqhead.TopEdge + fakefight + 12;
    llgagpacker.TopEdge = llgagsignature.TopEdge + fakefight + 9;
    llgagblanksubj.TopEdge = llgagpacker.TopEdge + 1 + checkoff;
    llgagmargin.TopEdge = llgagpacker.TopEdge + fakefight + 14;
    llgagxxindent.TopEdge = llgagmargin.TopEdge - 2 + checkoff;
    llgagload.TopEdge = llgagsave.TopEdge = llgagmailwin.TopEdge =
				llgagmargin.TopEdge + fakefight + 9;
    llneww.Height = llgagload.TopEdge + fakefight + 13;
    /* height when fakefight = 11: 197 */
    llneww.TopEdge = scr->Height - llneww.Height
				- (lace ? 3 * fakefight : fakefight);
    AbleGad(&llgagmailwin, !official_iemail);
    if (!qwk && localtagleadin != 3)
	localtagleadin = 0;
    llstranyname.MaxChars = l2striedummy.MaxChars = qwk ? 26 : NAMELEN;
    if (localwrapmargin >= 0) {
	utoa(localwrapmargin, localmargin);
	llstrmargin.LongInt = (long) localwrapmargin;
    } else
	localmargin[0] = llstrmargin.LongInt = 0;
    if (!(llwin = OpenBlueGagWin(&llww, &llgagtagfile))) {
	WindowErr("BBS local configuration options.");
	return;
    }

    for (i = 0; i < packers; i++)
	sprintf(llgtpackers[i + 1].IText, " %-7s ", packernames[i]);
    ToggleTagStyle(0, llwin);
    ToggleTagLeadin(0, llwin);
    ToggleLocalPacker(0);
    UnderstartedText(XActual(16), llgagtagstyle.TopEdge + 3, llwin->RPort,
				"Default tagline:");
    UnderstartedText(llgagtagleadin.LeftEdge - XActual(118),
				llgagtagleadin.TopEdge + 3, llwin->RPort,
				"Tagline after:");
    UnderstartedText(XActual(16), llgagpacker.TopEdge + 3, llwin->RPort,
				"Compressor:");
    ynwin = llwin;
    SeynLabel(&llgagblanksubj, allowblanksubj, "Permit blank To & Subject");
    SeynLabel(&llgagxxindent, indent_XX, "Indent XX> quotes");
    GhostCompose(true);
    do
	EventLoop(&DoBLocalIDCMP);
    while (!CheckQNKL() || !CheckFidoGate());
    CheckLocalWrap();
    BelieveQWKnetmail();
    DetectStealthiness(localtagleadin, false);
    if (l2win)
	CloseBlueGagWin(l2win);
    CloseBlueGagWin(llwin);
    ynwin = llwin = l2win = null;
    GhostCompose(false);
    SetNetmailFlags();
}
