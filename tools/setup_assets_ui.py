"""Hand-drawn first-run source selector; imports run off the UI thread.

The backdrop and wordmark are Canvas geometry. No game art, image files,
external fonts, or generated images are loaded by this window.
"""
import math
import codecs
import contextlib
import os
import queue
import signal
import subprocess
import sys
import threading
from pathlib import Path

from setup_pipe import read_available


WIDTH, HEIGHT = 1040, 720
INK = "#e4f5fb"
MUTED = "#94b5c8"
CYAN = "#8fe4f5"
PANEL = "#0b2032"
FIELD = "#061522"
EDGE = "#31566c"


class SetupProcess:
    """One main-thread-owned CLI import; its reader never writes game data."""

    def __init__(self, root, events, *, popen=None, cleanup_timeout=30):
        self.root, self.events = Path(root).resolve(), events
        self.popen = popen or subprocess.Popen
        self.cleanup_timeout = cleanup_timeout
        self.process = self.reader = None
        self.stop_signal = None
        self.previous = {}
        self.lock = threading.Lock()
        self.reader_stop, self.reader_done = threading.Event(), threading.Event()
        self.latest = self.tail = self.reader_error = ""

    def _handlers(self, install):
        if threading.current_thread() is not threading.main_thread():
            raise RuntimeError("MCC setup process ownership requires the main thread")
        if install:
            try:
                for number in (signal.SIGINT, signal.SIGTERM):
                    self.previous[number] = signal.signal(number, self._request_stop)
            except BaseException:
                self._handlers(False)
                raise
        else:
            for number, handler in self.previous.items():
                signal.signal(number, handler)
            self.previous.clear()

    def _request_stop(self, number, _frame):
        # Never raise between Popen creating a child and publishing its handle.
        self.stop_signal = self.stop_signal or number

    def _read(self, process):
        decoder = codecs.getincrementaldecoder("utf-8")("replace")
        try:
            fd = process.stdout.fileno()
            while not self.reader_stop.is_set():
                data = read_available(fd, 4096, .05)
                if data is None:
                    continue
                text = decoder.decode(data, final=not data)
                if text:
                    with self.lock:
                        self.tail = (self.tail + text)[-8192:]
                        self.latest = text.rstrip().splitlines()[-1][:250] if text.rstrip() else self.latest
                if not data:
                    break
        except Exception as error:
            with self.lock:
                self.reader_error = str(error)[:512]
        finally:
            self.reader_done.set()

    def start(self, selected):
        if self.process is not None:
            raise RuntimeError("An MCC setup process is already owned")
        if self.stop_signal is not None:
            return
        self._handlers(True)
        self.latest = self.tail = self.reader_error = ""
        self.reader_stop.clear(); self.reader_done.clear()
        from installation_paths import resource_root
        resources = resource_root(self.root)
        bundled = resources / ("python/python.exe" if os.name == "nt" else "python/bin/python3")
        python = str(bundled) if bundled.is_file() else sys.executable
        command = [python, "-u", "-B", str(resources / "tools/setup_assets.py"),
                   "--root", str(self.root), "--gui-owner"]
        if selected is not None:
            command += ["--mcc-root", str(Path(selected).expanduser().resolve())]
        env = os.environ.copy()
        env.update(PYTHONUNBUFFERED="1", PYTHONDONTWRITEBYTECODE="1", PYTHONIOENCODING="utf-8")
        try:
            self.process = self.popen(command, cwd=self.root, env=env, stdin=subprocess.PIPE,
                                      stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                                      start_new_session=os.name == "posix")
            self.reader = threading.Thread(target=self._read, args=(self.process,), daemon=True)
            self.reader.start()
            if self.stop_signal is not None:
                self.shutdown()
        except BaseException:
            self.shutdown()
            raise

    def _release(self):
        process, reader = self.process, self.reader
        # The bounded reader can retire even if an unexpected descendant holds
        # a pipe open. Such a stream is reported, never silently called success.
        drained = self.reader_done.wait(.25) if reader and reader.ident is not None else True
        self.reader_stop.set()
        if reader and reader.ident is not None:
            reader.join()
        if process is not None:
            for stream in (process.stdin, process.stdout):
                if stream is not None:
                    stream.close()
        self.process = self.reader = None
        self._handlers(False)
        return drained

    def shutdown(self):
        process = self.process
        forced = False
        try:
            if process is not None:
                # EOF requests cancellation in the child's main-thread scope;
                # it also works when the GUI disappears without running finally.
                if process.stdin is not None:
                    process.stdin.close()
                try:
                    process.wait(timeout=self.cleanup_timeout)
                except subprocess.TimeoutExpired:
                    forced = True
                    if process.poll() is None:
                        if os.name == "posix":
                            with contextlib.suppress(ProcessLookupError):
                                os.killpg(process.pid, signal.SIGTERM)
                        else:
                            process.terminate()
                    try:
                        process.wait(timeout=5)
                    except subprocess.TimeoutExpired:
                        forced = True
                        if os.name == "posix":
                            with contextlib.suppress(ProcessLookupError):
                                os.killpg(process.pid, signal.SIGKILL)
                        else:
                            process.kill()
                        process.wait()
                forced |= process.returncode is not None and process.returncode < 0
        finally:
            drained = self._release()
        if forced or not drained or self.reader_error:
            raise RuntimeError("MCC setup cleanup could not be verified. The child was stopped and reaped; "
                               "unfinished private files may remain. Existing valid caches were not deleted. "
                               "Retry setup after checking the import error.")

    def poll(self):
        process = self.process
        if process is None:
            return
        with self.lock:
            status, self.latest = self.latest, ""
            reader_error = self.reader_error
        if status:
            self.events.put(("status", status))
        if reader_error:
            try:
                self.shutdown()
            except RuntimeError as error:
                self.events.put(("error", str(error)))
            return
        code = process.poll()
        if code is None:
            return
        process.wait()
        drained = self._release()
        if code == 0 and drained and not self.reader_error:
            self.events.put(("done", None))
        else:
            detail = (self.tail.strip() + "\n" if self.tail.strip() else "") + f"MCC setup stopped (exit {code})."
            if not drained:
                detail += "\nSetup output did not close; cleanup could not be verified."
            self.events.put(("error", detail))


