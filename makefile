# makefile for Q-Blue, using my own enhancement of the PD make from Fish disk 69

# we have now moved things around so that only who.c and main.c have to know
# about the TRY and ALPHA defines... everything else is *.o, *.bo, or *.3o

OBSB = ob/screen.bo ob/load.bo ob/config.bo \
     ob/clip.bo ob/ask.bo ob/qwk.bo ob/compose.bo ob/write.bo ob/blue.bo \
     ob/bgag.bo ob/view.bo ob/setcoy.bo ob/setdel.bo ob/list.bo ob/lcore.bo \
     ob/search.bo ob/tagline.bo ob/trash.bo ob/pack.bo ob/setfk.bo \
     ob/door.bo # ob/stch.o

OBSM = ob/pack.o ob/load.o ob/door.o ob/trash.o \
     ob/blue.o ob/qwk.o ob/list.o ob/search.o ob/setcoy.o ob/setdel.o \
     ob/ask.o ob/config.o ob/write.o ob/screen.o ob/bgag.o ob/compose.o \
     ob/view.o ob/tagline.o ob/lcore.o ob/setfk.o ob/clip.o

# special bullshit version that sort of runs under dos 1.3, sometimes:
OBS3 = ob/who.3o ob/main.3o ob/pack.3o ob/load.3o ob/door.3o ob/trash.3o \
     ob/blue.3o ob/qwk.3o ob/list.3o ob/search.3o ob/setcoy.3o ob/setdel.3o \
     ob/ask.3o ob/config.3o ob/write.3o ob/screen.3o ob/bgag.3o ob/compose.3o \
     ob/view.3o ob/tagline.3o ob/lcore.3o ob/setfk.3o ob/clip.3o

AOBS = ob/who.abo ob/main.abo $(OBSB)

ATOBS = ob/who.abto ob/main.abto $(OBSB)

BOBS = ob/who.bo ob/main.bo $(OBSB)

BTOBS = ob/who.bto ob/main.bto $(OBSB)

PBTOBS = ob/who.pbto ob/main.pbto $(OBSB)

TOBS = ob/who.to ob/main.to $(OBSM)

MOBS = ob/who.o ob/main.o $(OBSM)


PRE = ######-hi QDUMP
## To force compiling without precompiled headers, go "make PRE=".
## For no inline library calls either, go "make PRE=-d__NO_PRAGMAS".

CFLAGS = -pe -wcpru -smabfnpu $(PRE)
## CCOPTS includes all that plus -qf
BUGFLAGS = -pe -wcpru -smabpu -bs -s0f0n $(PRE) #-bd
## -bs -s0f0n is for the source level debugger, -bd calls the stack check hook

L = -o ram:Q-Blue -lc


a :   env:q-blue-a

at :  env:q-blue-v

13 :  env:q-blue-3

b :   env:q-blue-b

bt :  env:q-blue-u

pbt : env:q-blue-p

t :   env:q-blue-t

f :   env:q-blue-f

m :   env:q-blue-m

env\:q-blue-a : $(AOBS)
	-@delete > nil: env:q-blue-?
	ln -wgm +ssq $(AOBS) $L
	-@echo > env:q-blue-a ""
	-@dr -l ram:Q-Blue\#?

env\:q-blue-v : $(ATOBS)
	-@delete > nil: env:q-blue-?
	ln -wgm +ssq $(ATOBS) $L
	-@echo > env:q-blue-v ""
	-@dr -l ram:Q-Blue\#?

env\:q-blue-3 : $(OBS3)
	-@delete > nil: env:q-blue-?
	ln -wgm +ssq $(OBS3) $L
	-@echo > env:q-blue-3 ""
	-@dr -l ram:Q-Blue\#?

env\:q-blue-b : $(BOBS)
	-@delete > nil: env:q-blue-?
	ln -m +ssq $(BOBS) $L
	-@echo > env:q-blue-b ""
	-@dr -l ram:Q-Blue\#?

env\:q-blue-u : $(BTOBS)
	-@delete > nil: env:q-blue-?
	ln -m +ssq $(BTOBS) $L
	-@echo > env:q-blue-u ""
	-@dr -l ram:Q-Blue\#?

env\:q-blue-p : $(PBTOBS)
	-@delete > nil: env:q-blue-?
	ln -m +ssq $(PBTOBS) $L
	-@echo > env:q-blue-p ""
	-@dr -l ram:Q-Blue\#?

