#!/usr/bin/env python3
"""
pocket-explorer — Pocket OS File Explorer
XP-inspired file manager with a sidebar.

Copyright (c) 2026 Monte Gardiner — MIT License
"""

import gi
gi.require_version('Gtk', '3.0')
gi.require_version('Gdk', '3.0')
from gi.repository import Gtk, Gdk, Gio, GLib

import os
import subprocess
import urllib.parse

POCKET_CSS = b"""
* { font-family: "Liberation Sans", sans-serif; font-size: 11px; }

#explorer-sidebar {
    background-color: #7BA2E7;
    background-image: linear-gradient(to bottom, #7BA2E7, #6385CE);
    color: white;
}

#explorer-sidebar-header {
    font-weight: bold;
    color: #1E395B;
    background-color: #FFFFFF;
    border-radius: 4px 4px 0 0;
    padding: 4px 8px;
    margin: 8px 8px 0 8px;
}

#explorer-sidebar-content {
    background-color: #D6E4FB;
    border-radius: 0 0 4px 4px;
    padding: 8px;
    margin: 0 8px 8px 8px;
}

.sidebar-link {
    color: #0A246A;
    text-decoration: underline;
    background: none;
    border: none;
    padding: 2px 4px;
}
.sidebar-link:hover {
    color: #2B5DCC;
}

#address-bar {
    background: #ECE9D8;
    border-bottom: 1px solid #ACA899;
    padding: 4px;
}
"""

