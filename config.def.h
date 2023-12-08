/* See LICENSE file for copyright and license details.
 * 4 Tab spaces; No tab characters use spaces for tabs
 * Basic overview of dwm => https://ratfactor.com/dwm
 */
/* after many many hours this only saved .2MiB of ram, what a waste */
struct Config
{
    /* BIT SIZES
     * max screen size         14
     * bool                     1 
     * tiny ints                2
     * small ints               5
     * medium ints             10
     */
    int tabmodkey :                 14;
    int tabcyclekey :               14;

    unsigned int borderpx :         14;
    unsigned int snap :             14;
    unsigned int barpadding :       14;
    unsigned int iconsz :           14;
    unsigned int iconspace :        14;
    unsigned int maxwtab:           14;
    unsigned int maxhtab:           14;
    unsigned int bttagpx :          14;
    unsigned int minwdraw :         14;


    unsigned int windowrate :       10;
    unsigned int nmaster :          10;
    unsigned int resizehints :      10;
    unsigned int lockfullscreen :   10;


    unsigned int bttagrow :          5;
    unsigned int scalepreview :      5;

    unsigned int tabposx :           2;
    unsigned int tabposy :           2;
    unsigned int tabtextposx :       2;


    unsigned int hoverfocus :        1;
    unsigned int showbar :           1;
    unsigned int previewbar :        1;
    unsigned int tabshowpreview :    1;
    unsigned int tabtextposxf :      1;
    unsigned int ignoredecorhints :  1;

    float mfact;
};

static struct Config cfg;

void initcfg()
{
    cfg.borderpx        = 0;    /* border pixel of windows                                          */
    cfg.snap            = 15;   /* snap window to border in pixels; 0 to disable (NOT RECOMMENDED)  */
    cfg.windowrate      = 120;  /* max refresh rate when resizing, moving windows; set 0 to disable */
    cfg.hoverfocus      = 0;    /* 1 on mouse hover focus that window; 0 to disable                 */

    cfg.barpadding      = 0;    /* padding in pixels (both sides)                                   */
    cfg.showbar         = 1;    /* 1 to show bar; 0 to disable                                      */
    cfg.iconsz          = 16;   /* icon size                                                        */
    cfg.iconspace       = 2;    /* space between icon and title                                     */

    /* alt-tab configuration */
    cfg.tabmodkey       = 0x40;	/* if this key is hold the alt-tab functionality stays acitve. This key must be the same as key that is used to active functin altTabStart `                                          */
    cfg.tabcyclekey     = 0x17;	/* if this key is hit the alt-tab program moves one position forward in clients stack. This key must be the same as key that is used to active functin altTabStart                    */
    cfg.scalepreview    = 4;    /* preview scaling (display w and h / scalepreview)                 */
    cfg.previewbar      = 1;    /* show the bar in the preview window                               */
    cfg.tabposx         = 1;	/* tab position on X axis, 0 = left, 1 = center, 2 = right          */
    cfg.tabposy 		= 1;	/* tab position on Y axis, 0 = bottom, 1 = center, 2 = top          */
    cfg.tabtextposx     = 1;    /* tab position on x axis, 0 = left , 1 = center, 2 = right         */
    cfg.tabtextposxf    = 0;    /* 0 Left, 1 Right; set tabtextposx = -1 to enable;                 */
    cfg.maxwtab         = 600;	/* MAX tab menu width                                               */
    cfg.maxhtab         = 200;	/* MAX tab menu height                                              */
    cfg.minwdraw        = 0;    /* Add padding if text length is shorter; 0 to disable              */
    cfg.tabshowpreview  = 1;    /* shows window preview when alt tabbing                            */

    cfg.mfact           = 0.55; /* factor of master area size [0.05..0.95]                          */

    cfg.nmaster         = 1;    /* number of clients in master area                                 */
    cfg.resizehints     = 1;    /* 1 means respect size hints in tiled resizals                     */
    cfg.lockfullscreen  = 1;    /* 1 will force focus on the fullscreen window                      */
    cfg.ignoredecorhints= 0;    /* 1 ignore hints made by windows for window decorations; 0 to disable */

}
static const short topbar       = 0; /* 0 for bottom bar                                            */
static const short fastinputbar = 0; /* prioritizes input over bar render; 0 to disable             */

static char WM_NAME[]           = "dwm.exe"; /* wm name displayed when using X (type neofetch to see this) */

static const char *fonts[]      =     {"monospace:size=12" };
static const char dmenufont[]   =    {"monospace:size=12"};


/* COLOURS */
static char col_black[]     = "#000000";
static char col_white[]     = "#ffffff";

static char col_grey[]      = "#C0C0C0";
static char col_red[]       = "#ff0000";
static char col_pink[]      = "#FF00FF";
static char col_blue[]      = "#0000FF";
static char col_yellow[]    = "#FFFF00";
/* static char col_term_blue[]   = "#ecffff"; */
static char *colors[][3] = 
{
    /*					fg         bg          border   */
    /*                  fg=textColour                   */
    [SchemeNorm]            = { col_white, col_black, col_white },
    [SchemeSel]             = { col_white, col_black, col_white },
    [SchemeUrgent]          = { col_blue,  col_red,   col_blue },
    [SchemeWarn]            = { col_white, col_yellow, col_white},

    [SchemeAltTab]          = { col_white, col_black, col_black },
    [SchemeAltTabSelect]    = { col_black, col_white, col_white},

    [SchemeBarTabActive]    = { col_black, col_white, col_white},
    [SchemeBarTabInactive]  = { col_white, col_black, col_black },
    [SchemeTagActive]       = { col_black, col_white, col_white},

};
/* appearance */
/* tagging */
static const char *tags[] = { "1", "2", "3", "4", "5", "6", "7", "8", "9" };
static const char *tagsel[][2] = {
    /* fg       bg */
    {col_white, col_black},
    {col_white, col_black},
    {col_white, col_black},
    {col_white, col_black},
    {col_white, col_black},
    {col_white, col_black},
    {col_white, col_black},
    {col_white, col_black},
    {col_white, col_black},
};
static const Rule rules[] = 
{
    /* xprop(1):
     *	WM_CLASS(STRING) = instance, class
     *	WM_NAME(STRING) = title
     */
    /* class      instance    title       tags mask     isfloating   monitor */
    { "Gimp",     NULL,       NULL,       0,            1,           -1 },
};

/* Bartab */
static void (*bartabmonfns[])(Monitor *) = { monocle /* , customlayoutfn */ };
static void (*bartabfloatfns[])(Monitor *) = { NULL /* , customlayoutfn */ };

/* layout(s) */

static const Layout layouts[] = 
{
    /* symbol     arrange function */
    { "[T]",      tile },    /* first entry is default */
    { "[F]",      NULL },    /* no layout function means floating behavior */
    { "[M]",      monocle },
    { "[G]",      grid },

};
