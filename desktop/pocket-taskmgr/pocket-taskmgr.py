#!/usr/bin/env python3
"""
pocket-taskmgr — Pocket OS Task Manager
XP-inspired process/resource viewer.

Copyright (c) 2026 Monte Gardiner — MIT License
"""

import gi
gi.require_version('Gtk', '3.0')
gi.require_version('Gdk', '3.0')
from gi.repository import Gtk, Gdk, GLib

import os
import glob
import subprocess
import signal
import time

POCKET_CSS = b"""
* { font-family: "Liberation Sans", "DejaVu Sans", sans-serif; font-size: 11px; }
#pocket-taskmgr-header {
    background: linear-gradient(to right, #2B5DCC, #0A246A);
    color: white; padding: 6px 12px;
}
#kill-btn { background: #CC2B2B; color: white; border-radius: 3px; }
#kill-btn:hover { background: #E83333; }
"""

REFRESH_INTERVAL_MS = 2000  # 2 seconds


def read_proc_stat():
    """Read /proc/stat for CPU usage calculation."""
    try:
        with open("/proc/stat") as f:
            line = f.readline()
        parts = line.split()
        # user, nice, system, idle, iowait, irq, softirq
        total  = sum(int(x) for x in parts[1:8])
        idle   = int(parts[4])
        return total, idle
    except Exception:
        return 0, 0


def get_memory_info():
    """Return (total_kb, available_kb) from /proc/meminfo."""
    try:
        mem = {}
        with open("/proc/meminfo") as f:
            for line in f:
                parts = line.split()
                if len(parts) >= 2:
                    mem[parts[0].rstrip(":")] = int(parts[1])
        total = mem.get("MemTotal", 0)
        avail = mem.get("MemAvailable", mem.get("MemFree", 0))
        return total, avail
    except Exception:
        return 0, 0


def get_processes():
    """Return list of (pid, name, user, state, cpu_pct, mem_kb) for all processes."""
    procs = []
    try:
        for piddir in glob.glob("/proc/[0-9]*/"):
            pid = int(os.path.basename(piddir.rstrip("/")))
            try:
                # Process name
                with open(f"/proc/{pid}/comm") as f:
                    name = f.read().strip()

                # Status (user, state, VmRSS)
                user = "?"
                state = "?"
                mem_kb = 0
                with open(f"/proc/{pid}/status") as f:
                    for line in f:
                        if line.startswith("Uid:"):
                            uid = int(line.split()[1])
                            try:
                                import pwd
                                user = pwd.getpwuid(uid).pw_name
                            except Exception:
                                user = str(uid)
                        elif line.startswith("State:"):
                            state = line.split()[1]
                        elif line.startswith("VmRSS:"):
                            mem_kb = int(line.split()[1])

                procs.append((pid, name, user, state, 0.0, mem_kb))
            except (IOError, OSError, ValueError):
                pass
    except Exception:
        pass
    return sorted(procs, key=lambda x: x[5], reverse=True)


