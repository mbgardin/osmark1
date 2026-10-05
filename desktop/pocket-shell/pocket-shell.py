#!/usr/bin/env python3
"""
pocket-shell — Pocket OS Desktop Shell
Provides the desktop, wallpaper, taskbar, Start menu, and system tray.

This is the Phase 3-6 prototype using Python + GTK3.
It will be rewritten in C++ for the production release.

Copyright (c) 2026 Monte Gardiner — MIT License
Built on Debian/Linux, GTK3, Python GObject introspection.
"""

import gi
gi.require_version('Gtk', '3.0')
gi.require_version('Gdk', '3.0')
gi.require_version('GdkPixbuf', '2.0')

from gi.repository import Gtk, Gdk, GdkPixbuf, GLib, Gio

import os
import sys
import subprocess
import signal
import time
from pathlib import Path

# ── Pocket OS Color Palette (XP-inspired) ────────────────────────────────────

POCKET_BLUE_DARK    = "#0A246A"   # Task bar, title bars
POCKET_BLUE_MID     = "#2B5DCC"   # Active elements
POCKET_BLUE_LIGHT   = "#3D6BCA"   # Highlights
POCKET_GREEN_START  = "#3A7A34"   # Start button
POCKET_GREEN_HOVER  = "#4A9644"   # Start button hover
POCKET_TRAY_BG      = "#0A246A"   # Tray / clock area
POCKET_TASKBAR_BG   = "#245EDC"   # Main taskbar gradient
POCKET_TEXT_WHITE   = "#FFFFFF"
POCKET_TEXT_LIGHT   = "#D8E4F8"

# ── Paths ─────────────────────────────────────────────────────────────────────

POCKET_SHARE      = Path("/usr/share/pocket")
WALLPAPER_PATH    = os.environ.get(
    "POCKET_WALLPAPER",
    str(POCKET_SHARE / "wallpapers" / "pocket-default.png")
)
ICON_PATH         = POCKET_SHARE / "icons"
POCKET_LOGO       = POCKET_SHARE / "icons" / "pocket-logo.svg"


# ── CSS Theme ─────────────────────────────────────────────────────────────────

POCKET_CSS = b"""
/* Pocket OS GTK Theme — XP-Inspired */

* {
    font-family: "Liberation Sans", "DejaVu Sans", sans-serif;
    font-size: 11px;
}

/* ── Taskbar ── */
#pocket-taskbar {
    background: linear-gradient(to bottom, #3A7ED8, #1E4FBF);
    border-top: 1px solid #6890E8;
    padding: 2px 4px;
    min-height: 36px;
}

/* ── Start Button ── */
#pocket-start-btn {
    background: linear-gradient(to bottom, #5CA644, #2E7A1E);
    color: #FFFFFF;
    font-weight: bold;
    font-size: 12px;
    border-radius: 10px 10px 0 0;
    border: 1px solid #1A5210;
    padding: 4px 14px 4px 10px;
    margin: 0;
    min-height: 28px;
    text-shadow: 0 1px 1px rgba(0,0,0,0.5);
}

#pocket-start-btn:hover {
    background: linear-gradient(to bottom, #6EC456, #3A9028);
}

#pocket-start-btn:active {
    background: linear-gradient(to bottom, #2E7A1E, #5CA644);
}

/* ── Task buttons ── */
#pocket-task-btn {
    background: linear-gradient(to bottom, #4A7ED8, #2656B8);
    color: #FFFFFF;
    border: 1px solid #1A3E98;
    border-radius: 2px;
    padding: 2px 8px;
    margin: 2px 1px;
    min-width: 80px;
    max-width: 140px;
    font-size: 11px;
}

#pocket-task-btn:hover {
    background: linear-gradient(to bottom, #5A90E8, #3266C8);
}

#pocket-task-btn.active-task {
    background: linear-gradient(to bottom, #2656B8, #4A7ED8);
    border-color: #0A246A;
}

/* ── System tray / clock area ── */
#pocket-tray {
    background: linear-gradient(to bottom, #1A3E98, #0A246A);
    border-left: 1px solid #4A70C8;
    padding: 2px 6px;
    border-radius: 0 0 0 2px;
}

#pocket-clock-label {
    color: #FFFFFF;
    font-size: 11px;
    font-weight: bold;
}

/* ── Start Menu ── */
#pocket-start-menu {
    background: #FFFFFF;
    border: 1px solid #0A246A;
    border-radius: 4px 4px 0 0;
}

#pocket-start-menu-header {
    background: linear-gradient(to right, #2B5DCC, #0A246A);
    padding: 8px 12px;
    border-radius: 3px 3px 0 0;
}

#pocket-start-menu-header-label {
    color: #FFFFFF;
    font-size: 18px;
    font-style: italic;
    font-weight: bold;
    text-shadow: 1px 1px 2px rgba(0,0,0,0.6);
}

#pocket-start-menu-item {
    padding: 5px 10px;
    border-radius: 3px;
    color: #000000;
}

#pocket-start-menu-item:hover {
    background: #2B5DCC;
    color: #FFFFFF;
}

/* ── Desktop ── */
#pocket-desktop {
    background: #3C7EC0;
}

/* ── Notifications ── */
.pocket-notification {
    background: #FFFFCC;
    border: 1px solid #CCCC88;
    border-radius: 3px;
    padding: 4px 8px;
    font-size: 11px;
}
"""

