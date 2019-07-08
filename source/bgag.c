/* bottom-of-the-screen mouse buttons and main menus for Q-Blue. */

/* unshifted letter shortcuts currently unused:               G H J K M Y Z  */
/* Alt-letter shortcuts currently unused:                 A G H I J N Q X Z  */
/* Ctrl-key shortcuts now in use:  A D F L N S T                             */

/* what keys to use for forthcoming commands?  Should cover:  loading reply  */
/* packet after mail packet is open.  Maybe Alt-A ("add" replies) for that?  */
/* Also commands to follow threads, run ARexx scripts, grouping messages for */
/* combined reply or save/print, and eventually database access.  Maybe use  */
/* more Ctrl-keys...                                                         */

/* Notes on GadTools menu item and division bar spacing: there is one pixel  */
/* of space between each row of text, outside the height of the character    */
/* cell height.  Q-Blue used to use two.  There are two pixels between the   */
/* border and the top of the top item.  Q-Blue also uses two currently.      */
/* This happens when the TopEdge of the first item is zero and the TopEdge   */
/* of its IntuiText is one.  We observe that the Workbench's menu item hit   */
/* box extends one pixel above, rather than below, the text.  There are four */
/* pixels between the border and the left edge.  Q-Blue used to have six.    */
/* This came from specifying a left edge of four on the IntuiText, and zero  */
/* for the MenuItem.  We now use two for the IntuiText.  The dividing bar is */
/* six pixels high, and four in from each end, with the middle two vertical  */
/* pixels being patterned in the ;,;';,;';,;' image.  The text item below it */
/* has the normal one pixel extra at the top.                                */

#include <intuition/intuition.h>
/* #include <libraries/gadtools.h> */
#include "qblue.h"
#include "pigment.h"


#define PACKNAMELEN   8
#define GREENBAY      8

#define TOPBOXCOLOR   3

#define MENUWID       96
#define ITEM1WID      (22 * 8 + 4)
#define ITEMWID       (22 * 8 + 4)
/* ...those two used to be different */
#define CHECKMARKLEFT 21
#define SUBITEMLEFT   (ITEMWID - 66)
#define SORTITEMWID   (19 * 8 + 2 + CHECKMARKLEFT)

#define BGPROPFLAGS (AUTOKNOB | FREEVERT | PROPNEWLOOK /* | PROPBORDERLESS */ )

#define MIT(N, S) struct IntuiText N = { 0, 1, JAM2, 2, 1, null, S, null };


void ScaleGadget(struct Gadget *gg);
void UnScaleGadget(struct Gadget *gg);
void ScaleGText(struct Gadget *gg);
void UnScaleGText(struct Gadget *gg);
short XTiActual(register short x);
void PaintOverPopupScroller(struct Mess *mm);


import struct RastPort *bgrp, bgrpblack;
import struct Border ldownpoint, laltdownpoint, luppoint, laltuppoint;

import char packernames[GREENBAY][PACKNAMELEN];
import ushort *arrowarea;

import ushort requestedfiles_limit, packers, currentpacker, nomwidth;
import ushort tifight, tifontwid;
import short shortexbot, extralines, winlines, shortwinlines, textop, sorder;
import short topline, remembertopline, lastcolor;
import bool menuset, backslide, slidestifle, rethinkmenus, lightbg;
import bool popupscroller;


bool bottached = false, deferflipb = false, obscuringslider = false;

ushort texbot, sliderspace;

local long botedge, newpot, newbod;
long lastbgpot;


MIT(itsup9, "Save setup...    Alt-S");

struct MenuItem misup9 = {
    null, 0, 0, ITEMWID, 10, ITEMTEXT | ITEMENABLED | HIGHCOMP, 0,
    &itsup9, null, 0, null, 0
};

MIT(itsup8, "Load setup...    Alt-L");

struct MenuItem misup8 = {
    &misup9, 0, 0, ITEMWID, 10, ITEMTEXT | ITEMENABLED | HIGHCOMP, 0,
    &itsup8, null, 0, null, 0
};

MIT(itsupZ, " -------------------- ");

struct MenuItem misupZ = {
    &misup8, 0, 0, ITEMWID, 10, ITEMTEXT, 0, &itsupZ, null, 0, null, 0
};

MIT(itsup7, "BBS Local...     Alt-B");

struct MenuItem misup7 = {
    &misupZ, 0, 0, ITEMWID, 10, ITEMTEXT | HIGHCOMP, 0,
    &itsup7, null, 0, null, 0
};

MIT(itsup6, "Options...       Alt-O");

struct MenuItem misup6 = {
    &misup7, 0, 0, ITEMWID, 10, ITEMTEXT | ITEMENABLED | HIGHCOMP, 0,
    &itsup6, null, 0, null, 0
};

#ifdef FUNCTION_KEYS
MIT(itsup5h, "Function keys... Alt-K");

struct MenuItem misup5h = {
    &misup6, 0, 0, ITEMWID, 10, ITEMTEXT | ITEMENABLED | HIGHCOMP, 0,
    &itsup5h, null, 0, null, 0
};


MIT(itsup5, "Font & Screen... Alt-F");

struct MenuItem misup5 = {
    &misup5h, 0, 0, ITEMWID, 10, ITEMTEXT | ITEMENABLED | HIGHCOMP, 0,
    &itsup5, null, 0, null, 0
};
#  define FK_OFF 1
#else
MIT(itsup5, "Font & Screen... Alt-F");

struct MenuItem misup5 = {
    &misup6, 0, 0, ITEMWID, 10, ITEMTEXT | ITEMENABLED | HIGHCOMP, 0,
    &itsup5, null, 0, null, 0
};
#  define FK_OFF 0
#endif FUNCTION_KEYS

MIT(itsup4, "Compressors...   Alt-C");

struct MenuItem misup4 = {
    &misup5, 0, 0, ITEMWID, 10, ITEMTEXT | ITEMENABLED | HIGHCOMP, 0,
    &itsup4, null, 0, null, 0
};

MIT(itsup3, "Replying...      Alt-Y");

struct MenuItem misup3 = {
    &misup4, 0, 0, ITEMWID, 10, ITEMTEXT | ITEMENABLED | HIGHCOMP, 0,
    &itsup3, null, 0, null, 0
};

MIT(itsup2, "Editor...        Alt-E");

struct MenuItem misup2 = {
    &misup3, 0, 0, ITEMWID, 10, ITEMTEXT | ITEMENABLED | HIGHCOMP, 0,
    &itsup2, null, 0, null, 0
};

MIT(itsup1, "Directories...   Alt-D");

struct MenuItem misup1 = {
    &misup2, 0, 0, ITEMWID, 10, ITEMTEXT | ITEMENABLED | HIGHCOMP, 0,
    &itsup1, null, 0, null, 0
};


MIT(itsortlast, "Last name?   Ctrl-L");

struct MenuItem misortlast = {
    null, SUBITEMLEFT, 0, SORTITEMWID, 10,
    ITEMTEXT | CHECKIT | MENUTOGGLE | ITEMENABLED | HIGHCOMP, 0,
    &itsortlast, null, 0, null, 0
};

MIT(itsortZ, " ----------------- ");

struct MenuItem misortZ = {
    &misortlast, SUBITEMLEFT, 0, SORTITEMWID, 10,
    ITEMTEXT | CHECKIT | HIGHNONE, 0, &itsortZ, null, 0, null, 0
};

MIT(itsort5, "by who to    Ctrl-T");

