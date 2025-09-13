#define FLUORITE_VERSION "Fluorite [EVO 2] (Beta 10)"

#include "config/keybinds.h"
#include "config/design.h"
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <X11/Xresource.h>
#include <X11/Xcursor/Xcursor.h>
#include <X11/extensions/Xrandr.h>
#include <xdo.h>
#include <stdio.h>
#include <sys/types.h>
#include <err.h>
#include <string.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/inotify.h>
#include <errno.h>

/* DEF: constant */
#define MAX_WS 10
#define MODMASK(mask)	(mask & ~(numlockmask | LockMask) & (ShiftMask|ControlMask|Mod1Mask|Mod2Mask|Mod3Mask|Mod4Mask|Mod5Mask))
#define LENGTH(X)		(sizeof X / sizeof X[0])
#define EVENT_SIZE		(sizeof(struct inotify_event))
#define BUF_LEN			(1024 * (EVENT_SIZE + 16))
#define HASH_SIZE		256

static inline int MAX(int a, int b) { return a > b ? a : b; }

enum SIDE
{
	LEFT,
	RIGHT,
	TOP,
	BOTTOM,
	LEFT_SY,
	LEFT_EY,
	RIGHT_SY,
	RIGHT_EY,
	TOP_SX,
	TOP_EX,
	BOTTOM_SX,
	BOTTOM_EX
};

/* DEF: struct */
typedef struct Windows
{
	Window w;
	int wx;
	int wy;
	unsigned int ww;
	unsigned int wh;
	pid_t pid;
	int fc;
	int fs;
	int can_sw;
	Window sw;
	struct Windows *next;
	struct Windows *prev;
} Windows;

typedef struct
{
	int  bf;
	int  bu;
	int  bw;
	int  gp;
	int  so;
	int  mo;
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
	int			mx;
	int 		my;
	int 		mw;
	int 		mh;
	int 		ws;
	int			st;
	int			sb;
	int			sr;
	int			sl;
	int 		primary;
	Windows		*fx_win;
} Monitors;

typedef struct
{
	Windows *t_wins;
	Windows *f_wins;
	Windows *tmp;
	int		layout;
	int		fs;
	int		fl_hdn;
	int		mo;
	int		ct_win;
} Workspaces;

typedef struct Scratchpads
{
	KeySym key;
	Windows *s_wins;
} Scratchpads;

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
	Scratchpads		*pads[HASH_SIZE];
	int				hpads;
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
static void 	*FInotifyXresources(void *ptr);
static void 	FRun();
static void 	FGetMonitorFromMouse();
static int		FCheckWindowToplevel(Window nw);
static void 	FMapRequest(XEvent ev);
static void 	FManageFloatingWindow(Windows *nw);
static void		FRedrawCascadeLayout();
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
static void 	FDestroyNotify(XEvent ev);
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
static void		FResetWindowOpacity(Window w);
static void		FRemoveActiveWindow();
static void		FSearchAndDestoryGhostWindows();
static void		FPolybarLayoutIPC(const int layout);
static void		FPolybarScratchpadsIPC();
static void		FGetFixedPartialStrut(Window w, int new_win);
static void		FRecalculateStrut();
	   void 	FShowWorkspace(int ws);
	   void 	FSendWindowToWorkspace(int ws);
       void 	FExecute(char *argument);
       void 	FQuit();
       void 	FCloseWindow();
       void 	FRotateStackWindows(int mode);
       void 	FRotateWindows(int mode);
	   void		FAddWindowToScratchpad();
	   void		FDelWindowFromScratchpad();
	   void		FScratchpadHideShow();

/* DEF: globals */
static Fluorite fluorite;
static int no_unmap = False;
static int no_warp = False;
static int no_refocus = False;
static unsigned int numlockmask = 0;

int main(void)
{
	pthread_t t_inotify;

	FInit();
	if (XRESOURCES_AUTO_RELOAD)
		pthread_create(&t_inotify, NULL, &FInotifyXresources, NULL);
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
	fluorite.hpads = -1;

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
	if (XrmGetResource(xdb, "fluorite.default_master_offset", "*", &type, &xval))
		if (xval.addr)
			fluorite.conf.mo = strtoul(xval.addr, NULL, 10);
	
	fluorite.conf.bf |= 0xff << 24;
	fluorite.conf.bu |= 0xff << 24;
	XrmDestroyDatabase(xdb);
	XCloseDisplay(dummy_display);
}