# ── Start Menu Items ───────────────────────────────────────────────────────────

START_MENU_ITEMS = [
    # (label, icon_name, command)
    ("Pocket Terminal", "utilities-terminal", "pocket-terminal || x-terminal-emulator || xterm"),
    ("Pocket Explorer", "system-file-manager", "pocket-explorer || nautilus || thunar"),
    ("Pocket Notepad",  "text-editor",         "pocket-notepad || gedit || mousepad"),
    ("Pocket Settings", "preferences-system",  "pocket-settings || gnome-control-center"),
    ("Pocket Task Manager", "utilities-system-monitor", "pocket-taskmgr || gnome-system-monitor"),
    ("---", None, None),   # separator
    ("Web Browser",     "web-browser",         "firefox-esr || firefox || chromium"),
    ("---", None, None),
    ("Log Out",         "system-log-out",      "pocket-session-logout"),
    ("Restart",         "system-restart",      "systemctl reboot"),
    ("Shut Down",       "system-shutdown",     "systemctl poweroff"),
]


class PocketTaskbar(Gtk.Window):
    """
    Pocket OS taskbar — docked to the bottom of the screen.
    Provides: Start button, task buttons, system tray, clock.
    """

    def __init__(self, screen_width, screen_height):
        super().__init__()
        self.screen_width  = screen_width
        self.screen_height = screen_height
        self.start_menu    = None
        self.start_menu_visible = False

        self._setup_window()
        self._setup_ui()
        self._setup_clock()
        self._set_strut()

    def _setup_window(self):
        self.set_name("pocket-taskbar-window")
        self.set_type_hint(Gdk.WindowTypeHint.DOCK)
        self.set_skip_taskbar_hint(True)
        self.set_skip_pager_hint(True)
        self.set_keep_above(True)
        self.set_decorated(False)
        self.set_resizable(False)
        self.stick()

        TASKBAR_HEIGHT = 36
        self.move(0, self.screen_height - TASKBAR_HEIGHT)
        self.resize(self.screen_width, TASKBAR_HEIGHT)

    def _setup_ui(self):
        self.main_box = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=0)
        self.main_box.set_name("pocket-taskbar")
        self.add(self.main_box)

        # ── Start Button
        self.start_btn = Gtk.Button(label="⊞ Start")
        self.start_btn.set_name("pocket-start-btn")
        self.start_btn.connect("clicked", self._on_start_clicked)
        self.main_box.pack_start(self.start_btn, False, False, 2)

        # ── Separator
        sep = Gtk.Separator(orientation=Gtk.Orientation.VERTICAL)
        self.main_box.pack_start(sep, False, False, 3)

        # ── Task area (dynamic window buttons)
        self.task_box = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=1)
        self.main_box.pack_start(self.task_box, True, True, 0)

        # ── Tray area
        self.tray_box = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=4)
        self.tray_box.set_name("pocket-tray")

        # Clock
        self.clock_label = Gtk.Label(label="00:00")
        self.clock_label.set_name("pocket-clock-label")
        self.tray_box.pack_end(self.clock_label, False, False, 4)

        self.main_box.pack_end(self.tray_box, False, False, 0)

    def _setup_clock(self):
        self._update_clock()
        GLib.timeout_add_seconds(1, self._update_clock)

    def _update_clock(self):
        import datetime
        now = datetime.datetime.now()
        self.clock_label.set_text(now.strftime("%I:%M %p\n%m/%d/%Y"))
        return True  # Keep running

    def _set_strut(self):
        """Reserve space at bottom of screen so WM doesn't place windows under taskbar."""
        def do_strut():
            win = self.get_window()
            if not win:
                return True
            TASKBAR_HEIGHT = 36
            # _NET_WM_STRUT: left, right, top, bottom
            win.property_change(
                Gdk.Atom.intern("_NET_WM_STRUT", False),
                Gdk.Atom.intern("CARDINAL", False),
                32,
                Gdk.PropMode.REPLACE,
                [0, 0, 0, TASKBAR_HEIGHT]
            )
            win.property_change(
                Gdk.Atom.intern("_NET_WM_STRUT_PARTIAL", False),
                Gdk.Atom.intern("CARDINAL", False),
                32,
                Gdk.PropMode.REPLACE,
                [
                    0, 0, 0, TASKBAR_HEIGHT,     # left, right, top, bottom
                    0, 0,                          # left_start, left_end
                    0, 0,                          # right_start, right_end
                    0, 0,                          # top_start, top_end
                    0, self.screen_width - 1       # bottom_start, bottom_end
                ]
            )
            return False
        GLib.idle_add(do_strut)

    def _on_start_clicked(self, btn):
        if self.start_menu_visible:
            self._hide_start_menu()
        else:
            self._show_start_menu()

    def _show_start_menu(self):
        if not self.start_menu:
            self.start_menu = PocketStartMenu(self)
        self.start_menu.show_all()
        self.start_menu_visible = True

    def _hide_start_menu(self):
        if self.start_menu:
            self.start_menu.hide()
        self.start_menu_visible = False

    def add_task_button(self, title, on_click):
        """Add a window task button to the taskbar."""
        btn = Gtk.Button(label=title[:20] + ("…" if len(title) > 20 else ""))
        btn.set_name("pocket-task-btn")
        btn.connect("clicked", on_click)
        self.task_box.pack_start(btn, False, False, 1)
        self.task_box.show_all()
        return btn


