/*
 * draw.c — Pocket WM Drawing / Rendering
 * XP-inspired window decorations using Xlib drawing primitives.
 * Copyright (c) 2026 Monte Gardiner — MIT License
 */

#include "pocket-wm.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* ── Color allocation ─────────────────────────────────────────────────────── */

unsigned long pwm_get_color(PocketWM *wm, unsigned long rgb) {
    XColor col;
    col.red   = ((rgb >> 16) & 0xFF) * 257;
    col.green = ((rgb >>  8) & 0xFF) * 257;
    col.blue  = ((rgb >>  0) & 0xFF) * 257;
    col.flags = DoRed | DoGreen | DoBlue;
    XAllocColor(wm->dpy, DefaultColormap(wm->dpy, wm->screen), &col);
    return col.pixel;
}

void pwm_init_colors(PocketWM *wm) {
    wm->col_title_active       = pwm_get_color(wm, COLOR_TITLEBAR_ACTIVE_BTN);
    wm->col_title_inactive     = pwm_get_color(wm, COLOR_TITLEBAR_INACTIVE);
    wm->col_border_active      = pwm_get_color(wm, COLOR_BORDER_ACTIVE);
    wm->col_border_inactive    = pwm_get_color(wm, COLOR_BORDER_INACTIVE);
    wm->col_title_text_active  = pwm_get_color(wm, COLOR_TITLE_TEXT_ACTIVE);
    wm->col_title_text_inactive= pwm_get_color(wm, COLOR_TITLE_TEXT_INACTIVE);
    wm->col_btn_close_hover    = pwm_get_color(wm, COLOR_BTN_CLOSE_HOVER);
    wm->col_btn_normal         = pwm_get_color(wm, COLOR_BTN_NORMAL);
    wm->col_btn_hover          = pwm_get_color(wm, COLOR_BTN_HOVER);
}

void pwm_init_fonts(PocketWM *wm) {
    /* Try several font names in preference order */
    const char *font_names[] = {
        "-misc-liberation sans-medium-r-normal--12-*-*-*-p-*-iso8859-1",
        "-*-dejavu sans-medium-r-*-*-12-*-*-*-p-*-iso8859-1",
        "-*-helvetica-medium-r-*-*-12-*-*-*-p-*-iso8859-1",
        "-*-lucida-medium-r-*-*-12-*-*-*-p-*-iso8859-1",
        "fixed",
        NULL
    };

    for (int i = 0; font_names[i]; i++) {
        wm->font = XLoadQueryFont(wm->dpy, font_names[i]);
        if (wm->font) {
            wm->font_ascent = wm->font->ascent;
            printf("[pocket-wm] Font: %s\n", font_names[i]);
            break;
        }
    }

    if (!wm->font) {
        fprintf(stderr, "[pocket-wm] Warning: no suitable font found\n");
        wm->font_ascent = 10;
    }

    /* Create GCs */
    XGCValues gcv;

    /* Active title bar */
    gcv.foreground = wm->col_title_active;
    gcv.fill_style = FillSolid;
    wm->gc_title_active = XCreateGC(wm->dpy, wm->root,
        GCForeground | GCFillStyle, &gcv);

    /* Inactive title bar */
    gcv.foreground = wm->col_title_inactive;
    wm->gc_title_inactive = XCreateGC(wm->dpy, wm->root,
        GCForeground | GCFillStyle, &gcv);

    /* Border */
    gcv.foreground = wm->col_border_active;
    wm->gc_border = XCreateGC(wm->dpy, wm->root,
        GCForeground | GCFillStyle, &gcv);

    /* Buttons */
    gcv.foreground = wm->col_btn_normal;
    wm->gc_button = XCreateGC(wm->dpy, wm->root,
        GCForeground | GCFillStyle, &gcv);

    /* Active title text */
    gcv.foreground = wm->col_title_text_active;
    if (wm->font) gcv.font = wm->font->fid;
    wm->gc_text_active = XCreateGC(wm->dpy, wm->root,
        GCForeground | GCFillStyle | (wm->font ? GCFont : 0), &gcv);

    /* Inactive title text */
    gcv.foreground = wm->col_title_text_inactive;
    wm->gc_text_inactive = XCreateGC(wm->dpy, wm->root,
        GCForeground | GCFillStyle | (wm->font ? GCFont : 0), &gcv);

    /* Close hover */
    gcv.foreground = wm->col_btn_close_hover;
    wm->gc_close_hover = XCreateGC(wm->dpy, wm->root,
        GCForeground | GCFillStyle, &gcv);

    /* Button hover */
    gcv.foreground = wm->col_btn_hover;
    wm->gc_btn_hover = XCreateGC(wm->dpy, wm->root,
        GCForeground | GCFillStyle, &gcv);
}

