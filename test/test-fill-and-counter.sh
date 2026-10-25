#!/bin/sh
#
# Headless tests for filled RECTs and numbered COUNTER bubbles.
#
# Runs Gromit-MPX on a display of its own (Xvfb plus the xcompmgr compositing
# manager) with a temporary config dir and its own D-Bus session, so neither
# the running desktop nor the user's config are touched. Input is faked via
# XTest, results are checked via the pixels on screen and the debug output.
#
# Needs Xvfb, xcompmgr, xinput and a C compiler with the Xlib and XTest
# headers. Xephyr and xrandr are optional and enable the screen size test.


GROMIT_MPX="$1"

[ -x "$GROMIT_MPX" ] || {
    echo "./test-fill-and-counter.sh <path to gromit-mpx>"
    exit 1
}

GROMIT_MPX=$(realpath "$GROMIT_MPX")
TEST_DIR=$(dirname "$(realpath "$0")")
WORK=$(mktemp -d)
XPROBE="$WORK/xprobe"
LOG="$WORK/gromit-mpx.log"
PASSED=0
FAILED=0

cc -o "$XPROBE" "$TEST_DIR/xprobe.c" -lX11 -lXtst || exit 1

# never touch the user's config, session bus or desktop
export XDG_CONFIG_HOME="$WORK/config"
export XDG_SESSION_TYPE=x11
export NO_AT_BRIDGE=1
unset XDG_CURRENT_DESKTOP
mkdir -p "$XDG_CONFIG_HOME"
DBUS=""
command -v dbus-run-session >/dev/null && DBUS="dbus-run-session --"