class PocketStartMenu(Gtk.Window):
    """
    Pocket OS Start Menu — pops up above the taskbar Start button.
    """

    def __init__(self, taskbar):
        super().__init__()
        self.taskbar = taskbar

        self.set_name("pocket-start-menu")
        self.set_type_hint(Gdk.WindowTypeHint.MENU)
        self.set_decorated(False)
        self.set_skip_taskbar_hint(True)
        self.set_skip_pager_hint(True)
        self.set_keep_above(True)

        # Position above start button
        MENU_WIDTH  = 220
        MENU_HEIGHT = 400
        TASKBAR_HEIGHT = 36
        self.move(0, taskbar.screen_height - TASKBAR_HEIGHT - MENU_HEIGHT)
        self.resize(MENU_WIDTH, MENU_HEIGHT)

        self.set_events(Gdk.EventMask.FOCUS_CHANGE_MASK)
        self.connect("focus-out-event", self._on_focus_out)

        self._build_ui()

    def _build_ui(self):
        vbox = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=0)
        self.add(vbox)

        # Header with user name
        header = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL)
        header.set_name("pocket-start-menu-header")
        header.set_size_request(-1, 54)

        header_label = Gtk.Label(label="Monte Gardiner")
        header_label.set_name("pocket-start-menu-header-label")
        header.pack_start(header_label, False, False, 8)
        vbox.pack_start(header, False, False, 0)

        # Menu items
        scroll = Gtk.ScrolledWindow()
        scroll.set_policy(Gtk.PolicyType.NEVER, Gtk.PolicyType.AUTOMATIC)

        items_box = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=0)
        scroll.add(items_box)

        for label, icon_name, cmd in START_MENU_ITEMS:
            if label == "---":
                sep = Gtk.Separator(orientation=Gtk.Orientation.HORIZONTAL)
                items_box.pack_start(sep, False, False, 2)
                continue

            row = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=8)
            row.set_name("pocket-start-menu-item")

            # Icon
            if icon_name:
                icon = Gtk.Image.new_from_icon_name(icon_name, Gtk.IconSize.MENU)
                row.pack_start(icon, False, False, 4)

            # Label
            lbl = Gtk.Label(label=label)
            lbl.set_xalign(0)
            row.pack_start(lbl, True, True, 0)

            btn = Gtk.Button()
            btn.set_name("pocket-start-menu-item")
            btn.set_relief(Gtk.ReliefStyle.NONE)
            btn.add(row)
            btn.connect("clicked", self._on_item_clicked, cmd)
            items_box.pack_start(btn, False, False, 0)

        vbox.pack_start(scroll, True, True, 0)

    def _on_item_clicked(self, btn, cmd):
        self.hide()
        self.taskbar.start_menu_visible = False
        if cmd:
            try:
                subprocess.Popen(
                    cmd, shell=True,
                    stdout=subprocess.DEVNULL,
                    stderr=subprocess.DEVNULL
                )
            except Exception as e:
                print(f"[pocket-shell] Failed to launch: {cmd}: {e}")

    def _on_focus_out(self, widget, event):
        self.hide()
        self.taskbar.start_menu_visible = False
        return False