env\:q-blue-t : $(TOBS)
	-@delete > nil: env:q-blue-?
	ln -m +ssq $W $(TOBS) $L
	-@echo > env:q-blue-t ""
	-@dr -l ram:Q-Blue\#?

env\:q-blue-f : $(MOBS)
	-@delete > nil: env:q-blue-?
	ln -m +ssq $(MOBS) $L
	-@echo > env:q-blue-f ""
	-@dr -l ram:Q-Blue\#?
#	-@echo " ******  Not registrated  ******"

env\:q-blue-m : $(MOBS)
	-@delete > nil: env:q-blue-?
	ln -wm +ssq $(MOBS) $L
	-@echo > env:q-blue-m ""
	-@dr -l ram:Q-Blue\#?
#	Registrate ram:Q-Blue 1 Paul Kienitz


.c.o :
	cc $(CFLAGS) -o $@ $<

.c.to :
	cc $(CFLAGS) -d TRY -o $@ $<

.c.3o :
	cc $(BUGFLAGS) -d TEST13 -d ALPHA -d BETA -d __NO_PRAGMAS -o $@ $<

.c.bo :
	cc $(BUGFLAGS) -d BETA -o $@ $<
	
.c.bto :
	cc $(BUGFLAGS) -d BETA -d TRY -o $@ $<

.c.abo :
	cc $(BUGFLAGS) -d ALPHA -d BETA -o $@ $<

.c.abto :
	cc $(BUGFLAGS) -d ALPHA -d BETA -d TRY -o $@ $<

.c.pbto :
	cc $(BUGFLAGS) -d PUBLIC_BETA -d TRY -o $@ $<

# SPECIAL CASES FOR VERSION 2.0a ONLY:
# ob/who.o : who.c
#	cc $(CFLAGS) -d VERSIONTAIL="a" -o $@ who.c
#
# ob/who.to : who.c
#	cc $(CFLAGS) -d TRY -d VERSIONTAIL="a" -o $@ who.c

ob/stch.o : scraps/stch.c
	cc $(CFLAGS) -b0d -o $@ scraps/stch.c


$(OBSM) $(OBSB) : qblue.h qbs.h ######QDUMP

$(OBS3) ob/main.o ob/main.3o ob/main.bo ob/main.to ob/main.bto \
    ob/main.abto ob/main.pbto ob/main.abo : qblue.h qbs.h ######QDUMP

ob/screen.o ob/screen.bo ob/blue.o ob/blue.bo \
    ob/write.o ob/write.bo ob/ask.o ob/ask.bo : version.h

ob/who.o ob/who.3o ob/who.bo ob/who.to ob/who.bto \
    ob/who.abto ob/who.pbto ob/who.abo : version.h env:VersionDate

env\:VersionDate :
	VersionDate > env:VersionDate

ob/compose.o ob/compose.bo ob/ask.o ob/ask.bo ob/pack.o ob/pack.bo \
    ob/list.o ob/list.bo ob/door.o ob/door.bo ob/bgag.o ob/bgag.bo \
    ob/setcoy.o ob/setcoy.bo ob/setdel.o ob/setdel.bo ob/config.o \
    ob/setfk.o ob/setfk.bo ob/lcore.o ob/lcore.bo ob/screen.o ob/screen.bo \
    ob/config.bo ob/search.o ob/search.bo ob/tagline.o ob/tagline.bo : pigment.h

# We'll leave out the dependency on bluewave.h ... don't recompile when updated


QDUMP : qbs.h dumpQ.h   # bluewave.h inc:Paul.h inc:IIfiles.h
	cc -pe -wcpru -sabfmnpu -ho QDUMP -a -o nil: dumpQ.h

# We are currently compiling QDUMP to include exec/memory.h, dos/dos.h,
# intuition/intuition.h, and qbs.h.  That pulls in Paul.h and bluewave.h.

# Some timing results for different precompiled headers on setcoy.c:
#    no header:                  58 seconds
#    intuition/intuition.h only: 55 seconds, identical CRC
#    generic header II:          47 seconds, differing CRC
#    generic header INTU:        33 seconds, another differing CRC
#    QDUMP with GRAB_IT_ALL:     28 seconds, yet another different CRC
#    smaller QDUMP:              28 seconds, still another different CRC
# Note that with no precompiled headers, the compiler consumes less memory.
# As we get further down into the differing CRC's, the likelihood of the
# compiler either having enforcer hits or producing output that chokes the
# linker, or both, increases.  So for now we use no precompiled headers.