static void FLoadTheme()
{
	fluorite.conf.bw = BORDER_WIDTH;
	fluorite.conf.bf = BORDER_FOCUSED;
	fluorite.conf.bu = BORDER_UNFOCUSED;
	fluorite.conf.gp = GAPS;
	fluorite.conf.so = STACK_OFFSET;
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
		fluorite.mon[i].sl = 0;
		fluorite.mon[i].sr = 0;
		fluorite.mon[i].st = 0;
		fluorite.mon[i].sb = 0;
		fluorite.mon[i].ws = default_monitor_workspace[i] - 1;
		fluorite.mon[i].primary = False;
		fluorite.mon[i].fx_win = NULL;
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
		usleep(200000);
		int primary = 0;
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
		if (fluorite.ws[fluorite.cr_mon].t_wins)
			XSetInputFocus(fluorite.dpy, fluorite.ws[fluorite.cr_ws].t_wins->w, RevertToPointerRoot, CurrentTime);
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
	XChangeProperty(fluorite.dpy, fluorite.root, XInternAtom(fluorite.dpy, "_NET_WM_VISIBLE_NAME", False), XInternAtom(fluorite.dpy, "UTF8_STRING", False), 8, PropModeReplace, (const unsigned char *) "Fluorite", strlen("Fluorite"));
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
	XFree(text.value);
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

static void *FInotifyXresources(void *useless)
{
	(void)useless;
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

void FReloadXresources()
{
	Window focused;
	int revert;

	XGetInputFocus(fluorite.dpy, &focused, &revert);
	char prog[255] = "xrdb ~/.Xresources";
	if (system(prog) == -1)
		printf("Error: can't start %s\n", prog);
	FLoadXresources();
	int keep_mon = fluorite.cr_mon;
	for (int i = 0; i < fluorite.ct_mon; i++)
	{
		if (keep_mon == i)
			continue;
		FChangeMonitor(i);
		FRedrawWindows();
		FApplyBorders();
	}
	FChangeMonitor(keep_mon);
	XWarpPointer(
		fluorite.dpy, None, fluorite.root,
		0, 0, 0, 0,
		fluorite.mon[keep_mon].mx + fluorite.mon[keep_mon].mw / 2,
		fluorite.mon[keep_mon].my + fluorite.mon[keep_mon].mh / 2
	);
	if (focused != None)
	{
		for (Windows *w = fluorite.ws[fluorite.cr_ws].t_wins; w != NULL; w = w->next)
			if (focused == w->w)
				w->fc = 1;
		for (Windows *w = fluorite.ws[fluorite.cr_ws].f_wins; w != NULL; w = w->next)
			if (focused == w->w)
				w->fc = 1;
		if (fluorite.hpads != -1)
		{
			Scratchpads *p = fluorite.pads[fluorite.hpads];
			for (Windows *w = p->s_wins; w != NULL; w = w->next)
				if (focused == w->w)
					w->fc = 1;
		}
		XSetInputFocus(fluorite.dpy, focused, RevertToPointerRoot, CurrentTime);
		FWarpCursor(focused);
	}
	FRedrawWindows();
	XSync(fluorite.dpy, True);
	FApplyBorders();
}

static void FRun()
{
	XEvent ev;

	fluorite.run = True;
	while (fluorite.run)
	{
		XNextEvent(fluorite.dpy, &ev);
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
				FGetMonitorFromMouse();
				// FFocusWindowUnderCursor();
				FMotionNotify(ev);
				break;
			case EnterNotify:
				FGetMonitorFromMouse();
				FFocusWindowUnderCursor();
				break;
			case ClientMessage:
				FClientMessage(ev);
				break;
			case DestroyNotify:
				FDestroyNotify(ev);
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
				w->fc = True;
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
				w->fc = True;
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
		if (fluorite.ws[fluorite.cr_ws].fl_hdn)
		{
			for (Windows *w = fluorite.ws[fluorite.cr_ws].f_wins; w != NULL; w = w->next)
				XMapWindow(fluorite.dpy, w->w);
			fluorite.ws[fluorite.cr_ws].fl_hdn = False;
		}
		XSetInputFocus(fluorite.dpy, w->w, RevertToPointerRoot, CurrentTime);
		FApplyBorders();
	}
	return True;
}

static int FCheckCanSwallow(Window w)
{
	XClassHint name;

	if (XGetClassHint(fluorite.dpy, w, &name))
	{
		for (long unsigned int i = 0; i < LENGTH(default_swallowing); i++)
		{
			if (strcmp(default_swallowing[i].wm_class, name.res_class) == 0 ||
					strcmp(default_swallowing[i].wm_class, name.res_name) == 0)
			{
				XFree(name.res_name);
				XFree(name.res_class);
				return True;
			}
		}
		XFree(name.res_name);
		XFree(name.res_class);
	}
	return False;
}

static pid_t FGetParentProcess(pid_t p)
{
	unsigned int v = 0;
	FILE *f;
	char buf[256];
	
	snprintf(buf, sizeof(buf) - 1, "/proc/%u/stat", (unsigned)p);

	if (!(f = fopen(buf, "r")))
		return 0;

	int no_error = fscanf(f, "%*u %*s %*c %u", &v);
	(void)no_error;
	fclose(f);
	return (pid_t)v;
}

static int FCheckDescProcess(pid_t p, pid_t c)
{
	while (p != c && c != 0)
		c = FGetParentProcess(c);
	return (int)c;
}

static int FCheckWindowNeedsSwallowing(Windows *nw)
{
	Windows *w;
	int mon;

	if (!nw->pid || nw->can_sw)
		return False;
	for (mon = 0; mon < fluorite.ct_mon; mon++)
	{
		for (w = fluorite.ws[fluorite.mon[mon].ws].t_wins; w != NULL; w = w->next)
			if (w->can_sw && !w->sw && w->pid && FCheckDescProcess(w->pid, nw->pid))
				goto found;
		for (w = fluorite.ws[fluorite.mon[mon].ws].f_wins; w != NULL; w = w->next)
			if (w->can_sw && !w->sw && w->pid && FCheckDescProcess(w->pid, nw->pid))
				goto found;
	}
	return False;

found:
	no_unmap = True;
	XUnmapWindow(fluorite.dpy, w->w);
	XSync(fluorite.dpy, True);
	no_unmap = False;
	w->sw = w->w;
	w->w = nw->w;
	XMapWindow(fluorite.dpy, w->w);
	if (fluorite.ws[fluorite.mon[mon].ws].layout == DWM || fluorite.ws[fluorite.mon[mon].ws].layout == CENTERED)
		XRaiseWindow(fluorite.dpy, w->w);
	else if (w == fluorite.ws[fluorite.mon[mon].ws].t_wins || w == fluorite.ws[fluorite.mon[mon].ws].t_wins->next)
		XRaiseWindow(fluorite.dpy, w->w);
	else
	{
		XLowerWindow(fluorite.dpy, w->w);
		if (fluorite.cr_ws == fluorite.mon[mon].ws)
			FRedrawCascadeLayout();
	}
	XSetWindowBorderWidth(fluorite.dpy, w->w, fluorite.conf.bw);
	XSetWindowBorder(fluorite.dpy, w->w, fluorite.conf.bu);
	if (w->fc)
		XSetWindowBorder(fluorite.dpy, w->w, fluorite.conf.bf);
	if (fluorite.cr_mon == mon && w->fc)
	{
		XSetInputFocus(fluorite.dpy, w->w, RevertToPointerRoot, CurrentTime);
		FRedrawWindows();
		XSync(fluorite.dpy, True);
		FApplyBorders();
	}
	XMoveResizeWindow(fluorite.dpy, w->w, w->wx, w->wy, w->ww, w->wh);
	if (fluorite.ws[fluorite.mon[mon].ws].fs && w->fs)
	{
		XMoveResizeWindow(fluorite.dpy, w->w, fluorite.mon[mon].mx, fluorite.mon[mon].my, fluorite.mon[mon].mw, fluorite.mon[mon].mh);
		XSetWindowBorderWidth(fluorite.dpy, w->w, 0);
	}
	return True;
}

static void FMapRequest(XEvent ev)
{
	Windows *nw = (Windows *) calloc(1, sizeof(Windows));
	XWindowAttributes wa;
	int is_floating = False;
	int is_fixed = False;

	if (!XGetWindowAttributes(fluorite.dpy, ev.xmaprequest.window, &wa))
		goto freeing;

	if (FCheckWindowToplevel(ev.xmaprequest.window))
	{
		if (JUMP_TO_URGENT)
			return free(nw);
		XWMHints *hints = XGetWMHints(fluorite.dpy, ev.xmaprequest.window);
		hints->flags |= XUrgencyHint;
		XSetWMHints(fluorite.dpy, ev.xmaprequest.window, hints);
		XChangeProperty(fluorite.dpy, fluorite.root, XA_WM_HINTS, XA_WM_HINTS, 32, PropModeAppend, (unsigned char *) NULL, 0);
		XFree(hints);
		goto freeing;
	}

	if (wa.override_redirect || wa.width <= 0 || wa.width <= 0)
	{
		XMapWindow(fluorite.dpy, ev.xmaprequest.window);
		goto freeing;
	}

	is_floating = FCheckWindowIsFloating(ev.xmaprequest.window);
	is_fixed = FCheckWindowIsFixed(ev.xmaprequest.window);

	nw->w = ev.xmaprequest.window;
	nw->pid = xdo_get_pid_window(fluorite.xdo, nw->w);
	nw->fc = True;
	nw->fs = False;
	nw->can_sw = FCheckCanSwallow(nw->w);
	nw->sw = 0;
	XSelectInput(fluorite.dpy, nw->w, EnterWindowMask | FocusChangeMask | PropertyChangeMask | StructureNotifyMask | KeyPressMask);
	XGrabButton(fluorite.dpy, Button1, METAKEY, nw->w, False, ButtonPressMask | ButtonReleaseMask | ButtonMotionMask, GrabModeAsync, GrabModeAsync, None, None);
	XGrabButton(fluorite.dpy, Button3, METAKEY, nw->w, False, ButtonPressMask | ButtonReleaseMask | ButtonMotionMask, GrabModeAsync, GrabModeAsync, None, None);
	XMapWindow(fluorite.dpy, nw->w);
	XRaiseWindow(fluorite.dpy, nw->w);
	XChangeProperty(fluorite.dpy, nw->w, XInternAtom(fluorite.dpy, "_NET_WM_DESKTOP", False), XA_CARDINAL, 32, PropModeReplace, (unsigned char *)&fluorite.cr_ws, 1);

	if (!is_fixed)
		if (FCheckWindowNeedsSwallowing(nw))
			goto freeing;

	if (fluorite.ws[fluorite.cr_ws].fs && !is_floating)
		FFullscreenToggle();

	if (is_fixed)
	{
		FGetFixedPartialStrut(nw->w, True);
		goto freeing;
	}

	for (Windows *w = fluorite.ws[fluorite.cr_ws].t_wins; w != NULL; w = w->next)
	{
		w->fc = False;
		XSetWindowBorder(fluorite.dpy, w->w, fluorite.conf.bu);
	}
	XSetInputFocus(fluorite.dpy, nw->w, RevertToPointerRoot, CurrentTime);
	XSetWindowBorderWidth(fluorite.dpy, nw->w, fluorite.conf.bw);

	if ((is_floating && AUTO_FLOATING) || OPEN_IN_FLOAT)
		FManageFloatingWindow(nw);
	else
		fluorite.ws[fluorite.cr_ws].t_wins = FAddWindow(fluorite.ws[fluorite.cr_ws].t_wins, nw);;

	FRedrawWindows();
	FWarpCursor(nw->w);
	XSync(fluorite.dpy, True);
	FApplyBorders();
	FUpdateClientList();
	if ((fluorite.ws[fluorite.cr_ws].fs && is_floating) || (fluorite.hpads != 1 && is_floating))
	{
		XRaiseWindow(fluorite.dpy, nw->w);
		XSetWindowBorder(fluorite.dpy, nw->w, fluorite.conf.bf);
		XSetInputFocus(fluorite.dpy, nw->w, RevertToPointerRoot, CurrentTime);
		FWarpCursor(nw->w);
	}
	return;

freeing:
	free(nw);
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
	nw->fs = False;
	if (fluorite.ws[fluorite.cr_ws].fl_hdn)
	{
		for (Windows *w = fluorite.ws[fluorite.cr_ws].f_wins; w != NULL; w = w->next)
			XMapWindow(fluorite.dpy, w->w);
		fluorite.ws[fluorite.cr_ws].fl_hdn = False;
	}
	XMoveResizeWindow(fluorite.dpy, nw->w, nw->wx, nw->wy, nw->ww, nw->wh);
	fluorite.ws[fluorite.cr_ws].f_wins = FAddWindow(cw, nw);
}

static void FRedrawCenteredMaster()
{
	Windows *w;
	int n = 0;
	for (w = fluorite.ws[fluorite.cr_ws].t_wins; w; w = w->next, n++);
	int mo = fluorite.ws[fluorite.cr_ws].mo;
	int gap = fluorite.conf.gp;
	int bw = fluorite.conf.bw;
	int mon_x = fluorite.mon[fluorite.cr_mon].mx;
	int mon_y = fluorite.mon[fluorite.cr_mon].my;
	int mon_w = fluorite.mon[fluorite.cr_mon].mw;
	int mon_h = fluorite.mon[fluorite.cr_mon].mh;
	
	// Calcul de la zone de contenu en tenant compte de tous les struts
	int content_x = mon_x + fluorite.mon[fluorite.cr_mon].sl;
	int content_y = mon_y + 2 * gap + fluorite.mon[fluorite.cr_mon].st;
	int content_w = mon_w - (fluorite.mon[fluorite.cr_mon].sl + fluorite.mon[fluorite.cr_mon].sr);
	int content_h = mon_h - 4 * gap - (fluorite.mon[fluorite.cr_mon].st + fluorite.mon[fluorite.cr_mon].sb);
	
	int base_mw = content_w / 2;
	int mw = base_mw + mo;
	if (mw > content_w - 2 * gap)
		mw = content_w - 2 * gap;
	int mx = content_x + (content_w - mw) / 2;
	int sw = (content_w - mw) / 2;
	
	w = fluorite.ws[fluorite.cr_ws].t_wins;
	XMoveResizeWindow(
		fluorite.dpy, w->w,
		mx + gap * 2,
		content_y,
		mw - 2 * bw - 4 * gap,
		content_h - 2 * bw
	);
	w->wx = mx + gap;
	w->wy = content_y;
	w->ww = mw - 2 * bw - 2 * gap;
	w->wh = content_h - 2 * bw;
	
	int stack_n = n - 1;
	int left_n = (stack_n + 1) / 2;
	int right_n = stack_n / 2;
	int total_gap_left = (left_n - 1) * gap;
	int total_gap_right = (right_n - 1) * gap;
	int left_h = (content_h - total_gap_left) / (left_n > 0 ? left_n : 1);
	int right_h = (content_h - total_gap_right) / (right_n > 0 ? right_n : 1);
	int left_y = content_y;
	int right_y = content_y;
	
	w = w->next;
	int i = 0;
	Windows *last_left = NULL, *last_right = NULL;
	while (w)
	{
		if (i % 2 == 0)
		{
			XMoveResizeWindow(
				fluorite.dpy, w->w,
				content_x + gap * 2,
				left_y,
				sw - 2 * bw - 2 * gap,
				left_h - 2 * bw
			);
			w->wx = content_x + gap;
			w->wy = left_y;
			w->ww = sw - 2 * bw - 2 * gap;
			w->wh = left_h - 2 * bw;
			last_left = w;
			left_y += left_h + gap;
		}
		else
		{
			XMoveResizeWindow(
				fluorite.dpy, w->w,
				mx + mw - (gap / 4),
				right_y,
				sw - 2 * bw - 2 * gap,
				right_h - 2 * bw
			);
			w->wx = mx + mw + gap;
			w->wy = right_y;
			w->ww = sw - 2 * bw - 2 * gap;
			w->wh = right_h - 2 * bw;
			last_right = w;
			right_y += right_h + gap;
		}
		i++;
		w = w->next;
	}
	
	if (stack_n % 2 == 0)
	{
		if (right_n < left_n && last_right)
		{
			int extra = (content_y + content_h) - (last_right->wy + last_right->wh + 2 * bw);
			last_right->wh += extra;
			XResizeWindow(fluorite.dpy, last_right->w, last_right->ww, last_right->wh);
		}
		else if (left_n < right_n && last_left)
		{
			int extra = (content_y + content_h) - (last_left->wy + last_left->wh + 2 * bw);
			last_left->wh += extra;
			XResizeWindow(fluorite.dpy, last_left->w, last_left->ww, last_left->wh);
		}
	}
}

static void FRedrawStackedLayout()
{
	Windows *raise;

	for (Windows *w = fluorite.ws[fluorite.cr_ws].t_wins; w != NULL; w = w->next)
	{
		w->wx = fluorite.mon[fluorite.cr_mon].mx + (fluorite.conf.gp * 2);
		w->wy = fluorite.mon[fluorite.cr_mon].my + (fluorite.conf.gp * 2);
		w->ww = fluorite.mon[fluorite.cr_mon].mw - (fluorite.conf.bw * 2) - (fluorite.conf.gp * 4);
		w->wh = fluorite.mon[fluorite.cr_mon].mh - (fluorite.conf.bw * 2) - (fluorite.conf.gp * 4);
		w->wy += fluorite.mon[fluorite.cr_mon].st;
		w->wh -= fluorite.mon[fluorite.cr_mon].st + fluorite.mon[fluorite.cr_mon].sb;
		w->wx += fluorite.mon[fluorite.cr_mon].sl;
		w->ww -= (fluorite.mon[fluorite.cr_mon].sl + fluorite.mon[fluorite.cr_mon].sr);
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

	int mon = fluorite.cr_mon;
	int mo = fluorite.ws[fluorite.cr_ws].mo;
	int mx = fluorite.mon[mon].mx;
	int my = fluorite.mon[mon].my;
	int mw = fluorite.mon[mon].mw;
	int mh = fluorite.mon[mon].mh;
	int gap = fluorite.conf.gp * 2;
	int border = fluorite.conf.bw;
	int usable_y = my + fluorite.mon[fluorite.cr_mon].st;
	int usable_height = mh - fluorite.mon[fluorite.cr_mon].st - fluorite.mon[fluorite.cr_mon].sb;

	int n = 0;
	for (Windows *w = win; w; w = w->next)
		n++;

	if (n == 0)
		return;

	int master_width = (n > 1) ? (mw * 0.5) : mw;
	master_width += mo;
	master_width += fluorite.conf.gp;
	int stack_width = mw - master_width;
	win->wx = mx + gap;
	win->wy = usable_y + gap;
	win->ww = master_width - 2 * gap - 2 * border;
	win->wh = usable_height - 2 * gap - 2 * border;
	win->wx += fluorite.mon[fluorite.cr_mon].sl;
	win->ww -= (fluorite.mon[fluorite.cr_mon].sl + fluorite.mon[fluorite.cr_mon].sr) / 2;
	XMoveResizeWindow(fluorite.dpy, win->w, win->wx, win->wy, win->ww, win->wh);
	win = win->next;

	if (n > 1)
	{
		int sn = n - 1;
		int total_gap = gap * (sn - 1);
		int total_window_height = usable_height - 2 * gap - total_gap;
		int sh_each = total_window_height / sn;
		for (int i = 0; win; win = win->next, i++)
		{
			win->wx = mx + master_width;
			win->ww = stack_width - 2 * gap - 2 * border + gap;
			win->wh = sh_each - 2 * border;
			win->wy = usable_y + gap + i * (sh_each + gap);
			win->wx += (fluorite.mon[fluorite.cr_mon].sl / 2);
			win->wx -= (fluorite.mon[fluorite.cr_mon].sr / 2);
			win->ww -= (fluorite.mon[fluorite.cr_mon].sl + fluorite.mon[fluorite.cr_mon].sr) / 2;
			XMoveResizeWindow(fluorite.dpy, win->w, win->wx, win->wy, win->ww, win->wh);
		}
	}
}

static void FRedrawCascadeLayout()
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
		last->wy += fluorite.mon[fluorite.cr_mon].st;
		last->wh -= (fluorite.mon[fluorite.cr_mon].st + fluorite.mon[fluorite.cr_mon].sb);
		last->wx += (fluorite.mon[fluorite.cr_mon].sl / 2);
		last->wx -= (fluorite.mon[fluorite.cr_mon].sr / 2);
		last->ww -= (fluorite.mon[fluorite.cr_mon].sl + fluorite.mon[fluorite.cr_mon].sr) / 2;
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
	fluorite.ws[fluorite.cr_ws].t_wins->wy += fluorite.mon[fluorite.cr_mon].st;
	fluorite.ws[fluorite.cr_ws].t_wins->wh -= (fluorite.mon[fluorite.cr_mon].st + fluorite.mon[fluorite.cr_mon].sb);
	fluorite.ws[fluorite.cr_ws].t_wins->wx += fluorite.mon[fluorite.cr_mon].sl;
	fluorite.ws[fluorite.cr_ws].t_wins->ww -= (fluorite.mon[fluorite.cr_mon].sl + fluorite.mon[fluorite.cr_mon].sr) / 2;
	XRaiseWindow(fluorite.dpy, fluorite.ws[fluorite.cr_ws].t_wins->w);
	XMoveResizeWindow(fluorite.dpy, fluorite.ws[fluorite.cr_ws].t_wins->w,
		fluorite.ws[fluorite.cr_ws].t_wins->wx, fluorite.ws[fluorite.cr_ws].t_wins->wy,
		fluorite.ws[fluorite.cr_ws].t_wins->ww, fluorite.ws[fluorite.cr_ws].t_wins->wh
	);
}



static void FRedrawFullscreen()
{
	Windows *w;
	for (w = fluorite.ws[fluorite.cr_ws].f_wins; w != NULL; w = w->next)
		if (w->fs)
			goto found;
	for (w = fluorite.ws[fluorite.cr_ws].t_wins; w != NULL; w = w->next)
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
	FFocusWindowUnderCursor();
}

static void FMoveWindowBasedOnMonitor(Windows *w)
{
	int from = -1;
	for (int i = 0; i < fluorite.ct_mon; ++i)
	{
		if (w->wx >= fluorite.mon[i].mx && w->wx < fluorite.mon[i].mx + fluorite.mon[i].mw &&
				w->wy >= fluorite.mon[i].my && w->wy < fluorite.mon[i].my + fluorite.mon[i].mh)
		{
			from = i;
			break;
		}
	}

	if (from != -1 && from != fluorite.cr_mon)
	{
		float rel_x = (float)(w->wx - fluorite.mon[from].mx) / fluorite.mon[from].mw;
		float rel_y = (float)(w->wy - fluorite.mon[from].my) / fluorite.mon[from].mh;
		w->wx = fluorite.mon[fluorite.cr_mon].mx + (int)(rel_x * fluorite.mon[fluorite.cr_mon].mw);
		w->wy = fluorite.mon[fluorite.cr_mon].my + (int)(rel_y * fluorite.mon[fluorite.cr_mon].mh);
	}

}

static void FRedrawWindows()
{
	FSearchAndDestoryGhostWindows();
	FPolybarLayoutIPC(fluorite.ws[fluorite.cr_ws].layout + 1);

	if (!fluorite.ws[fluorite.cr_ws].t_wins)
		goto floating;

	no_refocus = True;
	
	if ((fluorite.ws[fluorite.cr_ws].t_wins && !fluorite.ws[fluorite.cr_ws].t_wins->next) || fluorite.ws[fluorite.cr_ws].layout == STACKED)
		FRedrawStackedLayout();
	else if (fluorite.ws[fluorite.cr_ws].layout == DWM)
		FRedrawDWMLayout();
	else if (fluorite.ws[fluorite.cr_ws].layout == CENTERED && fluorite.ws[fluorite.cr_ws].t_wins->next->next)
		FRedrawCenteredMaster();
	else
		FRedrawCascadeLayout();

floating:
	for (Windows *w = fluorite.ws[fluorite.cr_ws].f_wins; w != NULL; w = w->next)
	{
		FMoveWindowBasedOnMonitor(w);
		XRaiseWindow(fluorite.dpy, w->w);
		XMoveResizeWindow(fluorite.dpy, w->w, w->wx, w->wy, w->ww, w->wh);
	}

	if (fluorite.hpads == -1)
		goto fullscreen;

	Scratchpads *p = fluorite.pads[fluorite.hpads];
	if (!p->s_wins)
		goto fullscreen;
	Windows *w;
	for (w = p->s_wins; w->next != NULL; w = w->next);
	for (; w != NULL; w = w->prev)
	{
		FMoveWindowBasedOnMonitor(w);
		XMoveResizeWindow(fluorite.dpy, w->w, w->wx, w->wy, w->ww, w->wh);
		XRaiseWindow(fluorite.dpy, w->w);
	}

fullscreen:
	if (fluorite.ws[fluorite.cr_ws].fs)
		FRedrawFullscreen();

	no_refocus = False;
}

static void FApplyActiveWindow(Window w)
{
	Atom net_active_window = XInternAtom(fluorite.dpy, "_NET_ACTIVE_WINDOW", False);
	XChangeProperty(fluorite.dpy, fluorite.root,
			net_active_window, XA_WINDOW, 32,
			PropModeReplace, (unsigned char *)&w, 1);
	XSetWindowBorder(fluorite.dpy, w, fluorite.conf.bf);
	FWarpCursor(w);
}

static void FApplyBorders()
{
	Window focused;
	int revert;

	XGetInputFocus(fluorite.dpy, &focused, &revert);
	if (focused == fluorite.root || fluorite.ws[fluorite.cr_ws].fs)
		return ;

	if (fluorite.hpads == -1)
		goto next;

	Scratchpads *p = fluorite.pads[fluorite.hpads];
	for (Windows *w = p->s_wins; w; w = w->next)
	{
		XSetWindowBorderWidth(fluorite.dpy, w->w, fluorite.conf.bw);
		XSetWindowBorder(fluorite.dpy, w->w, fluorite.conf.bu);
		if (focused == w->w)
			FApplyActiveWindow(focused);
	}

next:
	for (Windows *w = fluorite.ws[fluorite.cr_ws].f_wins; w; w = w->next)
	{
		XSetWindowBorderWidth(fluorite.dpy, w->w, fluorite.conf.bw);
		XSetWindowBorder(fluorite.dpy, w->w, fluorite.conf.bu);
		if (focused == w->w)
			FApplyActiveWindow(focused);
	}

	for (Windows *w = fluorite.ws[fluorite.cr_ws].t_wins; w; w = w->next)
	{
		XSetWindowBorderWidth(fluorite.dpy, w->w, fluorite.conf.bw);
		XSetWindowBorder(fluorite.dpy, w->w, fluorite.conf.bu);
		if (focused == w->w)
			FApplyActiveWindow(focused);
	}

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
			{
				XFree(name.res_name);
				XFree(name.res_class);
				return True;
			}
		}
		XFree(name.res_name);
		XFree(name.res_class);
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
			{
				XFree(name.res_name);
				XFree(name.res_class);
				return True;
			}
		}
		XFree(name.res_name);
		XFree(name.res_class);
	}
	if (XGetWindowProperty(fluorite.dpy, w, XInternAtom(fluorite.dpy, "_NET_WM_WINDOW_TYPE", False), 0L, sizeof(atom), False, XA_ATOM, &da, &di, &dl, &dl, &p) == Success && p)
	{
		atom = *(Atom *)p;
		if (atom == XInternAtom(fluorite.dpy, "_NET_WM_WINDOW_TYPE_DOCK", False))
		{
			XFree(p);
			return True;
		}
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
	no_warp = True;

	for (Windows *w = fluorite.ws[fluorite.cr_ws].t_wins; w != NULL; w = w->next)
		XSetWindowBorder(fluorite.dpy, w->w, fluorite.conf.bu);
	for (Windows *w = fluorite.ws[fluorite.cr_ws].f_wins; w != NULL; w = w->next)
		XSetWindowBorder(fluorite.dpy, w->w, fluorite.conf.bu);
	FRemoveActiveWindow();
	XSetInputFocus(fluorite.dpy, fluorite.root, RevertToPointerRoot, CurrentTime);
	fluorite.cr_mon = mon;
	fluorite.cr_ws = fluorite.mon[mon].ws;
	XChangeProperty(fluorite.dpy, fluorite.root, XInternAtom(fluorite.dpy, "_NET_CURRENT_DESKTOP", False), XA_CARDINAL, 32, PropModeReplace, (unsigned char *)&fluorite.cr_ws, 1);

	FRedrawWindows();
	XSync(fluorite.dpy, True);
	FApplyBorders();
	no_warp = False;
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
		w->fc = False;
}

