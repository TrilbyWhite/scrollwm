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

enum {Background, Default, Hidden, Normal, Sticky, Urgent, Title, TagList, LASTColor };

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
	int tags;
	Client *next;
	Window win;
};

static void buttonpress(XEvent *);
static void buttonrelease(XEvent *);
static void expose(XEvent *);
static void keypress(XEvent *);
static void maprequest(XEvent *);
static void motionnotify(XEvent *);
static void unmapnotify(XEvent *);

static void draw(Client *);
static void focusclient(Client *);
static void quit(const char *);
static void scrollwindows(Client *,int,int);
static void spawn(const char *);
static void tag(const char *);
static void tagconfig(const char *);
static void toggletag(const char *);
static Client *wintoclient(Window);
static void zoom(Client *,float,int,int);

#include "config.h"

static Display * dpy;
static Window root, bar;
static Pixmap buf;
static int scr, sw, sh;
static GC gc;
static Colormap cmap;
static XFontStruct *fontstruct;
static int fontheight, barheight;
static XWindowAttributes attr;
static XButtonEvent start;
static Client *clients=NULL;
static Client *focused;
static Bool running = True, showbar = True;
static int tags_stik = 0, tags_hide = 0, tags_urg = 0;
static int curtag = 0;
static void (*handler[LASTEvent]) (XEvent *) = {
	[ButtonPress]	= buttonpress,
	[ButtonRelease]	= buttonrelease,
	[Expose]		= expose,
	[KeyPress]		= keypress,
	[MapRequest]	= maprequest,
	[MotionNotify]	= motionnotify,
	[UnmapNotify]	= unmapnotify,
};

void buttonpress(XEvent *e) {
	if (e->xbutton.button == 4)
		zoom(clients,1.1,e->xbutton.x_root,e->xbutton.y_root);
	else if (e->xbutton.button == 5)
		zoom(clients,.92,e->xbutton.x_root,e->xbutton.y_root);
	else if (e->xbutton.button > 3) return;
	Client *c;
	Window w;
	if( (c=wintoclient(e->xbutton.subwindow))) w = c->win;
	else w = root;
	if (w==root && e->xbutton.state == Mod4Mask) return;
	if (c && e->xbutton.button == 2) {
		if (showbar) XMoveResizeWindow(dpy,c->win,(c->x=-2),(c->y=barheight-2),(c->w=sw),(c->h=sh-barheight));
		else XMoveResizeWindow(dpy,c->win,(c->x=-2),(c->y=-2),(c->w=sw),(c->h=sh));
	}
	if (c) focusclient(c);
	XGrabPointer(dpy,w,True,PointerMotionMask | ButtonReleaseMask,
		GrabModeAsync,GrabModeAsync, None, None, CurrentTime);
	XGetWindowAttributes(dpy,w, &attr);
	start = e->xbutton;
	draw(clients);
}

void buttonrelease(XEvent *e) {
	XUngrabPointer(dpy, CurrentTime);
}

void draw(Client *stack) {
	/* windows */
	XColor color;
	int tags_occ = 0;
	XSetWindowAttributes wa;
	while (stack) {
		tags_occ |= stack->tags;
		if (stack->tags & tags_hide) {
			XMoveWindow(dpy,stack->win,sw+2,0);
			stack = stack->next;
			continue;
		}
		XMoveResizeWindow(dpy,stack->win,stack->x,stack->y,
			MAX(stack->w,WIN_MIN),MAX(stack->h,WIN_MIN));
		XAllocNamedColor(dpy,cmap,
			colors[(stack->tags & tags_stik ? Sticky : Normal)],&color,&color);
		wa.border_pixel = color.pixel;
		XChangeWindowAttributes(dpy,stack->win,CWBorderPixel,&wa);
		stack = stack->next;
	}
	/* status bar */
	XAllocNamedColor(dpy,cmap,colors[Background],&color,&color);
	XSetForeground(dpy,gc,color.pixel);
	XFillRectangle(dpy,buf,gc,0,0,sw,barheight);
	int i, x=10,w;
	int col;
	for (i = 0; tag_name[i]; i++) {
		if (!(tags_occ & (1<<i)) && curtag != i) continue;
		col = (tags_urg & (1<<i) ? Urgent :
				(tags_hide & (1<<i) ? Hidden :
				(tags_stik & (1<<i) ? Sticky : 
				(tags_occ  & (1<<i) ? Normal : Default ))));
		XAllocNamedColor(dpy,cmap,colors[col],&color,&color);
		XSetForeground(dpy,gc,color.pixel);
		XDrawString(dpy,buf,gc,x,fontheight,tag_name[i],strlen(tag_name[i]));
		w = XTextWidth(fontstruct,tag_name[i],strlen(tag_name[i]));
		if (curtag == i) XFillRectangle(dpy,buf,gc,x-2,fontheight+1,w+4,barheight-fontheight);
		x+=w+10;
	}
	if (focused) {
		x = MAX(x+20,sw/4);
		XAllocNamedColor(dpy,cmap,colors[Title],&color,&color);
		XSetForeground(dpy,gc,color.pixel);
		XDrawString(dpy,buf,gc,x,fontheight,focused->title,strlen(focused->title));
		x += XTextWidth(fontstruct,focused->title,strlen(focused->title)) + 10;
		XAllocNamedColor(dpy,cmap,colors[TagList],&color,&color);
		XSetForeground(dpy,gc,color.pixel);
		XDrawString(dpy,buf,gc,x,fontheight,"[",1);
		x += XTextWidth(fontstruct,"[",1);
		for (i = 0; tag_name[i]; i++) if (focused->tags & (1<<i)) {
			XDrawString(dpy,buf,gc,x,fontheight,tag_name[i],strlen(tag_name[i]));
			x += XTextWidth(fontstruct,tag_name[i],strlen(tag_name[i]));
			if (tagname[i+1]) {
				XDrawString(dpy,buf,gc,x,fontheight,", ",2);
				x += XTextWidth(fontstruct,", ",2);
			}
		}
		XDrawString(dpy,buf,gc,x,fontheight,"]",1);
	}
	XCopyArea(dpy,buf,bar,gc,0,0,sw,barheight,0,0);
	XRaiseWindow(dpy,bar);
	XFlush(dpy);
}

