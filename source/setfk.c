/* the font&screen and function key setup windows */

#define ASL_V38_NAMES_ONLY

#include <intuition/sghooks.h>
#include <intuition/intuition.h>
#include <graphics/displayinfo.h>
#include <graphics/gfxmacros.h>
#include <libraries/asl.h>
#include "qblue.h"
#include "pigment.h"


#define SWINWIDTH   548
#define SWINHEIGHT  178

#define CPROPFLAGS  (AUTOKNOB | FREEHORIZ | PROPNEWLOOK  | PROPBORDERLESS)
#define CPOTBODY    0xFFF
#define CPOTSTEP    0x1111


struct TextFont *FindFont(struct TextAttr *fa, short scrheight);
void FixShine(struct Gadget *example);
void FitHeight(struct WhereWin *ww);
void SetAllSharedWindowTitles(str title);
void Iconify(bool instant);
bool AnySharedPresent(void);
void PadRight(str s, short len);
void UpdateClock(void);
bool Substitutions(str after, str fullname, str filesdir, str before,
				bool editing, bool un, bool rep);


import struct Border elongbox, yfsizebox, underborder;
import struct Library *AslBase;

import ushort palette[8], palette4[4];
#ifdef SOMEDAY
import ushort lb_palt[8], lb_palt4[4];
#endif
import char title[];
import str xcommand;

import long modeID, endtag, shadowcolor;
import ushort taheight, tifight;
import bool laterlace, latercustomode, laterfourcolors, lastkeywasshifted;
import bool customautoscroll, custom4color, lightbg, foundbutbad;


ushort ghostpat[2] = { 0x8888, 0x2222 };   /* one bit left of C= pattern */
ushort laterfontwid;
bool customshowprops = false;

local short palettones, slidecolor, lastslidecolor, lastmodeghost;
local ushort slotwid, slothi, slotsplit;
local ushort *paltouse;
local bool reopen;


short sslidedownright[10] = { 0, 10, 212, 10, 212, 0, 213, -1, 213, 10 };

short sslideupleft[10] = { -2, -1, -2, 10, -1, 9, -1, -1, 212, -1 };

struct Border sslidebor2 = {
    0, 0, SHADOWCOLOR, 0, JAM2, 5, sslidedownright, null
};

struct Border sslidebor = {
    0, 0, SHINECOLOR, 0, JAM2, 5, sslideupleft, &sslidebor2
};


struct Image sautoiblue = { 0 /*, 0, 0, 0, 3, null, 7, 0, null */ };

struct PropInfo spropsblue = {
    CPROPFLAGS, 0, 0, CPOTBODY, 0, 0, 0, 0, 0, 0, 0
};

struct Gadget sgagblue = {
    null, 120, 140, 212, 10, GFLG_GADGHNONE | GFLG_GADGIMAGE,
    GACT_RELVERIFY | GACT_FOLLOWMOUSE, GTYP_PROPGADGET,
    &sautoiblue, null, null, 0, &spropsblue, 1808, null
};


struct Image sautoigreen = { 0 /*, 0, 0, 0, 3, null, 7, 0, null */ };

struct PropInfo spropsgreen = {
    CPROPFLAGS, 0, 0, CPOTBODY, 0, 0, 0, 0, 0, 0, 0
};

struct Gadget sgaggreen = {
    &sgagblue, 120, 120, 212, 10, GFLG_GADGHNONE | GFLG_GADGIMAGE,
    GACT_RELVERIFY | GACT_FOLLOWMOUSE, GTYP_PROPGADGET,
    &sautoigreen, null, null, 0, &spropsgreen, 1807, null
};


struct Image sautoired = { 0 /*, 0, 0, 0, 3, null, 7, 0, null */ };

struct PropInfo spropsred = {
    CPROPFLAGS, 0, 0, CPOTBODY, 0, 0, 0, 0, 0, 0, 0
};

struct Gadget sgagred = {
    &sgaggreen, 120, 100, 212, 10, GFLG_GADGHNONE | GFLG_GADGIMAGE,
    GACT_RELVERIFY | GACT_FOLLOWMOUSE, GTYP_PROPGADGET,
    &sautoired, null, null, 0, &spropsred, 1806, null
};


short swatchdownright[10] = { 1, 14, 198, 14, 198, 1, 199, 0, 199, 14 };

short swatchupleft[10] = { 0, 0, 0, 14, 1, 13, 1, 0, 198, 0 };

struct Border sswatchborder2 = {
    0, 0, SHINECOLOR, 0, JAM2, 5, swatchdownright, null
};

struct Border sswatchborder = {
    0, 0, SHADOWCOLOR, 0, JAM2, 5, swatchupleft, &sswatchborder2
};

struct Gadget fakegagswatch = { null, 0, 0, 0, 0, 0, 0, 0, &sswatchborder };


short sbigdownright[10] = { 1, 14, 198, 14, 198, 1, 199, 0, 199, 14 };

short sbigupleft[10] = { 0, 0, 0, 14, 1, 13, 1, 0, 198, 0 };

struct Border sbigcolorbor2 = {
    0, 0, SHADOWCOLOR, 0, JAM2, 5, sbigdownright, null
};

struct Border sbigcolorbor = {
    0, 0, SHINECOLOR, 0, JAM2, 5, sbigupleft, &sbigcolorbor2
};

struct Gadget sgagpalette = {
    &sgagred, 60, 80, 198, 15, GFLG_GADGHNONE, GACT_IMMEDIATE,
    GTYP_BOOLGADGET, &sbigcolorbor, null, null, 0, null, 1805, null
};


struct IntuiText slabelundo = {
    TEXTCOLOR, FILLCOLOR, JAM2, 12, 2, null, "Undo", null
};

struct Gadget sgagundo = {
    null, 132, 21, 80, 13, GFLG_GADGHCOMP, GACT_RELVERIFY, GTYP_BOOLGADGET,
    &upborder, null, &slabelundo, 0, null, 1809, null
};


