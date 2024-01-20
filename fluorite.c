#include "config_fluorite.h"
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <X11/Xcursor/Xcursor.h>
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
	Display		*display;
	Window		root;
	int			screen;
	int			screen_width;
	int			screen_height;
	int			running;
	WinFrames	*master_winframe;
	WinFrames	**slaves_winframes;
	WinFrames	*tmp_winframe;
	int			frames_count;
	int			slaves_count;
	int			master_offset;
} Fluorite;

static Fluorite fluorite;

// Core functions
static void fluorite_init();
static void fluorite_run();
static void fluorite_clean();

// Core utilities
static void fluorite_handle_configuration(XConfigureRequestEvent e);
static void fluorite_handle_mapping(XMapRequestEvent e);
static void fluorite_handle_unmapping(Window e);
static void fluorite_handle_keys(XKeyPressedEvent e);
static WinFrames *fluorite_create_winframe();
static void fluorite_organise_stack(int mode);
static void fluorite_redraw_windows();
void dwm_grabkeys();

// Bindings functions (defined in config_fluorite.h)
void fluorite_execute(char *argument)
{
	strcat(argument, " &");
	system(argument);
}

void fluorite_close_window()
{
	if (fluorite.frames_count == 0)
		return ;

	if (fluorite.master_winframe->window)
		XKillClient(fluorite.display, fluorite.master_winframe->window);
	else
		XDestroyWindow(fluorite.display, fluorite.master_winframe->frame);
}

void fluorite_layout_change(int mode)
{
	switch(mode)
	{
		case FOCUS_TOP:
			fluorite_organise_stack(STACK_UP);
			fluorite_redraw_windows();
			break;
		case FOCUS_BOTTOM:
			fluorite_organise_stack(STACK_DOWN);
			fluorite_redraw_windows();
			break;
		case SLAVES_UP:
			fluorite_organise_stack(SLAVES_UP);
			fluorite_redraw_windows();
			break;
		case SLAVES_DOWN:
			fluorite_organise_stack(SLAVES_DOWN);
			fluorite_redraw_windows();
			break;
		case BIGGER_MASTER:
			fluorite.master_offset += 25;
			fluorite_redraw_windows();
			break;
		case SMALLER_MASTER:
			fluorite.master_offset -= 25;
			fluorite_redraw_windows();
			break;
	}
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

	fluorite.frames_count = 0;
	fluorite.master_offset = 0;

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

	fluorite.slaves_winframes = (WinFrames **) malloc(sizeof(WinFrames *) * MAX_WINDOWS);
	for (int i = 0; i <= MAX_WINDOWS; i++)
		fluorite.slaves_winframes[i] = (WinFrames *) malloc(sizeof(WinFrames));

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
				if (fluorite.master_winframe)
					fluorite_handle_unmapping(ev.xunmap.window);
				break;
			case ButtonPress:
				break;
			case KeyPress:
				fluorite_handle_keys(ev.xkey);
				break;
		}
	}
}