struct MenuItem misort5 = {
    &misortZ, SUBITEMLEFT, 0, SORTITEMWID, 10,
    ITEMTEXT | CHECKIT | ITEMENABLED | HIGHCOMP, 0x3F & ~bit(5),
    &itsort5, null, 0, null, 0
};

MIT(itsort4, "by who from  Ctrl-F");

struct MenuItem misort4 = {
    &misort5, SUBITEMLEFT, 0, SORTITEMWID, 10,
    ITEMTEXT | CHECKIT | ITEMENABLED | HIGHCOMP, 0x3F & ~bit(4),
    &itsort4, null, 0, null, 0
};

MIT(itsort3, "by thread    Ctrl-D");

struct MenuItem misort3 = {
    &misort4, SUBITEMLEFT, 0, SORTITEMWID, 10,
    ITEMTEXT | CHECKIT | ITEMENABLED | HIGHCOMP, 0x3F & ~bit(3),
    &itsort3, null, 0, null, 0
};

MIT(itsort2, "by subject   Ctrl-S");

struct MenuItem misort2 = {
    &misort3, SUBITEMLEFT, 0, SORTITEMWID, 10,
    ITEMTEXT | CHECKIT | ITEMENABLED | HIGHCOMP, 0x3F & ~bit(2),
    &itsort2, null, 0, null, 0
};

MIT(itsort1, "by age       Ctrl-A");

struct MenuItem misort1 = {
    &misort2, SUBITEMLEFT, 0, SORTITEMWID, 10,
    ITEMTEXT | CHECKIT | ITEMENABLED | HIGHCOMP, 0x3F & ~bit(1),
    &itsort1, null, 0, null, 0
};

MIT(itsort0, "by number    Ctrl-N");

struct MenuItem misort0 = {
    &misort1, SUBITEMLEFT, 0, SORTITEMWID, 10,
    ITEMTEXT | CHECKIT | ITEMENABLED | HIGHCOMP, 0x3F & ~bit(0),
    &itsort0, null, 0, null, 0
};

MIT(itsort, "Sort messages        »");

struct MenuItem misort = {
    &misup1, 0, 0, ITEMWID, 10, ITEMTEXT | ITEMENABLED, 0,
    &itsort, null, 0, &misort0, 0
};


struct Menu mainu3 = {
    null, 192, 0, MENUWID, 10, MENUENABLED, "Setup", &misort, 0, 0, 0, 0
};


MIT(itrep8, "Maintain taglines... T");

struct MenuItem mirep8 = {
    null, 0, 0, ITEM1WID, 10, ITEMTEXT | ITEMENABLED | HIGHCOMP, 0,
    &itrep8, null, 0, null, 0
};

MIT(itrep7, "Mail door...     Alt-M");

struct MenuItem mirep7 = {
    &mirep8, 0, 0, ITEM1WID, 10, ITEMTEXT | HIGHCOMP, 0,
    &itrep7, null, 0, null, 0
};

MIT(itrep6, "Request D/L...   Alt-R");

struct MenuItem mirep6 = {
    &mirep7, 0, 0, ITEM1WID, 10, ITEMTEXT | HIGHCOMP, 0,
    &itrep6, null, 0, null, 0
};

MIT(itrep5, "Carbon copy...   Alt-W");

struct MenuItem mirep5 = {
    &mirep6, 0, 0, ITEMWID, 10, ITEMTEXT | ITEMENABLED | HIGHCOMP, 0,
    &itrep5, null, 0, null, 0
};

MIT(itrep4a, "Write new email...   @");

struct MenuItem mirep4a = {
    &mirep5, 0, 0, ITEMWID, 10, ITEMTEXT | ITEMENABLED | HIGHCOMP, 0,
    &itrep4a, null, 0, null, 0
};

MIT(itrep4, "Write new message... W");

struct MenuItem mirep4 = {
    &mirep4a, 0, 0, ITEMWID, 10, ITEMTEXT | ITEMENABLED | HIGHCOMP, 0,
    &itrep4, null, 0, null, 0
};

MIT(itrep3, "Reply to addressee...E");

struct MenuItem mirep3 = {
    &mirep4, 0, 0, ITEMWID, 10, ITEMTEXT | ITEMENABLED | HIGHCOMP, 0,
    &itrep3, null, 0, null, 0
};

MIT(itrep2R, "Re-edit reply...     R");

MIT(itrep2, "Reply to this msg... R");

struct MenuItem mirep2 = {
    &mirep3, 0, 0, ITEMWID, 10, ITEMTEXT | ITEMENABLED | HIGHCOMP, 0,
    &itrep2, null, 0, null, 0
};

MIT(itrep1U, "Un-delete reply    Del");

MIT(itrep1, "Delete reply       Del");

struct MenuItem mirep1 = {
    &mirep2, 0, 0, ITEMWID, 10, ITEMTEXT | HIGHCOMP, 0,
    &itrep1, null, 0, null, 0
};

MIT(itrep0B, "Flip to original     F");

MIT(itrep0, "Flip to reply        F");

struct MenuItem mirep0 = {
    &mirep1, 0, 0, ITEMWID, 10, ITEMTEXT | HIGHCOMP, 0,
    &itrep0, null, 0, null, 0
};


struct Menu mainu2 = {
    &mainu3, 96, 0, MENUWID, 10, 0, "Replies", &mirep0, 0, 0, 0, 0
};


MIT(itmes7, "List area's msgs...  L");

struct MenuItem mimes7 = {
    null, 0, 0, ITEMWID, 10, ITEMTEXT | ITEMENABLED | HIGHCOMP, 0,
    &itmes7, null, 0, null, 0
};

MIT(itmes6, "Areas with msgs...   A");

struct MenuItem mimes6 = {
    &mimes7, 0, 0, ITEMWID, 10, ITEMTEXT | ITEMENABLED | HIGHCOMP, 0,
    &itmes6, null, 0, null, 0
};

/*****
MIT(itmes10, "Go to prev location  G");

struct MenuItem mimes10 = {
    &mimes6, 0, 0, ITEMWID, 10, ITEMTEXT | ITEMENABLED | HIGHCOMP, 0,
    &itmes10, null, 0, null, 0
};
*****/

MIT(itmes9, "Mark as unread       X");

struct MenuItem mimes9 = {
    &mimes6, 0, 0, ITEMWID, 10, ITEMTEXT | ITEMENABLED | HIGHCOMP, 0,
    &itmes9, null, 0, null, 0
};

MIT(itmes8, "Next unread (wraps)  N");

struct MenuItem mimes8 = {
    &mimes9, 0, 0, ITEMWID, 10, ITEMTEXT | ITEMENABLED | HIGHCOMP, 0,
    &itmes8, null, 0, null, 0
};

MIT(itsave3, "Attached file... Alt-T");

struct MenuItem misave3 = {
    null, SUBITEMLEFT, 0, 188, 10, ITEMTEXT | ITEMENABLED | HIGHCOMP, 0,
    &itsave3, null, 0, null, 0
};

MIT(itsave2, "whole area... AltCtl-V");

struct MenuItem misave2 = {
    &misave3, SUBITEMLEFT, 0, 188, 10, ITEMTEXT | ITEMENABLED | HIGHCOMP, 0,
    &itsave2, null, 0, null, 0
};

MIT(itsave1, "append previous  Alt-V");

struct MenuItem misave1 = {
    &misave2, SUBITEMLEFT, 0, 188, 10, ITEMTEXT | HIGHCOMP, 0,
    &itsave1, null, 0, null, 0
};

