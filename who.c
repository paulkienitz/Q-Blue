/* Version- and compile-time-sensitive stuff for Q-Blue */

#include <string.h>
#include <exec/memory.h>
#define NO_PROTOS
#include <Paul.h>
#include "version.h"

#ifndef VERSIONTAIL
#  define VERSIONTAIL    ""
#endif

#ifdef TRY
#  define REPLYLIMIT     4
#  define REPLYLIMITTEXT "four"
#else
#  define REPLYLIMIT     200
#  define REPLYLIMITTEXT "200"
#endif

#define CLOAK_QWK_BRAG   CLOAK_QEOL CLOAK_GHEAD CLOAK_VERSION CLOAK_SPACE

/* things I didn't want to pull in the headers for: */
APTR AllocMem(unsigned long byteSize, unsigned long requirements);
void FreeMem(APTR memoryBlock, unsigned long byteSize);
int sprintf(char *_s, const char *_format, ...);

long sput(long p, str in);
void Err(str text, ...);

ushort Chextr(str s, ushort initial);
#pragma regcall(Chextr(a0, d0))

#ifndef strend
str strend(str s);
#  pragma regcall(strend(a1))
#endif


import char workdir[], replydir[], edit1command[], uploaddir[], bbsesdir[];
import char anyname[], quoteheader[], myloginame[];
import ushort palette[];
import ustr *oldfilenames, *messageID, *newsgroups;

char /* bool */ attachment_hidden[REPLYLIMIT];		/* used in pack.c */

ubyte versionthing[] = "\0$VER: Q-Blue " VERSION " ("
#include "ENV:VersionDate"
")\r\n";

char copyright[] =
    "\nQ-Blue " VERSION VERSIONTAIL " Copyright (c) 1992-1999 by Paul Kienitz.\n";

char bversionI[] = CLOAK_VERSION CLOAK_SPACE CLOAK_NAME;

#ifdef ALPHA

char teargline[] = CLOAK_QWK_BRAG CLOAK_STAR;
char bversionL[] = CLOAK_VERSION;
char rawtitle[] = " Q-Blue " VERSION VERSIONTAIL " Alpha";
char x_version[] = CLOAK_NAME CLOAK_SPACE CLOAK_VERSION;

#elif BETA

char teargline[] = CLOAK_QWK_BRAG CLOAK_BETA CLOAK_SPACE CLOAK_STAR;
char bversionL[] = CLOAK_VERSION CLOAK_SPACE CLOAK_BETA;
char rawtitle[] = " Q-Blue " VERSION VERSIONTAIL " BETA";
char x_version[] = CLOAK_NAME CLOAK_SPACE CLOAK_VERSION CLOAK_SPACE CLOAK_BETA;

#elif TRY

char teargline[] = CLOAK_QWK_BRAG CLOAK_NR CLOAK_SPACE CLOAK_STAR;
char bversionL[] = CLOAK_VERSION CLOAK_SPACE CLOAK_NR;
char rawtitle[] = " Q-Blue " VERSION VERSIONTAIL " [NR]";
char x_version[] = CLOAK_NAME CLOAK_SPACE CLOAK_VERSION CLOAK_SPACE CLOAK_NR;

#else

char teargline[] = CLOAK_QWK_BRAG CLOAK_STAR;
char bversionL[] = CLOAK_VERSION;
char rawtitle[] = " Q-Blue " VERSION VERSIONTAIL;
char x_version[] = CLOAK_NAME CLOAK_SPACE CLOAK_VERSION;

#endif


