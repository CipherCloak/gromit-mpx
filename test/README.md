# Gromit-MPX Integration Tests

## Tool Multi-User Test

What this quick and dirty shell script does is:

- start Gromit-MPX
- add two new XI2 pointer-keyboard pairs
- simultaneously do an action with both of them
- remove them again

Launch with `./test-tool-multi-user.sh <path to gromit-mpx> <tool>`, e.g.

`./test-tool-multi-user.sh ../build/gromit-mpx PEN`

or

`./test-tool-multi-user.sh ../build/gromit-mpx RECT`

or any other tool.

## Headless Test

`test-headless.sh` checks filled `RECT`s, `COUNTER` bubbles and `SMOOTH`
strokes without any manual intervention. It

- starts a display of its own with Xvfb and the xcompmgr compositing manager
- runs Gromit-MPX with a temporary config dir and its own D-Bus session, so
  neither your desktop nor your config are touched
- fakes mouse and key input via XTest with the `xprobe` helper
- checks the pixels on screen and the debug output
- covers config parsing, fill blending, outline-less rectangles, the `RECT`
  and `CIRCLE` outlines, counting, undo, redo, clearing, restarts, numbers
  with several digits, two simultaneous pointers, `SMOOTH` strokes with
  repeated points and, if Xephyr and xrandr are installed, a screen size change

Launch with `./test-headless.sh <path to gromit-mpx>`, e.g.

`./test-headless.sh ../build/gromit-mpx`

It needs Xvfb, xcompmgr, xinput and the Xlib and XTest headers, on Debian and
Ubuntu `sudo apt install xvfb xcompmgr xinput libxtst-dev`, optionally plus
`xserver-xephyr x11-xserver-utils`. It exits non-zero if a check failed.
