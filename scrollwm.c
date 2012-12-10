/*******************************************************************\
* SCROLLWM - a floating WM with a single large scrollable workspace
*
* Author: Jesse McClure, copyright 2012
* License: GPLv3
*
* Based on code from TinyWM and TTWM
* TinyWM is written by Nick Welch <mack@incise.org>, 2005.
* TTWM is written by Jesse McClure, 2011-2012
\*******************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/XKBlib.h>
#include <X11/cursorfont.h>

#define MAX(a, b) ((a) > (b) ? (a) : (b))

enum {Background, Normal, Hidden, Inactive, Active, LASTColor };

typedef struct {
	unsigned int mod;
	KeySym keysym;
	void (*func)(const char *);
	const char *arg;
} Key;

typedef struct {
	unsigned int mask, button;
	void (*func)();
} Button;

typedef struct Client Client;
struct Client {
	char *title;
	int tlen;
	int x, y;
	float w, h;
	Client *next;
	Window win;
};

static void buttonpress(XEvent *);
static void buttonrelease(XEvent *);
static void keypress(XEvent *);
static void maprequest(XEvent *);
static void motionnotify(XEvent *);
static void unmapnotify(XEvent *);

static void scrollwindows(Client *,int,int);
static void spawn(const char *);
static Client *wintoclient(Window);
static void zoom(Client *,float,int,int);

#include "config.h"

static Display * dpy;
static Window root;
static int scr, sw, sh;
static Colormap cmap;
static XFontStruct *fontstruct;
static int fontheight;
static XWindowAttributes attr;
static XButtonEvent start;
static Client *clients=NULL;
static Bool running = True;
static void (*handler[LASTEvent]) (XEvent *) = {
	[ButtonPress]	= buttonpress,
	[ButtonRelease]	= buttonrelease,
	[KeyPress]		= keypress,
	[MapRequest]	= maprequest,
	[MotionNotify]	= motionnotify,
	[UnmapNotify]	= unmapnotify,
};

void buttonpress(XEvent *e) {
	if (e->xbutton.button == 4)
		zoom(clients,1.1,e->xbutton.x_root,e->xbutton.y_root);
	else if (e->xbutton.button == 5)
		zoom(clients,.89,e->xbutton.x_root,e->xbutton.y_root);
	else if (e->xbutton.button > 3) return;
	Client *c;
	Window w;
	if( (c=wintoclient(e->xbutton.subwindow))) w = c->win;
	else w = root;
	if (w==root && e->xbutton.state == Mod4Mask) return;
	if (c && e->xbutton.button == 2)
		XMoveResizeWindow(dpy,c->win,(c->x=0),(c->y=0),(c->w=sw),(c->h=sh));
	if (c) {
		XSetInputFocus(dpy,w,RevertToPointerRoot,CurrentTime);
		XRaiseWindow(dpy,w);
	}
	XGrabPointer(dpy,w,True,PointerMotionMask | ButtonReleaseMask,
		GrabModeAsync,GrabModeAsync, None, None, CurrentTime);
	XGetWindowAttributes(dpy,w, &attr);
	start = e->xbutton;
}

void buttonrelease(XEvent *e) {
	XUngrabPointer(dpy, CurrentTime);
}

void keypress(XEvent *e) {
	unsigned int i;
	XKeyEvent *ev = &e->xkey;
	KeySym keysym = XkbKeycodeToKeysym(dpy,(KeyCode)ev->keycode,0,0);
	for (i = 0; i < sizeof(keys)/sizeof(keys[0]); i++)
		if ( (keysym==keys[i].keysym) && keys[i].func &&
				keys[i].mod == ((ev->state&~Mod2Mask)&~LockMask) )
			keys[i].func(keys[i].arg);
}

void maprequest(XEvent *e) {
	Client *c;
	static XWindowAttributes wa;
	XMapRequestEvent *ev = &e->xmaprequest;
	if (!XGetWindowAttributes(dpy, ev->window, &wa)) return;
	if (wa.override_redirect) return;
	if (!wintoclient(ev->window)) {
		if (!(c=calloc(1,sizeof(Client)))) exit(1);
		c->win = ev->window;
		XGetWindowAttributes(dpy,c->win, &attr);
		c->x = attr.x; c->y = attr.y;
		c->w = attr.width; c->h = attr.height;
		if (XFetchName(dpy,c->win,&c->title)) c->tlen = strlen(c->title);
		XSelectInput(dpy,c->win,PropertyChangeMask);
		c->next = clients;
		clients = c;
		XMapWindow(dpy,c->win);
	}
}

void motionnotify(XEvent *e) {
	int xdiff, ydiff;
	while(XCheckTypedEvent(dpy,MotionNotify,e));
	Client *c = wintoclient(e->xbutton.window);
	xdiff = e->xbutton.x_root - start.x_root;
	ydiff = e->xbutton.y_root - start.y_root;
	if (start.button == 1 && start.state == Mod4Mask)
		XMoveWindow(dpy,c->win,(c->x=attr.x+xdiff),(c->y=attr.y+ydiff));
	else if (start.button == 3 && start.state == Mod4Mask)
		XResizeWindow(dpy,c->win,(c->w=attr.width+xdiff),(c->h=attr.height+ydiff));
	else if (start.button == 1 && start.state == Mod1Mask | Mod4Mask) {
		scrollwindows(clients,xdiff,ydiff);
		start.x_root+=xdiff; start.y_root+=ydiff;
	}
}

void unmapnotify(XEvent *e) {
	Client *c,*t;
	XUnmapEvent *ev = &e->xunmap;
	if (!(c=wintoclient(ev->window))) return;
	if (!ev->send_event) {
		if (c == clients) clients = c->next;
		else {
			for (t = clients; t && t->next != c; t = t->next);
			t->next = c->next;
		}
		//XFree(c->title);
		free(c);
	}
}

void scrollwindows(Client *stack, int x, int y) {
	while (stack) {
		XGetWindowAttributes(dpy,stack->win, &attr);
		XMoveWindow(dpy,stack->win,(stack->x=attr.x+x),(stack->y=attr.y+y));
		stack = stack->next;
	}
}

void spawn(const char *arg) {
	system(arg);
}

Client *wintoclient(Window w) {
	Client *c;
	for (c = clients; c && c->win != w; c = c->next);
	if (c) return c;
	else return NULL;
}

void zoom(Client *stack, float factor, int x, int y) {
	while (stack) {
		stack->w *= factor; stack->h *= factor;
		if (stack->w < ZOOM_MIN) stack->w = ZOOM_MIN;
		if (stack->h < ZOOM_MIN) stack->h = ZOOM_MIN;
		stack->x = (stack->x-x) * factor + x;
		stack->y = (stack->y-y) * factor + y;
		XMoveResizeWindow(dpy,stack->win,stack->x,stack->y,
			MAX(stack->w,WIN_MIN),MAX(stack->h,WIN_MIN));
		stack = stack->next;
	}
}
	

int main() {
    if(!(dpy = XOpenDisplay(0x0))) return 1;
	scr = DefaultScreen(dpy);
	sw = DisplayWidth(dpy,scr);
	sh = DisplayHeight(dpy,scr);
    root = DefaultRootWindow(dpy);
	XDefineCursor(dpy,root,XCreateFontCursor(dpy,SCROLLWM_CURSOR));

	cmap = DefaultColormap(dpy,scr);
	XGCValues val;
	val.font = XLoadFont(dpy,font);
	fontstruct = XQueryFont(dpy,val.font);
	fontheight = fontstruct->ascent+1;
	// TODO create status bar here

	XSetWindowAttributes wa;
	wa.event_mask = ExposureMask | FocusChangeMask | SubstructureNotifyMask |
					ButtonReleaseMask | PropertyChangeMask | SubstructureRedirectMask |
					StructureNotifyMask;
	XChangeWindowAttributes(dpy,root,CWEventMask,&wa);
	XSelectInput(dpy,root,wa.event_mask);

	unsigned int mods[] = {0, LockMask, Mod2Mask, LockMask|Mod2Mask};
	KeyCode code;
	XUngrabKey(dpy,AnyKey,AnyModifier,root);
	int i,j;
	for (i = 0; i < sizeof(keys)/sizeof(keys[0]); i++)
		if ( (code=XKeysymToKeycode(dpy,keys[i].keysym)) ) for (j = 0; j < 4; j++)
			XGrabKey(dpy,code,keys[i].mod|mods[j],root,True,GrabModeAsync,GrabModeAsync);
    XGrabButton(dpy,AnyButton,Mod4Mask,root,True,ButtonPressMask,GrabModeAsync,
		GrabModeAsync,None,None);
    XGrabButton(dpy,AnyButton,Mod4Mask|Mod1Mask,root,True,ButtonPressMask,GrabModeAsync,
		GrabModeAsync,None,None);
    XEvent ev;
	while (running && !XNextEvent(dpy,&ev))
		if (handler[ev.type])
			handler[ev.type](&ev);
	XFreeFontInfo(NULL,fontstruct,1);
	XUnloadFont(dpy,val.font);
	return 0;
}