str aboutlines[] = {
    "",
    "!Q-Blue release " VERSION VERSIONTAIL" by Paul Kienitz, \0"
					"xxxxxxxxxxxxxxxxxxxxxxxxx",
    "",
#ifdef BETA
# ifdef SHAREWARE
    "THIS IS A PRE-RELEASE TEST VERSION.  DO NOT DISTRIBUTE IT TO OTHER USERS.",
    "",
#  ifdef TRY
    "This beta-test version does not include the features of the registered",
    "version -- only those of the evaluation version.  It is limited to at",
    "most " REPLYLIMITTEXT " replies per upload packet.",
#  else
    "It includes all features of the forthcoming registered version.",
    "Normally this requester would be showing here a message either telling",
    "you (if registered) who this copy belongs to and what its serial number",
    "is, or (in the more limited free version) telling you that this is",
    "shareware and you should buy a registered copy if you use it regularly.",
#  endif
# else
    "THIS IS A PRE-RELEASE TEST VERSION.",
    "Q-Blue is an offline electronic mail reader for Amiga computers,",
    "compatible with QWK and Blue Wave mail formats.  Though it was",
    "formerly shareware, it is now freeware and may be redistributed",
    "by any means so long as all files in the original distribution",
    "archive are kept intact, and may be used with no restrictions.",
# endif
#elif TRY
# ifdef PUBLIC_BETA
    "This is a PUBLIC BETA-TEST version.  It may be freely distributed.",
    "A final non-beta release will be made after bug reports from this",
    "version have been resolved.  If you find a problem, please report",
    "it to the author.  Q-Blue is shareware.  This is an evaluation",
    "version, with some limitations.  You are free to use it regularly",
    "for up to thirty days, or until the non-beta release comes out,",
    "whichever is later.  If you want a version that has no limitations",
    "on features or usage, you must buy a registered copy.  See the",
    "accompanying file \"How-to-order\" for information on ordering a",
    "registered copy, which will be sent after the beta period ends.",
# else
    "This is the unregistered version.  It may be freely distributed.",
    "",
    "Q-Blue is an offline electronic mail reader for Amiga computers,",
    "compatible with QWK and Blue Wave mail formats.  Q-Blue is shareware.",
    "You are licensed to use this evaluation version regularly for up to",
    "thirty days free of charge.  If you wish to continue using it regularly",
    "after that time, or use a version that is not limited to "
						REPLYLIMITTEXT " replies",
    "per upload packet, you must buy a registered copy.  See the included",
    "\"How-to-order\" file for information on ordering a copy.",
# endif
#else	/* !TRY && !BETA */
# ifdef SHAREWARE
    "Q-Blue is an offline electronic mail reader for Amiga computers,",
    "compatible with QWK and Blue Wave mail formats.  This is a registered",
    "copy, not to be distributed to other users.  The evaluation version,",
    "with some features limited, may be copied and distributed freely.",
     "",
    "!This copy has serial number xxxxxxxxxxxx.",
    "!It belongs to xxxxxxxxxxxxxxxxxxxxxx"
		"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx.",
# else
    "Q-Blue is an offline electronic mail reader for Amiga computers,",
    "compatible with QWK and Blue Wave mail formats.  Though it was",
    "formerly shareware, it is now freeware and may be redistributed",
    "by any means so long as all files in the original distribution",
    "archive are kept intact, and may be used with no restrictions.",
# endif
#endif
    null
};


local ustr crypt_name;
ushort reg_number;

#if !defined(BETA) && !defined(TRY)
ubyte reg_name[NAMECOOKIE_LEN + 2 + CUSTOMERNAME_LEN] = NAMECOOKIE "\0\0";
#endif


ulong MagicIdentifierNumber(void)
{
    ulong sum = (Chextr(workdir, 'T') << 16) | Chextr(replydir, 'o');

    sum ^= Chextr(edit1command, 'n') << 8;
#ifdef TRY
    sum ^= ~(ulong) (Chextr(bbsesdir, 'y') << 8);	/* incorrect! */
#else
    sum += Chextr(bbsesdir, 'i') << 6;
#endif
    sum += Chextr(crypt_name + NAMECOOKIE_LEN + 2, 'R') << 12;
    sum += Chextr(uploaddir, 'i') << 4;
    sum ^= Chextr(anyname, 's') ^ (Chextr(quoteheader, 'o') << 16);
    return sum + (palette[3] << 14) + (palette[6] << 2) - reg_number;
}


