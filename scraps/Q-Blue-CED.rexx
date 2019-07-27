/* Q-Blue-CED.rexx: load two files into CygnusEd from Q-Blue, */
/* replacing any existing files with the same name */

parse arg replyfile quotefile .

if ~show('Ports', 'rexx_ced') then
    address command 'Ed'        /* CED's front end program */
address 'rexx_ced'

if quotefile ~= '' then
    call Q_load_proc(quotefile)
call Q_load_proc(replyfile)

'Set Right Border 75'          /* affects replyfile */
'CedToFront'                   /* pop up screen */
exit 0

Q_load_proc: procedure
    arg filename
    'Jump To' filename         /* see if already loaded */
    if rc == 0 then do
        'Open New'             /* create new view */
        if rc ~= 0 then
            'Open' filename
    end; else
        'Open 1' filename      /* replace existing contents */
    return

/********************************* ALTERNATIVE VERSION:

parse arg replyfile quotefile .

if quotefile ~= '' then
    address command 'Ed' quotefile '-o'
address command 'Ed' replyfile '-o'

'Set Right Border 75'
'CedToFront'
********************************************************/
