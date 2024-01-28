#include "config_fluorite.h"
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <X11/Xcursor/Xcursor.h>
#include <X11/extensions/Xcomposite.h>
#include <err.h>
#include <stdlib.h>
#include <string.h>

#define STACK_NEW		0
#define STACK_DEL		1
#define STACK_UP		2
#define STACK_DOWN		3

// Thanks suckless for these
static unsigned int numlockmask = 0;
#define MODMASK(mask)	(mask & ~(numlockmask | LockMask) & (ShiftMask|ControlMask|Mod1Mask|Mod2Mask|Mod3Mask|Mod4Mask|Mod5Mask))
#define LENGTH(X)		(sizeof X / sizeof X[0])

#define MAX_WINDOWS		1145
#define MAX_WORKSPACES	9

typedef struct
{
	Window	frame;
	Window	window;
	int		focused;
	int		pos_x;
	int		pos_y;
	int		width;
	int		height;
	int		border_color;
	int		border_width;
} WinFrames;

typedef struct
{
	WinFrames	*master_winframe;
	WinFrames	**slaves_winframes;
	WinFrames	*tmp_winframe;
	int			frames_count;
	int			slaves_count;
	int			master_offset;
	int			is_fullscreen;
} Workspaces;

typedef struct
{
	Display		*display;
	Window		root;
	int			screen;
	int			screen_width;
	int			screen_height;
	int			running;
	Workspaces	*workspaces;
	int			current_workspace;
} Fluorite;

static Fluorite fluorite;

// Core functions
static void fluorite_init();
static void fluorite_run();
static void fluorite_clean();

// Core utilities
static void			fluorite_handle_configuration(XConfigureRequestEvent e);
static void			fluorite_handle_mapping(XMapRequestEvent e);
static void			fluorite_handle_unmapping(Window e);
static void			fluorite_handle_keys(XKeyPressedEvent e);
static WinFrames	*fluorite_create_simple_winframe();
static WinFrames	*fluorite_create_winframe();
static void			fluorite_organise_stack(int mode, int offset);
static void			fluorite_redraw_windows();
static void			dwm_grabkeys();

// Bindings functions (defined in config_fluorite.h)
void fluorite_execute(char *argument)
{
	strcat(argument, " &");
	system(argument);
}

void fluorite_close_window()
{
	if (fluorite.workspaces[fluorite.current_workspace].frames_count == 0)
		return ;

	if (fluorite.workspaces[fluorite.current_workspace].master_winframe->window)
	{
		XEvent closing;
		memset(&closing, 0, sizeof(closing));
		closing.xclient.type = ClientMessage;
		closing.xclient.message_type = XInternAtom(fluorite.display, "WM_PROTOCOLS", False);
		closing.xclient.window = fluorite.workspaces[fluorite.current_workspace].master_winframe->window;
		closing.xclient.format = 32;
		closing.xclient.data.l[0] = XInternAtom(fluorite.display, "WM_DELETE_WINDOW", False);
		XSendEvent(fluorite.display, fluorite.workspaces[fluorite.current_workspace].master_winframe->window, False, 0, &closing);
	}
	else
		XDestroyWindow(fluorite.display, fluorite.workspaces[fluorite.current_workspace].master_winframe->frame);
}

void fluorite_user_close()
{
	fluorite.running = 0;
	fluorite_clean();
}

void fluorite_layout_change(int mode)
{
	switch(mode)
	{
		case FOCUS_TOP:
			fluorite_organise_stack(STACK_UP, -1);
			fluorite_redraw_windows();
			break;
		case FOCUS_BOTTOM:
			fluorite_organise_stack(STACK_DOWN, -1);
			fluorite_redraw_windows();
			break;
		case SLAVES_UP:
			fluorite_organise_stack(SLAVES_UP, -1);
			fluorite_redraw_windows();
			break;
		case SLAVES_DOWN:
			fluorite_organise_stack(SLAVES_DOWN, -1);
			fluorite_redraw_windows();
			break;
		case BIGGER_MASTER:
			if (fluorite.workspaces[fluorite.current_workspace].slaves_count > 0 && !fluorite.workspaces[fluorite.current_workspace].is_fullscreen)
			{
				fluorite.workspaces[fluorite.current_workspace].master_offset += 25;
				fluorite_redraw_windows();
			}
			break;
		case SMALLER_MASTER:
			if (fluorite.workspaces[fluorite.current_workspace].slaves_count > 0 && !fluorite.workspaces[fluorite.current_workspace].is_fullscreen)
			{
				fluorite.workspaces[fluorite.current_workspace].master_offset -= 25;
				fluorite_redraw_windows();
			}
			break;
		case FULLSCREEN_TOGGLE:
			fluorite.workspaces[fluorite.current_workspace].is_fullscreen = !fluorite.workspaces[fluorite.current_workspace].is_fullscreen;
			fluorite_redraw_windows();
			break;
	}
}

