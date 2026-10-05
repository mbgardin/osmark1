/*
 * window.c — Pocket WM Window Framing & Management
 * Copyright (c) 2026 Monte Gardiner — MIT License
 */

#include "pocket-wm.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

/* ── Window lookup ────────────────────────────────────────────────────────── */

PocketWindow *pwm_find_by_frame(PocketWM *wm, Window frame) {
    for (PocketWindow *pw = wm->windows; pw; pw = pw->next)
        if (pw->frame == frame || pw->titlebar == frame ||
            pw->btn_close == frame || pw->btn_max == frame ||
            pw->btn_min == frame)
            return pw;
    return NULL;
}

PocketWindow *pwm_find_by_client(PocketWM *wm, Window client) {
    for (PocketWindow *pw = wm->windows; pw; pw = pw->next)
        if (pw->client == client)
            return pw;
    return NULL;
}

PocketWindow *pwm_find_by_any(PocketWM *wm, Window win) {
    PocketWindow *pw = pwm_find_by_client(wm, win);
    if (!pw) pw = pwm_find_by_frame(wm, win);
    return pw;
}

/* ── Window title ─────────────────────────────────────────────────────────── */

char *pwm_get_window_title(PocketWM *wm, Window win) {
    /* Try _NET_WM_NAME (UTF-8) first */
    Atom actual_type;
    int actual_format;
    unsigned long nitems, bytes_after;
    unsigned char *prop = NULL;

    if (XGetWindowProperty(wm->dpy, win,
            wm->atoms._NET_WM_NAME, 0, 1024, False,
            wm->atoms.UTF8_STRING,
            &actual_type, &actual_format,
            &nitems, &bytes_after, &prop) == Success
            && prop && nitems > 0) {
        char *title = strdup((char *)prop);
        XFree(prop);
        return title;
    }
    if (prop) XFree(prop);

    /* Fall back to WM_NAME */
    char *name = NULL;
    if (XFetchName(wm->dpy, win, &name) && name) {
        char *title = strdup(name);
        XFree(name);
        return title;
    }

    return strdup("Untitled");
}

void pwm_update_title(PocketWM *wm, PocketWindow *pw) {
    char *title = pwm_get_window_title(wm, pw->client);
    if (title) {
        strncpy(pw->title, title, sizeof(pw->title) - 1);
        pw->title[sizeof(pw->title) - 1] = '\0';
        free(title);
    }
    pwm_draw_titlebar(wm, pw);
}

/* ── Window type detection ────────────────────────────────────────────────── */

int pwm_window_should_decorate(PocketWM *wm, Window win) {
    Atom actual_type;
    int actual_format;
    unsigned long nitems, bytes_after;
    unsigned char *prop = NULL;

    /* Check _NET_WM_WINDOW_TYPE */
    if (XGetWindowProperty(wm->dpy, win,
            wm->atoms._NET_WM_WINDOW_TYPE, 0, 32, False, XA_ATOM,
            &actual_type, &actual_format,
            &nitems, &bytes_after, &prop) == Success && prop) {
        Atom type = *(Atom *)prop;
        XFree(prop);
        
        if (type == wm->atoms._NET_WM_WINDOW_TYPE_DESKTOP ||
            type == wm->atoms._NET_WM_WINDOW_TYPE_DOCK    ||
            type == wm->atoms._NET_WM_WINDOW_TYPE_SPLASH  ||
            type == wm->atoms._NET_WM_WINDOW_TYPE_TOOLBAR) {
            return 0;
        }
    }

    /* Check _MOTIF_WM_HINTS for decoration request */
    if (XGetWindowProperty(wm->dpy, win,
            wm->atoms._MOTIF_WM_HINTS, 0, 5, False,
            wm->atoms._MOTIF_WM_HINTS,
            &actual_type, &actual_format,
            &nitems, &bytes_after, &prop) == Success && prop && nitems >= 3) {
        /* Motif hints: flags, functions, decorations... */
        long *hints = (long *)prop;
        if (hints[0] & (1L << 1)) {  /* MWM_HINTS_DECORATIONS */
            int decor = hints[2];
            XFree(prop);
            return decor != 0;
        }
        XFree(prop);
    }

    return 1;
}

