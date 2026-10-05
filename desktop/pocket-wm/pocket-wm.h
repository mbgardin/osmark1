/*
 * pocket-wm.h — Pocket OS Window Manager — Data Structures & Declarations
 *
 * Copyright (c) 2026 Monte Gardiner — MIT License
 */

#ifndef POCKET_WM_H
#define POCKET_WM_H

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/keysym.h>
#include <X11/XKBlib.h>
#include <X11/extensions/Xrandr.h>
#include <X11/extensions/shape.h>

/* ── Version ───────────────────────────────────────────────────────────────── */
#define POCKET_WM_VERSION "0.1.0"
#define POCKET_WM_NAME    "pocket-wm"

/* ── Layout constants ──────────────────────────────────────────────────────── */

/* Title bar height (pixels) */
#define TITLE_BAR_HEIGHT   25

/* Border width (pixels) */
#define BORDER_WIDTH       3

/* Window control button size */
#define BUTTON_SIZE        18
#define BUTTON_MARGIN      4

/* Button positions within title bar (right-aligned) */
#define BTN_CLOSE_X_OFFSET  (BUTTON_SIZE + BUTTON_MARGIN)
#define BTN_MAX_X_OFFSET    (2 * (BUTTON_SIZE + BUTTON_MARGIN))
#define BTN_MIN_X_OFFSET    (3 * (BUTTON_SIZE + BUTTON_MARGIN))

/* Minimum window dimensions */
#define WIN_MIN_WIDTH   100
#define WIN_MIN_HEIGHT  50

/* Snap-to-edge threshold */
#define SNAP_THRESHOLD  8

/* Double-click time (ms) */
#define DBLCLICK_TIME   400

/* ── XP-inspired color palette ─────────────────────────────────────────────── */

/* Active title bar: XP-style deep blue gradient (approximated as flat for now) */
#define COLOR_TITLEBAR_ACTIVE_TOP    0x0A246A   /* Deep navy */
#define COLOR_TITLEBAR_ACTIVE_BTN    0x2B5DCC   /* Mid blue */
#define COLOR_TITLEBAR_ACTIVE_BOT    0x3D6BCA   /* Lighter blue */

/* Inactive title bar */
#define COLOR_TITLEBAR_INACTIVE      0x7A96DF   /* Desaturated blue */
#define COLOR_TITLEBAR_INACTIVE_TXT  0xD8E4F8

/* Title text */
#define COLOR_TITLE_TEXT_ACTIVE      0xFFFFFF   /* White */
#define COLOR_TITLE_TEXT_INACTIVE    0xC0CDF5

/* Window border */
#define COLOR_BORDER_ACTIVE          0x0A246A
#define COLOR_BORDER_INACTIVE        0x7A96DF

/* Close button: XP-style red */
#define COLOR_BTN_CLOSE_HOVER        0xE81123
#define COLOR_BTN_CLOSE_NORMAL       0x4A7EBD

/* Maximize / Minimize buttons */
#define COLOR_BTN_NORMAL             0x4A7EBD
#define COLOR_BTN_HOVER              0x5B9FE4

/* Desktop background fallback */
#define COLOR_DESKTOP_BG             0x236B8E   /* Teal-ish blue */

/* ── Drag/resize state ──────────────────────────────────────────────────────── */

typedef enum {
    DRAG_NONE = 0,
    DRAG_MOVE,
    DRAG_RESIZE_N,
    DRAG_RESIZE_S,
    DRAG_RESIZE_E,
    DRAG_RESIZE_W,
    DRAG_RESIZE_NE,
    DRAG_RESIZE_NW,
    DRAG_RESIZE_SE,
    DRAG_RESIZE_SW
} DragMode;

/* ── Window state ────────────────────────────────────────────────────────────── */

typedef enum {
    STATE_NORMAL = 0,
    STATE_MINIMIZED,
    STATE_MAXIMIZED,
    STATE_FULLSCREEN
} WindowState;

/* ── Button hit areas ───────────────────────────────────────────────────────── */

typedef enum {
    HIT_NONE = 0,
    HIT_TITLE,
    HIT_CLOSE,
    HIT_MAXIMIZE,
    HIT_MINIMIZE,
    HIT_BORDER_N,
    HIT_BORDER_S,
    HIT_BORDER_E,
    HIT_BORDER_W,
    HIT_BORDER_NE,
    HIT_BORDER_NW,
    HIT_BORDER_SE,
    HIT_BORDER_SW
} HitArea;

/* ── Atoms ────────────────────────────────────────────────────────────────────── */

