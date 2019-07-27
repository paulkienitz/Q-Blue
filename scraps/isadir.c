/* a little pure program that tests whether the string passed in the argument */
/* line is a directory or not, and returns WARN to the CLI if not.  2.x only. */

#include <exec/libraries.h>
#include <utility/tagitem.h>
#include <dos/dos.h>
#include <dos/rdargs.h>
#include <clib/dos_protos.h>
#include <pragmas/dos_lib.h>

struct Library *DOSBase;

long _main(void)
{
    long done = TAG_DONE;
    char *pathptr;
    struct RDArgs *ra;
    struct FileInfoBlock *fib;
    BPTR pl;
    long ret = 0;

    if (DOSBase->lib_Version < 37) {
	Write(Output(), "AmigaDOS 2.04 required\n", 23);
	return 20;
    }
    if (!(ra = ReadArgs("PATH/A", (long *) &pathptr, NULL))) {
	PrintFault(IoErr(), "IsADir");
	return 20;
    }
    if (!(fib = AllocDosObject(DOS_FIB, (struct TagItem *) &done))) {
	FreeArgs(ra);
	PrintFault(IoErr(), "IsADir");
	return 10;
    }
    if (pl = Lock(pathptr, ACCESS_READ)) {
	if (Examine(pl, fib))
	    if (fib->fib_DirEntryType < 0 || fib->fib_DirEntryType == 3)
		ret = 5;
	UnLock(pl);
    } else
	ret = 10;
    FreeDosObject(DOS_FIB, fib);
    FreeArgs(ra);
    return ret;
}