/* ── Window framing ───────────────────────────────────────────────────────── */

void pwm_frame_window(PocketWM *wm, Window client, int was_viewable) {
    (void)was_viewable;
    /* Don't double-frame */
    if (pwm_find_by_client(wm, client)) return;

    /* Get window attributes */
    XWindowAttributes attr;
    if (!XGetWindowAttributes(wm->dpy, client, &attr)) return;

    /* Only manage InputOutput windows */
    if (attr.class != InputOutput) return;
    if (attr.override_redirect) return;

    int should_decorate = pwm_window_should_decorate(wm, client);

    PocketWindow *pw = calloc(1, sizeof(PocketWindow));
    if (!pw) return;

    pw->client    = client;
    pw->x         = attr.x;
    pw->y         = attr.y;
    pw->width     = attr.width;
    pw->height    = attr.height;
    pw->state     = STATE_NORMAL;
    pw->decorated = should_decorate;

    /* Get title */
    char *title = pwm_get_window_title(wm, client);
    if (title) {
        strncpy(pw->title, title, sizeof(pw->title) - 1);
        free(title);
    }

    /* Get ICCCM hints */
    XGetWMNormalHints(wm->dpy, client, &pw->size_hints, NULL);
    pw->wm_hints = XGetWMHints(wm->dpy, client);

    /* Check WM_PROTOCOLS for delete/focus support */
    {
        Atom *protos = NULL;
        int n_protos = 0;
        if (XGetWMProtocols(wm->dpy, client, &protos, &n_protos)) {
            for (int i = 0; i < n_protos; i++) {
                if (protos[i] == wm->atoms.WM_DELETE_WINDOW)
                    pw->delete_supported = 1;
                if (protos[i] == wm->atoms.WM_TAKE_FOCUS)
                    pw->take_focus = 1;
            }
            XFree(protos);
        }
    }

    if (should_decorate) {
        /* Position: apply title bar offset */
        if (pw->y < TITLE_BAR_HEIGHT)
            pw->y = TITLE_BAR_HEIGHT;

        /* Create frame window */
        int frame_x = pw->x - BORDER_WIDTH;
        int frame_y = pw->y - TITLE_BAR_HEIGHT - BORDER_WIDTH;
        int frame_w = pw->width  + 2 * BORDER_WIDTH;
        int frame_h = pw->height + TITLE_BAR_HEIGHT + 2 * BORDER_WIDTH;

        XSetWindowAttributes swa;
        swa.border_pixel      = wm->col_border_inactive;
        swa.background_pixel  = wm->col_border_inactive;
        swa.event_mask        = SubstructureRedirectMask |
                                SubstructureNotifyMask   |
                                ExposureMask             |
                                ButtonPressMask          |
                                ButtonReleaseMask        |
                                PointerMotionMask        |
                                EnterWindowMask;

        pw->frame = XCreateWindow(
            wm->dpy, wm->root,
            frame_x, frame_y, frame_w, frame_h,
            BORDER_WIDTH,
            CopyFromParent, InputOutput, CopyFromParent,
            CWBorderPixel | CWBackPixel | CWEventMask,
            &swa);

        /* Title bar sub-window */
        swa.background_pixel = wm->col_title_inactive;
        swa.event_mask       = ExposureMask          |
                               ButtonPressMask       |
                               ButtonReleaseMask     |
                               PointerMotionMask     |
                               EnterWindowMask;

        pw->titlebar = XCreateWindow(
            wm->dpy, pw->frame,
            0, 0, frame_w, TITLE_BAR_HEIGHT,
            0,
            CopyFromParent, InputOutput, CopyFromParent,
            CWBorderPixel | CWBackPixel | CWEventMask,
            &swa);

        /* Reparent client into frame */
        XReparentWindow(wm->dpy, client, pw->frame,
            BORDER_WIDTH,
            TITLE_BAR_HEIGHT + BORDER_WIDTH);

        /* Map frame and title bar */
        XMapWindow(wm->dpy, pw->titlebar);
        XMapWindow(wm->dpy, pw->frame);

        /* Update _NET_FRAME_EXTENTS */
        long extents[4] = { BORDER_WIDTH, BORDER_WIDTH,
                            TITLE_BAR_HEIGHT + BORDER_WIDTH, BORDER_WIDTH };
        XChangeProperty(wm->dpy, client,
            wm->atoms._NET_FRAME_EXTENTS, XA_CARDINAL, 32,
            PropModeReplace, (unsigned char *)extents, 4);

    } else {
        /* Undecorated: frame == client (no reparenting for docks, etc.) */
        pw->frame    = client;
        pw->titlebar = None;
    }

    /* Select input on client */
    XSelectInput(wm->dpy, client,
        PropertyChangeMask | EnterWindowMask | FocusChangeMask);

    /* Add to window list */
    pw->next = wm->windows;
    if (wm->windows) wm->windows->prev = pw;
    wm->windows = pw;
    wm->n_windows++;

    /* Set WM_STATE to Normal */
    {
        long state[2] = { NormalState, None };
        XChangeProperty(wm->dpy, client,
            wm->atoms.WM_STATE, wm->atoms.WM_STATE, 32,
            PropModeReplace, (unsigned char *)state, 2);
    }

    /* Focus the new window */
    pwm_focus_window(wm, pw);

    /* Draw frame */
    pwm_draw_frame(wm, pw);

    /* Update EWMH client list */
    pwm_update_ewmh_client_list(wm);

    printf("[pocket-wm] Framed window 0x%lx (%s)\n", client, pw->title);
}