MIT(itsave0, "ask filename...      V");

struct MenuItem misave0 = {
    &misave1, SUBITEMLEFT, 0, 188, 10, ITEMTEXT | ITEMENABLED | HIGHCOMP, 0,
    &itsave0, null, 0, null, 0
};

MIT(itmes5, "Save msg / attachment »");

struct MenuItem mimes5 = {
    &mimes8, 0, 0, ITEMWID, 10, ITEMTEXT | ITEMENABLED, 0,
    &itmes5, null, 0, &misave0, 0
};

MIT(itmes4, "Print message    Alt-P");

struct MenuItem mimes4 = {
    &mimes5, 0, 0, ITEMWID, 10, ITEMTEXT | ITEMENABLED | HIGHCOMP, 0,
    &itmes4, null, 0, null, 0
};

MIT(itundo1, "all areas  Alt-U");

struct MenuItem miundo1 = {
    null, SUBITEMLEFT, 0, 136, 10, ITEMTEXT | ITEMENABLED | HIGHCOMP, 0,
    &itundo1, null, 0, null, 0
};

MIT(itundo0, "this area      U");

struct MenuItem miundo0 = {
    &miundo1, SUBITEMLEFT, 0, 136, 10, ITEMTEXT | ITEMENABLED | HIGHCOMP, 0,
    &itundo0, null, 0, null, 0
};

MIT(itmes3, "Undo search          »");

struct MenuItem mimes3 = {
    &mimes4, 0, 0, ITEMWID, 10, ITEMTEXT, 0,
    &itmes3, null, 0, &miundo0, 0
};

MIT(itmes2, "Search for words...  S");

struct MenuItem mimes2 = {
    &mimes3, 0, 0, ITEMWID, 10, ITEMTEXT | ITEMENABLED | HIGHCOMP, 0,
    &itmes2, null, 0, null, 0
};

MIT(itmes1, "Previous area        [");

struct MenuItem mimes1 = {
    &mimes2, 0, 0, ITEMWID, 10, ITEMTEXT | ITEMENABLED | HIGHCOMP, 0,
    &itmes1, null, 0, null, 0
};

MIT(itmes0, "Next area            ]");

struct MenuItem mimes0 = {
    &mimes1, 0, 0, ITEMWID, 10, ITEMTEXT | ITEMENABLED | HIGHCOMP, 0,
    &itmes0, null, 0, null, 0
};


struct Menu mainu1 = {
    &mainu2, 96, 0, MENUWID, 10, 0, "Messages", &mimes0, 0, 0, 0, 0
};


MIT(itpac8, "Quit Q-Blue          Q");

struct MenuItem mipac8 = {
    null, 0, 0, ITEM1WID, 10, ITEMTEXT | ITEMENABLED | HIGHCOMP, 0,
    &itpac8, null, 0, null, 0
};

MIT(itpacZ, " -------------------- ");

struct MenuItem mipacZ = {
    &mipac8, 0, 0, ITEM1WID, 10, ITEMTEXT, 0,
    &itpacZ, null, 0, null, 0
};

MIT(itpac7, "About Q-Blue...  Alt-?");

struct MenuItem mipac7 = {
    &mipacZ, 0, 0, ITEM1WID, 10, ITEMTEXT | ITEMENABLED | HIGHCOMP, 0,
    &itpac7, null, 0, null, 0
};

MIT(itpac6, "Iconify screen       I");

struct MenuItem mipac6 = {
    &mipac7, 0, 0, ITEM1WID, 10, ITEMTEXT | ITEMENABLED | HIGHCOMP, 0,
    &itpac6, null, 0, null, 0
};

MIT(itpac5, "BBS information      B");

struct MenuItem mipac5 = {
    &mipac6, 0, 0, ITEM1WID, 10, ITEMTEXT | ITEMENABLED | HIGHCOMP, 0,
    &itpac5, null, 0, null, 0
};

char passtr[GREENBAY][20];

struct IntuiText passit[GREENBAY];

struct MenuItem passum[GREENBAY];

MIT(itcomp, "Compression type     »");

struct MenuItem micomp = {
    &mipac5, 0, 0, ITEMWID, 10, ITEMTEXT | ITEMENABLED, 0,
    &itcomp, null, 0, &passum[0], 0
};

MIT(itpac3, "Close packet         C");

struct MenuItem mipac3 = {
    &micomp, 0, 0, ITEM1WID, 10, ITEMTEXT | HIGHCOMP, 0,
    &itpac3, null, 0, null, 0
};

MIT(itpac2, "Pack replies         P");

struct MenuItem mipac2 = {
    &mipac3, 0, 0, ITEM1WID, 10, ITEMTEXT | HIGHCOMP, 0,
    &itpac2, null, 0, null, 0
};

MIT(itpac1, "Open (no packet)...  N");

struct MenuItem mipac1 = {
    &mipac2, 0, 0, ITEM1WID, 10, ITEMTEXT | ITEMENABLED | HIGHCOMP, 0,
    &itpac1, null, 0, null, 0
};

MIT(itpac0, "Open packet...       O");

struct MenuItem mipac0 = {
    &mipac1, 0, 0, ITEM1WID, 10, ITEMTEXT | ITEMENABLED | HIGHCOMP, 0,
    &itpac0, null, 0, null, 0
};


struct Menu mainu = {
    &mainu1, 0, 0, MENUWID, 10, MENUENABLED, "Packet", &mipac0, 0, 0, 0, 0
};


#ifdef GADTOOLS_NEWMENU

char passtr[GREENBAY][20];

struct IntuiText passit[GREENBAY];

struct MenuItem passum[GREENBAY];

MIT(itrep0B, "Flip to original     F");
MIT(itrep1U, "Un-delete reply      D");
MIT(itrep2R, "Re-edit reply...     R");

struct MenuItem misortlast = {
    null, SUBITEMLEFT, 0, ITEMWID + 36, 10,
    ITEMTEXT | CHECKIT | ITEMENABLED | HIGHCOMP, 0x1F & ~bit(0),
    null, null, 0, null, 0
};

struct MenuItem misort0 = {
    null, SUBITEMLEFT, 0, ITEMWID + 36, 10,
    ITEMTEXT | CHECKIT | ITEMENABLED | HIGHCOMP, 0x1F & ~bit(0),
    null, null, 0, null, 0
};

struct MenuItem mirep2 = {
    null, 0, 0, ITEMWID, 10, ITEMTEXT | ITEMENABLED | HIGHCOMP, 0,
    null, null, 0, null, 0
};