void fluorite_change_workspace(int new_workspace, int mode)
{
	if (fluorite.current_workspace == new_workspace)
		return;
	if (mode == 0)
	{
		if (fluorite.workspaces[fluorite.current_workspace].frames_count > 0)
		{
			if (fluorite.workspaces[fluorite.current_workspace].master_winframe->frame)
				XUnmapWindow(fluorite.display, fluorite.workspaces[fluorite.current_workspace].master_winframe->frame);
			for (int i = 0; i < fluorite.workspaces[fluorite.current_workspace].slaves_count; i++)
				XUnmapWindow(fluorite.display, fluorite.workspaces[fluorite.current_workspace].slaves_winframes[i]->frame);
		}

		fluorite.current_workspace = new_workspace;
		if (fluorite.workspaces[fluorite.current_workspace].frames_count > 0)
		{
			if (fluorite.workspaces[fluorite.current_workspace].master_winframe->frame)
				XMapWindow(fluorite.display, fluorite.workspaces[fluorite.current_workspace].master_winframe->frame);
			for (int i = 0; i < fluorite.workspaces[fluorite.current_workspace].slaves_count; i++)
				XMapWindow(fluorite.display, fluorite.workspaces[fluorite.current_workspace].slaves_winframes[i]->frame);
		}
	}
	else
	{
		int keep_workspace = fluorite.current_workspace;

		if (fluorite.workspaces[new_workspace].frames_count > 0)
		{
			fluorite.workspaces[new_workspace].tmp_winframe = (WinFrames *) malloc(sizeof(WinFrames));
			memcpy(fluorite.workspaces[new_workspace].tmp_winframe, fluorite.workspaces[new_workspace].master_winframe, sizeof(WinFrames));
			free(fluorite.workspaces[new_workspace].master_winframe);
			fluorite.workspaces[new_workspace].slaves_count++;
		}
		fluorite.workspaces[new_workspace].master_winframe = (WinFrames *) malloc(sizeof(WinFrames));
		fluorite.workspaces[new_workspace].frames_count++;
		memcpy(fluorite.workspaces[new_workspace].master_winframe, fluorite.workspaces[fluorite.current_workspace].master_winframe, sizeof(WinFrames));
		if (fluorite.workspaces[new_workspace].slaves_count > 0)
		{
			fluorite.current_workspace = new_workspace;
			fluorite_organise_stack(STACK_NEW, -1);
			free(fluorite.workspaces[new_workspace].tmp_winframe);
			fluorite.current_workspace = keep_workspace;
		}
		
		fluorite.workspaces[fluorite.current_workspace].frames_count--;
		XUnmapWindow(fluorite.display, fluorite.workspaces[fluorite.current_workspace].master_winframe->frame);
		XSetInputFocus(fluorite.display, fluorite.root, RevertToPointerRoot, CurrentTime);
		free(fluorite.workspaces[fluorite.current_workspace].master_winframe);
		if (fluorite.workspaces[fluorite.current_workspace].slaves_count > 0)
		{
			fluorite.workspaces[fluorite.current_workspace].master_winframe = (WinFrames *) malloc(sizeof(WinFrames));
			memcpy(fluorite.workspaces[fluorite.current_workspace].master_winframe, fluorite.workspaces[fluorite.current_workspace].slaves_winframes[0], sizeof(WinFrames));
			XSetInputFocus(fluorite.display, fluorite.workspaces[fluorite.current_workspace].master_winframe->frame, RevertToPointerRoot, CurrentTime);
			XRaiseWindow(fluorite.display, fluorite.workspaces[fluorite.current_workspace].master_winframe->frame);
			fluorite_organise_stack(STACK_DEL, 0);
			fluorite.workspaces[fluorite.current_workspace].slaves_count--;
		}
		if (FOLLOW_WINDOWS)
			fluorite_change_workspace(new_workspace, 0);
	}
	fluorite_redraw_windows();
}

