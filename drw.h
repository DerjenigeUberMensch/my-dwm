#ifndef DRW_H_
#define DRW_H_
/* See LICENSE file for copyright and license details. */
#include <X11/Xft/Xft.h>
#include <X11/Xcursor/Xcursor.h>

enum { ColFg, ColBg, ColBorder }; /* Clr scheme index */

typedef struct Cur Cur;
typedef struct XCur XCur;
typedef struct Fnt Fnt;
typedef struct Drw Drw;

typedef XftColor Clr;

struct Cur
{
    Cursor cursor;
    XcursorImage *img;
};
struct Fnt
{
    Display *dpy;
    unsigned int h;
    XftFont *xfont;
    FcPattern *pattern;
    struct Fnt *next;
};
struct Drw
{
    unsigned int w, h;
    Display *dpy;
    int screen;
    Window root;
    Visual *visual;
    unsigned int depth;
    Colormap cmap;
    Drawable drawable;
    Picture picture;
    GC gc;
    Clr *scheme;
    Fnt *fonts;
};

/* Drawable abstraction */
extern Drw *drw_create(Display *dpy, int screen, Window win, unsigned int w, unsigned int h);
extern void drw_resize(Drw *drw, unsigned int w, unsigned int h);
extern void drw_free(Drw *drw);

/* Fnt abstraction */
extern Fnt *drw_fontset_create(Drw* drw, const char *fonts[], size_t fontcount);
extern void drw_fontset_free(Fnt* set);
extern unsigned int drw_fontset_getwidth(Drw *drw, const char *text);
extern unsigned int drw_fontset_getwidth_clamp(Drw *drw, const char *text, unsigned int n);
extern void drw_font_getexts(Fnt *font, const char *text, unsigned int len, unsigned int *w, unsigned int *h);

/* Colorscheme abstraction */
extern void drw_clr_create(Drw *drw, Clr *dest, const char *clrname);
extern Clr *drw_scm_create(Drw *drw, char *clrnames[], size_t clrcount);

/* Cursor abstraction */
extern Cur *drw_cur_create(Drw *drw, int shape);
/* create cur from image */
extern Cur *drw_cur_create_img(Drw *drw, const char *img);
extern void drw_cur_free(Drw *drw, Cur *cursor);

/* Drawing context manipulation */
extern void drw_setfontset(Drw *drw, Fnt *set);
extern void drw_setscheme(Drw *drw, Clr *scm);

/* Drawing functions */
extern void drw_rect(Drw *drw, int x, int y, unsigned int w, unsigned int h, int filled, int invert);
extern void drw_line(Drw *drw, int x1, int x2, int y1, int y2);
extern void drw_arc(Drw *drw, int x, int y, int w, int h, int angle1, int angle2);
extern int drw_text(Drw *drw, int x, int y, unsigned int w, unsigned int h, unsigned int lpad, const char *text, int invert);

/* Map functions */
extern void drw_map(Drw *drw, Window win, int x, int y, unsigned int w, unsigned int h);

#endif
