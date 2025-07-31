#define FLUORITE_VERSION "Fluorite [EVO 2] (Beta 2)"

#include "config/keybinds.h"
#include "config/design.h"
#include <X11/X.h>
#include <X11/Xresource.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <X11/Xcursor/Xcursor.h>
#include <X11/extensions/Xcomposite.h>
#include <stdio.h>
#include <sys/types.h>
#include <xdo.h>
#include <X11/Xft/Xft.h>
#include <X11/extensions/Xrandr.h>
#include <err.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>
#include <pwd.h>
#include <sys/inotify.h>
#include <errno.h>

/* DEF: constant */
#define MAX_WS 10
#define MODMASK(mask)	(mask & ~(numlockmask | LockMask) & (ShiftMask|ControlMask|Mod1Mask|Mod2Mask|Mod3Mask|Mod4Mask|Mod5Mask))
#define LENGTH(X)		(sizeof X / sizeof X[0])
#define EVENT_SIZE		(sizeof(struct inotify_event))
#define BUF_LEN			(1024 * (EVENT_SIZE + 16))

/* DEF: struct */
typedef struct
{
	int  bf;
	int  bu;
	int  bw;
	int  gp;
	int  so;
	int  mo;
	int  tb;
	int  bb;
	char *home;
} Configuration;

typedef struct
{
	int				spx;
	int				spy;
	int				swx;
	int				swy;
	unsigned int	sww;
	unsigned int	swh;
} Mouse;

typedef struct
{
	int mx;
	int my;
	int mw;
	int mh;
	int ws;
	int primary;
} Monitors;

typedef struct Windows
{
	Window w;
	int wx;
	int wy;
	int ww;
	int wh;
	int pid;
	int fc;
	int fs;
	struct Windows *next;
	struct Windows *prev;
} Windows;

typedef struct
{
	Windows *t_wins;
	Windows *f_wins;
	Windows *tmp;
	int		layout;
	int		fs;
	Window	cr_fs;
	int		fl_hdn;
	int		mo;
	int   ct_win;
} Workspaces;

typedef struct
{
	Display			*dpy;
	Window			root;
	int				scr;
	int				run;
	Workspaces		ws[MAX_WS];
	Monitors		*mon;
	Configuration	conf;
	int				cr_ws;
	int				cr_mon;
	int				ct_mon;
	Mouse			mouse;
	xdo_t			*xdo;
	int				xrandr_ev;
} Fluorite;

/* DEF: functions */
static void		FInit();
static int  	FErrorHandler(Display *dis, XErrorEvent *ev);
static void 	FLoadXresources();
static void 	FLoadTheme();
static void 	FInitMonitors();
static void 	FInitWorkspaces();
static void 	FApplyProps();
static void 	FGrabKeys(Window w);
static void 	*FInotifyXresources();
static void 	FReloadXresources();
static void 	FRun();
static void 	FGetMonitorFromMouse();
static int		FCheckWindowToplevel(Window nw);
static void 	FMapRequest(XEvent ev);
static void 	FManageFloatingWindow(Windows *nw);
static void 	FRedrawWindows();
static void 	FApplyActiveWindow(Window w);
static void 	FApplyBorders();
static int  	FCheckWindowIsFloating(Window w);
static int  	FCheckWindowIsFixed(Window w);
static void 	FChangeMonitor(int mon);
static void 	FConfigureRequest(XEvent ev);
static void 	FKeyPress(XEvent ev);
static int  	FFindWorkspaceFromWindow(Window w);
static void		FResetFocus(Windows *w);
static void 	FUnmapNotify(XEvent ev);
static Windows	*FAddWindow(Windows *cw, Windows *nw);
static Windows	*FDelWindow(Windows *cw, Windows *nw);
static void		FFocusWindowUnderCursor();
static Window	FGetToplevel(Window w);
static void		FButtonPress(XEvent ev);
static void 	FClientMessage(XEvent ev);
static void 	FMotionNotify(XEvent ev);
static void 	FWarpCursor(Window w);
static void 	FSetWindowOpacity(Window w, double opacity);
static void		FUpdateClientList();
	   void 	FShowWorkspace(int ws);
	   void 	FSendWindowToWorkspace(int ws);
       void 	FExecute(char *argument);
       void 	FQuit();
       void 	FCloseWindow();
       void 	FRotateSlavesWindows(int mode);
       void 	FRotateWindows(int mode);

/* DEF: globals */
static Fluorite fluorite;
static int no_unmap = False;
static int no_warp = False;
static unsigned int numlockmask = 0;

int main(void)
{
	pthread_t t_inotify;

	FInit();
	if (XRESOURCES_AUTO_RELOAD)
		pthread_create(&t_inotify, NULL, FInotifyXresources, NULL);
	FRun();
	FQuit();
}

static void FInit()
{
	fluorite.dpy = XOpenDisplay(NULL);
	if (!fluorite.dpy)
		errx(1, "Can't open display.");

	fluorite.scr = DefaultScreen(fluorite.dpy);
	fluorite.root = RootWindow(fluorite.dpy, fluorite.scr);
	fluorite.xdo = xdo_new(NULL);
	fluorite.cr_ws = 0;
	fluorite.mon = NULL;

	XrmInitialize();
	XSetErrorHandler(FErrorHandler);
	FApplyProps();
	FInitMonitors();
	FInitWorkspaces();
	FLoadXresources();
	FGrabKeys(fluorite.root);
}

static int FErrorHandler(Display *dis, XErrorEvent *ev)
{
	int fd = open("/tmp/fluorite.log", O_WRONLY | O_CREAT | O_APPEND, 0666);
	char error[1024];

	XGetErrorText(dis, ev->error_code, error, sizeof(error));
	dprintf(fd, "(%d) %d: %s\n", ev->error_code, ev->request_code, error);
	return ev->error_code;
}

static void FLoadXresources()
{
	char *xrm;
	char *type;
	XrmDatabase xdb;
	XrmValue xval;
	Display *dummy_display;

	FLoadTheme();

	dummy_display = XOpenDisplay(NULL);
	xrm = XResourceManagerString(dummy_display);

	if (!xrm)
		return ;

	xdb = XrmGetStringDatabase(xrm);

	if (XrmGetResource(xdb, "fluorite.border_width", "*", &type, &xval))
		if (xval.addr)
			fluorite.conf.bw = strtoul(xval.addr, NULL, 10);
	if (XrmGetResource(xdb, "fluorite.border_focused", "*", &type, &xval))
		if (xval.addr)
			fluorite.conf.bf = strtoul(xval.addr, NULL, 0);
	if (XrmGetResource(xdb, "fluorite.border_unfocused", "*", &type, &xval))
		if (xval.addr)
			fluorite.conf.bu = strtoul(xval.addr, NULL, 0);
	if (XrmGetResource(xdb, "fluorite.gaps", "*", &type, &xval))
		if (xval.addr)
			fluorite.conf.gp = strtoul(xval.addr, NULL, 10);
	if (XrmGetResource(xdb, "fluorite.stack_offset", "*", &type, &xval))
		if (xval.addr)
			fluorite.conf.so = strtoul(xval.addr, NULL, 10);
	if (XrmGetResource(xdb, "fluorite.topbar_gaps", "*", &type, &xval))
		if (xval.addr)
			fluorite.conf.tb = strtoul(xval.addr, NULL, 10);
	if (XrmGetResource(xdb, "fluorite.bottombar_gaps", "*", &type, &xval))
		if (xval.addr)
			fluorite.conf.bb = strtoul(xval.addr, NULL, 10);
	if (XrmGetResource(xdb, "fluorite.default_master_offset", "*", &type, &xval))
		if (xval.addr)
			fluorite.conf.mo = strtoul(xval.addr, NULL, 10);
	
	fluorite.conf.bf |= 0xff << 24;
	fluorite.conf.bu |= 0xff << 24;
	XrmDestroyDatabase(xdb);
	free(xrm);
}

static void FLoadTheme()
{
	fluorite.conf.bw = BORDER_WIDTH;
	fluorite.conf.bf = BORDER_FOCUSED;
	fluorite.conf.bu = BORDER_UNFOCUSED;
	fluorite.conf.gp = GAPS;
	fluorite.conf.so = STACK_OFFSET;
	fluorite.conf.tb = TOPBAR_GAPS;
	fluorite.conf.bb = BOTTOMBAR_GAPS;
	fluorite.conf.mo = DEFAULT_MASTER_OFFSET;
}

