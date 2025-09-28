#include <X11/X.h>
#include <X11/keysym.h>
#include <X11/XF86keysym.h>

typedef struct
{
	unsigned int	mod;
	KeySym			key;
	void			(*func)();
} Bindings;

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
void FReloadXresources();
void FAddWindowToScratchpad();
void FDelWindowFromScratchpad();
void FScratchpadHideShow();
void FCenterScratchpadWindow();
void FToggleFixedStrut();
void FCycleLayouts();

// User defined Functions
// static void terminal() { char prog[255] = "st"; FExecute(prog); }
// static void browser()  { char prog[255] = "firefox"; FExecute(prog); }
// static void runner_app()   { char prog[255] = "dmenu_run_desktop"; FExecute(prog); }
// static void runner_cmd()   { char prog[255] = "dmenu_run"; FExecute(prog); }
// static void themes()   { char prog[255] = "~/tools/suckless_tools/scripts/fluorite_theme.sh"; FExecute(prog); }
// static void brightness_up() { char prog[255] = "brightnessctl set 5%+"; FExecute(prog);};
// static void brightness_down() { char prog[255] = "brightnessctl set 5%-"; FExecute(prog);};
// static void volume_mute() { char prog[255] = "pactl set-sink-mute @DEFAULT_SINK@ toggle"; FExecute(prog);};
// static void volume_up() { char prog[255] = "pactl set-sink-volume @DEFAULT_SINK@ +5%"; FExecute(prog);};
// static void volume_down() { char prog[255] = "pactl set-sink-volume @DEFAULT_SINK@ -5%"; FExecute(prog);};
// static void locking() { char prog[255] = "elogind-inhibit --what=sleep xsecurelock"; FExecute(prog); }
// static void dmenu_xresources() { char prog[255] = "~/tools/suckless_tools/scripts/dmenu_xresources.sh"; FExecute(prog); }
// static void st_music() { char prog[255] = "st -c st_music -g 100x30 mocp"; FExecute(prog); }
// // static void dmenu_music() { char prog[255] = "~/tools/suckless_tools/scripts/dmenu_music.sh"; FExecute(prog); }
// static void dmenu_font() { char prog[255] = "~/tools/suckless_tools/scripts/st_font.sh font"; FExecute(prog); }
// static void dmenu_font_size() { char prog[255] = "~/tools/suckless_tools/scripts/st_font.sh"; FExecute(prog); }
// static void print_screen() { char prog[255] = "~/tools/suckless_tools/scripts/print_screen.sh"; FExecute(prog); }
// static void toggle_mic() { char prog[255] = "pactl set-source-mute 1 toggle && pactl set-source-mute 2 toggle"; FExecute(prog); }
// static void dmenu_hdmi() { char prog[255] = "~/tools/suckless_tools/scripts/dmenu_hdmiplug.sh"; FExecute(prog); }
// static void dmenu_polybar_style() { char prog[255] = "~/tools/suckless_tools/scripts/dmenu_polybar_style.sh"; FExecute(prog); }

// // Fluorite based user defined function
// static void rotate_windows_up() { FRotateWindows(UP); }
// static void rotate_windows_down() { FRotateWindows(DOWN); }
// static void rotate_slaves_windows_up() { FRotateStackWindows(UP); }
// static void rotate_slaves_windows_down() { FRotateStackWindows(DOWN); }
// static void master_offset_up() { FChangeMasterOffset(UP); }
// static void master_offset_down() { FChangeMasterOffset(DOWN); }
// static void master_offset_reset() { FChangeMasterOffset(RESET); }
// static void workspace_one() { FShowWorkspace(0); }
// static void workspace_two() { FShowWorkspace(1); }
// static void workspace_three() { FShowWorkspace(2); }
// static void workspace_four() { FShowWorkspace(3); }
// static void workspace_five() { FShowWorkspace(4); }
// static void workspace_six() { FShowWorkspace(5); }
// static void workspace_seven() { FShowWorkspace(6); }
// static void workspace_eight() { FShowWorkspace(7); }
// static void workspace_nine() { FShowWorkspace(8); }
// static void workspace_ten() { FShowWorkspace(9); }
// static void window_workspace_one() { FSendWindowToWorkspace(0); }
// static void window_workspace_two() { FSendWindowToWorkspace(1); }
// static void window_workspace_three() { FSendWindowToWorkspace(2); }
// static void window_workspace_four() { FSendWindowToWorkspace(3); }
// static void window_workspace_five() { FSendWindowToWorkspace(4); }
// static void window_workspace_six() { FSendWindowToWorkspace(5); }
// static void window_workspace_seven() { FSendWindowToWorkspace(6); }
// static void window_workspace_eight() { FSendWindowToWorkspace(7); }
// static void window_workspace_nine() { FSendWindowToWorkspace(8); }
// static void window_workspace_ten() { FSendWindowToWorkspace(9); }
// // static void layout_cascade() { FChangeLayout(CASCADE); }
// // static void layout_dwm() { FChangeLayout(DWM); }
// static void layout_centered() { FChangeLayout(CENTERED); }
// static void layout_stacked() { FChangeLayout(STACKED); }

static const Bindings bind[] = {
  // {0,				XF86XK_MonBrightnessUp,		brightness_up},
  // {0,				XF86XK_MonBrightnessDown,	brightness_down},
  // {0,				XF86XK_AudioMute,			volume_mute},
  // {0,				XF86XK_AudioRaiseVolume,	volume_up},
  // {0,				XF86XK_AudioLowerVolume,	volume_down},
  // {0,				XF86XK_Favorites,			st_music},
  // {0,				XF86XK_AudioMicMute,		toggle_mic},
};
