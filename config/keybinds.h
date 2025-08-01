#include <X11/X.h>
#include <X11/keysym.h>
#include <X11/XF86keysym.h>
#include "options.h"

typedef struct
{
	unsigned int	mod;
	KeySym			key;
	void			(*func)();
} Bindings;

enum MODE {
	UP,
	DOWN,
	RESET
};

enum LAYOUT {
	FLUORITE,
	DWM,
	STACKED
};

#define METAKEY					Mod4Mask		/* key that will be used for bindings */

// Fluorite Core Functions
void FExecute(char *argument);
void FQuit();
void FCloseWindow();
void FRotateWindows(int mode);
void FRotateStackWindows(int mode);
void FChangeMasterOffset(int mode);
void FSwapWithMaster();
void FFocusNext();
void FFocusPrev();
void FNextWorkspace();
void FPrevWorkspace();
void FShowWorkspace(int ws);
void FSendWindowToWorkspace(int ws);
void FTileWindow();
void FChangeLayout(int layout);
void FTileAllWindows();
void FFullscreenToggle();
void FFloatingHideShow();
void FSendWindowToNextWorkspace();
void FSendWindowToPrevWorkspace();
void FFocusNextMonitor();
void FResetMasterOffset();

// User defined Functions
static void terminal() { char prog[255] = "st"; FExecute(prog); }
static void browser()  { char prog[255] = "firefox"; FExecute(prog); }
static void runner_app()   { char prog[255] = "dmenu_run_desktop"; FExecute(prog); }
static void runner_cmd()   { char prog[255] = "dmenu_run"; FExecute(prog); }
static void themes()   { char prog[255] = "~/tools/suckless_tools/scripts/fluorite_theme.sh"; FExecute(prog); }
static void brightness_up() { char prog[255] = "brightnessctl set 5%+"; FExecute(prog);};
static void brightness_down() { char prog[255] = "brightnessctl set 5%-"; FExecute(prog);};
static void volume_mute() { char prog[255] = "pactl set-sink-mute 0 toggle"; FExecute(prog);};
static void volume_up() { char prog[255] = "pactl set-sink-volume 0 +5%"; FExecute(prog);};
static void volume_down() { char prog[255] = "pactl set-sink-volume 0 -5%"; FExecute(prog);};
static void locking() { char prog[255] = "elogind-inhibit --what=sleep xsecurelock"; FExecute(prog); }
static void dmenu_xresources() { char prog[255] = "~/tools/suckless_tools/scripts/dmenu_xresources.sh"; FExecute(prog); }
static void st_music() { char prog[255] = "st -c st_music -g 100x30 mocp"; FExecute(prog); }
static void dmenu_music() { char prog[255] = "~/tools/suckless_tools/scripts/dmenu_music.sh"; FExecute(prog); }
static void dmenu_font() { char prog[255] = "~/tools/suckless_tools/scripts/st_font.sh font"; FExecute(prog); }
static void dmenu_font_size() { char prog[255] = "~/tools/suckless_tools/scripts/st_font.sh"; FExecute(prog); }
static void print_screen() { char prog[255] = "~/tools/suckless_tools/scripts/print_screen.sh"; FExecute(prog); }

// Fluorite based user defined function
static void rotate_windows_up() { FRotateWindows(UP); }
static void rotate_windows_down() { FRotateWindows(DOWN); }
static void rotate_slaves_windows_up() { FRotateStackWindows(UP); }
static void rotate_slaves_windows_down() { FRotateStackWindows(DOWN); }
static void master_offset_up() { FChangeMasterOffset(UP); }
static void master_offset_down() { FChangeMasterOffset(DOWN); }
static void master_offset_reset() { FChangeMasterOffset(RESET); }
static void workspace_one() { FShowWorkspace(0); }
static void workspace_two() { FShowWorkspace(1); }
static void workspace_three() { FShowWorkspace(2); }
static void workspace_four() { FShowWorkspace(3); }
static void workspace_five() { FShowWorkspace(4); }
static void workspace_six() { FShowWorkspace(5); }
static void workspace_seven() { FShowWorkspace(6); }
static void workspace_eight() { FShowWorkspace(7); }
static void workspace_nine() { FShowWorkspace(8); }
static void workspace_ten() { FShowWorkspace(9); }
static void window_workspace_one() { FSendWindowToWorkspace(0); }
static void window_workspace_two() { FSendWindowToWorkspace(1); }
static void window_workspace_three() { FSendWindowToWorkspace(2); }
static void window_workspace_four() { FSendWindowToWorkspace(3); }
static void window_workspace_five() { FSendWindowToWorkspace(4); }
static void window_workspace_six() { FSendWindowToWorkspace(5); }
static void window_workspace_seven() { FSendWindowToWorkspace(6); }
static void window_workspace_eight() { FSendWindowToWorkspace(7); }
static void window_workspace_nine() { FSendWindowToWorkspace(8); }
static void window_workspace_ten() { FSendWindowToWorkspace(9); }
static void layout_fluorite() { FChangeLayout(FLUORITE); }
static void layout_dwm() { FChangeLayout(DWM); }
static void layout_auto()
{
	if (STARTING_LAYOUT == DWM)
		layout_fluorite();
	else
		layout_dwm();
}
static void layout_stacked() { FChangeLayout(STACKED); }

