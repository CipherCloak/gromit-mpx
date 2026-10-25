/*
 * Input and pixel helper for the headless Gromit-MPX tests.
 *
 * Fakes pointer and key input via XTest, optionally for a given XTEST slave
 * device of an MPX master, and reads back pixels of the root window.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/XTest.h>

static Display *display;
static XDevice *device; /* NULL means the core XTEST pointer */

static void flush(void)
{
    XSync(display, False);
    usleep(20 * 1000);
}

static void move(int x, int y)
{
    if (device) {
        int axes[] = {x, y};
        XTestFakeDeviceMotionEvent(display, device, 0, 0, axes, 2, 0);
    } else {
        XTestFakeMotionEvent(display, -1, x, y, 0);
    }
    flush();
}

static void button(int button, Bool press)
{
    if (device)
        XTestFakeDeviceButtonEvent(display, device, button, press, NULL, 0, 0);
    else
        XTestFakeButtonEvent(display, button, press, 0);
    flush();
}

static int key(const char *keysym_name, Bool press)
{
    KeySym keysym = XStringToKeysym(keysym_name);
    KeyCode keycode = keysym == NoSymbol ? 0 : XKeysymToKeycode(display, keysym);
    if (!keycode) {
        fprintf(stderr, "Unknown key '%s'.\n", keysym_name);
        return 1;
    }
    XTestFakeKeyEvent(display, keycode, press, 0);
    flush();
    return 0;
}

static XImage *grab(int x, int y, unsigned int w, unsigned int h)
{
    return XGetImage(display, DefaultRootWindow(display), x, y, w, h,
                     AllPlanes, ZPixmap);
}

static void split(XImage *img, unsigned long p, int *r, int *g, int *b)
{
    /* assumes the usual 24 bit TrueColor layout of Xvfb */
    *r = (p & img->red_mask) >> 16;
    *g = (p & img->green_mask) >> 8;
    *b = (p & img->blue_mask);
}

/* prints "R G B" of one pixel */
static int pixel(int x, int y)
{
    XImage *img = grab(x, y, 1, 1);
    if (!img)
        return 1;
    int r, g, b;
    split(img, XGetPixel(img, 0, 0), &r, &g, &b);
    printf("%d %d %d\n", r, g, b);
    XDestroyImage(img);
    return 0;
}

/* prints how many pixels of the area are within tol of the color */
static int count(int x, int y, int w, int h, const char *hex, int tol)
{
    unsigned int c = strtoul(hex[0] == '#' ? hex + 1 : hex, NULL, 16);
    int cr = (c >> 16) & 0xff, cg = (c >> 8) & 0xff, cb = c & 0xff;
    XImage *img = grab(x, y, w, h);
    if (!img)
        return 1;
    long n = 0;
    for (int j = 0; j < h; j++)
        for (int i = 0; i < w; i++) {
            int r, g, b;
            split(img, XGetPixel(img, i, j), &r, &g, &b);
            if (abs(r - cr) <= tol && abs(g - cg) <= tol && abs(b - cb) <= tol)
                n++;
        }
    printf("%ld\n", n);
    XDestroyImage(img);
    return 0;
}

/* writes the whole screen as binary PPM */
static int snap(const char *path)
{
    int w = DisplayWidth(display, DefaultScreen(display));
    int h = DisplayHeight(display, DefaultScreen(display));
    XImage *img = grab(0, 0, w, h);
    FILE *f = fopen(path, "wb");
    if (!img || !f)
        return 1;
    fprintf(f, "P6\n%d %d\n255\n", w, h);
    for (int j = 0; j < h; j++)
        for (int i = 0; i < w; i++) {
            int r, g, b;
            split(img, XGetPixel(img, i, j), &r, &g, &b);
            fputc(r, f);
            fputc(g, f);
            fputc(b, f);
        }
    fclose(f);
    XDestroyImage(img);
    return 0;
}