void pwm_unframe_window(PocketWM *wm, PocketWindow *pw) {
    if (!pw) return;

    printf("[pocket-wm] Unframing window 0x%lx\n", pw->client);

    /* Remove from list */
    if (pw->prev) pw->prev->next = pw->next;
    else          wm->windows    = pw->next;
    if (pw->next) pw->next->prev = pw->prev;
    wm->n_windows--;

    /* Reparent client back to root before destroying frame */
    if (pw->decorated && pw->frame != pw->client) {
        XReparentWindow(wm->dpy, pw->client, wm->root, pw->x, pw->y);
        if (pw->titlebar) XDestroyWindow(wm->dpy, pw->titlebar);
        XDestroyWindow(wm->dpy, pw->frame);
    }

    /* Free WM hints */
    if (pw->wm_hints) XFree(pw->wm_hints);

    /* Focus something else */
    if (wm->focused == pw) {
        wm->focused = NULL;
        if (wm->windows) pwm_focus_window(wm, wm->windows);
    }

    /* Update EWMH */
    pwm_update_ewmh_client_list(wm);
    pwm_update_net_active_window(wm);
}

/* ── Focus ────────────────────────────────────────────────────────────────── */

void pwm_focus_window(PocketWM *wm, PocketWindow *pw) {
    if (!pw) return;

    /* Unfocus previous */
    if (wm->focused && wm->focused != pw) {
        pwm_unfocus_window(wm, wm->focused);
    }

    pw->focused = 1;
    wm->focused = pw;

    /* Set X input focus */
    if (pw->take_focus) {
        pwm_send_client_message(wm, pw->client, wm->atoms.WM_TAKE_FOCUS);
    } else {
        XSetInputFocus(wm->dpy, pw->client,
            RevertToPointerRoot, CurrentTime);
    }

    /* Raise */
    pwm_raise_window(wm, pw);

    /* Redraw with active colors */
    pwm_draw_frame(wm, pw);

    /* Update _NET_ACTIVE_WINDOW */
    pwm_update_net_active_window(wm);
}

void pwm_unfocus_window(PocketWM *wm, PocketWindow *pw) {
    if (!pw) return;
    pw->focused = 0;
    pwm_draw_frame(wm, pw);
}

/* ── Raise ────────────────────────────────────────────────────────────────── */

void pwm_raise_window(PocketWM *wm, PocketWindow *pw) {
    if (!pw) return;
    Window win = (pw->decorated && pw->frame != pw->client)
                 ? pw->frame : pw->client;
    XRaiseWindow(wm->dpy, win);
}

/* ── Move / Resize ────────────────────────────────────────────────────────── */

