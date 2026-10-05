#!/usr/bin/env python3
"""
pocket-notepad — Pocket OS Text Editor
A lightweight XP Notepad-inspired text editor.

Copyright (c) 2026 Monte Gardiner — MIT License
"""

import gi
gi.require_version('Gtk', '3.0')
gi.require_version('Gdk', '3.0')
from gi.repository import Gtk, Gdk, GLib, Pango

import os
import sys

POCKET_CSS = b"""
* { font-family: "Liberation Sans", sans-serif; font-size: 11px; }
#notepad-text {
    font-family: "Liberation Mono", "DejaVu Sans Mono", monospace;
    font-size: 12px;
}
"""


class PocketNotepad(Gtk.Window):
    """Pocket OS Notepad — lightweight text editor."""

    def __init__(self, filename=None):
        super().__init__()
        self.current_file = None
        self.modified = False

        self._apply_css()
        self._build_ui()
        self._update_title()

        if filename:
            self._open_file(filename)

        self.set_default_size(640, 480)
        self.connect("destroy", self._on_quit)
        self.connect("delete-event", self._on_delete_event)

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
        vbox.pack_start(menubar, False, False, 0)

        # File menu
        file_menu_item = Gtk.MenuItem(label="File")
        file_menu = Gtk.Menu()
        file_menu_item.set_submenu(file_menu)

        for label, accel_key, accel_mod, handler in [
            ("New",     Gdk.KEY_n, Gdk.ModifierType.CONTROL_MASK, self._on_new),
            ("Open...", Gdk.KEY_o, Gdk.ModifierType.CONTROL_MASK, self._on_open),
            ("Save",    Gdk.KEY_s, Gdk.ModifierType.CONTROL_MASK, self._on_save),
            ("Save As...", Gdk.KEY_s,
             Gdk.ModifierType.CONTROL_MASK | Gdk.ModifierType.SHIFT_MASK,
             self._on_save_as),
            (None, None, None, None),  # separator
            ("Exit",    Gdk.KEY_q, Gdk.ModifierType.CONTROL_MASK, self._on_quit),
        ]:
            if label is None:
                file_menu.append(Gtk.SeparatorMenuItem())
            else:
                item = Gtk.MenuItem(label=label)
                item.connect("activate", handler)
                file_menu.append(item)
        menubar.append(file_menu_item)

        # Edit menu
        edit_menu_item = Gtk.MenuItem(label="Edit")
        edit_menu = Gtk.Menu()
        edit_menu_item.set_submenu(edit_menu)

        for label, handler in [
            ("Undo",    self._on_undo),
            ("Redo",    self._on_redo),
            (None, None),
            ("Cut",     self._on_cut),
            ("Copy",    self._on_copy),
            ("Paste",   self._on_paste),
            ("Select All", self._on_select_all),
            (None, None),
            ("Find...", self._on_find),
        ]:
            if label is None:
                edit_menu.append(Gtk.SeparatorMenuItem())
            else:
                item = Gtk.MenuItem(label=label)
                item.connect("activate", handler)
                edit_menu.append(item)
        menubar.append(edit_menu_item)

        # Format menu
        fmt_menu_item = Gtk.MenuItem(label="Format")
        fmt_menu = Gtk.Menu()
        fmt_menu_item.set_submenu(fmt_menu)
        font_item = Gtk.MenuItem(label="Font...")
        font_item.connect("activate", self._on_font)
        fmt_menu.append(font_item)
        menubar.append(fmt_menu_item)

        # Text area
        scroll = Gtk.ScrolledWindow()
        scroll.set_policy(Gtk.PolicyType.AUTOMATIC, Gtk.PolicyType.AUTOMATIC)

        self.textview = Gtk.TextView()
        self.textview.set_name("notepad-text")
        self.textview.set_wrap_mode(Gtk.WrapMode.WORD)
        self.textview.set_left_margin(4)
        self.textview.set_right_margin(4)
        self.textview.set_top_margin(4)
        self.textbuffer = self.textview.get_buffer()
        self.textbuffer.connect("changed", self._on_text_changed)

        scroll.add(self.textview)
        vbox.pack_start(scroll, True, True, 0)

        # Status bar
        self.statusbar = Gtk.Statusbar()
        self.statusbar_ctx = self.statusbar.get_context_id("pocket-notepad")
        vbox.pack_start(self.statusbar, False, False, 0)
        self._update_status()

    def _update_title(self):
        name = os.path.basename(self.current_file) if self.current_file else "Untitled"
        mod = " *" if self.modified else ""
        self.set_title(f"{name}{mod} — Pocket Notepad")

    def _update_status(self):
        buf = self.textbuffer
        lines = buf.get_line_count()
        chars = buf.get_char_count()
        it = buf.get_iter_at_mark(buf.get_insert())
        line = it.get_line() + 1
        col  = it.get_line_offset() + 1
        self.statusbar.pop(self.statusbar_ctx)
        self.statusbar.push(self.statusbar_ctx,
            f"Line {line}, Col {col} | Lines: {lines} | Characters: {chars}")

    def _on_text_changed(self, buf):
        self.modified = True
        self._update_title()
        self._update_status()

    def _check_save(self):
        """Ask to save unsaved changes. Returns True if OK to proceed."""
        if not self.modified:
            return True
        dialog = Gtk.MessageDialog(
            transient_for=self,
            flags=Gtk.DialogFlags.MODAL,
            message_type=Gtk.MessageType.QUESTION,
            buttons=Gtk.ButtonsType.NONE,
            text="Save changes?"
        )
        dialog.format_secondary_text(
            "The document has unsaved changes. Save before closing?"
        )
        dialog.add_button("Don't Save", Gtk.ResponseType.NO)
        dialog.add_button("Cancel",     Gtk.ResponseType.CANCEL)
        dialog.add_button("Save",       Gtk.ResponseType.YES)
        resp = dialog.run()
        dialog.destroy()
        if resp == Gtk.ResponseType.YES:
            self._on_save(None)
            return not self.modified
        if resp == Gtk.ResponseType.NO:
            return True
        return False

    def _on_new(self, *args):
        if not self._check_save(): return
        self.textbuffer.set_text("")
        self.current_file = None
        self.modified = False
        self._update_title()

    def _on_open(self, *args):
        if not self._check_save(): return
        dialog = Gtk.FileChooserDialog(
            title="Open File",
            action=Gtk.FileChooserAction.OPEN
        )
        dialog.add_button("Cancel", Gtk.ResponseType.CANCEL)
        dialog.add_button("Open",   Gtk.ResponseType.OK)
        if dialog.run() == Gtk.ResponseType.OK:
            self._open_file(dialog.get_filename())
        dialog.destroy()

    def _open_file(self, path):
        try:
            with open(path, "r", encoding="utf-8", errors="replace") as f:
                content = f.read()
            self.textbuffer.set_text(content)
            self.current_file = path
            self.modified = False
            self._update_title()
        except Exception as e:
            self._show_error(f"Could not open file:\n{e}")

    def _on_save(self, *args):
        if self.current_file:
            self._save_to(self.current_file)
        else:
            self._on_save_as()

    def _on_save_as(self, *args):
        dialog = Gtk.FileChooserDialog(
            title="Save As",
            action=Gtk.FileChooserAction.SAVE
        )
        dialog.set_do_overwrite_confirmation(True)
        dialog.add_button("Cancel", Gtk.ResponseType.CANCEL)
        dialog.add_button("Save",   Gtk.ResponseType.OK)
        if self.current_file:
            dialog.set_filename(self.current_file)
        if dialog.run() == Gtk.ResponseType.OK:
            self._save_to(dialog.get_filename())
        dialog.destroy()

    def _save_to(self, path):
        try:
            start, end = self.textbuffer.get_bounds()
            text = self.textbuffer.get_text(start, end, False)
            with open(path, "w", encoding="utf-8") as f:
                f.write(text)
            self.current_file = path
            self.modified = False
            self._update_title()
        except Exception as e:
            self._show_error(f"Could not save file:\n{e}")

    def _on_undo(self, *args):
        if self.textbuffer.can_undo():
            self.textbuffer.undo()

    def _on_redo(self, *args):
        if self.textbuffer.can_redo():
            self.textbuffer.redo()

    def _on_cut(self, *args):
        self.textview.emit("cut-clipboard")

    def _on_copy(self, *args):
        self.textview.emit("copy-clipboard")

    def _on_paste(self, *args):
        self.textview.emit("paste-clipboard")

    def _on_select_all(self, *args):
        self.textbuffer.select_range(
            self.textbuffer.get_start_iter(),
            self.textbuffer.get_end_iter()
        )

    def _on_find(self, *args):
        dialog = Gtk.Dialog(title="Find", transient_for=self)
        dialog.add_button("Find Next", Gtk.ResponseType.OK)
        dialog.add_button("Close", Gtk.ResponseType.CANCEL)
        box = dialog.get_content_area()
        entry = Gtk.Entry()
        entry.set_placeholder_text("Search text...")
        box.pack_start(entry, False, False, 6)
        box.show_all()

        while dialog.run() == Gtk.ResponseType.OK:
            needle = entry.get_text()
            if needle:
                buf = self.textbuffer
                start = buf.get_iter_at_mark(buf.get_insert())
                found = start.forward_search(needle, 0, None)
                if found:
                    match_start, match_end = found
                    buf.select_range(match_start, match_end)
                    self.textview.scroll_to_iter(match_start, 0, False, 0, 0)
        dialog.destroy()

    def _on_font(self, *args):
        dialog = Gtk.FontChooserDialog(title="Choose Font", transient_for=self)
        if dialog.run() == Gtk.ResponseType.OK:
            font_desc = dialog.get_font_desc()
            self.textview.override_font(font_desc)
        dialog.destroy()

    def _on_delete_event(self, widget, event):
        return not self._check_save()

    def _on_quit(self, *args):
        if self._check_save():
            Gtk.main_quit()

    def _show_error(self, msg):
        dialog = Gtk.MessageDialog(
            transient_for=self,
            flags=Gtk.DialogFlags.MODAL,
            message_type=Gtk.MessageType.ERROR,
            buttons=Gtk.ButtonsType.OK,
            text=msg
        )
        dialog.run()
        dialog.destroy()


def main():
    filename = sys.argv[1] if len(sys.argv) > 1 else None
    win = PocketNotepad(filename)
    win.show_all()
    Gtk.main()


if __name__ == "__main__":
    main()
