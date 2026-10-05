/*
 * events.c — Pocket WM Event Handlers
 * Copyright (c) 2026 Monte Gardiner — MIT License
 */

#include "pocket-wm.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ── Map Request ──────────────────────────────────────────────────────────── */

void pwm_handle_map_request(PocketWM *wm, XMapRequestEvent *ev) {
    /* Only manage windows that aren't already managed */
    if (!pwm_find_by_client(wm, ev->window)) {
        pwm_frame_window(wm, ev->window, 0);
    }
    XMapWindow(wm->dpy, ev->window);
}

/* ── Unmap Notify ─────────────────────────────────────────────────────────── */

void pwm_handle_unmap_notify(PocketWM *wm, XUnmapEvent *ev) {
    /* Ignore UnmapNotify for our own frame windows */
    if (ev->event == wm->root && ev->send_event) {
        /* Client requested unmapping */
        PocketWindow *pw = pwm_find_by_client(wm, ev->window);
        if (pw && pw->state != STATE_MINIMIZED) {
            pwm_minimize_window(wm, pw);
        }
    }
}

/* ── Destroy Notify ───────────────────────────────────────────────────────── */

void pwm_handle_destroy_notify(PocketWM *wm, XDestroyWindowEvent *ev) {
    PocketWindow *pw = pwm_find_by_client(wm, ev->window);
    if (!pw) pw = pwm_find_by_frame(wm, ev->window);
    if (pw) {
        pwm_unframe_window(wm, pw);
        free(pw);
    }
}

/* ── Configure Request ────────────────────────────────────────────────────── */

void pwm_handle_configure_request(PocketWM *wm, XConfigureRequestEvent *ev) {
    PocketWindow *pw = pwm_find_by_client(wm, ev->window);

    if (pw && pw->decorated) {
        /* Translate configure request for framed window */
        if (ev->value_mask & (CWX | CWY)) {
            if (ev->value_mask & CWX) pw->x = ev->x;
            if (ev->value_mask & CWY) pw->y = ev->y;
            pwm_move_window(wm, pw, pw->x, pw->y);
        }
        if (ev->value_mask & (CWWidth | CWHeight)) {
            int w = (ev->value_mask & CWWidth)  ? ev->width  : pw->width;
            int h = (ev->value_mask & CWHeight) ? ev->height : pw->height;
            pwm_resize_window(wm, pw, w, h);
        }
        if (ev->value_mask & CWStackMode) {
            if (ev->detail == Above || ev->detail == TopIf)
                pwm_raise_window(wm, pw);
        }
    } else {
        /* Unmanaged or undecorated: pass through */
        XWindowChanges wc;
        wc.x            = ev->x;
        wc.y            = ev->y;
        wc.width        = ev->width;
        wc.height       = ev->height;
        wc.border_width = ev->border_width;
        wc.sibling      = ev->above;
        wc.stack_mode   = ev->detail;
        XConfigureWindow(wm->dpy, ev->window, ev->value_mask, &wc);
    }
}

/* ── Enter Notify (focus-follows-pointer) ─────────────────────────────────── */

void pwm_handle_enter_notify(PocketWM *wm, XCrossingEvent *ev) {
    if (ev->mode != NotifyNormal) return;
    if (ev->detail == NotifyInferior) return;

    PocketWindow *pw = pwm_find_by_any(wm, ev->window);
    if (pw && pw != wm->focused) {
        pwm_focus_window(wm, pw);
    }
}

/* ── Button Press ─────────────────────────────────────────────────────────── */

