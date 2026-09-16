# EVO3 Informations

Since the EVO2 was (close to ?) features complete, I wanted to focus on EWMH/ICCCM. Doing this will helps with strange behaviours from many apps.

- Discord notifications not working when the app was definitely not on screen.
- Ghidra (and probably other java apps) not showing popups on the right position.
- Dock apps not working properly in Multi-monitors setups
- Fluorite not handling Multi-monitors, struts, ... Properly causing issues with hot(un)plugging and specific setups.

Also some apps like pagers or docks, might need more informations from Fluorite to give full potential.

It was also time for me to format the code using normal standards. Right now it's using .clang-format file to handle this. The formating is subject to change.

I will finally try to clean the code to make it easier to work with. I might create multiple files, and stuff like that. It's not done for now since I'm not sure how I want it to be.

New features might sneak in, but it's not the point of the EVO 3 anyways.

## TODO

- [ ] Discord not giving controls when reappearing on screen, a click is required
- [ ] Missing EWMH
- [ ] Missing Client message handling
- [ ] Strange behavior from Firefox when setting the window fullscreen with binding
    - It leave the fullscreen of Youtube videos for exemple
- [ ] Create multiple files to have a cleaner project
- [ ] Find and remove deadcode
- [ ] (?) Rename functions that doesn't make sense

## DONE

- [x] Ghidra window placements
- [x] Multimonitors
    - [x] Quickshell place it's windows properly
    - [x] No strut issues after plugging/unplugging
    - [x] No strange process are required anymore to make the hot plug works
- [x] Discord will give everything notifications if it's not on an active workspace
- [x] Fullscreen is now set in EWMH