class PocketDesktop(Gtk.Window):
    """
    Pocket OS Desktop — full-screen desktop with wallpaper and icons.
    """

    def __init__(self, screen_width, screen_height):
        super().__init__()
        self.screen_width  = screen_width
        self.screen_height = screen_height

        self.set_name("pocket-desktop")
        self.set_type_hint(Gdk.WindowTypeHint.DESKTOP)
        self.set_decorated(False)
        self.set_skip_taskbar_hint(True)
        self.set_skip_pager_hint(True)

        self.move(0, 0)
        self.resize(screen_width, screen_height)

        self._setup_wallpaper()
        self._setup_desktop_icons()

        self.connect("button-press-event", self._on_right_click)
        self.add_events(Gdk.EventMask.BUTTON_PRESS_MASK)

    def _setup_wallpaper(self):
        """Draw the Pocket OS wallpaper."""
        self.drawing_area = Gtk.DrawingArea()
        self.drawing_area.connect("draw", self._draw_wallpaper)
        self.add(self.drawing_area)
        self.wallpaper_pixbuf = None

        # Try to load wallpaper file
        if os.path.exists(WALLPAPER_PATH):
            try:
                pb = GdkPixbuf.Pixbuf.new_from_file_at_scale(
                    WALLPAPER_PATH,
                    self.screen_width,
                    self.screen_height,
                    False
                )
                self.wallpaper_pixbuf = pb
            except Exception as e:
                print(f"[pocket-shell] Wallpaper load error: {e}")

    def _draw_wallpaper(self, widget, cr):
        if self.wallpaper_pixbuf:
            Gdk.cairo_set_source_pixbuf(cr, self.wallpaper_pixbuf, 0, 0)
            cr.paint()
        else:
            # Fallback: draw a Pocket OS gradient background
            self._draw_gradient_bg(cr)

    def _draw_gradient_bg(self, cr):
        """Draw an XP-inspired gradient desktop background."""
        import cairo
        w = self.screen_width
        h = self.screen_height

        # Sky gradient: dark teal-blue to lighter blue
        grad = cairo.LinearGradient(0, 0, 0, h)
        grad.add_color_stop_rgb(0.0,  0.05, 0.18, 0.45)  # dark navy
        grad.add_color_stop_rgb(0.45, 0.15, 0.45, 0.65)  # medium blue
        grad.add_color_stop_rgb(0.75, 0.22, 0.55, 0.40)  # greenish
        grad.add_color_stop_rgb(1.0,  0.18, 0.48, 0.32)  # green-teal

        cr.set_source(grad)
        cr.rectangle(0, 0, w, h)
        cr.fill()

        # Draw "Pocket OS" watermark text (bottom-right, faint)
        cr.set_source_rgba(1, 1, 1, 0.08)
        cr.select_font_face("Liberation Sans",
            cairo.FONT_SLANT_ITALIC, cairo.FONT_WEIGHT_BOLD)
        cr.set_font_size(48)
        cr.move_to(w - 380, h - 60)
        cr.show_text("Pocket OS")

    def _setup_desktop_icons(self):
        """Add desktop shortcut icons in an overlay."""
        # Desktop icons are overlaid on the drawing area
        # For Phase 6 we'll implement a proper icon grid.
        # For now: the desktop is functional as a drawing surface.
        pass

    def _on_right_click(self, widget, event):
        if event.button == 3:
            # Right-click context menu (Phase 6)
            pass