void fluorite_clean()
{
	// Needs more testing
	if (fluorite.master_winframe)
		free(fluorite.master_winframe);
	if (fluorite.slaves_winframes)
	{
		for (int i = 0; fluorite.slaves_winframes[i]; i++)
			free(fluorite.slaves_winframes[i]);
		free(fluorite.slaves_winframes);
	}
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
				/* skip modifier codes, we do that ourselves */
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
	if (fluorite.frames_count == MAX_WINDOWS)
		return ;

	if (fluorite.frames_count > 0)
	{
		fluorite.tmp_winframe = (WinFrames *) malloc(sizeof(WinFrames));
		memcpy(fluorite.tmp_winframe, fluorite.master_winframe, sizeof(WinFrames));
		free(fluorite.master_winframe);
		fluorite.slaves_count++;
	}

	fluorite.master_winframe = (WinFrames *) malloc(sizeof(WinFrames));
	fluorite.frames_count++;
	fluorite.master_winframe = fluorite_create_winframe();
	fluorite.master_winframe->window = e.window;

	XGrabServer(fluorite.display);
	XSelectInput(fluorite.display, fluorite.master_winframe->frame, SubstructureNotifyMask | SubstructureRedirectMask);
	XWMHints *source_hints = XGetWMHints(fluorite.display, e.window);
	if (source_hints)
	{
		XSetWMHints(fluorite.display, fluorite.master_winframe->frame, source_hints);
		XFree(source_hints);
	}
	XReparentWindow(fluorite.display, e.window, fluorite.master_winframe->frame, 0, 0);
	XResizeWindow(
			fluorite.display,
			e.window,
			fluorite.master_winframe->width,
			fluorite.master_winframe->height
	);
	XMoveResizeWindow(fluorite.display, fluorite.master_winframe->window, 0, 0, fluorite.master_winframe->width, fluorite.master_winframe->height);
	XMapWindow(fluorite.display, fluorite.master_winframe->frame);
	XRaiseWindow(fluorite.display, fluorite.master_winframe->frame);
	XMapWindow(fluorite.display, e.window);
	XUngrabServer(fluorite.display);
	XSync(fluorite.display, True);

	if (fluorite.slaves_count > 0)
	{
		fluorite_organise_stack(STACK_NEW);
		free(fluorite.tmp_winframe);
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

WinFrames *fluorite_create_winframe()
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

void fluorite_organise_stack(int mode)
{
	if (fluorite.slaves_count == 0)
		return ;
	switch (mode)
	{
		case STACK_NEW:
			for (int i = fluorite.slaves_count; i >= 1; i--)
				memcpy(fluorite.slaves_winframes[i], fluorite.slaves_winframes[i - 1], sizeof(WinFrames));
			memcpy(fluorite.slaves_winframes[0], fluorite.tmp_winframe, sizeof(WinFrames));
			break;
		case STACK_DEL:
			for (int i = 1; i <= fluorite.slaves_count; i++)
				memcpy(fluorite.slaves_winframes[i - 1], fluorite.slaves_winframes[i], sizeof(WinFrames));
			free(fluorite.slaves_winframes[fluorite.slaves_count]);
			fluorite.slaves_winframes[fluorite.slaves_count] = (WinFrames *) malloc(sizeof(WinFrames));
			break;
		case STACK_UP:
			fluorite.tmp_winframe = (WinFrames *) malloc(sizeof(WinFrames));
			memcpy(fluorite.tmp_winframe, fluorite.master_winframe, sizeof(WinFrames));
			memcpy(fluorite.master_winframe, fluorite.slaves_winframes[0], sizeof(WinFrames));
			fluorite_organise_stack(STACK_DEL);
			memcpy(fluorite.slaves_winframes[fluorite.slaves_count - 1], fluorite.tmp_winframe, sizeof(WinFrames));
			fluorite_redraw_windows();
			break;
		case STACK_DOWN:
			fluorite.tmp_winframe = (WinFrames *) malloc(sizeof(WinFrames));
			memcpy(fluorite.tmp_winframe, fluorite.master_winframe, sizeof(WinFrames));
			memcpy(fluorite.master_winframe, fluorite.slaves_winframes[fluorite.slaves_count - 1], sizeof(WinFrames));
			fluorite_organise_stack(STACK_NEW);
			memcpy(fluorite.slaves_winframes[0], fluorite.tmp_winframe, sizeof(WinFrames));
			fluorite_redraw_windows();
			break;
		case SLAVES_UP:
			fluorite.tmp_winframe = (WinFrames *) malloc(sizeof(WinFrames));
			memcpy(fluorite.tmp_winframe, fluorite.slaves_winframes[0], sizeof(WinFrames));
			for (int i = 0; i < fluorite.slaves_count; i++)
				memcpy(fluorite.slaves_winframes[i], fluorite.slaves_winframes[i + 1], sizeof(WinFrames));
			memcpy(fluorite.slaves_winframes[fluorite.slaves_count - 1], fluorite.tmp_winframe, sizeof(WinFrames));
			free(fluorite.tmp_winframe);
			break;
		case SLAVES_DOWN:
			fluorite.tmp_winframe = (WinFrames *) malloc(sizeof(WinFrames));
			memcpy(fluorite.tmp_winframe, fluorite.slaves_winframes[fluorite.slaves_count - 1], sizeof(WinFrames));
			fluorite_organise_stack(STACK_NEW);
			free(fluorite.tmp_winframe);
			break;
	}
	XSetInputFocus(fluorite.display, fluorite.master_winframe->window, RevertToPointerRoot, CurrentTime);
}

void fluorite_redraw_windows()
{
	if (fluorite.frames_count == 0)
		return;

	XGrabServer(fluorite.display);
	fluorite.master_winframe->pos_x = 0 + (GAPS * 2);
	fluorite.master_winframe->pos_y = 0 + (GAPS * 2);
	fluorite.master_winframe->width = fluorite.screen_width - (BORDER_WIDTH * 2) - (GAPS * 4);
	fluorite.master_winframe->height = fluorite.screen_height - (BORDER_WIDTH * 2) - (GAPS * 4);
	XResizeWindow(
			fluorite.display, fluorite.master_winframe->frame,
			fluorite.master_winframe->width, fluorite.master_winframe->height
	);
	XMoveWindow(
			fluorite.display, fluorite.master_winframe->frame,
			fluorite.master_winframe->pos_x, fluorite.master_winframe->pos_y
	);
	if (fluorite.slaves_count > 0)
	{
		fluorite.master_winframe->width = fluorite.screen_width / 2 - (BORDER_WIDTH * 2) - (GAPS * 2) + fluorite.master_offset;
		if (fluorite.master_winframe->width < 0)
			return ;
		XResizeWindow(
				fluorite.display, fluorite.master_winframe->frame,
				fluorite.master_winframe->width, fluorite.master_winframe->height
		);
		int position_offset;
		int size_offset;

		position_offset = 0;
		size_offset = STACK_OFFSET * (fluorite.slaves_count - 1) * 10;
		for (int i = fluorite.slaves_count - 1; i >= 0; i--)
		{
			fluorite.slaves_winframes[i]->pos_x = fluorite.screen_width / 2 + (GAPS * 2) + (position_offset / fluorite.slaves_count) + fluorite.master_offset;
			fluorite.slaves_winframes[i]->pos_y = 0 + (GAPS * 2) + (position_offset / fluorite.slaves_count);
			fluorite.slaves_winframes[i]->width = fluorite.screen_width / 2 - (BORDER_WIDTH * 2) - (GAPS * 4) - (size_offset / fluorite.slaves_count) - fluorite.master_offset;
			fluorite.slaves_winframes[i]->height = fluorite.screen_height - (BORDER_WIDTH * 2) - (GAPS * 4) - (size_offset / fluorite.slaves_count);
			if (fluorite.slaves_winframes[i]->width < 0 || fluorite.slaves_winframes[i]->height < 0)
				return ;
			XResizeWindow(
					fluorite.display, fluorite.slaves_winframes[i]->frame,
					fluorite.slaves_winframes[i]->width, fluorite.slaves_winframes[i]->height
			);
			XMoveWindow(
				fluorite.display, fluorite.slaves_winframes[i]->frame,
				fluorite.slaves_winframes[i]->pos_x, fluorite.slaves_winframes[i]->pos_y
			);
			XRaiseWindow(fluorite.display, fluorite.slaves_winframes[i]->frame);
			XMoveResizeWindow(fluorite.display, fluorite.slaves_winframes[i]->window, 0, 0, fluorite.slaves_winframes[i]->width, fluorite.slaves_winframes[i]->height);
			position_offset += STACK_OFFSET * 10;
		}
	}
	XRaiseWindow(fluorite.display, fluorite.master_winframe->frame);
	XResizeWindow(fluorite.display, fluorite.master_winframe->window, fluorite.master_winframe->width, fluorite.master_winframe->height);
	XUngrabServer(fluorite.display);
	XSync(fluorite.display, True);
	XChangeProperty(fluorite.display, fluorite.root, XInternAtom(fluorite.display, "_NET_ACTIVE_WINDOW", False), XA_WINDOW, 32, PropModeReplace, (unsigned char *) &(fluorite.master_winframe->window), 1);
}

void fluorite_handle_unmapping(Window e)
{
	if (fluorite.frames_count == 0)
		return;

	XGrabServer(fluorite.display);
	fluorite.frames_count--;
	if (fluorite.master_winframe->window == e)
	{
		XReparentWindow(fluorite.display, fluorite.master_winframe->frame, fluorite.root, 0, 0);
		XUnmapWindow(fluorite.display, fluorite.master_winframe->frame);
		XDestroyWindow(fluorite.display, fluorite.master_winframe->frame);
		XSetInputFocus(fluorite.display, fluorite.root, RevertToPointerRoot, CurrentTime);
		free(fluorite.master_winframe);
		if (fluorite.slaves_count > 0)
		{
			fluorite.master_winframe = (WinFrames *) malloc(sizeof(WinFrames));
			memcpy(fluorite.master_winframe, fluorite.slaves_winframes[0], sizeof(WinFrames));
			XSetInputFocus(fluorite.display, fluorite.master_winframe->frame, RevertToPointerRoot, CurrentTime);
			XRaiseWindow(fluorite.display, fluorite.master_winframe->frame);
			fluorite_organise_stack(STACK_DEL);
			fluorite.slaves_count--;
		}
	}
	else
	{
		int i;

		i = 0;
		while (i <= fluorite.slaves_count)
		{
			if (fluorite.slaves_winframes[i]->window == e)
			{
				XReparentWindow(fluorite.display, fluorite.slaves_winframes[i]->frame, fluorite.root, 0, 0);
				XUnmapWindow(fluorite.display, fluorite.slaves_winframes[i]->frame);
				XDestroyWindow(fluorite.display, fluorite.slaves_winframes[i]->frame);
				XSetInputFocus(fluorite.display, fluorite.root, RevertToPointerRoot, CurrentTime);
				free(fluorite.slaves_winframes[i]);
				fluorite.slaves_winframes[i] = (WinFrames *) malloc(sizeof(WinFrames));
				break;
			}
			i++;
		}
		while (i <= fluorite.slaves_count)
		{
			memcpy(fluorite.slaves_winframes[i], fluorite.slaves_winframes[i + 1], sizeof(WinFrames));
			i++;
		}
		fluorite.slaves_count--;
	}
	XUngrabServer(fluorite.display);
	XSync(fluorite.display, True);
	fluorite_redraw_windows();
}

int main(void)
{
	fluorite_init();
	fluorite_run();
	fluorite_clean();
	return 0;
}