def _color(a, b, amount):
    return "#" + "".join(f"{round(int(a[i:i + 2], 16) * (1 - amount) + int(b[i:i + 2], 16) * amount):02x}"
                         for i in (1, 3, 5))


class LauncherCanvas:
    """Shared canvas, typography and controls for setup and the launcher home."""

    def __init__(self, window, root, title):
        import tkinter as tk
        from tkinter import filedialog, font

        self.window, self.root = window, root
        self.tk, self.filedialog = tk, filedialog
        self.scale = min(1.0, (window.winfo_screenwidth() - 40) / WIDTH,
                         (window.winfo_screenheight() - 80) / HEIGHT)
        self.scale = max(0.4, self.scale)
        families = set(font.families(window))
        self.family = next((name for name in ("Bahnschrift", "DejaVu Sans", "Arial") if name in families), "TkDefaultFont")
        self.mono = next((name for name in ("Consolas", "DejaVu Sans Mono") if name in families), "TkFixedFont")
        w, h = self.px(WIDTH), self.px(HEIGHT)
        window.title(title)
        window.configure(background=FIELD)
        window.resizable(False, False)
        # Let the window manager choose the monitor; centering on Tk's virtual
        # desktop can otherwise put the window across two physical displays.
        window.geometry(f"{w}x{h}")
        self.canvas = tk.Canvas(window, width=w, height=h, background=FIELD,
                                highlightthickness=0, borderwidth=0)
        self.canvas.pack(fill="both", expand=True)
        self._backdrop()
        self._wordmark(48, 65)

    def px(self, value):
        return max(1, round(value * self.scale))

    def font(self, size, bold=False, mono=False):
        return (self.mono if mono else self.family, -self.px(size), "bold" if bold else "normal")

    def text(self, x, y, value, size=14, fill=INK, mono=False, **options):
        if "width" in options:
            options["width"] = self.px(options["width"])
        return self.canvas.create_text(x, y, text=value, anchor="nw", fill=fill,
                                       font=self.font(size, mono=mono), tags="art", **options)

    def line(self, *coords, **options):
        return self.canvas.create_line(*coords, tags="art", **options)

    def place(self, widget, x, y, width, height):
        widget.place(x=self.px(x), y=self.px(y), width=self.px(width), height=self.px(height))

    def button(self, label, command, x, y, width, height, primary=False):
        button = self.tk.Button(self.window, text=label, command=command,
                                font=self.font(15 if primary else 13),
                                background="#a6e4ef" if primary else "#16364c",
                                foreground="#092335" if primary else INK,
                                activebackground="#cef5fc" if primary else "#254b63",
                                activeforeground="#092335" if primary else "#ffffff",
                                disabledforeground="#60788a", relief="flat", borderwidth=0,
                                highlightthickness=1, highlightbackground=CYAN if primary else EDGE,
                                highlightcolor="#e3faff", cursor="hand2", takefocus=True)
        base = button.cget("background")
        button.bind("<Enter>", lambda event: button.configure(background=button.cget("activebackground"))
                    if button.cget("state") != "disabled" else None)
        button.bind("<Leave>", lambda event: button.configure(background=base)
                    if button.cget("state") != "disabled" else None)
        button.bind("<Return>", lambda event: button.invoke())
        self.place(button, x, y, width, height)
        return button

    def progressbar(self):
        from tkinter import ttk

        style = ttk.Style(self.window)
        # Clam uses colorable elements on Windows as well as Linux/macOS.
        style.theme_use("clam")
        style.layout("Halo.Horizontal.TProgressbar", [
            ("Horizontal.Progressbar.trough", {"sticky": "nswe", "children": [
                ("Horizontal.Progressbar.pbar", {"side": "left", "sticky": "ns"})]})])
        style.configure("Halo.Horizontal.TProgressbar", troughcolor=FIELD, background=CYAN,
                        bordercolor=FIELD, lightcolor=CYAN, darkcolor=CYAN,
                        borderwidth=0, thickness=self.px(3))
        return ttk.Progressbar(self.window, style="Halo.Horizontal.TProgressbar", mode="indeterminate")

    def _backdrop(self):
        c = self.canvas
        # All decoration is deterministic linework, including the tilted Halo
        # silhouette. It is drawn beneath every hit region and never takes focus.
        for y in range(HEIGHT):
            t = y / HEIGHT
            glow = math.exp(-((y - 200) / 230) ** 2) * 0.18
            c.create_line(0, y, WIDTH, y, fill=_color("#071321", "#173e59", t * 0.57 + glow), tags="art")
        for i in range(96):
            x, y = (i * 193 + 61) % WIDTH, (i * 97 + 29) % 554
            color = ("#325166", "#456b80", "#6b8b9a")[i % 3]
            c.create_rectangle(x, y, x + (1 if i % 9 else 2), y + 1, fill=color, outline="", tags="art")
        def ring_point(t, radius=1):
            x, y = 294 * math.cos(t) * radius, 92 * math.sin(t) * radius
            angle = -0.51
            return (966 + x * math.cos(angle) - y * math.sin(angle),
                    30 + x * math.sin(angle) + y * math.cos(angle))
        for outer, inner, color in ((1.03, 0.86, "#18374b"), (0.86, 0.83, "#34566a"),
                                    (0.83, 0.81, "#688898")):
            points = [ring_point(i * math.tau / 160, outer) for i in range(161)]
            points += [ring_point(i * math.tau / 160, inner) for i in range(160, -1, -1)]
            c.create_polygon(*[v for point in points for v in point], fill=color, outline="", tags="art")
        for i in range(48):
            t = i * math.tau / 48
            a, b = ring_point(t, 0.87), ring_point(t, 1.02)
            self.line(*a, *b, fill="#426175")
        for offset in range(3):
            self.line(-60, 603 + offset * 13, 356, 465 + offset * 14,
                      730, 570 + offset * 16, 1100, 443 + offset * 14,
                      fill=("#19364c", "#1e4056", "#24495e")[offset])
        # Fine edge details echo the game's blue menu framing.
        self.line(26, 44, 26, 21, 120, 21, fill="#6392a7")
        self.line(920, 21, 1014, 21, 1014, 44, fill="#6392a7")
        self.line(26, 676, 26, 701, 120, 701, fill="#6392a7")
        self.line(920, 701, 1014, 701, 1014, 676, fill="#6392a7")

    def _wordmark(self, x, y):
        # Original vector lettering: open, extended strokes and an elliptical
        # O, with an inset cut on the numeral. No traced logo or font asset.
        c = self.canvas
        strokes = [
            (0, 0, 0, 53), (51, 0, 51, 53), (0, 25, 51, 25),
            (69, 53, 92, 0, 99, 0, 122, 53), (80, 32, 111, 32),
            (141, 0, 141, 53, 187, 53),
            (271, 6, 280, 0, 313, 0, 321, 8, 321, 19, 277, 51, 321, 51),
        ]
        for width, color in ((5, "#24465b"), (2, "#d2eaf5")):
            for stroke in strokes:
                self.line(*[value + (x if i % 2 == 0 else y) for i, value in enumerate(stroke)],
                          fill=color, width=width)
            c.create_oval(x + 202, y, x + 251, y + 53, outline=color, width=width, tags="art")
        self.line(x + 279, y + 57, x + 315, y + 57, fill="#6c9caf")
        self.line(x, y + 63, x + 252, y + 63, fill="#466a7e")