void pwm_handle_button_press(PocketWM *wm, XButtonEvent *ev) {
    PocketWindow *pw = pwm_find_by_any(wm, ev->window);
    if (!pw) {
        /* Click on root — unfocus */
        if (ev->window == wm->root) {
            XSetInputFocus(wm->dpy, wm->root,
                RevertToPointerRoot, CurrentTime);
        }
        return;
    }

    /* Raise on click */
    pwm_raise_window(wm, pw);
    pwm_focus_window(wm, pw);

    if (!pw->decorated) return;

    /* Convert coordinates relative to frame */
    int fx = ev->x_root - (pw->x - BORDER_WIDTH);
    int fy = ev->y_root - (pw->y - TITLE_BAR_HEIGHT - BORDER_WIDTH);

    HitArea hit = pwm_hittest(wm, pw, fx, fy);

    if (ev->button == Button1) {
        switch (hit) {
            case HIT_CLOSE:
                pwm_close_window(wm, pw);
                return;
            case HIT_MAXIMIZE:
                pwm_maximize_window(wm, pw);
                return;
            case HIT_MINIMIZE:
                pwm_minimize_window(wm, pw);
                return;
            case HIT_TITLE:
                /* Double-click to maximize */
                if (ev->time - wm->last_click_time < DBLCLICK_TIME &&
                    wm->last_click_win == ev->window) {
                    pwm_maximize_window(wm, pw);
                    wm->last_click_time = 0;
                    wm->last_click_win  = None;
                    return;
                }
                wm->last_click_time = ev->time;
                wm->last_click_win  = ev->window;

                /* Start move drag */
                pw->drag_mode      = DRAG_MOVE;
                pw->drag_start_x   = ev->x_root;
                pw->drag_start_y   = ev->y_root;
                pw->drag_start_fx  = pw->x;
                pw->drag_start_fy  = pw->y;
                XGrabPointer(wm->dpy, wm->root, False,
                    ButtonReleaseMask | PointerMotionMask,
                    GrabModeAsync, GrabModeAsync,
                    None, None, CurrentTime);
                return;
            case HIT_BORDER_N:
            case HIT_BORDER_S:
            case HIT_BORDER_E:
            case HIT_BORDER_W:
            case HIT_BORDER_NE:
            case HIT_BORDER_NW:
            case HIT_BORDER_SE:
            case HIT_BORDER_SW:
                /* Start resize drag */
                pw->drag_mode      = (DragMode)hit;
                pw->drag_start_x   = ev->x_root;
                pw->drag_start_y   = ev->y_root;
                pw->drag_start_fx  = pw->x;
                pw->drag_start_fy  = pw->y;
                pw->drag_start_fw  = pw->width;
                pw->drag_start_fh  = pw->height;
                XGrabPointer(wm->dpy, wm->root, False,
                    ButtonReleaseMask | PointerMotionMask,
                    GrabModeAsync, GrabModeAsync,
                    None, None, CurrentTime);
                return;
            default:
                break;
        }
    } else if (ev->button == Button3 && hit == HIT_TITLE) {
        /* Right-click on title bar: window menu (future) */
    }
}

/* ── Button Release ───────────────────────────────────────────────────────── */

void pwm_handle_button_release(PocketWM *wm, XButtonEvent *ev) {
    /* Find the window being dragged */
    for (PocketWindow *pw = wm->windows; pw; pw = pw->next) {
        if (pw->drag_mode != DRAG_NONE) {
            pw->drag_mode = DRAG_NONE;
            XUngrabPointer(wm->dpy, CurrentTime);
            return;
        }
    }
}

/* ── Motion Notify ────────────────────────────────────────────────────────── */

void pwm_handle_motion_notify(PocketWM *wm, XMotionEvent *ev) {
    for (PocketWindow *pw = wm->windows; pw; pw = pw->next) {
        if (pw->drag_mode == DRAG_NONE) continue;

        int dx = ev->x_root - pw->drag_start_x;
        int dy = ev->y_root - pw->drag_start_y;

        if (pw->drag_mode == DRAG_MOVE) {
            int new_x = pw->drag_start_fx + dx;
            int new_y = pw->drag_start_fy + dy;

            /* Snap to screen edges */
            if (abs(new_x) < SNAP_THRESHOLD) new_x = 0;
            if (abs(new_y - wm->wa_y) < SNAP_THRESHOLD) new_y = wm->wa_y;
            if (abs(new_x + pw->width - wm->sw) < SNAP_THRESHOLD)
                new_x = wm->sw - pw->width;

            pwm_move_window(wm, pw, new_x, new_y);

        } else {
            /* Resize */
            int new_w = pw->drag_start_fw;
            int new_h = pw->drag_start_fh;
            int new_x = pw->drag_start_fx;
            int new_y = pw->drag_start_fy;

            switch (pw->drag_mode) {
                case DRAG_RESIZE_E:  new_w += dx; break;
                case DRAG_RESIZE_S:  new_h += dy; break;
                case DRAG_RESIZE_W:  new_w -= dx; new_x += dx; break;
                case DRAG_RESIZE_N:  new_h -= dy; new_y += dy; break;
                case DRAG_RESIZE_SE: new_w += dx; new_h += dy; break;
                case DRAG_RESIZE_SW: new_w -= dx; new_h += dy; new_x += dx; break;
                case DRAG_RESIZE_NE: new_w += dx; new_h -= dy; new_y += dy; break;
                case DRAG_RESIZE_NW: new_w -= dx; new_h -= dy; new_x += dx; new_y += dy; break;
                default: break;
            }

            if (new_w >= WIN_MIN_WIDTH && new_h >= WIN_MIN_HEIGHT) {
                pwm_move_window(wm, pw, new_x, new_y);
                pwm_resize_window(wm, pw, new_w, new_h);
            }
        }
        return;
    }
}