static Window FFindFocusedWindow()
{
	for (Windows *w = fluorite.ws[fluorite.cr_ws].t_wins; w != NULL; w = w->next)
		if (w->fc)
			return w->w;
	for (Windows *w = fluorite.ws[fluorite.cr_ws].f_wins; w != NULL; w = w->next)
		if (w->fc)
			return w->w;
	return -1;
}

static int FWindowExists(Display *dpy, Window win)
{
    XWindowAttributes attr;
    return XGetWindowAttributes(dpy, win, &attr) != 0;
}

static void FUnmapNotify(XEvent ev)
{
	int ws;

	if (no_unmap)
		return;

	if (fluorite.hpads == -1)
		goto next;

	Scratchpads *p = fluorite.pads[fluorite.hpads];
	for (Windows *w = p->s_wins; w != NULL; w = w->next)
	{
		if (w->w != ev.xunmap.window)
			continue;
		XSetInputFocus(fluorite.dpy, w->w, RevertToPointerRoot, CurrentTime);
		p->s_wins = FDelWindow(p->s_wins, w);
		w->next = NULL;
		w->prev = NULL;
		fluorite.ws[fluorite.cr_ws].t_wins = FAddWindow(fluorite.ws[fluorite.cr_ws].t_wins, w);
		if (!p->s_wins)
			fluorite.hpads = -1;
		break;
	}

next:
	ws = FFindWorkspaceFromWindow(ev.xunmap.window);
	if (ws == -1)
		return;

	for (Windows *w = fluorite.ws[ws].t_wins; w != NULL; w = w->next)
	{
		if (w->w == ev.xunmap.window)
		{
			if (w->sw && FWindowExists(fluorite.dpy, w->sw))
			{
				w->w = w->sw;
				XMapWindow(fluorite.dpy, w->w);
				FResetWindowOpacity(w->w);
				if (fluorite.ws[ws].fs)
				{
					XSetWindowBorderWidth(fluorite.dpy, w->w, 0);
					FSetWindowOpacity(w->w, 100);
				}
				w->sw = 0;
				goto redraw;
			}
force_unmap:
			if (fluorite.ws[ws].fs && w->fs)
				fluorite.ws[ws].fs = False;
			FResetFocus(fluorite.ws[ws].t_wins);
			fluorite.ws[ws].t_wins = FDelWindow(fluorite.ws[ws].t_wins, w);
			free(w);
			goto redraw;
		}
	}
	for (Windows *w = fluorite.ws[ws].f_wins; w; w = w->next)
	{
		if (w->sw == ev.xunmap.window) goto force_unmap;
		if (w->w == ev.xunmap.window)
		{
			if (w->sw && FWindowExists(fluorite.dpy, w->sw))
			{
				w->w = w->sw;
				XMapWindow(fluorite.dpy, w->w);
				FResetWindowOpacity(w->w);
				if (fluorite.ws[ws].fs)
				{
					XSetWindowBorderWidth(fluorite.dpy, w->w, 0);
					FSetWindowOpacity(w->w, 100);
				}
				w->sw = 0;
				goto redraw;
			}
			if (fluorite.ws[ws].fs && w->fs)
				fluorite.ws[ws].fs = False;
			FResetFocus(fluorite.ws[ws].f_wins);
			fluorite.ws[ws].f_wins = FDelWindow(fluorite.ws[ws].f_wins, w);
			free(w);
			goto redraw;
		}
	}

redraw:
	FUpdateClientList();
	FRemoveActiveWindow();
	FPolybarScratchpadsIPC();

	int keep_ws = fluorite.cr_ws;
	int keep_mon = fluorite.cr_mon;
	for (int i = 0; i < fluorite.ct_mon; i++)
	{
		fluorite.cr_mon = i;
		fluorite.cr_ws = fluorite.mon[i].ws;
		FRedrawWindows();
	}
	fluorite.cr_ws = keep_ws;
	fluorite.cr_mon = keep_mon;
	FRedrawWindows();
	Window w = FFindFocusedWindow();
	if (w != (long unsigned int)-1)
	{
		no_refocus = True;
		XSetInputFocus(fluorite.dpy, w, RevertToPointerRoot, CurrentTime);
		FApplyBorders();
		FWarpCursor(w);
		XSync(fluorite.dpy, True);
		no_refocus = False;
	}
	else
	{
		if (fluorite.ws[fluorite.cr_ws].t_wins)
		{
			no_refocus = True;
			fluorite.ws[fluorite.cr_ws].t_wins->fc = 1;
			XSetInputFocus(fluorite.dpy, fluorite.ws[fluorite.cr_ws].t_wins->w, RevertToPointerRoot, CurrentTime);
			FApplyBorders();
			FWarpCursor(fluorite.ws[fluorite.cr_ws].t_wins->w);
			XSync(fluorite.dpy, True);
			no_refocus = False;
		}
		else if (fluorite.ws[fluorite.cr_ws].f_wins)
		{
			no_refocus = True;
			fluorite.ws[fluorite.cr_ws].f_wins->fc = 1;
			XSetInputFocus(fluorite.dpy, fluorite.ws[fluorite.cr_ws].f_wins->w, RevertToPointerRoot, CurrentTime);
			FApplyBorders();
			FWarpCursor(fluorite.ws[fluorite.cr_ws].f_wins->w);
			XSync(fluorite.dpy, True);
			no_refocus = False;
		}
	}
}

