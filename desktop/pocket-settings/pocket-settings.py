#!/usr/bin/env python3
"""
pocket-settings — Pocket OS Control Panel
XP-inspired settings interface for Pocket OS.

Copyright (c) 2026 Monte Gardiner — MIT License
"""

import gi
gi.require_version('Gtk', '3.0')
gi.require_version('Gdk', '3.0')
from gi.repository import Gtk, Gdk, GLib

import subprocess
import os
import socket
import platform

POCKET_CSS = b"""
* { font-family: "Liberation Sans", sans-serif; font-size: 11px; }

#pocket-settings-header {
    background: linear-gradient(to right, #2B5DCC, #0A246A);
    padding: 12px;
    color: white;
}

#settings-category-btn {
    border-radius: 4px;
    padding: 8px;
    margin: 2px;
    border: 1px solid transparent;
}

#settings-category-btn:hover {
    background: #E8F0FF;
    border-color: #2B5DCC;
}

#settings-category-btn:checked {
    background: #2B5DCC;
    color: white;
    border-color: #0A246A;
}
"""


class PocketSettingsAbout(Gtk.Box):
    """About Pocket OS page — system information."""

    def __init__(self):
        super().__init__(orientation=Gtk.Orientation.VERTICAL, spacing=12)
        self.set_margin_top(20)
        self.set_margin_start(20)
        self.set_margin_end(20)

        # Logo + title
        title = Gtk.Label()
        title.set_markup('<span font="20" weight="bold">Pocket OS</span>')
        self.pack_start(title, False, False, 0)

        subtitle = Gtk.Label()
        subtitle.set_markup('<span font="12" style="italic">Version 0.1 (Alpha)</span>')
        self.pack_start(subtitle, False, False, 0)

        creator = Gtk.Label()
        creator.set_markup('<span>Created by <b>Monte Gardiner</b></span>')
        self.pack_start(creator, False, False, 4)

        sep = Gtk.Separator()
        self.pack_start(sep, False, False, 8)

        # System info
        grid = Gtk.Grid()
        grid.set_column_spacing(16)
        grid.set_row_spacing(6)
        self.pack_start(grid, False, False, 0)

        def add_row(grid, row, label, value):
            lbl = Gtk.Label(label=label + ":")
            lbl.set_xalign(1)
            lbl.set_markup(f'<b>{label}:</b>')
            val = Gtk.Label(label=value)
            val.set_xalign(0)
            val.set_selectable(True)
            grid.attach(lbl, 0, row, 1, 1)
            grid.attach(val, 1, row, 1, 1)

        # Kernel
        try:
            kernel = platform.release()
        except Exception:
            kernel = "unknown"

        # Hostname
        try:
            hostname = socket.gethostname()
        except Exception:
            hostname = "pocketos"

        # CPU info
        try:
            with open("/proc/cpuinfo") as f:
                for line in f:
                    if "model name" in line:
                        cpu = line.split(":", 1)[1].strip()
                        break
            else:
                cpu = platform.processor() or "x86-64"
        except Exception:
            cpu = platform.machine()

        # Memory
        try:
            with open("/proc/meminfo") as f:
                for line in f:
                    if line.startswith("MemTotal:"):
                        mem_kb = int(line.split()[1])
                        mem_gb = mem_kb / (1024 * 1024)
                        mem_str = f"{mem_gb:.1f} GB"
                        break
        except Exception:
            mem_str = "unknown"

        add_row(grid, 0, "Operating System", "Pocket OS 0.1 (Alpha)")
        add_row(grid, 1, "Creator",          "Monte Gardiner")
        add_row(grid, 2, "Linux Kernel",     kernel)
        add_row(grid, 3, "Architecture",     "x86-64")
        add_row(grid, 4, "Hostname",         hostname)
        add_row(grid, 5, "Processor",        cpu)
        add_row(grid, 6, "Memory",           mem_str)
        add_row(grid, 7, "Base System",      "Debian GNU/Linux Bookworm")

        sep2 = Gtk.Separator()
        self.pack_start(sep2, False, False, 8)

        note = Gtk.Label()
        note.set_markup(
            '<span size="small" color="gray">'
            'Built on Debian GNU/Linux. Linux Kernel and Debian components\n'
            'are the work of their respective contributors.\n'
            'Pocket OS desktop, branding, and integration © 2026 Monte Gardiner.'
            '</span>'
        )
        note.set_xalign(0)
        self.pack_start(note, False, False, 0)


