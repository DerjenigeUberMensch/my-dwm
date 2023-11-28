/* See LICENSE file for copyright and license details.
 * 4 Tab spaces; No tab characters use spaces for tabs
 * Basic overview of dwm => https://ratfactor.com/dwm
 */
/* appearance */
static char WM_NAME[]               = "dwm.exe"; /* wm name displayed when using X (type neofetch to see this) */

static unsigned int borderpx        = 0;    /* border pixel of windows                                          */
static unsigned int snap            = 15;   /* snap window to border in pixels; 0 to disable (NOT RECOMMENDED)  */
static unsigned int windowrate      = 120;  /* max refresh rate when resizing, moving windows; set 0 to disable */
static int hoverfocus               = 0;    /* 1 on mouse hover focus that window; 0 to disable                 */

static int barpadding               = 0;    /* padding in pixels (both sides)                   */
static int showbar                  = 1;    /* 1 to show bar; 0 to disable                      */
static const int topbar             = 0;    /* 0 for bottom bar                                 */
static const int fastinputbar       = 0;    /* prioritizes input over bar render; 0 to disable  */
static const int ICONSIZE           = 16;            /* icon size */
static const int ICONSPACING        = 5;          /* space between icon and title */
static const char *fonts[]          = {"monospace:size=12" };
static const char dmenufont[]       = "monospace:size=12";

/* alt-tab configuration */
static unsigned int tabmodkey       = 0x40;	/* if this key is hold the alt-tab functionality stays acitve. This key must be the same as key that is used to active functin altTabStart `*/
static unsigned int tabcyclekey     = 0x17;	/* if this key is hit the alt-tab program moves one position forward in clients stack. This key must be the same as key that is used to active functin altTabStart */
static unsigned int tabposx         = 1;	/* tab position on X axis, 0 = left, 1 = center, 2 = right  */
static unsigned int tabposy 		= 1;	/* tab position on Y axis, 0 = bottom, 1 = center, 2 = top  */
static unsigned int tabtextposx     = 1;    /* tab position on x axis, 0 = left , 1 = center, 2 = right */
static unsigned int maxWTab 		= 600;	/* MAX tab menu width                                       */
static unsigned int maxHTab 		= 200;	/* MAX tab menu height                                      */
static unsigned int minWidthDraw    = 0;    /* Add padding if text length is shorter; 0 to disable      */
static unsigned int tabshowpreview  = 1;    /* shows window preview when alt tabbing */

static char col_black[]     = "#000000";
static char col_white[]     = "#ffffff";

static char col_grey[]      = "#C0C0C0";
static char col_red[]       = "#ff0000";
static char col_pink[]      = "#FF00FF";
static char col_blue[]      = "#0000FF";
static char col_yellow[]    = "#FFFF00";
/* static char col_term_blue[]   = "#ecffff"; */
static char *colors[][3] = {
    /*					fg         bg          border   */
    /*                  fg=textColour                   */
    [SchemeNorm]        = { col_white, col_black, col_white },
    [SchemeSel]         = { col_white, col_black, col_white },
    [SchemeUrgent]      = { col_blue,  col_red,   col_blue },
    [SchemeWarn]        = { col_white, col_yellow, col_white},

    [SchemeAltTab]      = { col_white, col_black, col_black },
    [SchemeAltTabSelect]= { col_black, col_white, col_white},

    [SchemeBarTabActive]   = { col_black, col_white, col_white},
    [SchemeBarTabInactive] = { col_white, col_black, col_black },
    [SchemeTagActive]      = { col_black, col_white, col_white},
    [SchemeTagInactive]    = { col_white, col_black, col_black }

};
/* tagging */
static const char *tags[] = { "1", "2", "3", "4", "5", "6", "7", "8", "9" };

static const Rule rules[] = {
    /* xprop(1):
     *	WM_CLASS(STRING) = instance, class
     *	WM_NAME(STRING) = title
     */
    /* class      instance    title       tags mask     isfloating   monitor */
    { "Gimp",     NULL,       NULL,       0,            1,           -1 },
};