static void FDestroyNotify(XEvent ev)
{
	int ws;
	int i = 0;

	for (int i = 0; i <= fluorite.ct_mon; i++)
	{
		for (Windows *fx = fluorite.mon[i].fx_win; fx != NULL; fx = fx->next)
		{
			if (fx->w != ev.xdestroywindow.window)
				continue;
			fluorite.mon[i].fx_win = FDelWindow(fluorite.mon[i].fx_win, fx);
			fx->next = NULL;
			fx->prev = NULL;
			fluorite.mon[i].sl = 0;
			fluorite.mon[i].sr = 0;
			fluorite.mon[i].st = 0;
			fluorite.mon[i].sb = 0;
			FRecalculateStrut();
			goto update;
		}
	}

	for (Scratchpads *p = fluorite.pads[i]; i < HASH_SIZE; i++)
	{
		if (!p)
			continue;
		for (Windows *w = p->s_wins; w != NULL; w = w->next)
		{
			if (w->w != ev.xunmap.window)
				continue;
			XSetInputFocus(fluorite.dpy, w->w, RevertToPointerRoot, CurrentTime);
			p->s_wins = FDelWindow(p->s_wins, w);
			w->next = NULL;
			w->prev = NULL;
			fluorite.ws[fluorite.cr_ws].t_wins = FAddWindow(fluorite.ws[fluorite.cr_ws].t_wins, w);
			goto next;
		}
	}

next:
	ws = FFindWorkspaceFromWindow(ev.xdestroywindow.window);
	if (ws == -1)
		return;

	for (Windows *w = fluorite.ws[ws].t_wins; w != NULL; w = w->next)
	{
		if (w->sw == ev.xdestroywindow.window) goto force_destroy;
		if (w->w == ev.xdestroywindow.window)
		{
			if (w->sw && FWindowExists(fluorite.dpy, w->sw))
			{
				w->w = w->sw;
				FResetWindowOpacity(w->w);
				if (fluorite.ws[ws].fs)
				{
					XSetWindowBorderWidth(fluorite.dpy, w->w, 0);
					FSetWindowOpacity(w->w, 100);
				}
				w->sw = 0;
				goto update;
			}
force_destroy:
			if (fluorite.ws[ws].fs && w->fs)
				fluorite.ws[ws].fs = False;
			FResetFocus(fluorite.ws[ws].t_wins);
			fluorite.ws[ws].t_wins = FDelWindow(fluorite.ws[ws].t_wins, w);
			free(w);
			goto update;
		}
	}
	for (Windows *w = fluorite.ws[ws].f_wins; w; w = w->next)
	{
		if (w->sw == ev.xdestroywindow.window) goto force_destroy;
		if (w->w == ev.xdestroywindow.window)
		{
			if (w->sw && FWindowExists(fluorite.dpy, w->sw))
			{
				w->w = w->sw;
				FResetWindowOpacity(w->w);
				if (fluorite.ws[ws].fs)
				{
					XSetWindowBorderWidth(fluorite.dpy, w->w, 0);
					FSetWindowOpacity(w->w, 100);
				}
				w->sw = 0;
				goto update;
			}
			if (fluorite.ws[ws].fs && w->fs)
				fluorite.ws[ws].fs = False;
			FResetFocus(fluorite.ws[ws].f_wins);
			fluorite.ws[ws].f_wins = FDelWindow(fluorite.ws[ws].f_wins, w);
			free(w);
			goto update;
		}
	}

update:
	FUpdateClientList();
	FRedrawWindows();
	FPolybarScratchpadsIPC();

	int keep_ws = fluorite.cr_ws;
	int keep_mon = fluorite.cr_mon;
	for (int i = 0; i < fluorite.ct_mon; i++)
	{
		fluorite.cr_mon = i;
		fluorite.cr_ws = fluorite.mon[i].ws;
		FRedrawWindows();
	}
	fluorite.cr_ws = keep_ws;
	fluorite.cr_mon = keep_mon;

	Window w = FFindFocusedWindow();
	if (w != (long unsigned int)-1)
	{
		XSetInputFocus(fluorite.dpy, w, RevertToPointerRoot, CurrentTime);
		FApplyBorders();
		FWarpCursor(w);
	}
}

