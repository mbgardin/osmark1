/*
 * pocket-wm — Pocket OS Window Manager
 * 
 * A reparenting X11 window manager for Pocket OS.
 * Provides XP-inspired window decorations, focus management,
 * and EWMH compliance.
 *
 * Copyright (c) 2026 Monte Gardiner
 * License: MIT
 *
 * Built on Debian/Linux foundation using Xlib.
 * X11/ICCCM/EWMH compliance for application compatibility.
 */

#include "pocket-wm.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <sys/wait.h>

/* Global state */
PocketWM *pwm = NULL;
static volatile int running = 1;

/* ── Signal handling ──────────────────────────────────────────────────────── */

static void handle_signal(int sig) {
    if (sig == SIGCHLD) {
        /* Reap zombie child processes */
        while (waitpid(-1, NULL, WNOHANG) > 0);
    } else {
        running = 0;
    }
}

/* ── X11 error handler ────────────────────────────────────────────────────── */

static int x_error_handler(Display *dpy, XErrorEvent *ev) {
    char buf[256];
    XGetErrorText(dpy, ev->error_code, buf, sizeof(buf));
    
    /* Ignore common benign errors */
    if (ev->error_code == BadWindow ||
        ev->error_code == BadDrawable ||
        (ev->error_code == BadAccess && ev->request_code == X_GrabButton) ||
        (ev->error_code == BadAccess && ev->request_code == X_GrabKey)) {
        return 0;
    }
    
    fprintf(stderr, "[pocket-wm] X Error: %s (op=%d/%d, res=0x%lx)\n",
            buf, ev->request_code, ev->minor_code, ev->resourceid);
    return 0;
}

static int x_io_error_handler(Display *dpy) {
    fprintf(stderr, "[pocket-wm] Fatal X IO error. Exiting.\n");
    exit(EXIT_FAILURE);
}

/* ── WM initialization ────────────────────────────────────────────────────── */

PocketWM *pwm_init(void) {
    PocketWM *wm = calloc(1, sizeof(PocketWM));
    if (!wm) {
        perror("calloc");
        return NULL;
    }
    
    /* Connect to X display */
    const char *display_name = getenv("DISPLAY");
    wm->dpy = XOpenDisplay(display_name);
    if (!wm->dpy) {
        fprintf(stderr, "[pocket-wm] Cannot open display: %s\n",
                display_name ? display_name : ":0");
        free(wm);
        return NULL;
    }
    
    wm->screen = DefaultScreen(wm->dpy);
    wm->root   = RootWindow(wm->dpy, wm->screen);
    wm->sw     = DisplayWidth(wm->dpy, wm->screen);
    wm->sh     = DisplayHeight(wm->dpy, wm->screen);
    
    XSetErrorHandler(x_error_handler);
    XSetIOErrorHandler(x_io_error_handler);
    
    /* Become the WM — select SubstructureRedirectMask on root */
    XSelectInput(wm->dpy, wm->root,
        SubstructureRedirectMask |
        SubstructureNotifyMask  |
        ButtonPressMask          |
        KeyPressMask             |
        PointerMotionMask        |
        PropertyChangeMask       |
        EnterWindowMask);
    
    XSync(wm->dpy, False);
    
    /* Initialize atoms */
    pwm_init_atoms(wm);
    
    /* Initialize EWMH */
    pwm_init_ewmh(wm);
    
    /* Initialize color scheme (XP-inspired) */
    pwm_init_colors(wm);
    
    /* Initialize fonts */
    pwm_init_fonts(wm);
    
    /* Initialize key bindings */
    pwm_init_keybindings(wm);
    
    /* Manage any existing windows */
    pwm_scan_existing_windows(wm);
    
    printf("[pocket-wm] Pocket Window Manager started.\n");
    printf("[pocket-wm] Display: %dx%d\n", wm->sw, wm->sh);
    
    return wm;
}

/* ── Main event loop ──────────────────────────────────────────────────────── */