struct NewMenu mainudef[] = {
    { NM_TITLE, "Packet", 0, 0, 0, 0 },
      { NM_ITEM, "Open packet...       O", 0, 0, 0, 0 },
      { MN_ITEM, "Open (no packet)...  N", 0, 0, 0, 0 },
      { NM_ITEM, "Pack replies         P", 0, NM_ITEMDISABLED, 0, 0 },
      { NM_ITEM, "Close packet         C", 0, NM_ITEMDISABLED, 0, 0 },
      { NM_ITEM, "Pick compressor", 0, 0, 0, 0 },
	{ NM_SUB, "          Alt-1", 0, 0, 0, 0 },
	{ NM_SUB, "          Alt-2", 0, 0, 0, 0 },
	{ NM_SUB, "          Alt-3", 0, 0, 0, 0 },
	{ NM_SUB, "          Alt-4", 0, 0, 0, 0 },
	{ NM_SUB, "          Alt-5", 0, 0, 0, 0 },
	{ NM_SUB, "          Alt-6", 0, 0, 0, 0 },
	{ NM_SUB, "          Alt-7", 0, 0, 0, 0 },
	{ NM_SUB, "          Alt-8", 0, 0, 0, 0 },
      { NM_ITEM, "BBS information...   B", 0, NM_ITEMDISABLED, 0, 0 },
      { NM_ITEM, "Iconify screen       I", 0, 0, 0, 0 },
      { NM_ITEM, "About Q-Blue...       ", 0, 0, 0, 0 },
      { NM_ITEM, NM_BARLABEL, 0, 0, 0, 0 },
      { NM_ITEM, "Quit               Q", 0, 0, 0, 0 },
    { NM_TITLE, "Messages", 0, NM_MENUDISABLED, 0, 0 },
      { NM_ITEM, "Next area            ]", 0, 0, 0, 0 },
      { NM_ITEM, "Previous area        [", 0, 0, 0, 0 },
      { NM_ITEM, "Search for words...  S", 0, 0, 0, 0 },
      { NM_ITEM, "Undo search", 0, 0, 0, 0 },
	{ NM_SUB, "all areas  Alt-U", 0, 0, 0, 0 },
	{ NM_SUB, "this area      U", 0, 0, 0, 0 },
      { NM_ITEM, "Save msg as file     »", 0, 0, 0, 0 },
        { NM_SUB, "ask filename...   V", 0, 0, 0, 0 },
	{ NM_SUB, "append prev.  Alt-V", 0, 0, 0, 0 },
      { NM_ITEM, "Print message    Alt-P", 0, 0, 0, 0 },
      { NM_ITEM, "Areas with msgs...   A", 0, 0, 0, 0 },
      { NM_ITEM, "List area's msgs...  L", 0, 0, 0, 0 },
    { NM_TITLE, "Replies", 0, NM_MENUDISABLED, 0, 0 },
      { NM_ITEM, "Flip to reply        F", 0, 0, 0, 0 },
      { NM_ITEM, "Delete reply         D", 0, 0, 0, 0 },
      { NM_ITEM, "Reply to this msg... R", 0, 0, 0, 0 },
      { NM_ITEM, "Reply to addressee...E", 0, 0, 0, 0 },
      { NM_ITEM, "Write new message... W", 0, 0, 0, 0 },
      { NM_ITEM, "Carbon copy...   Alt-W", 0, 0, 0, 0 },
      { NM_ITEM, "Request D/L...   Alt-R", 0, NM_ITEMDISABLED, 0, 0 },
      { NM_ITEM, "Mail door...     Alt-M", 0, NM_ITEMDISABLED, 0, 0 },
      { NM_ITEM, "Maintain taglines    T", 0, 0, 0, 0 },
    { NM_TITLE, "Setup", 0, 0, 0, 0 },
      { NM_ITEM, "Sort messages", 0, 0, 0, 0 },
	{ NM_SUB, "by number         Ctrl-N", 0, CHECKIT | CHECKED, 0x1E, 0 },
	{ NM_SUB, "by age            Ctrl-A", 0, CHECKIT, 0x3D, 0 },
	{ NM_SUB, "by subject        Ctrl-S", 0, CHECKIT, 0x3B, 0 },
	{ NM_SUB, "by thread         Ctrl-D", 0, CHECKIT, 0x37, 0 },
	{ NM_SUB, "by who from       Ctrl-F", 0, CHECKIT, 0x2F, 0 },
	{ NM_SUB, "by who to         Ctrl-T", 0, CHECKIT, 0x1F, 0 },
	{ NM_SUB, NM_BARLABEL, 0, 0, 0, 0 },
	{ NM_SUB, "Last name first?  Ctrl-L", 0, CHECKIT | MENUTOGGLE, 0, 0 },
      { NM_ITEM, "Directories...   Alt-D", 0, 0, 0, 0 },
      { NM_ITEM, "Editor...        Alt-E", 0, 0, 0, 0 },
      { NM_ITEM, "Compressors...   Alt-C", 0, 0, 0, 0 },
      { NM_ITEM, "Font & screen... Alt-F", 0, 0, 0, 0 },
      { NM_ITEM, "Replying...      Alt-R", 0, 0, 0, 0 },
/*    { NM_ITEM, "Function keys... Alt-K", 0, 0, 0, 0 }, */
      { NM_ITEM, "Options...       Alt-O", 0, 0, 0, 0 },
      { NM_ITEM, "BBS Local...     Alt-B", 0, 0, 0, 0 },
      { NM_ITEM, NM_BARLABEL, 0, 0, 0, 0 },
      { NM_ITEM, "Load setup...    Alt-L", 0, 0, 0, 0 },
      { NM_ITEM, "Save setup...    Alt-S", 0, 0, 0, 0 },
    { NM_END, null, 0, 0, 0, 0 }
};

struct Menu *menoo;
#define mainu (*menoo)

#endif GADTOOLS_NEWMENU


struct Gadget *currentbotgag;

long currentbgcount;


short under[4] = { 11, 9, 20, 9 };

short downright[10] = { 1, 13, 78, 13, 78, 1, 79, 0, 79, 13 };

short upleft[10] = { 0, 0, 0, 13, 1, 12, 1, 0, 78, 0 };

short arrowright[10] = { 10, 1, 22, 1, 20, -1, 20, 3, 22, 1 };

short arrowleft[10] = { 22, 1, 10, 1, 12, -1, 12, 3, 10, 1 };


struct Border underborderR = {
    0, 0, UNDERCOLOR, 0, JAM2, 2, under, null
};

struct Border underborderV = {
    0, 0, UNDERCOLOR, 0, JAM2, 2, under, null
};

struct Border underborder = {
    0, 0, UNDERCOLOR, 0, JAM2, 2, under, null
};

/**
struct Border downborder2 = {
    0, 0, SHINECOLOR, 0, JAM2, 5, downright, &underborder
};

struct Border downborder = {
    0, 0, SHADOWCOLOR, 0, JAM2, 5, upleft, &downborder2
};
**/

struct Border upborderR2 = {
    0, 0, SHADOWCOLOR, 0, JAM2, 5, downright, &underborderR
};

struct Border upborderR = {
    0, 0, SHINECOLOR, 0, JAM2, 5, upleft, &upborderR2
};

struct Border upborderV2 = {
    0, 0, SHADOWCOLOR, 0, JAM2, 5, downright, &underborderV
};

struct Border upborderV = {
    0, 0, SHINECOLOR, 0, JAM2, 5, upleft, &upborderV2
};

struct Border upborder2 = {
    0, 0, SHADOWCOLOR, 0, JAM2, 5, downright, &underborder
};

struct Border upborder = {
    0, 0, SHINECOLOR, 0, JAM2, 5, upleft, &upborder2
};

/**
struct Border downborderright = {
    -1, 4, TEXTCOLOR, 0, JAM2, 5, arrowright, &downborder
};

struct Border downborderleft = {
    -1, 4, TEXTCOLOR, 0, JAM2, 5, arrowleft, &downborder
};
**/

struct Border upborderright = {
    -1, 4, TEXTCOLOR, 0, JAM2, 5, arrowright, &upborder
};

struct Border upborderleft = {
    -1, 4, TEXTCOLOR, 0, JAM2, 5, arrowleft, &upborder
};



/* The numbers for these seven gadgets no longer correspond to the order they
appear in on the screen: */

struct IntuiText bgt6 = {
    TEXTCOLOR, FILLCOLOR, JAM2, 32, 2, null, "Next", null
};