static void FInitMonitors()
{
	int hot_plug = False;
	XRRMonitorInfo *infos;

	if (fluorite.mon != NULL)
	{
		free(fluorite.mon);
		no_unmap = True;
		XGrabServer(fluorite.dpy);
		for (int i = 0; i < MAX_WS; i++)
		{
			for (Windows *w = fluorite.ws[i].t_wins; w != NULL; w = w->next)
				XUnmapWindow(fluorite.dpy, w->w);
			for (Windows *w = fluorite.ws[i].f_wins; w != NULL; w = w->next)
				XUnmapWindow(fluorite.dpy, w->w);
		}
		XSync(fluorite.dpy, True);
		XUngrabServer(fluorite.dpy);
		no_unmap = False;
		hot_plug = True;
	}

	infos = XRRGetMonitors(fluorite.dpy, fluorite.root, 0, &fluorite.ct_mon);
	fluorite.mon = (Monitors *) calloc(fluorite.ct_mon, sizeof(Monitors));
	for (int i = 0; i < fluorite.ct_mon; i++)
	{
		fluorite.mon[i].mx = infos[i].x;
		fluorite.mon[i].my = infos[i].y;
		fluorite.mon[i].mw = infos[i].width;
		fluorite.mon[i].mh = infos[i].height;
		fluorite.mon[i].ws = default_monitor_workspace[i] - 1;
		fluorite.mon[i].primary = False;
		if (default_monitor_workspace[i] == 0)
			fluorite.mon[i].ws = 9;
		if (infos[i].primary)
		{
			fluorite.mon[i].primary = True;
			fluorite.cr_mon = i;
			xdo_move_mouse(fluorite.xdo, fluorite.mon[i].mx + (fluorite.mon[i].mw / 2), fluorite.mon[i].my + (fluorite.mon[i].mh / 2), fluorite.scr);
		}
	}

	if (hot_plug)
	{
		int primary;
		for (int i = 0; i < fluorite.ct_mon; i++)
		{
			if (fluorite.mon[i].primary)
				primary = i;
			fluorite.cr_ws = fluorite.mon[i].ws;
			fluorite.cr_mon = i;
			for (Windows *w = fluorite.ws[fluorite.cr_ws].t_wins; w != NULL; w = w->next)
				XMapWindow(fluorite.dpy, w->w);
			for (Windows *w = fluorite.ws[fluorite.cr_ws].f_wins; w != NULL; w = w->next)
				XMapWindow(fluorite.dpy, w->w);
			FRedrawWindows();
			FApplyBorders();
		}
		fluorite.cr_mon = primary;
		fluorite.cr_ws = fluorite.mon[fluorite.cr_mon].ws;
		FRedrawWindows();
		FApplyBorders();
		XChangeProperty(fluorite.dpy, fluorite.root, XInternAtom(fluorite.dpy, "_NET_CURRENT_DESKTOP", False), XA_CARDINAL, 32, PropModeReplace, (unsigned char *)&fluorite.cr_ws, 1);
	}
}

static void FInitWorkspaces()
{
	for (int i = 0; i < MAX_WS; i++)
	{
		fluorite.ws[i].fs = False;
		fluorite.ws[i].layout = STARTING_LAYOUT;
		fluorite.ws[i].cr_fs = -1;
		fluorite.ws[i].fl_hdn = False;
		fluorite.ws[i].mo = fluorite.conf.mo;
		fluorite.ws[i].ct_win = 0;
		fluorite.ws[i].t_wins = NULL;
		fluorite.ws[i].f_wins = NULL;
	}
}

static void FApplyProps()
{
	XTextProperty text;
	XSetWindowAttributes attributes;
	int num_work_atom = MAX_WS;
	int rr_error_base;

	if (!XRRQueryExtension(fluorite.dpy, &fluorite.xrandr_ev, &rr_error_base))
		exit(1);
	XChangeProperty(fluorite.dpy, fluorite.root, XInternAtom(fluorite.dpy, "_NET_WM_NAME", False), XInternAtom(fluorite.dpy, "UTF8_STRING", False), 8, PropModeReplace, (const unsigned char *) FLUORITE_VERSION, strlen(FLUORITE_VERSION));
	XChangeProperty(fluorite.dpy, fluorite.root, XInternAtom(fluorite.dpy, "_NET_SUPPORTING_WM_CHECK", False), XA_WINDOW, 32, PropModeReplace, (const unsigned char *) &fluorite.root, 1);
	XChangeProperty(fluorite.dpy, fluorite.root, XInternAtom(fluorite.dpy, "_NET_NUMBER_OF_DESKTOPS", False), XA_CARDINAL, 32, PropModeReplace, (const unsigned char *)&num_work_atom, 1);
	Xutf8TextListToTextProperty(fluorite.dpy, (char **)workspace_names, MAX_WS, XUTF8StringStyle, &text);
	XSetTextProperty(fluorite.dpy, fluorite.root, &text, XInternAtom(fluorite.dpy, "_NET_DESKTOP_NAMES", False));
	XChangeProperty(fluorite.dpy, fluorite.root, XInternAtom(fluorite.dpy, "_NET_CURRENT_DESKTOP", False), XA_CARDINAL, 32, PropModeReplace, (unsigned char *)&fluorite.cr_ws, 1);
	Atom supported[8] = {
		XInternAtom(fluorite.dpy, "_NET_WM_NAME", False),			XInternAtom(fluorite.dpy, "_NET_SUPPORTING_WM_CHECK", False),
		XInternAtom(fluorite.dpy, "_NET_ACTIVE_WINDOW", False),		XInternAtom(fluorite.dpy, "_NET_DESKTOP_NAMES", False),
		XInternAtom(fluorite.dpy, "_NET_CURRENT_DESKTOP", False),	XInternAtom(fluorite.dpy, "_NET_CLIENT_LIST", False),
		XInternAtom(fluorite.dpy, "_NET_WM_DESKTOP", False),		XInternAtom(fluorite.dpy, "_NET_NUMBER_OF_DESKTOPS", False)
	};
	XChangeProperty(fluorite.dpy, fluorite.root, XInternAtom(fluorite.dpy, "_NET_SUPPORTED", False), XA_ATOM, 32, PropModeReplace, (unsigned char *)supported, 7);
	attributes.event_mask = SubstructureNotifyMask | SubstructureRedirectMask | StructureNotifyMask | ButtonPressMask | KeyPressMask | PointerMotionMask | PropertyChangeMask;
	XSelectInput(fluorite.dpy, fluorite.root, attributes.event_mask);
	XDefineCursor(fluorite.dpy, fluorite.root, XcursorLibraryLoadCursor(fluorite.dpy, "arrow"));
	XRRSelectInput(fluorite.dpy, fluorite.root, RROutputChangeNotifyMask | RRCrtcChangeNotifyMask | RRScreenChangeNotifyMask);
	XSync(fluorite.dpy, True);
}

static void FGrabKeys(Window w)
{
	unsigned int i;
	int j;
	XModifierKeymap *modmap;

	numlockmask = 0;
	modmap = XGetModifierMapping(fluorite.dpy);
	for (i = 0; i < 8; i++)
		for (j = 0; j < modmap->max_keypermod; j++)
			if (modmap->modifiermap[i * modmap->max_keypermod + j] == XKeysymToKeycode(fluorite.dpy, XK_Num_Lock))
				numlockmask = (1 << i);
	XFreeModifiermap(modmap);
	{
		unsigned int i, j, k;
		unsigned int modifiers[] = { 0, LockMask, numlockmask, numlockmask|LockMask };
		int start, end, skip;
		KeySym *syms;

		XUngrabKey(fluorite.dpy, AnyKey, AnyModifier, w);
		XDisplayKeycodes(fluorite.dpy, &start, &end);
		syms = XGetKeyboardMapping(fluorite.dpy, start, end - start + 1, &skip);
		if (!syms)
			return;
		for (k = start; k <= (unsigned int)end; k++)
			for (i = 0; i < LENGTH(bind); i++)
				if (bind[i].key == syms[(k - start) * skip])
					for (j = 0; j < LENGTH(modifiers); j++)
						XGrabKey(fluorite.dpy, k,
							 bind[i].mod | modifiers[j],
							 w, True,
							 GrabModeAsync, GrabModeAsync);
		XFree(syms);
	}
}

static void *FInotifyXresources()
{
	const char *filename = ".Xresources";
	char *home = getenv("HOME");
	char buf[BUF_LEN];
	int fd;

	if ((fd = inotify_init1(IN_NONBLOCK)) < 0)
		return NULL;
	inotify_add_watch(fd, home, IN_CREATE | IN_MODIFY | IN_DELETE | IN_MOVED_FROM | IN_MOVED_TO);

	while (1145)
	{
		ssize_t len = read(fd, buf, BUF_LEN);
		if (len < 0)
		{
			if (errno == EAGAIN || errno == EWOULDBLOCK)
			{
				usleep(100000);
				continue;
			}
			break;
		}

		ssize_t i = 0;
		while (i < len)
		{
			struct inotify_event *event = (struct inotify_event *) &buf[i];
			if (event->len > 0 && strcmp(event->name, filename) == 0)
					FReloadXresources();
			i += EVENT_SIZE + event->len;
		}
	}

	close(fd);
	return NULL;
}

static void FReloadXresources()
{
	char prog[255] = "xrdb ~/.Xresources";
	if (system(prog) == -1)
		printf("Error: can't start %s\n", prog);
	FLoadXresources();
	FRedrawWindows();
	FApplyBorders();
}

