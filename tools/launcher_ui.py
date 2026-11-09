"""Launcher home: Play opens the intro, title screen and main menu."""
import argparse
from pathlib import Path
import sys
import queue
import threading

from setup_assets_ui import LauncherCanvas

CANCELLED = 20
UNAVAILABLE_MESSAGE = "The game menu is unavailable. Update or repair the installed game files."

BACKGROUND = "#06101c"
LINE = "#25415a"
MUTED = "#9baec3"
ICE = "#c8edff"
BLUE = "#8bd5fb"


class LauncherSurface(LauncherCanvas):
    """The website's type, palette and authored ring shapes in a native window."""

    def _backdrop(self):
        from tkinter import font

        families = set(font.families(self.window))
        self.family = next((name for name in ('Arial', 'Liberation Sans', 'DejaVu Sans')
                            if name in families), self.family)
        self.window.configure(background=BACKGROUND)
        self.canvas.configure(background=BACKGROUND)
        self._launcher_background()
        self.line(48, 590, 992, 590, fill=LINE)
        self.text(48, 610, 'ENTER  /  PLAY', 9, MUTED)
        self.text(888, 610, 'ESC  /  CLOSE', 9, MUTED)

    def _wordmark(self, x, y):
        # The home places the shared text identity in its main content area.
        return

    def tracked_text(self, x, y, value, size, spacing, fill):
        from tkinter import font

        face = font.Font(self.window, font=self.font(size))
        for character in value:
            self.text(x, y, character, size, fill)
            x += face.measure(character) / self.scale + spacing

    def button(self, label, command, x, y, width, height, primary=False):
        button = super().button(label, command, x, y, width, height, primary)
        background = '#b8e2f7' if primary else '#10273c'
        hover = '#e0f5ff' if primary else '#1c3c56'
        foreground = '#071724' if primary else ICE
        button.configure(background=background, foreground=foreground,
                         activebackground=hover, activeforeground=foreground,
                         disabledforeground='#7892a6', font=self.font(18 if primary else 13),
                         highlightbackground='#527995', highlightcolor=ICE)
        button.bind('<Enter>', lambda event: button.configure(background=hover)
                    if str(button.cget('state')) != 'disabled' else None)
        button.bind('<Leave>', lambda event: button.configure(background=background)
                    if str(button.cget('state')) != 'disabled' else None)
        return button