/* Bartab */
static const int BARTAB_BORDERS = 1;                /* 1 to show borders; 0 to disable          */
static const int BARTAB_BOTTOMBORDER = 1;           /* 1 to show bottom border; 0 to disable    */
static const int BARTAB_TAGSINDICATOR = 1;          /* 1 to show tags in tabs; 0 to disable     */
static const int CENTER_TAB_GROUP_TEXT = 1;         /* 1 to enable; 0 to disable                */
static const int BARTAB_TAGSPX = 5;                 /* # pixels for tag grid boxes              */
static const int BARTAB_TAGSROWS= 3;                /* # rows in tag grid (9 tags, e.g. 3x3)    */
static const int BARTAB_SHARE_GROUP_COL = 0;        /* 1 same window type share same colour; 0 to disable  (recommended) */
static void (*bartabmonfns[])(Monitor *) = { monocle /* , customlayoutfn */ };
static void (*bartabfloatfns[])(Monitor *) = { NULL /* , customlayoutfn */ };

/* layout(s) */
static const float mfact     = 0.55; /* factor of master area size [0.05..0.95]      */
static const int nmaster     = 1;    /* number of clients in master area             */
static const int resizehints = 1;    /* 1 means respect size hints in tiled resizals */
static const int lockfullscreen = 1; /* 1 will force focus on the fullscreen window  */

static const Layout layouts[] = {
    /* symbol     arrange function */
    { "[T]",      tile },    /* first entry is default */
    { "[F]",      NULL },    /* no layout function means floating behavior */
    { "[M]",      monocle },
    { "[G]",      grid },
};
/* key definitions */
#define ALT     Mod1Mask
#define NUMLOCK Mod2Mask
#define SUPER   Mod4Mask /* windows key */
#define CTRL    ControlMask
#define SHIFT   ShiftMask
#define TAB     XK_Tab
/* #define ISO_LEVEL5_SHIFT Mod3Mask
 * #define ISO_LEVEL3_SHIFT Mod5Mask */

/* TAGGING IS NOT IMPLEMENT IN THIS BUILD OF DWM and is left for user discretion */
#define TAGKEYS(KEY,TAG) \
{ SUPER,                       KEY,      view,           {.ui = 1 << TAG} }, \
{ SUPER|CTRL,                  KEY,      toggleview,     {.ui = 1 << TAG} }, \
{ SUPER|SHIFT,                 KEY,      tag,            {.ui = 1 << TAG} }, \
{ SUPER|CTRL|SHIFT,            KEY,      toggletag,      {.ui = 1 << TAG} },

/* helper for spawning shell commands in the pre dwm-5.0 fashion */
#define SHCMD(cmd) { .v = (const char*[]){ "/bin/sh", "-c", cmd, NULL } }

/* Multimedia Keys (laptops and function keyboards) */
static const char *up_vol[]   = { "pactl", "set-sink-volume", "@DEFAULT_SINK@", "+1%",   NULL };
static const char *down_vol[] = { "pactl", "set-sink-volume", "@DEFAULT_SINK@", "-1%",   NULL };
static const char *mute_vol[] = { "pactl", "set-sink-mute",   "@DEFAULT_SINK@", "toggle", NULL };
static const char *pause_vol[]= { "playerctl", "play-pause"};
static const char *next_vol[] = { "playerctl", "next"};
static const char *prev_vol[] = { "playerctl", "previous"};
static const char *brighter[] = { "brightnessctl", "set", "1%+", NULL };
static const char *dimmer[]   = { "brightnessctl", "set", "1%-", NULL };

/* commands */
static char dmenumon[2] = "0"; /* component of dmenucmd, manipulated in spawn() */
static const char *dmenucmd[] = { "dmenu_run", "-m", dmenumon, "-fn", dmenufont, "-nb", col_black, "-nf", col_white, "-sb", col_black, "-sf", col_white, topbar ? NULL : "-b", fastinputbar ? "-f" : NULL, NULL }; /* flags -b == bottom bar; -f == getkeyboard input first then handle request; */
static const char *termcmd[]        = { "st", NULL };
static const char *screenshotcmd[]  = {"scrot","-d3", "$HOME/Pictures/Screenshots/%Y-%m-%d-%T-screenshot.png", NULL}; /*doesnt work*/