static void FRun()
{
	XEvent ev;

	fluorite.run = 1;
	while (fluorite.run)
	{
		XNextEvent(fluorite.dpy, &ev);
		FGetMonitorFromMouse();
		if (ev.type == fluorite.xrandr_ev + RRScreenChangeNotify)
		{
			XRRUpdateConfiguration(&ev);
			FInitMonitors();
		}
		switch (ev.type)
		{
			case ConfigureRequest:
				FConfigureRequest(ev);
				break;
			case MapRequest:
				FMapRequest(ev);
				break;
			case UnmapNotify:
				FUnmapNotify(ev);
				break;
			case ButtonPress:
				FButtonPress(ev);
				break;
			case KeyPress:
				FKeyPress(ev);
				break;
			case MotionNotify:
				FFocusWindowUnderCursor();
				FMotionNotify(ev);
				break;
			case EnterNotify:
				FFocusWindowUnderCursor();
				break;
			case ClientMessage:
				FClientMessage(ev);
				break;
		}
	}
}

static void FGetMonitorFromMouse()
{
	
	int pointer_x, pointer_y, s_num;
	int pos_x, pos_y, max_x, max_y;

	xdo_get_mouse_location(fluorite.xdo, &pointer_x, &pointer_y, &s_num);
	for (int i = 0; i < fluorite.ct_mon; i++)
	{
		max_x = fluorite.mon[i].mx + fluorite.mon[i].mw;
		max_y = fluorite.mon[i].my + fluorite.mon[i].mh;
		pos_x = fluorite.mon[i].mx;
		pos_y = fluorite.mon[i].my;
		if ((pointer_x >= pos_x && pointer_x <= max_x) && (pointer_y >= pos_y && pointer_y <= max_y) && fluorite.cr_mon != i)
			FChangeMonitor(i);
	}
}

int FCountWindows(Windows *w)
{
	int result = 0;
	while (w)
	{
		w = w->next;
		result++;
	}
	return result;
}

static int FCheckWindowToplevel(Window nw)
{
	int pid = xdo_get_pid_window(fluorite.xdo, nw);
	Window toplevel = FGetToplevel(nw);
	Windows *w;
	int is_floating = False;
	int i;

	for (i = 0; i < MAX_WS; i++)
	{
		for (w = fluorite.ws[i].t_wins; w != NULL; w = w->next)
		{
			if (pid == w->pid && toplevel == w->w)
			{
				FResetFocus(fluorite.ws[i].t_wins);
				FResetFocus(fluorite.ws[i].f_wins);
				w->fc = 1;
				if (JUMP_TO_URGENT)
					goto show_ws;
				return True;
			}
		}
		for (w = fluorite.ws[i].f_wins; w != NULL; w = w->next)
		{
			if (pid == w->pid && toplevel == w->w)
			{
				is_floating = True;
				FResetFocus(fluorite.ws[i].t_wins);
				FResetFocus(fluorite.ws[i].f_wins);
				w->fc = 1;
				FWarpCursor(w->w);
				if (JUMP_TO_URGENT)
					goto show_ws;
				return True;
			}
		}
	}
	return False;

show_ws:
	if (i != fluorite.cr_ws)
		FShowWorkspace(i);
	if (is_floating == True)
	{
		FFloatingHideShow();
		XSetInputFocus(fluorite.dpy, w->w, RevertToPointerRoot, CurrentTime);
		FApplyBorders();
	}
	return True;
}

static void FMapRequest(XEvent ev)
{
	Windows *nw = (Windows *) calloc(1, sizeof(Windows));
	XWindowAttributes wa;
	int is_floating = False;
	int is_fixed = False;

	if (FCountWindows(fluorite.ws[fluorite.cr_ws].t_wins) >= MAX_WINDOWS)
		return;

	if (!XGetWindowAttributes(fluorite.dpy, ev.xmaprequest.window, &wa))
		return;

	if (FCheckWindowToplevel(ev.xmaprequest.window))
	{
		if (JUMP_TO_URGENT)
			return;
		XWMHints *hints = XGetWMHints(fluorite.dpy, ev.xmaprequest.window);
		hints->flags |= XUrgencyHint;
		XSetWMHints(fluorite.dpy, ev.xmaprequest.window, hints);
		XChangeProperty(fluorite.dpy, fluorite.root, XA_WM_HINTS, XA_WM_HINTS, 32, PropModeAppend, (unsigned char *) NULL, 0);
		XFree(hints);
		return;
	}

	if (wa.override_redirect || wa.width <= 0 || wa.width <= 0)
	{
		XMapWindow(fluorite.dpy, ev.xmaprequest.window);
		return;
	}

	is_floating = FCheckWindowIsFloating(ev.xmaprequest.window);
	is_fixed = FCheckWindowIsFixed(ev.xmaprequest.window);

	if (fluorite.ws[fluorite.cr_ws].fs)
		FFullscreenToggle();

	nw->w = ev.xmaprequest.window;
	nw->pid = xdo_get_pid_window(fluorite.xdo, nw->w);
	nw->fc = 1;
	nw->fs = 0;
	XSelectInput(fluorite.dpy, nw->w, EnterWindowMask | FocusChangeMask | PropertyChangeMask | StructureNotifyMask | KeyPressMask);
	XGrabButton(fluorite.dpy, Button1, METAKEY, nw->w, False, ButtonPressMask | ButtonReleaseMask | ButtonMotionMask, GrabModeAsync, GrabModeAsync, None, None);
	XGrabButton(fluorite.dpy, Button3, METAKEY, nw->w, False, ButtonPressMask | ButtonReleaseMask | ButtonMotionMask, GrabModeAsync, GrabModeAsync, None, None);
	XMapWindow(fluorite.dpy, nw->w);
	XRaiseWindow(fluorite.dpy, nw->w);

	if (is_fixed)
		return;

	for (Windows *w = fluorite.ws[fluorite.cr_ws].t_wins; w != NULL; w = w->next)
	{
		w->fc = 0;
		XSetWindowBorder(fluorite.dpy, w->w, fluorite.conf.bu);
	}
	XSetInputFocus(fluorite.dpy, nw->w, RevertToPointerRoot, CurrentTime);
	XSetWindowBorderWidth(fluorite.dpy, nw->w, fluorite.conf.bw);
	FApplyActiveWindow(nw->w);

	if ((is_floating && AUTO_FLOATING) || OPEN_IN_FLOAT)
		FManageFloatingWindow(nw);
	else
		fluorite.ws[fluorite.cr_ws].t_wins = FAddWindow(fluorite.ws[fluorite.cr_ws].t_wins, nw);;

	XChangeProperty(fluorite.dpy, nw->w, XInternAtom(fluorite.dpy, "_NET_WM_DESKTOP", False), XA_CARDINAL, 32, PropModeReplace, (unsigned char *)&fluorite.cr_ws, 1);
	FRedrawWindows();
	FWarpCursor(nw->w);
	FUpdateClientList();
}

static void FManageFloatingWindow(Windows *nw)
{
	unsigned int ww, wh;
	Windows *cw = fluorite.ws[fluorite.cr_ws].f_wins;

	xdo_get_window_size(fluorite.xdo, nw->w, &ww, &wh);
	nw->wx = fluorite.mon[fluorite.cr_mon].mx + (fluorite.mon[fluorite.cr_mon].mw  - ww) / 2;
	nw->wy = fluorite.mon[fluorite.cr_mon].my + (fluorite.mon[fluorite.cr_mon].mh  - wh) / 2;
	nw->ww = ww;
	nw->wh = wh;
	nw->fs = 0;
	XMoveResizeWindow(fluorite.dpy, nw->w, nw->wx, nw->wy, nw->ww, nw->wh);
	fluorite.ws[fluorite.cr_ws].f_wins = FAddWindow(cw, nw);
}

static void FRedrawStackedLayout()
{
	Windows *raise;
	if (fluorite.ws[fluorite.cr_ws].t_wins == NULL)
		return ;

	for (Windows *w = fluorite.ws[fluorite.cr_ws].t_wins; w != NULL; w = w->next)
	{
		w->wx = fluorite.mon[fluorite.cr_mon].mx + (fluorite.conf.gp * 2);
		w->wy = fluorite.mon[fluorite.cr_mon].my + (fluorite.conf.gp * 2);
		w->ww = fluorite.mon[fluorite.cr_mon].mw - (fluorite.conf.bw * 2) - (fluorite.conf.gp * 4);
		w->wh = fluorite.mon[fluorite.cr_mon].mh - (fluorite.conf.bw * 2) - (fluorite.conf.gp * 4);
		if (fluorite.mon[fluorite.cr_mon].primary == True || PRIMARY_BAR_ONLY == False)
			w->wy += fluorite.conf.tb;
		if (fluorite.mon[fluorite.cr_mon].primary == True || PRIMARY_BAR_ONLY == False)
			w->wh -=  (fluorite.conf.tb + fluorite.conf.bb);
		XMoveResizeWindow(
			fluorite.dpy, w->w,
			w->wx, w->wy,
			w->ww, w->wh
		);
		raise = w;
	}
	for (; raise != NULL; raise = raise->prev)
		XRaiseWindow(fluorite.dpy, raise->w);
}