typedef struct {
    /* ICCCM */
    Atom WM_DELETE_WINDOW;
    Atom WM_PROTOCOLS;
    Atom WM_STATE;
    Atom WM_TAKE_FOCUS;
    Atom WM_CHANGE_STATE;
    Atom WM_NAME;
    Atom WM_ICON_NAME;
    Atom WM_CLASS;
    Atom WM_HINTS;
    Atom WM_NORMAL_HINTS;
    Atom WM_TRANSIENT_FOR;
    Atom WM_CLIENT_MACHINE;

    /* EWMH */
    Atom _NET_SUPPORTED;
    Atom _NET_CLIENT_LIST;
    Atom _NET_NUMBER_OF_DESKTOPS;
    Atom _NET_DESKTOP_NAMES;
    Atom _NET_CURRENT_DESKTOP;
    Atom _NET_ACTIVE_WINDOW;
    Atom _NET_WM_NAME;
    Atom _NET_WM_VISIBLE_NAME;
    Atom _NET_WM_ICON_NAME;
    Atom _NET_WM_DESKTOP;
    Atom _NET_WM_WINDOW_TYPE;
    Atom _NET_WM_WINDOW_TYPE_DESKTOP;
    Atom _NET_WM_WINDOW_TYPE_DOCK;
    Atom _NET_WM_WINDOW_TYPE_TOOLBAR;
    Atom _NET_WM_WINDOW_TYPE_MENU;
    Atom _NET_WM_WINDOW_TYPE_UTILITY;
    Atom _NET_WM_WINDOW_TYPE_SPLASH;
    Atom _NET_WM_WINDOW_TYPE_DIALOG;
    Atom _NET_WM_WINDOW_TYPE_NORMAL;
    Atom _NET_WM_STATE;
    Atom _NET_WM_STATE_MODAL;
    Atom _NET_WM_STATE_MAXIMIZED_VERT;
    Atom _NET_WM_STATE_MAXIMIZED_HORZ;
    Atom _NET_WM_STATE_FULLSCREEN;
    Atom _NET_WM_STATE_HIDDEN;
    Atom _NET_WM_STATE_ABOVE;
    Atom _NET_WM_STATE_BELOW;
    Atom _NET_WM_STRUT;
    Atom _NET_WM_STRUT_PARTIAL;
    Atom _NET_WM_PID;
    Atom _NET_WM_ICON;
    Atom _NET_CLOSE_WINDOW;
    Atom _NET_MOVERESIZE_WINDOW;
    Atom _NET_WM_MOVERESIZE;
    Atom _NET_WORKAREA;
    Atom _NET_SUPPORTING_WM_CHECK;
    Atom _NET_FRAME_EXTENTS;
    Atom _MOTIF_WM_HINTS;
    Atom UTF8_STRING;
} PocketAtoms;

/* ── Key binding ─────────────────────────────────────────────────────────────── */

typedef struct {
    unsigned int modifiers;
    KeyCode      keycode;
    void       (*action)(void *wm, void *data);
    void        *data;
} KeyBinding;

/* ── Strut (taskbar/dock reservations) ──────────────────────────────────────── */

typedef struct {
    int left, right, top, bottom;
} Strut;

/* ── PocketWindow — represents one managed client window ─────────────────── */

typedef struct PocketWindow {
    Window     client;    /* The actual application window */
    Window     frame;     /* Our decoration frame */
    Window     titlebar;  /* Title bar sub-window */
    Window     btn_close;
    Window     btn_max;
    Window     btn_min;
    
    /* Geometry (frame position) */
    int        x, y;
    int        width, height;  /* client area dimensions */
    
    /* Pre-maximize saved geometry */
    int        saved_x, saved_y;
    int        saved_width, saved_height;
    
    /* State */
    WindowState state;
    int         focused;
    int         decorated;  /* 0 for docks, splash, etc. */
    
    /* Window type */
    Atom        type;
    
    /* Title */
    char        title[256];
    
    /* ICCCM hints */
    XSizeHints  size_hints;
    XWMHints   *wm_hints;
    int         delete_supported;  /* WM_DELETE_WINDOW */
    int         take_focus;        /* WM_TAKE_FOCUS */
    
    /* Drag/resize tracking */
    DragMode    drag_mode;
    int         drag_start_x, drag_start_y;   /* pointer position */
    int         drag_start_fx, drag_start_fy; /* frame origin */
    int         drag_start_fw, drag_start_fh; /* frame size */
    
    /* Button hover state */
    int         btn_close_hover;
    int         btn_max_hover;
    int         btn_min_hover;
    
    /* Linked list */
    struct PocketWindow *next;
    struct PocketWindow *prev;
} PocketWindow;

/* ── PocketWM — global WM state ─────────────────────────────────────────── */