struct IntuiText slabelopen = {
    TEXTCOLOR, FILLCOLOR, JAM2, 12, 2, null, "Open!", null
};

struct Gadget sgagopen = {
    &sgagundo, SWINWIDTH - 96, 21, 80, 13, GFLG_GADGHCOMP, GACT_RELVERIFY,
    GTYP_BOOLGADGET, &upborder, null, &slabelopen, 0, null, 1810, null
};


struct IntuiText slabelmode = {
    TEXTCOLOR, FILLCOLOR, JAM2, 12, 2, null, "Mode:", null
};

struct Gadget sgagmode = {
    &sgagopen, 16, 21, 80, 13, GFLG_GADGHCOMP, GACT_RELVERIFY, GTYP_BOOLGADGET,
    &upborder, null, &slabelmode, 0, null, 1804, null
};


struct IntuiText sgtscrC = {
    TEXTCOLOR, FILLCOLOR, JAM2, 31, 3, null, " Custom ", null
};

struct IntuiText sgtscr8L = {
    TEXTCOLOR, FILLCOLOR, JAM2, 31, 3, null, "8  Lace ", null
};

struct IntuiText sgtscr8 = {
    TEXTCOLOR, FILLCOLOR, JAM2, 31, 3, null, "8 NoLace", null
};

struct IntuiText sgtscr4L = {
    TEXTCOLOR, FILLCOLOR, JAM2, 31, 3, null, "4  Lace ", null
};

struct IntuiText sgtscr4 = {
    TEXTCOLOR, FILLCOLOR, JAM2, 31, 3, null, "4 NoLace", null
};

struct IntuiText *(sgtscreen[5]) = {
    &sgtscr4, &sgtscr4L, &sgtscr8, &sgtscr8L, &sgtscrC
};

struct Gadget sgagscrcycle = {
    &sgagmode, 206, 130, 106, 12, GFLG_GADGHCOMP, GACT_RELVERIFY,
    GTYP_BOOLGADGET, &upcyclebor, null, null, 0, null, 1803, null
};


ubyte fontsize[14];

struct StringInfo sstrfsize = STRINF(fontsize, 3);

struct IntuiText slabelfsize = {
    LABELCOLOR, 0, JAM1, -68, 0, null, "Height:", null
};

struct Gadget sgagfsize = {
    &sgagscrcycle, 502, 23, 24, 8, GFLG_STRINGEXTEND | GFLG_TABCYCLE,
    GACT_RELVERIFY | GACT_STRINGLEFT | GACT_LONGINT, GTYP_STRGADGET,
    &yfsizebox, null, &slabelfsize, 0, &sstrfsize, 1802, null
};


ubyte newfontname[PATHLEN + 7], unfont[PATHLEN + 5];

struct StringInfo sstrfname = STRINF(newfontname, PATHLEN);

struct IntuiText slabelfname = {
    LABELCOLOR, 0, JAM1, -52, 0, null, "Name:", null
};

STRINGBORDER(sfnamebox)

struct Gadget sgagfname = {
    &sgagfsize, 166, 23, 248, 8, GFLG_STRINGEXTEND | GFLG_TABCYCLE,
    GACT_RELVERIFY | GACT_STRINGLEFT, GTYP_STRGADGET,
    &sfnamebox, null, &slabelfname, 0, &sstrfname, 1801, null
};


struct IntuiText slabelfontreq = {
    TEXTCOLOR, FILLCOLOR, JAM2, 12, 2, null, "Font:", null
};

struct Gadget sgagfontreq = {
    &sgagfname, 16, 21, 80, 13, GFLG_GADGHCOMP,
    GACT_RELVERIFY, GTYP_BOOLGADGET,
    &upborder, null, &slabelfontreq, 0, null, 1800, null
};


struct ExtNewWindow sneww = {
    46, 17, SWINWIDTH, SWINHEIGHT, 0, 1,
    IDCMP_GADGETDOWN | IDCMP_GADGETUP | IDCMP_MOUSEMOVE | IDCMP_MOUSEBUTTONS
		| IDCMP_CLOSEWINDOW | IDCMP_RAWKEY,
    WFLG_SMART_REFRESH | WFLG_CLOSEGADGET | WFLG_DEPTHGADGET
		| WFLG_DRAGBAR | WFLG_ACTIVATE | WFLG_NW_EXTENDED,
    null, null, "Font and screen specifications",
    null, null, 0, 0, 0, 0, CUSTOMSCREEN, null
};

struct WhereWin sww = { &sneww };

struct Window *swin;
struct RastPort *srp;

struct WhereWin fontfakww = { null }, modefakww = { null };
struct WhereWin modepropfakww = { null };


/* This array is maintained even if we don't support the feature: */
str functionkeys[20];

#ifdef FUNCTION_KEYS

local ushort fkeyshift;

STRINGBORDER(fkbox)

struct StringInfo fkstr[10] = {
    STRINF(functionkeys[0], COMMANDLEN),
    STRINF(functionkeys[1], COMMANDLEN),
    STRINF(functionkeys[2], COMMANDLEN),
    STRINF(functionkeys[3], COMMANDLEN),
    STRINF(functionkeys[4], COMMANDLEN),
    STRINF(functionkeys[5], COMMANDLEN),
    STRINF(functionkeys[6], COMMANDLEN),
    STRINF(functionkeys[7], COMMANDLEN),
    STRINF(functionkeys[8], COMMANDLEN),
    STRINF(functionkeys[9], COMMANDLEN)
};