void pwm_move_window(PocketWM *wm, PocketWindow *pw, int x, int y) {
    pw->x = x;
    pw->y = y;
    int frame_x = x - BORDER_WIDTH;
    int frame_y = y - TITLE_BAR_HEIGHT - BORDER_WIDTH;
    XMoveWindow(wm->dpy, pw->frame, frame_x, frame_y);
}

void pwm_resize_window(PocketWM *wm, PocketWindow *pw, int w, int h) {
    /* Apply size hints */
    if (pw->size_hints.flags & PMinSize) {
        w = w < pw->size_hints.min_width  ? pw->size_hints.min_width  : w;
        h = h < pw->size_hints.min_height ? pw->size_hints.min_height : h;
    }
    if (w < WIN_MIN_WIDTH)  w = WIN_MIN_WIDTH;
    if (h < WIN_MIN_HEIGHT) h = WIN_MIN_HEIGHT;

    pw->width  = w;
    pw->height = h;

    int frame_w = w + 2 * BORDER_WIDTH;
    int frame_h = h + TITLE_BAR_HEIGHT + 2 * BORDER_WIDTH;

    XResizeWindow(wm->dpy, pw->frame, frame_w, frame_h);
    XResizeWindow(wm->dpy, pw->titlebar, frame_w, TITLE_BAR_HEIGHT);
    XResizeWindow(wm->dpy, pw->client, w, h);
}

/* ── Minimize / Maximize / Restore ───────────────────────────────────────── */

void pwm_minimize_window(PocketWM *wm, PocketWindow *pw) {
    if (pw->state == STATE_MINIMIZED) return;

    pw->state = STATE_MINIMIZED;
    XUnmapWindow(wm->dpy, pw->frame);

    /* Set WM_STATE to Iconic */
    long state[2] = { IconicState, None };
    XChangeProperty(wm->dpy, pw->client,
        wm->atoms.WM_STATE, wm->atoms.WM_STATE, 32,
        PropModeReplace, (unsigned char *)state, 2);

    /* _NET_WM_STATE HIDDEN */
    Atom hidden = wm->atoms._NET_WM_STATE_HIDDEN;
    XChangeProperty(wm->dpy, pw->client,
        wm->atoms._NET_WM_STATE, XA_ATOM, 32,
        PropModeReplace, (unsigned char *)&hidden, 1);

    if (wm->focused == pw) {
        wm->focused = NULL;
        /* Focus next available */
        for (PocketWindow *w = wm->windows; w; w = w->next) {
            if (w != pw && w->state != STATE_MINIMIZED) {
                pwm_focus_window(wm, w);
                break;
            }
        }
    }
}

void pwm_maximize_window(PocketWM *wm, PocketWindow *pw) {
    if (pw->state == STATE_MAXIMIZED) {
        pwm_restore_window(wm, pw);
        return;
    }

    /* Save current geometry */
    pw->saved_x = pw->x;
    pw->saved_y = pw->y;
    pw->saved_width  = pw->width;
    pw->saved_height = pw->height;
    pw->state = STATE_MAXIMIZED;

    /* Fill work area */
    int new_x = wm->wa_x + BORDER_WIDTH;
    int new_y = wm->wa_y + TITLE_BAR_HEIGHT + BORDER_WIDTH;
    int new_w = wm->wa_width  - 2 * BORDER_WIDTH;
    int new_h = wm->wa_height - TITLE_BAR_HEIGHT - 2 * BORDER_WIDTH;

    pwm_move_window(wm, pw, new_x, new_y);
    pwm_resize_window(wm, pw, new_w, new_h);

    /* Advertise EWMH state */
    Atom states[2] = {
        wm->atoms._NET_WM_STATE_MAXIMIZED_VERT,
        wm->atoms._NET_WM_STATE_MAXIMIZED_HORZ
    };
    XChangeProperty(wm->dpy, pw->client,
        wm->atoms._NET_WM_STATE, XA_ATOM, 32,
        PropModeReplace, (unsigned char *)states, 2);
}