typedef struct {
    Display    *dpy;
    int         screen;
    Window      root;
    int         sw, sh;    /* screen width / height */
    
    /* Work area (excluding taskbar struts) */
    int         wa_x, wa_y, wa_width, wa_height;
    
    /* Window list */
    PocketWindow *windows;
    PocketWindow *focused;
    int          n_windows;
    
    /* EWMH support window */
    Window       ewmh_check_win;
    
    /* Atoms */
    PocketAtoms  atoms;
    
    /* Graphics contexts */
    GC           gc_title_active;
    GC           gc_title_inactive;
    GC           gc_border;
    GC           gc_button;
    GC           gc_text_active;
    GC           gc_text_inactive;
    GC           gc_close_hover;
    GC           gc_btn_hover;
    
    /* Colors */
    unsigned long col_title_active;
    unsigned long col_title_inactive;
    unsigned long col_border_active;
    unsigned long col_border_inactive;
    unsigned long col_title_text_active;
    unsigned long col_title_text_inactive;
    unsigned long col_btn_close_hover;
    unsigned long col_btn_normal;
    unsigned long col_btn_hover;
    
    /* Font */
    XFontStruct *font;
    int          font_ascent;
    
    /* Key bindings */
    KeyBinding  *keybindings;
    int          n_keybindings;
    
    /* Alt+Tab cycling */
    int          alttab_active;
    PocketWindow *alttab_candidate;
    
    /* Strut tracking (taskbar reservations) */
    Strut        strut;
    
    /* RandR event base */
    int          randr_event_base;
    int          randr_error_base;
    int          have_randr;
    
    /* Last button press time (for double-click) */
    Time         last_click_time;
    Window       last_click_win;
} PocketWM;

/* ── Function declarations ────────────────────────────────────────────────── */

/* Initialization */
PocketWM   *pwm_init(void);
void        pwm_cleanup(PocketWM *wm);
void        pwm_run(PocketWM *wm);
void        pwm_init_atoms(PocketWM *wm);
void        pwm_init_ewmh(PocketWM *wm);
void        pwm_init_colors(PocketWM *wm);
void        pwm_init_fonts(PocketWM *wm);
void        pwm_init_keybindings(PocketWM *wm);
void        pwm_scan_existing_windows(PocketWM *wm);

/* Window management */
PocketWindow *pwm_find_by_frame(PocketWM *wm, Window frame);
PocketWindow *pwm_find_by_client(PocketWM *wm, Window client);
PocketWindow *pwm_find_by_any(PocketWM *wm, Window win);
void          pwm_frame_window(PocketWM *wm, Window client, int was_viewable);
void          pwm_unframe_window(PocketWM *wm, PocketWindow *pw);
void          pwm_raise_window(PocketWM *wm, PocketWindow *pw);
void          pwm_focus_window(PocketWM *wm, PocketWindow *pw);
void          pwm_unfocus_window(PocketWM *wm, PocketWindow *pw);
void          pwm_minimize_window(PocketWM *wm, PocketWindow *pw);
void          pwm_maximize_window(PocketWM *wm, PocketWindow *pw);
void          pwm_restore_window(PocketWM *wm, PocketWindow *pw);
void          pwm_close_window(PocketWM *wm, PocketWindow *pw);
void          pwm_move_window(PocketWM *wm, PocketWindow *pw, int x, int y);
void          pwm_resize_window(PocketWM *wm, PocketWindow *pw, int w, int h);
void          pwm_update_title(PocketWM *wm, PocketWindow *pw);
void          pwm_draw_frame(PocketWM *wm, PocketWindow *pw);
void          pwm_draw_titlebar(PocketWM *wm, PocketWindow *pw);
void          pwm_draw_button(PocketWM *wm, PocketWindow *pw, int btn, int hover);
void          pwm_update_ewmh_client_list(PocketWM *wm);
void          pwm_update_net_active_window(PocketWM *wm);
void          pwm_update_struts(PocketWM *wm);
void          pwm_update_workarea(PocketWM *wm);
HitArea       pwm_hittest(PocketWM *wm, PocketWindow *pw, int x, int y);
unsigned long pwm_get_color(PocketWM *wm, unsigned long rgb);

/* Event handlers */
void pwm_handle_map_request(PocketWM *wm, XMapRequestEvent *ev);
void pwm_handle_unmap_notify(PocketWM *wm, XUnmapEvent *ev);
void pwm_handle_destroy_notify(PocketWM *wm, XDestroyWindowEvent *ev);
void pwm_handle_configure_request(PocketWM *wm, XConfigureRequestEvent *ev);
void pwm_handle_enter_notify(PocketWM *wm, XCrossingEvent *ev);
void pwm_handle_button_press(PocketWM *wm, XButtonEvent *ev);
void pwm_handle_button_release(PocketWM *wm, XButtonEvent *ev);
void pwm_handle_motion_notify(PocketWM *wm, XMotionEvent *ev);
void pwm_handle_key_press(PocketWM *wm, XKeyEvent *ev);
void pwm_handle_property_notify(PocketWM *wm, XPropertyEvent *ev);
void pwm_handle_client_message(PocketWM *wm, XClientMessageEvent *ev);
void pwm_handle_expose(PocketWM *wm, XExposeEvent *ev);
void pwm_handle_extension_event(PocketWM *wm, XEvent *ev);

/* Alt+Tab */
void pwm_alttab_start(PocketWM *wm);
void pwm_alttab_next(PocketWM *wm);
void pwm_alttab_commit(PocketWM *wm);
void pwm_alttab_cancel(PocketWM *wm);

/* Utilities */
void pwm_send_client_message(PocketWM *wm, Window win, Atom proto);
char *pwm_get_window_title(PocketWM *wm, Window win);
int  pwm_window_should_decorate(PocketWM *wm, Window win);
void pwm_spawn(const char *cmd);

#endif /* POCKET_WM_H */
