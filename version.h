#define VERSION       "2.4"
#define VERSION_MAJOR 2
#define VERSION_MINOR 40

/* same string as VERSION, each byte xor'd with INITMASK: */
#define CLOAK_VERSION "\x97\x8B\x91"

/* "Q-Blue" xor'd the same way: */
#define CLOAK_NAME    "\xF4\x88\xE7\xC9\xD0\xC0"

/* "Blue Wave" xor'd the same way (for stealth): */
#define CLOAK_GENERIC "\xE7\xC9\xD0\xC0\x85\xF2\xC4\xD3\xC0"

/* "2.30" (version number for imitation-BWave stealth): */
#define CLOAK_GENERIC_VER   "\x97\x8B\x96\x95"

/* a space character xor'd that way: */
#define CLOAK_SPACE   "\x85"

/* and an asterisk: */
#define CLOAK_STAR    "\x8F"

/* TEARGHEAD: */
#define CLOAK_GHEAD   CLOAK_SPACE CLOAK_STAR CLOAK_SPACE CLOAK_NAME CLOAK_SPACE

/* "Beta": */
#define CLOAK_BETA    "\xE7\xC0\xD1\xC4"

/* "[NR]": */
#define CLOAK_NR      "\xFE\xEB\xF7\xF8"

/* and a QWK newline character, 0xE3: */
#define CLOAK_QEOL    "\x46"

/* stuff for storing the customer's name semi-encrypted: */
#define NAMECOOKIE_LEN   12
#define NAMECOOKIE       "\xE3\x11\x4A\xFD\xC9\xED\x2C\xCD\xF0\x0B\xAD\xEE"
#define CUSTOMERNAME_LEN 48
#define INITMASK         0xA5