class SourceSelector(LauncherCanvas):
    """Detect MCC in the background and import into an owned local cache."""

    def __init__(self, window, root, mcc=None, setup_function=None,
                 discover_function=None, auto_detect=True, setup_process=None, steam_helper=None):
        from mcc_content import discover

        super().__init__(window, root, "Halo 2 — MCC setup")
        self.setup = setup_function
        self.discover = discover_function or discover
        self.events = setup_process.events if setup_process is not None else queue.Queue()
        self.process = setup_process if setup_process is not None else (
            SetupProcess(root, self.events) if setup_function is None else None)
        self.result, self.working, self.scanning, self.closed, self.error = 1, False, False, False, False
        self.modal = False
        self.steam = None
        if steam_helper is None:
            from macos.steam_setup import helper_path
            steam_helper = helper_path(root)
        self.steam_helper = steam_helper
        self.text(51, 158, "G A M E   F I L E   S E T U P", 12, CYAN)
        self.line(50, 193, 318, 193, fill=EDGE)
        self.text(49, 225, "Bring your copy.", 29, INK)
        self.text(51, 274, "Install Halo MCC.\nWe’ll prepare the cache.", 17, MUTED, width=280)
        self.text(51, 354, "G E T   S T A R T E D", 11, MUTED)
        self.steps = []
        for i, (title, detail) in enumerate((("Find MCC", "Search installed game libraries"),
                                           ("Prepare the cache", "Copy and convert game content"),
                                           ("Cache prepared", "MCC content copied locally"))):
            y = 397 + i * 66
            self.steps.append((self.text(52, y, f"0{i + 1}", 14, MUTED, mono=True),
                               self.text(88, y - 2, title, 16, MUTED),
                               self.text(88, y + 23, detail, 12, MUTED)))
        self.step_marker = self.canvas.create_rectangle(37, 395, 40, 436, fill=CYAN, outline="", tags="art")
        self._set_step(0)
        self.line(365, 223, 365, 660, fill="#29475b")
        self.text(400, 119, "Prepare from MCC", 30, INK)
        self.text(402, 163, "Halo MCC is the only installation you need.", 15, MUTED)
        self.canvas.create_polygon(400, 209, 977, 209, 988, 220, 988, 370,
                                   400, 370, fill=PANEL, outline=EDGE, tags="art")
        self.line(400, 209, 477, 209, fill=CYAN, width=2)
        self.text(423, 226, "01  /  HALO MCC", 10, CYAN, mono=True)
        self.text(422, 247, "Master Chief Collection", 21)
        self.text(423, 283, "MCC installation or halo2 folder. Leave blank to detect it.", 13, MUTED, width=535)
        self.value = self.tk.StringVar(window, value=str(mcc or ""))
        self.entry = self.tk.Entry(window, textvariable=self.value, font=self.font(13),
                                  background=FIELD, foreground=INK, insertbackground=CYAN,
                                  disabledbackground="#101f2b", disabledforeground="#637f90",
                                  selectbackground="#34677d", selectforeground="#ffffff",
                                  relief="flat", borderwidth=0, highlightthickness=1,
                                  highlightbackground=EDGE, highlightcolor=CYAN)
        self.place(self.entry, 423, 323, 426, 32)
        self.browse_button = self.button("Browse…", self.browse, 861, 323, 104, 32)
        self.scan_button = self.button("Scan installed games", self.scan, 400, 397, 220, 36)
        self.controls = [self.entry, self.browse_button, self.scan_button]
        if self.steam_helper is not None:
            self.steam_button = self.button('Download owned MCC from Steam…', self.start_steam, 644, 397, 344, 36)
            self.controls.append(self.steam_button)
        self.status_label = self.tk.Label(window, text="Scan installed games or browse to the MCC folder.",
                                         background=PANEL, foreground=MUTED, font=self.font(14),
                                         anchor="nw", justify="left", wraplength=self.px(555),
                                         padx=self.px(12), pady=self.px(12), borderwidth=0)
        self.place(self.status_label, 400, 451, 588, 138)
        self.bar = self.progressbar()
        self.place(self.bar, 400, 614, 588, 3)
        self.cancel = self.button("Cancel", self.close, 400, 641, 124, 43)
        self.prepare = self.button("Prepare MCC cache  ›", self.start, 696, 641, 292, 43, primary=True)
        self.text(52, 651, "YOUR ORIGINAL FILES STAY INTACT", 10, MUTED, mono=True)
        self.text(52, 672, "MCC content stays in your local cache.", 12, MUTED)
        self.line(26, 701, 1014, 701, fill=EDGE)
        self.canvas.scale("art", 0, 0, self.scale, self.scale)
        self.value.trace_add("write", self._selection_changed)
        self._selection_changed()
        window.protocol("WM_DELETE_WINDOW", self.close)
        window.bind("<Escape>", lambda event: self.close())
        self.poll_id = window.after(50, self.poll)
        self.entry.focus_set()
        if auto_detect and not mcc:
            window.after(1, self.scan)

    def _set_step(self, active):
        for i, (number, heading, caption) in enumerate(self.steps):
            self.canvas.itemconfigure(number, fill=CYAN if i == active else "#67879b")
            self.canvas.itemconfigure(heading, fill=INK if i == active else MUTED)
            self.canvas.itemconfigure(caption, fill=MUTED if i == active else "#7292a5")
        if hasattr(self, "step_marker"):
            scale = self.scale if hasattr(self, "prepare") else 1
            y = 395 + active * 66
            self.canvas.coords(self.step_marker, 37 * scale, y * scale, 40 * scale, (y + 41) * scale)

    def _selection_changed(self, *args):
        if self.working or self.scanning:
            return
        self.error = False
        self.status_label.configure(text="MCC folder selected. Prepare to copy its content into your local cache."
                                    if self.value.get().strip() else
                                    "Scan installed games or browse to the MCC folder.", foreground=MUTED)

    def browse(self):
        if self.working or self.scanning or self.modal:
            return
        path = self.filedialog.askdirectory(parent=self.window, title="Select the Halo MCC installation or halo2 folder")
        if path:
            self.value.set(path)

    def scan(self):
        if self.closed or self.working or self.scanning or self.modal:
            return
        self.scanning = True
        for widget in self.controls + [self.prepare]:
            widget.configure(state="disabled")
        self.status_label.configure(text="Searching installed MCC game libraries…", foreground=CYAN)
        self.bar.start(14)
        def work():
            try:
                self.events.put(("detected", str(self.discover())))
            except Exception as error:
                self.events.put(("not-found", str(error)))
        threading.Thread(target=work, daemon=True).start()

    def _busy(self, busy):
        self.working = busy
        for widget in self.controls + [self.prepare, self.cancel]:
            widget.configure(state="disabled" if busy else "normal")
        self.prepare.configure(text="Preparing MCC cache…" if busy else "Prepare MCC cache  ›",
                               background="#173448" if busy else "#a6e4ef")
        self._set_step(1 if busy else 0)
        if busy:
            self.bar.start(14)
        else:
            self.bar.stop()

    def start(self):
        if self.working or self.scanning or self.closed or self.modal:
            return
        value = self.value.get().strip()
        selected = Path(value).expanduser() if value else None
        self.error = False
        self._busy(True)
        self.status_label.configure(text="Preparing your private MCC cache. This may take a few minutes.", foreground=CYAN)
        if self.setup is None:
            try:
                self.process.start(selected)
            except Exception as error:
                self.events.put(("error", str(error)))
            return
        def work():
            try:
                self.setup(self.root, selected, progress=lambda message: self.events.put(("status", message)))
                self.events.put(("done", None))
            except Exception as error:
                self.events.put(("error", str(error)))
        threading.Thread(target=work, daemon=True).start()

    def start_steam(self):
        if self.working or self.scanning or self.closed or self.modal or self.steam_helper is None:
            return
        from macos.steam_setup import SteamSetupWindow
        self._busy(True)
        self.status_label.configure(text='Sign in with Steam to download your MCC Halo 2 content.', foreground=CYAN)
        try:
            self.steam = SteamSetupWindow(self, self.steam_helper)
        except Exception as error:
            self._busy(False)
            self.events.put(('error', str(error)))

    def _show_error(self, detail):
        """A modal in the same palette; grab prevents clicks on source fields."""
        tk = self.tk
        dialog = tk.Toplevel(self.window)
        dialog.title("Halo 2 — Import incomplete")
        dialog.transient(self.window)
        dialog.configure(background=PANEL, padx=self.px(26), pady=self.px(24))
        dialog.resizable(False, False)
        tk.Label(dialog, text="Import incomplete", background=PANEL, foreground=INK,
                 font=self.font(22), anchor="w").pack(fill="x", pady=(0, self.px(12)))
        # Long import errors remain readable without growing past the screen.
        detail_frame = tk.Frame(dialog, background=PANEL)
        detail_frame.pack(fill="both", expand=True)
        scrollbar = tk.Scrollbar(detail_frame, background=EDGE, troughcolor=FIELD,
                                 activebackground=CYAN, borderwidth=0, highlightthickness=0)
        scrollbar.pack(side="right", fill="y")
        text = tk.Text(detail_frame, width=65, height=8, background=FIELD, foreground=MUTED,
                       font=self.font(13), wrap="word", relief="flat", borderwidth=0,
                       padx=self.px(10), pady=self.px(10), highlightthickness=1,
                       highlightbackground=EDGE, highlightcolor=CYAN,
                       yscrollcommand=scrollbar.set)
        text.insert("1.0", str(detail))
        text.configure(state="disabled")
        text.pack(side="left", fill="both", expand=True)
        scrollbar.configure(command=text.yview)
        tk.Label(dialog, text="Check the MCC folder, then try again.", background=PANEL,
                 foreground=INK, font=self.font(13)).pack(anchor="w", pady=self.px(16))
        button = tk.Button(dialog, text="Back to setup", command=dialog.destroy, background="#a6e4ef",
                           foreground="#092335", activebackground="#cef5fc", relief="flat",
                           font=self.font(14), padx=self.px(18), pady=self.px(8))
        button.pack(anchor="e")
        button.bind("<Return>", lambda event: dialog.destroy())
        dialog.bind("<Escape>", lambda event: dialog.destroy())
        dialog.update_idletasks()
        dialog.geometry(f"+{self.window.winfo_rootx() + (self.window.winfo_width() - dialog.winfo_reqwidth()) // 2}"
                        f"+{self.window.winfo_rooty() + (self.window.winfo_height() - dialog.winfo_reqheight()) // 2}")
        dialog.grab_set()
        button.focus_set()
        self.window.wait_window(dialog)
        if not self.closed:
            self.prepare.focus_set()

    def poll(self):
        if self.steam is not None:
            self.steam.poll()
        if self.process is not None:
            if self.process.stop_signal is not None:
                self.result = 128 + self.process.stop_signal
                self.closed = True
                self.window.quit()
                return
            self.process.poll()
            if self.process.stop_signal is not None:
                self.result = 128 + self.process.stop_signal
                self.closed = True
                self.window.quit()
                return
        try:
            for _ in range(500):
                kind, value = self.events.get_nowait()
                if kind.startswith('steam'):
                    if self.steam is not None:
                        self.steam.event(kind, value)
                elif kind == "status":
                    text = str(value).replace("\n", " ")
                    self.status_label.configure(text=text if len(text) <= 250 else text[:247] + "…")
                elif kind in ("detected", "not-found"):
                    self.scanning = False
                    self.bar.stop()
                    for widget in self.controls + [self.prepare]:
                        widget.configure(state="normal")
                    if kind == "detected":
                        self.value.set(value)
                        self.status_label.configure(text="Found MCC Halo 2. Prepare to copy its content into your local cache.", foreground=CYAN)
                    else:
                        self.status_label.configure(text="MCC Halo 2 was not found. Install its Classic content, or browse to your MCC folder.", foreground="#ffd29b")
                elif kind == "done":
                    self.result = 0
                    self._set_step(2)
                    self.closed = True
                    self.bar.stop()
                    self.window.destroy()
                    return
                else:
                    self._busy(False)
                    self.error = True
                    self.status_label.configure(text="Import did not complete. Check the MCC folder and try again.", foreground="#ffd29b")
                    self.modal = True
                    try:
                        self._show_error(value)
                    finally:
                        self.modal = False
        except queue.Empty:
            pass
        if not self.closed:
            self.poll_id = self.window.after(50, self.poll)

    def close(self):
        if not self.working and not self.closed and not self.modal:
            self.closed = True
            self.window.after_cancel(self.poll_id)
            self.bar.stop()
            self.window.destroy()


