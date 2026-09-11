#define FLUORITE_VERSION "Fluorite [EVO 3] (Beta 3)"

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
#include <stdlib.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/inotify.h>
#include <errno.h>
#include <confuse.h>
#include <sys/socket.h>
#include <sys/un.h>

/* DEF: constant */
#define MAX_WS 10
#define MODMASK(mask)	(mask & ~(numlockmask | LockMask) & (ShiftMask|ControlMask|Mod1Mask|Mod2Mask|Mod3Mask|Mod4Mask|Mod5Mask))
#define LENGTH(X)		(sizeof X / sizeof X[0])
#define EVENT_SIZE		(sizeof(struct inotify_event))
#define BUF_LEN			(1024 * (EVENT_SIZE + 16))
#define HASH_SIZE		256
#define SOCKET_PATH		"/tmp/fluorite.sock"

static inline int MAX(int a, int b) { return a > b ? a : b; }

enum MODE
{
	UP,
	DOWN,
	RESET
};

enum LAYOUT
{
	CASCADE,
	DWM,
	CENTERED,
	STACKED,
	SCROLLING
};


enum SIDES
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

enum FUN_TYPE
{
	VOID,
	INT,
	CHAR
};

typedef struct
{
	char	*name;
	int		type;
	void	(*void_fun)();
	void	(*int_fun)(int arg);
	void	(*char_fun)(char *arg);
} UserFunc;

typedef struct
{
	unsigned int	mod;
	KeySym			key;
	int				type;
	void			(*int_fun)(int arg);
	void			(*char_fun)(char *arg);
	void			(*void_fun)();
	int				int_arg;
	char			*char_arg;
} NeoBindings;

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
	int swp;
	int stk_blw;
	struct Windows *next;
	struct Windows *prev;
} Windows;

typedef struct
{
	int			bf;
	int  		bu;
	int  		bw;
	int  		igp;
	int  		ogp;
	int  		so;
	int  		mo;
	cfg_bool_t	fw;
	cfg_bool_t	ff;
	int			sl;
	cfg_bool_t	wc;
	cfg_bool_t	jtu;
	KeySym		mt;
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
	int			fx_hdn;
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
	int				orgz;
} Fluorite;

/* DEF: functions */
static void		FInit();
static int  	FErrorHandler(Display *dis, XErrorEvent *ev);
static void 	FLoadXresources();
static void 	FLoadDefaultTheme();
static void		FLoadDefaultConfig();
static void 	FInitMonitors();
static void 	FInitWorkspaces();
static void 	FApplyProps();
static void 	FGrabKeys(Window w);
static void 	*FInotifyConfigAndXresources(void *ptr);
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
static int  	FCheckWindowIsTransientOrPopup(Window w);
static void 	FChangeMonitor(int mon);
static void 	FConfigureRequest(XEvent ev);
static void 	FKeyPress(XEvent ev);
static int  	FFindWorkspaceFromWindow(Window w);
static void		FResetFocus(Windows *w);
static int		FWindowExists(Display *dpy, Window win);
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
static void		FSetWindowFullscreen(Window w, int fs);
static void		FSearchAndDestoryGhostWindows();
static void		FPolybarLayoutIPC(const int layout);
static void		FPolybarScratchpadsIPC();
static void		FGetFixedPartialStrut(Window w, int new_win);
static void		FRecalculateStrut(int mon);
static void		FResetMonitorStrut(int mon);
static int		FCheckCanSwallow(Window w);
static void		FReloadConfig();
static void		FExecute(char *argument);
static void 	FQuit();
static void 	FCloseWindow();
static void 	FRotateWindows(int mode);
static void 	FRotateStackWindows(int mode);
static void 	FChangeMasterOffset(int mode);
static void 	FSwapWithMaster();
static void 	FFocusNext();
static void 	FFocusPrev();
static void 	FNextWorkspace();
static void 	FPrevWorkspace();
static void 	FShowWorkspace(int ws);
static void 	FSendWindowToWorkspace(int ws);
static void 	FTileWindow();
static void 	FChangeLayout(int layout);
static void 	FTileAllWindows();
static void 	FToggleFullscreen();
static void 	FFloatingHideShow();
static void 	FSendWindowToNextWorkspace();
static void 	FSendWindowToPrevWorkspace();
static void 	FFocusNextMonitor();
static void 	FResetMasterOffset();
static void 	FReloadXresources();
static void 	FAddWindowToScratchpad();
static void 	FDelWindowFromScratchpad();
static void 	FScratchpadHideShow();
static void 	FCenterScratchpadWindow();
static void 	FToggleFixedStrut();
static void 	FCycleLayouts();
static void		FToggleOrganizer();
static void		FRedrawOrganizer();
static void		FRedrawScrolling();
static Windows *FAddWindowScrolling(Windows *head, Windows *nw);
static Windows *FGetColStart(Windows *w);
static Windows *FGetColEnd(Windows *w);
static int		FCountColWins(Windows *w);
static void		FScrollingFocusLeft();
static void		FScrollingFocusRight();
static void		FScrollingMoveLeft();
static void		FScrollingMoveRight();
static void		FScrollingResizeIncrease();
static void		FScrollingResizeDecrease();
static void		FScrollingMoveWindowToColumnLeft();
static void		FScrollingMoveWindowToColumnRight();
static void		FScrollingFocusUp();
static void		FScrollingFocusDown();
static void		FScrollingMoveUp();
static void		FScrollingMoveDown();
static void		FUpdateWorkarea();
static void		FUpdateDesktopViewport();
static void		*FIPCServerThread(void *ptr);
static void		FScratchpadToggleByKeysym(char *arg);

/* DEF: globals */
static Fluorite fluorite;
static int no_unmap = False;
static int no_warp = False;
static int no_refocus = False;
static int no_redraw = False;
static unsigned int numlockmask = 0;
static char **workspaces_names;
static char **floating_windows;
static char **fixed_windows;
static char **swallowing_windows;
static NeoBindings *binds = NULL;
static int binds_count = 0;
static Cursor cnorm;
static Cursor cmove;
static Cursor cresz;
static UserFunc user_functions_list[] = {
	{"close_window",				VOID,	FCloseWindow, NULL, NULL},
	{"swap_with_master",			VOID,	FSwapWithMaster, NULL, NULL},
	{"focus_next_window",			VOID,	FFocusNext, NULL, NULL},
	{"focus_prev_window",			VOID,	FFocusPrev, NULL, NULL},
	{"focus_next_workspace",		VOID,	FNextWorkspace, NULL, NULL},
	{"focus_prev_workspace",		VOID,	FPrevWorkspace, NULL, NULL},
	{"tile_window",					VOID,	FTileWindow, NULL, NULL},
	{"tile_all",					VOID,	FTileAllWindows, NULL, NULL},
	{"toggle_fullscreen",			VOID,	FToggleFullscreen, NULL, NULL},
	{"hide_show_floating",			VOID,	FFloatingHideShow, NULL, NULL},
	{"hide_show_scratchpad",		VOID,	FScratchpadHideShow, NULL, NULL},
	{"send_window_next_workspace",	VOID,	FSendWindowToNextWorkspace, NULL, NULL},
	{"send_window_prev_workspace",	VOID,	FSendWindowToPrevWorkspace, NULL, NULL},
	{"focus_next_monitor",			VOID,	FFocusNextMonitor, NULL, NULL},
	{"reset_master_offset",			VOID,	FResetMasterOffset, NULL, NULL},
	{"add_window_scratchpad",		VOID,	FAddWindowToScratchpad, NULL, NULL},
	{"del_window_scratchpad",		VOID,	FDelWindowFromScratchpad, NULL, NULL},
	{"center_scratchpad_window",	VOID,	FCenterScratchpadWindow, NULL, NULL},
	{"toggle_fixed_strut",			VOID,	FToggleFixedStrut, NULL, NULL},
	{"cycle_layouts",				VOID,	FCycleLayouts, NULL, NULL},
	{"close_fluorite",				VOID,	FQuit, NULL, NULL},
	{"toggle_organizer",			VOID,	FToggleOrganizer, NULL, NULL},
	{"window_rotate",				INT,	NULL, FRotateWindows, NULL},
	{"stack_rotate",				INT, 	NULL, FRotateStackWindows, NULL},
	{"change_master_offset",		INT, 	NULL, FChangeMasterOffset, NULL},
	{"show_workspace",				INT, 	NULL, FShowWorkspace, NULL},
	{"send_window_to_workspace",	INT, 	NULL, FSendWindowToWorkspace, NULL},
	{"change_layout",				INT, 	NULL, FChangeLayout, NULL},
	{"exec",						CHAR,	NULL, NULL, FExecute},
	{"scrolling_focus_left",		VOID,	FScrollingFocusLeft, NULL, NULL},
	{"scrolling_focus_right",		VOID,	FScrollingFocusRight, NULL, NULL},
	{"scrolling_move_left",			VOID,	FScrollingMoveLeft, NULL, NULL},
	{"scrolling_move_right",		VOID,	FScrollingMoveRight, NULL, NULL},
	{"scrolling_resize_inc",		VOID,	FScrollingResizeIncrease, NULL, NULL},
	{"scrolling_resize_dec",		VOID,	FScrollingResizeDecrease, NULL, NULL},
	{"scrolling_move_window_to_column_left",	VOID,	FScrollingMoveWindowToColumnLeft, NULL, NULL},
	{"scrolling_move_window_to_column_right",	VOID,	FScrollingMoveWindowToColumnRight, NULL, NULL},
	{"scrolling_focus_up",			VOID,	FScrollingFocusUp, NULL, NULL},
	{"scrolling_focus_down",		VOID,	FScrollingFocusDown, NULL, NULL},
	{"scrolling_move_up",			VOID,	FScrollingMoveUp, NULL, NULL},
	{"scrolling_move_down",			VOID,	FScrollingMoveDown, NULL, NULL},
	{"toggle_scratchpad_key", CHAR, NULL, NULL, FScratchpadToggleByKeysym},
};

int main(void)
{
	pthread_t t_inotify;
	pthread_t t_ipc;

	FInit();
	pthread_create(&t_inotify, NULL, &FInotifyConfigAndXresources, NULL);
	pthread_create(&t_ipc, NULL, &FIPCServerThread, NULL);
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
	fluorite.orgz = False;

	XrmInitialize();
	XSetErrorHandler(FErrorHandler);
	FApplyProps();
	FLoadXresources();
	workspaces_names = (char **) calloc(10, sizeof(char *));
	floating_windows = (char **) calloc(1, sizeof(char *));
	fixed_windows = (char **) calloc(1, sizeof(char *));
	swallowing_windows = (char **) calloc(1, sizeof(char *));
	FReloadConfig();
	FInitMonitors();
	FInitWorkspaces();
	FUpdateWorkarea();
	FUpdateDesktopViewport();
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

	FLoadDefaultTheme();

	dummy_display = XOpenDisplay(NULL);
	xrm = XResourceManagerString(dummy_display);

	if (!xrm)
		return ;

	xdb = XrmGetStringDatabase(xrm);

	if (XrmGetResource(xdb, "fluorite.border_width", "*", &type, &xval))
		if (xval.addr) fluorite.conf.bw = strtoul(xval.addr, NULL, 10);
	if (XrmGetResource(xdb, "fluorite.gaps", "*", &type, &xval))
	{
		if (xval.addr)
		{
			fluorite.conf.igp = strtoul(xval.addr, NULL, 10);
			fluorite.conf.ogp = strtoul(xval.addr, NULL, 10);
		}
	}
	if (XrmGetResource(xdb, "fluorite.inner_gaps", "*", &type, &xval))
		if (xval.addr) fluorite.conf.igp = strtoul(xval.addr, NULL, 10);
	if (XrmGetResource(xdb, "fluorite.outer_gaps", "*", &type, &xval))
		if (xval.addr) fluorite.conf.ogp = strtoul(xval.addr, NULL, 10);
	if (XrmGetResource(xdb, "fluorite.stack_offset", "*", &type, &xval))
		if (xval.addr) fluorite.conf.so = strtoul(xval.addr, NULL, 10);
	if (XrmGetResource(xdb, "fluorite.default_master_offset", "*", &type, &xval))
		if (xval.addr) fluorite.conf.mo = strtoul(xval.addr, NULL, 10);

	if (XrmGetResource(xdb, "fluorite.border_focused", "*", &type, &xval))
	{
		if (xval.addr)
		{
			char *addr = xval.addr;
			if (*addr == '#') addr++;
			fluorite.conf.bf = strtoul(addr, NULL, 16);
		}
	}
	if (XrmGetResource(xdb, "fluorite.border_unfocused", "*", &type, &xval))
	{
		if (xval.addr)
		{
			char *addr = xval.addr;
			if (*addr == '#') addr++;
			fluorite.conf.bu = strtoul(addr, NULL, 16);
		}
	}

	fluorite.conf.bf |= 0xff << 24;
	fluorite.conf.bu |= 0xff << 24;
	XrmDestroyDatabase(xdb);
	XCloseDisplay(dummy_display);
}

static void FLoadDefaultTheme()
{
	fluorite.conf.bw = 2;
	fluorite.conf.bf = 0xeb6f92;
	fluorite.conf.bu = 0x524f67;
	fluorite.conf.igp = 10;
	fluorite.conf.ogp = 20;
	fluorite.conf.so = 5;
	fluorite.conf.mo = 0;
}

static void FLoadDefaultConfig()
{
	fluorite.conf.ff = False;
	fluorite.conf.sl = CASCADE;
	fluorite.conf.wc = False;
	fluorite.conf.jtu = True;
	for (int i = 0; i < 10; i++)
	{
		if (workspaces_names[i])
			free(workspaces_names[i]);
		char name[2] = { 0 };
		if (i == 9)
			name[1] = '0';
		else
			name[1] = i + '1';
		workspaces_names[i] = strdup(name);
	}
}