long sputCryptName(long p)
{
    char foo[CUSTOMERNAME_LEN + 10], *s;
    short i;
    strcpy(foo, crypt_name + NAMECOOKIE_LEN + 2);
    s = strend(foo);
    for (i = 3; i < 8; i++)
	*(s++) = crypt_name[i];		/* pad encrypted name with garbage */
    *(s++) = 0;
    return sput(p, foo);
}


#if !defined(BETA) && !defined(TRY) && defined(SHAREWARE)

void InitUserNameNum(void)
{
    ubyte reg_decrypt[CUSTOMERNAME_LEN], mask = INITMASK;
    ustr enc = reg_name + NAMECOOKIE_LEN + 2, dec = reg_decrypt;

    crypt_name = reg_name;
    reg_number = (reg_name[NAMECOOKIE_LEN] << 8) + reg_name[NAMECOOKIE_LEN + 1];
    strcat(aboutlines[1], __DATE__);
    if (reg_number) {
	do {
	    *dec = mask ^ *enc;
	    mask = *enc++;
	} while (*dec++);
	sprintf(aboutlines[8], "!This copy has serial number %lu.",
				(ulong) reg_number);
	sprintf(aboutlines[9], "!It belongs to %s.", reg_decrypt);
	memset(reg_decrypt, 0, CUSTOMERNAME_LEN);
    } else {
	strcpy(aboutlines[8], "!This copy is not registered to anyone;");
	strcpy(aboutlines[9], "!there is no name or serial number in it.");
    }
    reg_number = 3 * reg_number ^ INITMASK * 47;
}

#else

void InitUserNameNum(void)
{
    crypt_name = NAMECOOKIE - 8; /* we use crypt_name + NAMECOOKIE_LEN + 2 */
    reg_number = INITMASK * 47;
    strcat(aboutlines[1], __DATE__);
}

#endif


bool ReplyChoke(ushort current, bool reloading)
{
    if (current < REPLYLIMIT)
	return false;
    if (reloading)
	Err("Could not load all replies -- this\n"
#ifdef TRY
			"unregistered version cannot hold more\n"
			"than " REPLYLIMITTEXT " at a time.");
#else
			"version is limited to a maximum of\n"
			REPLYLIMITTEXT " replies at a time.");
#endif
    else
	Err("Sorry, you cannot add any more replies.\n"
#ifdef TRY
			"To create more than " REPLYLIMITTEXT " replies at"
			" a time,\nyou must buy the registered version.");
#else
			"This version cannot hold more than " REPLYLIMITTEXT
			"\nreplies at a time.");
#endif
    return true;
}


void FreeReplySpace(adr replymesses)
{
#ifndef TRY
    if (replymesses)
	NFREE((adr *) replymesses, REPLYLIMIT + 1);
    if (oldfilenames)
	NFREE(oldfilenames, REPLYLIMIT * 3);
#endif
}


void ZeroOldfilenamesEtc(void)
{
    if (!oldfilenames) return;
    memset(oldfilenames, 0, sizeof(ustr) * REPLYLIMIT);
    memset(messageID, 0, sizeof(ustr) * REPLYLIMIT);
    memset(newsgroups, 0, sizeof(ustr) * REPLYLIMIT);
    memset(attachment_hidden, 0, sizeof(attachment_hidden));
}


adr MakeReplySpace()
{
#ifdef TRY
    static ustr groups[REPLYLIMIT], msgids[REPLYLIMIT], oldfns[REPLYLIMIT];
    static adr repliespace[REPLYLIMIT];

    oldfilenames = &oldfns[0];
    messageID = &msgids[0];
    newsgroups = &groups[0];
    return &repliespace[0];
#else
    register adr *rs;
    if (!NNEWZ(oldfilenames, REPLYLIMIT * 3))
	return null;
    messageID = &oldfilenames[REPLYLIMIT];
    newsgroups = &oldfilenames[REPLYLIMIT * 2];
    return NNEW(rs, REPLYLIMIT + 1);
#endif
}
