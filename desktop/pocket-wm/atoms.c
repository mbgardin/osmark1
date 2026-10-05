/*
 * atoms.c — Pocket WM X11 Atom Initialization
 * Copyright (c) 2026 Monte Gardiner — MIT License
 */

#include "pocket-wm.h"
#include <string.h>

void pwm_init_atoms(PocketWM *wm) {
    Display *d = wm->dpy;

    /* ICCCM */
    wm->atoms.WM_DELETE_WINDOW  = XInternAtom(d, "WM_DELETE_WINDOW", False);
    wm->atoms.WM_PROTOCOLS      = XInternAtom(d, "WM_PROTOCOLS", False);
    wm->atoms.WM_STATE          = XInternAtom(d, "WM_STATE", False);
    wm->atoms.WM_TAKE_FOCUS     = XInternAtom(d, "WM_TAKE_FOCUS", False);
    wm->atoms.WM_CHANGE_STATE   = XInternAtom(d, "WM_CHANGE_STATE", False);
    wm->atoms.WM_NAME           = XInternAtom(d, "WM_NAME", False);
    wm->atoms.WM_CLASS          = XInternAtom(d, "WM_CLASS", False);
    wm->atoms.WM_HINTS          = XInternAtom(d, "WM_HINTS", False);
    wm->atoms.WM_NORMAL_HINTS   = XInternAtom(d, "WM_NORMAL_HINTS", False);
    wm->atoms.WM_TRANSIENT_FOR  = XInternAtom(d, "WM_TRANSIENT_FOR", False);
    wm->atoms.WM_CLIENT_MACHINE = XInternAtom(d, "WM_CLIENT_MACHINE", False);
    wm->atoms.WM_ICON_NAME      = XInternAtom(d, "WM_ICON_NAME", False);

    /* EWMH */
    wm->atoms._NET_SUPPORTED        = XInternAtom(d, "_NET_SUPPORTED", False);
    wm->atoms._NET_CLIENT_LIST      = XInternAtom(d, "_NET_CLIENT_LIST", False);
    wm->atoms._NET_NUMBER_OF_DESKTOPS = XInternAtom(d, "_NET_NUMBER_OF_DESKTOPS", False);
    wm->atoms._NET_DESKTOP_NAMES    = XInternAtom(d, "_NET_DESKTOP_NAMES", False);
    wm->atoms._NET_CURRENT_DESKTOP  = XInternAtom(d, "_NET_CURRENT_DESKTOP", False);
    wm->atoms._NET_ACTIVE_WINDOW    = XInternAtom(d, "_NET_ACTIVE_WINDOW", False);
    wm->atoms._NET_WM_NAME          = XInternAtom(d, "_NET_WM_NAME", False);
    wm->atoms._NET_WM_VISIBLE_NAME  = XInternAtom(d, "_NET_WM_VISIBLE_NAME", False);
    wm->atoms._NET_WM_ICON_NAME     = XInternAtom(d, "_NET_WM_ICON_NAME", False);
    wm->atoms._NET_WM_DESKTOP       = XInternAtom(d, "_NET_WM_DESKTOP", False);
    wm->atoms._NET_WM_WINDOW_TYPE   = XInternAtom(d, "_NET_WM_WINDOW_TYPE", False);
    wm->atoms._NET_WM_WINDOW_TYPE_DESKTOP = XInternAtom(d, "_NET_WM_WINDOW_TYPE_DESKTOP", False);
    wm->atoms._NET_WM_WINDOW_TYPE_DOCK    = XInternAtom(d, "_NET_WM_WINDOW_TYPE_DOCK", False);
    wm->atoms._NET_WM_WINDOW_TYPE_TOOLBAR = XInternAtom(d, "_NET_WM_WINDOW_TYPE_TOOLBAR", False);
    wm->atoms._NET_WM_WINDOW_TYPE_MENU    = XInternAtom(d, "_NET_WM_WINDOW_TYPE_MENU", False);
    wm->atoms._NET_WM_WINDOW_TYPE_UTILITY = XInternAtom(d, "_NET_WM_WINDOW_TYPE_UTILITY", False);
    wm->atoms._NET_WM_WINDOW_TYPE_SPLASH  = XInternAtom(d, "_NET_WM_WINDOW_TYPE_SPLASH", False);
    wm->atoms._NET_WM_WINDOW_TYPE_DIALOG  = XInternAtom(d, "_NET_WM_WINDOW_TYPE_DIALOG", False);
    wm->atoms._NET_WM_WINDOW_TYPE_NORMAL  = XInternAtom(d, "_NET_WM_WINDOW_TYPE_NORMAL", False);
    wm->atoms._NET_WM_STATE                = XInternAtom(d, "_NET_WM_STATE", False);
    wm->atoms._NET_WM_STATE_MODAL          = XInternAtom(d, "_NET_WM_STATE_MODAL", False);
    wm->atoms._NET_WM_STATE_MAXIMIZED_VERT = XInternAtom(d, "_NET_WM_STATE_MAXIMIZED_VERT", False);
    wm->atoms._NET_WM_STATE_MAXIMIZED_HORZ = XInternAtom(d, "_NET_WM_STATE_MAXIMIZED_HORZ", False);
    wm->atoms._NET_WM_STATE_FULLSCREEN     = XInternAtom(d, "_NET_WM_STATE_FULLSCREEN", False);
    wm->atoms._NET_WM_STATE_HIDDEN         = XInternAtom(d, "_NET_WM_STATE_HIDDEN", False);
    wm->atoms._NET_WM_STATE_ABOVE          = XInternAtom(d, "_NET_WM_STATE_ABOVE", False);
    wm->atoms._NET_WM_STATE_BELOW          = XInternAtom(d, "_NET_WM_STATE_BELOW", False);
    wm->atoms._NET_WM_STRUT                = XInternAtom(d, "_NET_WM_STRUT", False);
    wm->atoms._NET_WM_STRUT_PARTIAL        = XInternAtom(d, "_NET_WM_STRUT_PARTIAL", False);
    wm->atoms._NET_WM_PID                  = XInternAtom(d, "_NET_WM_PID", False);
    wm->atoms._NET_WM_ICON                 = XInternAtom(d, "_NET_WM_ICON", False);
    wm->atoms._NET_CLOSE_WINDOW            = XInternAtom(d, "_NET_CLOSE_WINDOW", False);
    wm->atoms._NET_MOVERESIZE_WINDOW       = XInternAtom(d, "_NET_MOVERESIZE_WINDOW", False);
    wm->atoms._NET_WM_MOVERESIZE           = XInternAtom(d, "_NET_WM_MOVERESIZE", False);
    wm->atoms._NET_WORKAREA                = XInternAtom(d, "_NET_WORKAREA", False);
    wm->atoms._NET_SUPPORTING_WM_CHECK    = XInternAtom(d, "_NET_SUPPORTING_WM_CHECK", False);
    wm->atoms._NET_FRAME_EXTENTS          = XInternAtom(d, "_NET_FRAME_EXTENTS", False);
    wm->atoms._MOTIF_WM_HINTS             = XInternAtom(d, "_MOTIF_WM_HINTS", False);
    wm->atoms.UTF8_STRING                 = XInternAtom(d, "UTF8_STRING", False);
}