struct Gadget fkgag[10] = {
    {
	&fkgag[1], 47, 24, 248, 8, GFLG_STRINGEXTEND | GFLG_TABCYCLE,
	GACT_RELVERIFY | GACT_STRINGLEFT, GTYP_STRGADGET,
	&fkbox, null, null, 0, &fkstr[0], 2200, null
    }, {
	&fkgag[2], 47, 45, 248, 8, GFLG_STRINGEXTEND | GFLG_TABCYCLE,
	GACT_RELVERIFY | GACT_STRINGLEFT, GTYP_STRGADGET,
	&fkbox, null, null, 0, &fkstr[1], 2201, null
    }, {
	&fkgag[3], 47, 66, 248, 8, GFLG_STRINGEXTEND | GFLG_TABCYCLE,
	GACT_RELVERIFY | GACT_STRINGLEFT, GTYP_STRGADGET,
	&fkbox, null, null, 0, &fkstr[2], 2202, null
    }, {
	&fkgag[4], 47, 87, 248, 8, GFLG_STRINGEXTEND | GFLG_TABCYCLE,
	GACT_RELVERIFY | GACT_STRINGLEFT, GTYP_STRGADGET,
	&fkbox, null, null, 0, &fkstr[3], 2203, null
    }, {
	&fkgag[5], 47, 108, 248, 8, GFLG_STRINGEXTEND | GFLG_TABCYCLE,
	GACT_RELVERIFY | GACT_STRINGLEFT, GTYP_STRGADGET,
	&fkbox, null, null, 0, &fkstr[4], 2204, null
    }, {
	&fkgag[6], 356, 24, 248, 8, GFLG_STRINGEXTEND | GFLG_TABCYCLE,
	GACT_RELVERIFY | GACT_STRINGLEFT, GTYP_STRGADGET,
	&fkbox, null, null, 0, &fkstr[5], 2205, null
    }, {
	&fkgag[7], 356, 45, 248, 8, GFLG_STRINGEXTEND | GFLG_TABCYCLE,
	GACT_RELVERIFY | GACT_STRINGLEFT, GTYP_STRGADGET,
	&fkbox, null, null, 0, &fkstr[6], 2206, null
    }, {
	&fkgag[8], 356, 66, 248, 8, GFLG_STRINGEXTEND | GFLG_TABCYCLE,
	GACT_RELVERIFY | GACT_STRINGLEFT, GTYP_STRGADGET,
	&fkbox, null, null, 0, &fkstr[7], 2207, null
    }, {
	&fkgag[9], 356, 87, 248, 8, GFLG_STRINGEXTEND | GFLG_TABCYCLE,
	GACT_RELVERIFY | GACT_STRINGLEFT, GTYP_STRGADGET,
	&fkbox, null, null, 0, &fkstr[8], 2208, null
    }, {
	null,      356, 108, 248, 8, GFLG_STRINGEXTEND | GFLG_TABCYCLE,
	GACT_RELVERIFY | GACT_STRINGLEFT, GTYP_STRGADGET,
	&fkbox, null, null, 0, &fkstr[9], 2209, null
    }
};


struct IntuiText fkshiftshift = {
    TEXTCOLOR, FILLCOLOR, JAM2, 31, 3, null, " Shift- ", null
};

struct IntuiText fkshiftnone = {
    TEXTCOLOR, FILLCOLOR, JAM2, 31, 3, null, "  None  ", null
};

struct Gadget fkgagshift = {
    &fkgag[0], 273, 130, 106, 12, GFLG_GADGHCOMP, GACT_RELVERIFY,
    GTYP_BOOLGADGET, &upcyclebor, null, null, 0, null, 2222, null
};


struct ExtNewWindow fkneww = {
    8, 20, 624, 120, 0, 1,
    IDCMP_GADGETUP | IDCMP_CLOSEWINDOW | IDCMP_RAWKEY,
    WFLG_SMART_REFRESH | WFLG_CLOSEGADGET | WFLG_DEPTHGADGET
		| WFLG_DRAGBAR | WFLG_ACTIVATE | WFLG_NW_EXTENDED,
    null, null, "Commands executed by function keys",
    null, null, 0, 0, 0, 0, CUSTOMSCREEN, null
};

struct WhereWin fkww = { &fkneww };

struct Window *fkwin;

#endif FUNCTION_KEYS


void EdgeAColor(short which, short what)
{
    long top = sgagpalette.TopEdge + 2 + slothi * (which >= slotsplit);
    long bot = top + slothi - 1;
    long left = sgagpalette.LeftEdge + (which & (slotsplit - 1)) * slotwid + 4;
    long right = left + slotwid - 1;

    SetAPen(srp, what);
    Move(srp, left, top);
    Draw(srp, right, top);
    Draw(srp, right, bot);
    Draw(srp, left, bot);
    Draw(srp, left, top);
}


void ShowColorNumbers(void)
{
    struct Gadget *gg;
    char foo[4];
    short l;

    SetAPen(srp, TEXTCOLOR);
    SetBPen(srp, backcolor);
    SetDrMd(srp, JAM2);
    for (gg = &sgagred; gg != sgagblue.NextGadget; gg = gg->NextGadget) {
	l = ((struct PropInfo *) gg->SpecialInfo)->HorizPot >> 12;
	Move(srp, gg->LeftEdge + gg->Width + fontwid + 2,
				gg->TopEdge + font->tf_Baseline + 1);
	sprintf(foo, "%2ld", (long) l);
	Text(srp, foo, 2);
    }
}


void FreshenSliders(void)
{
    short r, g, b, p;
    r = GetRGB4(scr->ViewPort.ColorMap, slidecolor);
    b = r & 0xF;
    r >>= 4;
    g = r & 0xF;
    r >>= 4;
    p = RemoveGList(swin, &sgagred, 3);
    spropsred.HorizPot = r * CPOTSTEP;		/* easier than NewModifyProp */
    spropsgreen.HorizPot = g * CPOTSTEP;
    spropsblue.HorizPot = b * CPOTSTEP;
    AddGList(swin, &sgagred, p, 3, null);
    RefreshGList(&sgagred, swin, null, 3);
}


void ResetPalette(void)
{
    LoadRGB4(&scr->ViewPort, paltouse, palettones);
    FreshenSliders();
    ShowColorNumbers();
}


