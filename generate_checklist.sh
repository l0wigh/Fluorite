#!/bin/bash

output="CHECKLIST.md"

cat > "$output" <<EOF
# ✅ Window Manager Test Checklist (General)

This checklist helps ensure your window manager is functioning correctly across multiple dimensions: layout, interaction, standards, and multi-monitor support.

---

## 🧱 Core Functionality

- [ ] Window manager starts without crashing
- [ ] Root window and background are displayed properly
- [ ] Terminal or launcher can be opened at startup
- [ ] All expected keybindings are responsive

---

## 🖼️ Window Management

- [ ] New windows are correctly managed (not floating unless needed)
- [ ] Windows are placed in correct positions (master/stack or grid/tiled)
- [ ] Windows resize as expected when layout changes
- [ ] Gaps, borders, and paddings are rendered consistently
- [ ] Default layout is respected

---

## 🖱️ Mouse & Input Focus

- [ ] Enter-to-focus works
- [ ] Cursor warp on focused windows when option is True
- [ ] Cursor does not warp when option is False
- [ ] Moving cursor already inside an unfocused window doesn't change focus
- [ ] Focus is restored to the next window in the stack
- [ ] Focus still works without cursor warping

---

## 🔁 Workspace / Desktop Switching

- [ ] Switching workspaces works without glitches
- [ ] Each workspace retains its window state
- [ ] Windows do not leak across workspaces
- [ ] Indicators (bar, HUD, etc.) reflect the active workspace
- [ ] Empty workspaces can be navigated to and from
- [ ] Workspaces keeps their currently selected layout
- [ ] You can send windows to other workspaces
	- [ ] Atoms are updated and reflected into polybar

---

## 📟 Status Bars / Panels (if used)

- [ ] Workspaces are displayed correctly in the bar
- [ ] Urgency or attention flags are highlighted in the bar
- [ ] Window titles or counts update when windows are opened/closed
- [ ] Clicking elements in the bar triggers appropriate actions

---

## 🖥️ Multi-Monitor Support

- [ ] All monitors are detected
- [ ] Monitors have a different default workspace based on user config
- [ ] Workspaces are uniquely attributed to monitors
- [ ] New windows appear on the correct monitor
- [!] Dragging or moving windows between monitors works
	- Not implemented yet
- [ ] Plugging/unplugging monitors
	- [ ] Go to default workspace on every monitors
	- [ ] Redraw occurs
	- [ ] Layout stay the same
	- [ ] No ghost windows
- [ ] Status bars appear per monitor (if configured)

---

## 🧠 EWMH/ICCCM Compliance

- [ ] \`_NET_CLIENT_LIST\` is updated when windows open/close
- [ ] \`_NET_WM_DESKTOP\` tracks the workspace a window is on
- [ ] \`_NET_ACTIVE_WINDOW\` updates when focus changes
- [ ] \`_NET_WM_NAME\` and \`WM_NAME\` are displayed correctly
- [ ] \`_NET_WM_WINDOW_TYPE\` is handled (normal, dialog, dock, etc.)

---

## 🔐 Floating and Special Windows

- [ ] Normal floating windows are centered by default
- [ ] Actions
	- [ ] Move
	- [ ] Resize
- [ ] Can extract windows from tiling by dragging or resizing
- [ ] Windows keeps their tiled properties (size/positions) when extracted
- [ ] Can be retiled with bindings
- [ ] Clicking on a floating window put it in top of the stack
- [ ] Floating windows remain on top when expected

---

## 📦 Application Compatibility

- [ ] GUI apps (e.g. browser, IDE, video player) display correctly
- [!] Games or fullscreen apps go fullscreen without issue
	- Not automatically (it's meh !)
- [ ] Notifications are not blocked or misplaced
- [ ] Clipboard works between apps
- [ ] Drag-and-drop behaves normally

---

## 🖼️ Wallpaper / Background

- [ ] Background is set and restored on startup
- [ ] Wallpaper appears correctly on each monitor (not stretched if unwanted)
- [ ] Wallpaper tool (e.g., \`feh\`, \`swww\`, \`nitrogen\`) works as expected

---

## 👄 Window Swallowing

- [ ] Opening
	- [ ] Tiled swallowers
	- [ ] Floating swallowers
- [ ] Closing
	- [ ] Tiled swallowers
	- [ ] Floating swallowers
- [ ] Apps not present in default_swallowing **can't** swallow apps
- [ ] Floating actions
	- [ ] Move
	- [ ] Resize

---

## 🖌️ Dynamic Theming

- [ ] Xresources modifications are monitored if True in options
- [ ] Reloading redraw every monitors
- [ ] If an option is unselected it use the compiled value

---


## ⚠️ Edge Cases

- [ ] Closing the last window does not crash the WM
- [ ] Reopening closed apps works as expected
- [ ] Rapid workspace switching does not break rendering
- [!] High-DPI scaling (if applicable) works
	- Not tested

---

_Last updated: $(date)_
EOF

echo "✅ Checklist generated in $output"