class PocketSettingsAppearance(Gtk.Box):
    """Appearance settings page."""

    def __init__(self):
        super().__init__(orientation=Gtk.Orientation.VERTICAL, spacing=12)
        self.set_margin_top(20)
        self.set_margin_start(20)

        title = Gtk.Label()
        title.set_markup('<b>Appearance</b>')
        title.set_xalign(0)
        self.pack_start(title, False, False, 0)

        sep = Gtk.Separator()
        self.pack_start(sep, False, False, 4)

        # Wallpaper section
        wp_label = Gtk.Label(label="Wallpaper")
        wp_label.set_xalign(0)
        wp_label.set_markup('<b>Desktop Wallpaper</b>')
        self.pack_start(wp_label, False, False, 4)

        wp_box = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=8)
        wp_entry = Gtk.Entry()
        wp_entry.set_text(os.environ.get(
            "POCKET_WALLPAPER",
            "/usr/share/pocket/wallpapers/pocket-default.jpg"
        ))
        wp_entry.set_hexpand(True)
        wp_box.pack_start(wp_entry, True, True, 0)

        wp_btn = Gtk.Button(label="Browse...")
        wp_btn.connect("clicked", self._on_browse_wallpaper, wp_entry)
        wp_box.pack_start(wp_btn, False, False, 0)
        self.pack_start(wp_box, False, False, 0)

        apply_btn = Gtk.Button(label="Apply Wallpaper")
        apply_btn.connect("clicked", self._on_apply_wallpaper, wp_entry)
        self.pack_start(apply_btn, False, False, 4)

        # Theme section
        sep2 = Gtk.Separator()
        self.pack_start(sep2, False, False, 8)
        theme_label = Gtk.Label()
        theme_label.set_markup('<b>Theme</b>')
        theme_label.set_xalign(0)
        self.pack_start(theme_label, False, False, 4)

        theme_note = Gtk.Label(label="Pocket XP (built-in) — more themes coming soon.")
        theme_note.set_xalign(0)
        self.pack_start(theme_note, False, False, 0)

    def _on_browse_wallpaper(self, btn, entry):
        dialog = Gtk.FileChooserDialog(
            title="Choose Wallpaper",
            action=Gtk.FileChooserAction.OPEN
        )
        dialog.add_button("Cancel", Gtk.ResponseType.CANCEL)
        dialog.add_button("Open",   Gtk.ResponseType.OK)
        f = Gtk.FileFilter()
        f.set_name("Images")
        f.add_mime_type("image/jpeg")
        f.add_mime_type("image/png")
        f.add_mime_type("image/svg+xml")
        dialog.add_filter(f)

        if dialog.run() == Gtk.ResponseType.OK:
            entry.set_text(dialog.get_filename())
        dialog.destroy()

    def _on_apply_wallpaper(self, btn, entry):
        path = entry.get_text()
        if os.path.exists(path):
            # Set wallpaper via xsetbg or feh
            try:
                subprocess.Popen(["feh", "--bg-fill", path])
            except FileNotFoundError:
                try:
                    subprocess.Popen(["xsetbg", path])
                except FileNotFoundError:
                    pass


class PocketSettings(Gtk.Window):
    """Main Pocket Settings window."""

    CATEGORIES = [
        ("🖥  About",        "about",      PocketSettingsAbout),
        ("🎨  Appearance",   "appearance", PocketSettingsAppearance),
        ("📡  Network",      "network",    None),
        ("🔊  Audio",        "audio",      None),
        ("🖱  Mouse",        "mouse",      None),
        ("⌨  Keyboard",     "keyboard",   None),
        ("🔋  Power",        "power",      None),
        ("🗓  Date & Time",  "datetime",   None),
        ("👤  Users",        "users",      None),
        ("💾  Storage",      "storage",    None),
    ]

    def __init__(self):
        super().__init__(title="Pocket Settings")
        self.set_default_size(700, 480)
        self.set_border_width(0)
        self.connect("destroy", Gtk.main_quit)

        self._apply_css()
        self._build_ui()

    def _apply_css(self):
        provider = Gtk.CssProvider()
        provider.load_from_data(POCKET_CSS)
        Gtk.StyleContext.add_provider_for_screen(
            Gdk.Screen.get_default(),
            provider,
            Gtk.STYLE_PROVIDER_PRIORITY_APPLICATION
        )

    def _build_ui(self):
        vbox = Gtk.Box(orientation=Gtk.Orientation.VERTICAL)
        self.add(vbox)

        # Header
        header = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL)
        header.set_name("pocket-settings-header")
        header_label = Gtk.Label()
        header_label.set_markup('<span color="white" font="14" weight="bold">⚙  Pocket Settings</span>')
        header.pack_start(header_label, False, False, 8)
        header.set_size_request(-1, 48)
        vbox.pack_start(header, False, False, 0)

        # Main content: sidebar + panel
        hbox = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL)
        vbox.pack_start(hbox, True, True, 0)

        # Sidebar
        sidebar = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=0)
        sidebar.set_size_request(160, -1)

        sc = Gtk.ScrolledWindow()
        sc.set_policy(Gtk.PolicyType.NEVER, Gtk.PolicyType.AUTOMATIC)
        sc.add(sidebar)
        hbox.pack_start(sc, False, False, 0)

        sep_vert = Gtk.Separator(orientation=Gtk.Orientation.VERTICAL)
        hbox.pack_start(sep_vert, False, False, 0)

        # Content panel
        self.content_stack = Gtk.Stack()
        self.content_stack.set_transition_type(Gtk.StackTransitionType.CROSSFADE)
        self.content_stack.set_transition_duration(150)
        hbox.pack_start(self.content_stack, True, True, 0)

        # Build sidebar buttons and stack pages
        first_btn = None
        for label, name, PageClass in self.CATEGORIES:
            btn = Gtk.ToggleButton(label=label)
            btn.set_name("settings-category-btn")
            btn.set_xalign(0)
            btn.connect("toggled", self._on_category_toggled, name)
            sidebar.pack_start(btn, False, False, 0)

            if PageClass:
                page = PageClass()
                self.content_stack.add_named(page, name)
            else:
                placeholder = Gtk.Label(label=f"{label} settings coming soon.")
                placeholder.set_margin_top(30)
                self.content_stack.add_named(placeholder, name)

            if first_btn is None:
                first_btn = btn

        if first_btn:
            first_btn.set_active(True)
            self.content_stack.set_visible_child_name("about")

        self.active_btn = first_btn

    def _on_category_toggled(self, btn, name):
        if btn.get_active():
            if self.active_btn and self.active_btn != btn:
                self.active_btn.set_active(False)
            self.active_btn = btn
            self.content_stack.set_visible_child_name(name)
        elif self.active_btn == btn:
            btn.set_active(True)


def main():
    win = PocketSettings()
    win.show_all()
    Gtk.main()


if __name__ == "__main__":
    main()
