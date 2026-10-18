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
    def __init__(self, window, root, build=None, update_function=None, availability_function=None, runtime_unavailable=None, preview_maps_function=None, preview_launch_function=None, launcher_executable=None, sandbox_launch_function=None, runtime_executable=None, native_test_function=None):
        from update_runtime import update_runtime
        from mcc_onboarding import NOT_PLAYABLE, runtime_status, preview_maps

        super().__init__(window, root, "Halo 2 — Launcher")
        self.build = build
        self.update = update_function or update_runtime
        self.availability = availability_function or (lambda: runtime_status(root))
        self.unavailable = runtime_unavailable or self.availability()
        def validated_native_test():
            from mcc_onboarding import selected
            from native_gameplay_test import native_gameplay_test_available
            try:
                source = selected(root)
                return bool(source and native_gameplay_test_available(root, runtime_executable, source[0]))
            except (OSError, ValueError, KeyError):
                return False
        self.native_test_available = native_test_function or validated_native_test
        self.native_test = self.unavailable == NOT_PLAYABLE and self.native_test_available()
        self.preview_maps = (preview_maps_function or (lambda: preview_maps(root)))()
        self.test_build = self.unavailable == NOT_PLAYABLE and bool(self.preview_maps) and not self.native_test
        self.test_message = "Play opens the MCC content test build. Native gameplay and Xbox TU1.5 conversion are still in progress."
        from native_gameplay_test import MESSAGE
        self.native_test_message = MESSAGE
        self.precompiled = (Path(root) / "PRECOMPILED_TEST_BUILD.json").is_file()
        self.launcher_executable = Path(launcher_executable) if launcher_executable else Path(root) / "Halo2-Launcher"
        self.preview_launch = preview_launch_function or (lambda argv: subprocess.Popen(argv, cwd=root, start_new_session=True))
        self.sandbox_launcher = Path(root) / "Halo2-Sandbox-Test"
        self.sandbox_launch = sandbox_launch_function or (lambda argv: subprocess.Popen(argv, cwd=root, start_new_session=True))
        sandbox_ready = self.sandbox_launcher.is_file() and os.access(self.sandbox_launcher, os.X_OK)
        self.events = queue.Queue()
        self.result, self.working, self.closed = CANCELLED, False, False
        self.text(51, 158, "G A M E   L A U N C H E R", 12, CYAN)
        self.line(50, 193, 318, 193, fill=EDGE)
        self.text(49, 225, "Welcome back.", 29, INK)
        self.text(51, 274, "Your MCC cache is prepared.\nPreview imported map content.", 17, MUTED, width=280)
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
        self.place(output, 400, 328, 588, 174)
        scrollbar = self.tk.Scrollbar(output, background=EDGE, troughcolor=FIELD,
                                      activebackground=CYAN, borderwidth=0, highlightthickness=0)
        scrollbar.pack(side="right", fill="y")
        self.output = self.tk.Text(output, background=FIELD, foreground=MUTED, font=self.font(12, mono=True),
                                   wrap="word", relief="flat", padx=self.px(12), pady=self.px(12),
                                   width=1, height=1, yscrollcommand=scrollbar.set,
                                   highlightthickness=1, highlightbackground=EDGE, highlightcolor=CYAN)
        initial_help = ("Play opens the validated local Lockout gameplay test. "
                        "Full gameplay, visual and audio parity remain incomplete." if self.native_test else
                        "Preview opens imported MCC map assets. Native gameplay testing requires a validated runtime.")
        self.output.insert("end", initial_help + "\n\nUpdate and build progress will appear here.\n")
        self.output.configure(state="disabled")
        self.output.pack(fill="both", expand=True)
        scrollbar.configure(command=self.output.yview)
        self.bar = self.progressbar()
        self.place(self.bar, 400, 635 if sandbox_ready else 614, 588, 3)
        self.exit_button = self.button("Exit", self.close, 400, 641, 124, 43)
        self.update_button = self.button("Update", self.start_update, 540, 641, 208, 43)
        self.play_button = self.button("Play  ›", self.play, 764, 641, 224, 43, primary=True)
        from tkinter import ttk
        choices = list(self.preview_maps)
        initial = next((name for name in choices if Path(name).name.casefold() == "lockout.map"), choices[0] if choices else "No imported MCC maps")
        self.preview_map = self.tk.StringVar(window, value=initial)
        style = ttk.Style(window)
        style.configure("Halo2Preview.TCombobox", fieldbackground=FIELD, background=PANEL, foreground=INK)
        style.map("Halo2Preview.TCombobox", fieldbackground=[("readonly", FIELD)], foreground=[("readonly", INK)])
        self.preview_selector = ttk.Combobox(window, textvariable=self.preview_map, values=choices, state="readonly", font=self.font(13), style="Halo2Preview.TCombobox")
        self.place(self.preview_selector, 400, 516, 588, 29)
        self.preview_button = self.button("Preview MCC content  ›", self.play_preview, 400, 555, 588, 43, primary=True)
        self.sandbox_button = self.button("Xbox TU1.5 sandbox test  ›", self.play_sandbox, 400, 606, 588, 26) if sandbox_ready else None
        if self.sandbox_button:
            self.sandbox_button.configure(font=self.font(13))
        if not choices:
            self.preview_selector.configure(state="disabled")
            self.preview_button.configure(state="disabled")
        if self.native_test:
            self.play_button.configure(text="Play native test  ›")
            self.status.configure(text=self.native_test_message, foreground="#ffd29b")
        elif self.test_build:
            self.status.configure(text=self.test_message, foreground="#ffd29b")
        elif self.unavailable:
            self.play_button.configure(state="disabled")
            self.status.configure(text=self.unavailable, foreground="#ffd29b")
        if self.precompiled:
            self.update_button.configure(state="disabled")
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
        if self.working or self.precompiled:
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
                    self.preview_selector.configure(state="readonly" if self.preview_maps else "disabled")
                    self.preview_button.configure(state="normal" if self.preview_maps else "disabled")
                    if self.sandbox_button:
                        self.sandbox_button.configure(state="normal")
                    # A failed update can leave source and runtime at different
                    # revisions. Require a successful retry before playing.
                    self.unavailable = self.availability() if kind == "done" else "Update stopped. Check the output below, then try Update again."
                    from mcc_onboarding import NOT_PLAYABLE
                    self.native_test = kind == "done" and self.unavailable == NOT_PLAYABLE and self.native_test_available()
                    self.test_build = self.unavailable == NOT_PLAYABLE and bool(self.preview_maps) and not self.native_test
                    ready = kind == "done" and (not self.unavailable or self.test_build or self.native_test)
                    self.play_button.configure(text="Play native test  ›" if self.native_test else "Play  ›")
                    self.play_button.configure(state="normal" if ready else "disabled")
                    self.status.configure(text=self.native_test_message if self.native_test else self.test_message if self.test_build else self.unavailable or message,
                                          foreground="#ffd29b" if self.test_build or self.native_test or not ready else CYAN)
                    (self.play_button if ready else self.update_button).focus_set()
                else:
                    self.status.configure(text=message[:130])
        except queue.Empty:
            pass
        if not self.closed:
            self.poll_id = self.window.after(50, self.poll)

    def play(self):
        if not self.working and str(self.play_button.cget("state")) != "disabled":
            if self.test_build:
                self.play_preview()
                return
            self.result = 0
            self.close()

    def actions(self):
        return tuple(action for action in (self.exit_button, self.update_button, self.play_button, self.preview_button, self.preview_selector, self.sandbox_button) if action)

    def play_sandbox(self):
        if self.working or not self.sandbox_button or str(self.sandbox_button.cget("state")) == "disabled":
            return
        try:
            self.sandbox_launch([str(self.sandbox_launcher)])
        except OSError as error:
            self.status.configure(text="Could not open sandbox test build: " + str(error), foreground="#ffd29b")
            return
        self.close()

    def play_preview(self):
        if self.working or str(self.preview_button.cget("state")) == "disabled":
            return
        selected = self.preview_maps.get(self.preview_map.get())
        if selected is None:
            self.status.configure(text="Select an imported MCC map to preview.", foreground="#ffd29b")
            return
        try:
            self.preview_launch([str(self.launcher_executable), "--preview-mcc", "--map", str(selected)])
        except OSError as error:
            self.status.configure(text="Could not open MCC content preview: " + str(error), foreground="#ffd29b")
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
    parser.add_argument("--launcher-executable", type=Path)
    parser.add_argument("--runtime-executable", type=Path, help="selected runtime for separate native-test validation")
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
    home = LauncherHome(window, args.root, args.build_dir, runtime_unavailable=args.runtime_unavailable,
                        launcher_executable=args.launcher_executable, runtime_executable=args.runtime_executable)
    window.mainloop()
    return home.result


if __name__ == "__main__":
    raise SystemExit(main())