struct IntuiText bgt5saVe = {
    TEXTCOLOR, FILLCOLOR, JAM2, 12, 2, null, "saVe   ", null
};

struct IntuiText bgt5drop = {
    TEXTCOLOR, FILLCOLOR, JAM2, 12, 2, null, "Drop   ", null
};

struct IntuiText bgt5 = {
    TEXTCOLOR, FILLCOLOR, JAM2, 32, 2, null, "Prev.", null
};

struct IntuiText bgt4load = {
    TEXTCOLOR, FILLCOLOR, JAM2, 12, 2, null, "Load   ", null
};

struct IntuiText bgt4del = {
    TEXTCOLOR, FILLCOLOR, JAM2, 12, 2, null, "Delete ", null
};

struct IntuiText bgt4new = {
    TEXTCOLOR, FILLCOLOR, JAM2, 12, 2, null, "Create ", null
};

struct IntuiText bgt4 = {
    TEXTCOLOR, FILLCOLOR, JAM2, 12, 2, null, "List   ", null
};

struct IntuiText bgt1none = {
    TEXTCOLOR, FILLCOLOR, JAM2, 12, 2, null, "None   ", null
};

struct IntuiText bgt1add = {
    TEXTCOLOR, FILLCOLOR, JAM2, 12, 2, null, "Add... ", null
};

struct IntuiText bgt1all = {
    TEXTCOLOR, FILLCOLOR, JAM2, 12, 2, null, "All    ", null
};

struct IntuiText bgt1active = {
    TEXTCOLOR, FILLCOLOR, JAM2, 12, 2, null, "Active ", null
};

struct IntuiText bgt1asl = {
    TEXTCOLOR, FILLCOLOR, JAM2, 12, 2, null, "ASL req", null
};

struct IntuiText bgt1q = {
    TEXTCOLOR, FILLCOLOR, JAM2, 12, 2, null, "Quit   ", null
};

struct IntuiText bgt1 = {
    TEXTCOLOR, FILLCOLOR, JAM2, 12, 2, null, "Areas  ", null
};

struct IntuiText bgt2s = {
    TEXTCOLOR, FILLCOLOR, JAM2, 12, 2, null, "Search ", null
};

struct IntuiText bgt2i = {
    TEXTCOLOR, FILLCOLOR, JAM2, 12, 2, null, "Iconify", null
};

struct IntuiText bgt2w = {
    TEXTCOLOR, FILLCOLOR, JAM2, 12, 2, null, "Write  ", null
};

struct IntuiText bgt3random = {
    TEXTCOLOR, FILLCOLOR, JAM2, 12, 2, null, "Random ", null
};

struct IntuiText bgt3reset = {
    TEXTCOLOR, FILLCOLOR, JAM2, 12, 2, null, "Reset  ", null
};

struct IntuiText bgt3re = {
    TEXTCOLOR, FILLCOLOR, JAM2, 12, 2, null, "Re-edit", null
};

struct IntuiText bgt3r = {
    TEXTCOLOR, FILLCOLOR, JAM2, 12, 2, null, "Reply  ", null
};

struct IntuiText bgt3f = {
    TEXTCOLOR, FILLCOLOR, JAM2, 12, 2, null, "No pkt.", null
};

struct IntuiText bgt0p = {
    TEXTCOLOR, FILLCOLOR, JAM2, 12, 2, null, "Pack   ", null
};

struct IntuiText bgt0c = {
    TEXTCOLOR, FILLCOLOR, JAM2, 12, 2, null, "Close  ", null
};

struct IntuiText bgt0 = {
    TEXTCOLOR, FILLCOLOR, JAM2, 12, 2, null, "Open   ", null
};

/* left to right order on the screen: 0, 3, 2, 1, 4, 5, 6. */
/* ...what can I say?  The raisins are hysterical.         */

struct Gadget readgag6 = {
    null, 550, 0, 80, 0, GFLG_GADGHCOMP | GFLG_DISABLED, GACT_RELVERIFY,
    GTYP_BOOLGADGET, &upborderright, null, &bgt6, 0, null, 6, null
};

struct Gadget readgag5 = {
    &readgag6, 460, 0, 80, 0, GFLG_GADGHCOMP | GFLG_DISABLED, GACT_RELVERIFY,
    GTYP_BOOLGADGET, &upborderleft, null, &bgt5, 0, null, 5, null
};

struct Gadget readgag4 = {
    &readgag5, 370, 0, 80, 0, GFLG_GADGHCOMP | GFLG_DISABLED, GACT_RELVERIFY,
    GTYP_BOOLGADGET, &upborder, null, &bgt4, 0, null, 4, null
};

struct Gadget readgag3 = {
    &readgag4, 100, 0, 80, 0, GFLG_GADGHCOMP | GFLG_DISABLED, GACT_RELVERIFY,
    GTYP_BOOLGADGET, &upborder, null, &bgt3f, 0, null, 3, null
};

struct Gadget readgag2 = {
    &readgag3, 190, 0, 80, 0, GFLG_GADGHCOMP | GFLG_DISABLED, GACT_RELVERIFY,
    GTYP_BOOLGADGET, &upborder, null, &bgt2i, 0, null, 2, null
};

struct Gadget readgag1 = {
    &readgag2, 280, 0, 80, 0, GFLG_GADGHCOMP | GFLG_DISABLED, GACT_RELVERIFY,
    GTYP_BOOLGADGET, &upborder, null, &bgt1q, 0, null, 1, null
};

struct Gadget readgag0 = {
    &readgag1, 10, 0, 80, 0, GFLG_GADGHCOMP | GFLG_DISABLED, GACT_RELVERIFY,
    GTYP_BOOLGADGET, &upborder, null, &bgt0, 0, null, 0, null
};


local struct Image bgmanualautoimage = {
    0 /*, 0, 0, 0, 3, null, 7, 0, null */
};

struct PropInfo bgprops = {
    BGPROPFLAGS, 0, 0, MAXBODY, MAXBODY, 0, 0, 0, 0, 0, 0
};

struct Gadget bgscroll = {
    null, -14, 16, 15, 160, GFLG_GADGHNONE | GFLG_RELRIGHT | GFLG_GADGIMAGE,
    GACT_IMMEDIATE | GACT_RELVERIFY | GACT_FOLLOWMOUSE, GTYP_PROPGADGET,
    &bgmanualautoimage, null, null, 0, &bgprops, 12, null
};


local short bgarboxdotsshad[6] = { 14, 1, 14, 9, 1, 9 };

local short bgarboxdots[6] = { 0, 9, 0, 0, 14, 0 };

local short bgupdots[6] = { 4, 6, 7, 3, 10, 6 };

local short bgdowndots[6] = { 4, 3, 7, 6, 10, 3 };

local struct Border bgarrowboxshad = {
    0, 0, SHADOWCOLOR, 0, JAM2, 3, bgarboxdotsshad, null
};

local struct Border bgarrowbox = {
    0, 0, SHINECOLOR, 0, JAM2, 3, bgarboxdots, &bgarrowboxshad
};

struct Border bguppoint = {
    0, 0, 4, 0, JAM2, 3, bgupdots, &bgarrowbox
};

struct Border bgdownpoint = {
    0, 0, 4, 0, JAM2, 3, bgdowndots, &bgarrowbox
};


