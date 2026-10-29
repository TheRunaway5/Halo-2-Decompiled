#!/usr/bin/env python3
"""Start an unbounded MCC split-screen match through real X11 menu input.

Run through tools/run_test.py under an X11 display. No captures or finite-frame
diagnostic options are supplied; success requires actual multiplayer frames and
a clean exit after WM_DELETE_WINDOW.
"""
import argparse
import ctypes as C
import ctypes.util
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import tempfile
import time


EXACT_CRASH = "Live shadow GPU reference requires the actual640x480 drawable window"
CACHE_DIRECTORIES = {
    "XDG_CONFIG_HOME": "config", "XDG_DATA_HOME": "data", "XDG_CACHE_HOME": "cache",
    "HALO2_PIPELINE_CACHE_DIRECTORY": "pipelines", "MESA_SHADER_CACHE_DIR": "mesa",
    "__GL_SHADER_DISK_CACHE_PATH": "nvidia",
}


def sha256(path):
    with Path(path).open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest()


class X11Input:
    def __init__(self):
        libraries = {name: ctypes.util.find_library(name) for name in ("X11", "Xtst")}
        if not all(libraries.values()):
            raise RuntimeError("X11 and Xtst shared libraries are required")
        self.x = C.CDLL(libraries["X11"])
        self.xt = C.CDLL(libraries["Xtst"])
        self.display = None
        self.window = None
        signatures = {
            "XOpenDisplay": ([C.c_char_p], C.c_void_p),
            "XDefaultRootWindow": ([C.c_void_p], C.c_ulong),
            "XQueryTree": ([C.c_void_p, C.c_ulong, C.POINTER(C.c_ulong), C.POINTER(C.c_ulong),
                            C.POINTER(C.POINTER(C.c_ulong)), C.POINTER(C.c_uint)], C.c_int),
            "XFetchName": ([C.c_void_p, C.c_ulong, C.POINTER(C.c_char_p)], C.c_int),
            "XFree": ([C.c_void_p], C.c_int),
            "XStringToKeysym": ([C.c_char_p], C.c_ulong),
            "XKeysymToKeycode": ([C.c_void_p, C.c_ulong], C.c_ubyte),
            "XSetInputFocus": ([C.c_void_p, C.c_ulong, C.c_int, C.c_ulong], C.c_int),
            "XInternAtom": ([C.c_void_p, C.c_char_p, C.c_int], C.c_ulong),
            "XFlush": ([C.c_void_p], C.c_int),
            "XCloseDisplay": ([C.c_void_p], C.c_int),
        }
        for name, (arguments, result) in signatures.items():
            function = getattr(self.x, name)
            function.argtypes, function.restype = arguments, result
        self.xt.XTestFakeKeyEvent.argtypes = [C.c_void_p, C.c_uint, C.c_int, C.c_ulong]
        self.xt.XTestFakeKeyEvent.restype = C.c_int
        self.display = self.x.XOpenDisplay(None)
        if not self.display:
            raise RuntimeError("An X11 display is required; use xvfb-run")

    def focus(self):
        if self.window is None:
            returned, parent, count = C.c_ulong(), C.c_ulong(), C.c_uint()
            children = C.POINTER(C.c_ulong)()
            if not self.x.XQueryTree(self.display, self.x.XDefaultRootWindow(self.display),
                                    C.byref(returned), C.byref(parent), C.byref(children), C.byref(count)):
                raise RuntimeError("Cannot enumerate the private X11 display")
            try:
                for index in range(count.value):
                    name = C.c_char_p()
                    if self.x.XFetchName(self.display, children[index], C.byref(name)) and name.value:
                        title = name.value.decode(errors="replace")
                        self.x.XFree(name)
                        if "halo" in title.lower():
                            self.window = children[index]
                            break
            finally:
                if children:
                    self.x.XFree(children)
            if self.window is None:
                raise RuntimeError("The actual native Halo window was not found")
        self.x.XSetInputFocus(self.display, self.window, 2, 0)
        self.x.XFlush(self.display)

    def key(self, name):
        self.focus()
        code = self.x.XKeysymToKeycode(self.display, self.x.XStringToKeysym(name.encode()))
        if not code:
            raise RuntimeError("X11 has no keycode for " + name)
        if not self.xt.XTestFakeKeyEvent(self.display, code, 1, 0):
            raise RuntimeError("XTest could not press " + name)
        self.x.XFlush(self.display)
        time.sleep(.07)
        if not self.xt.XTestFakeKeyEvent(self.display, code, 0, 0):
            raise RuntimeError("XTest could not release " + name)
        self.x.XFlush(self.display)
        time.sleep(.12)

    def close_window(self):
        class Data(C.Union):
            _fields_ = [("bytes", C.c_char * 20), ("shorts", C.c_short * 10), ("longs", C.c_long * 5)]

        class ClientMessage(C.Structure):
            _fields_ = [("type", C.c_int), ("serial", C.c_ulong), ("send_event", C.c_int),
                        ("display", C.c_void_p), ("window", C.c_ulong), ("message_type", C.c_ulong),
                        ("format", C.c_int), ("data", Data)]

        class Event(C.Union):
            _fields_ = [("client", ClientMessage), ("padding", C.c_long * 24)]

        self.x.XSendEvent.argtypes = [C.c_void_p, C.c_ulong, C.c_int, C.c_long, C.POINTER(Event)]
        self.x.XSendEvent.restype = C.c_int
        event = Event()
        event.client.type, event.client.send_event, event.client.format = 33, 1, 32
        event.client.display, event.client.window = self.display, self.window
        event.client.message_type = self.x.XInternAtom(self.display, b"WM_PROTOCOLS", 0)
        event.client.data.longs[0] = self.x.XInternAtom(self.display, b"WM_DELETE_WINDOW", 0)
        if not self.x.XSendEvent(self.display, self.window, 0, 0, C.byref(event)):
            raise RuntimeError("X11 rejected WM_DELETE_WINDOW")
        self.x.XFlush(self.display)

    def close(self):
        if self.display:
            self.x.XCloseDisplay(self.display)
            self.display = None