static Windows *FAddWindow(Windows *cw, Windows *w)
{
	for (Windows *i = cw; i != NULL; i = i->next)
		i->fc = False;

	if (cw)
	{
		cw->fc = False;
		cw->fs = False;
		cw->prev = w;
		w->next = cw;
		w->prev = NULL;
	}
	return w;
}

static Windows *FDelWindow(Windows *cw, Windows *w)
{
	if (!w->next && w == cw)
	{
		cw = NULL;
		XSetInputFocus(fluorite.dpy, fluorite.root, RevertToPointerRoot, CurrentTime);
	}
	else if (w->next && w == cw)
	{
		w->next->prev = NULL;
		cw = w->next;
		cw->fc = True;
		XSetInputFocus(fluorite.dpy, cw->w, RevertToPointerRoot, CurrentTime);
	}
	else
	{
		if (w->prev)
		{
			w->prev->next = w->next;
			w->prev->fc = True;
			XSetInputFocus(fluorite.dpy, w->prev->w, RevertToPointerRoot, CurrentTime);
		}
		if (w->next)
		{
			w->next->prev = w->prev;
			w->next->fc = True;
			w->prev->fc = False;
			XSetInputFocus(fluorite.dpy, w->next->w, RevertToPointerRoot, CurrentTime);
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

	no_warp = True;
	
	if (fluorite.hpads == -1)
		goto next;

	Scratchpads *p = fluorite.pads[fluorite.hpads];
	for (Windows *w = p->s_wins; w != NULL; w = w->next)
	{
		if (w->w != target)
			continue;
		FResetFocus(p->s_wins);
		XSetInputFocus(fluorite.dpy, (w->sw && w->sw == target) ? w->sw : w->w, RevertToPointerRoot, CurrentTime);
		w->fc = True;
		goto end;
	}

next:
	for (Windows *w = fluorite.ws[fluorite.cr_ws].t_wins; w != NULL; w = w->next)
	{
		if (w->w != target)
			continue;
		FResetFocus(fluorite.ws[fluorite.cr_ws].t_wins);
		XSetInputFocus(fluorite.dpy, (w->sw && w->sw == target) ? w->sw : w->w, RevertToPointerRoot, CurrentTime);
		w->fc = True;
		goto end;
	}
	for (Windows *w = fluorite.ws[fluorite.cr_ws].f_wins; w != NULL; w = w->next)
	{
		if (w->w != target)
			continue;
		FResetFocus(fluorite.ws[fluorite.cr_ws].f_wins);
		XSetInputFocus(fluorite.dpy, (w->sw && w->sw == target) ? w->sw : w->w, RevertToPointerRoot, CurrentTime);
		w->fc = True;
		goto end;
	}

end:
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

	if (fluorite.hpads == -1)
		goto next;

	Scratchpads *p = fluorite.pads[fluorite.hpads];
	for (Windows *w = p->s_wins; w != NULL; w = w->next)
	{
		if (target != w->w)
			continue;
		p->s_wins = FDelWindow(p->s_wins, w);
		p->s_wins = FAddWindow(p->s_wins, w);
		XRaiseWindow(fluorite.dpy, w->w);
		XSetInputFocus(fluorite.dpy, w->w, RevertToPointerRoot, CurrentTime);
		no_warp = True;
		FApplyBorders();
		no_warp = False;
		return;
	}

next:
	for (Windows *w = fluorite.ws[fluorite.cr_ws].f_wins; w != NULL; w = w->next)
	{
		if (target != w->w)
			continue;
		fluorite.ws[fluorite.cr_ws].f_wins = FDelWindow(fluorite.ws[fluorite.cr_ws].f_wins, w);
		fluorite.ws[fluorite.cr_ws].f_wins = FAddWindow(fluorite.ws[fluorite.cr_ws].f_wins, w);
		XRaiseWindow(fluorite.dpy, w->w);
		XSetInputFocus(fluorite.dpy, w->w, RevertToPointerRoot, CurrentTime);
		no_warp = True;
		FApplyBorders();
		no_warp = False;
		return;
	}
	for (Windows *w = fluorite.ws[fluorite.cr_ws].t_wins; w != NULL; w = w->next)
	{
		if (target != w->w)
			continue;

		fluorite.ws[fluorite.cr_ws].t_wins = FDelWindow(fluorite.ws[fluorite.cr_ws].t_wins, w);
		w->next = NULL;
		w->prev = NULL;
		fluorite.ws[fluorite.cr_ws].f_wins = FAddWindow(fluorite.ws[fluorite.cr_ws].f_wins, w);
		xdo_get_window_size(fluorite.xdo, w->w, &xdo_w, &xdo_h);
		xdo_get_window_location(fluorite.xdo, w->w, &w->wx, &w->wy, &scr);
		w->ww = xdo_w;
		w->wh = xdo_h;
		XResizeWindow(fluorite.dpy, w->w, w->ww, w->wh);
		XMoveWindow(fluorite.dpy, w->w, w->wx, w->wy);
		no_warp = True;
		no_refocus = True;
		if (fluorite.ws[fluorite.cr_ws].fl_hdn)
		{
			for (Windows *w = fluorite.ws[fluorite.cr_ws].f_wins; w != NULL; w = w->next)
				XMapWindow(fluorite.dpy, w->w);
			fluorite.ws[fluorite.cr_ws].fl_hdn = False;
		}
		FRedrawWindows();
		FApplyBorders();
		no_warp = False;
		no_refocus = False;
		return;
	}
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
	XSizeHints hints;
	Windows *target;

	if (fluorite.ws[fluorite.cr_ws].fs)
		return ;

	dpx = ev.xmotion.x_root - fluorite.mouse.spx;
	dpy = ev.xmotion.y_root - fluorite.mouse.spy;

	if (fluorite.hpads == -1)
		goto next;

	Scratchpads *p = fluorite.pads[fluorite.hpads];
	for (target = p->s_wins; target != NULL; target = target->next)
		if (ev.xmotion.window == target->w)
			goto found;
next:
	for (target = fluorite.ws[fluorite.cr_ws].f_wins; target != NULL; target = target->next)
		if (ev.xmotion.window == target->w)
			goto found;

	if (!target)
		return ;

found:
	no_warp = True;
	if (ev.xmotion.state & Button1Mask)
	{
		ddx = fluorite.mouse.swx + dpx;
		ddy = fluorite.mouse.swy + dpy;
		XMoveWindow(fluorite.dpy, target->w, ddx, ddy);
		target->wx = ddx;
		target->wy = ddy;
		hints.x = ddx;
		hints.y = ddy;
		goto pressed;
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
		goto pressed;
	}
	goto exit;

pressed:
	XRaiseWindow(fluorite.dpy, target->w);
	FResetFocus(fluorite.ws[fluorite.cr_ws].f_wins);
	target->fc = True;
	XSetInputFocus(fluorite.dpy, target->w, RevertToPointerRoot, CurrentTime);
	XSetWMNormalHints(fluorite.dpy, target->w, &hints);
	FApplyBorders();

exit:
	no_warp = False;
}

static void FWarpCursor(Window w)
{
	int x, y, ww, wh;
	Window dummy;

	if (!WARP_CURSOR || no_warp)
		return;

	no_refocus = True;
	XGetGeometry(fluorite.dpy, w, &dummy, &x, &y, (unsigned int *)&ww, (unsigned int *)&wh, (unsigned int *)&x, (unsigned int *)&y);
	XWarpPointer(fluorite.dpy, None, w, 0, 0, 0, 0, ww / 2, wh / 2);
	XSync(fluorite.dpy, True);
	no_refocus = False;
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
	int magic_number = 1145;
	Window list[magic_number];
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

static void FRemoveActiveWindow()
{
	Atom atom = XInternAtom(fluorite.dpy, "_NET_ACTIVE_WINDOW", False);
	XDeleteProperty(fluorite.dpy, fluorite.root, atom);
}

static void FSwapMonitorWorkspace(int ws, int mon)
{
	int swap_mon = fluorite.cr_mon;

	for (Windows *w = fluorite.ws[fluorite.cr_ws].t_wins; w != NULL; w = w->next)
	{
		w->fc = False;
		XSetWindowBorder(fluorite.dpy, w->w, fluorite.conf.bu);
	}
	for (Windows *w = fluorite.ws[fluorite.cr_ws].f_wins; w != NULL; w = w->next)
	{
		w->fc = False;
		XSetWindowBorder(fluorite.dpy, w->w, fluorite.conf.bu);
	}

	FRemoveActiveWindow();
	fluorite.mon[mon].ws = fluorite.cr_ws;
	fluorite.cr_mon = mon;
	FRedrawWindows();
	fluorite.cr_mon = swap_mon;
	fluorite.cr_ws = ws;
	fluorite.mon[fluorite.cr_mon].ws = ws;
	XChangeProperty(fluorite.dpy, fluorite.root, XInternAtom(fluorite.dpy, "_NET_CURRENT_DESKTOP", False), XA_CARDINAL, 32, PropModeReplace, (unsigned char *)&fluorite.cr_ws, 1);
	XSetInputFocus(fluorite.dpy, fluorite.root, RevertToPointerRoot, CurrentTime);
	if (fluorite.ws[fluorite.cr_ws].t_wins && !fluorite.ws[fluorite.cr_ws].fs)
	{
		XSetInputFocus(fluorite.dpy, fluorite.ws[fluorite.cr_ws].t_wins->w, RevertToPointerRoot, CurrentTime);
		XSetWindowBorder(fluorite.dpy, fluorite.ws[fluorite.cr_ws].t_wins->w, fluorite.conf.bf);
	}
	FRedrawWindows();
	FApplyBorders();
}

void FShowWorkspace(int ws)
{
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

	for (Windows *w = fluorite.ws[fluorite.cr_ws].t_wins; w != NULL; w = w->next)
		XUnmapWindow(fluorite.dpy, w->w);
	for (Windows *w = fluorite.ws[fluorite.cr_ws].f_wins; w != NULL; w = w->next)
		XUnmapWindow(fluorite.dpy, w->w);

	fluorite.cr_ws = ws;
	fluorite.mon[fluorite.cr_mon].ws = ws;
	XSetInputFocus(fluorite.dpy, fluorite.root, RevertToPointerRoot, CurrentTime);
	FRemoveActiveWindow();
	XSync(fluorite.dpy, True);

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
	else if (fluorite.hpads != -1)
	{
		Scratchpads	*p = fluorite.pads[fluorite.hpads];
		for (Windows *w = p->s_wins; w != NULL; w = w->next)
			if (w->fc)
				XSetInputFocus(fluorite.dpy, w->w, RevertToPointerRoot, CurrentTime);
	}

	XChangeProperty(fluorite.dpy, fluorite.root, XInternAtom(fluorite.dpy, "_NET_CURRENT_DESKTOP", False), XA_CARDINAL, 32, PropModeReplace, (unsigned char *)&fluorite.cr_ws, 1);
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

	if (fluorite.ws[ws].fs)
	{
		fluorite.ws[ws].fs = False;
		for (Windows *w = fluorite.ws[ws].t_wins; w != NULL; w = w->next)
		{ FResetWindowOpacity(w->w); XSetWindowBorderWidth(fluorite.dpy, w->w, fluorite.conf.bw); w->fs = False; }
		for (Windows *w = fluorite.ws[ws].f_wins; w != NULL; w = w->next)
		{ FResetWindowOpacity(w->w); XSetWindowBorderWidth(fluorite.dpy, w->w, fluorite.conf.bw); w->fs = False; }
	}

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
	XUngrabServer(fluorite.dpy);
	return;

next:
	if (w->fs)
	{
		w->fs = False;
		fluorite.ws[fluorite.cr_ws].fs = False;
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

	FRemoveActiveWindow();
	XChangeProperty(fluorite.dpy, w->w, XInternAtom(fluorite.dpy, "_NET_WM_DESKTOP", False), XA_CARDINAL, 32, PropModeReplace, (unsigned char *)&ws, 1);
	FUpdateClientList();
	FRedrawWindows();
	FApplyBorders();

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

void FRotateStackWindows(int mode)
{
	if (fluorite.ws[fluorite.cr_ws].fs)
		return;

    Windows *first;
    Windows *last;
    Windows *ws_head;
    Windows *master;
    Windows *moved;
    Window focused = FFindFocusedWindow();
	Windows *w;

    ws_head = fluorite.ws[fluorite.cr_ws].t_wins;
    if (!ws_head || !ws_head->next || !ws_head->next->next)
        return;

    master = ws_head;
    first = master->next;

    switch (mode)
    {
        case UP:
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

    FResetFocus(fluorite.ws[fluorite.cr_ws].t_wins);
    first = master->next;
    if (!first)
        return;
    last = first;
    while (last->next)
        last = last->next;
    for (w = first; w; w = w->next)
    {
        if (w->w == focused)
        {
            if (mode == UP)
                w = (w->next) ? w->next : first;
            else
                w = (w->prev && w->prev != master) ? w->prev : last;
			goto redraw;
        }
    }

	if (!fluorite.ws[fluorite.cr_ws].t_wins)
		return;
	w = fluorite.ws[fluorite.cr_ws].t_wins;

redraw:
	w->fc = True;
	XSetInputFocus(fluorite.dpy, w->w, RevertToPointerRoot, CurrentTime);
    FRedrawWindows();
    FApplyBorders();
}


void FRotateWindows(int mode)
{
	if (fluorite.ws[fluorite.cr_ws].fs)
		return;

	Windows *first, *last;
	Window focused = FFindFocusedWindow();

	no_refocus = True;
	switch (mode)
	{
		case UP:
			if (!fluorite.ws[fluorite.cr_ws].t_wins || !fluorite.ws[fluorite.cr_ws].t_wins->next)
				return;
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
				return;
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
			return;
	}

	FResetFocus(fluorite.ws[fluorite.cr_ws].t_wins);
	first = fluorite.ws[fluorite.cr_ws].t_wins;
	last = first;
	while (last->next)
		last = last->next;
	for (Windows *w = fluorite.ws[fluorite.cr_ws].t_wins; w; w = w->next)
	{
		if (w->w == focused)
		{
			if (mode == UP)
				w = (w->next) ? w->next : fluorite.ws[fluorite.cr_ws].t_wins;
			else
				w = (w->prev) ? w->prev : last;
			XSetInputFocus(fluorite.dpy, w->w, RevertToPointerRoot, CurrentTime);
			w->fc = True;
			break;
		}
	}
	FRedrawWindows();
	FApplyBorders();
	XSync(fluorite.dpy, True);
	no_refocus = False;
}


void FChangeMasterOffset(int mode)
{
	if (fluorite.ws[fluorite.cr_ws].fs || fluorite.ws[fluorite.cr_ws].layout == STACKED)
		return ;

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

	no_refocus = True;
	XGetInputFocus(fluorite.dpy, &focused, &revert);
	if (focused == fluorite.root || !fluorite.ws[fluorite.cr_ws].t_wins || focused == fluorite.ws[fluorite.cr_ws].t_wins->w)
		return;

	for (w = fluorite.ws[fluorite.cr_ws].t_wins; w != NULL; w = w->next)
		if (w->w == focused)
			goto found;
	return ;

found:
	fluorite.ws[fluorite.cr_ws].t_wins = FDelWindow(fluorite.ws[fluorite.cr_ws].t_wins, w);
	ev.xmaprequest.window = focused;
	FMapRequest(ev);
	XSync(fluorite.dpy, True);
	no_refocus = False;
}

void FFocusNext()
{
	if (!fluorite.ws[fluorite.cr_ws].t_wins ||
			!fluorite.ws[fluorite.cr_ws].t_wins->next ||
			fluorite.ws[fluorite.cr_ws].fs ||
			fluorite.ws[fluorite.cr_ws].layout == STACKED)
		return;

	if (fluorite.ws[fluorite.cr_ws].layout == CASCADE)
	{
		FFocusPrev();
		return;
	}

	for (Windows *w = fluorite.ws[fluorite.cr_ws].t_wins; w != NULL; w = w->next)
	{
		if (w->fc)
		{
			w->fc = False;
			if (w->next)
			{
				w->next->fc = True;
				XSetInputFocus(fluorite.dpy, w->next->w, RevertToPointerRoot, CurrentTime);
				FWarpCursor(w->next->w);
			}
			else
			{
				fluorite.ws[fluorite.cr_ws].t_wins->fc = True;
				XSetInputFocus(fluorite.dpy, fluorite.ws[fluorite.cr_ws].t_wins->w, RevertToPointerRoot, CurrentTime);
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

	if (!fluorite.ws[fluorite.cr_ws].t_wins ||
			!fluorite.ws[fluorite.cr_ws].t_wins->next ||
			fluorite.ws[fluorite.cr_ws].fs ||
			fluorite.ws[fluorite.cr_ws].layout == STACKED)
		return;

	for (last = fluorite.ws[fluorite.cr_ws].t_wins; last->next != NULL; last = last->next);
	for (Windows *w = fluorite.ws[fluorite.cr_ws].t_wins; w != NULL; w = w->next)
	{
		if (w->fc)
		{
			w->fc = False;
			if (w->prev)
			{
				w->prev->fc = True;
				XSetInputFocus(fluorite.dpy, w->prev->w, RevertToPointerRoot, CurrentTime);
				FWarpCursor(w->prev->w);
			}
			else
			{
				if (fluorite.ws[fluorite.cr_ws].layout == CASCADE)
				{
					fluorite.ws[fluorite.cr_ws].t_wins->next->fc = True;
					XSetInputFocus(fluorite.dpy, fluorite.ws[fluorite.cr_ws].t_wins->next->w, RevertToPointerRoot, CurrentTime);
					FWarpCursor(fluorite.ws[fluorite.cr_ws].t_wins->next->w);
					break;
				}
				last->fc = True;
				XSetInputFocus(fluorite.dpy, last->w, RevertToPointerRoot, CurrentTime);
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

	if (fluorite.ws[fluorite.cr_ws].fs)
		return;

	XGetInputFocus(fluorite.dpy, &focused, &revert);
	for (Windows *w = fluorite.ws[fluorite.cr_ws].f_wins; w != NULL; w = w->next)
	{
		if (focused == w->w)
		{
			fluorite.ws[fluorite.cr_ws].f_wins = FDelWindow(fluorite.ws[fluorite.cr_ws].f_wins, w);
			w->prev = NULL;
			w->next = NULL;
			FResetFocus(fluorite.ws[fluorite.cr_ws].t_wins);
			w->fc = True;
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

	if (fluorite.ws[fluorite.cr_ws].fs)
		return;

	for (w = fluorite.ws[fluorite.cr_ws].f_wins; w->next != NULL; w = w->next);
	prev = w->prev;
	fluorite.ws[fluorite.cr_ws].t_wins = FAddWindow(fluorite.ws[fluorite.cr_ws].t_wins, w);
	fluorite.ws[fluorite.cr_ws].t_wins->fc = False;
	fluorite.ws[fluorite.cr_ws].t_wins->prev = prev;
	fluorite.ws[fluorite.cr_ws].t_wins = fluorite.ws[fluorite.cr_ws].f_wins;
	fluorite.ws[fluorite.cr_ws].f_wins = NULL;

	FRedrawWindows();
	FApplyBorders();
}

void FChangeLayout(int layout)
{
	if (fluorite.ws[fluorite.cr_ws].fs)
		return;

	if (fluorite.ws[fluorite.cr_ws].layout == layout)
		fluorite.ws[fluorite.cr_ws].layout = STARTING_LAYOUT;
	else
		fluorite.ws[fluorite.cr_ws].layout = layout;

	FResetFocus(fluorite.ws[fluorite.cr_ws].t_wins);
	FResetFocus(fluorite.ws[fluorite.cr_ws].f_wins);
	if (fluorite.ws[fluorite.cr_ws].t_wins)
	{
		fluorite.ws[fluorite.cr_ws].t_wins->fc = True;
		XSetInputFocus(fluorite.dpy, fluorite.ws[fluorite.cr_ws].t_wins->w, RevertToPointerRoot, CurrentTime);
	}

	FRedrawWindows();
	XSync(fluorite.dpy, True);
	FApplyBorders();
}

void FFullscreenToggle()
{
	Window focused;
	int revert;
	Windows *w;

	XGetInputFocus(fluorite.dpy, &focused, &revert);

	for (w = fluorite.ws[fluorite.cr_ws].t_wins; w != NULL; w = w->next)
		if (focused == w->w)
			goto next;
	for (w = fluorite.ws[fluorite.cr_ws].f_wins; w != NULL; w = w->next)
		if (focused == w->w)
			goto next;

	goto exit;
	

next:
	if (fluorite.ws[fluorite.cr_ws].fs)
	{
		FResetWindowOpacity(w->w);
		XSetWindowBorderWidth(fluorite.dpy, w->w, fluorite.conf.bw);
	}
	else
	{
		FSetWindowOpacity(w->w, 100);
		XSetWindowBorderWidth(fluorite.dpy, w->w, 0);
	}
	w->fs = !w->fs;
	fluorite.ws[fluorite.cr_ws].fs = !fluorite.ws[fluorite.cr_ws].fs;
	FRedrawWindows();
	XSync(fluorite.dpy, True);
	FApplyBorders();
	FWarpCursor(w->w);

exit:
	return ;
}

void FFloatingHideShow()
{
	if (fluorite.ws[fluorite.cr_ws].fs)
		return; 
	
	no_unmap = True;
	XGrabServer(fluorite.dpy);
	for (Windows *w = fluorite.ws[fluorite.cr_ws].f_wins; w != NULL; w = w->next)
	{
		if (fluorite.ws[fluorite.cr_ws].fl_hdn)
		{
			XMapWindow(fluorite.dpy, w->w);
			XSetWindowBorder(fluorite.dpy, w->w, fluorite.conf.bu);
			if (w->fc)
			{
				XSetInputFocus(fluorite.dpy, w->w, RevertToPointerRoot, CurrentTime);
				XSetWindowBorder(fluorite.dpy, w->w, fluorite.conf.bf);
			}
			FApplyBorders();
		}
		else
		{
			XUnmapWindow(fluorite.dpy, w->w);
			XSetInputFocus(fluorite.dpy, fluorite.root, RevertToPointerRoot, CurrentTime);
			for (Windows *w = fluorite.ws[fluorite.cr_ws].t_wins; w != NULL; w = w->next)
			{
				if (!w->fc)
					continue;
				no_refocus = True;
				XSetInputFocus(fluorite.dpy, w->w, RevertToPointerRoot, CurrentTime);
				FWarpCursor(w->w);
				FApplyActiveWindow(w->w);
				FApplyBorders();
				no_refocus = False;
			}
			FRemoveActiveWindow();
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
	Windows *w;
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

	for (w = fluorite.ws[fluorite.cr_ws].f_wins; w != NULL; w = w->next)
	{
		if (!w->fc)
			continue;
		XSetInputFocus(fluorite.dpy, w->w, RevertToPointerRoot, CurrentTime);
		FWarpCursor(w->w);
		goto next;
	}
	for (w = fluorite.ws[fluorite.cr_ws].t_wins; w != NULL; w = w->next)
	{
		if (!w->fc)
			continue;
		XSetInputFocus(fluorite.dpy, w->w, RevertToPointerRoot, CurrentTime);
		FWarpCursor(w->w);
		goto next;
	}

	if (fluorite.ws[fluorite.cr_ws].t_wins)
	{
		fluorite.ws[fluorite.cr_ws].t_wins->fc = True;
		XSetInputFocus(fluorite.dpy, fluorite.ws[fluorite.cr_ws].t_wins->w, RevertToPointerRoot, CurrentTime);
		FWarpCursor(fluorite.ws[fluorite.cr_ws].t_wins->w);
	}

next:
	if (!fluorite.ws[fluorite.cr_ws].fs)
		FApplyBorders();
}

void FResetMasterOffset()
{
	fluorite.ws[fluorite.cr_ws].mo = fluorite.conf.mo;
	FRedrawWindows();
	FApplyBorders();
}

static inline unsigned FHashKey(KeySym key) { return (unsigned)(key % HASH_SIZE); }

static Scratchpads *FGetScratchpad(KeySym key)
{
	unsigned hash = FHashKey(key);
	Scratchpads *p = fluorite.pads[hash];
	if (p && p->key == key)
		return p;
	return NULL;
}

static Scratchpads *FCreateOrGetScratchpad(KeySym key)
{
	unsigned hash = FHashKey(key);
	Scratchpads *p = FGetScratchpad(key);
	if (p && p->key == key)
		return p;

	p = (Scratchpads *) calloc(1, sizeof(Scratchpads));
	p->key = key;
	p->s_wins = NULL;
	fluorite.pads[hash] = p;
	return p;
}

void FAddWindowToScratchpad()
{
	XEvent ev;
	Window focused;
	int revert;
	KeySym key;
	int hkey;
	Windows *w;

	XGrabServer(fluorite.dpy);
	XSync(fluorite.dpy, True);
	XGrabKeyboard(fluorite.dpy, fluorite.root, True, GrabModeAsync, GrabModeAsync, CurrentTime);
	while (1)
	{
		XNextEvent(fluorite.dpy, &ev);
		if (ev.type == KeyPress)
		{
			key = XLookupKeysym(&ev.xkey, 0);
			hkey = FHashKey(key);
			break;
		}
	}
	XUngrabKeyboard(fluorite.dpy, CurrentTime);
	XUngrabServer(fluorite.dpy);

	XGetInputFocus(fluorite.dpy, &focused, &revert);
	for (w = fluorite.ws[fluorite.cr_ws].t_wins; w != NULL; w = w->next)
	{
		if (focused != w->w)
			continue;
		fluorite.ws[fluorite.cr_ws].t_wins = FDelWindow(fluorite.ws[fluorite.cr_ws].t_wins, w);
		goto next;
	}
	for (w = fluorite.ws[fluorite.cr_ws].f_wins; w != NULL; w = w->next)
	{
		if (focused != w->w)
			continue;
		fluorite.ws[fluorite.cr_ws].f_wins = FDelWindow(fluorite.ws[fluorite.cr_ws].f_wins, w);
		goto next;
	}
	return;

next:
	if (w->fs || fluorite.ws[fluorite.cr_ws].fs)
	{
		fluorite.ws[fluorite.cr_ws].fs = False;
		w->fs = False;
		FResetWindowOpacity(w->w);
		XSetWindowBorderWidth(fluorite.dpy, w->w, fluorite.conf.bw);
	}
	w->ww = fluorite.mon[fluorite.cr_mon].mw / 2;
	w->wh = fluorite.mon[fluorite.cr_mon].mh / 2;
	w->wx = fluorite.mon[fluorite.cr_mon].mx + (fluorite.mon[fluorite.cr_mon].mw - w->ww) / 2;
	w->wy = fluorite.mon[fluorite.cr_mon].my + (fluorite.mon[fluorite.cr_mon].mh - w->wh) / 2;
    XMoveResizeWindow(fluorite.dpy, w->w, w->wx, w->wy, w->ww, w->wh);
	XChangeProperty(fluorite.dpy, w->w, XInternAtom(fluorite.dpy, "_NET_WM_DESKTOP", False), XA_CARDINAL, 32, PropModeReplace, (unsigned char *)&key, sizeof(key));

	Scratchpads *p = FCreateOrGetScratchpad(key);
	for (Windows *w = p->s_wins; w != NULL; w = w->next)
		XMapWindow(fluorite.dpy, w->w);
	no_unmap = True;
	if (fluorite.hpads != -1 && fluorite.hpads != hkey)
		for (Windows *w = fluorite.pads[fluorite.hpads]->s_wins; w != NULL; w = w->next)
			XUnmapWindow(fluorite.dpy, w->w);
	XSync(fluorite.dpy, True);
	no_unmap = False;
	w->next = NULL;
	w->prev = NULL;
	p->s_wins = FAddWindow(p->s_wins, w);
	fluorite.hpads = hkey;
	FRedrawWindows();
	XSync(fluorite.dpy, True);
	XSetInputFocus(fluorite.dpy, w->w, RevertToPointerRoot, CurrentTime);
	FApplyBorders();
	FPolybarScratchpadsIPC();
}

void FDelWindowFromScratchpad()
{
	if (fluorite.hpads == -1)
		return;

	Window focused;
	int revert;
	Scratchpads *p = fluorite.pads[fluorite.hpads];

	XGetInputFocus(fluorite.dpy, &focused, &revert);
	for (Windows *w = p->s_wins; w != NULL; w = w->next)
	{
		if (focused != w->w)
			continue;
		p->s_wins = FDelWindow(p->s_wins, w);
		w->next = NULL;
		w->prev = NULL;
		fluorite.ws[fluorite.cr_ws].t_wins = FAddWindow(fluorite.ws[fluorite.cr_ws].t_wins, w);
		if (!p->s_wins)
		{
			memset(fluorite.pads[fluorite.hpads], 0, sizeof(Scratchpads));
			fluorite.hpads = -1;
		}
		FRedrawWindows();
		FApplyBorders();
		XChangeProperty(fluorite.dpy, w->w, XInternAtom(fluorite.dpy, "_NET_WM_DESKTOP", False), XA_CARDINAL, 32, PropModeReplace, (unsigned char *)&fluorite.cr_ws, 1);
		XSetInputFocus(fluorite.dpy, w->w, RevertToPointerRoot, CurrentTime);
		FWarpCursor(w->w);
		break;
	}
	FPolybarScratchpadsIPC();
}

void FScratchpadHideShow()
{
	XEvent ev;
	KeySym key;
	Scratchpads *p, *op;
	int hkey;

	XGrabServer(fluorite.dpy);
	XSync(fluorite.dpy, True);
	XGrabKeyboard(fluorite.dpy, fluorite.root, True, GrabModeAsync, GrabModeAsync, CurrentTime);
	while (1)
	{
		XNextEvent(fluorite.dpy, &ev);
		if (ev.type == KeyPress)
		{
			key = XLookupKeysym(&ev.xkey, 0);
			break;
		}
	}
	XUngrabKeyboard(fluorite.dpy, CurrentTime);
	XUngrabServer(fluorite.dpy);

	hkey = FHashKey(key);
	p = fluorite.pads[hkey];
	if (!p || p->key != key || !p->s_wins)
		return;

	op = (fluorite.hpads != -1) ? fluorite.pads[fluorite.hpads] : NULL;

	if (op && op != p)
	{
		no_unmap = True;
		for (Windows *w = op->s_wins; w != NULL; w = w->next)
			XUnmapWindow(fluorite.dpy, w->w);
		XSync(fluorite.dpy, True);
		no_unmap = False;
	}

	FRemoveActiveWindow();
	if (fluorite.hpads != hkey || fluorite.hpads == -1)
	{
		for (Windows *w = p->s_wins; w != NULL; w = w->next)
		{
			XMapWindow(fluorite.dpy, w->w);
			XSetWindowBorder(fluorite.dpy, w->w, fluorite.conf.bu);
			if (w->fc)
			{
				XSetWindowBorder(fluorite.dpy, w->w, fluorite.conf.bf);
				XSetInputFocus(fluorite.dpy, w->w, RevertToPointerRoot, CurrentTime);
			}
		}
		fluorite.hpads = hkey;
		FRedrawWindows();
	}
	else
	{
		no_unmap = True;
		for (Windows *w = p->s_wins; w != NULL; w = w->next)
			XUnmapWindow(fluorite.dpy, w->w);
		XSync(fluorite.dpy, True);
		no_unmap = False;
		fluorite.hpads = -1;
		if (fluorite.ws[fluorite.cr_ws].t_wins)
		{
			Windows *w;
			for (w = fluorite.ws[fluorite.cr_ws].t_wins; w != NULL; w = w->next)
				if (w->fc)
					XSetInputFocus(fluorite.dpy, w->w, RevertToPointerRoot, CurrentTime);
			for (w = fluorite.ws[fluorite.cr_ws].f_wins; w != NULL; w = w->next)
				if (w->fc)
					XSetInputFocus(fluorite.dpy, w->w, RevertToPointerRoot, CurrentTime);
		}
	}
	FApplyBorders();
	FPolybarScratchpadsIPC();
}

void FCenterScratchpadWindow()
{
	Window focused;
	int revert;
	Windows *w;

	if (fluorite.hpads == -1)
		return;

	XGetInputFocus(fluorite.dpy, &focused, &revert);
	for (w = fluorite.pads[fluorite.hpads]->s_wins; w != NULL; w = w->next)
		if (focused == w->w)
			goto found;
	return;

found:
	w->wx = fluorite.mon[fluorite.cr_mon].mx + (fluorite.mon[fluorite.cr_mon].mw - w->ww) / 2;
	w->wy = fluorite.mon[fluorite.cr_mon].my + (fluorite.mon[fluorite.cr_mon].mh - w->wh) / 2;
    XMoveResizeWindow(fluorite.dpy, w->w, w->wx, w->wy, w->ww, w->wh);
	FWarpCursor(w->w);
}

static void FSearchAndDestoryGhostWindows()
{
	for (Windows *w = fluorite.ws[fluorite.cr_ws].t_wins; w != NULL; w = w->next)
		if (!FWindowExists(fluorite.dpy, w->w))
			fluorite.ws[fluorite.cr_ws].t_wins = FDelWindow(fluorite.ws[fluorite.cr_ws].t_wins, w);
	for (Windows *w = fluorite.ws[fluorite.cr_ws].f_wins; w != NULL; w = w->next)
		if (!FWindowExists(fluorite.dpy, w->w))
			fluorite.ws[fluorite.cr_ws].f_wins = FDelWindow(fluorite.ws[fluorite.cr_ws].f_wins, w);
	// for (int i = 0; i < HASH_SIZE; i++)
	// {
	// 	for (Windows *w = fluorite.pads[i]->s_wins; w != NULL; w = w->next)
	// 		if (!FWindowExists(fluorite.dpy, w->w))
	// 			fluorite.pads[i]->s_wins = FDelWindow(fluorite.pads[i]->s_wins, w);
	// }
}

static void FPolybarLayoutIPC(const int msg)
{
	if (!POLYBAR_IPC)
		return ;
	char command[256];
	snprintf(command, sizeof(command), "polybar-msg hook fluorite_layout %d", msg);
	FExecute(command);
}

static void FPolybarScratchpadsIPC()
{
	if (!POLYBAR_IPC)
		return ;
	char *scratchpads_value = NULL;
	char *tmp;
	char command[256];
	Atom scratchpads_atom = XInternAtom(fluorite.dpy, "FLUORITE_SCRATCHPADS", False);

	scratchpads_value = (char *) calloc(HASH_SIZE, sizeof(char));
	for (int i = 0; i < HASH_SIZE; i++) 
	{
		if (fluorite.pads[i] && fluorite.pads[i]->key)
		{
			tmp = strdup(XKeysymToString(fluorite.pads[i]->key));
			if (tmp && fluorite.pads[i]->s_wins)
			{
				if (i == fluorite.hpads)
					strcat(scratchpads_value, "[");
				strcat(scratchpads_value, tmp);
				if (i == fluorite.hpads)
					strcat(scratchpads_value, "]");
				strcat(scratchpads_value, " ");
				free(tmp);
			}
		}
	}

	if (strlen(scratchpads_value) > 1)
	{
		XChangeProperty(fluorite.dpy, fluorite.root, scratchpads_atom, XA_STRING, 8, PropModeReplace, (unsigned char *)scratchpads_value, strlen(scratchpads_value));
		XSync(fluorite.dpy, True);
	}
	else
		XDeleteProperty(fluorite.dpy, fluorite.root, scratchpads_atom);

	free(scratchpads_value);
	snprintf(command, sizeof(command), "polybar-msg hook fluorite_scratchpads 1");
	FExecute(command);
}

static void FGetFixedPartialStrut(Window w, int new_win)
{
	Atom strut_atom = XInternAtom(fluorite.dpy, "_NET_WM_STRUT_PARTIAL", False);
	Atom actual_type;
	int actual_format;
	unsigned long nitems, bytes_after;
	unsigned char *prop = NULL;
	long *strut;
	int sh, sw;
	int redraw = False;
	Windows *fx;
	int keep_mon = fluorite.cr_mon;
	Window focused;
	int revert;

	if (XGetWindowProperty(fluorite.dpy, w, strut_atom, 0, 12, False, XA_CARDINAL, &actual_type, &actual_format, &nitems, &bytes_after, &prop) == !Success)
		return;
	if (!prop)
		return;
	sh = DisplayHeight(fluorite.dpy, fluorite.scr);
	sw = DisplayWidth(fluorite.dpy, fluorite.scr);
	XGetInputFocus(fluorite.dpy, &focused, &revert);
	if (actual_type == XA_CARDINAL && actual_format == 32 && nitems >= 4)
	{
		strut = (long *) prop;
		for (int i = 0; i < fluorite.ct_mon; i++)
		{
			Monitors mon = fluorite.mon[i];
			if (mon.mx < strut[LEFT] && strut[LEFT] < (mon.mx + mon.mw - 1) && strut[LEFT_EY] >= mon.my && strut[LEFT_SY] < (mon.my + mon.mh))
			{
				if (mon.sl < 0) fluorite.mon[i].sl += strut[LEFT] - mon.mx;
				else fluorite.mon[i].sl = MAX(strut[LEFT] - mon.mx, mon.sl);
				redraw = True;
			}
			if ((mon.mx + mon.mw) > (sw - strut[RIGHT]) && (sw - strut[RIGHT]) > mon.mx && strut[RIGHT_EY] >= mon.my && strut[RIGHT_SY] < (mon.my + mon.mh))
			{
				if (mon.sr < 0) fluorite.mon[i].sr += (mon.mx + mon.mw) - sw + strut[RIGHT];
				else fluorite.mon[i].sr = MAX((mon.mx + mon.mw) - sw + strut[RIGHT], mon.sr);
				redraw = True;
			}
			if (mon.my < strut[TOP] && strut[TOP] < (mon.my + mon.mh - 1) && strut[TOP_EX] >= mon.mx && strut[TOP_SX] < (mon.mx + mon.mw))
			{
				if (mon.st < 0) fluorite.mon[i].st += strut[TOP] - mon.my;
				else fluorite.mon[i].st = MAX(strut[TOP] - mon.my, mon.st);
				redraw = True;
			}
			if ((mon.my + mon.mh) > (sh - strut[BOTTOM]) && (sh - strut[BOTTOM]) > mon.my && strut[BOTTOM_EX] >= mon.mx && strut[BOTTOM_SX] < (mon.mx + mon.mw))
			{
				if (mon.sb < 0) fluorite.mon[i].sb += (mon.my + mon.mh) - sh + strut[BOTTOM];
				else fluorite.mon[i].sb = MAX((mon.my + mon.mh) - sh + strut[BOTTOM], mon.sb);
				redraw = True;
			}
			if (redraw)
			{
				if (new_win)
				{
					fx = (Windows *) calloc(1, sizeof(Windows));
					fx->w = w;
					fluorite.mon[i].fx_win = FAddWindow(fluorite.mon[i].fx_win, fx);
				}
				FChangeMonitor(i);
				redraw = False;
			}
		}
	}
	FChangeMonitor(keep_mon);
	XWarpPointer(
		fluorite.dpy, None, fluorite.root,
		0, 0, 0, 0,
		fluorite.mon[keep_mon].mx + fluorite.mon[keep_mon].mw / 2,
		fluorite.mon[keep_mon].my + fluorite.mon[keep_mon].mh / 2
	);
	if (focused != None)
	{
		for (Windows *w = fluorite.ws[fluorite.cr_ws].t_wins; w != NULL; w = w->next)
			if (focused == w->w)
				w->fc = 1;
		for (Windows *w = fluorite.ws[fluorite.cr_ws].f_wins; w != NULL; w = w->next)
			if (focused == w->w)
				w->fc = 1;
		if (fluorite.hpads != -1)
		{
			Scratchpads *p = fluorite.pads[fluorite.hpads];
			for (Windows *w = p->s_wins; w != NULL; w = w->next)
				if (focused == w->w)
					w->fc = 1;
		}
		XSetInputFocus(fluorite.dpy, focused, RevertToPointerRoot, CurrentTime);
		FWarpCursor(focused);
	};
	FRedrawWindows();
	XSync(fluorite.dpy, True);
	FApplyBorders();
	XFree(prop);
}

static void FRecalculateStrut()
{
	for (int i = 0; i < fluorite.ct_mon; i++)
		for (Windows *fx = fluorite.mon[i].fx_win; fx != NULL; fx = fx->next)
			FGetFixedPartialStrut(fx->w, False);
}