/* ── Title bar rendering ──────────────────────────────────────────────────── */

/*
 * Draw the XP-inspired title bar gradient effect using horizontal bands.
 * Xlib doesn't do gradients natively; we approximate with a few fill bands.
 */
static void draw_titlebar_gradient(PocketWM *wm, Window win, int w, int h, int active) {
    /* Three-stop gradient approximation */
    struct { unsigned long rgb; int y_pct; } stops[] = {
        { active ? 0x1C4FBB : 0x9EB2D8, 0   },
        { active ? 0x2B5DCC : 0x7A96DF, 40  },
        { active ? 0x1840A8 : 0x6888CC, 80  },
        { active ? 0x122E80 : 0x5A78BC, 100 },
    };
    int n = 4;

    for (int i = 0; i < n - 1; i++) {
        int y1 = (stops[i].y_pct * h) / 100;
        int y2 = (stops[i+1].y_pct * h) / 100;
        int band_h = y2 - y1;
        if (band_h <= 0) continue;

        /* Interpolate color between stops */
        unsigned long r1 = (stops[i].rgb >> 16) & 0xFF;
        unsigned long g1 = (stops[i].rgb >>  8) & 0xFF;
        unsigned long b1 = (stops[i].rgb >>  0) & 0xFF;
        unsigned long r2 = (stops[i+1].rgb >> 16) & 0xFF;
        unsigned long g2 = (stops[i+1].rgb >>  8) & 0xFF;
        unsigned long b2 = (stops[i+1].rgb >>  0) & 0xFF;

        for (int y = y1; y < y2; y++) {
            int t = band_h > 0 ? (y - y1) * 256 / band_h : 0;
            unsigned long r = r1 + (r2 - r1) * t / 256;
            unsigned long g = g1 + (g2 - g1) * t / 256;
            unsigned long b = b1 + (b2 - b1) * t / 256;
            unsigned long col = pwm_get_color(wm, (r << 16) | (g << 8) | b);

            XGCValues gcv;
            gcv.foreground = col;
            XChangeGC(wm->dpy, wm->gc_title_active, GCForeground, &gcv);
            XFillRectangle(wm->dpy, win, wm->gc_title_active, 0, y, w, 1);
        }
    }
}

void pwm_draw_titlebar(PocketWM *wm, PocketWindow *pw) {
    if (!pw || !pw->titlebar) return;

    int tw = pw->width + 2 * BORDER_WIDTH;
    int th = TITLE_BAR_HEIGHT;
    int active = pw->focused;

    /* Gradient background */
    draw_titlebar_gradient(wm, pw->titlebar, tw, th, active);

    /* Top highlight line (XP shine effect) */
    {
        unsigned long highlight = pwm_get_color(wm,
            active ? 0x6890E8 : 0xB0C4E8);
        XGCValues gcv;
        gcv.foreground = highlight;
        XChangeGC(wm->dpy, wm->gc_title_active, GCForeground, &gcv);
        XFillRectangle(wm->dpy, pw->titlebar, wm->gc_title_active, 0, 0, tw, 1);
    }

    /* Window title text */
    if (pw->title[0]) {
        GC gc_text = active ? wm->gc_text_active : wm->gc_text_inactive;
        int text_y = (th + wm->font_ascent) / 2 - 1;
        int text_x = 8;  /* Left margin */

        /* Shadow for depth */
        {
            unsigned long shadow = pwm_get_color(wm, active ? 0x0A1E60 : 0x4A6090);
            XGCValues gcv;
            gcv.foreground = shadow;
            if (wm->font) gcv.font = wm->font->fid;
            XChangeGC(wm->dpy, gc_text, GCForeground | (wm->font ? GCFont : 0), &gcv);
            XDrawString(wm->dpy, pw->titlebar, gc_text,
                text_x + 1, text_y + 1,
                pw->title, strlen(pw->title));
        }

        /* Main text */
        {
            unsigned long col = active
                ? wm->col_title_text_active
                : wm->col_title_text_inactive;
            XGCValues gcv;
            gcv.foreground = col;
            if (wm->font) gcv.font = wm->font->fid;
            XChangeGC(wm->dpy, gc_text, GCForeground | (wm->font ? GCFont : 0), &gcv);
            XDrawString(wm->dpy, pw->titlebar, gc_text,
                text_x, text_y,
                pw->title, strlen(pw->title));
        }
    }

    /* Draw window control buttons */
    pwm_draw_button(wm, pw, 0, pw->btn_close_hover);  /* Close */
    pwm_draw_button(wm, pw, 1, pw->btn_max_hover);    /* Maximize */
    pwm_draw_button(wm, pw, 2, pw->btn_min_hover);    /* Minimize */
}