struct Gadget bguparrow = {
    &bgscroll, -14, -20, 15, 10,
    GFLG_GADGHCOMP | GFLG_RELRIGHT | GFLG_RELBOTTOM,
    GACT_IMMEDIATE | GACT_RELVERIFY,
    GTYP_BOOLGADGET, &bguppoint, null, null, 0, null, 11, null
};


struct Gadget bgdownarrow = {
    &bguparrow, -14, -10, 15, 10,
    GFLG_GADGHCOMP | GFLG_RELRIGHT | GFLG_RELBOTTOM,
    GACT_IMMEDIATE | GACT_RELVERIFY,
    GTYP_BOOLGADGET, &bgdownpoint, null, null, 0, null, 10, null
};


local struct IntuiText *(lastgags[7]) = {
    &bgt0, &bgt1q, &bgt2i, &bgt3f, &bgt4, &bgt5, &bgt6
};

local ushort lastmask = 0;

/* ----------------------------------------------------------------------- */


local void PickBGsliderNumbers(struct Mess *vis)
{
    if (vis && (vis->bits & LOADED)) {
	sliderspace = vis->linect + !!ATTACHED(vis) + 1;
	/* compensate for any empty space showing past end of message: */
	if (topline && winlines + topline > sliderspace)
	    sliderspace = winlines + topline;
    } else
	sliderspace = 1;
		/* this offset VVV is for one line overlap between pages */
    newbod = ((winlines - (sliderspace >= 2 * winlines)) * MAXBODY
				+ (MAXBODY / 2)) / sliderspace;
    if (newbod > MAXBODY)
	newbod = MAXBODY;
    if (sliderspace <= winlines)
	newpot = 0;
    else
	newpot = (topline * MAXPOT) / (sliderspace - winlines);
}


local void EdgeTheSlider(void)
{
    long gbl;

    ASSERT(bgwin);
    gbl = bgwin->Width + bgscroll.LeftEdge - 1;
    SetAPen(bgrp, lastcolor = (fourcolors ? 3 : 5));
    Move(bgrp, bgwin->Width - 1, textop - 1);
    Draw(bgrp, gbl, textop - 1);
    Draw(bgrp, gbl, textop + bgscroll.Height - 2);
    Move(&bgrpblack, gbl, textop + bgscroll.Height - 1);
    Draw(&bgrpblack, bgwin->Width - 1, textop + bgscroll.Height - 1);
/*  SetAPen(bgrp, lastcolor = 2);
    Draw(bgrp, bgwin->Width - 1, textop + bgscroll.Height);  */
}


void AdjustBGslider(struct Mess *vis, bool force)
{
    if ((!backslide && !obscuringslider) || slidestifle
		|| (!force && bgprops.VertPot != lastbgpot))
	return;
    PickBGsliderNumbers(vis);
    NewModifyProp(&bgscroll, bgwin, null, BGPROPFLAGS,
				MAXPOT, newpot, MAXBODY, newbod, 1);
    lastbgpot = bgprops.VertPot;
    EdgeTheSlider();
}


void MakeBGslider(void)
{
    short bottom, upwidth;
    if (!backslide && !popupscroller)
	return;
    ASSERT(bgrp);
    obscuringslider = !backslide;
    bottom = obscuringslider ? shortexbot : botedge;
    bgdownarrow.TopEdge = bottom - bgwin->Height - 8;
    bguparrow.TopEdge = bgdownarrow.TopEdge - 10;
    bgscroll.Height = bgwin->Height + bguparrow.TopEdge - textop - 1;
    bgscroll.TopEdge = textop - 1;
    PickBGsliderNumbers(onscreen);
    bgprops.VertPot = newpot;
    bgprops.VertBody = newbod;
    bgdownpoint.FrontPen = bguppoint.FrontPen = /* fourcolors ? 1 : 4; */ 1;
    SetAPen(bgrp, FILLCOLOR);
    RectFill(bgrp, bgwin->Width - 15, bgwin->Height + bguparrow.TopEdge,
				bgwin->Width - 2, bottom - 1);
    AddGList(bgwin, &bgdownarrow, 0, 3, null);
    RefreshGList(&bgdownarrow, bgwin, null, 3);
    upwidth = ((bgscroll.Width + fontwid) / fontwid) * fontwid;
    if (upwidth > bgscroll.Width)
	RectFill(&bgrpblack, bgwin->Width - upwidth, textop - 1,
	         bgwin->Width - bgscroll.Width - 1, bottom);
    EdgeTheSlider();
}


void NukeBGslider(void)
{
    if (backslide || obscuringslider) {
	ASSERT(bgwin);
	RemoveGList(bgwin, &bgdownarrow, 3);
	bgscroll.NextGadget = null;
	obscuringslider = false;
	if (!backslide) {
	    int upwidth = ((bgscroll.Width + fontwid) / fontwid) * fontwid;
	    RectFill(&bgrpblack, bgwin->Width - upwidth,
				 textop - 1, bgwin->Width - 1, shortexbot);
	    if (onscreen)
		PaintOverPopupScroller(onscreen);
	}
    }
}


void FixGreenGadMaybe(struct Gadget *gg, struct Window *ww, bool rend)
{
    short l = gg->LeftEdge + 2, t = gg->TopEdge + 1;

    ASSERT(gg && ww);
    if (gg->Width > 30)
	gg->Height = fight + 5;
    if (gg->Flags & GFLG_RELBOTTOM)
	t += ww->Height - 1;
    if (gg->Flags & GFLG_RELRIGHT)
	l += ww->Width - 1;
    if (rend) {
	SetAPen(ww->RPort, lastcolor = FILLCOLOR);
	RectFill(ww->RPort, l, t, l + gg->Width - 5, t + gg->Height - 3);
    }
}


void FixGreenGad(struct Gadget *gg, struct Window *ww)
{
    FixGreenGadMaybe(gg, ww, true);
}


void MakeBoxTop(void)
{
    long y = fight * 4 + 6, wid = bgwin->Width - 1;

    SetAPen(bgrp, TOPBOXCOLOR);
    Move(bgrp, 0, 1);
    Draw(bgrp, wid, 1);
    Draw(bgrp, wid, y);
    Draw(bgrp, 0, y);
    Draw(bgrp, 0, 1);
    Move(bgrp, 1, 2);
    Draw(bgrp, 1, --y);
    Move(bgrp, --wid, 2);
    Draw(bgrp, wid, y);
    SetAPen(bgrp, lastcolor = backcolor);
    RectFill(bgrp, 4, 3, wid - 3, fight * 4 + 4);
}


void NukeBotGadgets(void)
{
    struct Gadget *gg;

    if (!bottached)
	return;
    RemoveGList(bgwin, &readgag0, 7);
    bottached = false;
    readgag6.NextGadget = null;
    for (gg = &readgag0; gg; gg = gg->NextGadget)
	UnScaleGadget(gg);
}