def exercise(args, scratch, report):
    env = os.environ.copy()
    for key, relative in CACHE_DIRECTORIES.items():
        env[key] = str(scratch / relative)
    env.update(SDL_VIDEODRIVER="x11", SDL_AUDIODRIVER="dummy")
    stdbuf = shutil.which("stdbuf")
    if not stdbuf:
        raise RuntimeError("stdbuf is required for observable native menu progress")
    command = [stdbuf, "-oL", "-eL", str(args.runtime), "--menu", "--menu-maps", str(args.maps),
               "--skip-intro", "--viewport", "1280", "720", "--mute"]
    report.update(command=command, cache_directories={key: env[key] for key in CACHE_DIRECTORIES})
    log = scratch / "runtime.log"
    started = time.monotonic()
    deadline = started + 210
    process = keyboard = None

    def text():
        return log.read_text(errors="replace") if log.exists() else ""

    def wait_for(pattern, seconds=45):
        until = min(deadline, time.monotonic() + seconds)
        while time.monotonic() < until:
            if re.search(pattern, text()):
                return
            if process.poll() is not None:
                raise RuntimeError("Native process exited before " + pattern)
            time.sleep(.1)
        raise RuntimeError("Timed out awaiting " + pattern)

    def key(name):
        if process.poll() is not None:
            raise RuntimeError("Native process exited before key " + name)
        keyboard.key(name)
        report["keys"].append(name)

    try:
        keyboard = X11Input()
        with log.open("w") as stream:
            process = subprocess.Popen(command, cwd=args.root, env=env, stdout=stream, stderr=subprocess.STDOUT)
            wait_for(r"Menu ready screen=9")
            key("Return"); wait_for(r"Menu screen 10\b")
            key("Return"); wait_for(r"Menu screen 167\b")
            for letter in "startup":
                key(letter)
            key("Return"); wait_for(r"Menu screen 38\b")
            key("Escape"); wait_for(r"Menu screen 6\b")
            key("Down"); key("Down"); key("Return"); wait_for(r"Menu screen 30\b")
            key("Return"); wait_for(r"Menu screen 14\b")
            key("Return"); wait_for(r"actual offline lobby countdown started")
            for _ in range(10):
                if process.poll() is not None or "actual native world running" in text():
                    break
                key("Return")
            wait_for(r"Multiplayer GPU:", 90)
            render_until = min(deadline, time.monotonic() + 10)
            while time.monotonic() < render_until:
                if process.poll() is not None:
                    raise RuntimeError("Native process exited before window-manager close")
                time.sleep(.1)
            keyboard.close_window()
            report["wm_delete_window_sent"] = True
            process.wait(timeout=min(15, max(.1, deadline - time.monotonic())))
    except Exception as error:
        report["harness_error"] = str(error)
    finally:
        if process is not None:
            if process.poll() is None:
                process.terminate()
                try:
                    process.wait(timeout=5)
                except subprocess.TimeoutExpired:
                    process.kill()
                    process.wait()
            report["exit_code"] = process.returncode
        if keyboard is not None:
            keyboard.close()
        report["elapsed_seconds"] = round(time.monotonic() - started, 3)
        data = text()
        report["exact_crash"] = EXACT_CRASH in data
        report["screens"] = [int(value) for value in re.findall(r"Menu (?:ready screen=|screen )(\d+)", data)]
        frames = re.search(r"Multiplayer frames=(\d+) ticks=(\d+) players=(\d+)", data)
        report.update(frames=int(frames[1]) if frames else 0, ticks=int(frames[2]) if frames else 0,
                      players=int(frames[3]) if frames else 0)
        report["checks"] = {
            "fresh_profile_path": report["screens"][:7] == [9, 10, 167, 38, 6, 30, 14],
            "actual_offline_countdown": "actual offline lobby countdown started" in data,
            "actual_native_handoff": "actual native world running" in data,
            "multiplayer_renderer_ready": "Multiplayer GPU:" in data,
            "actual_multiplayer_frames": report["frames"] > 0 and report["players"] == 1,
            "normal_window_close": report["wm_delete_window_sent"] and report.get("exit_code") == 0,
            "no_drawable_reference_crash": not report["exact_crash"],
        }
        report["passed"] = all(report["checks"].values()) and "harness_error" not in report
        report.update(log_bytes=log.stat().st_size if log.exists() else 0,
                      log_sha256=sha256(log) if log.exists() else None, log_tail=data[-6000:])
        args.report.with_suffix(".log").write_text(data[-131072:])


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("root", "runtime", "maps", "report"):
        parser.add_argument("--" + name, type=Path, required=True)
    args = parser.parse_args()
    if "HALO2_TEST_SCRATCH" not in os.environ:
        parser.error("Run through tools/run_test.py for owned temporary storage")
    args.root, args.runtime, args.maps, args.report = (getattr(args, name).resolve() for name in
                                                    ("root", "runtime", "maps", "report"))
    args.report.parent.mkdir(parents=True, exist_ok=True)
    report = dict(passed=False, scope=__doc__, runtime=str(args.runtime), maps=str(args.maps),
                  viewport=[1280, 720], keys=[], wm_delete_window_sent=False, retained_large_artifacts=[])
    try:
        if not args.runtime.is_file() or not os.access(args.runtime, os.X_OK):
            raise RuntimeError("An executable native runtime is required")
        if not (args.maps / "mainmenu.map").is_file():
            raise RuntimeError("MCC menu maps are required")
        report.update(runtime_sha256=sha256(args.runtime), test_sha256=sha256(__file__))
        with tempfile.TemporaryDirectory(prefix="split-menu-", dir=os.environ["HALO2_TEST_SCRATCH"]) as directory:
            report["scratch"] = directory
            exercise(args, Path(directory), report)
        report["scratch_removed"] = not Path(directory).exists()
    except Exception as error:
        report["harness_error"] = str(error)
    report["passed"] = report["passed"] and report.get("scratch_removed", False) and "harness_error" not in report
    args.report.write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps({key: value for key, value in report.items() if key not in
                      ("log_tail", "keys", "command", "scope", "cache_directories")}))
    return 0 if report["passed"] else 1


if __name__ == "__main__":
    raise SystemExit(main())