static void FInitMonitors()
{
	int hot_plug = False;
	int prev_ct = fluorite.ct_mon;
	XRRMonitorInfo *infos = XRRGetMonitors(fluorite.dpy, fluorite.root, 0, &fluorite.ct_mon);
	Monitors *prev_mon = (fluorite.mon != NULL) ? (Monitors *) calloc(prev_ct, sizeof(Monitors)) : NULL;

	if (fluorite.mon != NULL)
	{
		if (prev_ct == fluorite.ct_mon)
		{
			int n = (prev_ct < fluorite.ct_mon) ? prev_ct : fluorite.ct_mon;
			for (int i = 0; i < n; i++)
				prev_mon[i] = fluorite.mon[i];
		}
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

	fluorite.mon = (Monitors *) calloc(fluorite.ct_mon, sizeof(Monitors));
	for (int i = 0; i < fluorite.ct_mon; i++)
	{
		fluorite.mon[i].mx = infos[i].x;
		fluorite.mon[i].my = infos[i].y;
		fluorite.mon[i].mw = infos[i].width;
		fluorite.mon[i].mh = infos[i].height;
		fluorite.mon[i].ws = i;
		FResetMonitorStrut(i);
		fluorite.mon[i].primary = False;
		fluorite.mon[i].fx_win = NULL;
		fluorite.mon[i].fx_hdn = False;
		if (infos[i].primary)
		{
			fluorite.mon[i].primary = True;
			fluorite.cr_mon = i;
			if (!hot_plug)
				xdo_move_mouse(fluorite.xdo, fluorite.mon[i].mx + (fluorite.mon[i].mw / 2), fluorite.mon[i].my + (fluorite.mon[i].mh / 2), fluorite.scr);
		}
	}
	XFree(infos);

	if (hot_plug)
	{
		no_warp = True;
		int primary = 0;
		for (int i = 0; i < fluorite.ct_mon; i++)
		{
			if (prev_ct == fluorite.ct_mon)
				fluorite.mon[i] = prev_mon[i];
			if (fluorite.mon[i].primary)
				primary = i;
			fluorite.cr_ws = fluorite.mon[i].ws;
			fluorite.cr_mon = i;
			FResetMonitorStrut(i);
			FRecalculateStrut(i);
			for (Windows *w = fluorite.ws[fluorite.cr_ws].t_wins; w != NULL; w = w->next)
				XMapWindow(fluorite.dpy, w->w);
			for (Windows *w = fluorite.ws[fluorite.cr_ws].f_wins; w != NULL; w = w->next)
				XMapWindow(fluorite.dpy, w->w);
			FRedrawWindows();
			XSync(fluorite.dpy, True);
			FApplyBorders();
		}
		free(prev_mon);
		fluorite.cr_mon = primary;
		fluorite.cr_ws = fluorite.mon[fluorite.cr_mon].ws;
		if (fluorite.ws[fluorite.cr_mon].t_wins)
			XSetInputFocus(fluorite.dpy, fluorite.ws[fluorite.cr_ws].t_wins->w, RevertToPointerRoot, CurrentTime);
		FRedrawWindows();
		XSync(fluorite.dpy, True);
		FApplyBorders();
		XChangeProperty(fluorite.dpy, fluorite.root, XInternAtom(fluorite.dpy, "_NET_DESKTOP_NAMES", False), XA_CARDINAL, 32, PropModeReplace, (unsigned char *)&fluorite.cr_ws, 1);
		FUpdateWorkarea();
		no_warp = False;
	}
}

static void FInitWorkspaces()
{
	for (int i = 0; i < MAX_WS; i++)
	{
		fluorite.ws[i].fs = False;
		fluorite.ws[i].layout = fluorite.conf.sl;
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
	Xutf8TextListToTextProperty(fluorite.dpy, (char **)workspaces_names, MAX_WS, XUTF8StringStyle, &text);
	XSetTextProperty(fluorite.dpy, fluorite.root, &text, XInternAtom(fluorite.dpy, "_NET_DESKTOP_NAMES", False));
	XChangeProperty(fluorite.dpy, fluorite.root, XInternAtom(fluorite.dpy, "_NET_CURRENT_DESKTOP", False), XA_CARDINAL, 32, PropModeReplace, (unsigned char *)&fluorite.cr_ws, 1);
	Atom supported[11] = {
		XInternAtom(fluorite.dpy, "_NET_WM_NAME", False),			XInternAtom(fluorite.dpy, "_NET_SUPPORTING_WM_CHECK", False),
		XInternAtom(fluorite.dpy, "_NET_ACTIVE_WINDOW", False),		XInternAtom(fluorite.dpy, "_NET_DESKTOP_NAMES", False),
		XInternAtom(fluorite.dpy, "_NET_CURRENT_DESKTOP", False),	XInternAtom(fluorite.dpy, "_NET_CLIENT_LIST", False),
		XInternAtom(fluorite.dpy, "_NET_WM_DESKTOP", False),		XInternAtom(fluorite.dpy, "_NET_NUMBER_OF_DESKTOPS", False),
		XInternAtom(fluorite.dpy, "_NET_WM_STATE", False),				XInternAtom(fluorite.dpy, "_NET_WORKAREA", False),
		XInternAtom(fluorite.dpy, "_NET_DESKTOP_VIEWPORT", False),
	};
	XChangeProperty(fluorite.dpy, fluorite.root, XInternAtom(fluorite.dpy, "_NET_SUPPORTED", False), XA_ATOM, 32, PropModeReplace, (unsigned char *)supported, 11);
	attributes.event_mask = SubstructureNotifyMask | SubstructureRedirectMask | StructureNotifyMask | ButtonPressMask | KeyPressMask | PointerMotionMask | PropertyChangeMask;
	XSelectInput(fluorite.dpy, fluorite.root, attributes.event_mask);
	cnorm = XcursorLibraryLoadCursor(fluorite.dpy, "arrow");
	cmove = XcursorLibraryLoadCursor(fluorite.dpy, "fleur");
	cresz = XcursorLibraryLoadCursor(fluorite.dpy, "bottom_right_corner");
	XDefineCursor(fluorite.dpy, fluorite.root, cnorm);
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
			for (i = 0; i < (unsigned int) binds_count; i++)
				if (binds[i].key == syms[(k - start) * skip])
					for (j = 0; j < LENGTH(modifiers); j++)
						XGrabKey(fluorite.dpy, k,
							 binds[i].mod | modifiers[j],
							 w, True,
							 GrabModeAsync, GrabModeAsync);
		XFree(syms);
	}
}

static void *FInotifyConfigAndXresources(void *useless)
{
	(void)useless;
	char *theme_path = getenv("HOME");
	char config_path[1024];
	const char *theme_file = ".Xresources";
	const char *config_file = "fluorite.conf";
	char buf[BUF_LEN];
	int fd;

	if ((fd = inotify_init1(IN_NONBLOCK)) < 0)
		return NULL;
	snprintf(config_path, sizeof(config_path), "%s/.config/fluorite", getenv("HOME"));
	inotify_add_watch(fd, theme_path, IN_CREATE | IN_MODIFY | IN_DELETE | IN_MOVED_FROM | IN_MOVED_TO);
	inotify_add_watch(fd, config_path, IN_CREATE | IN_MODIFY | IN_DELETE | IN_MOVED_FROM | IN_MOVED_TO);

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
			if (event->len > 0 && strcmp(event->name, theme_file) == 0)
					FReloadXresources();
			if (event->len > 0 && strcmp(event->name, config_file) == 0)
					FReloadConfig();
			i += EVENT_SIZE + event->len;
		}
	}

	close(fd);
	return NULL;
}

static void FReloadXresources()
{
	Window focused = fluorite.root;

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
		XSync(fluorite.dpy, True);
		FApplyBorders();
	}
	FChangeMonitor(keep_mon);
	for (Windows *w = fluorite.ws[fluorite.cr_ws].t_wins; w != NULL; w = w->next)
		if (w->fc)
			focused = w->w;
	for (Windows *w = fluorite.ws[fluorite.cr_ws].f_wins; w != NULL; w = w->next)
		if (w->fc)
			focused = w->w;
	if (fluorite.hpads != -1)
	{
		Scratchpads *p = fluorite.pads[fluorite.hpads];
		for (Windows *w = p->s_wins; w != NULL; w = w->next)
			if (w->fc)
				focused = w->w;
	}
	XSetInputFocus(fluorite.dpy, focused, RevertToPointerRoot, CurrentTime);
	FWarpCursor(focused);
	FApplyBorders();
	XSync(fluorite.dpy, True);
	FRedrawWindows();
}

static int FGetModifier(char *mod)
{
	if (strcasecmp(mod, "Mod4") == 0) return Mod4Mask;
	else if (strcasecmp(mod, "Mod3") == 0) return Mod3Mask;
	else if (strcasecmp(mod, "Mod2") == 0) return Mod2Mask;
	else if (strcasecmp(mod, "Mod1") == 0) return Mod1Mask;
	else if (strcasecmp(mod, "Alt") == 0) return Mod1Mask;
	else if (strcasecmp(mod, "Shift") == 0) return ShiftMask;
	else if (strcasecmp(mod, "Ctrl") == 0) return ControlMask;
	else return 0;
}

static void FParseModsAndKeys(cfg_t *user_bind, NeoBindings *b)
{
	char *bind_dup = strdup(cfg_title(user_bind));
	char *tok = strtok(bind_dup, "+");

	b->mod = 0;
	while (tok)
	{
		int mod = FGetModifier(tok);
		if (mod) b->mod |= mod;
		else b->key = XStringToKeysym(tok);
		tok = strtok(NULL, "+");
	}
	free(bind_dup);
}

static int FAssignBindAction(NeoBindings *cur, unsigned int mod, KeySym key, const char *action, const char *arg)
{
	for (unsigned int j = 0; j < LENGTH(user_functions_list); j++)
	{
		if (strcasecmp(action, user_functions_list[j].name) != 0)
			continue;

		cur->mod  = mod;
		cur->key  = key;
		cur->type = user_functions_list[j].type;

		if (strcasecmp(user_functions_list[j].name, "window_rotate") == 0 ||
				strcasecmp(user_functions_list[j].name, "stack_rotate") == 0 ||
				strcasecmp(user_functions_list[j].name, "change_master_offset") == 0)
		{
			cur->int_fun = user_functions_list[j].int_fun;
			if (strcasecmp(arg, "up") == 0) cur->int_arg = UP;
			else if (strcasecmp(arg, "down") == 0) cur->int_arg = DOWN;
			else cur->int_arg = UP;
		}
		else if (strcasecmp(user_functions_list[j].name, "change_layout") == 0)
		{
			cur->int_fun = user_functions_list[j].int_fun;
			if (strcasecmp(arg, "cascade") == 0) cur->int_arg = CASCADE;
			else if (strcasecmp(arg, "dwm") == 0) cur->int_arg = DWM;
			else if (strcasecmp(arg, "centered") == 0) cur->int_arg = CENTERED;
			else if (strcasecmp(arg, "stacked") == 0) cur->int_arg = STACKED;
			else cur->int_arg = CASCADE;
		}
		else
		{
			switch (user_functions_list[j].type)
			{
				case VOID:
					cur->void_fun = user_functions_list[j].void_fun;
					break;
				case INT:
					cur->int_fun = user_functions_list[j].int_fun;
					cur->int_arg = atoi(arg) - 1;
					if (cur->int_arg > 9) cur->int_arg = 9;
					else if (cur->int_arg < 0) cur->int_arg = 0;
					break;
				case CHAR:
					cur->char_fun = user_functions_list[j].char_fun;
					cur->char_arg = strdup(arg);
					break;
			}
		}
		return True;
	}
	return False;
}

static void FParseBindings(cfg_t *cfg)
{
	int old_binds = binds_count;
	int total = (int) cfg_size(cfg, "bind");

	for (int i = 0; i < (int) cfg_size(cfg, "multi_bind"); i++)
	{
		cfg_t *mb = cfg_getnsec(cfg, "multi_bind", i);
		total += (int) cfg_size(mb, "action");
	}

	NeoBindings *b = (NeoBindings *) calloc(total + 1, sizeof(NeoBindings));
	binds_count = 0;

	for (int i = 0; i < (int) cfg_size(cfg, "bind"); i++)
	{
		cfg_t *user_bind = cfg_getnsec(cfg, "bind", i);
		char *action = cfg_getstr(user_bind, "action");
		char *arg = cfg_getstr(user_bind, "arg");
		NeoBindings modkey = {0};

		FParseModsAndKeys(user_bind, &modkey);
		if (FAssignBindAction(&b[binds_count], modkey.mod, modkey.key, action, arg ? arg : ""))
			binds_count++;
	}

	for (int i = 0; i < (int) cfg_size(cfg, "multi_bind"); i++)
	{
		cfg_t *user_bind = cfg_getnsec(cfg, "multi_bind", i);
		unsigned int nb_actions = cfg_size(user_bind, "action");
		unsigned int nb_args    = cfg_size(user_bind, "arg");
		NeoBindings modkey = {0};

		FParseModsAndKeys(user_bind, &modkey);

		for (unsigned int a = 0; a < nb_actions; a++)
		{
			char *action = cfg_getnstr(user_bind, "action", a);
			char *arg = (a < nb_args) ? cfg_getnstr(user_bind, "arg", a) : "";

			if (FAssignBindAction(&b[binds_count], modkey.mod, modkey.key, action, arg))
				binds_count++;
		}
	}

	if (binds)
	{
		for (int i = 0; i < old_binds; i++)
			if (binds[i].type == CHAR && binds[i].char_arg)
				free(binds[i].char_arg);
		free(binds);
	}
	binds = b;
}