void MakeBotGadgets(void)
{
    short ex = min(extralines, 3);
    short h = fight + 5, top = shortexbot + ex + 2;
    long edge = shortexbot + 1 + (extralines >= 2);
    long rightedge = nomwidth - 1;
    struct Gadget *gg;

    botedge = (top << 1) + h - edge - 1;
    under[1] = under[3] = h - 3;
    upborderleft.TopEdge = upborderright.TopEdge = fight / 2;
    upborderright.LeftEdge = (fontwid * 3 / 2) - 12;	/* 8 => 0 */
    upborderleft.LeftEdge = upborderright.LeftEdge - (~fontwid & 1);
    if (gagrow)
	SetAPen(bgrp, lastcolor = backcolor);
    RectFill(gagrow ? bgrp : &bgrpblack, 0, edge, rightedge, botedge);
    if (botedge < bgwin->Height - 2)
	RectFill(&bgrpblack, 0, botedge + 1, rightedge, bgwin->Height - 1);
    RectFill(&bgrpblack, 0, (long) shortexbot, rightedge, edge - 1);
    readgag6.NextGadget = null;
    for (gg = &readgag0; gg; gg = gg->NextGadget) {
	gg->TopEdge = top;
	if (!gg->UserData)
	    ScaleGadget(gg);
	FixGreenGadMaybe(gg, bgwin, gagrow);
    }
    if (gagrow) {
	GhostOn(bgwin);
	if (!bottached)
	    AddGList(bgwin, &readgag0, 0, 7, null);
	RefreshGadgets(&readgag0, bgwin, null);
	GhostOff(bgwin);
	winlines = shortwinlines;
	bottached = true;
    } else {
	NukeBotGadgets();
	winlines = (bgwin->Height - textop) / fight;
    }
    texbot = textop + fight * winlines;
    if (onscreen) {			/* this happens when un-iconifying */
	struct Mess *mm = onscreen;
	onscreen = null;
	remembertopline = topline;
	ShowNewMessage(null, mm);
	ASSERT(slidestifle);
	remembertopline = 0;
    }
}


void PresentPackers(void)
{
    short i, hit = tifight + 1 /*, topoff = ((packers - 1) * hit) / 2 */;

    if (menuset)
	NoMenus();
    for (i = 0; i < packers; i++) {
	passum[i].NextItem = &passum[i + 1];
	passum[i].LeftEdge = XTiActual(SUBITEMLEFT);
	passum[i].TopEdge = i * hit /* - topoff */ + 2;
	passum[i].Width = XTiActual(15 * 8) + 2 + CHECKMARKLEFT;
	passum[i].Height = hit;
	passum[i].Flags = CHECKIT | ITEMENABLED | ITEMTEXT | HIGHCOMP;
	if (i == currentpacker)
	    passum[i].Flags |= CHECKED;
	passum[i].MutualExclude = ~bit(i);
	passum[i].ItemFill = &passit[i];
	passum[i].SelectFill = null;
	passum[i].Command = 0;
	passum[i].SubItem = null;
	passum[i].NextSelect = 0;
	passit[i].FrontPen = lightbg ? 6 : 0;
	passit[i].BackPen = 1;
	passit[i].DrawMode = JAM2;
	passit[i].LeftEdge = CHECKMARKLEFT;
	passit[i].TopEdge = 1;
	passit[i].ITextFont = null;
	passit[i].IText = (ustr) &passtr[i];
	passit[i].NextText = null;
	sprintf(passtr[i], "%-9s Alt-%ld", packernames[i], i + 1L);
    }
    passum[packers - 1].NextItem = null;
    if (menuset)
	YesMenus();
}


void FullPresentPackers(void)
{
    if (scr)
	NoMenus();
    PresentPackers();
    rethinkmenus = true;
    if (scr)
	YesMenus();
}


/* **** Try to find some way to use LayoutMenus? */

void StretchMenus(void)
{
    short i, j, hi = tifight + 1, mwi = XTiActual(MENUWID), iwi, ile;
    short color = lightbg ? 6 : 0;
    struct Menu *mu;
    struct MenuItem *mi;

    j = ile = 0;
    for (mu = &mainu; mu; mu = mu->NextMenu) {
	iwi = XTiActual(22 * 8) + 4;
	mu->Width = mwi;
	mu->LeftEdge = mwi * j++;
	mu->Height = hi;
	if (mu->LeftEdge + iwi + 10 > scr->Width)
	    ile = scr->Width - iwi - mu->LeftEdge - 10;    /* negative offset */
	i = 0;
	for (mi = mu->FirstItem; mi; mi = mi->NextItem) {
	    mi->Height = hi;
	    mi->Width = iwi;
	    mi->TopEdge = i;
	    mi->LeftEdge = ile;
	    ((struct IntuiText *) mi->ItemFill)->FrontPen = color;
	    i += hi;
	}
    }
    iwi = XTiActual(19 * 8) + 2 + CHECKMARKLEFT;
    ile = XTiActual(SUBITEMLEFT);
    if (mainu3.LeftEdge + iwi + ile + 10 > scr->Width) {
	ile = scr->Width - mainu3.LeftEdge - iwi - 10;
	if (ile < XTiActual(SUBITEMLEFT / 3))
	    ile = -XTiActual(SUBITEMLEFT / 2);
    }
    i = 0;
    for (mi = &misort0; mi; mi = mi->NextItem) {
	mi->Height = hi;
	mi->TopEdge = i * hi;
	mi->Width = iwi;
	mi->LeftEdge = ile;
	((struct IntuiText *) mi->ItemFill)->LeftEdge = CHECKMARKLEFT;
	((struct IntuiText *) mi->ItemFill)->FrontPen = color;
	if (i == sorder)
	    mi->Flags |= CHECKED;
	i++;
    }
    ile = XTiActual(SUBITEMLEFT);
    miundo0.Height = miundo1.Height = miundo1.TopEdge = hi;
    miundo0.LeftEdge = miundo1.LeftEdge = ile;
    miundo0.Width = miundo1.Width = XTiActual(16 * 8) + 4;
    itundo0.FrontPen = itundo1.FrontPen = color;
    i = 0;
    for (mi = &misave0; mi; mi = mi->NextItem) {
	mi->Height = hi;
	mi->TopEdge = i * hi;
	mi->LeftEdge = ile;
	mi->Width = XTiActual(22 * 8) + 4;
	((struct IntuiText *) mi->ItemFill)->FrontPen = color;
	i++;
    }
    PresentPackers();
}


void ActSubMenu(short menu, short item, short sub, bool enable)
{
    struct MenuItem *mi = ItemAddress(&mainu, FULLMENUNUM(menu, item, sub));
    if (!mi || !enable == !(mi->Flags & ITEMENABLED))
	return;
    if (enable)
	if (bgwin->MenuStrip)
	    OnMenu(bgwin, FULLMENUNUM(menu, item, sub));
	else
	    mi->Flags |= ITEMENABLED;
    else
	if (bgwin->MenuStrip)
	    OffMenu(bgwin, FULLMENUNUM(menu, item, sub));
	else
	    mi->Flags &= ~ITEMENABLED;
}


void ActMenu(short menu, short item, bool enable)
{
    struct MenuItem *mi = ItemAddress(&mainu, FULLMENUNUM(menu, item, NOSUB));
    if (!mi || !enable == !(mi->Flags & ITEMENABLED))
	return;
    if (enable)
	if (bgwin->MenuStrip)
	    OnMenu(bgwin, FULLMENUNUM(menu, item, NOSUB));
	else
	    mi->Flags |= ITEMENABLED;
    else
	if (bgwin->MenuStrip)
	    OffMenu(bgwin, FULLMENUNUM(menu, item, NOSUB));
	else
	    mi->Flags &= ~ITEMENABLED;
}


/* This does not refresh the change.  ChangeBotGagText can refresh. */

void ChangeGagText(struct Gadget *gg, struct Window *ww, struct IntuiText *ii)
{
    long p;

    if (gg->GadgetText == ii)
	return;
    p = RemoveGadget(ww, gg);
    if (gg->UserData) {
	UnScaleGText(gg);
	gg->GadgetText = ii;
	ScaleGText(gg);
    } else
	gg->GadgetText = ii;
    AddGadget(ww, gg, p);
}