static void FRedrawDWMLayout()
{
	Windows *win = fluorite.ws[fluorite.cr_ws].t_wins;
	if (!win)
		return;

	int mon = fluorite.cr_mon;
	int mo = fluorite.ws[fluorite.cr_ws].mo;

	int mx = fluorite.mon[mon].mx;
	int my = fluorite.mon[mon].my;
	int mw = fluorite.mon[mon].mw;
	int mh = fluorite.mon[mon].mh;

	int gap = fluorite.conf.gp * 2;
	int border = fluorite.conf.bw;

	int bar_top = (fluorite.mon[mon].primary || !PRIMARY_BAR_ONLY) ? fluorite.conf.tb : 0;
	int bar_bottom = (fluorite.mon[mon].primary || !PRIMARY_BAR_ONLY) ? fluorite.conf.bb : 0;

	int usable_y = my + bar_top;
	int usable_height = mh - bar_top - bar_bottom;

	int n = 0;
	for (Windows *c = win; c; c = c->next)
		n++;

	if (n == 0)
		return;

	int master_width = (n > 1) ? (mw * 0.5) : mw;
	master_width += mo;
	master_width += fluorite.conf.gp;
	int stack_width = mw - master_width;
	int master_x = mx + gap;
	int master_y = usable_y + gap;
	int master_w = master_width - 2 * gap - 2 * border;
	int master_h = usable_height - 2 * gap - 2 * border;
	XMoveResizeWindow(fluorite.dpy, win->w, master_x, master_y, master_w, master_h);
	win = win->next;

	if (n > 1)
	{
		int sn = n - 1;
		int total_gap = gap * (sn - 1);
		int total_window_height = usable_height - 2 * gap - total_gap;
		int sh_each = total_window_height / sn;
		int stack_x = mx + master_width;
		for (int i = 0; win; win = win->next, i++)
		{
			int w = stack_width - 2 * gap - 2 * border + gap;
			int h = sh_each - 2 * border;
			int y = usable_y + gap + i * (sh_each + gap);
			XMoveResizeWindow(fluorite.dpy, win->w, stack_x, y, w, h);
		}
	}
}

// TODO: Need a fix
static void FRedrawFluoriteLayout()
{
	int position_offset = 0;
	int stack_count = 0;
	int size_offset;
	Windows *last = NULL;
	int mx = fluorite.mon[fluorite.cr_mon].mx;
	int my = fluorite.mon[fluorite.cr_mon].my;
	int mw = fluorite.mon[fluorite.cr_mon].mw;
	int mh = fluorite.mon[fluorite.cr_mon].mh;
	int gp = fluorite.conf.gp;
	int tb = fluorite.conf.tb;
	int bb = fluorite.conf.bb;
	int bw = fluorite.conf.bw;
	int mo = fluorite.ws[fluorite.cr_ws].mo;

	for (Windows *w = fluorite.ws[fluorite.cr_ws].t_wins->next; w != NULL; w = w->next, stack_count++)
		last = w;

	size_offset = stack_count - 1;
	size_offset *= fluorite.conf.so;
	size_offset *= 10;
	for (int i = stack_count - 1; i >= 0; i--, last = last->prev)
	{
		last->wx = mx + (mw / 2) + gp + (position_offset / stack_count) + mo;
		last->wy = my + (gp * 2) + (position_offset / stack_count);
		last->ww = (mw / 2) - (bw * 2) - (gp * 3) - (size_offset / stack_count) - mo;
		last->wh = mh - (bw * 2) - (gp * 4) - (size_offset / stack_count);
		if (fluorite.mon[fluorite.cr_mon].primary == True || !PRIMARY_BAR_ONLY)
		{
			last->wy += tb;
			last->wh -= tb;
			last->wh -= bb;
		}
		XRaiseWindow(fluorite.dpy, last->w);
		XMoveResizeWindow(fluorite.dpy, last->w,
			last->wx, last->wy,
			last->ww, last->wh
		);
		position_offset += fluorite.conf.so * 10;
	}

	fluorite.ws[fluorite.cr_ws].t_wins->wx = mx + (gp * 2);
	fluorite.ws[fluorite.cr_ws].t_wins->wy = my + (gp * 2);
	fluorite.ws[fluorite.cr_ws].t_wins->ww = (mw / 2) - (gp * 3) - (bw * 2) + mo;
	fluorite.ws[fluorite.cr_ws].t_wins->wh = mh - (bw * 2) - (gp * 4);
	if (fluorite.mon[fluorite.cr_mon].primary == True || !PRIMARY_BAR_ONLY)
	{
		fluorite.ws[fluorite.cr_ws].t_wins->wy += tb;
		fluorite.ws[fluorite.cr_ws].t_wins->wh -= tb;
		fluorite.ws[fluorite.cr_ws].t_wins->wh -= bb;
	}
	XRaiseWindow(fluorite.dpy, fluorite.ws[fluorite.cr_ws].t_wins->w);
	XMoveResizeWindow(fluorite.dpy, fluorite.ws[fluorite.cr_ws].t_wins->w,
		fluorite.ws[fluorite.cr_ws].t_wins->wx, fluorite.ws[fluorite.cr_ws].t_wins->wy,
		fluorite.ws[fluorite.cr_ws].t_wins->ww, fluorite.ws[fluorite.cr_ws].t_wins->wh
	);
}



static void FRedrawFullscreen()
{
	Windows *w;
	for (w = fluorite.ws[fluorite.cr_ws].t_wins; w != NULL; w = w->next)
		if (w->fs)
			goto found;
	for (w = fluorite.ws[fluorite.cr_ws].f_wins; w != NULL; w = w->next)
		if (w->fs)
			goto found;
return;
	
found:
	XMoveResizeWindow(fluorite.dpy, w->w,
			fluorite.mon[fluorite.cr_mon].mx, fluorite.mon[fluorite.cr_mon].my,
			fluorite.mon[fluorite.cr_mon].mw, fluorite.mon[fluorite.cr_mon].mh
		);
	XRaiseWindow(fluorite.dpy, w->w);
	XSetInputFocus(fluorite.dpy, w->w, RevertToPointerRoot, CurrentTime);
}

static void FRedrawWindows()
{
	if (!fluorite.ws[fluorite.cr_ws].t_wins)
		return ;

	if (fluorite.ws[fluorite.cr_ws].fs)
	{
		FRedrawFullscreen();
		return ;
	}
	
	if ((fluorite.ws[fluorite.cr_ws].t_wins && !fluorite.ws[fluorite.cr_ws].t_wins->next) || fluorite.ws[fluorite.cr_ws].layout == STACKED)
		FRedrawStackedLayout();
	else if (fluorite.ws[fluorite.cr_ws].layout == DWM)
		FRedrawDWMLayout();
	else
		FRedrawFluoriteLayout();

	for (Windows *w = fluorite.ws[fluorite.cr_ws].f_wins; w != NULL; w = w->next)
	{
		XRaiseWindow(fluorite.dpy, w->w);
		XMoveResizeWindow(fluorite.dpy, w->w, w->wx, w->wy, w->ww, w->wh);
	}
}

static void FApplyActiveWindow(Window w)
{
	Atom net_active_window = XInternAtom(fluorite.dpy, "_NET_ACTIVE_WINDOW", False);
	XChangeProperty(fluorite.dpy, fluorite.root,
			net_active_window, XA_WINDOW, 32,
			PropModeReplace, (unsigned char *)&w, 1);
	if (WARP_CURSOR && !no_warp)
		FWarpCursor(w);
}

static void FApplyBorders()
{
	Window focused;
	int revert;

	XGetInputFocus(fluorite.dpy, &focused, &revert);
	if (focused == fluorite.root)
		return ;

	for (Windows *w = fluorite.ws[fluorite.cr_ws].f_wins; w; w = w->next)
	{
		if (focused == w->w)
			FApplyActiveWindow(focused);
		XSetWindowBorderWidth(fluorite.dpy, w->w, fluorite.conf.bw);
		XSetWindowBorder(fluorite.dpy, w->w, fluorite.conf.bu);
	}

	for (Windows *w = fluorite.ws[fluorite.cr_ws].t_wins; w; w = w->next)
	{
		if (focused == w->w)
			FApplyActiveWindow(focused);
		XSetWindowBorderWidth(fluorite.dpy, w->w, fluorite.conf.bw);
		XSetWindowBorder(fluorite.dpy, w->w, fluorite.conf.bu);
	}

	XSetWindowBorder(fluorite.dpy, focused, fluorite.conf.bf);
}