static void FReloadConfig()
{
	char path[1024];
	char *sl = strdup("cascade");
	char wn;
	char dfl;
	char dfx;
	char dsw;
	char mt = 0;
	XTextProperty text;

	FLoadDefaultConfig();

	cfg_opt_t bopts[] = {
		CFG_STR("action", NULL, CFGF_NONE),
		CFG_STR("arg", NULL, CFGF_NONE),
		CFG_END()
	};

	cfg_opt_t mopts[] = {
		CFG_STR_LIST("action", "{}", CFGF_NONE),
		CFG_STR_LIST("arg", "{}", CFGF_NONE),
		CFG_END()
	};

	cfg_opt_t opts[] = {
		CFG_STR("meta_key", "Mod4", mt),
		CFG_SIMPLE_BOOL("follow_windows", &fluorite.conf.fw),
		CFG_SIMPLE_BOOL("force_floating", &fluorite.conf.ff),
		CFG_SIMPLE_STR("starting_layout", &sl),
		CFG_SIMPLE_BOOL("warp_cursor", &fluorite.conf.wc),
		CFG_SIMPLE_BOOL("jump_to_urgent", &fluorite.conf.jtu),
		CFG_STR_LIST("workspaces_names", "{ 1, 2, 3, 4, 5, 6, 7, 8, 9, 0 }", wn),
		CFG_STR_LIST("floating_windows", "", dfl),
		CFG_STR_LIST("fixed_windows", "", dfx),
		CFG_STR_LIST("swallowing_windows", "", dsw),
		CFG_SEC("bind", bopts, CFGF_MULTI | CFGF_TITLE),
		CFG_SEC("multi_bind", mopts, CFGF_MULTI | CFGF_TITLE),
		CFG_END()
	};
	cfg_t *cfg;

	cfg = cfg_init(opts, 0);
	snprintf(path, sizeof(path), "%s/.config/fluorite/fluorite.conf", getenv("HOME"));
	cfg_parse(cfg, path);

	fluorite.conf.mt = FGetModifier(cfg_getstr(cfg, "meta_key"));
	if (!fluorite.conf.mt)
		fluorite.conf.mt = Mod4Mask;

	if (strcasecmp(sl, "Cascade") == 0)
		fluorite.conf.sl = CASCADE;
	else if (strcasecmp(sl, "DWM") == 0)
		fluorite.conf.sl = DWM;
	else if (strcasecmp(sl, "Centered") == 0)
		fluorite.conf.sl = CENTERED;
	else if (strcasecmp(sl, "Stacked") == 0)
		fluorite.conf.sl = STACKED;
	else if (strcasecmp(sl, "Scrolling") == 0)
		fluorite.conf.sl = SCROLLING;
	free(sl);

	for (int i = 0; i < (int) cfg_size(cfg, "workspaces_names"); i++)
	{
		if (workspaces_names[i])
			free(workspaces_names[i]);
		workspaces_names[i] = strdup(cfg_getnstr(cfg, "workspaces_names", i));
	}
	Xutf8TextListToTextProperty(fluorite.dpy, (char **)workspaces_names, MAX_WS, XUTF8StringStyle, &text);
	XSetTextProperty(fluorite.dpy, fluorite.root, &text, XInternAtom(fluorite.dpy, "_NET_DESKTOP_NAMES", False));
	XFree(text.value);

	if (floating_windows)
	{
		for (int i = 0; floating_windows[i]; i++)
			free(floating_windows[i]);
		free(floating_windows);
	}
	floating_windows = (char **) calloc(cfg_size(cfg, "floating_windows") + 1, sizeof(char *));
	for (int i = 0; i < (int) cfg_size(cfg, "floating_windows"); i++)
		floating_windows[i] = strdup(cfg_getnstr(cfg, "floating_windows", i));

	if (fixed_windows)
	{
		for (int i = 0; fixed_windows[i]; i++)
			free(fixed_windows[i]);
		free(fixed_windows);
	}
	fixed_windows = (char **) calloc(cfg_size(cfg, "fixed_windows") + 1, sizeof(char *));
	for (int i = 0; i < (int) cfg_size(cfg, "fixed_windows"); i++)
		fixed_windows[i] = strdup(cfg_getnstr(cfg, "fixed_windows", i));

	if (swallowing_windows)
	{
		for (int i = 0; swallowing_windows[i]; i++)
			free(swallowing_windows[i]);
		free(swallowing_windows);
	}
	swallowing_windows = (char **) calloc(cfg_size(cfg, "swallowing_windows") + 1, sizeof(char *));
	for (int i = 0; i < (int) cfg_size(cfg, "swallowing_windows"); i++)
		swallowing_windows[i] = strdup(cfg_getnstr(cfg, "swallowing_windows", i));

	for (int i = 0; i < MAX_WS; i++)
	{
		if (!fluorite.ws[i].t_wins && !fluorite.ws[i].f_wins) { fluorite.ws[i].layout = fluorite.conf.sl; continue; }
		for (Windows *w = fluorite.ws[i].t_wins; w != NULL; w = w->next)
			w->can_sw = FCheckCanSwallow(w->w);
		for (Windows *w = fluorite.ws[i].f_wins; w != NULL; w = w->next)
			w->can_sw = FCheckCanSwallow(w->w);
	}

	FParseBindings(cfg);
	XUngrabKey(fluorite.dpy, AnyKey, AnyModifier, fluorite.root);
	FGrabKeys(fluorite.root);

	FRedrawWindows();
	XSync(fluorite.dpy, True);
	FApplyBorders();
	cfg_free(cfg);
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
			case ButtonRelease:
				for (Windows *w = fluorite.ws[fluorite.cr_ws].f_wins; w != NULL; w = w->next)
					XDefineCursor(fluorite.dpy, w->w, cnorm);
				if (fluorite.hpads != -1)
					for (Windows *w = fluorite.pads[fluorite.hpads]->s_wins; w != NULL; w = w->next)
						XDefineCursor(fluorite.dpy, w->w, cnorm);
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
				if (ev.xcrossing.mode != NotifyNormal || ev.xcrossing.detail == NotifyInferior)
					break;
				FGetMonitorFromMouse();
				FFocusWindowUnderCursor();
				break;
			case ClientMessage:
				FClientMessage(ev);
				break;
			case DestroyNotify:
				FDestroyNotify(ev);
				break;
			case PropertyNotify:
				if (ev.xproperty.atom == XInternAtom(fluorite.dpy, "_NET_WM_STRUT_PARTIAL", False))
					for (int i = 0; i < fluorite.ct_mon; i++)
					{
						FResetMonitorStrut(i);
						FRecalculateStrut(i);
					}
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
				if (fluorite.conf.jtu)
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
				if (fluorite.conf.jtu)
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
		for (long unsigned int i = 0; swallowing_windows[i]; i++)
		{
			if (strcmp(swallowing_windows[i], name.res_class) == 0 ||
					strcmp(swallowing_windows[i], name.res_name) == 0)
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
	// TODO: Check if this cause issues with swallowing in the future
	// if (fluorite.ws[fluorite.mon[mon].ws].layout == DWM || fluorite.ws[fluorite.mon[mon].ws].layout == CENTERED)
	// 	XRaiseWindow(fluorite.dpy, w->w);
	// else if (w == fluorite.ws[fluorite.mon[mon].ws].t_wins || w == fluorite.ws[fluorite.mon[mon].ws].t_wins->next)
	// 	XRaiseWindow(fluorite.dpy, w->w);
	// else
	// {
	// 	XLowerWindow(fluorite.dpy, w->w);
	// 	if (fluorite.cr_ws == fluorite.mon[mon].ws)
	// 		FRedrawCascadeLayout();
	// }
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

static void FSetWindowState(Window w, long state)
{
	long data[] = { state, None };

	XChangeProperty(
		fluorite.dpy,
		w,
		XInternAtom(fluorite.dpy, "WM_STATE", False),
		XInternAtom(fluorite.dpy, "WM_STATE", False),
		32,
		PropModeReplace,
		(unsigned char *) data,
		2
	);
}

static void FMapRequest(XEvent ev)
{
	if (fluorite.orgz) FToggleOrganizer();

	Windows *nw = (Windows *) calloc(1, sizeof(Windows));
	XWindowAttributes wa;
	int is_floating = False;
	int is_fixed = False;

	if (!XGetWindowAttributes(fluorite.dpy, ev.xmaprequest.window, &wa))
		goto freeing;

	if (FCheckWindowToplevel(ev.xmaprequest.window))
	{
		if (fluorite.conf.jtu)
			return free(nw);
		XWMHints *hints = XGetWMHints(fluorite.dpy, ev.xmaprequest.window);
		hints->flags |= XUrgencyHint;
		XSetWMHints(fluorite.dpy, ev.xmaprequest.window, hints);
		XChangeProperty(fluorite.dpy, fluorite.root, XA_WM_HINTS, XA_WM_HINTS, 32, PropModeAppend, (unsigned char *) NULL, 0);
		XFree(hints);
		goto freeing;
	}

	if (wa.override_redirect || wa.height <= 0 || wa.width <= 0)
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
	nw->swp = 50;
	XSelectInput(fluorite.dpy, nw->w, EnterWindowMask | FocusChangeMask | PropertyChangeMask | StructureNotifyMask | KeyPressMask);
	XGrabButton(fluorite.dpy, Button1, fluorite.conf.mt, nw->w, False, ButtonPressMask | ButtonReleaseMask | ButtonMotionMask, GrabModeAsync, GrabModeAsync, None, None);
	XGrabButton(fluorite.dpy, Button3, fluorite.conf.mt, nw->w, False, ButtonPressMask | ButtonReleaseMask | ButtonMotionMask, GrabModeAsync, GrabModeAsync, None, None);
	XGrabButton(fluorite.dpy, Button4, fluorite.conf.mt, nw->w, False, ButtonPressMask | ButtonReleaseMask | ButtonMotionMask, GrabModeAsync, GrabModeAsync, None, None);
	XGrabButton(fluorite.dpy, Button5, fluorite.conf.mt, nw->w, False, ButtonPressMask | ButtonReleaseMask | ButtonMotionMask, GrabModeAsync, GrabModeAsync, None, None);
	XMapWindow(fluorite.dpy, nw->w);
	XRaiseWindow(fluorite.dpy, nw->w);
	XChangeProperty(fluorite.dpy, nw->w, XInternAtom(fluorite.dpy, "_NET_WM_DESKTOP", False), XA_CARDINAL, 32, PropModeReplace, (unsigned char *)&fluorite.cr_ws, 1);

	if (!is_fixed && !is_floating)
		if (FCheckWindowNeedsSwallowing(nw))
			goto freeing;

	if (fluorite.ws[fluorite.cr_ws].fs && !is_floating)
		FToggleFullscreen();

	if (is_fixed)
	{
		XSelectInput(fluorite.dpy, nw->w, PropertyChangeMask | StructureNotifyMask);
		FGetFixedPartialStrut(nw->w, True);
		goto freeing;
	}

	for (Windows *w = fluorite.ws[fluorite.cr_ws].t_wins; w != NULL; w = w->next)
	{
		// TODO: check if this is a fix that can really be done
		// w->fc = False;
		XSetWindowBorder(fluorite.dpy, w->w, fluorite.conf.bu);
	}
	XSetInputFocus(fluorite.dpy, nw->w, RevertToPointerRoot, CurrentTime);
	XSetWindowBorderWidth(fluorite.dpy, nw->w, fluorite.conf.bw);

	if (is_floating || fluorite.conf.ff)
		FManageFloatingWindow(nw);
	else
	{
		if (fluorite.ws[fluorite.cr_ws].layout == SCROLLING)
			fluorite.ws[fluorite.cr_ws].t_wins = FAddWindowScrolling(fluorite.ws[fluorite.cr_ws].t_wins, nw);
		else 
			fluorite.ws[fluorite.cr_ws].t_wins = FAddWindow(fluorite.ws[fluorite.cr_ws].t_wins, nw);
	}

	FSetWindowState(nw->w, NormalState);
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

static int FCheckWindowIsTransientOrPopup(Window w)
{
	Window tr;
	if (XGetTransientForHint(fluorite.dpy, w, &tr))
		return True;

	Atom da, atom = None;
	int di;
	unsigned long nitems, bytes_after;
	unsigned char *p = NULL;

	if (XGetWindowProperty(fluorite.dpy, w, XInternAtom(fluorite.dpy, "_NET_WM_WINDOW_TYPE", False), 0L, sizeof(atom), False, XA_ATOM, &da, &di, &nitems, &bytes_after, &p) == Success && p)
	{
		atom = *(Atom *) p;
		XFree(p);
		p = NULL;
		if (atom == XInternAtom(fluorite.dpy, "_NET_WM_WINDOW_TYPE_DIALOG", False) ||
		    atom == XInternAtom(fluorite.dpy, "_NET_WM_WINDOW_TYPE_UTILITY", False) ||
		    atom == XInternAtom(fluorite.dpy, "_NET_WM_WINDOW_TYPE_POPUP_MENU", False) ||
		    atom == XInternAtom(fluorite.dpy, "_NET_WM_WINDOW_TYPE_TOOLBAR", False) ||
		    atom == XInternAtom(fluorite.dpy, "_NET_WM_WINDOW_TYPE_SPLASH", False))
			return True;
	}

	if (XGetWindowProperty(fluorite.dpy, w, XInternAtom(fluorite.dpy, "_NET_WM_STATE", False), 0L, 1024L, False, XA_ATOM, &da, &di, &nitems, &bytes_after, &p) == Success && p)
	{
		Atom *atoms = (Atom *) p;
		Atom skip_taskbar = XInternAtom(fluorite.dpy, "_NET_WM_STATE_SKIP_TASKBAR", False);
		for (unsigned long i = 0; i < nitems; i++)
		{
			if (atoms[i] == skip_taskbar)
			{
				XFree(p);
				return True;
			}
		}
		XFree(p);
	}

	return False;
}

static void FManageFloatingWindow(Windows *nw)
{
	unsigned int ww, wh;
	Windows *cw = fluorite.ws[fluorite.cr_ws].f_wins;
	XSizeHints hints;
	long msize;
	int custom_pos = False;

	xdo_get_window_size(fluorite.xdo, nw->w, &ww, &wh);

	if (XGetWMNormalHints(fluorite.dpy, nw->w, &hints, &msize))
	{
		if (hints.flags & USPosition)
		{
			nw->wx = hints.x;
			nw->wy = hints.y;
			custom_pos = True;
		}
		else if ((hints.flags & PPosition) && FCheckWindowIsTransientOrPopup(nw->w))
		{
			nw->wx = hints.x;
			nw->wy = hints.y;
			custom_pos = True;
		}
	}

	if (custom_pos)
	{
		int mon_x = fluorite.mon[fluorite.cr_mon].mx;
		int mon_y = fluorite.mon[fluorite.cr_mon].my;
		int mon_w = fluorite.mon[fluorite.cr_mon].mw;
		int mon_h = fluorite.mon[fluorite.cr_mon].mh;

		if (nw->wx + (int)ww > mon_x + mon_w)
			nw->wx = mon_x + mon_w - ww;
		if (nw->wx < mon_x)
			nw->wx = mon_x;
		if (nw->wy + (int)wh > mon_y + mon_h)
			nw->wy = mon_y + mon_h - wh;
		if (nw->wy < mon_y)
			nw->wy = mon_y;
	}
	else
	{
		nw->wx = fluorite.mon[fluorite.cr_mon].mx + (fluorite.mon[fluorite.cr_mon].mw  - ww) / 2;
		nw->wy = fluorite.mon[fluorite.cr_mon].my + (fluorite.mon[fluorite.cr_mon].mh  - wh) / 2;
	}
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

    if (n == 0)
        return;

    int mo = fluorite.ws[fluorite.cr_ws].mo;
    int igp = fluorite.conf.igp;
    int ogp = fluorite.conf.ogp;
    int bw = fluorite.conf.bw;
    int mon_x = fluorite.mon[fluorite.cr_mon].mx;
    int mon_y = fluorite.mon[fluorite.cr_mon].my;
    int mon_w = fluorite.mon[fluorite.cr_mon].mw;
    int mon_h = fluorite.mon[fluorite.cr_mon].mh;

    int content_x = mon_x + fluorite.mon[fluorite.cr_mon].sl + ogp;
    int content_y = mon_y + fluorite.mon[fluorite.cr_mon].st + ogp;
    int content_w = mon_w - (fluorite.mon[fluorite.cr_mon].sl + fluorite.mon[fluorite.cr_mon].sr) - 2 * ogp;
    int content_h = mon_h - (fluorite.mon[fluorite.cr_mon].st + fluorite.mon[fluorite.cr_mon].sb) - 2 * ogp;

    w = fluorite.ws[fluorite.cr_ws].t_wins;

    if (n == 1)
    {
        w->wx = content_x;
        w->wy = content_y;
        w->ww = content_w - 2 * bw;
        w->wh = content_h - 2 * bw;
        XMoveResizeWindow(fluorite.dpy, w->w, w->wx, w->wy, w->ww, w->wh);
        return;
    }

    int stack_n = n - 1;
    int left_n = (stack_n + 1) / 2;
    int right_n = stack_n / 2;

    int base_mw = content_w / 2;
    int mw = base_mw + mo;
    if (mw > content_w - 2 * igp)
        mw = content_w - 2 * igp;
    if (mw < 100)
        mw = 100;

    int total_sw = content_w - mw - 2 * igp;
    int sw = total_sw / 2;
    int mx = content_x + sw + igp;

    w->wx = mx;
    w->wy = content_y;
    w->ww = mw - 2 * bw;
    w->wh = content_h - 2 * bw;
    XMoveResizeWindow(fluorite.dpy, w->w, w->wx, w->wy, w->ww, w->wh);

    int total_gap_left = (left_n - 1) * igp;
    int total_gap_right = (right_n - 1) * igp;
    int left_h = (left_n > 0) ? (content_h - total_gap_left) / left_n : content_h;
    int right_h = (right_n > 0) ? (content_h - total_gap_right) / right_n : content_h;
    int left_y = content_y;
    int right_y = content_y;

    w = w->next;
    int i = 0;
    Windows *last_left = NULL, *last_right = NULL;

    while (w)
    {
        if (i % 2 == 0)
        {
            w->wx = content_x;
            w->wy = left_y;
            w->ww = sw - 2 * bw;
            w->wh = left_h - 2 * bw;
            XMoveResizeWindow(fluorite.dpy, w->w, w->wx, w->wy, w->ww, w->wh);
            last_left = w;
            left_y += left_h + igp;
        }
        else
        {
            w->wx = mx + mw + igp;
            w->wy = right_y;
            w->ww = sw - 2 * bw;
            w->wh = right_h - 2 * bw;
            XMoveResizeWindow(fluorite.dpy, w->w, w->wx, w->wy, w->ww, w->wh);
            last_right = w;
            right_y += right_h + igp;
        }
        i++;
        w = w->next;
    }

    if (last_left)
    {
        int extra = (content_y + content_h) - (last_left->wy + last_left->wh + 2 * bw);
        if (extra != 0)
        {
            last_left->wh += extra;
            XResizeWindow(fluorite.dpy, last_left->w, last_left->ww, last_left->wh);
        }
    }

    if (last_right)
    {
        int extra = (content_y + content_h) - (last_right->wy + last_right->wh + 2 * bw);
        if (extra != 0)
        {
            last_right->wh += extra;
            XResizeWindow(fluorite.dpy, last_right->w, last_right->ww, last_right->wh);
        }
    }
}

static void FRedrawStackedLayout()
{
    Windows *raise = NULL;
    int ogp = fluorite.conf.ogp;
    int bw = fluorite.conf.bw;
    Monitors *m = &fluorite.mon[fluorite.cr_mon];

    int usable_x = m->mx + m->sl + ogp;
    int usable_y = m->my + m->st + ogp;
    int usable_w = m->mw - (m->sl + m->sr) - 2 * ogp - 2 * bw;
    int usable_h = m->mh - (m->st + m->sb) - 2 * ogp - 2 * bw;

    for (Windows *w = fluorite.ws[fluorite.cr_ws].t_wins; w != NULL; w = w->next)
    {
        w->wx = usable_x;
        w->wy = usable_y;
        w->ww = usable_w;
        w->wh = usable_h;
        XMoveResizeWindow(fluorite.dpy, w->w, w->wx, w->wy, w->ww, w->wh);
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
    int igp = fluorite.conf.igp;
    int ogp = fluorite.conf.ogp;
    int border = fluorite.conf.bw;

    int n = 0;
    for (Windows *w = win; w; w = w->next)
        n++;

    if (n == 0)
        return;

    int usable_x = mx + fluorite.mon[mon].sl + ogp;
    int usable_y = my + fluorite.mon[mon].st + ogp;
    int usable_w = mw - (fluorite.mon[mon].sl + fluorite.mon[mon].sr) - 2 * ogp;
    int usable_h = mh - (fluorite.mon[mon].st + fluorite.mon[mon].sb) - 2 * ogp;

    if (n == 1)
    {
        win->wx = usable_x;
        win->wy = usable_y;
        win->ww = usable_w - 2 * border;
        win->wh = usable_h - 2 * border;
        XMoveResizeWindow(fluorite.dpy, win->w, win->wx, win->wy, win->ww, win->wh);
        return;
    }

    int master_width = (usable_w - igp) / 2 + mo;
    int stack_width  = usable_w - igp - master_width;

    win->wx = usable_x;
    win->wy = usable_y;
    win->ww = master_width - 2 * border;
    win->wh = usable_h - 2 * border;
    XMoveResizeWindow(fluorite.dpy, win->w, win->wx, win->wy, win->ww, win->wh);
    win = win->next;

    int sn = n - 1;
    int total_stack_igp = (sn - 1) * igp;
    int sh_each = (usable_h - total_stack_igp) / sn;

    for (int i = 0; win; win = win->next, i++)
    {
        win->wx = usable_x + master_width + igp;
        win->wy = usable_y + i * (sh_each + igp);
        win->ww = stack_width - 2 * border;
        win->wh = sh_each - 2 * border;
        XMoveResizeWindow(fluorite.dpy, win->w, win->wx, win->wy, win->ww, win->wh);
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
    int igp = fluorite.conf.igp;
    int ogp = fluorite.conf.ogp;
    int bw = fluorite.conf.bw;
    int mo = fluorite.ws[fluorite.cr_ws].mo;

    for (Windows *w = fluorite.ws[fluorite.cr_ws].t_wins->next; w != NULL; w = w->next, stack_count++)
        last = w;

    int usable_y = my + fluorite.mon[fluorite.cr_mon].st + ogp;
    int usable_h = mh - (fluorite.mon[fluorite.cr_mon].st + fluorite.mon[fluorite.cr_mon].sb) - 2 * ogp;
    int usable_w = mw - (fluorite.mon[fluorite.cr_mon].sl + fluorite.mon[fluorite.cr_mon].sr) - 2 * ogp;

    int master_w = (usable_w - igp) / 2 + mo;
    int stack_w  = usable_w - igp - master_w;

    if (stack_count > 0)
    {
        size_offset = (stack_count - 1) * fluorite.conf.so * 10;

        for (int i = stack_count - 1; i >= 0; i--, last = last->prev)
        {
            int p_off = position_offset / stack_count;
            int s_off = size_offset / stack_count;

            last->wx = mx + ogp + fluorite.mon[fluorite.cr_mon].sl + master_w + igp + p_off;
            last->wy = usable_y + p_off;
            last->ww = stack_w - (bw * 2) - s_off;
            last->wh = usable_h - (bw * 2) - s_off;

            XRaiseWindow(fluorite.dpy, last->w);
            XMoveResizeWindow(fluorite.dpy, last->w,
                last->wx, last->wy,
                last->ww, last->wh
            );
            position_offset += fluorite.conf.so * 10;
        }
    }

    Windows *master = fluorite.ws[fluorite.cr_ws].t_wins;
    master->wx = mx + ogp + fluorite.mon[fluorite.cr_mon].sl;
    master->wy = usable_y;
    master->ww = (stack_count > 0 ? master_w : usable_w) - (bw * 2);
    master->wh = usable_h - (bw * 2);

    XRaiseWindow(fluorite.dpy, master->w);
    XMoveResizeWindow(fluorite.dpy, master->w,
        master->wx, master->wy,
        master->ww, master->wh
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

static void FRaiseAboveWindows(void)
{
	Window root_ret, parent_ret, *children = NULL;
	unsigned int nchildren = 0;

	if (!XQueryTree(fluorite.dpy, fluorite.root, &root_ret, &parent_ret, &children, &nchildren) || !children)
		return;

	Atom net_wm_state = XInternAtom(fluorite.dpy, "_NET_WM_STATE", False);
	Atom net_wm_state_above = XInternAtom(fluorite.dpy, "_NET_WM_STATE_ABOVE", False);
	Atom net_wm_state_stays_on_top = XInternAtom(fluorite.dpy, "_NET_WM_STATE_STAYS_ON_TOP", False);

	for (unsigned int i = 0; i < nchildren; i++)
	{
		Atom actual_type;
		int actual_format;
		unsigned long nitems, bytes_after;
		unsigned char *prop = NULL;

		if (XGetWindowProperty(fluorite.dpy, children[i], net_wm_state, 0L, 1024L, False, XA_ATOM,
				&actual_type, &actual_format, &nitems, &bytes_after, &prop) == Success && prop)
		{
			Atom *atoms = (Atom *) prop;
			for (unsigned long j = 0; j < nitems; j++)
			{
				if (atoms[j] == net_wm_state_above || atoms[j] == net_wm_state_stays_on_top)
				{
					XRaiseWindow(fluorite.dpy, children[i]);
					break;
				}
			}
			XFree(prop);
		}
	}
	XFree(children);
}

static void FRedrawWindows()
{
	if (no_redraw) return;
	if (fluorite.orgz)
	{
		FRedrawOrganizer();
		return;
	}
	FSearchAndDestoryGhostWindows();
	for (int i = 0; i < fluorite.ct_mon; i++)
		FRecalculateStrut(i);
	FPolybarLayoutIPC(fluorite.ws[fluorite.cr_ws].layout);

	if (!fluorite.ws[fluorite.cr_ws].t_wins)
		goto floating;

	no_refocus = True;

	if ((fluorite.ws[fluorite.cr_ws].t_wins && !fluorite.ws[fluorite.cr_ws].t_wins->next) || fluorite.ws[fluorite.cr_ws].layout == STACKED)
		FRedrawStackedLayout();
	else if (fluorite.ws[fluorite.cr_ws].layout == DWM)
		FRedrawDWMLayout();
	else if (fluorite.ws[fluorite.cr_ws].layout == CENTERED && fluorite.ws[fluorite.cr_ws].t_wins->next->next)
		FRedrawCenteredMaster();
	else if (fluorite.ws[fluorite.cr_ws].layout == SCROLLING)
		FRedrawScrolling();
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
	FRaiseAboveWindows();

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
		{
			if (!fluorite.ws[fluorite.cr_ws].fl_hdn)
				FApplyActiveWindow(focused);
			else
			{
				w->fc = False;
				if (fluorite.ws[fluorite.cr_ws].t_wins)
				{
					fluorite.ws[fluorite.cr_ws].t_wins->fc = True;
					break;
				}
			}
		}
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
		for (long unsigned int i = 0; floating_windows[i]; i++)
		{
			if (strcmp(floating_windows[i], name.res_class) == 0 ||
					strcmp(floating_windows[i], name.res_name) == 0)
			{
				XFree(name.res_name);
				XFree(name.res_class);
				return True;
			}
		}
		XFree(name.res_name);
		XFree(name.res_class);
	}

	XTextProperty text_prop;
	if (XGetTextProperty(fluorite.dpy, w, &text_prop, XInternAtom(fluorite.dpy, "_NET_WM_NAME", False)) && text_prop.value)
	{
		for (long unsigned int i = 0; floating_windows[i]; i++)
		{
			if (strcmp(floating_windows[i], (char *) text_prop.value) == 0)
			{
				XFree(text_prop.value);
				return True;
			}
		}
		XFree(text_prop.value);
	}

	char *win_name = NULL;
	if (XFetchName(fluorite.dpy, w, &win_name) && win_name)
	{
		for (long unsigned int i = 0; floating_windows[i]; i++)
		{
			if (strcmp(floating_windows[i], win_name) == 0)
			{
				XFree(win_name);
				return True;
			}
		}
		XFree(win_name);
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

	if (fluorite.conf.ff)
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
		for (long unsigned int i = 0; fixed_windows[i]; i++)
		{
			if (strcmp(fixed_windows[i], name.res_class) == 0 || strcmp(fixed_windows[i], name.res_name) == 0)
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
		// TODO: Check if this doesn't break anything, it's a discord updated fix
		// Seems like it causes some issues with authentification dialog where you type your password, it opens on 0 0
		// After check, removing this helps with Discord, but breaks games
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
	// TODO: Check if that helps or not.
	// TODO: If it helps check with warp_cursor

	FFocusWindowUnderCursor();
	no_warp = True;
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
		{
			if (ev.xconfigurerequest.window == w->w)
			{
				if (ev.xconfigurerequest.value_mask & CWX)
					w->wx = ev.xconfigurerequest.x;
				if (ev.xconfigurerequest.value_mask & CWY)
					w->wy = ev.xconfigurerequest.y;
				if (ev.xconfigurerequest.value_mask & CWWidth)
					w->ww = ev.xconfigurerequest.width;
				if (ev.xconfigurerequest.value_mask & CWHeight)
					w->wh = ev.xconfigurerequest.height;
				break;
			}
		}
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

	for (long unsigned int i = 0; i < (long unsigned int) binds_count; i++)
	{
		switch (binds[i].type)
		{
			case INT:
				if (e.keycode == XKeysymToKeycode(fluorite.dpy, binds[i].key) && MODMASK(binds[i].mod) == MODMASK(e.state) && binds[i].int_fun)
					binds[i].int_fun(binds[i].int_arg);
				break;
			case CHAR:
				if (e.keycode == XKeysymToKeycode(fluorite.dpy, binds[i].key) && MODMASK(binds[i].mod) == MODMASK(e.state) && binds[i].char_fun && binds[i].char_arg)
					binds[i].char_fun(binds[i].char_arg);
				break;
			case VOID:
				if (e.keycode == XKeysymToKeycode(fluorite.dpy, binds[i].key) && MODMASK(binds[i].mod) == MODMASK(e.state) && binds[i].void_fun)
					binds[i].void_fun();
				break;
			default:
				break;
		}
	}
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

	if (no_unmap) return;
	no_redraw = True;
	if (fluorite.orgz) FToggleOrganizer();
	no_redraw = False;
	if (fluorite.hpads == -1) goto next;

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
			{
				fluorite.ws[ws].fs = False;
				FSetWindowFullscreen(w->w, 0);
			}
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
			{
				fluorite.ws[ws].fs = False;
				FSetWindowFullscreen(w->w, 0);
			}
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

	if (fluorite.orgz) FToggleOrganizer();

	for (int i = 0; i < fluorite.ct_mon; i++)
	{
		for (Windows *fx = fluorite.mon[i].fx_win; fx != NULL; fx = fx->next)
		{
			if (fx->w != ev.xdestroywindow.window)
				continue;
			fluorite.mon[i].fx_win = FDelWindow(fluorite.mon[i].fx_win, fx);
			fx->next = NULL;
			fx->prev = NULL;
			FResetMonitorStrut(i);
			FRecalculateStrut(i);
			goto update;
		}
	}

	for (int i = 0; i < HASH_SIZE; i++)
	{
		if (!fluorite.pads[i])
			continue;
		for (Windows *w = fluorite.pads[i]->s_wins; w != NULL; w = w->next)
		{
			if (w->w != ev.xdestroywindow.window)
				continue;
			fluorite.pads[i]->s_wins = FDelWindow(fluorite.pads[i]->s_wins, w);
			w->next = NULL;
			w->prev = NULL;
			fluorite.ws[fluorite.cr_ws].t_wins = FAddWindow(fluorite.ws[fluorite.cr_ws].t_wins, w);
			goto update;
		}
	}

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
			{
				fluorite.ws[ws].fs = False;
				FSetWindowFullscreen(w->w, 0);
			}
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
			{
				fluorite.ws[ws].fs = False;
				FSetWindowFullscreen(w->w, 0);
			}
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
	if (w->prev && w->prev->stk_blw)
	{
		if (!w->stk_blw)
			w->prev->stk_blw = 0;
	}

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

static void FExecute(char *argument)
{
	if (fluorite.orgz) return;
	char *prepared_cmd = strdup(argument);
	strcat(prepared_cmd, " &");
	if (system(prepared_cmd) == -1)
		printf("Failed to execute program\n");
}

static void FQuit()
{
	free(fluorite.mon);
	xdo_free(fluorite.xdo);
	XCloseDisplay(fluorite.dpy);
}

static void FCloseWindow()
{
    if (fluorite.orgz) return;

    Window focused;
    int revert;
    XGetInputFocus(fluorite.dpy, &focused, &revert);
    if (focused == fluorite.root || focused == None)
        return;

    Window target = FGetToplevel(focused);
    if (target == None || target == fluorite.root)
        target = focused;

    Atom wm_protocols = XInternAtom(fluorite.dpy, "WM_PROTOCOLS", False);
    Atom wm_delete = XInternAtom(fluorite.dpy, "WM_DELETE_WINDOW", False);

    Atom *protocols;
    int n;
    if (XGetWMProtocols(fluorite.dpy, target, &protocols, &n))
    {
        for (int i = 0; i < n; i++)
        {
            if (protocols[i] == wm_delete)
            {
                XEvent ev;
                memset(&ev, 0, sizeof(ev));
                ev.xclient.type = ClientMessage;
                ev.xclient.window = target;
                ev.xclient.message_type = wm_protocols;
                ev.xclient.format = 32;
                ev.xclient.data.l[0] = wm_delete;
                ev.xclient.data.l[1] = CurrentTime;
                XSendEvent(fluorite.dpy, target, False, NoEventMask, &ev);
                XFree(protocols);
                return;
            }
        }
        XFree(protocols);
    }

    XWithdrawWindow(fluorite.dpy, target, fluorite.scr);
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
	if (fluorite.orgz) return;

	XWindowAttributes wa;
	if (XGetWindowAttributes(fluorite.dpy, ev.xbutton.window, &wa) && wa.override_redirect)
        return;

	unsigned b_w, d;
	unsigned xdo_w, xdo_h;
	Screen *scr;
	Window target;
	static Time lst = 0;

	if (fluorite.ws[fluorite.cr_ws].fs)
		return ;

	if (ev.xbutton.state & fluorite.conf.mt && fluorite.ws[fluorite.cr_ws].layout == SCROLLING)
	{
		if (ev.xbutton.button == Button4 || ev.xbutton.button == Button5)
		{
			if (ev.xbutton.time - lst > 125)
			{
				if (ev.xbutton.button == Button4)
				{
					FScrollingFocusLeft();
					lst = ev.xbutton.time;
				}
				else if (ev.xbutton.button == Button5)
				{
					FScrollingFocusRight();
					lst = ev.xbutton.time;
				}
			}
			return;
		}
	}

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
	no_refocus = True;
	no_warp = True;
	if (ev.xclient.message_type == XInternAtom(fluorite.dpy, "_NET_CURRENT_DESKTOP", False))
	{
		int request_ws = ev.xclient.data.l[0];
		if (request_ws < 0 || request_ws > 9)
			return;
		FShowWorkspace((int)ev.xclient.data.l[0]);
	}
	else if (ev.xclient.message_type == XInternAtom(fluorite.dpy, "_NET_ACTIVE_WINDOW", False))
	{
		no_warp = False;
		for (Windows *w = fluorite.ws[fluorite.cr_ws].t_wins; w != NULL; w = w->next)
		{
			w->fc = 0;
			if (ev.xclient.window == w->w)
				w->fc = 1;
		}
		for (Windows *w = fluorite.ws[fluorite.cr_ws].f_wins; w != NULL; w = w->next)
		{
			w->fc = 0;
			if (ev.xclient.window == w->w)
				w->fc = 1;
		}
		XSetInputFocus(fluorite.dpy, ev.xclient.window, RevertToPointerRoot, CurrentTime);
		FRedrawWindows();
		FApplyBorders();
	}
	else if (ev.xclient.message_type == XInternAtom(fluorite.dpy, "_NET_WM_STATE", False))
	{
		Atom net_wm_state_fullscreen = XInternAtom(fluorite.dpy, "_NET_WM_STATE_FULLSCREEN", False);
		if ((Atom)ev.xclient.data.l[1] == net_wm_state_fullscreen || (Atom)ev.xclient.data.l[2] == net_wm_state_fullscreen)
		{
			// action: 0 = _NET_WM_STATE_REMOVE, 1 = _NET_WM_STATE_ADD, 2 = _NET_WM_STATE_TOGGLE
			int action = ev.xclient.data.l[0];
			Windows *target = NULL;
			for (target = fluorite.ws[fluorite.cr_ws].t_wins; target != NULL; target = target->next)
				if (target->w == ev.xclient.window) break;
			if (!target)
			{
				for (target = fluorite.ws[fluorite.cr_ws].f_wins; target != NULL; target = target->next)
					if (target->w == ev.xclient.window) break;
			}
			if (target)
			{
				if ((action == 1 && !target->fs) || (action == 0 && target->fs) || (action == 2))
				{
					XSetInputFocus(fluorite.dpy, target->w, RevertToPointerRoot, CurrentTime);
					FToggleFullscreen();
				}
			}
		}
	}
	no_warp = False;
	no_refocus = False;
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
		XDefineCursor(fluorite.dpy, target->w, cmove);
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
		XDefineCursor(fluorite.dpy, target->w, cresz);
		ddx = fluorite.mouse.sww + dpx;
		ddy = fluorite.mouse.swh + dpy;
		XResizeWindow(fluorite.dpy, target->w, ddx, ddy);
		target->ww = ddx;
		target->wh = ddy;
		hints.width = ddx;
		hints.height = ddy;
		goto pressed;
	}
	XDefineCursor(fluorite.dpy, target->w, cnorm);
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

	if (!fluorite.conf.wc || no_warp)
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

static void FSetWindowFullscreen(Window w, int fs)
{
	Atom net_wm_state = XInternAtom(fluorite.dpy, "_NET_WM_STATE", False);
	if (fs)
	{
		Atom net_wm_state_fullscreen = XInternAtom(fluorite.dpy, "_NET_WM_STATE_FULLSCREEN", False);
		XChangeProperty(fluorite.dpy, w, net_wm_state, XA_ATOM, 32,
				PropModeReplace, (unsigned char *)&net_wm_state_fullscreen, 1);
	}
	else
		XDeleteProperty(fluorite.dpy, w, net_wm_state);
}

static void FSwapMonitorWorkspace(int ws, int mon)
{
	int swap_mon = fluorite.cr_mon;

	for (Windows *w = fluorite.ws[fluorite.cr_ws].t_wins; w != NULL; w = w->next)
		XSetWindowBorder(fluorite.dpy, w->w, fluorite.conf.bu);
	for (Windows *w = fluorite.ws[fluorite.cr_ws].f_wins; w != NULL; w = w->next)
		XSetWindowBorder(fluorite.dpy, w->w, fluorite.conf.bu);

	FRemoveActiveWindow();
	fluorite.mon[mon].ws = fluorite.cr_ws;
	fluorite.cr_mon = mon;
	FRedrawWindows();
	fluorite.cr_mon = swap_mon;
	fluorite.cr_ws = ws;
	fluorite.mon[fluorite.cr_mon].ws = ws;
	XChangeProperty(fluorite.dpy, fluorite.root, XInternAtom(fluorite.dpy, "_NET_CURRENT_DESKTOP", False), XA_CARDINAL, 32, PropModeReplace, (unsigned char *)&fluorite.cr_ws, 1);
	XSetInputFocus(fluorite.dpy, fluorite.root, RevertToPointerRoot, CurrentTime);
	for (Windows *w = fluorite.ws[fluorite.cr_ws].t_wins; w != NULL; w = w->next)
		if (w->fc) { XSetInputFocus(fluorite.dpy, w->w, RevertToPointerRoot, CurrentTime); goto redraw; }
	for (Windows *w = fluorite.ws[fluorite.cr_ws].f_wins; w != NULL; w = w->next)
		if (w->fc) { XSetInputFocus(fluorite.dpy, w->w, RevertToPointerRoot, CurrentTime); goto redraw; }
redraw:
	FRedrawWindows();
	XSync(fluorite.dpy, True);
	FApplyBorders();
}

static void FShowWorkspace(int ws)
{
	if (ws == fluorite.cr_ws || fluorite.orgz)
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
	XSync(fluorite.dpy, True);
	FApplyBorders();
}

static void FSendWindowToWorkspace(int ws)
{
	Window focused;
	int revert;
	Windows *w;

	if (ws == fluorite.cr_ws || (!fluorite.ws[fluorite.cr_ws].t_wins && !fluorite.ws[fluorite.cr_ws].f_wins) || fluorite.orgz)
		return ;

	no_unmap = True;
	XGrabServer(fluorite.dpy);
	XGetInputFocus(fluorite.dpy, &focused, &revert);

	if (fluorite.ws[ws].fs)
	{
		fluorite.ws[ws].fs = False;
		for (Windows *w = fluorite.ws[ws].t_wins; w != NULL; w = w->next)
		{ FResetWindowOpacity(w->w); XSetWindowBorderWidth(fluorite.dpy, w->w, fluorite.conf.bw); FSetWindowFullscreen(w->w, 0); w->fs = False; }
		for (Windows *w = fluorite.ws[ws].f_wins; w != NULL; w = w->next)
		{ FResetWindowOpacity(w->w); XSetWindowBorderWidth(fluorite.dpy, w->w, fluorite.conf.bw); FSetWindowFullscreen(w->w, 0); w->fs = False; }
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
		FSetWindowFullscreen(w->w, 0);
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

	if (fluorite.conf.fw)
		FShowWorkspace(ws);
}

static void FNextWorkspace()
{
	if (fluorite.orgz) return;

	int ws = fluorite.cr_ws + 1;
	if (ws == MAX_WS)
		ws = 0;
	FShowWorkspace(ws);
}

static void FPrevWorkspace()
{
	if (fluorite.orgz) return;

	int ws = fluorite.cr_ws - 1;
	if (ws < 0)
		ws = MAX_WS - 1;
	FShowWorkspace(ws);
}

static void FRotateStackWindows(int mode)
{
	if (fluorite.ws[fluorite.cr_ws].fs || fluorite.orgz || fluorite.ws[fluorite.cr_ws].layout == SCROLLING)
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
	XSync(fluorite.dpy, True);
    FApplyBorders();
}


static void FRotateWindows(int mode)
{
	if (fluorite.ws[fluorite.cr_ws].fs || fluorite.ws[fluorite.cr_ws].layout == SCROLLING)
		return;

	Windows *first, *last;
	Window focused = FFindFocusedWindow();

	no_refocus = True;
	switch (mode)
	{
		case UP:
			if (!fluorite.ws[fluorite.cr_ws].t_wins || !fluorite.ws[fluorite.cr_ws].t_wins->next)
				return;
			if (fluorite.orgz)
			{
				for (Windows *w = fluorite.ws[fluorite.cr_ws].t_wins; w != NULL; w = w->next)
				{
					if (w->fc)
					{
						if (!w->prev)
						{
							Windows *l = fluorite.ws[fluorite.cr_ws].t_wins;
							for (; l->next != NULL; l = l->next);
							w->next->prev = NULL;
							fluorite.ws[fluorite.cr_ws].t_wins = w->next;
							w->next = NULL;
							l->next = w;
							w->prev = l;
							goto redraw;
						}
						Windows *prev = w->prev;
						Windows *next = w->next;
						Windows *n_prev = prev->prev;
						if (next)
							next->prev = prev;
						prev->next = next;
						w->prev = n_prev;
						w->next = prev;
						prev->prev = w;
						if (n_prev)
							n_prev->next = w;
						else
							fluorite.ws[fluorite.cr_ws].t_wins = w;
						XRaiseWindow(fluorite.dpy, w->w);
						goto redraw;
					}
				}
			}
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
			if (fluorite.orgz)
			{
				for (Windows *w = fluorite.ws[fluorite.cr_ws].t_wins; w != NULL; w = w->next)
				{
					if (w->fc)
					{
						if (!w->next)
						{
							w->prev->next = NULL;
							w->prev = NULL;
							fluorite.ws[fluorite.cr_ws].t_wins->prev = w;
							w->next = fluorite.ws[fluorite.cr_ws].t_wins;
							fluorite.ws[fluorite.cr_ws].t_wins = w;
							goto redraw;
						}
						Windows *next = w->next;
						Windows *prev = w->prev;
						Windows *n_next = next ? next->next : NULL;
						Windows *n_prev = next;
						next->prev = prev;
						if (prev)
							prev->next = next;
						else
							fluorite.ws[fluorite.cr_ws].t_wins = next;
						if (n_next)
							n_next->prev = w;
						w->next = n_next;
						w->prev = n_prev;
						n_prev->next = w;
						XLowerWindow(fluorite.dpy, w->w);
						goto redraw;
					}
				}
			}
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

redraw:
	FRedrawWindows();
	XSync(fluorite.dpy, True);
	FApplyBorders();
	no_refocus = False;
}


static void FChangeMasterOffset(int mode)
{
	if (fluorite.ws[fluorite.cr_ws].fs || FCountWindows(fluorite.ws[fluorite.cr_ws].t_wins) < 2 || fluorite.orgz || fluorite.ws[fluorite.cr_ws].layout == SCROLLING)
		return ;

	// I know it's reversed, don't ask why
	switch (mode)
	{
		case DOWN:
			fluorite.ws[fluorite.cr_ws].mo += 25;
			break;
		case UP:
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
static void FSwapWithMaster()
{
	if (fluorite.orgz) return;

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

static void FFocusNext()
{
	if (fluorite.ws[fluorite.cr_ws].layout == SCROLLING) return;

	if (!fluorite.ws[fluorite.cr_ws].t_wins ||
			!fluorite.ws[fluorite.cr_ws].t_wins->next ||
			fluorite.ws[fluorite.cr_ws].fs ||
			fluorite.ws[fluorite.cr_ws].layout == STACKED)
		return;

	if (fluorite.ws[fluorite.cr_ws].layout == CASCADE && !fluorite.orgz)
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
	if (fluorite.ws[fluorite.cr_ws].layout == SCROLLING)
	{
		FRedrawWindows();
		XSync(fluorite.dpy, True);
	}
}

static void FFocusPrev()
{
	if (fluorite.ws[fluorite.cr_ws].layout == SCROLLING) return;

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
				if (fluorite.ws[fluorite.cr_ws].layout == CASCADE && !fluorite.orgz)
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
	if (fluorite.ws[fluorite.cr_ws].layout == SCROLLING)
	{
		FRedrawWindows();
		XSync(fluorite.dpy, True);
	}
}

static void FTileWindow()
{
	if (fluorite.orgz) return;

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
			if (fluorite.ws[fluorite.cr_ws].layout == SCROLLING)
				fluorite.ws[fluorite.cr_ws].t_wins = FAddWindowScrolling(fluorite.ws[fluorite.cr_ws].t_wins, w);
			else
			{
				FResetFocus(fluorite.ws[fluorite.cr_ws].t_wins);
				fluorite.ws[fluorite.cr_ws].t_wins = FAddWindow(fluorite.ws[fluorite.cr_ws].t_wins, w);
			}
			w->fc = True;
			XSetInputFocus(fluorite.dpy, w->w, RevertToPointerRoot, CurrentTime);
			FApplyActiveWindow(w->w);
		}
	}
	FRedrawWindows();
	XSync(fluorite.dpy, True);
	FApplyBorders();
}

static void FTileAllWindows()
{
	if (fluorite.orgz) return;
	if (fluorite.ws[fluorite.cr_ws].f_wins == NULL) return;

	Windows *w;
	Windows *prev;

	if (fluorite.ws[fluorite.cr_ws].fs)
		return;

	for (w = fluorite.ws[fluorite.cr_ws].f_wins; w->next != NULL; w = w->next);
	prev = w->prev;
	if (fluorite.ws[fluorite.cr_ws].layout == SCROLLING)
		fluorite.ws[fluorite.cr_ws].t_wins = FAddWindowScrolling(fluorite.ws[fluorite.cr_ws].t_wins, w);
	else
		fluorite.ws[fluorite.cr_ws].t_wins = FAddWindow(fluorite.ws[fluorite.cr_ws].t_wins, w);
	fluorite.ws[fluorite.cr_ws].t_wins->fc = False;
	fluorite.ws[fluorite.cr_ws].t_wins->prev = prev;
	fluorite.ws[fluorite.cr_ws].t_wins = fluorite.ws[fluorite.cr_ws].f_wins;
	fluorite.ws[fluorite.cr_ws].f_wins = NULL;
	FResetFocus(fluorite.ws[fluorite.cr_ws].t_wins);
	fluorite.ws[fluorite.cr_ws].t_wins->fc = True;

	FRedrawWindows();
	XSync(fluorite.dpy, True);
	FApplyBorders();
}

static void FChangeLayout(int layout)
{
	if (fluorite.ws[fluorite.cr_ws].fs || fluorite.orgz)
		return;

	if (fluorite.ws[fluorite.cr_ws].layout == layout)
		fluorite.ws[fluorite.cr_ws].layout = fluorite.conf.sl;
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

static void FToggleFullscreen()
{
	if (fluorite.orgz) return;

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
	FSetWindowFullscreen(w->w, w->fs);
	FRedrawWindows();
	XSync(fluorite.dpy, True);
	FApplyBorders();
	FWarpCursor(w->w);

exit:
	return ;
}

static void FFloatingHideShow()
{
	if (fluorite.ws[fluorite.cr_ws].fs || fluorite.orgz)
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

static void FSendWindowToNextWorkspace()
{
	if (fluorite.orgz) return;

	int ws = fluorite.cr_ws + 1;
	if (ws == MAX_WS)
		ws = 0;
	FSendWindowToWorkspace(ws);
}

static void FSendWindowToPrevWorkspace()
{
	if (fluorite.orgz) return;

	int ws = fluorite.cr_ws - 1;
	if (ws < 0)
		ws = MAX_WS - 1;
	FSendWindowToWorkspace(ws);
}

static void FFocusNextMonitor()
{
	if (fluorite.orgz) return;

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

static void FResetMasterOffset()
{
	if (fluorite.ws[fluorite.cr_ws].fs || FCountWindows(fluorite.ws[fluorite.cr_ws].t_wins) < 2 || fluorite.orgz)
		return ;
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

static void FAddWindowToScratchpad()
{
	if (fluorite.orgz) return;

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
		FSetWindowFullscreen(w->w, 0);
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

static void FDelWindowFromScratchpad()
{
	if (fluorite.hpads == -1 || fluorite.orgz)
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
		if (fluorite.ws[fluorite.cr_ws].layout == SCROLLING)
			fluorite.ws[fluorite.cr_ws].t_wins = FAddWindowScrolling(fluorite.ws[fluorite.cr_ws].t_wins, w);
		else 
			fluorite.ws[fluorite.cr_ws].t_wins = FAddWindow(fluorite.ws[fluorite.cr_ws].t_wins, w);
		if (!p->s_wins)
		{
			memset(fluorite.pads[fluorite.hpads], 0, sizeof(Scratchpads));
			fluorite.hpads = -1;
		}
		FRedrawWindows();
		XSync(fluorite.dpy, True);
		FApplyBorders();
		XChangeProperty(fluorite.dpy, w->w, XInternAtom(fluorite.dpy, "_NET_WM_DESKTOP", False), XA_CARDINAL, 32, PropModeReplace, (unsigned char *)&fluorite.cr_ws, 1);
		FUpdateClientList();
		XSetInputFocus(fluorite.dpy, w->w, RevertToPointerRoot, CurrentTime);
		FWarpCursor(w->w);
		break;
	}
	FPolybarScratchpadsIPC();
}

static void FScratchpadHideShow()
{
	if (fluorite.orgz) return;

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
		int found = False;
		for (Windows *w = p->s_wins; w != NULL; w = w->next)
		{
			XMapWindow(fluorite.dpy, w->w);
			XSetWindowBorder(fluorite.dpy, w->w, fluorite.conf.bu);
			if (w->fc)
			{
				XSetWindowBorder(fluorite.dpy, w->w, fluorite.conf.bf);
				XSetInputFocus(fluorite.dpy, w->w, RevertToPointerRoot, CurrentTime);
				found = True;
			}
		}
		if (!found)
		{
			p->s_wins->fc = True;
			XSetWindowBorder(fluorite.dpy, p->s_wins->w, fluorite.conf.bf);
			XSetInputFocus(fluorite.dpy, p->s_wins->w, RevertToPointerRoot, CurrentTime);
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

static void FCenterScratchpadWindow()
{
	if (fluorite.orgz) return;

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
	for (int i = 0; i < HASH_SIZE; i++)
	{
		if (!fluorite.pads[i]) continue;
		for (Windows *w = fluorite.pads[i]->s_wins; w != NULL; w = w->next)
			if (!FWindowExists(fluorite.dpy, w->w))
				fluorite.pads[i]->s_wins = FDelWindow(fluorite.pads[i]->s_wins, w);
	}
	FPolybarScratchpadsIPC();
}

static void FPolybarLayoutIPC(const int msg)
{
	char command[256];
	snprintf(command, sizeof(command), "polybar-msg action \"#fluorite_layout.hook.%d\"", msg);
	FExecute(command);
}

static void FPolybarScratchpadsIPC()
{
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
	snprintf(command, sizeof(command), "polybar-msg action \"#fluorite_scratchpads.hook.0\"");
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

static int FCheckAndSetStrut(int i, unsigned char *prop)
{
	int redraw = False;
	Monitors mon = fluorite.mon[i];
	long *strut;

	int sh = DisplayHeight(fluorite.dpy, fluorite.scr);
	int sw = DisplayWidth(fluorite.dpy, fluorite.scr);
	strut = (long *) prop;
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

	return redraw;
}

static void FRecalculateStrut(int mon)
{
	if (fluorite.mon[mon].fx_hdn)
		return;
	Atom strut_atom = XInternAtom(fluorite.dpy, "_NET_WM_STRUT_PARTIAL", False);
	Atom actual_type;
	int actual_format;
	unsigned long nitems, bytes_after;
	unsigned char *prop = NULL;

	if (!fluorite.mon[mon].fx_win)
		return ;
	for (Windows *fx = fluorite.mon[mon].fx_win; fx != NULL; fx = fx->next)
	{
		if (XGetWindowProperty(fluorite.dpy, fx->w, strut_atom, 0, 12, False, XA_CARDINAL, &actual_type, &actual_format, &nitems, &bytes_after, &prop) == !Success)
			continue;
		if (!prop)
			continue;
		FCheckAndSetStrut(mon, prop);
		XFree(prop);
		prop = NULL;
	}
	FUpdateWorkarea();
}

static void FResetMonitorStrut(int mon)
{
	fluorite.mon[mon].sl = 0;
	fluorite.mon[mon].sr = 0;
	fluorite.mon[mon].st = 0;
	fluorite.mon[mon].sb = 0;
}

static void FToggleFixedStrut()
{
	if (fluorite.orgz) return;

	if (fluorite.mon[fluorite.cr_mon].fx_hdn)
	{
		fluorite.mon[fluorite.cr_mon].fx_hdn = !fluorite.mon[fluorite.cr_mon].fx_hdn;
		for (Windows *fx = fluorite.mon[fluorite.cr_mon].fx_win; fx != NULL; fx = fx->next)
		{
			XMapWindow(fluorite.dpy, fx->w);
			FGetFixedPartialStrut(fx->w, False);
		}
	}
	else
	{
		fluorite.mon[fluorite.cr_mon].fx_hdn = !fluorite.mon[fluorite.cr_mon].fx_hdn;
		no_unmap = True;
		for (Windows *fx = fluorite.mon[fluorite.cr_mon].fx_win; fx != NULL; fx = fx->next)
			XUnmapWindow(fluorite.dpy, fx->w);
		FResetMonitorStrut(fluorite.cr_mon);
		XSync(fluorite.dpy, True);
		no_unmap = False;
		FRedrawWindows();
		XSync(fluorite.dpy, True);
		FApplyBorders();
	}
	FUpdateWorkarea();
}

static void FCycleLayouts()
{
	if (fluorite.orgz) return;

	switch (fluorite.ws[fluorite.cr_ws].layout)
	{
		case CASCADE:
			FChangeLayout(DWM);
			break;
		case DWM:
			FChangeLayout(CENTERED);
			break;
		case CENTERED:
			FChangeLayout(STACKED);
			break;
		case STACKED:
			FChangeLayout(SCROLLING);
			break;
		case SCROLLING:
			FChangeLayout(CASCADE);
			break;
	}
}

static void FToggleOrganizer()
{
	if (fluorite.ws[fluorite.cr_ws].fs || FCountWindows(fluorite.ws[fluorite.cr_ws].t_wins) < 2)
		return;

	if (!fluorite.orgz)
	{
		no_unmap = True;
		if (!fluorite.ws[fluorite.cr_ws].fl_hdn)
		{
			for (Windows *w = fluorite.ws[fluorite.cr_ws].f_wins; w != NULL; w = w->next)
			{
				XUnmapWindow(fluorite.dpy, w->w);
				w->fc = False;
			}
		}
		if (fluorite.hpads != -1)
		{
			for (Windows *s = fluorite.pads[fluorite.hpads]->s_wins; s != NULL; s = s->next)
			{
				XUnmapWindow(fluorite.dpy, s->w);
				s->fc = False;
			}
		}
		XSync(fluorite.dpy, True);
		no_unmap = False;
	}
	else
	{
		if (!fluorite.ws[fluorite.cr_ws].fl_hdn)
			for (Windows *w = fluorite.ws[fluorite.cr_ws].f_wins; w != NULL; w = w->next)
				XMapWindow(fluorite.dpy, w->w);
		if (fluorite.hpads != -1)
			for (Windows *s = fluorite.pads[fluorite.hpads]->s_wins; s != NULL; s = s->next)
				XMapWindow(fluorite.dpy, s->w);
	}

	FResetMonitorStrut(fluorite.cr_mon);
	FRecalculateStrut(fluorite.cr_mon);
	fluorite.orgz = !fluorite.orgz;
	FRedrawWindows();
	XSync(fluorite.dpy, True);
	FResetFocus(fluorite.ws[fluorite.cr_ws].t_wins);
	fluorite.ws[fluorite.cr_ws].t_wins->fc = True;
	XSetInputFocus(fluorite.dpy, fluorite.ws[fluorite.cr_ws].t_wins->w, RevertToPointerRoot, CurrentTime);
	FApplyBorders();
}

static void FRedrawOrganizer()
{
    int n = FCountWindows(fluorite.ws[fluorite.cr_ws].t_wins);
    if (n < 2)
    {
        FToggleOrganizer();
        return;
    }

    Monitors *m = &fluorite.mon[fluorite.cr_mon];
    int igp = fluorite.conf.igp;
    int ogp = fluorite.conf.ogp;
    int bw = fluorite.conf.bw;

    int usable_x = m->mx + m->sl + ogp;
    int usable_y = m->my + m->st + ogp;
    int usable_w = m->mw - (m->sl + m->sr) - 2 * ogp;
    int usable_h = m->mh - (m->st + m->sb) - 2 * ogp;

    int total_igp = (n - 1) * igp;
    int col_w = (usable_w - total_igp) / n;
    int wh = usable_h - 2 * bw;
    int ww = col_w - 2 * bw;

    int i = 0;
    for (Windows *w = fluorite.ws[fluorite.cr_ws].t_wins; w != NULL; w = w->next)
    {
        int wx = usable_x + i * (col_w + igp);
        w->wx = wx;
        w->wy = usable_y;
        w->ww = ww;
        w->wh = wh;

        XMoveResizeWindow(fluorite.dpy, w->w, wx, usable_y, ww, wh);
        i++;
    }
}

static Windows *FGetColStart(Windows *w)
{
	Windows *curr;

	if (!w) return NULL;
	curr = w;
	while (curr->prev && curr->prev->stk_blw)
		curr = curr->prev;
	return curr;
}

static Windows *FGetColEnd(Windows *w)
{
	Windows *curr;

	if (!w) return NULL;
	curr = w;
	while (curr->stk_blw && curr->next)
		curr = curr->next;
	return curr;
}

static int FCountColWins(Windows *w)
{
	int count = 0;
	Windows *curr = FGetColStart(w);

	while (curr)
	{
		count++;
		if (!curr->stk_blw) break;
		curr = curr->next;
	}
	return count;
}

static void FRedrawScrolling()
{
    Workspaces *ws = &fluorite.ws[fluorite.cr_ws];
    Monitors *m = &fluorite.mon[fluorite.cr_mon];
    Windows *focus_win = NULL;
    Windows *focus_col_start;
    Windows *focus_col_end;
    Windows *w;
    Windows *win;
    Windows *first_col_start;
    Windows *last_w;
    Windows *last_col_start;
    int igp = fluorite.conf.igp;
    int ogp = fluorite.conf.ogp;
    int bw = fluorite.conf.bw;
    int avail_w = m->mw - m->sl - m->sr - 2 * ogp;
    int usable_y = m->my + m->st + ogp;
    int usable_height = m->mh - m->st - m->sb - 2 * ogp;
    int focus_ww;
    int min_x;
    int max_x;
    int target_x;
    int only_one_col;
    int current_x;
    int n_cols = 0;
    int N;
    int col_usable_h;
    int sh_each;
    int i;

    if (!ws->t_wins) return;

    for (w = ws->t_wins; w; w = FGetColEnd(w)->next)
        n_cols++;

    for (w = ws->t_wins; w; w = w->next)
        if (w->fc) focus_win = w;
    if (!focus_win) focus_win = ws->t_wins;

    focus_col_start = FGetColStart(focus_win);
    focus_col_end = FGetColEnd(focus_win);

    int has_left = (focus_col_start->prev != NULL);
    int has_right = (focus_col_end->next != NULL);

    int has_full_width = 0;
    for (w = ws->t_wins; w; w = FGetColEnd(w)->next)
    {
        if (w->swp == 100)
        {
            has_full_width = 1;
            break;
        }
    }

    int apply_peek = (n_cols > 2) || (n_cols == 2 && has_full_width);
    int peek_left = 0;
    int peek_right = 0;
    if (apply_peek)
    {
        if (!has_left)
        {
            peek_left = 0;
            peek_right = 80;
        }
        else if (!has_right)
        {
            peek_left = 80;
            peek_right = 0;
        }
        else
        {
            peek_left = 40;
            peek_right = 40;
        }
    }
    int eff_avail_w = apply_peek ? (avail_w - 80) : avail_w;

    #define GET_WW(w) (((eff_avail_w - igp) * (w)->swp) / 100 - (bw * 2))
    #define GET_COL_W(w) (((eff_avail_w - igp) * (w)->swp) / 100 + igp)

    focus_ww = GET_WW(focus_col_start);
    min_x = m->mx + ogp + m->sl + peek_left;
    max_x = m->mx + m->mw - m->sr - ogp - focus_ww - (bw * 2) - peek_right;

    target_x = focus_col_start->wx;
    only_one_col = (focus_col_start == ws->t_wins && !focus_col_end->next);

    if (only_one_col || focus_col_start->swp == 100)
        target_x = m->mx + m->sl + ogp + peek_left + (eff_avail_w - focus_ww - (bw * 2)) / 2;
    else if (target_x < min_x)
        target_x = min_x;
    else if (target_x > max_x)
        target_x = max_x;

    N = FCountColWins(focus_col_start);
    col_usable_h = usable_height;
    sh_each = (N > 1) ? (col_usable_h - (N - 1) * igp) / N : col_usable_h;
    i = 0;
    for (w = focus_col_start; w; w = w->next)
    {
        w->wx = target_x;
        w->ww = focus_ww;
        w->wh = sh_each - 2 * bw;
        w->wy = usable_y + i * (sh_each + igp);
        i++;
        if (!w->stk_blw) break;
    }

    w = focus_col_start->prev;
    current_x = focus_col_start->wx;
    while (w)
    {
        Windows *col_s = FGetColStart(w);
        current_x -= GET_COL_W(col_s);
        N = FCountColWins(col_s);
        col_usable_h = usable_height;
        sh_each = (N > 1) ? (col_usable_h - (N - 1) * igp) / N : col_usable_h;
        i = 0;
        for (win = col_s; win; win = win->next)
        {
            win->wx = current_x;
            win->ww = GET_WW(col_s);
            win->wh = sh_each - 2 * bw;
            win->wy = usable_y + i * (sh_each + igp);
            i++;
            if (!win->stk_blw) break;
        }
        w = col_s->prev;
    }

    w = focus_col_end->next;
    current_x = focus_col_start->wx + GET_COL_W(focus_col_start);
    while (w)
    {
        N = FCountColWins(w);
        col_usable_h = usable_height;
        sh_each = (N > 1) ? (col_usable_h - (N - 1) * igp) / N : col_usable_h;
        i = 0;
        for (win = w; win; win = win->next)
        {
            win->wx = current_x;
            win->ww = GET_WW(w);
            win->wh = sh_each - 2 * bw;
            win->wy = usable_y + i * (sh_each + igp);
            i++;
            if (!win->stk_blw) break;
        }
        current_x += GET_COL_W(w);
        w = FGetColEnd(w)->next;
    }

    if (n_cols > 1)
    {
        first_col_start = ws->t_wins;
        int abs_min_x = m->mx + ogp + m->sl;
        if (first_col_start->wx > abs_min_x)
        {
            int shift = first_col_start->wx - abs_min_x;
            for (w = ws->t_wins; w; w = w->next) w->wx -= shift;
        }
        else
        {
            last_w = ws->t_wins;
            while (last_w->next) last_w = last_w->next;
            last_col_start = FGetColStart(last_w);
            int abs_max_x = m->mx + m->mw - m->sr - ogp - last_col_start->ww - (bw * 2);
            if (last_col_start->wx < abs_max_x && first_col_start->wx < abs_min_x)
            {
                int shift = abs_max_x - last_col_start->wx;
                if (first_col_start->wx + shift > abs_min_x) shift = abs_min_x - first_col_start->wx;
                for (w = ws->t_wins; w; w = w->next) w->wx += shift;
            }
        }
    }

    for (w = ws->t_wins; w; w = w->next)
        XMoveResizeWindow(fluorite.dpy, w->w, w->wx, w->wy, w->ww, w->wh);

    if (focus_win)
        XRaiseWindow(fluorite.dpy, focus_win->w);

    #undef GET_WW
    #undef GET_COL_W
}

static Windows *FAddWindowScrolling(Windows *head, Windows *nw)
{
	Windows *target = NULL;
	Windows *col_end;
	Windows *w;

	no_refocus = True;

	for (w = head; w != NULL; w = w->next)
	{
		if (w->fc) target = w;
		w->fc = False;
	}

	if (!head)
	{
		nw->next = NULL;
		nw->prev = NULL;
		return nw;
	}

	if (!target)
	{
		nw->next = head;
		nw->prev = NULL;
		head->prev = nw;
		return nw;
	}

	col_end = FGetColEnd(target);
	nw->prev = col_end;
	nw->next = col_end->next;
	if (col_end->next)
		col_end->next->prev = nw;
	col_end->next = nw;

	return head;
}

static void FScrollingFocusLeft()
{
	Windows *w;
	Windows *col_start;
	Windows *prev_col_start;

	if (fluorite.ws[fluorite.cr_ws].layout != SCROLLING || fluorite.ws[fluorite.cr_ws].fs || fluorite.orgz) return;

	for (w = fluorite.ws[fluorite.cr_ws].t_wins; w != NULL; w = w->next)
	{
		if (w->fc)
		{
			col_start = FGetColStart(w);
			prev_col_start = FGetColStart(col_start->prev);
			if (prev_col_start)
			{
				w->fc = False;
				prev_col_start->fc = True;
				XSetInputFocus(fluorite.dpy, prev_col_start->w, RevertToPointerRoot, CurrentTime);
				FApplyBorders();
				FRedrawWindows();
				FWarpCursor(prev_col_start->w);
				XSync(fluorite.dpy, True);
			}
			return;
		}
	}
}

static void FScrollingFocusRight()
{
	Windows *w;
	Windows *col_end;
	Windows *next_col_start;

	if (fluorite.ws[fluorite.cr_ws].layout != SCROLLING || fluorite.ws[fluorite.cr_ws].fs || fluorite.orgz) return;

	for (w = fluorite.ws[fluorite.cr_ws].t_wins; w != NULL; w = w->next)
	{
		if (w->fc)
		{
			col_end = FGetColEnd(w);
			next_col_start = col_end->next;
			if (next_col_start)
			{
				w->fc = False;
				next_col_start->fc = True;
				XSetInputFocus(fluorite.dpy, next_col_start->w, RevertToPointerRoot, CurrentTime);
				FApplyBorders();
				FRedrawWindows();
				FWarpCursor(next_col_start->w);
				XSync(fluorite.dpy, True);
			}
			return;
		}
	}
}

static void FScrollingFocusUp()
{
	Windows *w;

	if (fluorite.ws[fluorite.cr_ws].layout != SCROLLING || fluorite.ws[fluorite.cr_ws].fs || fluorite.orgz) return;

	for (w = fluorite.ws[fluorite.cr_ws].t_wins; w != NULL; w = w->next)
	{
		if (w->fc)
		{
			if (w->prev && w->prev->stk_blw)
			{
				w->fc = False;
				w->prev->fc = True;
				XSetInputFocus(fluorite.dpy, w->prev->w, RevertToPointerRoot, CurrentTime);
				FApplyBorders();
				FRedrawWindows();
				FWarpCursor(w->prev->w);
				XSync(fluorite.dpy, True);
			}
			return;
		}
	}
}

static void FScrollingFocusDown()
{
	Windows *w;

	if (fluorite.ws[fluorite.cr_ws].layout != SCROLLING || fluorite.ws[fluorite.cr_ws].fs || fluorite.orgz) return;

	for (w = fluorite.ws[fluorite.cr_ws].t_wins; w != NULL; w = w->next)
	{
		if (w->fc)
		{
			if (w->stk_blw && w->next)
			{
				w->fc = False;
				w->next->fc = True;
				XSetInputFocus(fluorite.dpy, w->next->w, RevertToPointerRoot, CurrentTime);
				FApplyBorders();
				FRedrawWindows();
				FWarpCursor(w->next->w);
				XSync(fluorite.dpy, True);
			}
			return;
		}
	}
}

static void FScrollingMoveLeft()
{
	Workspaces *ws = &fluorite.ws[fluorite.cr_ws];
	Windows *focus_win = NULL;
	Windows *w;

	if (ws->layout != SCROLLING || ws->fs || fluorite.orgz) return;

	for (w = ws->t_wins; w; w = w->next)
		if (w->fc) focus_win = w;
	if (!focus_win) return;

	int N = FCountColWins(focus_win);

	if (N > 1)
	{
		Windows *col_start = FGetColStart(focus_win);

		if (focus_win == col_start)
			focus_win->stk_blw = 0;
		else
		{
			if (focus_win->prev && focus_win->prev->stk_blw)
			{
				if (!focus_win->stk_blw)
					focus_win->prev->stk_blw = 0;
			}

			if (focus_win->prev) focus_win->prev->next = focus_win->next;
			else ws->t_wins = focus_win->next;
			if (focus_win->next) focus_win->next->prev = focus_win->prev;

			focus_win->prev = col_start->prev;
			focus_win->next = col_start;
			if (col_start->prev) col_start->prev->next = focus_win;
			else ws->t_wins = focus_win;
			col_start->prev = focus_win;

			focus_win->stk_blw = 0;
		}
		focus_win->swp = 50;
	}
	else
	{
		Windows *prev_col_start = focus_win->prev ? FGetColStart(focus_win->prev) : NULL;
		if (prev_col_start)
		{
			if (focus_win->prev) focus_win->prev->next = focus_win->next;
			else ws->t_wins = focus_win->next;
			if (focus_win->next) focus_win->next->prev = focus_win->prev;

			focus_win->prev = prev_col_start->prev;
			focus_win->next = prev_col_start;
			if (prev_col_start->prev) prev_col_start->prev->next = focus_win;
			else ws->t_wins = focus_win;
			prev_col_start->prev = focus_win;
		}
	}

	FRedrawWindows();
	FWarpCursor(focus_win->w);
	XSync(fluorite.dpy, True);
}

static void FScrollingMoveRight()
{
	Workspaces *ws = &fluorite.ws[fluorite.cr_ws];
	Windows *focus_win = NULL;
	Windows *w;

	if (ws->layout != SCROLLING || ws->fs || fluorite.orgz) return;

	for (w = ws->t_wins; w; w = w->next)
		if (w->fc) focus_win = w;
	if (!focus_win) return;

	int N = FCountColWins(focus_win);

	if (N > 1)
	{
		Windows *col_end = FGetColEnd(focus_win);

		if (focus_win == col_end)
		{
			if (focus_win->prev)
				focus_win->prev->stk_blw = 0;
		}
		else
		{
			if (focus_win->prev && focus_win->prev->stk_blw)
			{
				if (!focus_win->stk_blw)
					focus_win->prev->stk_blw = 0;
			}

			if (focus_win->prev) focus_win->prev->next = focus_win->next;
			else ws->t_wins = focus_win->next;
			if (focus_win->next) focus_win->next->prev = focus_win->prev;

			focus_win->next = col_end->next;
			focus_win->prev = col_end;
			if (col_end->next) col_end->next->prev = focus_win;
			col_end->next = focus_win;

			focus_win->stk_blw = 0;
		}
		focus_win->swp = 50;
	}
	else
	{
		Windows *next_col_start = focus_win->next;
		if (next_col_start)
		{
			Windows *next_col_end = FGetColEnd(next_col_start);

			if (focus_win->prev) focus_win->prev->next = focus_win->next;
			else ws->t_wins = focus_win->next;
			if (focus_win->next) focus_win->next->prev = focus_win->prev;

			focus_win->next = next_col_end->next;
			focus_win->prev = next_col_end;
			if (next_col_end->next) next_col_end->next->prev = focus_win;
			next_col_end->next = focus_win;
		}
	}

	FRedrawWindows();
	FWarpCursor(focus_win->w);
	XSync(fluorite.dpy, True);
}

static void FScrollingMoveUp()
{
	Windows *w;
	Windows *prev_win;
	Windows *prev_prev;
	Windows *next_win;
	int tmp_stk;

	if (fluorite.ws[fluorite.cr_ws].layout != SCROLLING || fluorite.ws[fluorite.cr_ws].fs || fluorite.orgz) return;

	for (w = fluorite.ws[fluorite.cr_ws].t_wins; w != NULL; w = w->next)
	{
		if (w->fc)
		{
			prev_win = w->prev;
			if (!prev_win || !prev_win->stk_blw) return;

			prev_prev = prev_win->prev;
			next_win = w->next;

			if (prev_prev) prev_prev->next = w;
			else fluorite.ws[fluorite.cr_ws].t_wins = w;
			w->prev = prev_prev;

			prev_win->next = next_win;
			if (next_win) next_win->prev = prev_win;

			w->next = prev_win;
			prev_win->prev = w;

			tmp_stk = w->stk_blw;
			w->stk_blw = prev_win->stk_blw;
			prev_win->stk_blw = tmp_stk;

			FRedrawWindows();
			FWarpCursor(w->w);
			XSync(fluorite.dpy, True);
			return;
		}
	}
}

static void FScrollingMoveDown()
{
	Windows *w;
	Windows *next_win;
	Windows *prev_win;
	Windows *next_next;
	int tmp_stk;

	if (fluorite.ws[fluorite.cr_ws].layout != SCROLLING || fluorite.ws[fluorite.cr_ws].fs || fluorite.orgz) return;

	for (w = fluorite.ws[fluorite.cr_ws].t_wins; w != NULL; w = w->next)
	{
		if (w->fc)
		{
			next_win = w->next;
			if (!next_win || !w->stk_blw) return;

			prev_win = w->prev;
			next_next = next_win->next;

			if (prev_win) prev_win->next = next_win;
			else fluorite.ws[fluorite.cr_ws].t_wins = next_win;
			next_win->prev = prev_win;

			w->next = next_next;
			if (next_next) next_next->prev = w;

			next_win->next = w;
			w->prev = next_win;

			tmp_stk = w->stk_blw;
			w->stk_blw = next_win->stk_blw;
			next_win->stk_blw = tmp_stk;

			FRedrawWindows();
			FWarpCursor(w->w);
			XSync(fluorite.dpy, True);
			return;
		}
	}
}

static void FScrollingResizeIncrease()
{
	Windows *w;
	Windows *col_s;
	Windows *col_e;
	Windows *curr;
	
	if (fluorite.ws[fluorite.cr_ws].layout != SCROLLING || fluorite.ws[fluorite.cr_ws].fs || fluorite.orgz || !fluorite.ws[fluorite.cr_ws].t_wins->next) return;

	for (w = fluorite.ws[fluorite.cr_ws].t_wins; w != NULL; w = w->next)
	{
		if (w->fc)
		{
			col_s = FGetColStart(w);
			col_e = FGetColEnd(w);

			for (curr = col_s; curr != NULL; curr = curr->next)
			{	
				if (curr->swp < 100) curr->swp += 50;
				if (curr->swp > 100) curr->swp = 100;
				if (curr == col_e) break;
			}
			
			FRedrawWindows();
			XSync(fluorite.dpy, True);
			FWarpCursor(w->w);
			return;
		}
	}
}

static void FScrollingResizeDecrease()
{
	Windows *w;
	Windows *col_s;
	Windows *col_e;
	Windows *curr;

	if (fluorite.ws[fluorite.cr_ws].layout != SCROLLING || fluorite.ws[fluorite.cr_ws].fs || fluorite.orgz || !fluorite.ws[fluorite.cr_ws].t_wins->next) return;

	for (w = fluorite.ws[fluorite.cr_ws].t_wins; w != NULL; w = w->next)
	{
		if (w->fc)
		{
			col_s = FGetColStart(w);
			col_e = FGetColEnd(w);

			for (curr = col_s; curr != NULL; curr = curr->next)
			{	
				if (curr->swp > 50) curr->swp -= 50;
				if (curr->swp < 50) curr->swp = 50;
				if (curr == col_e) break;
			}

			FRedrawWindows();
			XSync(fluorite.dpy, True);
			FWarpCursor(w->w);
			return;
		}
	}
}

static void FScrollingMoveWindowToColumnLeft()
{
	Workspaces *ws = &fluorite.ws[fluorite.cr_ws];
	Windows *focus_win = NULL;
	Windows *w;

	if (ws->layout != SCROLLING || ws->fs || fluorite.orgz) return;

	for (w = ws->t_wins; w; w = w->next)
		if (w->fc) focus_win = w;
	if (!focus_win) return;

	Windows *col_start = FGetColStart(focus_win);
	Windows *prev_col_start = col_start->prev ? FGetColStart(col_start->prev) : NULL;

	if (prev_col_start)
	{
		Windows *target_col_end = FGetColEnd(prev_col_start);

		if (focus_win->prev && focus_win->prev->stk_blw)
		{
			if (!focus_win->stk_blw)
				focus_win->prev->stk_blw = 0;
		}

		if (focus_win->prev) focus_win->prev->next = focus_win->next;
		else ws->t_wins = focus_win->next;
		if (focus_win->next) focus_win->next->prev = focus_win->prev;

		focus_win->next = target_col_end->next;
		focus_win->prev = target_col_end;
		if (target_col_end->next) target_col_end->next->prev = focus_win;
		target_col_end->next = focus_win;

		target_col_end->stk_blw = 1;
		focus_win->stk_blw = 0;
		focus_win->swp = 50;
		focus_win->prev->swp = 50;

		FRedrawWindows();
		FWarpCursor(focus_win->w);
		XSync(fluorite.dpy, True);
	}
}

static void FScrollingMoveWindowToColumnRight()
{
	Workspaces *ws = &fluorite.ws[fluorite.cr_ws];
	Windows *focus_win = NULL;
	Windows *w;

	if (ws->layout != SCROLLING || ws->fs || fluorite.orgz) return;

	for (w = ws->t_wins; w; w = w->next)
		if (w->fc) focus_win = w;
	if (!focus_win) return;

	Windows *col_end = FGetColEnd(focus_win);
	Windows *next_col_start = col_end->next;

	if (next_col_start)
	{
		if (focus_win->prev && focus_win->prev->stk_blw)
		{
			if (!focus_win->stk_blw)
				focus_win->prev->stk_blw = 0;
		}

		if (focus_win->prev) focus_win->prev->next = focus_win->next;
		else ws->t_wins = focus_win->next;
		if (focus_win->next) focus_win->next->prev = focus_win->prev;

		focus_win->prev = next_col_start->prev;
		focus_win->next = next_col_start;
		if (next_col_start->prev) next_col_start->prev->next = focus_win;
		else ws->t_wins = focus_win;
		next_col_start->prev = focus_win;

		focus_win->stk_blw = 1;
		focus_win->swp = 50;
		focus_win->next->swp = 50;

		FRedrawWindows();
		FWarpCursor(focus_win->w);
		XSync(fluorite.dpy, True);
	}
}

static void FUpdateWorkarea()
{
    unsigned long workarea[MAX_WS * 4];
    int mon = fluorite.cr_mon;

    int wx = fluorite.mon[mon].mx + fluorite.mon[mon].sl;
    int wy = fluorite.mon[mon].my + fluorite.mon[mon].st;
    int ww = fluorite.mon[mon].mw - fluorite.mon[mon].sl - fluorite.mon[mon].sr;
    int wh = fluorite.mon[mon].mh - fluorite.mon[mon].st - fluorite.mon[mon].sb;

    if (ww < 0) ww = 0;
    if (wh < 0) wh = 0;

    for (int i = 0; i < MAX_WS; i++)
    {
        workarea[i * 4 + 0] = wx;
        workarea[i * 4 + 1] = wy;
        workarea[i * 4 + 2] = ww;
        workarea[i * 4 + 3] = wh;
    }

    XChangeProperty(
        fluorite.dpy,
        fluorite.root,
        XInternAtom(fluorite.dpy, "_NET_WORKAREA", False),
        XA_CARDINAL,
        32,
        PropModeReplace,
        (unsigned char *)workarea,
        MAX_WS * 4
    );
}

static void FUpdateDesktopViewport()
{
    unsigned long viewports[2] = {0, 0};

    XChangeProperty(
        fluorite.dpy,
        fluorite.root,
        XInternAtom(fluorite.dpy, "_NET_DESKTOP_VIEWPORT", False),
        XA_CARDINAL,
        32,
        PropModeReplace,
        (unsigned char *)viewports,
        2
    );
}

static void FHandleIPCCommand(char *cmd)
{
	cmd[strcspn(cmd, "\r\n")] = 0;

	char action[64] = {0};
	char arg[64] = {0};

	sscanf(cmd, "%63s %63s", action, arg);

	for (unsigned int j = 0; j < LENGTH(user_functions_list); j++)
	{
		if (strcasecmp(action, user_functions_list[j].name) == 0)
		{
			if (user_functions_list[j].type == VOID && user_functions_list[j].void_fun)
				user_functions_list[j].void_fun();
			else if (user_functions_list[j].type == INT && user_functions_list[j].int_fun)
			{
				if (strcasecmp(arg, "up") == 0) user_functions_list[j].int_fun(UP);
				else if (strcasecmp(arg, "down") == 0) user_functions_list[j].int_fun(DOWN);
				else user_functions_list[j].int_fun(atoi(arg));
			}
			else if (user_functions_list[j].type == CHAR && user_functions_list[j].char_fun)
				user_functions_list[j].char_fun(arg);
			break;
		}
	}
	FRedrawWindows();
	XSync(fluorite.dpy, True);
	FApplyBorders();
}

static void *FIPCServerThread(void *unused)
{
	(void)unused;
	int server_fd, client_fd;
	struct sockaddr_un addr;
	char buffer[256];

	unlink(SOCKET_PATH);
	if ((server_fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
		return NULL;

	memset(&addr, 0, sizeof(addr));
	addr.sun_family = AF_UNIX;
	strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);

	if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) == -1)
	{
		close(server_fd);
		return NULL;
	}

	if (listen(server_fd, 5) == -1)
	{
		close(server_fd);
		return NULL;
	}

	while (fluorite.run)
	{
		if ((client_fd = accept(server_fd, NULL, NULL)) == -1)
			continue;

		ssize_t bytes_read = read(client_fd, buffer, sizeof(buffer) - 1);
		if (bytes_read > 0)
		{
			buffer[bytes_read] = '\0';
			FHandleIPCCommand(buffer);
			int res = write(client_fd, "OK\n", 3);
			(void)res;
		}
		close(client_fd);
	}
	close(server_fd);
	unlink(SOCKET_PATH);
	return NULL;
}

static void FScratchpadToggleByKeysym(char *arg)
{
    if (fluorite.orgz || !arg || strlen(arg) == 0) return;

    KeySym key = XStringToKeysym(arg);
    if (key == NoSymbol)
        return;

    int hkey = FHashKey(key);
    Scratchpads *p = fluorite.pads[hkey];
    if (!p || p->key != key || !p->s_wins)
        return;

    Scratchpads *op = (fluorite.hpads != -1) ? fluorite.pads[fluorite.hpads] : NULL;

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
        int found = False;
        for (Windows *w = p->s_wins; w != NULL; w = w->next)
        {
            XMapWindow(fluorite.dpy, w->w);
            XSetWindowBorder(fluorite.dpy, w->w, fluorite.conf.bu);
            if (w->fc)
            {
                XSetWindowBorder(fluorite.dpy, w->w, fluorite.conf.bf);
                XSetInputFocus(fluorite.dpy, w->w, RevertToPointerRoot, CurrentTime);
                found = True;
            }
        }
        if (!found)
        {
            p->s_wins->fc = True;
            XSetWindowBorder(fluorite.dpy, p->s_wins->w, fluorite.conf.bf);
            XSetInputFocus(fluorite.dpy, p->s_wins->w, RevertToPointerRoot, CurrentTime);
        }
        fluorite.hpads = hkey;
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

    FRedrawWindows();
    XSync(fluorite.dpy, True);
    FApplyBorders();
    FPolybarScratchpadsIPC();
}
