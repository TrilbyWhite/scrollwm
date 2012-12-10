/* See LICENSE file for copyright and license details. */

static const char font[] = "-misc-fixed-medium-r-normal--13-120-75-75-c-70-*-*";
//static int barheight = 0;

#define SCROLLWM_CURSOR		XC_left_ptr

static const char colors[LASTColor][9] = {
	[Background]	= "#101010",
	[Normal]		= "#686868",
	[Hidden]		= "#AA68AA",
	[Inactive]		= "#6868AA",
	[Active]		= "#68AADD",
};

#define ZOOM_MIN	0.2
#define WIN_MIN		30
#define ZOOM_UP		1.1
#define ZOOM_DOWN	0.89

#define DMENU		"dmenu_run -fn \"-*-terminus-bold-r-*--12-120-72-72-c-60-*-*\" -nb \"#101010\" -nf \"#484862\" -sb \"#080808\" -sf \"#FFDD0E\""
#define TERM		"urxvt" 		/* or "urxvtc","xterm","terminator",etc */
#define CMD(app)	app "&"

/* key definitions */
#define MODKEY Mod4Mask
static Key keys[] = {
	/* modifier			key			function	argument */
	/* launchers: */
	{ MODKEY,			XK_Return,	spawn,		CMD(TERM)		},
	{ MODKEY,			XK_p,		spawn,		CMD(DMENU)		},
	{ MODKEY,			XK_w,		spawn,		CMD("luakit")	},
};

// vim: ts=4