static const Key keys[] = {
    /* modifier                     key        function        argument */
    { SUPER,                        XK_n,       tester,         {0} },
    { SUPER,                        XK_d,       spawn,          {.v = dmenucmd } },
    { SUPER,                        XK_Return,  spawn,          {.v = termcmd } },
    { SUPER,                        XK_Print,   spawn,          {.v = screenshotcmd } },
    { SUPER,                        XK_b,       togglebar,      {0} },
    { SUPER,                        XK_q,	    view,           {0} },
    { SUPER|SHIFT,                  XK_q,       killclient,     {0} },
    { CTRL|ALT,                     XK_q,	    forcekillclient,{0} },
    { SUPER,                        XK_w,       togglemaximize, {0} },
    { SUPER|SHIFT,                  XK_p,       quit,           {0} },
    { SUPER,                        XK_z,       setlayout,      {.v = &layouts[0]} },/* TILED    */
    { SUPER,                        XK_x,       setlayout,      {.v = &layouts[1]} },/* FLOATING */
    { SUPER,                        XK_c,       setlayout,      {.v = &layouts[2]} },/* MONOCLE  */
    { SUPER,                        XK_g,       setlayout,      {.v = &layouts[3]} },/*GRID*/
    { SUPER,                        XK_o,       winview,        {0} }, /*broken*/
    { 0,                            XK_F11,     togglefullscr,  {0} },

    { ALT,             		        TAB,        altTabStart,	{0} },

    { 0, XF86XK_AudioMute,                      spawn, {.v = mute_vol } },
    { 0, XF86XK_AudioLowerVolume,               spawn, {.v = down_vol } },
    { 0, XF86XK_AudioRaiseVolume,               spawn, {.v = up_vol } },
    { 0, XF86XK_MonBrightnessDown,              spawn, {.v = dimmer } },
    { 0, XF86XK_MonBrightnessUp,                spawn, {.v = brighter } },
    { 0, XF86XK_AudioPlay,                      spawn, {.v = pause_vol } },
    { 0, XF86XK_AudioPause,                     spawn, {.v = pause_vol } },
    { 0, XF86XK_AudioNext,                      spawn, {.v = next_vol } },
    { 0, XF86XK_AudioPrev,                      spawn, {.v = prev_vol } },

    TAGKEYS(                        XK_1,                      0)
    TAGKEYS(                        XK_2,                      1)
    TAGKEYS(                        XK_3,                      2)
    TAGKEYS(                        XK_4,                      3)
    TAGKEYS(                        XK_5,                      4)
    TAGKEYS(                        XK_6,                      5)
    TAGKEYS(                        XK_7,                      6)
    TAGKEYS(                        XK_8,                      7)
    TAGKEYS(                        XK_9,                      8)
};

/* button definitions */
/* click can be ClkTagBar, ClkLtSymbol, ClkStatusText, ClkWinTitle, ClkClientWin, or ClkRootWin */
static const Button buttons[] = {
    /* click                event mask      button          function        argument */
    { ClkLtSymbol,          0,              Button1,        setlayout,      {0} },
    { ClkLtSymbol,          0,              Button3,        setlayout,      {.v = &layouts[2]} },
    { ClkWinTitle,          0,              Button2,        zoom,           {0} },
    { ClkStatusText,        0,              Button2,        spawn,          {.v = termcmd } },
    { ClkClientWin,         SUPER,          Button1,        movemouse,      {0} },
    { ClkClientWin,         SUPER,          Button2,        togglefloating, {0} },
    { ClkClientWin,         SUPER,          Button3,        resizemouse,    {0} },
    { ClkTagBar,            0,              Button1,        view,           {0} },
    { ClkTagBar,            0,              Button3,        toggleview,     {0} },
    { ClkTagBar,            SUPER,          Button1,        tag,            {0} },
    { ClkTagBar,            SUPER,          Button3,        toggletag,      {0} },
};