static int FCheckWindowIsFloating(Window w)
{
	int di;
	unsigned long dl;
	unsigned char *p = NULL;
	Atom da, atom = None;
	XClassHint name;
	int ret_val = False;
	Window tr;

	if (XGetClassHint(fluorite.dpy, w, &name))
	{
		for (long unsigned int i = 0; i < LENGTH(default_floating); i++)
		{
			if (strcmp(default_floating[i].wm_class, name.res_class) == 0 ||
					strcmp(default_floating[i].wm_class, name.res_name) == 0)
				return True;
		}
	}

	if (XGetWindowProperty(fluorite.dpy, w, XInternAtom(fluorite.dpy, "_NET_WM_WINDOW_TYPE", False), 0L, sizeof(atom), False, XA_ATOM, &da, &di, &dl, &dl, &p) == Success && p)
	{
		atom = *(Atom *) p;
		if (atom == XInternAtom(fluorite.dpy, "_NET_WM_WINDOW_TYPE_DIALOG", False))
			ret_val = True;
		else if (atom == XInternAtom(fluorite.dpy, "_NET_WM_WINDOW_TYPE_UTILITY", False))
			ret_val = True;
		else if (atom == XInternAtom(fluorite.dpy, "_NET_WM_WINDOW_TYPE_TOOLBAR", False))
			ret_val = True;
		else if (atom == XInternAtom(fluorite.dpy, "_NET_WM_WINDOW_TYPE_SPLASH", False))
			ret_val = True;
		else if (atom == XInternAtom(fluorite.dpy, "_NET_WM_WINDOW_TYPE_POPUP_MENU", False))
			ret_val = True;
	}

	if (XGetTransientForHint(fluorite.dpy, w, &tr))
		ret_val = True;

	if (OPEN_IN_FLOAT)
		ret_val = True;
	
	XFree(p);
	return ret_val;
}

static int FCheckWindowIsFixed(Window w)
{
	
	long msize;
	XSizeHints size;
	XClassHint name;
	int maxw, maxh, minw, minh, di;
	unsigned long dl;
	unsigned char *p = NULL;
	Atom da, atom = None;

	if (XGetClassHint(fluorite.dpy, w, &name))
	{
		for (long unsigned int i = 0; i < LENGTH(default_fixed); i++)
		{
			if (strcmp(default_fixed[i].wm_class, name.res_class) == 0 || strcmp(default_fixed[i].wm_class, name.res_name) == 0)
				return True;
		}
	}
	if (XGetWindowProperty(fluorite.dpy, w, XInternAtom(fluorite.dpy, "_NET_WM_WINDOW_TYPE", False), 0L, sizeof(atom), False, XA_ATOM, &da, &di, &dl, &dl, &p) == Success && p)
	{
		atom = *(Atom *)p;
		if (atom == XInternAtom(fluorite.dpy, "_NET_WM_WINDOW_TYPE_NORMAL", False))
		{
			XFree(p);
			return False;
		}
	}
	if (!XGetWMNormalHints(fluorite.dpy, w, &size, &msize))
		return False;
	if (size.flags & PMaxSize)
	{
		maxw = size.max_width;
		maxh = size.max_height;
	}
	else
		maxw = maxh = 0;
	if (size.flags & PMinSize)
	{
		minw = size.min_width;
		minh = size.min_height;
	}
	else if (size.flags & PBaseSize)
	{
		minw = size.base_width;
		minh = size.base_height;
	}
	else
		minw = minh = 0;
	if (maxw && maxh && maxw == minw && maxh == minh)
		return True;
	return False;
}

static void FChangeMonitor(int mon)
{
	Atom net_active_window = XInternAtom(fluorite.dpy, "_NET_ACTIVE_WINDOW", False);

	for (Windows *w = fluorite.ws[fluorite.cr_ws].t_wins; w != NULL; w = w->next)
	{
		w->fc = 0;
		XSetWindowBorder(fluorite.dpy, w->w, fluorite.conf.bu);
	}
	for (Windows *w = fluorite.ws[fluorite.cr_ws].f_wins; w != NULL; w = w->next)
	{
		w->fc = 0;
		XSetWindowBorder(fluorite.dpy, w->w, fluorite.conf.bu);
	}
	XDeleteProperty(fluorite.dpy, fluorite.root, net_active_window);
	XSetInputFocus(fluorite.dpy, fluorite.root, RevertToPointerRoot, CurrentTime);
	fluorite.cr_mon = mon;
	fluorite.cr_ws = fluorite.mon[mon].ws;
	XChangeProperty(fluorite.dpy, fluorite.root, XInternAtom(fluorite.dpy, "_NET_CURRENT_DESKTOP", False), XA_CARDINAL, 32, PropModeReplace, (unsigned char *)&fluorite.cr_ws, 1);
}

static void FConfigureRequest(XEvent ev)
{
	for (int i = 0; i < MAX_WS; i++)
	{
		for (Windows *w = fluorite.ws[i].t_wins; w != NULL; w = w->next)
			if (ev.xconfigurerequest.window == w->w)
				return;
		for (Windows *w = fluorite.ws[i].f_wins; w != NULL; w = w->next)
			if (ev.xconfigurerequest.window == w->w)
				return;
	}

	XWindowChanges wc;
	wc.x = ev.xconfigurerequest.x;
	wc.y = ev.xconfigurerequest.y;
	wc.width = ev.xconfigurerequest.width;
	wc.height = ev.xconfigurerequest.height;
	wc.border_width = ev.xconfigurerequest.border_width;
	wc.sibling = ev.xconfigurerequest.above;
	wc.stack_mode = ev.xconfigurerequest.detail;
	XConfigureWindow(fluorite.dpy, ev.xconfigurerequest.window, ev.xconfigurerequest.value_mask, &wc);
}

static void FKeyPress(XEvent ev)
{
	XKeyPressedEvent e = ev.xkey;
	for (long unsigned int i = 0; i < LENGTH(bind); i++)
		if (e.keycode == XKeysymToKeycode(fluorite.dpy, bind[i].key) && MODMASK(bind[i].mod) == MODMASK(e.state) && bind[i].func)
				bind[i].func();
}

static int FFindWorkspaceFromWindow(Window w)
{
	for (int i = 0; i < MAX_WS; i++)
	{
		for (Windows *sw = fluorite.ws[i].t_wins; sw; sw = sw->next)
			if (sw->w == w)
				return i;
		for (Windows *sw = fluorite.ws[i].f_wins; sw; sw = sw->next)
			if (sw->w == w)
				return i;
	}
	return -1;
}

static void FResetFocus(Windows *w)
{
	for (; w != NULL; w = w->next)
		w->fc = 0;
}

static void FUnmapNotify(XEvent ev)
{
	int ws;
	int found = 0;
	Atom net_active_window = XInternAtom(fluorite.dpy, "_NET_ACTIVE_WINDOW", False);

	if (no_unmap)
		return;

	ws = FFindWorkspaceFromWindow(ev.xunmap.window);
	if (ws == -1)
		return;

	for (Windows *i = fluorite.ws[ws].t_wins; i && found != 1; i = i->next)
	{
		if (i->w == ev.xunmap.window)
		{
			FResetFocus(fluorite.ws[fluorite.cr_ws].t_wins);
			fluorite.ws[ws].t_wins = FDelWindow(fluorite.ws[ws].t_wins, i);
			free(i);
			goto redraw;
		}
	}
	for (Windows *i = fluorite.ws[ws].f_wins; i; i = i->next)
	{
		if (i->w == ev.xunmap.window)
		{
			FResetFocus(fluorite.ws[fluorite.cr_ws].f_wins);
			fluorite.ws[ws].f_wins = FDelWindow(fluorite.ws[ws].f_wins, i);
			free(i);
			goto redraw;
		}
	}

redraw:
	FUpdateClientList();
	XDeleteProperty(fluorite.dpy, fluorite.root, net_active_window);
	FRedrawWindows();
	FApplyBorders();
}

static Windows *FAddWindow(Windows *cw, Windows *nw)
{
	for (Windows *i = cw; i != NULL; i = i->next)
		i->fc = 0;

	if (cw)
	{
		cw->fc = 0;
		cw->fs = 0;
		cw->prev = nw;
		nw->next = cw;
		nw->prev = NULL;
	}
	return nw;
}

static Windows *FDelWindow(Windows *cw, Windows *i)
{
	if (!i->next && i == cw)
	{
		cw = NULL;
		XSetInputFocus(fluorite.dpy, fluorite.root, RevertToPointerRoot, CurrentTime);
	}
	else if (i->next && i == cw)
	{
		i->next->prev = NULL;
		cw = i->next;
		cw->fc = 1;
		XSetInputFocus(fluorite.dpy, cw->w, RevertToPointerRoot, CurrentTime);
	}
	else
	{
		if (i->prev)
		{
			i->prev->next = i->next;
			i->prev->fc = 1;
			XSetInputFocus(fluorite.dpy, i->prev->w, RevertToPointerRoot, CurrentTime);
		}
		if (i->next)
		{
			i->next->prev = i->prev;
			i->next->fc = 1;
			XSetInputFocus(fluorite.dpy, i->next->w, RevertToPointerRoot, CurrentTime);
		}
	}
	return cw;
}

void FExecute(char *argument)
{
	strcat(argument, " &");
	if (system(argument) == -1)
		printf("Failed to execute program\n");
}

void FQuit()
{
	free(fluorite.mon);
	xdo_free(fluorite.xdo);
	XCloseDisplay(fluorite.dpy);
}

void FCloseWindow()
{
	XEvent closing;
	Window focused;
	int revert;

	XGetInputFocus(fluorite.dpy, &focused, &revert);
	if (focused == fluorite.root)
		return;
	memset(&closing, 0, sizeof(closing));
	closing.xclient.type = ClientMessage;
	closing.xclient.message_type = XInternAtom(fluorite.dpy, "WM_PROTOCOLS", False);
	closing.xclient.format = 32;
	closing.xclient.data.l[0] = XInternAtom(fluorite.dpy, "WM_DELETE_WINDOW", False);
	closing.xclient.window = focused;
	XSendEvent(fluorite.dpy, focused, False, 0, &closing);
}