// Core functions
void fluorite_init()
{
	fluorite.display = XOpenDisplay(NULL);
	if (fluorite.display == NULL)
		errx(1, "Can't open display.");

	fluorite.screen = DefaultScreen(fluorite.display);
	fluorite.root = RootWindow(fluorite.display, fluorite.screen);

	fluorite.screen_width = DisplayWidth(fluorite.display, fluorite.screen);
	fluorite.screen_height = DisplayHeight(fluorite.display, fluorite.screen);

	fluorite.current_workspace = 0;
	fluorite.workspaces = malloc(sizeof(Workspaces) * MAX_WORKSPACES);
	for (int i = 0; i <= MAX_WORKSPACES; i++)
	{
		fluorite.workspaces[i].frames_count = 0;
		fluorite.workspaces[i].slaves_count = 0;
		fluorite.workspaces[i].master_offset = 0;
		fluorite.workspaces[i].is_fullscreen = 0;
	}

	XChangeProperty(
		fluorite.display,
		fluorite.root,
		XInternAtom(fluorite.display, "_NET_WM_NAME", False),
		XInternAtom(fluorite.display, "UTF8_STRING", False),
		8,
		PropModeReplace,
		(const unsigned char *) "Fluorite",
		8
	);
	XChangeProperty(
		fluorite.display,
		fluorite.root,
		XInternAtom(fluorite.display, "_NET_SUPPORTING_WM_CHECK", False),
		XA_WINDOW,
		32,
		PropModeReplace,
		(const unsigned char *) &fluorite.root,
		1
	);

	XFlush(fluorite.display);
	XSelectInput(fluorite.display, fluorite.root, SubstructureNotifyMask | SubstructureRedirectMask);
	XSync(fluorite.display, False);

	XSetWindowAttributes attributes;
	attributes.event_mask =
		SubstructureNotifyMask |
		SubstructureRedirectMask |
		StructureNotifyMask |
		ButtonPressMask |
		KeyPressMask |
		PointerMotionMask |
		PropertyChangeMask;
	XChangeWindowAttributes(fluorite.display, fluorite.root, CWEventMask, &attributes);

	XDefineCursor(
			fluorite.display,
			fluorite.root,
			XcursorLibraryLoadCursor(fluorite.display, "arrow")
	);

	XSync(fluorite.display, False);

	for (int j = 0; j <= MAX_WORKSPACES; j++)
	{
		fluorite.workspaces[j].slaves_winframes = (WinFrames **) malloc(sizeof(WinFrames *) * MAX_WINDOWS);
		for (int i = 0; i <= MAX_WINDOWS; i++)
			fluorite.workspaces[j].slaves_winframes[i] = (WinFrames *) malloc(sizeof(WinFrames));
	}

	dwm_grabkeys();

	return ;
}

void fluorite_run()
{
	XEvent ev;

	fluorite.running = 1;
	while (fluorite.running)
	{
		XNextEvent(fluorite.display, &ev);
		switch(ev.type)
		{
			case ConfigureRequest:
				fluorite_handle_configuration(ev.xconfigurerequest);
				break;
			case MapRequest:
				fluorite_handle_mapping(ev.xmaprequest);
				break;
			case UnmapNotify:
				fluorite_handle_unmapping(ev.xunmap.window);
				break;
			case ButtonPress:
				break;
			case EnterNotify:
				if (ev.xcrossing.window == fluorite.workspaces[fluorite.current_workspace].slaves_winframes[0]->frame)
					XSetInputFocus(fluorite.display, fluorite.workspaces[fluorite.current_workspace].slaves_winframes[0]->window, RevertToPointerRoot, CurrentTime);
				else
					XSetInputFocus(fluorite.display, fluorite.workspaces[fluorite.current_workspace].master_winframe->window, RevertToPointerRoot, CurrentTime);
				break;
			case KeyPress:
				fluorite_handle_keys(ev.xkey);
				break;
		}
	}
}

void fluorite_clean()
{
	if (fluorite.workspaces[fluorite.current_workspace].master_winframe)
		free(fluorite.workspaces[fluorite.current_workspace].master_winframe);
	if (fluorite.workspaces[fluorite.current_workspace].slaves_winframes)
	{
		for (int i = 0; fluorite.workspaces[fluorite.current_workspace].slaves_winframes[i]; i++)
			free(fluorite.workspaces[fluorite.current_workspace].slaves_winframes[i]);
		free(fluorite.workspaces[fluorite.current_workspace].slaves_winframes);
	}
	XSync(fluorite.display, True);
	XCloseDisplay(fluorite.display);
	return ;
}


// Core utilities
void dwm_updatenumlockmask()
{
	unsigned int i;
	int j;
	XModifierKeymap *modmap;

	numlockmask = 0;
	modmap = XGetModifierMapping(fluorite.display);
	for (i = 0; i < 8; i++)
		for (j = 0; j < modmap->max_keypermod; j++)
			if (modmap->modifiermap[i * modmap->max_keypermod + j] == XKeysymToKeycode(fluorite.display, XK_Num_Lock))
				numlockmask = (1 << i);
	XFreeModifiermap(modmap);
}

