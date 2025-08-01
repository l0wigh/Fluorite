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
- [ ] Floating windows remain on top when expected

---

## 🖱️ Mouse & Input Focus

- [ ] Hover-to-focus works
- [ ] Focus follows mouse
- [ ] Focus is restored to previous window on close
- [ ] Cursor warping (if used) positions pointer correctly
- [ ] Focus still works nicely without cursor warping

---

## 🔁 Workspace / Desktop Switching

- [ ] Switching workspaces works without glitches
- [ ] Each workspace retains its window state
- [ ] Windows do not leak across workspaces
- [ ] Indicators (bar, HUD, etc.) reflect the active workspace
- [ ] Empty workspaces can be navigated to and from

---

## 📟 Status Bars / Panels (if used)

- [ ] Workspaces are displayed correctly in the bar
- [ ] Urgency or attention flags are highlighted in the bar
- [ ] Window titles or counts update when windows are opened/closed
- [ ] Clicking elements in the bar triggers appropriate actions

---

## 🖥️ Multi-Monitor Support

- [ ] All monitors are detected
- [ ] Workspaces can be assigned independently per monitor
- [ ] New windows appear on the correct monitor
- [ ] Dragging or moving windows between monitors works
- [ ] Plugging/unplugging monitors updates layouts properly
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

- [ ] Dialogs, modals, and splash windows are centered
- [ ] Windows with \`WM_TRANSIENT_FOR\` are grouped correctly
- [ ] Windows with \`SKIP_TASKBAR\` are not shown in the bar
- [ ] Input-less windows (e.g., tooltips) are not focused

---

## 📦 Application Compatibility

- [ ] GUI apps (e.g. browser, IDE, video player) display correctly
- [ ] Games or fullscreen apps go fullscreen without issue
- [ ] Notifications are not blocked or misplaced
- [ ] Clipboard works between apps
- [ ] Drag-and-drop behaves normally

---

## 🖼️ Wallpaper / Background

- [ ] Background is set and restored on startup
- [ ] Wallpaper appears correctly on each monitor (not stretched if unwanted)
- [ ] Wallpaper tool (e.g., \`feh\`, \`swww\`, \`nitrogen\`) works as expected

---

## ⚠️ Edge Cases

- [ ] Closing the last window does not crash the WM
- [ ] Reopening closed apps works as expected
- [ ] Rapid workspace switching does not break rendering
- [ ] High-DPI scaling (if applicable) works

---

_Last updated: $(date)_
EOF

echo "✅ Checklist generated in $output"