class PocketExplorer(Gtk.Window):
    def __init__(self, start_dir=None):
        super().__init__(title="Pocket Explorer")
        self.set_default_size(800, 500)
        self.connect("destroy", Gtk.main_quit)

        if not start_dir:
            start_dir = os.path.expanduser("~")
        self.current_dir = os.path.abspath(start_dir)

        self._apply_css()
        self._build_ui()
        self._load_directory(self.current_dir)

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

        # Menu bar
        menubar = Gtk.MenuBar()
        file_menu = Gtk.Menu()
        file_item = Gtk.MenuItem(label="File")
        file_item.set_submenu(file_menu)
        exit_item = Gtk.MenuItem(label="Close")
        exit_item.connect("activate", lambda w: Gtk.main_quit())
        file_menu.append(exit_item)
        menubar.append(file_item)
        vbox.pack_start(menubar, False, False, 0)

        # Toolbar
        toolbar = Gtk.Toolbar()
        toolbar.set_style(Gtk.ToolbarStyle.BOTH)
        
        btn_back = Gtk.ToolButton(icon_name="go-previous", label="Back")
        btn_back.connect("clicked", self._on_back)
        toolbar.insert(btn_back, -1)
        
        btn_up = Gtk.ToolButton(icon_name="go-up", label="Up")
        btn_up.connect("clicked", self._on_up)
        toolbar.insert(btn_up, -1)

        vbox.pack_start(toolbar, False, False, 0)

        # Address bar
        addr_box = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=6)
        addr_box.set_name("address-bar")
        
        lbl_addr = Gtk.Label(label="Address:")
        addr_box.pack_start(lbl_addr, False, False, 4)
        
        self.entry_addr = Gtk.Entry()
        self.entry_addr.connect("activate", self._on_address_entered)
        addr_box.pack_start(self.entry_addr, True, True, 0)
        
        btn_go = Gtk.Button(label="Go")
        btn_go.connect("clicked", self._on_address_entered)
        addr_box.pack_start(btn_go, False, False, 0)

        vbox.pack_start(addr_box, False, False, 0)

        # Main content area
        hpaned = Gtk.Paned(orientation=Gtk.Orientation.HORIZONTAL)
        hpaned.set_position(200)
        vbox.pack_start(hpaned, True, True, 0)

        # Sidebar
        sidebar_scroll = Gtk.ScrolledWindow()
        sidebar_scroll.set_policy(Gtk.PolicyType.NEVER, Gtk.PolicyType.AUTOMATIC)
        
        sidebar_vbox = Gtk.Box(orientation=Gtk.Orientation.VERTICAL)
        sidebar_vbox.set_name("explorer-sidebar")
        
        # System Tasks
        sys_lbl = Gtk.Label(label="System Tasks")
        sys_lbl.set_name("explorer-sidebar-header")
        sys_lbl.set_xalign(0)
        sidebar_vbox.pack_start(sys_lbl, False, False, 0)
        
        sys_box = Gtk.Box(orientation=Gtk.Orientation.VERTICAL)
        sys_box.set_name("explorer-sidebar-content")
        
        btn_sys = Gtk.Button(label="View system information")
        btn_sys.get_style_context().add_class("sidebar-link")
        btn_sys.set_relief(Gtk.ReliefStyle.NONE)
        btn_sys.set_xalign(0)
        btn_sys.connect("clicked", lambda w: subprocess.Popen(["pocket-settings"]))
        sys_box.pack_start(btn_sys, False, False, 2)
        
        sidebar_vbox.pack_start(sys_box, False, False, 0)
        
        # Places
        places_lbl = Gtk.Label(label="Other Places")
        places_lbl.set_name("explorer-sidebar-header")
        places_lbl.set_xalign(0)
        sidebar_vbox.pack_start(places_lbl, False, False, 0)
        
        places_box = Gtk.Box(orientation=Gtk.Orientation.VERTICAL)
        places_box.set_name("explorer-sidebar-content")
        
        for name, path in [
            ("Home", os.path.expanduser("~")),
            ("Desktop", os.path.expanduser("~/Desktop")),
            ("File System", "/")
        ]:
            btn = Gtk.Button(label=name)
            btn.get_style_context().add_class("sidebar-link")
            btn.set_relief(Gtk.ReliefStyle.NONE)
            btn.set_xalign(0)
            btn.connect("clicked", lambda w, p=path: self._load_directory(p))
            places_box.pack_start(btn, False, False, 2)

        sidebar_vbox.pack_start(places_box, False, False, 0)
        
        sidebar_scroll.add(sidebar_vbox)
        hpaned.add1(sidebar_scroll)

        # File View
        file_scroll = Gtk.ScrolledWindow()
        file_scroll.set_policy(Gtk.PolicyType.AUTOMATIC, Gtk.PolicyType.AUTOMATIC)
        
        self.iconview = Gtk.IconView()
        self.liststore = Gtk.ListStore(str, GdkPixbuf.Pixbuf, str, str)
        # 0: name, 1: pixbuf, 2: full path, 3: is_dir ("1" or "0")
        
        self.iconview.set_model(self.liststore)
        self.iconview.set_text_column(0)
        self.iconview.set_pixbuf_column(1)
        self.iconview.set_item_width(80)
        self.iconview.connect("item-activated", self._on_item_activated)
        
        file_scroll.add(self.iconview)
        hpaned.add2(file_scroll)
        
        # Statusbar
        self.statusbar = Gtk.Statusbar()
        self.statusbar_id = self.statusbar.get_context_id("pocket-explorer")
        vbox.pack_start(self.statusbar, False, False, 0)

    def _load_directory(self, path):
        try:
            items = os.listdir(path)
        except PermissionError:
            self._show_error("Permission denied")
            return
        except FileNotFoundError:
            self._show_error("Directory not found")
            return

        self.current_dir = os.path.abspath(path)
        self.entry_addr.set_text(self.current_dir)
        self.set_title(f"{os.path.basename(self.current_dir) or '/'} — Pocket Explorer")
        self.liststore.clear()

        # Get icon theme
        icon_theme = Gtk.IconTheme.get_default()
        folder_pixbuf = icon_theme.load_icon("folder", 48, 0)
        file_pixbuf = icon_theme.load_icon("text-x-generic", 48, 0)
        
        dirs = []
        files = []
        
        for item in items:
            if item.startswith("."):
                continue # hidden
            full_path = os.path.join(self.current_dir, item)
            if os.path.isdir(full_path):
                dirs.append((item, folder_pixbuf, full_path, "1"))
            else:
                files.append((item, file_pixbuf, full_path, "0"))
                
        dirs.sort(key=lambda x: x[0].lower())
        files.sort(key=lambda x: x[0].lower())
        
        for d in dirs:
            self.liststore.append(d)
        for f in files:
            self.liststore.append(f)
            
        self.statusbar.pop(self.statusbar_id)
        self.statusbar.push(self.statusbar_id, f"{len(dirs) + len(files)} objects")

    def _on_address_entered(self, widget):
        path = self.entry_addr.get_text()
        self._load_directory(path)

    def _on_back(self, widget):
        pass # To be implemented in a real version with history

    def _on_up(self, widget):
        parent = os.path.dirname(self.current_dir)
        if parent:
            self._load_directory(parent)

    def _on_item_activated(self, widget, path):
        it = self.liststore.get_iter(path)
        full_path = self.liststore.get_value(it, 2)
        is_dir = self.liststore.get_value(it, 3)
        
        if is_dir == "1":
            self._load_directory(full_path)
        else:
            # Open file
            subprocess.Popen(["xdg-open", full_path])

    def _show_error(self, message):
        dialog = Gtk.MessageDialog(
            transient_for=self,
            flags=0,
            message_type=Gtk.MessageType.ERROR,
            buttons=Gtk.ButtonsType.OK,
            text=message
        )
        dialog.run()
        dialog.destroy()

if __name__ == "__main__":
    import sys
    start_dir = sys.argv[1] if len(sys.argv) > 1 else None
    app = PocketExplorer(start_dir)
    app.show_all()
    Gtk.main()