class PocketTaskManager(Gtk.Window):
    """Pocket OS Task Manager window."""

    def __init__(self):
        super().__init__(title="Pocket Task Manager")
        self.set_default_size(640, 480)
        self.connect("destroy", Gtk.main_quit)

        self._prev_stat = read_proc_stat()
        self._cpu_pct = 0.0

        self._apply_css()
        self._build_ui()
        self._refresh()
        GLib.timeout_add(REFRESH_INTERVAL_MS, self._refresh)

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
        header = Gtk.Box()
        header.set_name("pocket-taskmgr-header")
        header.set_size_request(-1, 40)
        hdr_lbl = Gtk.Label()
        hdr_lbl.set_markup('<span color="white" font="13" weight="bold">📊  Pocket Task Manager</span>')
        header.pack_start(hdr_lbl, False, False, 8)
        vbox.pack_start(header, False, False, 0)

        # Stats bar
        stats_box = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=20)
        stats_box.set_margin_top(6)
        stats_box.set_margin_start(12)
        stats_box.set_margin_bottom(4)

        self.cpu_label = Gtk.Label(label="CPU: 0%")
        self.mem_label = Gtk.Label(label="Memory: 0 MB / 0 MB")
        self.proc_count_label = Gtk.Label(label="Processes: 0")

        for lbl in [self.cpu_label, self.mem_label, self.proc_count_label]:
            lbl.set_markup(f'<b>{lbl.get_text()}</b>')
            stats_box.pack_start(lbl, False, False, 0)

        vbox.pack_start(stats_box, False, False, 0)
        sep = Gtk.Separator()
        vbox.pack_start(sep, False, False, 0)

        # Process list
        scroll = Gtk.ScrolledWindow()
        scroll.set_policy(Gtk.PolicyType.AUTOMATIC, Gtk.PolicyType.AUTOMATIC)

        # Columns: PID, Name, User, State, Memory (KB)
        self.store = Gtk.ListStore(int, str, str, str, float, int)
        self.tree = Gtk.TreeView(model=self.store)
        self.tree.set_enable_search(True)
        self.tree.get_selection().set_mode(Gtk.SelectionMode.SINGLE)

        cols = [
            ("PID",    0, 60,  False),
            ("Name",   1, 180, True),
            ("User",   2, 80,  False),
            ("State",  3, 50,  False),
            ("Mem KB", 5, 80,  False),
        ]
        for title, col_idx, width, expand in cols:
            renderer = Gtk.CellRendererText()
            col = Gtk.TreeViewColumn(title, renderer, text=col_idx)
            col.set_resizable(True)
            col.set_min_width(width)
            col.set_sort_column_id(col_idx)
            if expand:
                col.set_expand(True)
            self.tree.append_column(col)

        scroll.add(self.tree)
        vbox.pack_start(scroll, True, True, 0)

        # Bottom buttons
        btn_box = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=8)
        btn_box.set_margin_top(6)
        btn_box.set_margin_bottom(6)
        btn_box.set_margin_start(8)

        kill_btn = Gtk.Button(label="⛔ End Process")
        kill_btn.set_name("kill-btn")
        kill_btn.connect("clicked", self._on_kill_process)
        btn_box.pack_start(kill_btn, False, False, 0)

        refresh_btn = Gtk.Button(label="🔄 Refresh")
        refresh_btn.connect("clicked", lambda b: self._refresh())
        btn_box.pack_start(refresh_btn, False, False, 0)

        vbox.pack_start(btn_box, False, False, 0)

    def _refresh(self):
        """Update CPU, memory stats and process list."""
        # CPU
        total, idle = read_proc_stat()
        prev_total, prev_idle = self._prev_stat
        delta_total = total - prev_total
        delta_idle  = idle - prev_idle
        if delta_total > 0:
            self._cpu_pct = 100.0 * (1.0 - delta_idle / delta_total)
        self._prev_stat = (total, idle)

        # Memory
        mem_total, mem_avail = get_memory_info()
        mem_used = (mem_total - mem_avail) // 1024  # MB
        mem_tot_mb = mem_total // 1024

        # Update labels
        self.cpu_label.set_markup(f'<b>CPU: {self._cpu_pct:.1f}%</b>')
        self.mem_label.set_markup(
            f'<b>Memory: {mem_used} MB / {mem_tot_mb} MB</b>')

        # Processes
        procs = get_processes()
        self.proc_count_label.set_markup(f'<b>Processes: {len(procs)}</b>')

        self.store.clear()
        for pid, name, user, state, cpu_p, mem_kb in procs:
            self.store.append([pid, name, user, state, cpu_p, mem_kb])

        return True  # Keep timeout running

    def _on_kill_process(self, btn):
        sel = self.tree.get_selection()
        model, it = sel.get_selected()
        if not it:
            return
        pid = model[it][0]
        name = model[it][1]

        dialog = Gtk.MessageDialog(
            transient_for=self,
            flags=Gtk.DialogFlags.MODAL,
            message_type=Gtk.MessageType.WARNING,
            buttons=Gtk.ButtonsType.OK_CANCEL,
            text=f"End process '{name}' (PID {pid})?"
        )
        dialog.format_secondary_text(
            "The process will be terminated immediately.\n"
            "Any unsaved work will be lost."
        )
        response = dialog.run()
        dialog.destroy()

        if response == Gtk.ResponseType.OK:
            try:
                os.kill(pid, signal.SIGTERM)
            except (ProcessLookupError, PermissionError) as e:
                err = Gtk.MessageDialog(
                    transient_for=self,
                    flags=Gtk.DialogFlags.MODAL,
                    message_type=Gtk.MessageType.ERROR,
                    buttons=Gtk.ButtonsType.OK,
                    text=f"Could not terminate PID {pid}: {e}"
                )
                err.run()
                err.destroy()
            GLib.timeout_add(500, self._refresh)


def main():
    win = PocketTaskManager()
    win.show_all()
    Gtk.main()


if __name__ == "__main__":
    main()