void dwm_grabkeys()
{
	// Since it works fine, why recode it ?
	dwm_updatenumlockmask();
	{
		unsigned int i, j, k;
		unsigned int modifiers[] = { 0, LockMask, numlockmask, numlockmask|LockMask };
		int start, end, skip;
		KeySym *syms;

		XUngrabKey(fluorite.display, AnyKey, AnyModifier, fluorite.root);
		XDisplayKeycodes(fluorite.display, &start, &end);
		syms = XGetKeyboardMapping(fluorite.display, start, end - start + 1, &skip);
		if (!syms)
			return;
		for (k = start; k <= end; k++)
			for (i = 0; i < LENGTH(bind); i++)
				if (bind[i].key == syms[(k - start) * skip])
					for (j = 0; j < LENGTH(modifiers); j++)
						XGrabKey(fluorite.display, k,
							 bind[i].mod | modifiers[j],
							 fluorite.root, True,
							 GrabModeAsync, GrabModeAsync);
		XFree(syms);
	}
}

void fluorite_handle_configuration(XConfigureRequestEvent e)
{
	XWindowChanges changes;
	changes.x = e.x;
	changes.y = e.y;
	changes.width = e.width;
	changes.height = e.height;
	changes.border_width = e.border_width;
	changes.sibling = e.above;
	changes.stack_mode = e.detail;
	XConfigureWindow(fluorite.display, e.window, e.value_mask, &changes);
}

void fluorite_handle_mapping(XMapRequestEvent e)
{
	if (fluorite.workspaces[fluorite.current_workspace].frames_count == MAX_WINDOWS)
		return ;

	if (fluorite.workspaces[fluorite.current_workspace].frames_count > 0)
	{
		fluorite.workspaces[fluorite.current_workspace].tmp_winframe = (WinFrames *) malloc(sizeof(WinFrames));
		memcpy(fluorite.workspaces[fluorite.current_workspace].tmp_winframe, fluorite.workspaces[fluorite.current_workspace].master_winframe, sizeof(WinFrames));
		free(fluorite.workspaces[fluorite.current_workspace].master_winframe);
		fluorite.workspaces[fluorite.current_workspace].slaves_count++;
	}

	fluorite.workspaces[fluorite.current_workspace].master_winframe = (WinFrames *) malloc(sizeof(WinFrames));
	fluorite.workspaces[fluorite.current_workspace].frames_count++;
	fluorite.workspaces[fluorite.current_workspace].master_winframe = fluorite_create_winframe();
	fluorite.workspaces[fluorite.current_workspace].master_winframe->window = e.window;

	XGrabServer(fluorite.display);
	XSelectInput(fluorite.display, fluorite.workspaces[fluorite.current_workspace].master_winframe->frame, SubstructureNotifyMask | SubstructureRedirectMask | EnterWindowMask);
	XWMHints *source_hints = XGetWMHints(fluorite.display, e.window);
	if (source_hints)
	{
		XSetWMHints(fluorite.display, fluorite.workspaces[fluorite.current_workspace].master_winframe->frame, source_hints);
		XFree(source_hints);
	}
	XReparentWindow(fluorite.display, e.window, fluorite.workspaces[fluorite.current_workspace].master_winframe->frame, 0, 0);
	XMapWindow(fluorite.display, fluorite.workspaces[fluorite.current_workspace].master_winframe->frame);
	XRaiseWindow(fluorite.display, fluorite.workspaces[fluorite.current_workspace].master_winframe->frame);
	XMapWindow(fluorite.display, e.window);
	XUngrabServer(fluorite.display);
	XSync(fluorite.display, True);
	XMoveResizeWindow(fluorite.display, fluorite.workspaces[fluorite.current_workspace].master_winframe->window, 0, 0, fluorite.workspaces[fluorite.current_workspace].master_winframe->width, fluorite.workspaces[fluorite.current_workspace].master_winframe->height);

	if (fluorite.workspaces[fluorite.current_workspace].slaves_count > 0)
	{
		fluorite_organise_stack(STACK_NEW, -1);
		free(fluorite.workspaces[fluorite.current_workspace].tmp_winframe);
	}

	fluorite_redraw_windows();
}

void fluorite_handle_keys(XKeyPressedEvent e)
{
	int i;
	unsigned int cleaned_mod;
	unsigned int user_keycode;

	i = 0;
	while (bind[i].mod)
	{
		cleaned_mod = MODMASK(bind[i].mod);
		user_keycode = XKeysymToKeycode(fluorite.display, bind[i].key);
		if (cleaned_mod == MODMASK(e.state) && user_keycode == e.keycode)
				bind[i].func();
		i++;
	}
}

// TODO: Might be deleted later if it's not used anymore
WinFrames *fluorite_create_simple_winframe()
{
	WinFrames *new_frame;
	
	new_frame = malloc(sizeof(WinFrames));
	new_frame->pos_x = 0;
	new_frame->pos_y = 0;
	new_frame->width = fluorite.screen_width - (BORDER_WIDTH * 2);
	new_frame->height = fluorite.screen_height - (BORDER_WIDTH * 2);
	new_frame->border_color = BORDER_COLORS;
	new_frame->border_width = BORDER_WIDTH;
	new_frame->frame = XCreateSimpleWindow(
		fluorite.display, fluorite.root,
		new_frame->pos_x, new_frame->pos_y,
		new_frame->width, new_frame->height,
		new_frame->border_width,
		new_frame->border_color,
		0x1e1e1e
	);

	return new_frame;
}