void pwm_init_ewmh(PocketWM *wm) {
    /* Create EWMH supporting check window */
    wm->ewmh_check_win = XCreateSimpleWindow(
        wm->dpy, wm->root,
        -100, -100, 1, 1, 0, 0, 0);

    /* _NET_SUPPORTING_WM_CHECK on root and check window */
    XChangeProperty(wm->dpy, wm->root,
        wm->atoms._NET_SUPPORTING_WM_CHECK, XA_WINDOW, 32,
        PropModeReplace, (unsigned char *)&wm->ewmh_check_win, 1);
    XChangeProperty(wm->dpy, wm->ewmh_check_win,
        wm->atoms._NET_SUPPORTING_WM_CHECK, XA_WINDOW, 32,
        PropModeReplace, (unsigned char *)&wm->ewmh_check_win, 1);

    /* WM name */
    const char *wm_name = "pocket-wm";
    XChangeProperty(wm->dpy, wm->ewmh_check_win,
        wm->atoms._NET_WM_NAME, wm->atoms.UTF8_STRING, 8,
        PropModeReplace, (unsigned char *)wm_name, strlen(wm_name));

    /* _NET_SUPPORTED — advertise supported EWMH atoms */
    Atom supported[] = {
        wm->atoms._NET_SUPPORTED,
        wm->atoms._NET_CLIENT_LIST,
        wm->atoms._NET_NUMBER_OF_DESKTOPS,
        wm->atoms._NET_CURRENT_DESKTOP,
        wm->atoms._NET_ACTIVE_WINDOW,
        wm->atoms._NET_WM_NAME,
        wm->atoms._NET_WM_DESKTOP,
        wm->atoms._NET_WM_WINDOW_TYPE,
        wm->atoms._NET_WM_WINDOW_TYPE_DESKTOP,
        wm->atoms._NET_WM_WINDOW_TYPE_DOCK,
        wm->atoms._NET_WM_WINDOW_TYPE_DIALOG,
        wm->atoms._NET_WM_WINDOW_TYPE_NORMAL,
        wm->atoms._NET_WM_STATE,
        wm->atoms._NET_WM_STATE_MAXIMIZED_VERT,
        wm->atoms._NET_WM_STATE_MAXIMIZED_HORZ,
        wm->atoms._NET_WM_STATE_HIDDEN,
        wm->atoms._NET_WM_STATE_FULLSCREEN,
        wm->atoms._NET_WM_STATE_ABOVE,
        wm->atoms._NET_WM_STATE_BELOW,
        wm->atoms._NET_WM_STRUT,
        wm->atoms._NET_WM_STRUT_PARTIAL,
        wm->atoms._NET_WM_PID,
        wm->atoms._NET_CLOSE_WINDOW,
        wm->atoms._NET_WORKAREA,
        wm->atoms._NET_SUPPORTING_WM_CHECK,
        wm->atoms._NET_FRAME_EXTENTS,
    };
    XChangeProperty(wm->dpy, wm->root,
        wm->atoms._NET_SUPPORTED, XA_ATOM, 32,
        PropModeReplace, (unsigned char *)supported,
        sizeof(supported) / sizeof(Atom));

    /* _NET_NUMBER_OF_DESKTOPS: 1 desktop for now */
    long n_desktops = 1;
    XChangeProperty(wm->dpy, wm->root,
        wm->atoms._NET_NUMBER_OF_DESKTOPS, XA_CARDINAL, 32,
        PropModeReplace, (unsigned char *)&n_desktops, 1);

    /* _NET_CURRENT_DESKTOP: 0 */
    long cur_desktop = 0;
    XChangeProperty(wm->dpy, wm->root,
        wm->atoms._NET_CURRENT_DESKTOP, XA_CARDINAL, 32,
        PropModeReplace, (unsigned char *)&cur_desktop, 1);

    /* Initialize work area */
    wm->wa_x = 0;
    wm->wa_y = 0;
    wm->wa_width  = wm->sw;
    wm->wa_height = wm->sh;
    pwm_update_workarea(wm);
}