/* ── Key Press ────────────────────────────────────────────────────────────── */

void pwm_init_keybindings(PocketWM *wm) {
    /* Alt+F4 — close focused window */
    XGrabKey(wm->dpy, XKeysymToKeycode(wm->dpy, XK_F4),
        Mod1Mask, wm->root, True, GrabModeAsync, GrabModeAsync);
    
    /* Alt+Tab — cycle windows */
    XGrabKey(wm->dpy, XKeysymToKeycode(wm->dpy, XK_Tab),
        Mod1Mask, wm->root, True, GrabModeAsync, GrabModeAsync);
    
    /* Super+D — show desktop (minimize all) */
    XGrabKey(wm->dpy, XKeysymToKeycode(wm->dpy, XK_d),
        Mod4Mask, wm->root, True, GrabModeAsync, GrabModeAsync);
    
    /* Super+E — launch Pocket Explorer */
    XGrabKey(wm->dpy, XKeysymToKeycode(wm->dpy, XK_e),
        Mod4Mask, wm->root, True, GrabModeAsync, GrabModeAsync);
    
    /* Super+T / Super+Return — launch terminal */
    XGrabKey(wm->dpy, XKeysymToKeycode(wm->dpy, XK_Return),
        Mod4Mask, wm->root, True, GrabModeAsync, GrabModeAsync);
    XGrabKey(wm->dpy, XKeysymToKeycode(wm->dpy, XK_t),
        Mod4Mask, wm->root, True, GrabModeAsync, GrabModeAsync);
    
    /* Alt+F10 — maximize/restore */
    XGrabKey(wm->dpy, XKeysymToKeycode(wm->dpy, XK_F10),
        Mod1Mask, wm->root, True, GrabModeAsync, GrabModeAsync);
}

void pwm_handle_key_press(PocketWM *wm, XKeyEvent *ev) {
    KeySym sym = XkbKeycodeToKeysym(wm->dpy, ev->keycode, 0, 0);

    /* Alt+F4 — close */
    if (sym == XK_F4 && (ev->state & Mod1Mask)) {
        if (wm->focused) pwm_close_window(wm, wm->focused);
        return;
    }

    /* Alt+Tab — cycle */
    if (sym == XK_Tab && (ev->state & Mod1Mask)) {
        pwm_alttab_start(wm);
        return;
    }

    /* Alt+F10 — maximize/restore */
    if (sym == XK_F10 && (ev->state & Mod1Mask)) {
        if (wm->focused) pwm_maximize_window(wm, wm->focused);
        return;
    }

    /* Super+D — minimize all */
    if (sym == XK_d && (ev->state & Mod4Mask)) {
        for (PocketWindow *pw = wm->windows; pw; pw = pw->next) {
            if (pw->state == STATE_NORMAL)
                pwm_minimize_window(wm, pw);
        }
        return;
    }

    /* Super+Return or Super+T — terminal */
    if ((sym == XK_Return || sym == XK_t) && (ev->state & Mod4Mask)) {
        pwm_spawn("pocket-terminal 2>/dev/null || x-terminal-emulator 2>/dev/null || xterm");
        return;
    }

    /* Super+E — explorer */
    if (sym == XK_e && (ev->state & Mod4Mask)) {
        pwm_spawn("pocket-explorer 2>/dev/null || xterm");
        return;
    }
}

/* ── Property Notify ──────────────────────────────────────────────────────── */

void pwm_handle_property_notify(PocketWM *wm, XPropertyEvent *ev) {
    PocketWindow *pw = pwm_find_by_client(wm, ev->window);
    if (!pw) return;

    if (ev->atom == wm->atoms.WM_NAME ||
        ev->atom == wm->atoms._NET_WM_NAME) {
        pwm_update_title(wm, pw);
    }
}