class PocketShell:
    """
    Pocket OS Shell — orchestrates desktop + taskbar.
    """

    def __init__(self):
        self._apply_css()

        screen = Gdk.Screen.get_default()
        display = Gdk.Display.get_default()

        # Use primary monitor dimensions
        monitor = display.get_primary_monitor()
        if monitor:
            geo = monitor.get_geometry()
            w, h = geo.width, geo.height
        else:
            w = screen.get_width()
            h = screen.get_height()

        print(f"[pocket-shell] Screen: {w}x{h}")
        print(f"[pocket-shell] Starting Pocket OS Shell...")

        # Desktop (lowest layer)
        self.desktop = PocketDesktop(w, h)
        self.desktop.show_all()

        # Taskbar (top layer, docked)
        self.taskbar = PocketTaskbar(w, h)
        self.taskbar.show_all()

        # Signal handling
        signal.signal(signal.SIGTERM, self._on_signal)
        signal.signal(signal.SIGINT,  self._on_signal)

        print("[pocket-shell] Pocket OS Shell ready.")

    def _apply_css(self):
        """Apply the Pocket OS CSS theme to the GTK provider."""
        css_provider = Gtk.CssProvider()
        css_provider.load_from_data(POCKET_CSS)
        Gtk.StyleContext.add_provider_for_screen(
            Gdk.Screen.get_default(),
            css_provider,
            Gtk.STYLE_PROVIDER_PRIORITY_APPLICATION
        )

    def _on_signal(self, signum, frame):
        print(f"\n[pocket-shell] Received signal {signum}. Exiting.")
        Gtk.main_quit()

    def run(self):
        Gtk.main()


def main():
    print("=" * 50)
    print("  Pocket OS Desktop Shell")
    print("  Version 0.1 (Alpha)")
    print("  Created by Monte Gardiner")
    print("=" * 50)

    shell = PocketShell()
    shell.run()


if __name__ == "__main__":
    main()
