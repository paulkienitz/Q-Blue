/* a bunch of declarations used in those Q-Blue source files that   */
/* deal with gadgets and dialog windows, especially string gadgets. */

#define STRING_LINER

/* what pens to use for various gadget parts: */

#define SHINECOLOR     7
#define SHADOWCOLOR    0
#define OFFBGCOLOR     2
#define ONBGCOLOR      0
#define OFFTXCOLOR     1
#define ONTXCOLOR      1
#define TEXTCOLOR      5
#define FILLCOLOR      2
#define UNDERCOLOR     1
#define LABELCOLOR     7
#define CHECKCOLOR     UNDERCOLOR

/* how checkmark gadgets are set up: */

#define CHECKWID       26
#define CHECKHILITE    GFLG_GADGHIMAGE
#define CHECKACTION    GACT_RELVERIFY | GACT_TOGGLESELECT
#define CHECK(gg, var) (var = !!((gg).Flags & GFLG_SELECTED))

/* create an appropriate struct StringInfo: */

#define STRINF(B, L) {B, undospace, 0, L, 0, 0, 0, 0, 0, 0, &stringex, 0, null}

/* The following defines a raised ridge border to be fed to FixStringGad: */

#define _SBH(x, n) local short x##X[10]; local struct Border x={0,0,SHADOWCOLOR,0,JAM2,5,x##X,n};
#define _SBS(x) local short x##X[10]; struct Border x={0,0,SHINECOLOR,0,JAM2,5,x##X,&x##0};
#define _SBCORE(x)  _SBH(x##1, null) _SBH(x##00, &x##1) _SBS(x##0) _SBS(x)

#ifdef STRING_LINER

#define _BLH(x, xn, c)  local short x##X[8]; struct Border x = {0,0,c,0,JAM2,4,x##X,xn};
#define _BLINN(x, xn)   _BLH(x##2, xn, OFFBGCOLOR) _BLH(x, &x##2, OFFBGCOLOR)
#define STRINGBORDER(x) _SBCORE(x##C) _BLINN(x, &x##C)

#else

#define STRINGBORDER(x) _SBCORE(x)

#endif