Window FWindowUnderCursor()
{
    Window returned_root, child;
    int root_x, root_y, win_x, win_y;
    unsigned int mask;

    if (XQueryPointer(fluorite.dpy, fluorite.root, &returned_root, &child,
                      &root_x, &root_y, &win_x, &win_y, &mask)) {
        return child;
    }
    return None;
}

static void FFocusWindowUnderCursor()
{
	Window target = FWindowUnderCursor();
	Atom net_active_window = XInternAtom(fluorite.dpy, "_NET_ACTIVE_WINDOW", False);

	no_warp = True;
	if (target == None)
	{
		XDeleteProperty(fluorite.dpy, fluorite.root, net_active_window);
		return ;
	}

	for (Windows *w = fluorite.ws[fluorite.cr_ws].t_wins; w != NULL; w = w->next)
	{
		if (w->w == target)
		{
			w->fc = 1;
			XSetInputFocus(fluorite.dpy, w->w, RevertToPointerRoot, CurrentTime);
		}
		else
			w->fc = 0;
	}
	for (Windows *w = fluorite.ws[fluorite.cr_ws].f_wins; w != NULL; w = w->next)
	{
		if (w->w == target)
		{
			w->fc = 1;
			XSetInputFocus(fluorite.dpy, w->w, RevertToPointerRoot, CurrentTime);
		}
		else
			w->fc = 0;
	}

	if (!fluorite.ws[fluorite.cr_ws].fs)
		FApplyBorders();
	no_warp = False;
}

static Window FGetToplevel(Window w)
{
	while (1)
	{
		Window root, parent;
		Window *children;
		unsigned int nchildren;
		if (!XQueryTree(fluorite.dpy, w, &root, &parent, &children, &nchildren) || parent == root || parent == 0)
			break;
		w = parent;
		if (children) XFree(children);
	}
	return w;
}


static void FButtonPress(XEvent ev)
{
	unsigned b_w, d;
	unsigned xdo_w, xdo_h;
	Screen *scr;
	Window target;

	if (fluorite.ws[fluorite.cr_ws].fs)
		return ;

	target = FGetToplevel(ev.xbutton.window);
	fluorite.mouse.spx = ev.xbutton.x_root;
	fluorite.mouse.spy = ev.xbutton.y_root;
	XGetGeometry(fluorite.dpy, target, &fluorite.root, &fluorite.mouse.swx, &fluorite.mouse.swy, &fluorite.mouse.sww, &fluorite.mouse.swh, &b_w, &d);
	for (Windows *w = fluorite.ws[fluorite.cr_ws].t_wins; w != NULL; w = w->next)
	{
		if (target == w->w)
		{
			fluorite.ws[fluorite.cr_ws].t_wins = FDelWindow(fluorite.ws[fluorite.cr_ws].t_wins, w);
			w->next = NULL;
			w->prev = NULL;
			FManageFloatingWindow(w);
			xdo_get_window_size(fluorite.xdo, w->w, &xdo_w, &xdo_h);
			xdo_get_window_location(fluorite.xdo, w->w, &w->wx, &w->wy, &scr);
			w->ww = xdo_w;
			w->wh = xdo_h;
			XResizeWindow(fluorite.dpy, w->w, w->ww, w->wh);
			XMoveWindow(fluorite.dpy, w->w, w->wx, w->wy);
		}
	}
	FRedrawWindows();
}

static void FClientMessage(XEvent ev)
{
	no_warp = True;
	if (ev.xclient.message_type == XInternAtom(fluorite.dpy, "_NET_CURRENT_DESKTOP", False))
		FShowWorkspace((int)ev.xclient.data.l[0]);
	no_warp = False;
}

static void FMotionNotify(XEvent ev)
{
	int dpx, dpy, ddx, ddy;
	long lhints;
	XSizeHints hints;
	Windows *target;

	if (fluorite.ws[fluorite.cr_ws].fs)
		return ;

	if (!XGetWMNormalHints(fluorite.dpy, ev.xmotion.window, &hints, &lhints))
		return ;

	no_warp = True;
	dpx = ev.xmotion.x_root - fluorite.mouse.spx;
	dpy = ev.xmotion.y_root - fluorite.mouse.spy;

	for (target = fluorite.ws[fluorite.cr_ws].f_wins; target != NULL; target = target->next)
		if (ev.xmotion.window == target->w)
			break;

	if (!target)
		return ;

	if (ev.xmotion.state & Button1Mask)
	{
		ddx = fluorite.mouse.swx + dpx;
		ddy = fluorite.mouse.swy + dpy;
		XMoveWindow(fluorite.dpy, target->w, ddx, ddy);
		target->wx = ddx;
		target->wy = ddy;
		hints.x = ddx;
		hints.y = ddy;
	}
	else if (ev.xmotion.state & Button3Mask)
	{
		ddx = fluorite.mouse.sww + dpx;
		ddy = fluorite.mouse.swh + dpy;
		XResizeWindow(fluorite.dpy, target->w, ddx, ddy);
		target->ww = ddx;
		target->wh = ddy;
		hints.width = ddx;
		hints.height = ddy;
	}

	XRaiseWindow(fluorite.dpy, target->w);
	FResetFocus(fluorite.ws[fluorite.cr_ws].f_wins);
	target->fc = 1;
	XSetInputFocus(fluorite.dpy, target->w, RevertToPointerRoot, CurrentTime);
	XSetWMNormalHints(fluorite.dpy, target->w, &hints);
	FApplyBorders();
	no_warp = False;
}

static void FWarpCursor(Window w)
{
	int x, y, ww, wh;
	Window dummy;

	XGetGeometry(fluorite.dpy, w, &dummy, &x, &y, (unsigned int *)&ww, (unsigned int *)&wh, (unsigned int *)&x, (unsigned int *)&y);
	XWarpPointer(fluorite.dpy, None, w, 0, 0, 0, 0, ww / 2, wh / 2);
}

static void FSetWindowOpacity(Window w, double opacity)
{
    unsigned long op = (unsigned long)(opacity * 0xFFFFFFFF);
    Atom atom = XInternAtom(fluorite.dpy, "_NET_WM_WINDOW_OPACITY", False);

    if (opacity < 0.0) opacity = 0.0;
    if (opacity > 1.0) opacity = 1.0;
    XChangeProperty(fluorite.dpy, w, atom, XA_CARDINAL, 32, PropModeReplace, (unsigned char *)&op, 1);
}

static void FUpdateClientList()
{
	Window list[(MAX_WINDOWS * 2) * MAX_WS];
	int list_idx = 0;

	for (int i = 0; i < MAX_WS; i++)
	{
		for (Windows *w = fluorite.ws[i].t_wins; w != NULL; w = w->next, list_idx++)
			list[list_idx] = w->w;
		for (Windows *w = fluorite.ws[i].f_wins; w != NULL; w = w->next, list_idx++)
			list[list_idx] = w->w;
	}
	XChangeProperty(fluorite.dpy, fluorite.root, XInternAtom(fluorite.dpy, "_NET_CLIENT_LIST", False), XA_WINDOW, 32, PropModeReplace, (unsigned char *)list, list_idx);
}

static void FResetWindowOpacity(Window w)
{
	Atom atom = XInternAtom(fluorite.dpy, "_NET_WM_WINDOW_OPACITY", False);
	XDeleteProperty(fluorite.dpy, w, atom);
}

static void FSwapMonitorWorkspace(int ws, int mon)
{
	int swap_mon = fluorite.cr_mon;

	for (Windows *w = fluorite.ws[fluorite.cr_ws].t_wins; w != NULL; w = w->next)
	{
		w->fc = 0;
		XSetWindowBorder(fluorite.dpy, w->w, fluorite.conf.bu);
	}
	for (Windows *w = fluorite.ws[fluorite.cr_ws].f_wins; w != NULL; w = w->next)
	{
		w->fc = 0;
		XSetWindowBorder(fluorite.dpy, w->w, fluorite.conf.bu);
	}

	fluorite.mon[mon].ws = fluorite.cr_ws;
	fluorite.cr_mon = mon;
	FRedrawWindows();
	fluorite.cr_mon = swap_mon;
	fluorite.cr_ws = ws;
	fluorite.mon[fluorite.cr_mon].ws = ws;
	XChangeProperty(fluorite.dpy, fluorite.root, XInternAtom(fluorite.dpy, "_NET_CURRENT_DESKTOP", False), XA_CARDINAL, 32, PropModeReplace, (unsigned char *)&fluorite.cr_ws, 1);
	XSetInputFocus(fluorite.dpy, fluorite.root, RevertToPointerRoot, CurrentTime);
	FRedrawWindows();
	FApplyBorders();
}