void pwm_run(PocketWM *wm) {
    XEvent ev;
    
    while (running) {
        XNextEvent(wm->dpy, &ev);
        
        switch (ev.type) {
            /* Map/unmap */
            case MapRequest:
                pwm_handle_map_request(wm, &ev.xmaprequest);
                break;
            case UnmapNotify:
                pwm_handle_unmap_notify(wm, &ev.xunmap);
                break;
            case DestroyNotify:
                pwm_handle_destroy_notify(wm, &ev.xdestroywindow);
                break;
            case ConfigureRequest:
                pwm_handle_configure_request(wm, &ev.xconfigurerequest);
                break;
            case ConfigureNotify:
                /* Handle screen geometry changes */
                if (ev.xconfigure.window == wm->root) {
                    wm->sw = ev.xconfigure.width;
                    wm->sh = ev.xconfigure.height;
                }
                break;
            
            /* Focus */
            case EnterNotify:
                pwm_handle_enter_notify(wm, &ev.xcrossing);
                break;
            case FocusIn:
            case FocusOut:
                /* handled implicitly via EnterNotify for now */
                break;
            
            /* Mouse */
            case ButtonPress:
                pwm_handle_button_press(wm, &ev.xbutton);
                break;
            case ButtonRelease:
                pwm_handle_button_release(wm, &ev.xbutton);
                break;
            case MotionNotify:
                pwm_handle_motion_notify(wm, &ev.xmotion);
                break;
            
            /* Keyboard */
            case KeyPress:
                pwm_handle_key_press(wm, &ev.xkey);
                break;
            
            /* Properties */
            case PropertyNotify:
                pwm_handle_property_notify(wm, &ev.xproperty);
                break;
            
            /* Client messages (EWMH) */
            case ClientMessage:
                pwm_handle_client_message(wm, &ev.xclient);
                break;
            
            /* Expose (title bar redraws) */
            case Expose:
                if (ev.xexpose.count == 0) {
                    pwm_handle_expose(wm, &ev.xexpose);
                }
                break;
            
            /* RandR / Xinerama */
            case ReparentNotify:
                /* Track reparenting */
                break;
            
            default:
                /* Extension events (RandR etc.) handled separately */
                pwm_handle_extension_event(wm, &ev);
                break;
        }
    }
}

/* ── Cleanup ──────────────────────────────────────────────────────────────── */

void pwm_cleanup(PocketWM *wm) {
    if (!wm) return;
    
    /* Destroy all frame windows */
    PocketWindow *pw = wm->windows;
    while (pw) {
        PocketWindow *next = pw->next;
        pwm_unframe_window(wm, pw);
        free(pw);
        pw = next;
    }
    
    /* Free graphics contexts */
    if (wm->gc_title_active)   XFreeGC(wm->dpy, wm->gc_title_active);
    if (wm->gc_title_inactive) XFreeGC(wm->dpy, wm->gc_title_inactive);
    if (wm->gc_border)         XFreeGC(wm->dpy, wm->gc_border);
    if (wm->gc_button)         XFreeGC(wm->dpy, wm->gc_button);
    if (wm->font)              XFreeFont(wm->dpy, wm->font);
    
    XCloseDisplay(wm->dpy);
    free(wm);
}

/* ── Entry point ──────────────────────────────────────────────────────────── */

int main(int argc, char *argv[]) {
    /* Parse simple arguments */
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--version") == 0) {
            printf("pocket-wm 0.1.0\n");
            printf("Pocket OS Window Manager\n");
            printf("Created by Monte Gardiner\n");
            printf("Built on Debian/Linux, X11/Xlib\n");
            return 0;
        }
        if (strcmp(argv[i], "--help") == 0) {
            printf("Usage: pocket-wm [OPTIONS]\n");
            printf("  --version  Show version\n");
            printf("  --help     Show this help\n");
            printf("\n");
            printf("pocket-wm is the Pocket OS window manager.\n");
            printf("It is normally started by pocket-session.\n");
            return 0;
        }
    }
    
    /* Set up signal handlers */
    signal(SIGTERM, handle_signal);
    signal(SIGINT,  handle_signal);
    signal(SIGCHLD, handle_signal);
    signal(SIGHUP,  handle_signal);
    
    /* Initialize */
    pwm = pwm_init();
    if (!pwm) {
        fprintf(stderr, "[pocket-wm] Failed to initialize. Exiting.\n");
        return EXIT_FAILURE;
    }
    
    /* Run event loop */
    pwm_run(pwm);
    
    /* Cleanup */
    pwm_cleanup(pwm);
    
    return EXIT_SUCCESS;
}