void ChangeBotGagText(struct Gadget *gg, struct IntuiText *ii, bool fresh)
{
    if (bottached) {
	if (fresh)
	    GhostOn(bgwin);
	ChangeGagText(gg, bgwin, ii);
	if (fresh) {
	    RefreshGList(gg, bgwin, null, 1);
	    GhostOff(bgwin);
	}
/********************   NO, un-attached gags are always unscaled!
    } else if (gg->UserData) {
	UnScaleGText(gg);
	gg->GadgetText = ii;
	ScaleGText(gg);
***********************/
    } else
	gg->GadgetText = ii;
}


void SwitchRGag(bool replies, bool deleted)
{
    struct Conf *cc = readareaz.confs[whicha];
    struct Mess *mm = cc->messes[whichm];
    struct IntuiText *foo;
    bool bull = cc == &bullstuff;

    ASSERT(whicha >= 0 && whicha < readareaz.messct);
    NoMenus();
    if (replies) {
	foo = (deleted ? &itrep1U : &itrep1);		/* Un-delete / Delete */
	if (mirep1.ItemFill != foo)
	    mirep1.ItemFill = foo, rethinkmenus = true;
    }
    if (replies && readgag3.GadgetText != &bgt3re) {
	ChangeBotGagText(&readgag3, &bgt3re, true);	/* Re-edit */
	mirep2.ItemFill = &itrep2R;			/* Re-edit reply */
	mirep0.ItemFill = &itrep0B;			/* Flip to original */
	ActMenu(2, 1, true);				/* (Un-)Delete reply */
	rethinkmenus = true;
    } else if (!replies && readgag3.GadgetText == &bgt3re) {
	ChangeBotGagText(&readgag3, &bgt3r, true);	/* Reply */
	mirep2.ItemFill = &itrep2;			/* Reply to this msg */
	mirep1.ItemFill = &itrep1;			/* (Un-)Delete reply */
	mirep0.ItemFill = &itrep0;			/* Flip to reply */
	ActMenu(2, 1, false);				/* Delete reply */
	rethinkmenus = true;
    }
    ActMenu(2, 0, !!mm->mreplyee);			/* Flip to reply */
    ActMenu(2, 2, !bull);				/* Reply to this msg */
    ActMenu(2, 3, !replies && !bull);			/* Reply to addressee */
    ActMenu(2, 6, !bull);				/* Carbon copy */
    ActMenu(1, 0, whicha < readareaz.messct - 1);	/* Next area */
    ActMenu(1, 1, whicha > 0);				/* Previous area */
    ActSubMenu(1, 5, 3, /*!replies &&*/ !!ATTACHED(mm));/* Save attached file */
    YesMenus();
}


void TweakCloseGag(void)
{
    bool anything = repchanges && anythingtosave;
    if ((anything && readgag0.GadgetText != &bgt0p)
			|| (!anything && readgag0.GadgetText != &bgt0c)) {
	if (anything) {
	    ChangeBotGagText(&readgag0, &bgt0p, true);	/* Pack */
	    ActMenu(0, 2, true);			/* Pack replies */
	} else {
	    ChangeBotGagText(&readgag0, &bgt0c, true);	/* Close */
	    ActMenu(0, 2, false);			/* Pack replies */
	}
    }
}


ushort FlipBGadgets(ushort mask)
{
    short gct;
    long bar;
    register ushort old = 0, ma = mask, llama = lastmask;
    struct Gadget *gg;

    if (deferflipb) {
	for (gct = 0, gg = &readgag0; gct < 7; gg = gg->NextGadget, gct++)
	    if (!(gg->Flags & GFLG_DISABLED))
		old |= bit(gct);
	return old;
    }
    lastmask = mask;
    if (bgwin)
	GhostOn(bgwin);
    for (gct = 0, gg = &readgag0; gct < 7; gg = gg->NextGadget, gct++) {
	if (bottached)
	    bar = RemoveGadget(bgwin, gg);
	if (!(gg->Flags & GFLG_DISABLED))
	    old |= bit(gct);
	if (!(ma & 1))
	    gg->Flags |= GFLG_DISABLED;
	else
	    gg->Flags &= ~GFLG_DISABLED;
	if (bottached) {
	    AddGadget(bgwin, gg, bar);
	    if ((llama & 1) ^ (ma & 1) || lastgags[gct] != gg->GadgetText) {
		if (!(gg->Flags & GFLG_DISABLED))
		    FixGreenGad(gg, bgwin);
		RefreshGList(gg, bgwin, null, 1);
		lastgags[gct] = gg->GadgetText;
	    }
	}
	ma >>= 1;
	llama >>= 1;
    }
    if (bgwin)
	GhostOff(bgwin);
    return old;
}


void SetMessGags(bool on)
{
    if (on) {
	OnMenu(bgwin, FULLMENUNUM(1, NOITEM, NOSUB));	/* Messages */
	OnMenu(bgwin, FULLMENUNUM(2, NOITEM, NOSUB));	/* Replies */
	OnMenu(bgwin, FULLMENUNUM(0, 3, NOSUB));	/* Close packet */
	OffMenu(bgwin, FULLMENUNUM(0, 0, NOSUB));	/* Open packet */
	OffMenu(bgwin, FULLMENUNUM(0, 1, NOSUB));	/* Open (no packet) */
	OnMenu(bgwin, FULLMENUNUM(3, 7+FK_OFF, NOSUB));	/* BBS Local */
	ActMenu(2, 7, requestedfiles_limit > 0);	/* Request D/L */
	OnMenu(bgwin, FULLMENUNUM(2, 8, NOSUB));	/* Mail door */
	OnMenu(bgwin, FULLMENUNUM(0, 5, NOSUB));	/* BBS information */
	TweakCloseGag();				/* Close / Pack */
	ChangeBotGagText(&readgag3, &bgt3r, false);	/* Reply */
	ChangeBotGagText(&readgag2, &bgt2w, false);	/* Write */
	ChangeBotGagText(&readgag1, &bgt1, false);	/* Areas */
	FlipBGadgets(0);
    } else {
	OffMenu(bgwin, FULLMENUNUM(1, NOITEM, NOSUB));	/* Messages */
	OffMenu(bgwin, FULLMENUNUM(2, NOITEM, NOSUB));	/* Replies */
	OffMenu(bgwin, FULLMENUNUM(0, 3, NOSUB));	/* Close packet */
	OnMenu(bgwin, FULLMENUNUM(0, 0, NOSUB));	/* Open packet */
	OnMenu(bgwin, FULLMENUNUM(0, 1, NOSUB));	/* Open (no packet) */
	OffMenu(bgwin, FULLMENUNUM(3, 7+FK_OFF, NOSUB));/* BBS Local */
	OffMenu(bgwin, FULLMENUNUM(2, 7, NOSUB));	/* Request D/L */
	OffMenu(bgwin, FULLMENUNUM(2, 8, NOSUB));	/* Mail door */
	OffMenu(bgwin, FULLMENUNUM(0, 5, NOSUB));	/* BBS information */
	ChangeBotGagText(&readgag0, &bgt0, false);	/* Open */
	ChangeBotGagText(&readgag3, &bgt3f, false);	/* No Pkt. */
	ChangeBotGagText(&readgag2, &bgt2i, false);	/* Iconify */
	ChangeBotGagText(&readgag1, &bgt1q, false);	/* Quit */
	FlipBGadgets(0x0F);
    }
}
