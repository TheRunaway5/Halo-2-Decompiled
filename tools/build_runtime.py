"""Build the local runtime after asset preparation, preserving moved build trees."""
import argparse
import os
from pathlib import Path
import queue
import shutil
import subprocess
import threading


def current_cache(root, build):
    cache = build / "CMakeCache.txt"
    if not cache.is_file():
        return (not build.exists() and not build.is_symlink()) or (build.is_dir() and not any(build.iterdir()))
    values = {}
    for line in cache.read_text(errors="replace").splitlines():
        if "=" in line and not line.startswith(("#", "//")):
            key, value = line.split("=", 1)
            values[key] = value
    return all(values.get(key) and Path(values[key]).resolve() == expected.resolve()
               for key, expected in (("CMAKE_HOME_DIRECTORY:INTERNAL", root),
                                     ("CMAKE_CACHEFILE_DIR:INTERNAL", build)))


def build_runtime(root, build=None, progress=print):
    root = Path(root).resolve()
    build = Path(build).absolute() if build is not None else root / "build"
    if not (root / "CMakeLists.txt").is_file():
        raise RuntimeError(f"Project CMakeLists.txt is missing: {root}")
    if build.resolve() == root or build.resolve() in root.parents:
        raise RuntimeError("Automatic builds require a separate build directory outside the source files.")
    cmake = shutil.which("cmake")
    if cmake is None:
        raise RuntimeError("CMake is required to build Halo 2. Install CMake, then launch again.")
    reuse = current_cache(root, build)
    fresh = not reuse or not (build / "CMakeCache.txt").is_file()
    if fresh and shutil.which("ninja") is None:
        raise RuntimeError("Ninja is required to build Halo 2. Install Ninja, then launch again.")
    if not reuse:
        backup = build.with_name(build.name + ".previous")
        number = 2
        while backup.exists() or backup.is_symlink():
            backup = build.with_name(build.name + f".previous-{number}")
            number += 1
        build.rename(backup)
        progress(f"Preserved the build from the old location: {backup}")
    build.mkdir(parents=True, exist_ok=True)
    log_path = build / "launcher-build.log"
    configure = [cmake, "-S", str(root), "-B", str(build)]
    if fresh:
        configure += ["-G", "Ninja", "-DCMAKE_BUILD_TYPE=RelWithDebInfo", "-DBUILD_TESTING=OFF"]
    compile_command = [cmake, "--build", str(build), "--target", "halo2_launcher",
                       "halo2_title_launcher", "--parallel"]
    if not os.environ.get("CMAKE_BUILD_PARALLEL_LEVEL"):
        jobs = min(os.cpu_count() or 1, 8)
        try:
            available = os.sysconf("SC_AVPHYS_PAGES") * os.sysconf("SC_PAGE_SIZE")
            jobs = min(jobs, max(1, available // (2 * 1024 ** 3)))
        except (AttributeError, OSError, ValueError):
            pass
        compile_command.append(str(jobs))
    with log_path.open("w") as log:
        for stage, command in (("Configuring Halo 2", configure), ("Building Halo 2", compile_command)):
            progress(stage + "…")
            log.write(stage + "\n" + repr(command) + "\n")
            log.flush()
            with subprocess.Popen(command, cwd=root, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                                  text=True, errors="replace") as process:
                for line in process.stdout:
                    log.write(line)
                    log.flush()
                    progress(line.rstrip())
                status = process.wait()
            if status:
                raise RuntimeError(f"{stage} failed (exit {status}). Full build output: {log_path}")
    progress("Build complete.")


class BuildWindow:
    def __init__(self, window, root, build, operation=build_runtime):
        import tkinter as tk
        from tkinter import ttk
        from setup_assets_ui import CYAN, FIELD, INK, MUTED, PANEL

        self.window, self.result, self.finished = window, 1, False
        self.events = queue.Queue()
        window.title("Halo 2 — Building the game")
        window.configure(background=PANEL)
        width, height = min(920, window.winfo_screenwidth() - 40), min(600, window.winfo_screenheight() - 80)
        window.geometry(f"{width}x{height}")
        window.resizable(False, False)
        window.protocol("WM_DELETE_WINDOW", self.close)
        frame = tk.Frame(window, background=PANEL, padx=24, pady=20)
        frame.pack(fill="both", expand=True)
        frame.columnconfigure(0, weight=1)
        frame.rowconfigure(4, weight=1)
        tk.Label(frame, text="Building Halo 2", background=PANEL, foreground=INK,
                 font=("TkDefaultFont", 22), anchor="w").grid(row=0, column=0, sticky="ew")
        tk.Label(frame, text="Your files are ready. The game will start when the build completes.",
                 background=PANEL, foreground=MUTED, anchor="w", wraplength=width - 48).grid(row=1, column=0, sticky="ew", pady=(8, 16))
        self.status = tk.Label(frame, text="Preparing the build…", background=PANEL,
                               foreground=CYAN, anchor="w", wraplength=width - 48)
        self.status.grid(row=2, column=0, sticky="ew")
        self.bar = ttk.Progressbar(frame, mode="indeterminate")
        self.bar.grid(row=3, column=0, sticky="ew", pady=(10, 16))
        self.bar.start(14)
        output = tk.Frame(frame, background=FIELD)
        output.grid(row=4, column=0, sticky="nsew")
        scrollbar = tk.Scrollbar(output)
        scrollbar.pack(side="right", fill="y")
        self.output = tk.Text(output, background=FIELD, foreground=MUTED, font="TkFixedFont",
                              wrap="word", relief="flat", padx=10, pady=10, height=8, width=1,
                              yscrollcommand=scrollbar.set, state="disabled")
        self.output.pack(fill="both", expand=True)
        scrollbar.configure(command=self.output.yview)
        self.button = tk.Button(frame, text="Building…", state="disabled", command=self.close,
                                background="#a6e4ef", foreground="#092335", relief="flat", padx=18, pady=8)
        self.button.grid(row=5, column=0, sticky="e", pady=(16, 0))

        def progress(message):
            print(message, flush=True)
            self.events.put(("status", message))

        def work():
            try:
                operation(root, build, progress)
                self.events.put(("done", None))
            except Exception as error:
                self.events.put(("error", str(error)))

        threading.Thread(target=work, daemon=True).start()
        window.after(50, self.poll)

    def poll(self):
        try:
            for _ in range(500):
                kind, message = self.events.get_nowait()
                if kind == "done":
                    self.result = 0
                    self.window.destroy()
                    return
                self.output.configure(state="normal")
                self.output.insert("end", message + "\n")
                if int(self.output.index("end-1c").split(".")[0]) > 2000:
                    self.output.delete("1.0", "501.0")
                self.output.see("end")
                self.output.configure(state="disabled")
                if kind == "error":
                    self.finished = True
                    self.bar.stop()
                    self.status.configure(text="Build stopped. Check the output below, then launch again.", foreground="#ffd29b")
                    self.button.configure(text="Close", state="normal")
                    self.button.focus_set()
                    return
                self.status.configure(text=message[:120])
        except queue.Empty:
            pass
        self.window.after(50, self.poll)

    def close(self):
        if self.finished:
            self.window.destroy()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=Path(__file__).resolve().parents[1])
    parser.add_argument("--build-dir", type=Path)
    parser.add_argument("--no-gui", action="store_true")
    args = parser.parse_args()
    if not args.no_gui:
        try:
            import tkinter as tk
        except ImportError:
            pass
        else:
            try:
                window = tk.Tk()
            except tk.TclError:
                pass
            else:
                progress = BuildWindow(window, args.root, args.build_dir)
                window.mainloop()
                return progress.result
    try:
        build_runtime(args.root, args.build_dir, progress=lambda message: print(message, flush=True))
        return 0
    except (OSError, RuntimeError) as error:
        print(f"Halo 2 build: {error}", flush=True)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