/*
 * Draw a window control button.
 * btn: 0=close, 1=maximize, 2=minimize
 */
void pwm_draw_button(PocketWM *wm, PocketWindow *pw, int btn, int hover) {
    if (!pw || !pw->titlebar) return;

    int tw = pw->width + 2 * BORDER_WIDTH;
    int bsize = BUTTON_SIZE;
    int bmargin = BUTTON_MARGIN;
    int vtop = (TITLE_BAR_HEIGHT - bsize) / 2;

    int bx;
    switch (btn) {
        case 0: bx = tw - BTN_CLOSE_X_OFFSET - bmargin; break;  /* close */
        case 1: bx = tw - BTN_MAX_X_OFFSET   - bmargin; break;  /* max */
        case 2: bx = tw - BTN_MIN_X_OFFSET   - bmargin; break;  /* min */
        default: return;
    }

    /* Button background */
    GC gc = (btn == 0 && hover) ? wm->gc_close_hover :
            (hover)              ? wm->gc_btn_hover   :
                                   wm->gc_button;
    
    unsigned long bg_col = (btn == 0 && hover) ? wm->col_btn_close_hover :
                           hover                ? wm->col_btn_hover       :
                                                  wm->col_btn_normal;
    XGCValues gcv;
    gcv.foreground = bg_col;
    XChangeGC(wm->dpy, gc, GCForeground, &gcv);
    XFillRectangle(wm->dpy, pw->titlebar, gc, bx, vtop, bsize, bsize);

    /* Button border (rounded effect approximation) */
    unsigned long border_col = pwm_get_color(wm, 0x1E3A8A);
    gcv.foreground = border_col;
    XChangeGC(wm->dpy, gc, GCForeground, &gcv);
    XDrawRectangle(wm->dpy, pw->titlebar, gc, bx, vtop, bsize - 1, bsize - 1);

    /* Button icon */
    unsigned long icon_col = pwm_get_color(wm, 0xFFFFFF);
    gcv.foreground = icon_col;
    XChangeGC(wm->dpy, gc, GCForeground, &gcv);

    int cx = bx + bsize / 2;  (void)cx;
    int cy = vtop + bsize / 2; (void)cy;

    switch (btn) {
        case 0: /* Close — X mark */
            XDrawLine(wm->dpy, pw->titlebar, gc,
                bx+3, vtop+3, bx+bsize-4, vtop+bsize-4);
            XDrawLine(wm->dpy, pw->titlebar, gc,
                bx+bsize-4, vtop+3, bx+3, vtop+bsize-4);
            /* Thicker X */
            XDrawLine(wm->dpy, pw->titlebar, gc,
                bx+4, vtop+3, bx+bsize-3, vtop+bsize-4);
            XDrawLine(wm->dpy, pw->titlebar, gc,
                bx+bsize-3, vtop+3, bx+4, vtop+bsize-4);
            break;
        case 1: /* Maximize — square */
            XDrawRectangle(wm->dpy, pw->titlebar, gc,
                bx+3, vtop+4, bsize-7, bsize-8);
            XDrawLine(wm->dpy, pw->titlebar, gc,
                bx+3, vtop+5, bx+bsize-4, vtop+5);  /* title bar on square */
            break;
        case 2: /* Minimize — line */
            XFillRectangle(wm->dpy, pw->titlebar, gc,
                bx+3, vtop+bsize-6, bsize-6, 2);
            break;
    }
}

void pwm_draw_frame(PocketWM *wm, PocketWindow *pw) {
    if (!pw || !pw->frame) return;
    
    /* Border color based on focus */
    unsigned long border_col = pw->focused
        ? wm->col_border_active
        : wm->col_border_inactive;
    
    XSetWindowBorder(wm->dpy, pw->frame, border_col);
    
    /* Redraw title bar */
    pwm_draw_titlebar(wm, pw);
}
