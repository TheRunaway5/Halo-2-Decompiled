"""Launcher home shown in place of setup once the game files are prepared."""
import argparse
from pathlib import Path
import queue
import sys
import threading
import subprocess
import os

from setup_assets_ui import LauncherCanvas, CYAN, EDGE, FIELD, INK, MUTED, PANEL


CANCELLED = 20


class LauncherHome(LauncherCanvas):
    def __init__(self, window, root, build=None, update_function=None, availability_function=None, runtime_unavailable=None, sandbox_launch_function=None):
        from update_runtime import update_runtime
        from mcc_onboarding import runtime_status

        super().__init__(window, root, "Halo 2 — Launcher")
        self.build = build
        self.update = update_function or update_runtime
        self.availability = availability_function or (lambda: runtime_status(root))
        self.unavailable = runtime_unavailable or self.availability()
        self.sandbox_launcher = Path(root) / "Halo2-Sandbox-Test"
        self.sandbox_launch = sandbox_launch_function or (lambda argv: subprocess.Popen(argv, cwd=root, start_new_session=True))
        sandbox_ready = self.sandbox_launcher.is_file() and os.access(self.sandbox_launcher, os.X_OK)
        self.events = queue.Queue()
        self.result, self.working, self.closed = CANCELLED, False, False
        self.text(51, 158, "G A M E   L A U N C H E R", 12, CYAN)
        self.line(50, 193, 318, 193, fill=EDGE)
        self.text(49, 225, "Welcome back.", 29, INK)
        self.text(51, 274, "Your MCC cache is prepared.\nCheck the status to continue.", 17, MUTED, width=280)
        self.text(51, 397, "GAME FILES PREPARED", 12, CYAN)
        self.text(51, 432, "Halo MCC content is copied\nto your private local cache.", 15, MUTED, width=280)
        self.text(51, 519, "Keep Halo 2 current", 19, INK)
        self.text(51, 554, "Update downloads changes and\nbuilds the latest game.", 15, MUTED, width=280)
        self.line(365, 223, 365, 660, fill="#29475b")
        self.text(400, 119, "Halo 2", 30, INK)
        self.text(402, 163, "Launch Halo 2 or install the latest changes.", 15, MUTED)
        self.status = self.tk.Label(window, text="Select Play to start Halo 2.", background=PANEL,
                                    foreground=CYAN, font=self.font(15), anchor="nw", justify="left",
                                    wraplength=self.px(563), padx=self.px(12), pady=self.px(12))
        self.place(self.status, 400, 218, 588, 92)
        output = self.tk.Frame(window, background=FIELD)
        self.place(output, 400, 328, 588, 210 if sandbox_ready else 274)
        scrollbar = self.tk.Scrollbar(output, background=EDGE, troughcolor=FIELD,
                                      activebackground=CYAN, borderwidth=0, highlightthickness=0)
        scrollbar.pack(side="right", fill="y")
        self.output = self.tk.Text(output, background=FIELD, foreground=MUTED, font=self.font(12, mono=True),
                                   wrap="word", relief="flat", padx=self.px(12), pady=self.px(12),
                                   width=1, height=1, yscrollcommand=scrollbar.set,
                                   highlightthickness=1, highlightbackground=EDGE, highlightcolor=CYAN)
        self.output.insert("end", "Update and build progress will appear here.\n")
        self.output.configure(state="disabled")
        self.output.pack(fill="both", expand=True)
        scrollbar.configure(command=self.output.yview)
        self.bar = self.progressbar()
        self.place(self.bar, 400, 614, 588, 3)
        self.exit_button = self.button("Exit", self.close, 400, 641, 124, 43)
        self.update_button = self.button("Update", self.start_update, 540, 641, 208, 43)
        self.play_button = self.button("Play  ›", self.play, 764, 641, 224, 43, primary=True)
        self.sandbox_button = self.button("Sandbox test build  ›", self.play_sandbox, 400, 555, 588, 43, primary=True) if sandbox_ready else None
        if self.unavailable:
            self.play_button.configure(state="disabled")
            self.status.configure(text=self.unavailable, foreground="#ffd29b")
        if (Path(root) / "launcher-update.pending").exists():
            self.play_button.configure(state="disabled")
            self.status.configure(text="An update needs to finish. Select Update to retry the build.", foreground="#ffd29b")
        self.text(52, 651, "YOUR GAME FILES STAY INTACT", 10, MUTED, mono=True)
        self.text(52, 672, "MCC content. Existing native shaders.", 12, MUTED)
        self.canvas.scale("art", 0, 0, self.scale, self.scale)
        window.protocol("WM_DELETE_WINDOW", self.close)
        window.bind("<Escape>", lambda event: self.close())
        self.poll_id = window.after(50, self.poll)
        (self.update_button if str(self.play_button.cget("state")) == "disabled" else self.play_button).focus_set()

    def start_update(self):
        if self.working:
            return
        self.working = True
        for button in self.actions():
            button.configure(state="disabled")
        self.status.configure(text="Updating and building Halo 2…", foreground=CYAN)
        self.bar.start(14)

        def work():
            try:
                self.update(self.root, self.build, progress=lambda message: self.events.put(("status", message)))
                self.events.put(("done", "Update complete. Select Play to start Halo 2."))
            except Exception as error:
                self.events.put(("error", str(error)))

        threading.Thread(target=work, daemon=True).start()

    def poll(self):
        try:
            for _ in range(500):
                kind, message = self.events.get_nowait()
                self.output.configure(state="normal")
                self.output.insert("end", message + "\n")
                if int(self.output.index("end-1c").split(".")[0]) > 2000:
                    self.output.delete("1.0", "501.0")
                self.output.see("end")
                self.output.configure(state="disabled")
                if kind in ("done", "error"):
                    self.working = False
                    self.bar.stop()
                    self.update_button.configure(state="normal")
                    self.exit_button.configure(state="normal")
                    if self.sandbox_button:
                        self.sandbox_button.configure(state="normal")
                    # A failed update can leave source and runtime at different
                    # revisions. Require a successful retry before playing.
                    self.unavailable = self.availability() if kind == "done" else "Update stopped. Check the output below, then try Update again."
                    ready = kind == "done" and not self.unavailable
                    self.play_button.configure(state="normal" if ready else "disabled")
                    self.status.configure(text=self.unavailable or message, foreground=CYAN if ready else "#ffd29b")
                    (self.play_button if ready else self.update_button).focus_set()
                else:
                    self.status.configure(text=message[:130])
        except queue.Empty:
            pass
        if not self.closed:
            self.poll_id = self.window.after(50, self.poll)

    def play(self):
        if not self.working and str(self.play_button.cget("state")) != "disabled":
            self.result = 0
            self.close()

    def actions(self):
        return tuple(button for button in (self.exit_button, self.update_button, self.play_button, self.sandbox_button) if button)

    def play_sandbox(self):
        if self.working or not self.sandbox_button or str(self.sandbox_button.cget("state")) == "disabled":
            return
        try:
            self.sandbox_launch([str(self.sandbox_launcher)])
        except OSError as error:
            self.status.configure(text="Could not open sandbox test build: " + str(error), foreground="#ffd29b")
            return
        self.close()

    def close(self):
        if not self.working and not self.closed:
            self.closed = True
            self.window.after_cancel(self.poll_id)
            self.window.destroy()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, required=True)
    parser.add_argument("--build-dir", type=Path)
    parser.add_argument("--runtime-unavailable", help="initial native MCC readiness message")
    args = parser.parse_args()
    try:
        import tkinter as tk
        window = tk.Tk()
    except Exception as error:
        print(f"Cannot open the Halo 2 launcher in {sys.executable}: {error}\n"
              "Install Python Tk and run in a desktop session. "
              "Terminal launch: ./Halo2-Launcher --launch-game; update: ./Halo2-Launcher --update",
              file=sys.stderr)
        return 1
    home = LauncherHome(window, args.root, args.build_dir, runtime_unavailable=args.runtime_unavailable)
    window.mainloop()
    return home.result


if __name__ == "__main__":
    raise SystemExit(main())