void DrawPalette(void)
{
    short edgc = 1, unedgc = 7;

    if (lightbg) edgc = 6;
    EdgeAColor(lastslidecolor,
			lastslidecolor == backcolor ? shadowcolor : backcolor);
    EdgeAColor(slidecolor, slidecolor == edgc ? unedgc : edgc);
    SetAPen(srp, lastslidecolor = slidecolor);
    RectFill(srp, fakegagswatch.LeftEdge + 4, fakegagswatch.TopEdge + 2,
			fakegagswatch.LeftEdge + fakegagswatch.Width - 5,
			fakegagswatch.TopEdge + fakegagswatch.Height - 3);
    FreshenSliders();
    ShowColorNumbers();
}


void TrackColors(void)
{
    short r, g, b;
    r = spropsred.HorizPot >> 12;
    g = spropsgreen.HorizPot >> 12;
    b = spropsblue.HorizPot >> 12;
    SetRGB4(&scr->ViewPort, slidecolor, r, g, b);
    ShowColorNumbers();
}


#ifdef TEST13

ulong GetDisplayInfoData(DisplayInfoHandle a, ubyte *b,
			 ulong c, ulong d, ulong e)     { return 0; }

#endif


void DisplayModeName(void)
{
    struct NameInfo nii;
    struct DimensionInfo dii;
    char name[DISPLAYNAMELEN + 18];
    long left = sgagmode.LeftEdge + XActual(96), top = sgagmode.TopEdge + 2;

    if (!GetDisplayInfoData(null, (adr) &nii,
				sizeof(nii), DTAG_NAME, modeID))
	sprintf(name, " ** UNKNOWN MODE %08lx **                       ",
			modeID);	/* ^^  use "lX" with c.lib sprintf */
    else {
	strcpy(name, nii.Name);
	PadRight(name, DISPLAYNAMELEN - 1);
	if (!GetDisplayInfoData(null, (adr) &dii,
				sizeof(dii), DTAG_DIMS, modeID))
	    strcpy(name + DISPLAYNAMELEN - 1, "                  ");
	else
	    sprintf(name + DISPLAYNAMELEN - 1, " (%ld x %ld x %ld)    ",
				1L + dii.Nominal.MaxX - dii.Nominal.MinX,
				1L + dii.Nominal.MaxY - dii.Nominal.MinY,
				custom4color ? 4L : 8L);
    }
    SetAPen(srp, TEXTCOLOR);
    SetBPen(srp, backcolor);
    Move(srp, left, top + font->tf_Baseline);
    Text(srp, name, DISPLAYNAMELEN + 17);
    if (!(lastmodeghost = latercustomode)) {
	SetAPen(srp, backcolor);
	SetAfPt(srp, ghostpat, 1);
	SetDrMd(srp, JAM1);
	RectFill(srp, left, top,
		left + (DISPLAYNAMELEN + 17) * fontwid - 1, top + fight - 1);
	SetAfPt(srp, null, 0);
	SetDrMd(srp, JAM2);
    }
}


void InitASLfake(struct WhereWin *fakww,
		 ushort left, ushort top, ushort width, ushort height)
{
    if (!fakww->moved || fakww->scrheight != scr->Height) {
	fakww->left = left;
	fakww->top = top;
	if (fakww != &modepropfakww) {
	    fakww->width = width;
	    fakww->height = height;
	}
	fakww->scrheight = scr->Height;
	fakww->moved = true;
    }
    FitHeight(fakww);
}


void DoScreenModeReq(void)
{
    struct ScreenModeRequester *srq;
    ushort odpen = scr->DetailPen, obpen = scr->BlockPen;
    ustr obgwt = bgwin->ScreenTitle;
    bool srret;

    if (!latercustomode)
	return;
    if (!(AslBase = OpenLibrary("asl.library", 38))) {
	Err("No screen mode requester available; cannot\n"
			"open asl.library version 38 or newer.");
	return;
    }
    InitASLfake(&modefakww, 30, 20 << lace, 38 * fontwid + 14,
						60 + scr->Height / 2);
/* Shit, nobody DOCUMENTS that it uses these left/top numbers relative */
/* to the position of the other window!  Width/height numbers ignored. */
    InitASLfake(&modepropfakww, modefakww.width + 12, 2 * fight, 0, 0);
    if (!(srq = AllocAslRequestTags(ASL_ScreenModeRequest, ASLSM_Window, swin,
			ASLSM_TitleText, "Custom mode for screen",
			ASLSM_InitialLeftEdge, (long) modefakww.left,
			ASLSM_InitialTopEdge, (long) modefakww.top,
			ASLSM_InitialWidth, (long) modefakww.width,
			ASLSM_InitialHeight, (long) modefakww.height,
			ASLSM_InitialInfoLeftEdge, (long) modepropfakww.left,
			ASLSM_InitialInfoTopEdge, (long) modepropfakww.top,
			ASLSM_InitialDisplayID, modeID, ASLSM_DoDepth,
			(long) TRUE, ASLSM_MinDepth, 2L, ASLSM_MaxDepth, 3L,
			ASLSM_PropertyFlags, (long) DIPF_IS_WB,
			ASLSM_PropertyMask, (long) DIPF_IS_WB,
			ASLSM_DoAutoScroll, (long) TRUE,
			ASLSM_SleepWindow, (long) TRUE,
			ASLSM_InitialDisplayDepth, 3L - custom4color,
			ASLSM_InitialAutoScroll, (long) customautoscroll,
			ASLSM_InitialInfoOpened, (long) customshowprops,
			TAG_DONE))) {
	Err("Could not open screen mode requester.");
	return;
    }
/*    srq->sm_InfoLeftEdge = srq->sm_InfoTopEdge = -1; */
    SetAllSharedWindowTitles(title);
    scr->DetailPen = 0;
    scr->BlockPen = 1;
    srret = AslRequest(srq, (adr) &endtag);
/* impossible to tell if it failed to open at all... */
    scr->DetailPen = odpen;
    scr->BlockPen = obpen;
    SetAllSharedWindowTitles(obgwt);
    UpdateClock();
    if (srret) {
	modeID = srq->sm_DisplayID;
	custom4color = srq->sm_DisplayDepth <= 2;
	customautoscroll = !!srq->sm_AutoScroll;
	modefakww.left = srq->sm_LeftEdge;
	modefakww.top = srq->sm_TopEdge;
	modefakww.width = srq->sm_Width;
	modefakww.height = srq->sm_Height;
	if ((customshowprops = !!srq->sm_InfoOpened)
					|| srq->sm_InfoLeftEdge >= 0) {
	    modepropfakww.left = srq->sm_InfoLeftEdge;
	    modepropfakww.top = srq->sm_InfoTopEdge;
	    modepropfakww.width = srq->sm_InfoWidth;
	    modepropfakww.height = srq->sm_InfoHeight;
	}
	DisplayModeName();
    }
    FreeAslRequest(srq);
    CloseLibrary(AslBase);
    AslBase = null;
}