WinFrames *fluorite_create_winframe()
{
	WinFrames *new_frame;
	XVisualInfo vinfo;
	XSetWindowAttributes attribs_set;
	XMatchVisualInfo(fluorite.display, fluorite.screen, 32, TrueColor, &vinfo);
	attribs_set.background_pixel = 0;
	attribs_set.border_pixel = BORDER_COLORS;
	attribs_set.colormap = XCreateColormap(fluorite.display, fluorite.root, vinfo.visual, AllocNone);
	attribs_set.event_mask = StructureNotifyMask | ExposureMask;

	new_frame = malloc(sizeof(WinFrames));
	new_frame->pos_x = 0;
	new_frame->pos_y = 0;
	new_frame->width = fluorite.screen_width - (BORDER_WIDTH * 2);
	new_frame->height = fluorite.screen_height - (BORDER_WIDTH * 2);
	new_frame->border_color = BORDER_COLORS;
	new_frame->border_width = BORDER_WIDTH;
	new_frame->frame = XCreateWindow(fluorite.display, fluorite.root, 
			0, 
			0,
			fluorite.screen_width - (BORDER_WIDTH * 2),
			fluorite.screen_height - (BORDER_WIDTH * 2),
			BORDER_WIDTH,
			vinfo.depth,
			InputOutput,
			vinfo.visual,
			CWBackPixel | CWBorderPixel | CWColormap | CWEventMask,
			&attribs_set
	);
	XCompositeRedirectWindow(fluorite.display, new_frame->frame, CompositeRedirectAutomatic);

	return new_frame;
}

void fluorite_organise_stack(int mode, int offset)
{
	if (fluorite.workspaces[fluorite.current_workspace].slaves_count == 0)
		return ;
	switch (mode)
	{
		case STACK_NEW:
			for (int i = fluorite.workspaces[fluorite.current_workspace].slaves_count; i >= 1; i--)
				memcpy(fluorite.workspaces[fluorite.current_workspace].slaves_winframes[i], fluorite.workspaces[fluorite.current_workspace].slaves_winframes[i - 1], sizeof(WinFrames));
			memcpy(fluorite.workspaces[fluorite.current_workspace].slaves_winframes[0], fluorite.workspaces[fluorite.current_workspace].tmp_winframe, sizeof(WinFrames));
			break;
		case STACK_DEL:
			for (int i = offset + 1; i <= fluorite.workspaces[fluorite.current_workspace].slaves_count; i++)
				memcpy(fluorite.workspaces[fluorite.current_workspace].slaves_winframes[i - 1], fluorite.workspaces[fluorite.current_workspace].slaves_winframes[i], sizeof(WinFrames));
			free(fluorite.workspaces[fluorite.current_workspace].slaves_winframes[fluorite.workspaces[fluorite.current_workspace].slaves_count]);
			fluorite.workspaces[fluorite.current_workspace].slaves_winframes[fluorite.workspaces[fluorite.current_workspace].slaves_count] = (WinFrames *) malloc(sizeof(WinFrames));
			break;
		case STACK_UP:
			fluorite.workspaces[fluorite.current_workspace].tmp_winframe = (WinFrames *) malloc(sizeof(WinFrames));
			memcpy(fluorite.workspaces[fluorite.current_workspace].tmp_winframe, fluorite.workspaces[fluorite.current_workspace].master_winframe, sizeof(WinFrames));
			memcpy(fluorite.workspaces[fluorite.current_workspace].master_winframe, fluorite.workspaces[fluorite.current_workspace].slaves_winframes[0], sizeof(WinFrames));
			fluorite_organise_stack(STACK_DEL, 0);
			memcpy(fluorite.workspaces[fluorite.current_workspace].slaves_winframes[fluorite.workspaces[fluorite.current_workspace].slaves_count - 1], fluorite.workspaces[fluorite.current_workspace].tmp_winframe, sizeof(WinFrames));
			break;
		case STACK_DOWN:
			fluorite.workspaces[fluorite.current_workspace].tmp_winframe = (WinFrames *) malloc(sizeof(WinFrames));
			memcpy(fluorite.workspaces[fluorite.current_workspace].tmp_winframe, fluorite.workspaces[fluorite.current_workspace].master_winframe, sizeof(WinFrames));
			memcpy(fluorite.workspaces[fluorite.current_workspace].master_winframe, fluorite.workspaces[fluorite.current_workspace].slaves_winframes[fluorite.workspaces[fluorite.current_workspace].slaves_count - 1], sizeof(WinFrames));
			fluorite_organise_stack(STACK_NEW, -1);
			memcpy(fluorite.workspaces[fluorite.current_workspace].slaves_winframes[0], fluorite.workspaces[fluorite.current_workspace].tmp_winframe, sizeof(WinFrames));
			break;
		case SLAVES_UP:
			fluorite.workspaces[fluorite.current_workspace].tmp_winframe = (WinFrames *) malloc(sizeof(WinFrames));
			memcpy(fluorite.workspaces[fluorite.current_workspace].tmp_winframe, fluorite.workspaces[fluorite.current_workspace].slaves_winframes[0], sizeof(WinFrames));
			for (int i = 0; i < fluorite.workspaces[fluorite.current_workspace].slaves_count; i++)
				memcpy(fluorite.workspaces[fluorite.current_workspace].slaves_winframes[i], fluorite.workspaces[fluorite.current_workspace].slaves_winframes[i + 1], sizeof(WinFrames));
			memcpy(fluorite.workspaces[fluorite.current_workspace].slaves_winframes[fluorite.workspaces[fluorite.current_workspace].slaves_count - 1], fluorite.workspaces[fluorite.current_workspace].tmp_winframe, sizeof(WinFrames));
			free(fluorite.workspaces[fluorite.current_workspace].tmp_winframe);
			break;
		case SLAVES_DOWN:
			fluorite.workspaces[fluorite.current_workspace].tmp_winframe = (WinFrames *) malloc(sizeof(WinFrames));
			memcpy(fluorite.workspaces[fluorite.current_workspace].tmp_winframe, fluorite.workspaces[fluorite.current_workspace].slaves_winframes[fluorite.workspaces[fluorite.current_workspace].slaves_count - 1], sizeof(WinFrames));
			fluorite_organise_stack(STACK_NEW, -1);
			free(fluorite.workspaces[fluorite.current_workspace].tmp_winframe);
			break;
	}
}

