/* A variation of the Skip command which doesn't blow up when the script */
/* file has lines 500 characters long.  By Paul Kienitz, public domain.  */

#include <exec/libraries.h>
#include <dos/dosextens.h>
#include <dos/rdargs.h>
#include <clib/dos_protos.h>
#include <string.h>

int toupper(int c);

#define LINSIZ   256

struct Library *SysBase;

long _main(void)
{
    struct CommandLineInterface *cli;
    struct RDArgs *ra;
    unsigned char *(argz[2]), *p, *q;
    BPTR shand;
    long llen;
    char line[LINSIZ + 2];
    
    if (SysBase->lib_Version < 37 || !(cli = Cli()))
	return 100;
    if ((shand = cli->cli_CurrentInput) == cli->cli_StandardInput) {
	PutStr("XSkip is valid only within a command file\n");
	return 20;
    }
    argz[0] = argz[1] = NULL;
    if (!(ra = ReadArgs("LABEL,BACK/S", (long *) argz, NULL))) {
	PrintFault(IoErr(), "XSkip");
	return 20;
    }
    Flush(shand);
    if (argz[1])
	Seek(shand, 0, OFFSET_BEGINNING);	/* skip back! */
    for (;;) {
	if (!FGets(shand, line, LINSIZ))
	    break;
	llen = strlen(p = line);
	while (*p == ' ' || *p == '\t') p++;
	if (toupper(*p) == 'L' && toupper(*++p) == 'A' && toupper(*++p) == 'B'
				&& (*++p == ' ' || *p == '\t')) {
	    q = argz[0];
	    if (!*q)
		p = "";
	    else
		while (*p == ' ' || *p == '\t')
		    p++;
	    while (*q && toupper(*p) == toupper(*q))
		p++, q++;
	    if (!*q && *p <= ' ') {		/* we've found our spot! */
		while (line[llen - 1] != '\n' && FGets(shand, line, LINSIZ))
		    llen = strlen(line);	/* get to end of line */
		Flush(shand);		/* seek to where FGets stopped! */
		return 0;
	    }
	    while (line[llen - 1] != '\n' && FGets(shand, line, LINSIZ))
		llen = strlen(line);
	}
    }
    Printf("Label \"%s\" not found\n", argz[0]);
    /* as with the real Skip, script file will now exit -- input exausted */
    return 20;
}