void ShowFontWidths(void)
{
    char foo[80];
    long l;

    sprintf(foo, " Font width = %ld pixels; screen width needed = %ld ",
				(long) laterfontwid, 80L * laterfontwid);
    l = strlen(foo);
    MoveNominal(srp, SWINWIDTH / 2 - 4 * l, sgagfontreq.TopEdge
				+ sgagfontreq.Height + font->tf_Baseline + 6);
    SetAPen(srp, TEXTCOLOR);
    SetBPen(srp, backcolor);
    Text(srp, foo, l);
}


bool DoNewHeight(str name, ushort size, bool exiting)
{
    struct TextFont *ff;
    struct TextAttr fa;
    bool ok = false;

    if (size < 8 || size > 32) {
	sstrfsize.LongInt = taheight;
	utoa((ushort) sstrfsize.LongInt, fontsize);
	RefreshGList(&sgagfsize, swin, null, 1);
	ActivateGadget(&sgagfsize, swin, null);
	reopen = false;
    } else if (name[0]) {
	fa.ta_Name = name;
	fa.ta_YSize = size;
	fa.ta_Style = 0;
	if (ff = FindFont(&fa, 9999)) {
	    taheight = size;	    /* FindFont ensures ff->tf_YSize == size */
	    laterfontwid = ff->tf_XSize;
	    CloseFont(ff);
	    ok = true;
	}
	if (!ok) {
	    if (foundbutbad)
		Err("%s size %ld is not usable by Q-Blue.\n"
			    "Fonts must be fixed width and nonscaled, from\n"
			    "8 to 32 points tall, and 5 to 16 points wide.",
			    name, (long) size);
	    else
		Err("%s size %ld not found.", name, (long) size);
	    /* be lenient, allow the window to close... */
	    if (exiting && !reopen) {
		ok = true;
		taheight = size;
	    }
	    reopen = false;
	} else if (!exiting)
	    ShowFontWidths();
    } else
	ok = true;	/* OpenScreen will use the default font */
    return ok;
}


long MyFontFilter(ULONG Mask, APTR Object, struct FontRequester *AslReq)
{
    register struct TextAttr *tt = Object;
    return (tt->ta_Flags & /* FPF_DESIGNED */ (FPF_DISKFONT | FPF_ROMFONT))
				&& tt->ta_YSize >= 8 && tt->ta_YSize <= 32;
}



void DoFontReq(void)
{
    struct FontRequester *frq;
    ushort odpen = scr->DetailPen, obpen = scr->BlockPen, hi;
    ustr obgwt = bgwin->ScreenTitle;
    bool frret;

    if (!(AslBase = OpenL("asl"))) {
	Err("Cannot open asl.library for font\nrequester.  Select a font by\n"
				    "editing the string gadgets.");
	return;
    }
    InitASLfake(&fontfakww, 30, 20 << lace, 38 * fontwid + 14,
						    60 + scr->Height / 2);
    if (!(frq = AllocAslRequestTags(ASL_FontRequest, ASLFO_Window, swin,
			ASLFO_InitialLeftEdge, (long) fontfakww.left,
			ASLFO_InitialTopEdge, (long) fontfakww.top,
			ASLFO_InitialWidth, (long) fontfakww.width,
			ASLFO_InitialHeight, (long) fontfakww.height,
			ASLFO_TitleText, "Select font for screen",
			ASLFO_InitialSize, (long) taheight,
			ASLFO_InitialName, newfontname,
			ASLFO_InitialFlags, 0L,
			ASLFO_SleepWindow, (long) TRUE,
			ASLFO_Flags, FOF_FILTERFUNC | FOF_FIXEDWIDTHONLY,
			ASLFO_HookFunc, MyFontFilter,
			ASLFO_MinHeight, 8L, ASLFO_MaxHeight, 32L, TAG_DONE))) {
	Err("Could not open font requester.\n"
			"Select a font by editing\nthe string gadgets.");
	return;
    }
    SetAllSharedWindowTitles(title);
    scr->DetailPen = 0;
    scr->BlockPen = 1;
    frret = AslRequest(frq, (adr) &endtag);
/* impossible to tell if it failed to open at all... */
    scr->DetailPen = odpen;
    scr->BlockPen = obpen;
    SetAllSharedWindowTitles(obgwt);
    UpdateClock();
    hi = frq->fo_Attr.ta_YSize;
    if (frret && DoNewHeight(frq->fo_Attr.ta_Name, hi, false)) {
	if (AslBase->lib_Version > 37) {
	    fontfakww.left = frq->fo_LeftEdge;
	    fontfakww.top = frq->fo_TopEdge;
	    fontfakww.width = frq->fo_Width;
	    fontfakww.height = frq->fo_Height;
	}
	if (strlen(frq->fo_Attr.ta_Name) >= PATHLEN)
	    strcpy(newfontname, FilePart(frq->fo_Attr.ta_Name));
	else
	    strcpy(newfontname, frq->fo_Attr.ta_Name);
	sstrfsize.LongInt = hi;
	utoa(hi, fontsize);
	RefreshGList(&sgagfname, swin, null, 2);
    }
    FreeAslRequest(frq);
    CloseLibrary(AslBase);
    AslBase = null;
}


