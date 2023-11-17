void display_value(char *s, int debugW, int debugH)
{
    int wX, wY;
    int wW;
    int wH;
    Monitor *m = selmon;

    XSetWindowAttributes wa =
    {
        .override_redirect = True,
        .background_pixmap = ParentRelative,
        .event_mask = ButtonPressMask|ExposureMask
    };
    wX = m->mx + (m->mw >> 1) - (debugW >> 1);
    wY = m->my + (m->mh >> 1) - (debugH >> 1)
    if(showbar)
        wH+=bh;
    m->debug;
}
