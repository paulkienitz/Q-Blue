/* Sets the datestamp of the file named in the command line to a day   */
/* one week in the future.  By Paul Kienitz, 24-Sep-94, public domain. */
/* For Aztec C compiler.  Template is FILE/A,DAYS/N.  Optional DAYS    */
/* arg is days into the future (or past, if the number is negative) to */
/* set the file relative to today.  The default value is 7: one week   */
/* in the future.  The file date's time-of-day setting is not changed; */
/* only the day is modified.                                           */

#include <exec/libraries.h>
#include <dos/dos.h>
#include <dos/rdargs.h>
#include <clib/dos_protos.h>
#include <pragmas/dos_lib.h>

struct Library *DOSBase;
char pname[] = "Future";

long days = 7, ret = RETURN_ERROR, args[2] = { 0, 0 };


long _main(void)
{
    struct RDArgs *ra;
    char *path;
    BPTR rl;
    struct FileInfoBlock *fib;
    struct DateStamp dd;

    if (DOSBase->lib_Version < 37) {
	Write(Output(), "AmigaDOS 2.04 required.\n", 24);
	return RETURN_FAIL;
    }
    if (ra = ReadArgs("FILE/A,DAYS/N", args, NULL)) {
	path = (char *) args[0];
	if (args[1]) days = *(long *) args[1];
	if (rl = Lock(path, ACCESS_READ)) {
	    if (fib = AllocMem(sizeof(*fib), MEMF_PUBLIC)) {
		if (Examine(rl, fib)) {
		    DateStamp(&dd);
		    dd.ds_Days += days;
		    dd.ds_Minute = fib->fib_Date.ds_Minute;
		    dd.ds_Tick = fib->fib_Date.ds_Tick;
		    if (SetFileDate(path, &dd))
			ret = RETURN_OK;
		    else
			PrintFault(IoErr(), pname);
		} else
		    PrintFault(IoErr(), pname);
		FreeMem(fib, sizeof(*fib));
	    } else
		PrintFault(ERROR_NO_FREE_STORE, pname);
	    UnLock(rl);
	} else
	    PrintFault(IoErr(), pname);
	FreeArgs(ra);
    } else
	PrintFault(IoErr(), pname);
    return ret;
}