void FreshScreenGag(short increment)
{
    short nowsetting = latercustomode ? 4 : (!laterfourcolors * 2) + laterlace;
    short whichdisplay = (nowsetting + increment + 5) % 5;

    ChangeGagText(&sgagscrcycle, swin, sgtscreen[whichdisplay]);
    RefreshGList(&sgagscrcycle, swin, null, 1);
    if (latercustomode = whichdisplay == 4)
	laterlace = laterfourcolors = false;
    else
	laterlace = whichdisplay & 1, laterfourcolors = whichdisplay < 2;
    if (latercustomode != lastmodeghost) {
	AbleAddGad(&sgagmode, swin, latercustomode);
	DisplayModeName();
    }
}


bool AppendDotFont(str name)
{
    if (name[0]) {
	if (!strchr(name, '.')) {
	    short l = strlen(name);
	    strcpy(name + l, ".font");
	    if (l >= PATHLEN - 5) {
		name[PATHLEN - 1] = 0;
		return false;
	    }
	}
    }
    return true;
}


bool DoFontScreenIDCMP(struct IntuiMessage *im)
{
    struct PropInfo *sp;
    short inc, x, y;
    ubyte k;
    ushort shifty = im->Qualifier & SHIFTKEYS;
    bool closure = false;

    switch (im->Class) {
      case IDCMP_RAWKEY:
	if (im->Qualifier & ALTKEYS)
	    break;
	switch (k = KeyToAscii(im->Code, im->Qualifier)) {
	  case ESC:
	    closure = true;
	    break;
	  case '\t':
	    ActivateGag(&sgagfname, swin);
	    break;
	  case 'S':
	    FreshScreenGag(shifty ? -1 : 1);
	    break;
	  case 'F':
	    DoFontReq();
	    break;
	  case 'M':
	    DoScreenModeReq();
	    break;
	  case 'R':
	  case 'G':
	  case 'B':
	    sp = (k == 'R' ? &spropsred : (k == 'G' ? &spropsgreen
							: &spropsblue));
	    inc = (shifty ? -CPOTSTEP : CPOTSTEP);
	    if (((long) sp->HorizPot + inc) & 0xFFFF0000)
		break;
	    sp->HorizPot += inc;
	    TrackColors();
	    FreshenSliders();
	    break;
	  case 'U':
	    ResetPalette();
	    break;
	  case 'O':
	    if (!(sgagopen.Flags & GFLG_DISABLED)) {
		reopen = closure = true;
	    }
	    break;
	  case 0xCF: case 0xAD:			 	/* left */
	    if (slidecolor > 0) {
		slidecolor--;
		DrawPalette();
	    }
	    break;
	  case 0xCE: case 0xAF:				/* right */
	    if (slidecolor < palettones - 1) {
		slidecolor++;
		DrawPalette();
	    }
	    break;
	  case 0xCC: case 0xBE:			 	/* up */
	    if (slidecolor >= slotsplit) {
		slidecolor -= slotsplit;
		DrawPalette();
	    }
	    break;
	  case 0xCD: case 0x9E:				/* down */
	    if (slidecolor < slotsplit) {
		slidecolor += slotsplit;
		DrawPalette();
	    }
	    break;
	}
	break;
      case IDCMP_GADGETDOWN:
      case IDCMP_GADGETUP:
	switch (((struct Gadget *) im->IAddress)->GadgetID) {
	  case 1800:					/* font: */
	    DoFontReq();
	    break;
	  case 1801:					/* font name */
	    if (!GAGFINISH(im))
		break;
	    strcpy(unfont, newfontname);
	    StripString(&sgagfname, swin);
	    if (!AppendDotFont(newfontname)) {
		DisplayBeep(scr);
		sstrfname.BufferPos = PATHLEN;
		sstrfname.DispPos = PATHLEN - 28;
		ActivateGadget(&sgagfname, swin, null);
	    } else if (strcmp(newfontname, unfont))
		RefreshGList(&sgagfname, swin, null, 1);
	    if (newfontname[0] && GAGCHAIN(im))
		ActivateGadget(&sgagfsize, swin, null);
	    break;
	  case 1802:				    /* font height */
	    if (GAGFINISH(im) && DoNewHeight(newfontname, sstrfsize.LongInt,
					false) && shifty && GAGCHAIN(im))
		ActivateGadget(&sgagfname, swin, null);
	    break;
	  case 1803:					/* screen */
	    FreshScreenGag(shifty ? -1 : 1);
	    break;
	  case 1804:					/* mode */
	    DoScreenModeReq();
	    break;
	  case 1805:					/* palette */
	    x = (im->MouseX - sgagpalette.LeftEdge - 4) / slotwid;
	    if (x < 0)
		x = 0;
	    else if (x >= slotsplit)
		x = slotsplit - 1;
	    y = (im->MouseY - sgagpalette.TopEdge - 2) >= slothi;
	    slidecolor = x + slotsplit * y;
	    DrawPalette();
	    break;
	  case 1806: case 1807: case 1808:		/* R/G/B */
	    TrackColors();
	    FreshenSliders();				/* snap to best spot */
	    break;
	  case 1809:
	    ResetPalette();
	    break;
	  case 1810:					/* Open! */
	    reopen = closure = true;
	    break;
		
	}
      case IDCMP_MOUSEMOVE:				/* track a slider */
	TrackColors();
	break;
      case IDCMP_CLOSEWINDOW:
	closure = im->IDCMPWindow == swin;
	break;
    }
    if (!closure)
	return false;
    StripString(&sgagfname, swin);	/* VV clears reopen if fails */
    return DoNewHeight(newfontname, (ushort) sstrfsize.LongInt, true);
}


