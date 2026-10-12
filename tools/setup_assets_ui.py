"""Hand-drawn first-run source selector; imports run off the UI thread.

The backdrop and wordmark are Canvas geometry. No game art, image files,
external fonts, or generated images are loaded by this window.
"""
import math
import queue
import threading
from pathlib import Path


WIDTH, HEIGHT = 1040, 720
INK = "#e4f5fb"
MUTED = "#94b5c8"
CYAN = "#8fe4f5"
PANEL = "#0b2032"
FIELD = "#061522"
EDGE = "#31566c"


def _color(a, b, amount):
    return "#" + "".join(f"{round(int(a[i:i + 2], 16) * (1 - amount) + int(b[i:i + 2], 16) * amount):02x}"
                         for i in (1, 3, 5))


class SourceSelector:
    """Own the window so previews can exercise the actual controls safely."""

    def __init__(self, window, root, vista=None, xbox=None, setup_function=None):
        import tkinter as tk
        from tkinter import filedialog, font, ttk

        self.window, self.root = window, root
        self.tk, self.filedialog = tk, filedialog
        if setup_function is None:
            from setup_assets import setup
            setup_function = setup
        self.setup = setup_function
        self.events = queue.Queue()
        self.result = 1
        self.working = False
        self.closed = False
        self.error = False
        self.controls = []
        self.entries = []
        self.source_indicators = []
        self.scale = min(1.0, (window.winfo_screenwidth() - 40) / WIDTH,
                         (window.winfo_screenheight() - 80) / HEIGHT)
        self.scale = max(0.4, self.scale)
        families = set(font.families(window))
        self.family = next((name for name in ("Bahnschrift", "DejaVu Sans", "Arial") if name in families), "TkDefaultFont")
        self.mono = next((name for name in ("Consolas", "DejaVu Sans Mono") if name in families), "TkFixedFont")
        w, h = self.px(WIDTH), self.px(HEIGHT)
        window.title("Halo 2 — Game file setup")
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
        self.text(51, 158, "G A M E   F I L E   S E T U P", 12, CYAN)
        self.line(50, 193, 318, 193, fill=EDGE)
        self.text(49, 225, "Welcome back.", 29, INK)
        self.text(51, 274, "Bring your copy.\nWe’ll prepare the rest.", 17, MUTED, width=280)
        self.text(51, 354, "G E T   S T A R T E D", 11, MUTED)
        self.steps = []
        for i, (title, detail) in enumerate((("Select your sources", "Xbox disc + Vista installer"),
                                           ("Build the game cache", "Prepared automatically"),
                                           ("Ready to play", "Launch into Halo 2"))):
            y = 397 + i * 66
            number = self.text(52, y, f"0{i + 1}", 14, MUTED, mono=True)
            heading = self.text(88, y - 2, title, 16, MUTED)
            caption = self.text(88, y + 23, detail, 12, MUTED)
            self.steps.append((number, heading, caption))
        self.step_marker = self.canvas.create_rectangle(37, 395, 40, 436, fill=CYAN, outline="", tags="art")
        self._set_step(0)
        self.line(365, 223, 365, 660, fill="#29475b")
        self.text(400, 119, "Prepare your game", 30, INK)
        self.text(402, 163, "Choose both sources to build your cache.", 15, MUTED)
        # Keep the importer's Vista/Xbox argument order, while presenting the
        # original game disc first in the visual reading and keyboard order.
        self.values = [tk.StringVar(window, value=str(vista or "")),
                       tk.StringVar(window, value=str(xbox or ""))]
        self._source(1, 209, "01", "Xbox game disc", "Original gameplay, models and world textures", "Browse ISO…")
        self._source(0, 383, "02", "Vista installer", "Menus, HUD, fonts and match announcer audio", "Browse file…")
        self.status_label = tk.Label(window, text="Select your Xbox disc image and Halo 2 Vista installer.",
                                     background="#0d263b", foreground=MUTED, font=self.font(13),
                                     anchor="nw", justify="left", wraplength=self.px(563), borderwidth=0)
        self.place(self.status_label, 402, 562, 585, 44)
        style = ttk.Style(window)
        # Clam uses colorable elements on Windows as well as Linux/macOS.
        style.theme_use("clam")
        style.layout("Halo.Horizontal.TProgressbar", [
            ("Horizontal.Progressbar.trough", {"sticky": "nswe", "children": [
                ("Horizontal.Progressbar.pbar", {"side": "left", "sticky": "ns"})]})])
        style.configure("Halo.Horizontal.TProgressbar", troughcolor=FIELD, background=CYAN,
                        bordercolor=FIELD, lightcolor=CYAN, darkcolor=CYAN,
                        borderwidth=0, thickness=self.px(3))
        self.bar = ttk.Progressbar(window, style="Halo.Horizontal.TProgressbar", mode="indeterminate")
        self.place(self.bar, 401, 614, 587, 3)
        self.cancel = self.button("Cancel", self.close, 400, 641, 124, 43)
        self.prepare = self.button("Prepare game files  ›", self.start, 696, 641, 292, 43, primary=True)
        self.text(52, 651, "YOUR ORIGINAL FILES STAY INTACT", 10, MUTED, mono=True)
        self.text(52, 672, "Select once. Play from your local cache.", 12, MUTED)
        self.line(26, 701, 1014, 701, fill=EDGE)
        # Scale geometry once; text uses pixel fonts and widgets have scaled
        # placement so Tk's platform-specific DPI setting cannot cause overlaps.
        self.canvas.scale("art", 0, 0, self.scale, self.scale)
        for value in self.values:
            value.trace_add("write", self._selection_changed)
        self._selection_changed()
        window.protocol("WM_DELETE_WINDOW", self.close)
        window.bind("<Escape>", lambda event: self.close())
        self.poll_id = window.after(100, self.poll)
        self.entries[0].focus_set()

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

    def _source(self, index, y, number, title, hint, browse_label):
        c = self.canvas
        border = c.create_polygon(400, y, 977, y, 988, y + 11, 988, y + 150,
                                  400, y + 150, fill=PANEL, outline=EDGE, tags="art")
        self.line(400, y, 477, y, fill=CYAN, width=2)
        self.text(423, y + 17, number + "  /  " + ("ORIGINAL XBOX" if index else "HALO 2 VISTA"), 10, CYAN, mono=True)
        self.text(422, y + 38, title, 21)
        indicator = self.text(970, y + 19, "REQUIRED", 10, MUTED, mono=True)
        c.itemconfigure(indicator, anchor="ne")
        self.source_indicators.append((index, indicator))
        self.text(423, y + 71, hint, 13, MUTED)
        entry = self.tk.Entry(self.window, textvariable=self.values[index], font=self.font(13),
                              background=FIELD, foreground=INK, insertbackground=CYAN,
                              disabledbackground="#101f2b", disabledforeground="#637f90",
                              selectbackground="#34677d", selectforeground="#ffffff",
                              relief="flat", borderwidth=0, highlightthickness=1,
                              highlightbackground=EDGE, highlightcolor=CYAN)
        self.place(entry, 423, y + 104, 311, 32)
        self.entries.append(entry)
        file_button = self.button(browse_label, lambda: self.browse(index), 746, y + 104, 111, 32)
        folder_button = self.button("Folder…", lambda: self.browse(index, folder=True), 869, y + 104, 96, 32)
        entry.bind("<FocusIn>", lambda event: c.itemconfigure(border, outline="#6caabc"))
        entry.bind("<FocusOut>", lambda event: c.itemconfigure(border, outline=EDGE))
        self.controls.extend((entry, file_button, folder_button))

    def _set_step(self, active):
        for i, (number, heading, caption) in enumerate(self.steps):
            self.canvas.itemconfigure(number, fill=CYAN if i == active else "#67879b")
            self.canvas.itemconfigure(heading, fill=INK if i == active else MUTED)
            self.canvas.itemconfigure(caption, fill=MUTED if i == active else "#7292a5")
        # Called before the one-time geometry scale during construction.
        if hasattr(self, "step_marker"):
            scale = self.scale if hasattr(self, "prepare") else 1
            y = 395 + active * 66
            self.canvas.coords(self.step_marker, 37 * scale, y * scale, 40 * scale, (y + 41) * scale)

    def _selection_changed(self, *args):
        for index, indicator in self.source_indicators:
            selected = bool(self.values[index].get().strip())
            self.canvas.itemconfigure(indicator, text="SELECTED" if selected else "REQUIRED",
                                      fill=CYAN if selected else MUTED)
        if self.working:
            return
        self.error = False
        if all(v.get().strip() for v in self.values):
            self.status_label.configure(text="Both sources selected. Ready to prepare your game files.", foreground=CYAN)
        else:
            self.status_label.configure(text="Select your Xbox disc image and Halo 2 Vista installer.", foreground=MUTED)

    def browse(self, index, folder=False):
        if self.working:
            return
        if folder:
            path = self.filedialog.askdirectory(parent=self.window, title="Select Halo 2 source folder")
        else:
            types = [("Halo 2 Vista installer", "*.zip *.bin *.exe *.7z"), ("All files", "*")] if index == 0 else [
                ("Xbox disc image", "*.iso"), ("All files", "*")]
            path = self.filedialog.askopenfilename(parent=self.window,
                    title="Select " + ("Vista installer" if index == 0 else "Xbox disc image"), filetypes=types)
        if path:
            self.error = False
            self.values[index].set(path)

    def _busy(self, busy):
        self.working = busy
        for widget in self.controls + [self.prepare, self.cancel]:
            widget.configure(state="disabled" if busy else "normal")
        self.prepare.configure(text="Preparing game files…" if busy else "Prepare game files  ›",
                               background="#173448" if busy else "#a6e4ef")
        self._set_step(1 if busy else 0)
        if busy:
            self.bar.start(14)
        else:
            self.bar.stop()

    def start(self):
        if self.working:
            return
        missing = next((i for i in (1, 0) if not self.values[i].get().strip()), None)
        if missing is not None:
            self.error = True
            self.status_label.configure(text="Select both sources: your Xbox disc image and Vista installer.", foreground="#ffd29b")
            self.controls[0 if missing == 1 else 3].focus_set()
            return
        selected = [Path(v.get().strip()).expanduser() for v in self.values]
        self.error = False
        self._busy(True)
        self.status_label.configure(text="Preparing your local cache. This may take a few minutes.", foreground=CYAN)

        def work():
            try:
                self.setup(self.root, *selected, progress=lambda message: self.events.put(("status", message)))
                self.events.put(("done", None))
            except Exception as error:
                self.events.put(("error", str(error)))
        threading.Thread(target=work, daemon=True).start()

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
        # Long archive errors remain readable without growing past the screen.
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
        tk.Label(dialog, text="Check your sources, then try again.", background=PANEL,
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
        try:
            while True:
                kind, value = self.events.get_nowait()
                if kind == "status":
                    # Stage paths can be very long. Keep status inside its
                    # region; full errors remain in the scrollable dialog.
                    text = str(value).replace("\n", " ")
                    self.status_label.configure(text=text if len(text) <= 150 else text[:147] + "…")
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
                    self.status_label.configure(text="Import did not complete. Check your sources and try again.", foreground="#ffd29b")
                    self._show_error(value)
        except queue.Empty:
            pass
        if not self.closed:
            self.poll_id = self.window.after(100, self.poll)

    def close(self):
        if not self.working and not self.closed:
            self.closed = True
            self.window.after_cancel(self.poll_id)
            self.window.destroy()


def run(root, vista=None, xbox=None):
    try:
        import tkinter as tk
    except ImportError as error:
        raise RuntimeError("The source selector requires Python Tk. Alternatively use --vista-source and --xbox-source.") from error
    try:
        window = tk.Tk()
    except tk.TclError as error:
        raise RuntimeError("Cannot open the source selector. Use --vista-source and --xbox-source for terminal setup.") from error
    selector = SourceSelector(window, root, vista, xbox)
    window.mainloop()
    return selector.result


if __name__ == "__main__":
    # Open the actual selector without an asset import for design inspection.
    import argparse
    import tkinter as tk

    parser = argparse.ArgumentParser(description="Preview the hand-drawn Halo 2 setup window (no imports).")
    parser.parse_args()
    window = tk.Tk()
    def preview_only(*args, **kwargs):
        raise RuntimeError("This is a visual preview. Reopen setup through Halo2-Launcher --setup to prepare game files.")
    SourceSelector(window, Path(__file__).resolve().parents[1], setup_function=preview_only)
    window.mainloop()
