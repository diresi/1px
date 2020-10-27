#include <stdio.h>
#include <stdlib.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include <cairo.h>
#include <cairo-xlib.h>

#include <chrono>
#include <fstream>
#include <thread>

// FIXME: add some argument parser
const unsigned int INTERVAL = 1000; // ms
const unsigned int HEIGHT = 5;
const char *fn_bat = "/sys/class/power_supply/BAT1/capacity";

const int EXIT_ERR = -1;

void panic(const char* msg, int err=EXIT_ERR)
{
    fprintf(stderr, msg);
    exit(err);
}

int get_bat_capacity()
{
    std::ifstream fin(fn_bat);

    std::string line;
    std::getline(fin, line);
    return std::stoi(line);
}

void draw(cairo_t *cr, int capacity) {
    cairo_save(cr);

    cairo_surface_t* surf = cairo_get_target(cr);
    int w = cairo_xlib_surface_get_width(surf);
    int h = cairo_xlib_surface_get_height(surf);

    // since we're redrawing everything there's no need to restore the OVER
    // operator, just stay with SOURCE. also, we're stashing the context
    // anyway to keep other code unaffected.
    cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);

    // clear entire surface
    cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 0.0);
    cairo_paint(cr);

    // cairo_set_operator(cr, CAIRO_OPERATOR_OVER);

    // draw the new bar using some yellowish green, I'd happily accept
    // suggestions for prettier colors :)
    cairo_set_source_rgba(cr, 0.8, 1.0, 0.0, 0.5);

    w = capacity * w / 100;
    cairo_rectangle(cr, 0, 0, w, h);
    cairo_fill(cr);

    cairo_restore(cr);
}

int main() {
    Display *d = XOpenDisplay(NULL);
    Window root = DefaultRootWindow(d);

    int x = 0;
    int y = 0;
    unsigned int width = 0;
    unsigned int height = 0;
    unsigned int border = 0;
    unsigned int depth = 0;

    if(!XGetGeometry(d, root, &root, &x, &y, &width, &height, &border, &depth)) {
        panic("Failed to get root window geometry, terminating\n");
    }

    // these two lines are really all you need
    XSetWindowAttributes attrs;
    attrs.override_redirect = true;

    XVisualInfo vinfo;
    if (!XMatchVisualInfo(d, DefaultScreen(d), 32, TrueColor, &vinfo)) {
        panic("No visual found supporting 32 bit color, terminating\n");
    }
    // these next three lines add 32 bit depth, remove if you dont need and change the flags below
    attrs.colormap = XCreateColormap(d, root, vinfo.visual, AllocNone);
    attrs.background_pixel = 0;
    attrs.border_pixel = 0;

    // Window XCreateWindow(
    //     Display *display, Window parent,
    //     int x, int y, unsigned int width, unsigned int height, unsigned int border_width,
    //     int depth, unsigned int class, 
    //     Visual *visual,
    //     unsigned long valuemask, XSetWindowAttributes *attributes
    // );
    Window overlay = XCreateWindow(
        d, root,
        0, height-HEIGHT, width, HEIGHT, 0,
        vinfo.depth, InputOutput, 
        vinfo.visual,
        CWOverrideRedirect | CWColormap | CWBackPixel | CWBorderPixel, &attrs
    );

    XMapWindow(d, overlay);

    cairo_surface_t* surf = cairo_xlib_surface_create(d, overlay,
                                  vinfo.visual,
                                  width, HEIGHT);
    cairo_t* cr = cairo_create(surf);

    int last_cap = 0;
    while(true) {
        // FIXME: add graceful shutdown?

        int cap = get_bat_capacity();
        if (cap  != last_cap) {
            last_cap = cap;
            draw(cr, cap);
            XFlush(d);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(INTERVAL));
    }

    cairo_destroy(cr);
    cairo_surface_destroy(surf);

    XUnmapWindow(d, overlay);
    XCloseDisplay(d);
    return 0;
}