void DrawColorStuff(void)
{
    struct Border *batl, *batr;
    short foo;
    long left, top, right, bot;

    sgagred.LeftEdge = sgaggreen.LeftEdge = sgagblue.LeftEdge = XActual(290);
    sgagpalette.LeftEdge = sgagmode.LeftEdge;
    sgagpalette.TopEdge = fakegagswatch.TopEdge = sgagred.TopEdge - 1;
    sgagpalette.Width = slotwid * slotsplit + 8;
    sgagpalette.Height = 2 * slothi + 4;
    fakegagswatch.LeftEdge = sgagundo.LeftEdge;
    fakegagswatch.Width = sgagundo.Width;
    fakegagswatch.Height = sgagblue.TopEdge + sgagblue.Height
				- fakegagswatch.TopEdge - fight - 13;
    FixShine(&sgagpalette);
    batl = sgagred.GadgetRender;
    sgagred.GadgetRender = &sslidebor;
    FixShine(&sgagred);
    sgagred.GadgetRender = batl;
    FixShine(&fakegagswatch);
    foo = upborderleft.FrontPen;
    batl = upborderleft.NextBorder;
    batr = upborderright.NextBorder;
    upborderleft.NextBorder = upborderright.NextBorder = &underborder;
    upborderleft.FrontPen = upborderright.FrontPen = LABELCOLOR;
    top = sgagpalette.TopEdge + sgagpalette.Height;
    DrawBorder(srp, &upborderleft, sgagpalette.LeftEdge
				+ 3 - upborderleft.LeftEdge, top);
    DrawBorder(srp, &upborderright, sgagpalette.LeftEdge + sgagpalette.Width
				- 36 - upborderright.LeftEdge, top);
    upborderleft.FrontPen = upborderright.FrontPen = foo;
    upborderleft.NextBorder = batl;
    upborderright.NextBorder = batr;
    DrawBorder(srp, &sswatchborder,
			fakegagswatch.LeftEdge, fakegagswatch.TopEdge);
    DrawBorder(srp, &sslidebor, sgagred.LeftEdge, sgagred.TopEdge);
    DrawBorder(srp, &sslidebor, sgaggreen.LeftEdge, sgaggreen.TopEdge);
    DrawBorder(srp, &sslidebor, sgagblue.LeftEdge, sgagblue.TopEdge);
    left = sgagpalette.LeftEdge + 5;
    top = sgagpalette.TopEdge + 3;
    bot = top + slothi - 3;
    for (foo = 0; foo < palettones; foo++) {
	if (foo == slotsplit)
	    top += slothi, bot += slothi, left = sgagpalette.LeftEdge + 5;
	right = left + slotwid - 3;
	SetAPen(srp, foo);
	RectFill(srp, left, top, right, bot);
	left += slotwid;
    }
    EdgeAColor(backcolor, shadowcolor);
    left = sgagred.LeftEdge - XActual(56);
    UnderstartedText(left, sgagred.TopEdge + 1, srp, "Red:");
    UnderstartedText(left, sgaggreen.TopEdge + 1, srp, "Green:");
    UnderstartedText(left, sgagblue.TopEdge + 1, srp, "Blue:");
    UnderstartedText(XActual(16), sgagscrcycle.TopEdge + 3, srp,
					"Screen colors and type:");
}


void ConfigFontScreen(void)
{
    short i;

    if (!newfontname[0])
	taheight = fight;
    sstrfsize.LongInt = taheight;
    utoa(taheight, fontsize);
    palettones = 8 >> fourcolors;
    slotwid = XActual(24) << fourcolors;
    slothi = fakefight + 8;
    slotsplit = palettones / 2;
    lastslidecolor = slidecolor = 0;
#ifdef SOMEDAY
    paltouse = lightbg ? (fourcolors ? lb_palt4 : lb_palt)
			: (fourcolors ? palette4 : palette);
#else
    paltouse = fourcolors ? palette4 : palette;
#endif
    lastmodeghost = -1;
    reopen = false;

    sgagfname.TopEdge = sgagfsize.TopEdge = fakefight + 12;
    sgagfontreq.TopEdge = sgagfname.TopEdge - 2;
    sgagscrcycle.TopEdge = sgagopen.TopEdge
			= sgagfontreq.TopEdge + 2 * fakefight + 17;
    sgagmode.TopEdge = sgagscrcycle.TopEdge + fakefight + 11;
    /****** compensate for these gadgets VVVVVVV not linking in till later! */
    sgagred.TopEdge = sgagmode.TopEdge + tifight + (lace ? 18 : 16);
    sgagred.Height = sgaggreen.Height = sgagblue.Height = (fakefight + 2) & ~1;
    sgaggreen.TopEdge = sgagred.TopEdge + sgagred.Height + 8;
    sgagblue.TopEdge = sgaggreen.TopEdge + sgaggreen.Height + 8;
    sgagundo.TopEdge = sgagblue.TopEdge + sgagblue.Height - fight - 4;
    sgagred.Width = sgaggreen.Width = sgagblue.Width = XActual(204) + 8;
    sneww.Height = sgagblue.TopEdge + sgagblue.Height + 9 + fakefight - tifight;
    /****** compensate for the fudging of slider topedges   ^^^^^^^^^^^^^^^^^ */
    sneww.TopEdge = (scr->Height - sneww.Height) / 2;
    /* window height for fakefight = 11: 170 (172 for lace) */
    AbleGad(&sgagmode, latercustomode);
    AbleGad(&sgagopen, !AnySharedPresent());
    if (!(swin = OpenBlueGagWin(&sww, &sgagfontreq))) {
	WindowErr("font and screen settings.");
	return;
    }

    srp = swin->RPort;
    SetDrMd(srp, JAM2);		/* just to make sure */
    FreshScreenGag(0);
    DrawColorStuff();
    ShowFontWidths();
    AddGList(swin, &sgagpalette, 0, 4, null);
    RefreshGList(&sgagpalette, swin, null, 1);
    DrawPalette();
    GhostCompose(true);
    EventLoop(&DoFontScreenIDCMP);
    for (i = 0; i < palettones; i++)
	paltouse[i] = GetRGB4(scr->ViewPort.ColorMap, i);
    RemoveGList(swin, &sgagpalette, 4);
    CloseBlueGagWin(swin);
    swin = null;
    GhostCompose(false);
    if (reopen)
	Iconify(true);
}