class LauncherHome(LauncherSurface):
    def __init__(self, window, root, build=None, *, runtime_unavailable=None,
                 launcher_executable=None, runtime_executable=None, frontend_function=None):
        super().__init__(window, root, "Forever Launcher", width=1040, height=640)
        self.runtime_executable = runtime_executable
        self.result, self.closed = CANCELLED, False
        self.repair_dialog = None
        self._readiness_after = None
        from installation_paths import resource_root
        self.precompiled = (resource_root(root) != Path(root).resolve() or
                            (Path(root) / "PRECOMPILED_TEST_BUILD.json").is_file())

        def validated_frontend():
            from launcher_health import inspect_launcher
            return inspect_launcher(root, self.runtime_executable)

        self.frontend_check = frontend_function or validated_frontend

        self.frontend = False
        self.tracked_text(58, 196, 'FOREVER', 34, 9, ICE)
        self.tracked_text(61, 248, 'LAUNCHER', 12, 5, MUTED)
        self.play_button = self.button("Play", self.play, 58, 358, 344, 60, primary=True)
        self.play_button.configure(state="disabled", background='#18344b')
        self.rebuild_button = self.button('Rebuild Cache', self.rebuild_cache, 418, 358, 192, 60)
        self.repair_button = self.button('Repair', self.repair, 58, 514, 160, 40)
        self.repair_button.place_forget()
        self.rebuild_button.configure(state='disabled')
        self.status_dot = self.canvas.create_oval(60, 450, 65, 455, fill=BLUE,
                                                 outline='', tags='art')
        self.status = self.text(78, 445, "Checking game files…", 12, MUTED, width=350)
        self.canvas.scale("art", 0, 0, self.scale, self.scale)
        window.protocol("WM_DELETE_WINDOW", self.close)
        window.bind("<Escape>", lambda event: self.close())
        window.bind("<Return>", lambda event: self.play())
        self.play_button.focus_set()
        self.check_readiness()

    def check_readiness(self):
        if self.closed:
            return
        window = self.window
        self.frontend = False
        self.play_button.configure(state='disabled', background='#18344b')
        self.rebuild_button.configure(state='disabled')
        self.repair_button.place_forget()
        self.canvas.itemconfigure(self.status, text='Checking game files…', fill=MUTED)
        # Keep Tk responsive while the independent shader/resource admission
        # uses CPU workers. Only this UI thread ever touches a widget.
        results = queue.SimpleQueue()
        check = self.frontend_check

        def validate():
            try:
                results.put(check())
            except Exception:
                results.put(False)

        def poll():
            self._readiness_after = None
            if self.closed:
                return
            try:
                result = results.get_nowait()
                self.frontend = bool(result)
            except queue.Empty:
                self._readiness_after = window.after(15, poll)
                return
            self.canvas.itemconfigure(self.status, text=("Ready to play"
                                                       if self.frontend else getattr(result, 'message', UNAVAILABLE_MESSAGE)),
                                      fill=MUTED if self.frontend else '#e7ba8f')
            self.canvas.itemconfigure(self.status_dot, fill=BLUE if self.frontend else '#e7ba8f')
            self.play_button.configure(state="normal" if self.frontend and self.repair_dialog is None else "disabled",
                                       background='#b8e2f7' if self.frontend else '#18344b')
            if self.frontend:
                self.play_button.focus_set()
            else:
                self.place(self.repair_button, 58, 514, 160, 40)
                self.repair_button.focus_set()
            self.rebuild_button.configure(state='normal' if self.repair_dialog is None else 'disabled')

        self.readiness = threading.Thread(target=validate, name="halo2-launcher-readiness", daemon=True)
        self.readiness.start()
        self._readiness_after = window.after(0, poll)

    def repair(self):
        if self.closed or self.frontend or self.repair_dialog is not None:
            return
        from launcher_repair_ui import LauncherRepairWindow
        self.repair_dialog = LauncherRepairWindow(self)

    def rebuild_cache(self):
        if self.closed or self.repair_dialog is not None:
            return
        from launcher_repair_ui import LauncherRepairWindow
        self.play_button.configure(state='disabled', background='#18344b')
        self.rebuild_button.configure(state='disabled')
        self.repair_dialog = LauncherRepairWindow(self, rebuild=True)

    def play(self):
        if not self.closed and str(self.play_button.cget("state")) != "disabled":
            self.result = 0
            self.close()

    def close(self):
        if not self.closed:
            if self.repair_dialog is not None:
                self.repair_dialog.close(recheck=False)
            self.closed = True
            if self._readiness_after is not None:
                self.window.after_cancel(self._readiness_after)
                self._readiness_after = None
            self.window.destroy()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, required=True)
    parser.add_argument("--build-dir", type=Path)
    parser.add_argument("--runtime-unavailable")
    parser.add_argument("--launcher-executable", type=Path)
    parser.add_argument("--runtime-executable", type=Path)
    args = parser.parse_args()
    try:
        import tkinter as tk
        window = tk.Tk()
    except Exception as error:
        print(f"Cannot open the Halo 2 launcher in {sys.executable}: {error}\n"
              "Install Python Tk and run in a desktop session. "
              "Terminal launch: ./Halo2-Launcher --launch-game", file=sys.stderr)
        return 1
    home = LauncherHome(window, args.root, args.build_dir, runtime_unavailable=args.runtime_unavailable,
                        launcher_executable=args.launcher_executable, runtime_executable=args.runtime_executable)
    window.mainloop()
    return home.result


if __name__ == "__main__":
    raise SystemExit(main())
