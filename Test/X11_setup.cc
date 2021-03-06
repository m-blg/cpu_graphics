#include <X11/Xlib.h>

#include <X11/Xutil.h>
#include "../cp_lib/basic.cc"
#include "../cp_lib/array.cc"
#include "../cp_lib/vector.cc"
#include "../cp_lib/memory.cc"
#include <stdlib.h>
#include <unistd.h>

#include "../draw.cc"

using namespace cp;

void x_shutdown(Display* display, Window *window) {
    XDestroyWindow(display, *window);
    exit(1);
}

void x_set_size_hint(Display* display, Window *window, vec2i min_size, vec2i max_size) {
    XSizeHints hints = {};
    if (min_size.x > 0 && min_size.y > 0) hints.flags |= PMinSize;
    if (max_size.x > 0 && max_size.y > 0) hints.flags |= PMaxSize;

    hints.min_width = min_size.x;
    hints.min_height = min_size.y;
    hints.max_width = max_size.x;
    hints.max_height = max_size.y;

    XSetNormalHints(display, *window, &hints);
}

int main() {
    Display* display = XOpenDisplay(0);
    i32 screen = XDefaultScreen(display);
    Window root = XDefaultRootWindow(display);
    GC default_gc = XDefaultGC(display, screen);

    i32 screen_bit_depth = 24;
    XVisualInfo visual_info = {};
    XMatchVisualInfo(display, screen, screen_bit_depth, TrueColor, &visual_info);
    
    Colormap color_map = XCreateColormap(display, root, visual_info.visual, AllocNone);

    XSetWindowAttributes attributes;
    attributes.bit_gravity = StaticGravity;
    attributes.background_pixel = 0;
    attributes.colormap = color_map;
    attributes.event_mask = StructureNotifyMask | KeyPressMask | KeyReleaseMask;
    u64 attribute_mask = CWBitGravity | CWBackPixel | CWColormap | CWEventMask;

    vec2i window_max_size = {1920, 1080};
    vec2i window_size = {640, 480};
    Window window = XCreateWindow(display, root, 100, 100, window_size.x, window_size.y, 0, visual_info.depth, InputOutput, visual_info.visual, attribute_mask, &attributes);

    x_set_size_hint(display, &window, {100, 100}, window_max_size);

    XStoreName(display, window, "X11 Test");
    XMapWindow(display, window);

    XFlush(display);

    // create buffer

    dbuff2<u32> window_buffer;
    window_buffer.init(window_max_size.y, window_max_size.x);
    XImage* x_window_buffer = XCreateImage(display, visual_info.visual, visual_info.depth, ZPixmap, 0, (char*)window_buffer.buffer, window_max_size.x, window_max_size.y, sizeof(u32) * 8, 0);


    bool is_running = true;
    while (is_running) 
    {
        // event loop

        XEvent e;
        while (XPending(display) > 0) {
            XNextEvent(display, &e);
            switch (e.type) {
                case DestroyNotify: {
                    XDestroyWindowEvent* destroy_event = (XDestroyWindowEvent*)&e;
                    if (destroy_event->window == window) {
                        is_running = false;
                    }
                } break;
                case ConfigureNotify: {
                    XConfigureEvent* configure_event = (XConfigureEvent*)&e;
                    window_size.x = configure_event->width;
                    window_size.y = configure_event->height;
                } break;
                case KeyPress: {
                    XKeyEvent* key_event = (XKeyEvent*)&e;
                    if (key_event->keycode == XKeysymToKeycode(display, XK_q)) {
                        is_running = false;
                    }
                } break;
            }
        }

        // write to buffer

        for (u32 i = 0u; i < window_size.y; i++) {
            for (u32 j = 0u; j < window_size.x; j++) {
                if (((i/100u) + (j/100u)) % 2 > 0) {
                    window_buffer.get(i, j) = 0xffffffff;
                } else {
                    window_buffer.get(i, j) = 0;
                }
            }
        }

        XPutImage(display, window, default_gc, x_window_buffer, 0, 0, 0, 0, window_size.x, window_size.y);

        XFlush(display);
    }

    XDestroyWindow(display, window);
    XFlush(display);

    window_buffer.shut();

    return 0;
}

