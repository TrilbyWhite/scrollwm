/* See LICENSE file for copyright and license details. */

static const char font[] =  "-misc-fixed-medium-r-normal--13-120-75-75-c-70-*-*";
static const char *tag_name[] = {"one", "two", "three", "four", "five", NULL};
static const char *tile_modes[] = {"ttwm", "rstack", "bstack", "flow", NULL};

static const char colors[LASTColor][9] = {
	[Background]	= "#101010",
	[Default]		= "#686868",
	[Target]		= "#68E080",
	[Hidden]		= "#FFAA48",
	[Normal]		= "#68B0E0",
	[Sticky]		= "#288428",
	[Urgent]		= "#FF8880",
	[Title]			= "#DDDDDD",
	[TagList]		= "#424242",
};

static const float	zoom_min			= 0.2;
static const float	win_min				= 30;
static const float	zoom_up				= 1.1;
static const float	zoom_down			= 0.9;
static const char 	scrollwm_cursor		= XC_left_ptr;
static Bool			showbar				= True;
static Bool			topbar				= True;
static const Bool	focusfollowmouse	= False;
static const Bool	highlightfocused	= True;
static const Bool	scrolltofocused		= True;
static const Bool	animations			= True;
static const int	animatespeed		= 18;
static const int	borderwidth			= 1;
static const int	tilegap				= 4;
static int			tilebias			= 0;

#define DMENU		"dmenu_run -fn \"-misc-fixed-medium-r-normal--13-120-75-75-c-70-*-*\" -nb \"#101010\" -nf \"#484862\" -sb \"#080808\" -sf \"#FFDD0E\""
#define TERM		"urxvt" 		/* or "urxvtc","xterm","terminator",etc */
#define CMD(app)	app "&"

/* key definitions */
#define MOD1 Mod4Mask
#define MOD2 Mod1Mask
static Key keys[] = {
	/* modifier				key			function	argument */
	/* launchers + misc: */
	{ MOD1,				XK_e,		switcher,	NULL			},
	{ MOD1,				XK_Return,	spawn,		CMD(TERM)		},
	{ MOD1,				XK_p,		spawn,		CMD(DMENU)		},
	{ MOD1,				XK_w,		spawn,		CMD("luakit")	},
	{ MOD1|ShiftMask,	XK_q,		quit,		NULL			},
	{ MOD2,				XK_F4,		killclient,	NULL			},
	{ MOD1,				XK_f,		fullscreen,	NULL			},
	/* checkpoints */
	{ MOD1,				XK_c,		checkpoint,			NULL	},
	{ MOD1|MOD2,		XK_c,		checkpoint_set,		NULL	},
	/* tiling bindings	*/
	{ MOD1,				XK_space,	cycle_tile,	NULL			},
	{ MOD1,				XK_i,		tile,		"increase"		},
	{ MOD1,				XK_d,		tile,		"decrease"		},
	{ MOD1|MOD2,		XK_t,		tile,		"ttwm"			},
	{ MOD1|MOD2,		XK_r,		tile,		"rstack"		},
	{ MOD1|MOD2,		XK_b,		tile,		"bstack"		},
	{ MOD1|MOD2,		XK_f,		tile,		"flow"			},
	{ MOD1|MOD2,		XK_m,		monocle,	NULL			},
	{ MOD1,				XK_s,		target,		"screen"		},
	{ MOD1,				XK_t,		target,		"tag"			},
	{ MOD1,				XK_v,		target,		"visible"		},
	/* desktop movement */
	{ MOD1,				XK_Down,	move,		"DOWN"			},
	{ MOD1,				XK_Up,		move,		"UP"			},
	{ MOD1,				XK_Left,	move,		"LEFT"			},
	{ MOD1,				XK_Right,	move,		"RIGHT"			},
	{ MOD1,				XK_j,		move,		"down"			},
	{ MOD1,				XK_k,		move,		"up"			},
	{ MOD1,				XK_h,		move,		"left"			},
	{ MOD1,				XK_l,		move,		"right"			},
	/* window cycling */
	{ MOD1,				XK_Tab, 	cycle,		NULL			},
	{ MOD1|ShiftMask,	XK_Tab, 	cycle,		"screen"		},
	{ MOD1|MOD2,		XK_Tab, 	cycle,		"tag"			},
	{ MOD1|ControlMask,	XK_Tab, 	cycle,		"visible"		},
	/* select tag */
	{ MOD1,				XK_1,		tag,		"1"		},
	{ MOD1,				XK_2,		tag,		"2"		},
	{ MOD1,				XK_3,		tag,		"3"		},
	{ MOD1,				XK_4,		tag,		"4"		},
	{ MOD1,				XK_5,		tag,		"5"		},
	/* tag operations: hide-others, hidden,  sticky, normal(unstick+unhide) */
	{ MOD1|MOD2,		XK_o,		tagconfig,	"others"},
	{ MOD1|MOD2,		XK_h,		tagconfig,	"hide"	},
	{ MOD1|MOD2,		XK_s,		tagconfig,	"stick"	},
	{ MOD1|MOD2,		XK_n,		tagconfig,	"normal"},
	/* assign/remove a window to/from a tag */
	{ MOD1|MOD2,		XK_1,		toggletag,	"1"				},
	{ MOD1|MOD2,		XK_2,		toggletag,	"2"				},
	{ MOD1|MOD2,		XK_3,		toggletag,	"3"				},
	{ MOD1|MOD2,		XK_4,		toggletag,	"4"				},
	{ MOD1|MOD2,		XK_5,		toggletag,	"5"				},
	/* toggle statusbar */
	{ MOD1,				XK_a,		tagconfig,	"togglebar"		},
	{ MOD1,				XK_x,		tagconfig,	"movebar"		},
};

/* mouse buttons with no modifiers only work when triggered */
/* with the	mouse pointer over the desktop					*/
static Button buttons[] = {
	/* modifier			button		function 	arg */
	{0,					1,			window,		"move"		},
	{0,					2,			cycle_tile,	NULL		},
	{0,					3,			window,		"resize"	},
	{0,					4,			monocle,	NULL		},
//	{0,					5,			UNASSIGNED,	NULL		},
	{MOD1,				1,			window,		"move"		},
	{MOD1,				2,			window,		"zoom"		},
	{MOD1,				3,			window,		"resize"	},
	{MOD1,				4,			window,		"grow"		},
	{MOD1,				5,			window,		"shrink"	},
	{MOD1|MOD2,			1,			desktop,	"move"		},
	{MOD1|MOD2,			2,			tile,		"rstack"	},
	{MOD1|MOD2,			3,			cycle_tile,	NULL		},
	{MOD1|MOD2,			4,			desktop,	"grow"		},
	{MOD1|MOD2,			5,			desktop,	"shrink"	},
};


// vim: ts=4