def run(root, mcc=None):
    terminal_setup = ('Terminal setup: ./Halo2-Launcher --setup '
                      '--mcc-source "/path/to/Halo The Master Chief Collection" --setup-only')
    try:
        import tkinter as tk
    except ImportError as error:
        raise RuntimeError(f"MCC setup requires Python Tk, which could not load in {sys.executable}: {error}\n"
                           f"Install Tk for this Python interpreter and retry.\n{terminal_setup}") from error
    process = SetupProcess(root, queue.Queue())
    window = None
    try:
        try:
            window = tk.Tk()
        except tk.TclError as error:
            raise RuntimeError(f"Cannot open MCC setup: {error}\n{terminal_setup}") from error
        callback_errors = []
        def callback_error(kind, value, traceback):
            callback_errors.append(value.with_traceback(traceback))
            window.quit()
        window.report_callback_exception = callback_error
        selector = SourceSelector(window, root, mcc, setup_process=process)
        window.mainloop()
        if callback_errors:
            raise RuntimeError(f"MCC setup window failed: {callback_errors[0]}") from callback_errors[0]
        return 128 + process.stop_signal if process.stop_signal is not None else selector.result
    finally:
        try:
            try:
                if 'selector' in locals() and selector.steam is not None:
                    selector.steam.finish()
            finally:
                process.shutdown()
        finally:
            if window is not None:
                with contextlib.suppress(tk.TclError):
                    window.destroy()


if __name__ == "__main__":
    import argparse
    import tkinter as tk
    parser = argparse.ArgumentParser(description="Preview MCC setup without importing assets.")
    parser.parse_args()
    window = tk.Tk()
    def preview_only(*args, **kwargs):
        raise RuntimeError("This is a visual preview. Reopen Halo2-Launcher --setup to prepare the MCC cache.")
    SourceSelector(window, Path(__file__).resolve().parents[1], setup_function=preview_only)
    window.mainloop()