void FShowWorkspace(int ws)
{
	Atom net_active_window = XInternAtom(fluorite.dpy, "_NET_ACTIVE_WINDOW", False);

	if (ws == fluorite.cr_ws)
		return ;

	for (int i = 0; i < fluorite.ct_mon; i++)
	{
		if (i == fluorite.cr_mon)
			continue;

		if (fluorite.mon[i].ws == ws)
		{
			FSwapMonitorWorkspace(ws, i);
			return ;
		}
	}

	no_unmap = True;
	XGrabServer(fluorite.dpy);

	for (Windows *w = fluorite.ws[fluorite.cr_ws].t_wins; w != NULL; w = w->next)
		XUnmapWindow(fluorite.dpy, w->w);
	for (Windows *w = fluorite.ws[fluorite.cr_ws].f_wins; w != NULL; w = w->next)
		XUnmapWindow(fluorite.dpy, w->w);

	XSync(fluorite.dpy, True);
	fluorite.cr_ws = ws;
	fluorite.mon[fluorite.cr_mon].ws = ws;
	XSetInputFocus(fluorite.dpy, fluorite.root, RevertToPointerRoot, CurrentTime);
	XDeleteProperty(fluorite.dpy, fluorite.root, net_active_window);

	for (Windows *w = fluorite.ws[fluorite.cr_ws].t_wins; w != NULL; w = w->next)
		XMapWindow(fluorite.dpy, w->w);
	for (Windows *w = fluorite.ws[fluorite.cr_ws].f_wins; w != NULL && !fluorite.ws[fluorite.cr_ws].fl_hdn; w = w->next)
		XMapWindow(fluorite.dpy, w->w);

	if (fluorite.ws[fluorite.cr_ws].t_wins)
	{
		for (Windows *w = fluorite.ws[fluorite.cr_ws].t_wins; w != NULL; w = w->next)
			if (w->fc)
				XSetInputFocus(fluorite.dpy, w->w, RevertToPointerRoot, CurrentTime);
	}
	else if (fluorite.ws[fluorite.cr_ws].f_wins && !fluorite.ws[fluorite.cr_ws].fl_hdn)
	{
		for (Windows *w = fluorite.ws[fluorite.cr_ws].f_wins; w != NULL; w = w->next)
			if (w->fc)
				XSetInputFocus(fluorite.dpy, w->w, RevertToPointerRoot, CurrentTime);
	}

	XChangeProperty(fluorite.dpy, fluorite.root, XInternAtom(fluorite.dpy, "_NET_CURRENT_DESKTOP", False), XA_CARDINAL, 32, PropModeReplace, (unsigned char *)&fluorite.cr_ws, 1);
	XSync(fluorite.dpy, True);
	XUngrabServer(fluorite.dpy);
	no_unmap = False;

	FRedrawWindows();
	FApplyBorders();
}

void FSendWindowToWorkspace(int ws)
{
	Window focused;
	int revert;
	Windows *w;

	if (ws == fluorite.cr_ws || (!fluorite.ws[fluorite.cr_ws].t_wins && !fluorite.ws[fluorite.cr_ws].f_wins))
		return ;

	no_unmap = True;
	XGrabServer(fluorite.dpy);
	XGetInputFocus(fluorite.dpy, &focused, &revert);

	for (w = fluorite.ws[fluorite.cr_ws].t_wins; w != NULL; w = w->next)
	{
		if (focused == w->w)
		{
			fluorite.ws[fluorite.cr_ws].t_wins = FDelWindow(fluorite.ws[fluorite.cr_ws].t_wins, w);
			w->next = NULL;
			w->prev = NULL;
			fluorite.ws[ws].t_wins = FAddWindow(fluorite.ws[ws].t_wins, w);
			goto next;
		}
	}

	for (w = fluorite.ws[fluorite.cr_ws].f_wins; w != NULL; w = w->next)
	{
		if (focused == w->w)
		{
			fluorite.ws[ws].f_wins = FAddWindow(fluorite.ws[ws].f_wins, w);
			w->next = NULL;
			w->prev = NULL;
			fluorite.ws[fluorite.cr_ws].f_wins = FDelWindow(fluorite.ws[fluorite.cr_ws].f_wins, w);
			goto next;
		}
	}

next:
	if (w->fs)
	{
		w->fs = 0;
		fluorite.ws[fluorite.cr_ws].fs = 0;
		XSetWindowBorderWidth(fluorite.dpy, w->w, fluorite.conf.bw);
		XSetWindowBorder(fluorite.dpy, w->w, fluorite.conf.bu);
		FResetWindowOpacity(w->w);
	}

	XUnmapWindow(fluorite.dpy, w->w);
	XSync(fluorite.dpy, True);
	XUngrabServer(fluorite.dpy);
	no_unmap = False;

	for (int i = 0; i < fluorite.ct_mon; i++)
	{
		if (ws == fluorite.mon[i].ws)
		{
			int swap_mon = fluorite.cr_mon;
			int swap_ws = fluorite.cr_ws;
			fluorite.cr_mon = i;
			fluorite.cr_ws = fluorite.mon[i].ws;
			XSetWindowBorder(fluorite.dpy, w->w, fluorite.conf.bu);
			XMapWindow(fluorite.dpy, w->w);
			FRedrawWindows();
			fluorite.cr_mon = swap_mon;
			fluorite.cr_ws = swap_ws;
			FRedrawWindows();
			break;
		}
	}

	XChangeProperty(fluorite.dpy, w->w, XInternAtom(fluorite.dpy, "_NET_WM_DESKTOP", False), XA_CARDINAL, 32, PropModeReplace, (unsigned char *)&ws, 1);
	FUpdateClientList();
	FRedrawWindows();

	if (FOLLOW_WINDOWS)
		FShowWorkspace(ws);
}

void FNextWorkspace()
{
	int ws = fluorite.cr_ws + 1;
	if (ws == MAX_WS)
		ws = 0;
	FShowWorkspace(ws);
}

void FPrevWorkspace()
{
	int ws = fluorite.cr_ws - 1;
	if (ws < 0)
		ws = MAX_WS - 1;
	FShowWorkspace(ws);
}

void FRotateSlavesWindows(int mode)
{
	Windows *first;
	Windows *last;
	Windows *ws_head;
	Windows *master;
	Windows *moved;

	switch (mode)
	{
		case UP:
			ws_head = fluorite.ws[fluorite.cr_ws].t_wins;
			if (!ws_head || !ws_head->next)
				return;
			master = ws_head;
			first = master->next;
			last = first;
			while (last->next)
				last = last->next;
			moved = first;
			master->next = moved->next;
			if (moved->next)
				moved->next->prev = master;
			last->next = moved;
			moved->prev = last;
			moved->next = NULL;
			break;
		case DOWN:
			ws_head = fluorite.ws[fluorite.cr_ws].t_wins;
			if (!ws_head || !ws_head->next)
				return;
			master = ws_head;
			first = master->next;
			last = first;
			while (last->next)
				last = last->next;
			if (last->prev)
				last->prev->next = NULL;
			last->prev = master;
			last->next = first;
			first->prev = last;
			master->next = last;
			break;
	}
	FRedrawWindows();
}

void FRotateWindows(int mode)
{
	Windows *first;
	Windows *last;

	switch (mode)
	{
		case UP:
			if (!fluorite.ws[fluorite.cr_ws].t_wins || !fluorite.ws[fluorite.cr_ws].t_wins->next)
				break;
			first = fluorite.ws[fluorite.cr_ws].t_wins;
			last = first;
			while (last->next)
				last = last->next;
			fluorite.ws[fluorite.cr_ws].t_wins = first->next;
			fluorite.ws[fluorite.cr_ws].t_wins->prev = NULL;
			last->next = first;
			first->prev = last;
			first->next = NULL;
			break;
		case DOWN:
			if (!fluorite.ws[fluorite.cr_ws].t_wins || !fluorite.ws[fluorite.cr_ws].t_wins->next)
				break;
			last = fluorite.ws[fluorite.cr_ws].t_wins;
			while (last->next)
				last = last->next;
			if (last->prev)
				last->prev->next = NULL;
			first = fluorite.ws[fluorite.cr_ws].t_wins;
			last->prev = NULL;
			last->next = first;
			first->prev = last;
			fluorite.ws[fluorite.cr_ws].t_wins = last;
			break;
		default:
			break;
	}
	FRedrawWindows();
}

void FChangeMasterOffset(int mode)
{
	switch (mode)
	{
		case UP:
			fluorite.ws[fluorite.cr_ws].mo += 25;
			break;
		case DOWN:
			fluorite.ws[fluorite.cr_ws].mo -= 25;
			break;
		case RESET:
			FLoadXresources();
			break;
		default:
			break;
	}
	FRedrawWindows();
}

// TODO: Fix this when floating will be implemented fully
void FSwapWithMaster()
{
	Window focused;
	int revert;
	XEvent ev;
	Windows *w;

	XGetInputFocus(fluorite.dpy, &focused, &revert);
	if (focused == fluorite.root || !fluorite.ws[fluorite.cr_ws].t_wins || focused == fluorite.ws[fluorite.cr_ws].t_wins->w)
		return;

	for (w = fluorite.ws[fluorite.cr_ws].t_wins; w != NULL; w = w->next)
		if (w->w == focused)
			break;

	fluorite.ws[fluorite.cr_ws].t_wins = FDelWindow(fluorite.ws[fluorite.cr_ws].t_wins, w);
	ev.xmaprequest.window = focused;
	FMapRequest(ev);
}