void pwm_restore_window(PocketWM *wm, PocketWindow *pw) {
    if (pw->state == STATE_MINIMIZED) {
        XMapWindow(wm->dpy, pw->frame);
        pw->state = STATE_NORMAL;

        long state[2] = { NormalState, None };
        XChangeProperty(wm->dpy, pw->client,
            wm->atoms.WM_STATE, wm->atoms.WM_STATE, 32,
            PropModeReplace, (unsigned char *)state, 2);

        XDeleteProperty(wm->dpy, pw->client, wm->atoms._NET_WM_STATE);
        pwm_focus_window(wm, pw);
        return;
    }

    if (pw->state == STATE_MAXIMIZED) {
        pw->state = STATE_NORMAL;
        pwm_move_window(wm, pw, pw->saved_x, pw->saved_y);
        pwm_resize_window(wm, pw, pw->saved_width, pw->saved_height);
        XDeleteProperty(wm->dpy, pw->client, wm->atoms._NET_WM_STATE);
    }
}

/* ── Close ────────────────────────────────────────────────────────────────── */

void pwm_close_window(PocketWM *wm, PocketWindow *pw) {
    if (pw->delete_supported) {
        /* Polite close via WM_DELETE_WINDOW */
        pwm_send_client_message(wm, pw->client, wm->atoms.WM_DELETE_WINDOW);
    } else {
        /* Force kill */
        XKillClient(wm->dpy, pw->client);
    }
}

/* ── EWMH updates ─────────────────────────────────────────────────────────── */

void pwm_update_ewmh_client_list(PocketWM *wm) {
    Window *wins = malloc(wm->n_windows * sizeof(Window));
    if (!wins) return;
    int i = 0;
    for (PocketWindow *pw = wm->windows; pw; pw = pw->next) {
        wins[i++] = pw->client;
    }
    XChangeProperty(wm->dpy, wm->root,
        wm->atoms._NET_CLIENT_LIST, XA_WINDOW, 32,
        PropModeReplace, (unsigned char *)wins, i);
    free(wins);
}

void pwm_update_net_active_window(PocketWM *wm) {
    Window active = wm->focused ? wm->focused->client : None;
    XChangeProperty(wm->dpy, wm->root,
        wm->atoms._NET_ACTIVE_WINDOW, XA_WINDOW, 32,
        PropModeReplace, (unsigned char *)&active, 1);
}

void pwm_update_workarea(PocketWM *wm) {
    long workarea[4] = {
        wm->wa_x, wm->wa_y,
        wm->wa_width, wm->wa_height
    };
    XChangeProperty(wm->dpy, wm->root,
        wm->atoms._NET_WORKAREA, XA_CARDINAL, 32,
        PropModeReplace, (unsigned char *)workarea, 4);
}

void pwm_update_struts(PocketWM *wm) {
    /* Recalculate work area from all window struts */
    int left = 0, right = 0, top = 0, bottom = 0;

    for (PocketWindow *pw = wm->windows; pw; pw = pw->next) {
        Atom actual_type;
        int actual_format;
        unsigned long nitems, bytes_after;
        unsigned char *prop = NULL;

        if (XGetWindowProperty(wm->dpy, pw->client,
                wm->atoms._NET_WM_STRUT_PARTIAL, 0, 12, False, XA_CARDINAL,
                &actual_type, &actual_format, &nitems, &bytes_after, &prop)
                == Success && prop && nitems >= 4) {
            long *strut = (long *)prop;
            if (strut[0] > left)   left   = strut[0];
            if (strut[1] > right)  right  = strut[1];
            if (strut[2] > top)    top    = strut[2];
            if (strut[3] > bottom) bottom = strut[3];
            XFree(prop);
        } else if (prop) {
            XFree(prop);
            /* Try simple strut */
            if (XGetWindowProperty(wm->dpy, pw->client,
                    wm->atoms._NET_WM_STRUT, 0, 4, False, XA_CARDINAL,
                    &actual_type, &actual_format, &nitems, &bytes_after, &prop)
                    == Success && prop && nitems >= 4) {
                long *strut = (long *)prop;
                if (strut[0] > left)   left   = strut[0];
                if (strut[1] > right)  right  = strut[1];
                if (strut[2] > top)    top    = strut[2];
                if (strut[3] > bottom) bottom = strut[3];
                XFree(prop);
            } else if (prop) XFree(prop);
        }
    }

    wm->strut.left   = left;
    wm->strut.right  = right;
    wm->strut.top    = top;
    wm->strut.bottom = bottom;

    wm->wa_x      = left;
    wm->wa_y      = top;
    wm->wa_width  = wm->sw - left - right;
    wm->wa_height = wm->sh - top  - bottom;

    pwm_update_workarea(wm);
}