/* maps a full screen window of the given color below everything else, until killed */
static int background(const char *hex)
{
    int screen = DefaultScreen(display);
    XColor color;
    if (!XParseColor(display, DefaultColormap(display, screen), hex, &color) ||
        !XAllocColor(display, DefaultColormap(display, screen), &color)) {
        fprintf(stderr, "Unknown color '%s'.\n", hex);
        return 1;
    }
    XSetWindowAttributes attr;
    attr.override_redirect = True;
    attr.background_pixel = color.pixel;
    Window win = XCreateWindow(display, DefaultRootWindow(display), 0, 0,
                               DisplayWidth(display, screen), DisplayHeight(display, screen),
                               0, CopyFromParent, InputOutput, CopyFromParent,
                               CWOverrideRedirect | CWBackPixel, &attr);
    XMapWindow(display, win);
    XLowerWindow(display, win);
    XSync(display, False);
    for (;;)
        pause();
}

static void usage(const char *name)
{
    fprintf(stderr,
            "Usage: %s [-d <xtest device id>] <command>\n"
            "  move X Y\n"
            "  down BUTTON | up BUTTON\n"
            "  click X Y [BUTTON]\n"
            "  drag X1 Y1 X2 Y2 [BUTTON] [STEPS]\n"
            "  keydown KEYSYM | keyup KEYSYM\n"
            "  pixel X Y\n"
            "  count X Y W H RRGGBB [TOLERANCE]\n"
            "  snap FILE.ppm\n"
            "  background COLOR\n",
            name);
}

int main(int argc, char *argv[])
{
    const char *name = argv[0];
    int dev_id = -1;
    if (argc > 2 && strcmp(argv[1], "-d") == 0) {
        dev_id = atoi(argv[2]);
        argc -= 2;
        argv += 2;
    }
    if (argc < 2) {
        usage(name);
        return 1;
    }

    display = XOpenDisplay(NULL);
    if (!display) {
        fprintf(stderr, "Unable to open display.\n");
        return 1;
    }
    XDevice xdev;
    if (dev_id >= 0) {
        xdev.device_id = dev_id;
        device = &xdev;
    }

    const char *cmd = argv[1];
    int ret = 0;
    if (strcmp(cmd, "move") == 0 && argc == 4) {
        move(atoi(argv[2]), atoi(argv[3]));
    } else if ((strcmp(cmd, "down") == 0 || strcmp(cmd, "up") == 0) && argc == 3) {
        button(atoi(argv[2]), strcmp(cmd, "down") == 0);
    } else if (strcmp(cmd, "click") == 0 && (argc == 4 || argc == 5)) {
        int b = argc == 5 ? atoi(argv[4]) : 1;
        move(atoi(argv[2]), atoi(argv[3]));
        button(b, True);
        button(b, False);
    } else if (strcmp(cmd, "drag") == 0 && argc >= 6 && argc <= 8) {
        int x1 = atoi(argv[2]), y1 = atoi(argv[3]);
        int x2 = atoi(argv[4]), y2 = atoi(argv[5]);
        int b = argc >= 7 ? atoi(argv[6]) : 1;
        int steps = argc == 8 ? atoi(argv[7]) : 10;
        move(x1, y1);
        button(b, True);
        for (int i = 1; i <= steps; i++)
            move(x1 + (x2 - x1) * i / steps, y1 + (y2 - y1) * i / steps);
        button(b, False);
    } else if ((strcmp(cmd, "keydown") == 0 || strcmp(cmd, "keyup") == 0) && argc == 3) {
        ret = key(argv[2], strcmp(cmd, "keydown") == 0);
    } else if (strcmp(cmd, "pixel") == 0 && argc == 4) {
        ret = pixel(atoi(argv[2]), atoi(argv[3]));
    } else if (strcmp(cmd, "count") == 0 && (argc == 7 || argc == 8)) {
        ret = count(atoi(argv[2]), atoi(argv[3]), atoi(argv[4]), atoi(argv[5]),
                    argv[6], argc == 8 ? atoi(argv[7]) : 0);
    } else if (strcmp(cmd, "snap") == 0 && argc == 3) {
        ret = snap(argv[2]);
    } else if (strcmp(cmd, "background") == 0 && argc == 3) {
        ret = background(argv[2]);
    } else {
        usage(name);
        ret = 1;
    }

    XCloseDisplay(display);
    return ret;
}