void fluorite_redraw_windows()
{
	if (fluorite.workspaces[fluorite.current_workspace].frames_count == 0)
		return;

	if (fluorite.workspaces[fluorite.current_workspace].is_fullscreen || fluorite.workspaces[fluorite.current_workspace].frames_count == 1)
	{
		fluorite.workspaces[fluorite.current_workspace].master_winframe->pos_x = 0 + (GAPS * 2);
		fluorite.workspaces[fluorite.current_workspace].master_winframe->pos_y = 0 + (GAPS * 2);
		fluorite.workspaces[fluorite.current_workspace].master_winframe->width = fluorite.screen_width - (BORDER_WIDTH * 2) - (GAPS * 4);
		fluorite.workspaces[fluorite.current_workspace].master_winframe->height = fluorite.screen_height - (BORDER_WIDTH * 2) - (GAPS * 4);
		XResizeWindow(
				fluorite.display, fluorite.workspaces[fluorite.current_workspace].master_winframe->frame,
				fluorite.workspaces[fluorite.current_workspace].master_winframe->width, fluorite.workspaces[fluorite.current_workspace].master_winframe->height
		);
		XMoveWindow(
				fluorite.display, fluorite.workspaces[fluorite.current_workspace].master_winframe->frame,
				fluorite.workspaces[fluorite.current_workspace].master_winframe->pos_x, fluorite.workspaces[fluorite.current_workspace].master_winframe->pos_y
		);
		if (fluorite.workspaces[fluorite.current_workspace].slaves_count > 0)
		{
			for (int i = fluorite.workspaces[fluorite.current_workspace].slaves_count - 1; i >= 0; i--)
			{
				fluorite.workspaces[fluorite.current_workspace].slaves_winframes[i]->pos_x = 0 + (GAPS * 2);
				fluorite.workspaces[fluorite.current_workspace].slaves_winframes[i]->pos_y = 0 + (GAPS * 2);
				fluorite.workspaces[fluorite.current_workspace].slaves_winframes[i]->width = fluorite.screen_width - (BORDER_WIDTH * 2) - (GAPS * 4);
				fluorite.workspaces[fluorite.current_workspace].slaves_winframes[i]->height = fluorite.screen_height - (BORDER_WIDTH * 2) - (GAPS * 4);
				if (fluorite.workspaces[fluorite.current_workspace].slaves_winframes[i]->width < 0 || fluorite.workspaces[fluorite.current_workspace].slaves_winframes[i]->height < 0)
				{
					fluorite.workspaces[fluorite.current_workspace].master_offset -= 25;
					break;
				}
				XResizeWindow(
						fluorite.display, fluorite.workspaces[fluorite.current_workspace].slaves_winframes[i]->frame,
						fluorite.workspaces[fluorite.current_workspace].slaves_winframes[i]->width, fluorite.workspaces[fluorite.current_workspace].slaves_winframes[i]->height
						);
				XMoveWindow(
						fluorite.display, fluorite.workspaces[fluorite.current_workspace].slaves_winframes[i]->frame,
						fluorite.workspaces[fluorite.current_workspace].slaves_winframes[i]->pos_x, fluorite.workspaces[fluorite.current_workspace].slaves_winframes[i]->pos_y
						);
				XRaiseWindow(fluorite.display, fluorite.workspaces[fluorite.current_workspace].slaves_winframes[i]->frame);
				XMoveResizeWindow(fluorite.display, fluorite.workspaces[fluorite.current_workspace].slaves_winframes[i]->window, 0, 0, fluorite.workspaces[fluorite.current_workspace].slaves_winframes[i]->width, fluorite.workspaces[fluorite.current_workspace].slaves_winframes[i]->height);
			}
		}
	}

	else
	{
		fluorite.workspaces[fluorite.current_workspace].master_winframe->pos_x = 0 + (GAPS * 2);
		fluorite.workspaces[fluorite.current_workspace].master_winframe->pos_y = 0 + (GAPS * 2);
		fluorite.workspaces[fluorite.current_workspace].master_winframe->width = fluorite.screen_width - (BORDER_WIDTH * 2) - (GAPS * 4);
		fluorite.workspaces[fluorite.current_workspace].master_winframe->height = fluorite.screen_height - (BORDER_WIDTH * 2) - (GAPS * 4);
		XResizeWindow(
				fluorite.display, fluorite.workspaces[fluorite.current_workspace].master_winframe->frame,
				fluorite.workspaces[fluorite.current_workspace].master_winframe->width, fluorite.workspaces[fluorite.current_workspace].master_winframe->height
				);
		XMoveWindow(
				fluorite.display, fluorite.workspaces[fluorite.current_workspace].master_winframe->frame,
				fluorite.workspaces[fluorite.current_workspace].master_winframe->pos_x, fluorite.workspaces[fluorite.current_workspace].master_winframe->pos_y
				);
		if (fluorite.workspaces[fluorite.current_workspace].slaves_count > 0)
		{
			fluorite.workspaces[fluorite.current_workspace].master_winframe->width = fluorite.screen_width / 2 - (BORDER_WIDTH * 2) - (GAPS * 3) + fluorite.workspaces[fluorite.current_workspace].master_offset;
			if (fluorite.workspaces[fluorite.current_workspace].master_winframe->width < 0)
			{
				fluorite.workspaces[fluorite.current_workspace].master_offset += 25;
				return ;
			}
			XResizeWindow(
					fluorite.display, fluorite.workspaces[fluorite.current_workspace].master_winframe->frame,
					fluorite.workspaces[fluorite.current_workspace].master_winframe->width, fluorite.workspaces[fluorite.current_workspace].master_winframe->height
					);
			int position_offset;
			int size_offset;

			position_offset = 0;
			size_offset = STACK_OFFSET * (fluorite.workspaces[fluorite.current_workspace].slaves_count - 1) * 10;
			for (int i = fluorite.workspaces[fluorite.current_workspace].slaves_count - 1; i >= 0; i--)
			{
				fluorite.workspaces[fluorite.current_workspace].slaves_winframes[i]->pos_x = fluorite.screen_width / 2 + (GAPS) + (position_offset / fluorite.workspaces[fluorite.current_workspace].slaves_count) + fluorite.workspaces[fluorite.current_workspace].master_offset;
				fluorite.workspaces[fluorite.current_workspace].slaves_winframes[i]->pos_y = 0 + (GAPS * 2) + (position_offset / fluorite.workspaces[fluorite.current_workspace].slaves_count);
				fluorite.workspaces[fluorite.current_workspace].slaves_winframes[i]->width = fluorite.screen_width / 2 - (BORDER_WIDTH * 2) - (GAPS * 3) - (size_offset / fluorite.workspaces[fluorite.current_workspace].slaves_count) - fluorite.workspaces[fluorite.current_workspace].master_offset;
				fluorite.workspaces[fluorite.current_workspace].slaves_winframes[i]->height = fluorite.screen_height - (BORDER_WIDTH * 2) - (GAPS * 4) - (size_offset / fluorite.workspaces[fluorite.current_workspace].slaves_count);
				if (fluorite.workspaces[fluorite.current_workspace].slaves_winframes[i]->width < 0 || fluorite.workspaces[fluorite.current_workspace].slaves_winframes[i]->height < 0)
				{
					fluorite.workspaces[fluorite.current_workspace].master_offset -= 25;
					break;
				}
				XResizeWindow(
						fluorite.display, fluorite.workspaces[fluorite.current_workspace].slaves_winframes[i]->frame,
						fluorite.workspaces[fluorite.current_workspace].slaves_winframes[i]->width, fluorite.workspaces[fluorite.current_workspace].slaves_winframes[i]->height
						);
				XMoveWindow(
						fluorite.display, fluorite.workspaces[fluorite.current_workspace].slaves_winframes[i]->frame,
						fluorite.workspaces[fluorite.current_workspace].slaves_winframes[i]->pos_x, fluorite.workspaces[fluorite.current_workspace].slaves_winframes[i]->pos_y
						);
				XRaiseWindow(fluorite.display, fluorite.workspaces[fluorite.current_workspace].slaves_winframes[i]->frame);
				XMoveResizeWindow(fluorite.display, fluorite.workspaces[fluorite.current_workspace].slaves_winframes[i]->window, 0, 0, fluorite.workspaces[fluorite.current_workspace].slaves_winframes[i]->width, fluorite.workspaces[fluorite.current_workspace].slaves_winframes[i]->height);
				position_offset += STACK_OFFSET * 10;
			}
		}
	}
	XRaiseWindow(fluorite.display, fluorite.workspaces[fluorite.current_workspace].master_winframe->frame);
	XResizeWindow(fluorite.display, fluorite.workspaces[fluorite.current_workspace].master_winframe->window, fluorite.workspaces[fluorite.current_workspace].master_winframe->width, fluorite.workspaces[fluorite.current_workspace].master_winframe->height);
	XSetInputFocus(fluorite.display, fluorite.workspaces[fluorite.current_workspace].master_winframe->window, RevertToPointerRoot, CurrentTime);
}

