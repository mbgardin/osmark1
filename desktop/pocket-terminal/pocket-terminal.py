#!/usr/bin/env python3
"""
pocket-terminal — Pocket OS Terminal Emulator
A lightweight VTE-based terminal emulator for Pocket OS.

Copyright (c) 2026 Monte Gardiner — MIT License
"""

import gi
gi.require_version('Gtk', '3.0')
gi.require_version('Vte', '2.91')
from gi.repository import Gtk, Vte, GLib

import os

POCKET_CSS = b"""
* { font-family: "Liberation Sans", sans-serif; font-size: 11px; }

#terminal-header {
    background: linear-gradient(to right, #2B5DCC, #0A246A);
    color: white;
    padding: 6px;
    font-weight: bold;
}
"""

class PocketTerminal(Gtk.Window):
    def __init__(self):
        super().__init__(title="Pocket Terminal")
        self.set_default_size(700, 450)
        self.connect("destroy", Gtk.main_quit)

        self._apply_css()
        self._build_ui()

    def _apply_css(self):
        provider = Gtk.CssProvider()
        provider.load_from_data(POCKET_CSS)
        Gtk.StyleContext.add_provider_for_screen(
            Gdk.Screen.get_default(), provider,
            Gtk.STYLE_PROVIDER_PRIORITY_APPLICATION
        )

    def _build_ui(self):
        vbox = Gtk.Box(orientation=Gtk.Orientation.VERTICAL)
        self.add(vbox)

        # Header
        header = Gtk.Box()
        header.set_name("terminal-header")
        lbl = Gtk.Label(label=">_ Pocket Terminal")
        lbl.set_xalign(0)
        header.pack_start(lbl, False, False, 4)
        vbox.pack_start(header, False, False, 0)

        # Terminal
        self.terminal = Vte.Terminal()
        
        # Configure terminal
        self.terminal.set_font_scale(1.1)
        self.terminal.set_colors(
            fg=Gdk.RGBA(0.9, 0.9, 0.9, 1),
            bg=Gdk.RGBA(0, 0, 0, 1),
            palette=[],
            palette_size=0
        )
        self.terminal.connect("child-exited", lambda w, s: Gtk.main_quit())
        
        # Spawn shell
        shell = os.environ.get("SHELL", "/bin/bash")
        self.terminal.spawn_sync(
            Vte.PtyFlags.DEFAULT,
            os.environ.get("HOME", "/"),
            [shell],
            [],
            GLib.SpawnFlags.SEARCH_PATH,
            None,
            None
        )

        scroll = Gtk.ScrolledWindow()
        scroll.add(self.terminal)
        vbox.pack_start(scroll, True, True, 0)


if __name__ == "__main__":
    from gi.repository import Gdk
    app = PocketTerminal()
    app.show_all()
    Gtk.main()