void expose(XEvent *e) {
	draw(clients);
}

void focusclient(Client *c) {
	focused = c;
	if (!c) return;
	XSetInputFocus(dpy,c->win,RevertToPointerRoot,CurrentTime);
	XRaiseWindow(dpy,c->win);
	XRaiseWindow(dpy,bar);
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
		if (c->y < barheight+2 && showbar) c->y = barheight+2;
		c->tags = (1<<curtag);
		if (XFetchName(dpy,c->win,&c->title)) c->tlen = strlen(c->title);
		XSelectInput(dpy,c->win,PropertyChangeMask);
		c->next = clients;
		clients = c;
		XSetWindowBorderWidth(dpy,c->win,2);
		XMapWindow(dpy,c->win);
		focusclient(c);
	}
	draw(clients);
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
	else if (start.button == 1 && start.state == (Mod1Mask | Mod4Mask)) {
		scrollwindows(clients,xdiff,ydiff);
		start.x_root+=xdiff; start.y_root+=ydiff;
	}
}

void quit(const char *arg) {
	running = False;
}

void scrollwindows(Client *stack, int x, int y) {
	while (stack) {
		if ( !(stack->tags & tags_hide) && !(stack->tags & tags_stik) )
			XMoveWindow(dpy,stack->win,(stack->x+=x),(stack->y+=y));
		stack = stack->next;
	}
}

void spawn(const char *arg) {
	system(arg);
}

void tag(const char *arg) {
	curtag = arg[0] - 49;
	draw(clients);
}

void tagconfig(const char *arg) {
	if (arg[0] == 'h') tags_hide |= (1<<curtag);
	else if (arg[0] == 's') tags_stik |= (1<<curtag);
	else if (arg[0] == 'n') { tags_stik &= ~(1<<curtag); tags_hide &= ~(1<<curtag); }
	else if (arg[0] == 'b') showbar = ~showbar;
	draw(clients);
}

void toggletag(const char *arg) {
	if (!focused) return;
	int t = arg[0] - 49;
	focused->tags = focused->tags ^ (1<<t);
	draw(clients);
}

void unmapnotify(XEvent *e) {
	Client *c,*t;
	XUnmapEvent *ev = &e->xunmap;
	if (!(c=wintoclient(ev->window))) return;
	if (!ev->send_event) {
		if (c == focused) focusclient(c->next);
		if (c == clients) clients = c->next;
		else {
			for (t = clients; t && t->next != c; t = t->next);
			t->next = c->next;
		}
		XFree(c->title);
		free(c);
		c = NULL;
	}
	if (!focused) focusclient(clients);
	draw(clients);
}

Client *wintoclient(Window w) {
	Client *c;
	for (c = clients; c && c->win != w; c = c->next);
	if (c) return c;
	else return NULL;
}

void zoom(Client *stack, float factor, int x, int y) {
	while (stack) {
		if (!(stack->tags & tags_stik)) {
			stack->w *= factor; stack->h *= factor;
			if (stack->w < ZOOM_MIN) stack->w = ZOOM_MIN;
			if (stack->h < ZOOM_MIN) stack->h = ZOOM_MIN;
			stack->x = (stack->x-x) * factor + x;
			stack->y = (stack->y-y) * factor + y;
		}
		stack = stack->next;
	}
	draw(clients);
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
	barheight = fontstruct->ascent+fontstruct->descent+2;
	gc = XCreateGC(dpy,root,GCFont,&val);

	bar = XCreateSimpleWindow(dpy,root,0,0,sw,barheight,0,0,0);
	buf = XCreatePixmap(dpy,root,sw,barheight,DefaultDepth(dpy,scr));
	XSetWindowAttributes wa;
	wa.override_redirect = True;
	wa.event_mask = ExposureMask;
	XChangeWindowAttributes(dpy,bar,CWOverrideRedirect|CWEventMask,&wa);
	XMapWindow(dpy,bar);
	wa.event_mask = FocusChangeMask | SubstructureNotifyMask | ButtonReleaseMask |
			PropertyChangeMask | SubstructureRedirectMask | StructureNotifyMask;
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