void fluorite_handle_unmapping(Window e)
{
	int keep_workspace = fluorite.current_workspace;

	for (int i = 0; i <= MAX_WORKSPACES; i++)
	{
		if (fluorite.workspaces[i].frames_count <= 0)
			continue;
		fluorite.current_workspace = i;
		if (fluorite.workspaces[fluorite.current_workspace].master_winframe->window == e)
		{
			XReparentWindow(fluorite.display, fluorite.workspaces[fluorite.current_workspace].master_winframe->frame, fluorite.root, 0, 0);
			if (keep_workspace == i)
			{
				XUnmapWindow(fluorite.display, fluorite.workspaces[fluorite.current_workspace].master_winframe->frame);
				XDestroyWindow(fluorite.display, fluorite.workspaces[fluorite.current_workspace].master_winframe->frame);
				XSetInputFocus(fluorite.display, fluorite.root, RevertToPointerRoot, CurrentTime);
			}
			free(fluorite.workspaces[fluorite.current_workspace].master_winframe);
			fluorite.workspaces[fluorite.current_workspace].frames_count--;
			if (fluorite.workspaces[fluorite.current_workspace].slaves_count > 0)
			{
				fluorite.workspaces[fluorite.current_workspace].master_winframe = (WinFrames *) malloc(sizeof(WinFrames));
				memcpy(fluorite.workspaces[fluorite.current_workspace].master_winframe, fluorite.workspaces[fluorite.current_workspace].slaves_winframes[0], sizeof(WinFrames));
				fluorite.workspaces[fluorite.current_workspace].slaves_count--;
				fluorite_organise_stack(STACK_DEL, 0);
				if (keep_workspace == i)
					fluorite_redraw_windows();
				break;
			}
		}
		else
		{
			if (fluorite.workspaces[i].slaves_count <= 0)
				continue;
			int stack_offset = 0;
			int closed = False;

			while (stack_offset < fluorite.workspaces[fluorite.current_workspace].slaves_count)
			{
				if (fluorite.workspaces[fluorite.current_workspace].slaves_winframes[stack_offset]->window == e)
				{
					XReparentWindow(fluorite.display, fluorite.workspaces[fluorite.current_workspace].slaves_winframes[stack_offset]->frame, fluorite.root, 0, 0);
					if (keep_workspace == i)
					{
						XUnmapWindow(fluorite.display, fluorite.workspaces[fluorite.current_workspace].slaves_winframes[stack_offset]->frame);
						XDestroyWindow(fluorite.display, fluorite.workspaces[fluorite.current_workspace].slaves_winframes[stack_offset]->frame);
					}
					fluorite.workspaces[fluorite.current_workspace].frames_count--;
					fluorite.workspaces[fluorite.current_workspace].slaves_count--;
					closed = True;
					break;
				}
				stack_offset++;
			}
			if (closed)
				fluorite_organise_stack(STACK_DEL, stack_offset);
			if (keep_workspace == i)
				fluorite_redraw_windows();
		}
	}
	fluorite.current_workspace = keep_workspace;
}

int main(void)
{
	fluorite_init();
	fluorite_run();
	fluorite_clean();
	return 0;
}
