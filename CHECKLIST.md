# ✅ Window Manager Test Checklist (General)

This checklist helps ensure your window manager is functioning correctly across multiple dimensions: layout, interaction, standards, and multi-monitor support.

---

## 🧱 Core Functionality

- [X] Window manager starts without crashing
- [X] Root window and background are displayed properly
- [X] Terminal or launcher can be opened at startup
- [X] All expected keybindings are responsive

---

## 🖼️ Window Management

- [X] New windows are correctly managed (not floating unless needed)
- [X] Windows are placed in correct positions (master/stack or grid/tiled)
- [X] Windows resize as expected when layout changes
- [X] Gaps, borders, and paddings are rendered consistently
- [X] Default layout is respected

---

## 🖱️ Mouse & Input Focus

- [X] Enter-to-focus works
- [X] Cursor warp on focused windows when option is True
- [X] Cursor does not warp when option is False
- [X] Moving cursor already inside an unfocused window doesn't change focus
- [X] Focus is restored to the next window in the stack
- [X] Focus still works without cursor warping

---

## 🔁 Workspace / Desktop Switching

- [X] Switching workspaces works without glitches
- [X] Each workspace retains its window state
- [X] Windows do not leak across workspaces
- [X] Indicators (bar, HUD, etc.) reflect the active workspace
- [X] Empty workspaces can be navigated to and from
- [X] Workspaces keeps their currently selected layout

---

## 📟 Status Bars / Panels (if used)

- [X] Workspaces are displayed correctly in the bar
- [X] Urgency or attention flags are highlighted in the bar
- [X] Window titles or counts update when windows are opened/closed
- [X] Clicking elements in the bar triggers appropriate actions

---

## 🖥️ Multi-Monitor Support

- [X] All monitors are detected
- [X] Monitors have a different default workspace based on user config
- [X] Workspaces are uniquely attributed to monitors
- [X] New windows appear on the correct monitor
- [!] Dragging or moving windows between monitors works
	- Not implemented yet
- [!] Plugging/unplugging monitors
	- [X] Go to default workspace on every monitors
	- [!] Redraw occurs
        - When monitors goes to 2 -> 1 another XEvent needs to occurs to see windows
        - Maybe add a little sleep
	- [X] Layout stay the same
	- [X] No ghost windows
- [X] Status bars appear per monitor (if configured)

---

## 🧠 EWMH/ICCCM Compliance

- [X] `_NET_CLIENT_LIST` is updated when windows open/close
- [X] `_NET_WM_DESKTOP` tracks the workspace a window is on
- [X] `_NET_ACTIVE_WINDOW` updates when focus changes
- [X] `_NET_WM_NAME` and `WM_NAME` are displayed correctly
- [X] `_NET_WM_WINDOW_TYPE` is handled (normal, dialog, dock, etc.)

---

## 🔐 Floating and Special Windows

- [X] Normal floating windows are centered by default
- [X] Actions
	- [X] Move
	- [X] Resize
- [X] Can extract windows from tiling by dragging or resizing
- [X] Windows keeps their tiled properties (size/positions) when extracted
- [X] Can be retiled with bindings
- [X] Clicking on a floating window put it in top of the stack
- [X] Floating windows remain on top when expected

---

## 📦 Application Compatibility

- [X] GUI apps (e.g. browser, IDE, video player) display correctly
- [!] Games or fullscreen apps go fullscreen without issue
	- Not automatically (it's meh !)
- [X] Notifications are not blocked or misplaced
- [X] Clipboard works between apps
- [X] Drag-and-drop behaves normally

---

## 🖼️ Wallpaper / Background

- [X] Background is set and restored on startup
- [X] Wallpaper appears correctly on each monitor (not stretched if unwanted)
- [X] Wallpaper tool (e.g., `feh`, `swww`, `nitrogen`) works as expected

---

## 👄 Window Swallowing

- [X] Opening
	- [X] Tiled swallowers
	- [X] Floating swallowers
- [X] Closing
	- [X] Tiled swallowers
	- [X] Floating swallowers
- [X] Apps not present in default_swallowing **can't** swallow apps
- [X] Floating actions
	- [X] Move
	- [X] Resize

---

## 🖌️ Dynamic Theming

- [X] Xresources modifications are monitored if True in options
- [X] Reloading redraw every monitors
- [X] If an option is unselected it use the compiled value

---


## ⚠️ Edge Cases

- [X] Closing the last window does not crash the WM
- [X] Reopening closed apps works as expected
- [X] Rapid workspace switching does not break rendering
- [!] High-DPI scaling (if applicable) works
    - Not tested

---

_Last updated: sam. 02 août 2025 13:24:05 CEST_