# the cursors, as gromit-mpx is not installed
mkdir -p "$WORK/share/icons/hicolor/48x48/apps"
cp "$TEST_DIR"/../data/*cursor.png "$WORK/share/icons/hicolor/48x48/apps"
export XDG_DATA_DIRS="$WORK/share:${XDG_DATA_DIRS:-/usr/local/share:/usr/share}"


#
# Helpers
#

check () { # description, command...
    desc="$1"
    shift
    if "$@"; then
        PASSED=$((PASSED + 1))
        printf '\033[32mPASS\033[0m %s\n' "$desc"
    else
        FAILED=$((FAILED + 1))
        printf '\033[31mFAIL\033[0m %s\n' "$desc"
    fi
}

wait_for () { # pattern in the log
    i=0
    while ! grep -q -- "$1" "$LOG" 2>/dev/null; do
        i=$((i + 1))
        [ $i -gt 100 ] && { echo "timeout waiting for '$1'"; return 1; }
        sleep 0.1
    done
}

wait_for_file () { # file that a server writes its display number to
    i=0
    while [ ! -s "$1" ]; do
        i=$((i + 1))
        [ $i -gt 100 ] && { echo "timeout waiting for $1"; exit 1; }
        sleep 0.1
    done
}

start_display () {
    rm -f "$WORK/display"
    Xvfb -displayfd 3 -screen 0 800x600x24 -br -nolisten tcp \
         3>"$WORK/display" >/dev/null 2>&1 &
    XVFB_PID=$!
    wait_for_file "$WORK/display"
    export DISPLAY=":$(cat "$WORK/display")"
    xcompmgr -n >/dev/null 2>&1 &
    COMPMGR_PID=$!
    # something to annotate: a blue window below everything else
    "$XPROBE" background '#0000ff' &
    BG_PID=$!
    sleep 0.5
}

stop_display () {
    kill $BG_PID $COMPMGR_PID $XVFB_PID 2>/dev/null
    wait $BG_PID $COMPMGR_PID $XVFB_PID 2>/dev/null
}

start_gromit () { # config, opacity, gromit-mpx arguments...
    printf '%s\n' "$1" > "$XDG_CONFIG_HOME/gromit-mpx.cfg"
    printf '[General]\nShowIntroOnStartup=false\n[Drawing]\nOpacity=%s\n' "$2" \
           > "$XDG_CONFIG_HOME/gromit-mpx.ini"
    shift 2
    : > "$LOG"
    $DBUS "$GROMIT_MPX" "$@" > "$LOG" 2>&1 &
    GROMIT_PID=$!
    wait_for "enabled devices"
    sleep 0.5
}

stop_gromit () {
    remote --quit
    wait $GROMIT_PID
}

remote () { # command for the running gromit-mpx
    "$GROMIT_MPX" "$@" >/dev/null 2>&1
    sleep 0.3
}

with_keys () { # keysyms separated by +, xprobe arguments...
    keys=$(echo "$1" | tr '+' ' ')
    shift
    for k in $keys; do "$XPROBE" keydown "$k"; done
    "$XPROBE" "$@"
    for k in $keys; do "$XPROBE" keyup "$k"; done
}

pixel_near () { # x, y, r, g, b, tolerance
    p=$("$XPROBE" pixel "$1" "$2")
    tol=$6
    set -- $p "$3" "$4" "$5"
    for d in $(($1 - $4)) $(($2 - $5)) $(($3 - $6)); do
        [ "${d#-}" -le "$tol" ] || { echo "    pixel is $p"; return 1; }
    done
}

pixels_matching () { # x, y, width, height, RRGGBB, tolerance
    "$XPROBE" count "$@"
}

tool_line () { # tool name as in the config
    grep -F "Tool name: \"$1|0|0" "$LOG"
}

tool_has () { # tool name, expected text in its info line
    tool_line "$1" | grep -q -F -- "$2" || { echo "    $(tool_line "$1")"; return 1; }
}

counts () { # the numbers of all counters drawn so far
    sed -n 's/.*draw counter with .* and count \(-*[0-9]*\)$/\1/p' "$LOG" | tr '\n' ' ' | sed 's/ $//'
}

counts_are () { # expected counts
    [ "$(counts)" = "$1" ] || { echo "    counts are '$(counts)'"; return 1; }
}

counter_font_size () { # count
    sed -n "s/.*font_size \([0-9]*\) and count $1\$/\1/p" "$LOG" | tail -n 1
}

less_than () {
    [ -n "$1" ] && [ "$1" -lt "$2" ]
}

settle () {
    sleep 0.3
}


start_display


#
# RECT with fillcolor: configuration
#

RECT_CONFIG='
"outline Rect"     = RECT (color="red" size=4);
"filled Rect"      = RECT (color="lime" size=6 fillcolor="rgba(255, 0, 0, 0.5)");
"borderless Rect"  = RECT (size=0 fillcolor="rgba(255, 255, 0, 0.5)");
"opaque Rect"      = RECT (color="black" size=2 fillcolor="white");
"derived Rect"     = "filled Rect" (fillcolor="rgba(0, 0, 255, 0.25)");
"bad fill Rect"    = RECT (color="red" fillcolor="no-such-color");
"unknown arg Rect" = RECT (bordercolor="blue" size=3);
"filled Circle"    = CIRCLE (color="black" size=3 fillcolor="rgba(255, 0, 0, 0.5)");
"default"          = "outline Rect";
"default"[SHIFT]   = "filled Rect";
"default"[CONTROL] = "borderless Rect";
"default"[ALT]     = "filled Circle";
"default"[SHIFT,CONTROL] = "opaque Rect";
'

echo "== RECT with fillcolor"
start_gromit "$RECT_CONFIG" 1.0 -a

check "RECT without fillcolor keeps the transparent default fill" \
      tool_has "outline Rect" "fillcolor: rgba(0,0,0,0), color: rgb(255,0,0)"
check "RECT parses an RGBA fillcolor next to color and size" \
      tool_has "filled Rect" "width: 6, minwidth: 1, maxwidth: 4294967295,  fillcolor: rgba(255,0,0,0.5), color: rgb(0,255,0)"
check "RECT with size=0 has no outline" \
      tool_has "borderless Rect" "width: 0,"
check "RECT parses an opaque fillcolor" \
      tool_has "opaque Rect" "fillcolor: rgb(255,255,255), color: rgb(0,0,0)"
check "derived RECT inherits size and color, overrides fillcolor" \
      tool_has "derived Rect" "Rect,       width: 6, minwidth: 1, maxwidth: 4294967295,  fillcolor: rgba(0,0,255,0.25), color: rgb(0,255,0)"
check "invalid fillcolor is reported" \
      grep -q "Unable to parse fillcolor. Keeping default." "$LOG"
check "invalid fillcolor keeps the transparent default" \
      tool_has "bad fill Rect" "fillcolor: rgba(0,0,0,0), color: rgb(255,0,0)"
check "unknown parameter is skipped with a warning" \
      grep -q "skipped token" "$LOG"
check "tool with unknown parameter is still defined" \
      tool_has "unknown arg Rect" "Rect,       width: 3,"


#
# RECT with fillcolor: drawing
#

"$XPROBE" drag 40 40 190 140
with_keys Shift_L drag 220 40 370 140
with_keys Control_L drag 400 40 550 140
with_keys Shift_L+Control_L drag 580 40 730 140
with_keys Alt_L drag 115 250 165 250
settle

check "RECT outline is drawn" pixel_near 40 90 255 0 0 0
check "RECT without fillcolor leaves the inside untouched" pixel_near 115 90 0 0 255 0
check "semi-transparent fill blends with the content below" pixel_near 295 90 128 0 127 3
check "filled RECT has its own outline color" pixel_near 220 90 0 255 0 0
check "fill is not accumulated while dragging" pixel_near 240 60 128 0 127 3
check "size=0 RECT is filled" pixel_near 475 90 128 128 127 3
check "size=0 RECT has no outline outside its fill" pixel_near 397 90 0 0 255 0
check "size=0 RECT is filled up to its edge" pixel_near 402 90 128 128 127 3
check "opaque fill covers the content below" pixel_near 655 90 255 255 255 0
check "opaque filled RECT has its outline" pixel_near 580 90 0 0 0 0
check "CIRCLE fill still blends" pixel_near 115 250 128 0 127 3
check "CIRCLE outline is still drawn" pixel_near 165 250 0 0 0 0
check "CIRCLE outside is untouched" pixel_near 175 250 0 0 255 0

# a fill drawn on top must blend with annotations below, not erase them
"$XPROBE" drag 40 320 190 420
with_keys Control_L drag 20 340 100 400
settle
check "fill over an annotation blends with it" pixel_near 40 370 255 128 0 3
remote --undo
check "undo removes the fill and restores the annotation" pixel_near 40 370 255 0 0 0
remote --redo
check "redo brings the fill back" pixel_near 40 370 255 128 0 3
"$XPROBE" snap "$WORK/fill.ppm"

stop_gromit

# the canvas opacity applies on top of the fill's own alpha
start_gromit "$RECT_CONFIG" 0.5 -a
with_keys Shift_L drag 220 40 370 140
settle
check "canvas opacity 0.5 halves the fill's alpha" pixel_near 295 90 64 0 191 3
stop_gromit


#
# COUNTER bubbles, one device
#

COUNTER_CONFIG='
"green Pen"   = PEN (size=5 color="green");
"red Bubble"  = COUNTER (size=2 color="white" fillcolor="red" fontcolor="white" xlength=30 ylength=30 radius=15);
"from 998"    = COUNTER (size=2 color="black" fillcolor="red" fontcolor="white" xlength=30 ylength=30 radius=15 start=998);
"bare number" = COUNTER (size=0 fontcolor="yellow" fontsize=30);
"default"     = "green Pen";
"default"[SHIFT,CONTROL] = "red Bubble";
"default"[ALT] = "from 998";
"default"[2]  = "bare number";
'

echo "== COUNTER bubbles"
start_gromit "$COUNTER_CONFIG" 1.0 -a -d

for x in 100 160 220; do
    with_keys Shift_L+Control_L click $x 100
done
settle
check "first bubble is 1, then counting up" counts_are "1 2 3"
check "bubble is placed at the pointer" grep -q "draw counter with center 160, 100," "$LOG"
check "bubble is filled" pixel_near 161 111 255 0 0 0
check "bubble shows its number" less_than 20 "$(pixels_matching 153 93 17 17 ffffff 60)"
check "bubble stays after the button release" pixel_near 101 111 255 0 0 0

remote --undo
check "undo removes the last bubble" pixel_near 221 111 255 0 255 0
with_keys Shift_L+Control_L click 280 100
check "undo gives back the number of the removed bubble" counts_are "1 2 3 3"
remote --undo
remote --redo
check "redo restores the bubble" pixel_near 281 111 255 0 0 0
with_keys Shift_L+Control_L click 340 100
check "redo restores the count" counts_are "1 2 3 3 4"

"$XPROBE" drag 100 150 300 150
remote --undo
with_keys Shift_L+Control_L click 400 100
check "undoing another tool's stroke keeps the count" counts_are "1 2 3 3 4 5"

remote --clear
with_keys Shift_L+Control_L click 100 200
check "clearing the screen restarts the count" counts_are "1 2 3 3 4 5 1"

for x in 100 160 220 280; do
    with_keys Alt_L click $x 300
done
settle
check "counter with start=998 counts into four digits" counts_are "1 2 3 3 4 5 1 998 999 1000 1001"
check "four digit number gets a smaller font" less_than "$(counter_font_size 1000)" 18
# white pixels right outside a bubble's black border are a number sticking out
for n in 220:1000 280:1001; do
    x=${n%:*}
    check "number ${n#*:} stays inside its bubble, left" test "$(pixels_matching $((x - 19)) 285 4 32 ffffff 60)" -eq 0
    check "number ${n#*:} stays inside its bubble, right" test "$(pixels_matching $((x + 18)) 285 4 32 ffffff 60)" -eq 0
done

"$XPROBE" click 400 450 2
settle
check "frameless counter via button 2 shows its number" less_than 30 "$(pixels_matching 370 420 64 64 ffff00 60)"
"$XPROBE" snap "$WORK/counter.ppm"

stop_gromit

start_gromit "$COUNTER_CONFIG" 1.0 -a -d
with_keys Shift_L+Control_L click 100 100
check "a restarted gromit-mpx starts counting at 1" counts_are "1"
stop_gromit


#
# COUNTER bubbles, several devices at once
#

echo "== COUNTER bubbles with two pointers"
MULTI_CONFIG='
"red Bubble" = COUNTER (size=2 color="white" fillcolor="red" fontcolor="white" xlength=30 ylength=30 radius=15);
"default" = "red Bubble";
'
start_gromit "$MULTI_CONFIG" 1.0 -d
xinput create-master one
xinput create-master two
ONE=$(xinput list --id-only "one XTEST pointer")
TWO=$(xinput list --id-only "two XTEST pointer")
wait_for "Now 3 enabled devices"
sleep 0.5
remote --toggle

for y in 100 150 200; do
    "$XPROBE" -d "$ONE" click 100 $y
    "$XPROBE" -d "$TWO" click 300 $y
done
for y in 300 350 400 450; do
    "$XPROBE" -d "$ONE" click 100 $y &
    CLICK_ONE=$!
    "$XPROBE" -d "$TWO" click 300 $y &
    wait $CLICK_ONE $!
done
settle
check "both pointers draw bubbles" test "$(counts | wc -w)" -eq 14
check "bubbles of both pointers share one sequence without duplicates" \
      test "$(counts | tr ' ' '\n' | sort -n | tr '\n' ' ')" = "1 2 3 4 5 6 7 8 9 10 11 12 13 14 "
check "alternating pointers count in click order" \
      test "$(counts | cut -d ' ' -f 1-6)" = "1 2 3 4 5 6"

xinput remove-master "$(xinput list --id-only "one pointer")"
xinput remove-master "$(xinput list --id-only "two pointer")"
stop_gromit

stop_display


#
# COUNTER bubbles across a screen size change, needs a resizable X server
#

if command -v Xephyr >/dev/null && command -v xrandr >/dev/null; then
    echo "== COUNTER bubbles across a screen size change"
    start_display
    rm -f "$WORK/ephyr"
    Xephyr -displayfd 3 -screen 800x600 -resizeable -br -nolisten tcp \
           3>"$WORK/ephyr" >/dev/null 2>&1 &
    EPHYR_PID=$!
    wait_for_file "$WORK/ephyr"
    export DISPLAY=":$(cat "$WORK/ephyr")"

    start_gromit "$MULTI_CONFIG" 1.0 -a -d
    "$XPROBE" click 100 100
    "$XPROBE" click 160 100
    xrandr -s 640x480 2>/dev/null
    wait_for "screen size changed to 640 x 480"
    sleep 1
    remote --toggle
    check "bubbles survive the size change" pixel_near 101 111 255 0 0 0
    "$XPROBE" click 220 100
    check "counting goes on after the size change" counts_are "1 2 3"
    remote --undo
    "$XPROBE" click 280 100
    check "undo still restores the count after the size change" counts_are "1 2 3 3"
    stop_gromit

    kill $EPHYR_PID
    wait $EPHYR_PID 2>/dev/null
    stop_display
else
    echo "Xephyr or xrandr missing, skipping the screen size change test"
fi


printf '\n%d passed, %d failed\n' $PASSED $FAILED
echo "Screenshots in $WORK"
[ $FAILED -eq 0 ]
