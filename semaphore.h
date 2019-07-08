/* Stuff for avoiding collisions between multiple instances of Q-Blue:       */

/* We tell Q-Blue 1.0 QSemNodes from 1.9 and later QSemNodes by byte 19 of   */
/* the public screen name.  It is always 0 for 1.0 and is nonzero otherwise. */

struct QSemNode {
    struct MinNode node;
    str workdinst, replydinst, editrepf, editquof, screename;
    bool packetopen, anyrepliesyet, editingdirs, editingeditor, composingnow;
/* fields from here on must not be referenced when !screename[19]: */
    ushort qversion;		/* same number used in config file */
    short instancenum;		/* negative means not selected yet */
    bool fakeopen;		/* means that workd is not really in use */
/* fields from here on must not be referenced unless qversion >= 5: */
    str rawworkd, rawreplyd;
};
/* We'll allow the context dir to be shared by all processes. */

/* bits returned by CheckSemaphoreOverlap(): */
#define SOF_MAYBEWORK   0x0001
#define SOF_INUSEWORK   0x0002
#define SOF_MAYBEREPLY  0x0004
#define SOF_EMPTYREPLY  0x0008
#define SOF_INUSEREPLY  0x0010
#define SOF_EDITINFILE  0x0020
#define SOF_EDITOUTFILE 0x0040
#define SOF_EDITORINUSE 0x0080
#define SOF_PUBSCRNAME  0x0100
#define SOF_INSTANCENUM 0x0200