static const Bindings bind[] = {
  {METAKEY,				XK_Return,			terminal},
  {METAKEY,     		XK_w,				browser},
  {METAKEY,     		XK_k,				rotate_windows_up},
  {METAKEY,     		XK_l,				rotate_windows_down},
  {METAKEY,     		XK_j,				master_offset_down},
  {METAKEY,     		XK_m,				master_offset_up},
  {METAKEY,     		XK_equal,			master_offset_reset},
  {METAKEY,     		XK_space,			FTileWindow},
  {METAKEY,     		XK_s,				layout_stacked},
  {METAKEY,     		XK_t,				themes},
  {METAKEY,     		XK_f,				FFullscreenToggle},
  {METAKEY,     		XK_c,				dmenu_xresources},
  {METAKEY,				XK_d,	   			runner_app},
  {METAKEY,				XK_r,	   			layout_auto},
  {METAKEY, 			XK_Print,			print_screen},

  {METAKEY|ShiftMask,	XK_Return,			FSwapWithMaster},
  {METAKEY|ShiftMask, 	XK_q,				FCloseWindow},
  {METAKEY|ShiftMask, 	XK_p,    			FQuit},
  {METAKEY|ShiftMask, 	XK_d,	   			runner_cmd},
  {METAKEY|ShiftMask,	XK_k,	   			rotate_slaves_windows_down},
  {METAKEY|ShiftMask, 	XK_l,	   			rotate_slaves_windows_up},
  {METAKEY|ShiftMask, 	XK_e,	   			locking},
  {METAKEY|ShiftMask,	XK_m,				dmenu_music},
  {METAKEY|ShiftMask, 	XK_c,				dmenu_font},
  {METAKEY|ShiftMask,	XK_space,			FFloatingHideShow},


  {METAKEY,				XK_ampersand, 		workspace_one},
  {METAKEY,				XK_eacute,    		workspace_two},
  {METAKEY,				XK_quotedbl,  		workspace_three},
  {METAKEY,				XK_apostrophe,		workspace_four},
  {METAKEY,				XK_parenleft, 		workspace_five},
  {METAKEY,				XK_minus,     		workspace_six},
  {METAKEY,				XK_egrave,    		workspace_seven},
  {METAKEY,				XK_underscore,		workspace_eight},
  {METAKEY,				XK_ccedilla,  		workspace_nine},
  {METAKEY,				XK_agrave,			workspace_ten},
  {METAKEY,				XK_equal,			FResetMasterOffset},

  {METAKEY|ShiftMask,	XK_ampersand, 		window_workspace_one},
  {METAKEY|ShiftMask,	XK_eacute,    		window_workspace_two},
  {METAKEY|ShiftMask,	XK_quotedbl,  		window_workspace_three},
  {METAKEY|ShiftMask,	XK_apostrophe,		window_workspace_four},
  {METAKEY|ShiftMask,	XK_parenleft, 		window_workspace_five},
  {METAKEY|ShiftMask,	XK_minus,     		window_workspace_six},
  {METAKEY|ShiftMask,	XK_egrave,    		window_workspace_seven},
  {METAKEY|ShiftMask,	XK_underscore,		window_workspace_eight},
  {METAKEY|ShiftMask,	XK_ccedilla,  		window_workspace_nine},
  {METAKEY|ShiftMask,	XK_agrave,			window_workspace_ten},

  {METAKEY|ControlMask,	XK_j,				FPrevWorkspace},
  {METAKEY|ControlMask,	XK_m,				FNextWorkspace},
  {METAKEY|ControlMask,	XK_space,			FTileAllWindows},
  {METAKEY|ControlMask,	XK_c,				dmenu_font_size},

  {METAKEY,				XK_n,				FFocusNext},
  {METAKEY|ShiftMask,	XK_n,				FFocusPrev},
  {METAKEY,				XK_comma,			FFocusNextMonitor},

  {METAKEY|ControlMask|ShiftMask,	XK_j,	FSendWindowToPrevWorkspace},
  {METAKEY|ControlMask|ShiftMask,	XK_m,	FSendWindowToNextWorkspace},

  {0,				XF86XK_MonBrightnessUp,		brightness_up},
  {0,				XF86XK_MonBrightnessDown,	brightness_down},
  {0,				XF86XK_AudioMute,			volume_mute},
  {0,				XF86XK_AudioRaiseVolume,	volume_up},
  {0,				XF86XK_AudioLowerVolume,	volume_down},
  {0,				XF86XK_Favorites,			st_music},
};