/* ── Scan existing windows at startup ─────────────────────────────────────── */

void pwm_scan_existing_windows(PocketWM *wm) {
    Window root_ret, parent_ret;
    Window *children = NULL;
    unsigned int n = 0;

    if (!XQueryTree(wm->dpy, wm->root, &root_ret, &parent_ret, &children, &n))
        return;

    for (unsigned int i = 0; i < n; i++) {
        XWindowAttributes attr;
        if (!XGetWindowAttributes(wm->dpy, children[i], &attr)) continue;
        if (attr.override_redirect) continue;
        if (attr.map_state == IsViewable)
            pwm_frame_window(wm, children[i], 1);
    }

    if (children) XFree(children);
}

/* ── Hit testing ──────────────────────────────────────────────────────────── */

HitArea pwm_hittest(PocketWM *wm, PocketWindow *pw, int x, int y) {
    (void)wm;
    int tw = pw->width + 2 * BORDER_WIDTH;
    int bsize = BUTTON_SIZE;
    int bmargin = BUTTON_MARGIN;
    int vtop = (TITLE_BAR_HEIGHT - bsize) / 2;

    /* Within title bar */
    if (y >= 0 && y < TITLE_BAR_HEIGHT) {
        /* Close button */
        int close_x = tw - BTN_CLOSE_X_OFFSET - bmargin;
        if (x >= close_x && x < close_x + bsize &&
            y >= vtop && y < vtop + bsize)
            return HIT_CLOSE;

        /* Maximize button */
        int max_x = tw - BTN_MAX_X_OFFSET - bmargin;
        if (x >= max_x && x < max_x + bsize &&
            y >= vtop && y < vtop + bsize)
            return HIT_MAXIMIZE;

        /* Minimize button */
        int min_x = tw - BTN_MIN_X_OFFSET - bmargin;
        if (x >= min_x && x < min_x + bsize &&
            y >= vtop && y < vtop + bsize)
            return HIT_MINIMIZE;

        return HIT_TITLE;
    }

    /* Border resize areas */
    int fh = pw->height + TITLE_BAR_HEIGHT + 2 * BORDER_WIDTH;
    int CORNER = 16;

    if (y < BORDER_WIDTH) {
        if (x < CORNER) return HIT_BORDER_NW;
        if (x > tw - CORNER) return HIT_BORDER_NE;
        return HIT_BORDER_N;
    }
    if (y > fh - BORDER_WIDTH) {
        if (x < CORNER) return HIT_BORDER_SW;
        if (x > tw - CORNER) return HIT_BORDER_SE;
        return HIT_BORDER_S;
    }
    if (x < BORDER_WIDTH) return HIT_BORDER_W;
    if (x > tw - BORDER_WIDTH) return HIT_BORDER_E;

    return HIT_NONE;
}

/* ── Utilities ────────────────────────────────────────────────────────────── */

void pwm_send_client_message(PocketWM *wm, Window win, Atom proto) {
    XEvent ev;
    memset(&ev, 0, sizeof(ev));
    ev.type                 = ClientMessage;
    ev.xclient.window       = win;
    ev.xclient.message_type = wm->atoms.WM_PROTOCOLS;
    ev.xclient.format       = 32;
    ev.xclient.data.l[0]    = proto;
    ev.xclient.data.l[1]    = CurrentTime;
    XSendEvent(wm->dpy, win, False, NoEventMask, &ev);
}

void pwm_spawn(const char *cmd) {
    if (fork() == 0) {
        setsid();
        execl("/bin/sh", "sh", "-c", cmd, NULL);
        _exit(127);
    }
}
