#include <X11/Xlib.h>

#include <X11/Xutil.h>
#include "../cp_lib/basic.cc"
#include "../cp_lib/array.cc"
#include "../cp_lib/vector.cc"
#include "../cp_lib/memory.cc"
#include <stdlib.h>
#include <unistd.h>

#include "../draw.cc"
#include "wireframe_test.cc"

using namespace cp;

sbuff<vec3f, 8> cube_vertices = {{
    { -1, -1, -1 },
	{ 1, -1, -1 },
	{ 1, 1, -1 },
	{ -1, 1, -1 },
	{ -1, 1, 1 },
	{ 1, 1, 1 },
	{ 1, -1, 1 },
	{ -1, -1, 1 }
}};
sbuff<u32[3], 12> cube_triangles = {{
    {0, 2, 1}, //face front
    {0, 3, 2},
    {2, 3, 4}, //face top
    {2, 4, 5},
    {1, 2, 5}, //face right
    {1, 5, 6},
    {0, 7, 4}, //face left
    {0, 4, 3},
    {5, 4, 7}, //face back
    {5, 7, 6},
    {0, 6, 7}, //face bottom
    {0, 1, 6}
}};

Mesh cube_mesh = {{cube_vertices.buffer, cap(&cube_vertices)}, {cube_triangles.buffer, cap(&cube_triangles)}};

vec3f cube_position = { 0, 0, 3};
quat cube_rotation = {1, 0, 0, 0};

bool is_ortho = true;


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

    dbuff<vec3f> _proj_buffer;
    _proj_buffer.init(8);

    dbuff<u8> proc_buffer;
    proc_buffer.init(80000);

    float line_color[2][4] = {{1, 0.9, 0, 0.9}, {1, 0, 0.9, 0.9}};
    dbuff2f line_color_buffer = {(f32*)line_color, 2, 4};

    // float triangle_color[3][4] = {{1, 0.9, 0, 0.9}, {1, 0, 0.9, 0.9}, {1, 0.9, 0.9, 0}};
    float triangle_color[3][4] = {{1, 1, 0, 0}, {1, 0, 0, 1}, {1, 0, 1, 0}};
    dbuff2f triangle_color_buffer = {(f32*)triangle_color, 3, 4};


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

    dbuff2u window_buffer;
    window_buffer.init(window_max_size.y, window_max_size.x);
    dbuff2f z_buffer;
    z_buffer.init(window_max_size.y, window_max_size.x);
    
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
                    if (key_event->keycode == XKeysymToKeycode(display, XK_o)) {
                        is_ortho = !is_ortho;
                    }

                    if (key_event->keycode == XKeysymToKeycode(display, XK_w)) {
                        cube_position += vec3f(0, 0, 0.1);
                    }
                    if (key_event->keycode == XKeysymToKeycode(display, XK_s)) {
                        cube_position += vec3f(0, 0, -0.1);
                    }
                    if (key_event->keycode == XKeysymToKeycode(display, XK_a)) {
                        cube_position += vec3f(-0.1, 0, 0);
                    }
                    if (key_event->keycode == XKeysymToKeycode(display, XK_d)) {
                        cube_position += vec3f(0.1, 0, 0);
                    }
                } break;
            }
        }
        // write to buffer

        for (auto p = begin(&window_buffer); p < end(&window_buffer); p++) {
            *p = 0xff223344;
        }
        for (auto p = begin(&z_buffer); p < end(&z_buffer); p++) {
            *p = INT_MAX;
        }

        // Color c = {0xff5533ff};
        vec2i pointer_local_pos;
        {
            vec2i pointer_global_pos;
            u32 pointer_mask;
            Window temp1, temp2;
            XQueryPointer(display, window, &temp1, &temp2, &pointer_global_pos.x, &pointer_global_pos.y, &pointer_local_pos.x, &pointer_local_pos.y, &pointer_mask);
        }
        // if (pointer_local_pos.x < window_size.x && pointer_local_pos.y < window_size.y)
        //     rasterize_line(window_buffer, {500, 500}, pointer_local_pos - vec2i(100, 100), 
        //     line_color_buffer, color_itpl_frag_shader, null, set_pixel_color);
        
        // if (pointer_local_pos.x < window_size.x && pointer_local_pos.y < window_size.y)
        //     rasterize_triangle_scanline(window_buffer, {500, 500}, {300, 200}, pointer_local_pos, {0xff00ff00}, proc_buffer, set_pixel_color);

        if (pointer_local_pos.x < window_size.x && pointer_local_pos.y < window_size.y)
            rasterize_triangle_scanline(window_buffer, {500, 500}, {300, 200}, pointer_local_pos, 
                triangle_color_buffer, color_itpl_frag_shader, null, proc_buffer, set_pixel_color);

        // rasterize_triangle_scanline(window_buffer, {500, 500}, {300, 200}, {700, 600}, 
        //         triangle_color_buffer, color_itpl_frag_shader, null, proc_buffer, set_pixel_color);

        // for (i32 i = 1; i < 200; i++) {
        //     rasterize_line(window_buffer, {1800, 100 + i}, {300, 900 + i}, {0xffff55ff + i}, set_pixel_color);
        // }

        // if (is_ortho) {
        //     render_wireframe({&cube_mesh, &cube_position, &cube_rotation}, project_xy_orthogonal, _proj_buffer, 
        //                 window_buffer, {0xffffffff}, window_size, {100, 100});
        // } else {
        //     render_wireframe({&cube_mesh, &cube_position, &cube_rotation}, project_xy_perspective, _proj_buffer, 
        //                 window_buffer, {0xffffffff}, window_size, {100, 100});
        // }
        // if (is_ortho) {
        //     render_wireframe({&cube_mesh, &cube_position, &cube_rotation}, project_xy_orthogonal, _proj_buffer, 
        //                 window_buffer, z_buffer, window_size, {100, 100});
        // } else {
        //     render_wireframe({&cube_mesh, &cube_position, &cube_rotation}, project_xy_perspective, _proj_buffer, 
        //                 window_buffer, z_buffer, window_size, {100, 100});
        // }
        if (is_ortho) {
            render_mesh({&cube_mesh, &cube_position, &cube_rotation}, project_xy_orthogonal, proc_buffer, 
                        window_buffer, z_buffer, window_size, {100, 100});
        } else {
            render_mesh({&cube_mesh, &cube_position, &cube_rotation}, project_xy_perspective, proc_buffer, 
                        window_buffer, z_buffer, window_size, {100, 100});
        }
        

        static quat rot;
        rot.init(normalized(vec3f{1, 1, 1}), M_PI/1000);
        cube_rotation = rot * cube_rotation;


        XPutImage(display, window, default_gc, x_window_buffer, 0, 0, 0, 0, window_size.x, window_size.y);

        XFlush(display);
    }

    XDestroyWindow(display, window);
    XFlush(display);

    window_buffer.shut();

    return 0;
}

