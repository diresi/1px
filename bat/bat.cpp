#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/Xfixes.h>
#include <X11/extensions/shape.h>

#include <stdio.h>
#include <stdlib.h>

#include <cairo-xlib.h>
#include <cairo.h>

#include <chrono>
#include <iostream>
#include <filesystem>
#include <fstream>
#include <thread>

using namespace std;

// FIXME: add some argument parser
const unsigned int INTERVAL = 1000; // ms
const unsigned int HEIGHT = 3;
const char *basedir = "/sys/class/power_supply/";

const int EXIT_ERR = -1;

void panic(const char *msg, int err = EXIT_ERR)
{
  fputs(msg, stderr);
  exit(err);
}


int read_capacity_file(const filesystem::path& cap)
{
  ifstream fin(cap);

  string line;
  getline(fin, line);
  return stoi(line);
}

int get_bat_capacity()
{
  auto bd = filesystem::path(basedir);
  for(const auto &e : filesystem::directory_iterator(bd)) {
    if (string(e.path().filename()).starts_with("BAT")) {
      return read_capacity_file(e.path() / "capacity");
    }
  }

  return 0;
}

void draw(cairo_t *cr, int capacity)
{
  cairo_save(cr);

  cairo_surface_t *surf = cairo_get_target(cr);
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

int main()
{
  Display *d = XOpenDisplay(NULL);
  Window root = DefaultRootWindow(d);

  int x = 0;
  int y = 0;
  unsigned int width = 0;
  unsigned int height = 0;
  unsigned int border = 0;
  unsigned int depth = 0;

  if (!XGetGeometry(d, root, &root, &x, &y, &width, &height, &border, &depth)) {
    panic("Failed to get root window geometry, terminating\n");
  }

  // these two lines are really all you need
  XSetWindowAttributes attrs;
  attrs.override_redirect = true;

  XVisualInfo vinfo;
  if (!XMatchVisualInfo(d, DefaultScreen(d), 32, TrueColor, &vinfo)) {
    panic("No visual found supporting 32 bit color, terminating\n");
  }
  // these next three lines add 32 bit depth, remove if you dont need and change
  // the flags below
  attrs.colormap = XCreateColormap(d, root, vinfo.visual, AllocNone);
  attrs.background_pixel = 0;
  attrs.border_pixel = 0;

  // Window XCreateWindow(
  //     Display *display, Window parent,
  //     int x, int y, unsigned int width, unsigned int height, unsigned int
  //     border_width, int depth, unsigned int class, Visual *visual, unsigned
  //     long valuemask, XSetWindowAttributes *attributes
  // );
  Window overlay = XCreateWindow(
      d, root, 0, height - HEIGHT, width, HEIGHT, 0, vinfo.depth, InputOutput,
      vinfo.visual,
      CWOverrideRedirect | CWColormap | CWBackPixel | CWBorderPixel, &attrs);

  // add some information
  char name[] = "1pxbat";
  XStoreName(d, overlay, name);
  XClassHint ch{name, name};
  XSetClassHint(d, overlay, &ch);

  // zero out the input region (i.e. pass it all on to the next lower windows)
  // gratefully taken from https://stackoverflow.com/a/50806584/203515
  XRectangle rect;
  XserverRegion region = XFixesCreateRegion(d, &rect, 1);
  XFixesSetWindowShapeRegion(d, overlay, ShapeInput, 0, 0, region);
  XFixesDestroyRegion(d, region);

  XMapWindow(d, overlay);

  cairo_surface_t *surf =
      cairo_xlib_surface_create(d, overlay, vinfo.visual, width, HEIGHT);
  cairo_t *cr = cairo_create(surf);

  int last_cap = 0;
  while (true) {
    // FIXME: add graceful shutdown?

    int cap = get_bat_capacity();
    if (cap != last_cap) {
      last_cap = cap;
      draw(cr, cap);
      XFlush(d);
    }
    this_thread::sleep_for(chrono::milliseconds(INTERVAL));
  }

  cairo_destroy(cr);
  cairo_surface_destroy(surf);

  XUnmapWindow(d, overlay);
  XCloseDisplay(d);
  return 0;
}
