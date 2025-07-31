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
- [!] Gaps, borders, and paddings are rendered consistently
    - Borders aren't actualized when Xresources are reloaded
- [X] Floating windows remain on top when expected

---

## 🖱️ Mouse & Input Focus

- [X] Hover-to-focus works
- [X] Focus follows mouse (if configured)
- [X] Focus is restored to previous window on close
- [X] Cursor warping (if used) positions pointer correctly

---

## 🔁 Workspace / Desktop Switching

- [X] Switching workspaces works without glitches
- [X] Each workspace retains its window state
- [X] Windows do not leak across workspaces
- [X] Indicators (bar, HUD, etc.) reflect the active workspace
- [X] Empty workspaces can be navigated to and from

---

## 📟 Status Bars / Panels (if used)

- [X] Workspaces are displayed correctly in the bar
- [X] Urgency or attention flags are highlighted in the bar
- [X] Window titles or counts update when windows are opened/closed
- [X] Clicking elements in the bar triggers appropriate actions

---

## 🖥️ Multi-Monitor Support

- [X] All monitors are detected
- [X] Workspaces can be assigned independently per monitor
- [X] New windows appear on the correct monitor
- [!] Dragging or moving windows between monitors works
    - Not implemented yet
- [X] Plugging/unplugging monitors updates layouts properly
- [X] Status bars appear per monitor (if configured)

---

## 🧠 EWMH/ICCCM Compliance

- [X] `_NET_CLIENT_LIST` is updated when windows open/close
- [X] `_NET_WM_DESKTOP` tracks the workspace a window is on
- [X] `_NET_ACTIVE_WINDOW` updates when focus changes
- [!] `_NET_WM_STATE_DEMANDS_ATTENTION` triggers urgency
    - Not Implemented
- [X] `_NET_WM_NAME` and `WM_NAME` are displayed correctly
- [X] `_NET_WM_WINDOW_TYPE` is handled (normal, dialog, dock, etc.)

---

## 🔐 Floating and Special Windows

- [X] Dialogs, modals, and splash windows are centered
- [!] Windows with `WM_TRANSIENT_FOR` are grouped correctly
    - Maybe, maybe not
- [X] Windows with `SKIP_TASKBAR` are not shown in the bar
- [!] Input-less windows (e.g., tooltips) are not focused
    - Maybe, maybe not

---

## 📦 Application Compatibility

- [X] GUI apps (e.g. browser, IDE, video player) display correctly
- [!] Games or fullscreen apps go fullscreen without issue
    - Not automatically, but can be put in this state
- [X] Notifications are not blocked or misplaced
- [X] Clipboard works between apps
- [X] Drag-and-drop behaves normally

---

## 🖼️ Wallpaper / Background

- [X] Background is set and restored on startup
- [X] Wallpaper appears correctly on each monitor (not stretched if unwanted)
- [X] Wallpaper tool (e.g., `feh`, `swww`, `nitrogen`) works as expected

---

## ⚠️ Edge Cases

- [X] Closing the last window does not crash the WM
- [X] Reopening closed apps works as expected
- [X] Rapid workspace switching does not break rendering
- [!] High-DPI scaling (if applicable) works
    - Untested

---

_Last updated: jeu. 31 juil. 2025 13:51:54 CEST_