void FFocusNext()
{
	if (fluorite.ws[fluorite.cr_ws].layout == FLUORITE)
	{
		FFocusPrev();
		return;
	}

	for (Windows *w = fluorite.ws[fluorite.cr_ws].t_wins; w != NULL; w = w->next)
	{
		if (w->fc)
		{
			w->fc = 0;
			if (w->next)
			{
				w->next->fc = 1;
				XSetInputFocus(fluorite.dpy, w->next->w, RevertToPointerRoot, CurrentTime);
				if (WARP_CURSOR)
					FWarpCursor(w->next->w);
			}
			else
			{
				fluorite.ws[fluorite.cr_ws].t_wins->fc = 1;
				XSetInputFocus(fluorite.dpy, fluorite.ws[fluorite.cr_ws].t_wins->w, RevertToPointerRoot, CurrentTime);
				if (WARP_CURSOR)
					FWarpCursor(fluorite.ws[fluorite.cr_ws].t_wins->w);
			}
			break;
		}
	}
	FApplyBorders();
}

void FFocusPrev()
{
	Windows *last;

	if (!fluorite.ws[fluorite.cr_ws].t_wins || !fluorite.ws[fluorite.cr_ws].t_wins->next)
		return;

	for (last = fluorite.ws[fluorite.cr_ws].t_wins; last->next != NULL; last = last->next);
	for (Windows *w = fluorite.ws[fluorite.cr_ws].t_wins; w != NULL; w = w->next)
	{
		if (w->fc)
		{
			w->fc = 0;
			if (w->prev)
			{
				w->prev->fc = 1;
				XSetInputFocus(fluorite.dpy, w->prev->w, RevertToPointerRoot, CurrentTime);
				if (WARP_CURSOR)
					FWarpCursor(w->prev->w);
			}
			else
			{
				if (fluorite.ws[fluorite.cr_ws].layout == FLUORITE)
				{
					fluorite.ws[fluorite.cr_ws].t_wins->next->fc = 1;
					XSetInputFocus(fluorite.dpy, fluorite.ws[fluorite.cr_ws].t_wins->next->w, RevertToPointerRoot, CurrentTime);
					if (WARP_CURSOR)
						FWarpCursor(fluorite.ws[fluorite.cr_ws].t_wins->next->w);
					break;
				}
				last->fc = 1;
				XSetInputFocus(fluorite.dpy, last->w, RevertToPointerRoot, CurrentTime);
				if (WARP_CURSOR)
					FWarpCursor(last->w);
			}
			break;
		}
	}
	FApplyBorders();
}

void FTileWindow()
{
	Window focused;
	int revert;

	XGetInputFocus(fluorite.dpy, &focused, &revert);
	for (Windows *w = fluorite.ws[fluorite.cr_ws].f_wins; w != NULL; w = w->next)
	{
		if (focused == w->w)
		{
			fluorite.ws[fluorite.cr_ws].f_wins = FDelWindow(fluorite.ws[fluorite.cr_ws].f_wins, w);
			w->prev = NULL;
			w->next = NULL;
			FResetFocus(fluorite.ws[fluorite.cr_ws].t_wins);
			w->fc = 1;
			fluorite.ws[fluorite.cr_ws].t_wins = FAddWindow(fluorite.ws[fluorite.cr_ws].t_wins, w);
			XSetInputFocus(fluorite.dpy, w->w, RevertToPointerRoot, CurrentTime);
			FApplyActiveWindow(w->w);
		}
	}
	FRedrawWindows();
	FApplyBorders();
}

void FTileAllWindows()
{
	Windows *w;
	Windows *prev;

	XGrabServer(fluorite.dpy);
	for (w = fluorite.ws[fluorite.cr_ws].f_wins; w->next != NULL; w = w->next);

	prev = w->prev;
	fluorite.ws[fluorite.cr_ws].t_wins = FAddWindow(fluorite.ws[fluorite.cr_ws].t_wins, w);
	fluorite.ws[fluorite.cr_ws].t_wins->fc = 0;
	fluorite.ws[fluorite.cr_ws].t_wins->prev = prev;
	fluorite.ws[fluorite.cr_ws].t_wins = fluorite.ws[fluorite.cr_ws].f_wins;
	fluorite.ws[fluorite.cr_ws].f_wins = NULL;

	XSync(fluorite.dpy, True);
	XUngrabServer(fluorite.dpy);
	FRedrawWindows();
	FApplyBorders();
}

void FChangeLayout(int layout)
{
	if (fluorite.ws[fluorite.cr_ws].layout == layout)
		fluorite.ws[fluorite.cr_ws].layout = STARTING_LAYOUT;
	else
		fluorite.ws[fluorite.cr_ws].layout = layout;

	FRedrawWindows();
	FApplyBorders();
}

void FFullscreenToggle()
{
	Window focused;
	int revert;

	XGetInputFocus(fluorite.dpy, &focused, &revert);
	if (fluorite.ws[fluorite.cr_ws].fs)
	{
		FResetWindowOpacity(focused);
		XSetWindowBorderWidth(fluorite.dpy, focused, fluorite.conf.bw);
		goto found;
	}
	else
	{
		FSetWindowOpacity(focused, 100);
		XSetWindowBorderWidth(fluorite.dpy, focused, 0);
		goto found;
	}
	goto exit;
	

found:
	for (Windows *w = fluorite.ws[fluorite.cr_ws].t_wins; w != NULL; w = w->next)
		if (focused == w->w)
			w->fs = !w->fs;
	for (Windows *w = fluorite.ws[fluorite.cr_ws].f_wins; w != NULL; w = w->next)
		if (focused == w->w)
			w->fs = !w->fs;
	fluorite.ws[fluorite.cr_ws].fs = !fluorite.ws[fluorite.cr_ws].fs;
	FRedrawWindows();

exit:
	return ;
}

void FFloatingHideShow()
{
	Atom net_active_window = XInternAtom(fluorite.dpy, "_NET_ACTIVE_WINDOW", False);

	if (fluorite.ws[fluorite.cr_ws].fs)
		return; 
	
	no_unmap = True;
	XGrabServer(fluorite.dpy);
	for (Windows *w = fluorite.ws[fluorite.cr_ws].f_wins; w != NULL; w = w->next)
	{
		if (fluorite.ws[fluorite.cr_ws].fl_hdn)
		{
			XMapWindow(fluorite.dpy, w->w);
			XSetInputFocus(fluorite.dpy, w->w, RevertToPointerRoot, CurrentTime);
			FWarpCursor(w->w);
			FApplyActiveWindow(w->w);
			FApplyBorders();
		}
		else
		{
			XUnmapWindow(fluorite.dpy, w->w);
			XSetInputFocus(fluorite.dpy, fluorite.root, RevertToPointerRoot, CurrentTime);
			if (fluorite.ws[fluorite.cr_ws].t_wins)
			{
				Windows *w = fluorite.ws[fluorite.cr_ws].t_wins;
				XSetInputFocus(fluorite.dpy, w->w, RevertToPointerRoot, CurrentTime);
				FWarpCursor(w->w);
				FApplyActiveWindow(w->w);
				FApplyBorders();
			}
			else
				XDeleteProperty(fluorite.dpy, fluorite.root, net_active_window);
		}
	}
	fluorite.ws[fluorite.cr_ws].fl_hdn = !fluorite.ws[fluorite.cr_ws].fl_hdn;
	XSync(fluorite.dpy, True);
	XUngrabServer(fluorite.dpy);
	no_unmap = False;
}

void FSendWindowToNextWorkspace()
{
	int ws = fluorite.cr_ws + 1;
	if (ws == MAX_WS)
		ws = 0;
	FSendWindowToWorkspace(ws);
}

void FSendWindowToPrevWorkspace()
{
	int ws = fluorite.cr_ws - 1;
	if (ws < 0)
		ws = MAX_WS - 1;
	FSendWindowToWorkspace(ws);
}

void FFocusNextMonitor()
{
	int mon = fluorite.cr_mon + 1;
	if (mon == fluorite.ct_mon)
		mon = 0;
	FChangeMonitor(mon);
	XWarpPointer(
		fluorite.dpy, None, fluorite.root,
		0, 0, 0, 0,
		fluorite.mon[mon].mx + fluorite.mon[mon].mw / 2,
		fluorite.mon[mon].my + fluorite.mon[mon].mh / 2
	);
	if (fluorite.ws[fluorite.cr_ws].f_wins)
		XSetInputFocus(fluorite.dpy, fluorite.ws[fluorite.cr_ws].f_wins->w, RevertToPointerRoot, CurrentTime);
	if (fluorite.ws[fluorite.cr_ws].t_wins)
		XSetInputFocus(fluorite.dpy, fluorite.ws[fluorite.cr_ws].t_wins->w, RevertToPointerRoot, CurrentTime);
	FApplyBorders();
}

void FResetMasterOffset()
{
	fluorite.ws[fluorite.cr_ws].mo = fluorite.conf.mo;
	FRedrawWindows();
	FApplyBorders();
}