/* ========================================================================== */

#ifdef FUNCTION_KEYS


/* Function key codes: @N = newline, @F = current message in basic text form, */
/* @A = current message in raw form, i.e. no stripping of ANSI, @V = current  */
/* message quoted in "Add >" form, @W = msg quoted in "Wrap XX>" form, @X =   */
/* msg quoted in "Add XX>" form, @O = quotee file that will be appended to    */
/* when you write a reply.  Support @P/@S and @B/@Q as in other commands.     */

bool DoFunctionKeyCommand(ubyte k)
{
    struct Conf *cc = readareaz.confs[whicha];
    struct Mess *mm = cc->messes[whichm];
    bool rep = cc == &replies;
    char kname[4];

    if (k < 0xD0 || k > 0xD9)      /* what KeyToAscii turns F-keys into */
	return false;
    k = k - 0xD0 + 10 * lastkeywasshifted;
    if (!functionkeys[k][0])
	return true;
Err("I would do this command except I don't know\n"
"how to run function key commands yet:\n%s", functionkeys[k]);
    sprintf(kname, ".%lc%ld", lastkeywasshifted ? 'S' : 'F', k % 10 + 1);
    if (!Substitutions(xcommand, kname, "", functionkeys[k], true, true, rep))
	return true;	/* Substitutions() gives error message */
    
    return true;
}


void ToggleFKQualifier(short increment)
{
    register short i, p;
    if (increment)		/* <<< fix this if Alt- or Ctrl- added  VVV */
	fkeyshift = !fkeyshift;
    if (fkwin) {
	ChangeGagText(&fkgagshift, fkwin,
				fkeyshift ? &fkshiftshift : &fkshiftnone);
	RefreshGList(&fkgagshift, fkwin, null, 1);
    }
    if (increment) {
	if (fkwin)
	    p = RemoveGList(fkwin, &fkgag[0], 10);
	for (i = 0; i < 10; i++)
	    fkstr[i].Buffer = &functionkeys[i + 10 * fkeyshift][0];
	if (fkwin) {
	    AddGList(fkwin, &fkgag[0], p, 10, null);
	    RefreshGList(&fkgag[0], fkwin, null, 10);
	}
    }
}


bool ConfigFKeysIDCMP(struct IntuiMessage *im)
{
    if (im->Class == IDCMP_RAWKEY && !(im->Qualifier & ALTKEYS)) {
	char k = KeyToAscii(im->Code, im->Qualifier);
	if (DoFunctionKeyCommand(k))
	    return false;
	if (k == '\t')
	    ActivateGag(&fkgag[0], fkwin);
	else if (k == ESC)
	    return true;
	else if (k == 'Q')
	    ToggleFKQualifier(1);
    } else if (im->Class == IDCMP_CLOSEWINDOW)
	return true;
    else if (im->Class == IDCMP_GADGETUP) {
	register short i = ((struct Gadget *) im->IAddress)->GadgetID - 2200;
	if (i == 22)
	    ToggleFKQualifier(1);
	else if (GAGFINISH(im)) {
	    StripString(&fkgag[i], fkwin);
	    if (!(im->Qualifier & SHIFTKEYS))
		i++;
	    else if (--i < 0)
		i = 9;
	    if (i < 10 && GAGCHAIN(im))
		ActivateGadget(&fkgag[i], fkwin, null);
	}
    }
    return false;
}


void ConfigFKeys(void)
{
    short i, j = fakefight + 12, k = 0;
    struct RastPort *fkrp;
    static char fklab[10][5] = {
	" F1:", " F2:", " F3:", " F4:", " F5:",
	" F6:", " F7:", " F8:", " F9:", "F10:"
    };

    fkeyshift = 0;
    for (i = 0; i < 10; i++) {
	fkstr[i].Buffer = &functionkeys[i][0];
	StripString(&fkgag[i], null);
    }
    for (i = 0; i < 5; i++)
	fkgag[i].TopEdge = fkgag[i + 5].TopEdge = (k += j);
    fkgagshift.TopEdge = (k += j) - 3;
    fkneww.Height = k + j - 1;
    /* height when fakefight = 11: 157 */
    if (!(fkwin = OpenBlueGagWin(&fkww, &fkgagshift))) {
	WindowErr("setting function key commands.");
	return;
    }
    fkrp = fkwin->RPort;
    SetAPen(fkrp, LABELCOLOR);
    SetBPen(fkrp, backcolor);
    for (i = 0; i < 10; i++) {
	MoveNominal(fkrp, fkgag[i].LeftEdge - 41,
				fkgag[i].TopEdge + font->tf_Baseline);
	Text(fkwin->RPort, fklab[i], 4);
    }
    UnderstartedText(fkgagshift.LeftEdge - 88,
				fkgagshift.TopEdge + 3, fkrp, "Qualifier:");
    ToggleFKQualifier(0);
    ActivateGag(&fkgag[0], fkwin);
    EventLoop(&ConfigFKeysIDCMP);
    CloseBlueGagWin(fkwin);
    fkwin = null;
    for (i = 0; i < 10; i++)
	StripString(&fkgag[i], null);
    ToggleFKQualifier(1);
    for (i = 0; i < 10; i++)
	StripString(&fkgag[i], null);
}

#endif FUNCTION_KEYS
