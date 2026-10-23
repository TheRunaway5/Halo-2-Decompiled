"""Launcher home: Play opens the intro, title screen and main menu."""
import argparse
from pathlib import Path
import sys

from setup_assets_ui import LauncherCanvas, CYAN, EDGE, INK, MUTED, PANEL

CANCELLED = 20
UNAVAILABLE_MESSAGE = "The game menu is unavailable. Update or repair the installed game files."


class LauncherHome(LauncherCanvas):
    def __init__(self, window, root, build=None, *, runtime_unavailable=None,
                 launcher_executable=None, runtime_executable=None, frontend_function=None):
        super().__init__(window, root, "Halo 2 — Launcher")
        self.runtime_executable = runtime_executable
        self.result, self.closed = CANCELLED, False
        from installation_paths import resource_root
        self.precompiled = (resource_root(root) != Path(root).resolve() or
                            (Path(root) / "PRECOMPILED_TEST_BUILD.json").is_file())

        def validated_frontend():
            from mcc_onboarding import selected
            from frontend_runtime_selection import selected_frontend_runtime
            try:
                source = selected(root)
                return bool(source and selected_frontend_runtime(root, self.runtime_executable, source[0]))
            except (OSError, ValueError, KeyError, TypeError, RuntimeError):
                return False

        self.frontend = (frontend_function or validated_frontend)()
        self.text(51, 158, "G A M E   L A U N C H E R", 12, CYAN)
        self.line(50, 193, 318, 193, fill=EDGE)
        self.text(49, 225, "Welcome back.", 29, INK)
        self.text(51, 274, "Your next match starts here.", 17, MUTED, width=280)
        self.line(365, 223, 365, 660, fill="#29475b")
        self.text(400, 119, "Halo 2", 30, INK)
        self.text(402, 163, "Ready when you are.", 15, MUTED)
        self.status = self.tk.Label(window, text=("Play opens the intro, title screen and main menu."
                                    if self.frontend else UNAVAILABLE_MESSAGE), background=PANEL,
                                    foreground=CYAN if self.frontend else "#ffd29b", font=self.font(17),
                                    anchor="nw", justify="left", wraplength=self.px(540))
        self.place(self.status, 402, 275, 560, 100)
        self.play_button = self.button("Play", self.play, 400, 572, 588, 70, primary=True)
        if not self.frontend:
            self.play_button.configure(state="disabled")
        self.canvas.scale("art", 0, 0, self.scale, self.scale)
        window.protocol("WM_DELETE_WINDOW", self.close)
        window.bind("<Escape>", lambda event: self.close())
        window.bind("<Return>", lambda event: self.play())
        self.play_button.focus_set()

    def play(self):
        if not self.closed and str(self.play_button.cget("state")) != "disabled":
            self.result = 0
            self.close()

    def close(self):
        if not self.closed:
            self.closed = True
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