/* ── Client Message ───────────────────────────────────────────────────────── */

void pwm_handle_client_message(PocketWM *wm, XClientMessageEvent *ev) {
    PocketWindow *pw = pwm_find_by_client(wm, ev->window);

    if (ev->message_type == wm->atoms._NET_WM_STATE) {
        if (!pw) return;
        /* action: 0=remove, 1=add, 2=toggle */
        long action = ev->data.l[0];
        Atom prop1  = ev->data.l[1];
        Atom prop2  = ev->data.l[2];

        for (int i = 0; i < 2; i++) {
            Atom prop = (i == 0) ? prop1 : prop2;
            if (!prop) continue;

            if (prop == wm->atoms._NET_WM_STATE_MAXIMIZED_VERT ||
                prop == wm->atoms._NET_WM_STATE_MAXIMIZED_HORZ) {
                if (action == 1 || (action == 2 && pw->state != STATE_MAXIMIZED))
                    pwm_maximize_window(wm, pw);
                else
                    pwm_restore_window(wm, pw);
            }
            if (prop == wm->atoms._NET_WM_STATE_FULLSCREEN) {
                /* Handle fullscreen (TODO: Phase 5) */
            }
            if (prop == wm->atoms._NET_WM_STATE_HIDDEN) {
                if (action == 1)
                    pwm_minimize_window(wm, pw);
                else
                    pwm_restore_window(wm, pw);
            }
        }
        return;
    }

    if (ev->message_type == wm->atoms._NET_CLOSE_WINDOW) {
        if (pw) pwm_close_window(wm, pw);
        return;
    }

    if (ev->message_type == wm->atoms._NET_ACTIVE_WINDOW) {
        if (pw) {
            if (pw->state == STATE_MINIMIZED)
                pwm_restore_window(wm, pw);
            pwm_focus_window(wm, pw);
        }
        return;
    }

    if (ev->message_type == wm->atoms.WM_CHANGE_STATE) {
        if (pw && ev->data.l[0] == IconicState)
            pwm_minimize_window(wm, pw);
        return;
    }
}

/* ── Expose ───────────────────────────────────────────────────────────────── */

void pwm_handle_expose(PocketWM *wm, XExposeEvent *ev) {
    PocketWindow *pw = pwm_find_by_any(wm, ev->window);
    if (pw) pwm_draw_frame(wm, pw);
}

/* ── Extension events (RandR) ─────────────────────────────────────────────── */

void pwm_handle_extension_event(PocketWM *wm, XEvent *ev) {
    if (!wm->have_randr) return;
    
    if (ev->type == wm->randr_event_base + RRScreenChangeNotify) {
        XRRScreenChangeNotifyEvent *rrev = (XRRScreenChangeNotifyEvent *)ev;
        wm->sw = rrev->width;
        wm->sh = rrev->height;
        wm->wa_width  = wm->sw;
        wm->wa_height = wm->sh;
        pwm_update_struts(wm);
        printf("[pocket-wm] Screen change: %dx%d\n", wm->sw, wm->sh);
    }
}

/* ── Alt+Tab ──────────────────────────────────────────────────────────────── */

void pwm_alttab_start(PocketWM *wm) {
    /* Simple Alt+Tab: focus next window in list */
    if (!wm->windows) return;

    PocketWindow *start = wm->focused ? wm->focused->next : wm->windows;
    if (!start) start = wm->windows;

    /* Skip minimized */
    PocketWindow *candidate = start;
    while (candidate && candidate->state == STATE_MINIMIZED)
        candidate = candidate->next;
    if (!candidate) {
        candidate = wm->windows;
        while (candidate && candidate->state == STATE_MINIMIZED)
            candidate = candidate->next;
    }

    if (candidate && candidate != wm->focused) {
        if (candidate->state == STATE_MINIMIZED)
            pwm_restore_window(wm, candidate);
        pwm_focus_window(wm, candidate);
    }
}

void pwm_alttab_next(PocketWM *wm) { pwm_alttab_start(wm); }
void pwm_alttab_commit(PocketWM *wm) { /* no-op for now */ }
void pwm_alttab_cancel(PocketWM *wm) { /* no-op for now */ }
