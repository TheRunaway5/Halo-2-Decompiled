"""Launch a pinned local sandbox test package with the owned Xbox inputs."""
import argparse
import datetime
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
import uuid


def digest(path):
    with Path(path).open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest()


def installation(path):
    config = json.loads(path.read_text())
    if config.get("schema") != "HALO2_SANDBOX_TEST_V1":
        raise RuntimeError("Unrecognized sandbox test package")
    for name in ("runtime", "gameplay_xbe"):
        item = config[name]
        if digest(item["path"]) != item["sha256"]:
            raise RuntimeError(f"The test build's {name} changed; rebuild this package.")
    for relative, expected in config["shaders"].items():
        if digest(path.parent / relative) != expected:
            raise RuntimeError("A packaged test shader changed; rebuild this package.")
    if not config["maps"] or "lockout" not in config["maps"]:
        raise RuntimeError("The owned Lockout test map is missing")
    return config


def command(config, args):
    selected = config["maps"].get(args.map)
    if selected is None:
        raise RuntimeError("Select an installed Xbox multiplayer map")
    if digest(selected["path"]) != selected["sha256"]:
        raise RuntimeError("The selected Xbox test map changed")
    result = [config["runtime"]["path"], "--multiplayer", "--map", selected["path"],
              "--shared-maps", config["shared_maps"], "--gameplay-xbe", config["gameplay_xbe"]["path"],
              "--mp-source-object-world", str(Path(config["state_directory"]) / "source-savegames"),
              "--local-players", str(args.local_players), "--mp-starting-weapon", "5",
              "--viewport", str(args.width), str(args.height), "--hud-ui-assets", ""]
    if args.mute:
        result.append("--mute")
    if args.hidden:
        result.append("--hidden")
    if args.frames:
        result.extend(("--frames", str(args.frames)))
    if args.screenshot:
        result.extend(("--screenshot", str(args.screenshot.resolve())))
    return result


def run(config, args):
    argv = command(config, args)
    state = Path(config["state_directory"])
    state.mkdir(parents=True, exist_ok=True)
    (state / "source-savegames").mkdir(exist_ok=True)
    identity = datetime.datetime.now(datetime.timezone.utc).strftime("%Y%m%dT%H%M%SZ") + "-" + uuid.uuid4().hex[:8]
    log = state / (identity + ".log")
    print(f"Sandbox test build: {config['label']}\nRuntime: {argv[0]}\nLog: {log}", flush=True)
    record = {"label": config["label"], "command": argv, "runtime_sha256": digest(argv[0]),
              "map": args.map, "local_players": args.local_players, "log": str(log)}
    with log.open("w") as output:
        output.write(json.dumps(record) + "\n")
        output.flush()
        process = subprocess.Popen(argv, cwd=config["project"], stdout=output, stderr=subprocess.STDOUT)
        record["pid"] = process.pid
        # Record the inode actually executed, even if a later build replaces a path.
        executed = Path(f"/proc/{process.pid}/exe")
        try:
            record["executed_runtime_sha256"] = digest(executed)
        except FileNotFoundError:
            pass
        record["exit_code"] = process.wait()
    (state / (identity + ".json")).write_text(json.dumps(record, indent=2) + "\n")
    (state / "latest-run.json").write_text(json.dumps(record, indent=2) + "\n")
    if record["exit_code"] and args.report_errors:
        show_error("The test build stopped.\n\n" + log.read_text()[-2000:] + "\n\nFull log: " + str(log))
    return record["exit_code"]


def show_error(message):
    try:
        import tkinter as tk
        from tkinter import messagebox
        window = tk.Tk()
        window.withdraw()
        messagebox.showerror("Halo 2 sandbox test", message, parent=window)
        window.destroy()
    except Exception:
        pass  # The original error remains on stderr and in the run log.


def show(config, args):
    import tkinter as tk
    from tkinter import ttk, messagebox
    window = tk.Tk()
    window.title("Halo 2 — Sandbox test build")
    window.configure(background="#061522")
    window.resizable(False, False)
    panel = tk.Frame(window, background="#061522", padx=24, pady=22)
    panel.pack(fill="both", expand=True)

    def label(text, row, size=12, color="#94b5c8"):
        item = tk.Label(panel, text=text, background="#061522", foreground=color,
                        font=("DejaVu Sans", size), justify="left", anchor="w", wraplength=550)
        item.grid(row=row, column=0, columnspan=2, sticky="w", pady=(0, 12))
        return item

    label("Halo 2 · Sandbox test", 0, 20, "#e4f5fb")
    label(config["label"] + "\nExperimental build. Full Xbox TU1.5 parity is still in progress.", 1)
    tk.Label(panel, text="Map", bg="#061522", fg="#e4f5fb").grid(row=2, column=0, sticky="w", pady=8)
    map_name = tk.StringVar(value=args.map)
    maps = ttk.Combobox(panel, textvariable=map_name, values=sorted(config["maps"]), state="readonly", width=35)
    maps.grid(row=2, column=1, sticky="ew", pady=8)
    tk.Label(panel, text="Local players", bg="#061522", fg="#e4f5fb").grid(row=3, column=0, sticky="w", pady=8)
    players = tk.StringVar(value=str(args.local_players))
    ttk.Combobox(panel, textvariable=players, values=("1", "2", "3", "4"), state="readonly", width=35).grid(row=3, column=1, sticky="ew", pady=8)
    muted = tk.BooleanVar(value=args.mute)
    ttk.Checkbutton(panel, text="Mute audio", variable=muted).grid(row=4, column=1, sticky="w", pady=(8, 16))
    label("Keyboard and mouse control player 1. Connect controllers for split screen.\nClick the game to capture the mouse; Escape releases it, then exits.", 5)
    label("Try movement, pickups, firing/reload, melee, grenades and respawn.\nSome sandbox systems and presentation are still unfinished.", 6)
    actions = tk.Frame(panel, bg="#061522")
    actions.grid(row=7, column=0, columnspan=2, sticky="e")
    ttk.Button(actions, text="Close", command=window.destroy).pack(side="left", padx=(0, 12))

    def play():
        argv = [sys.executable, str(Path(__file__).resolve()), "--installation", str(args.installation.resolve()),
                "--no-ui", "--report-errors", "--map", map_name.get(), "--local-players", players.get(),
                "--width", str(args.width), "--height", str(args.height)]
        if muted.get():
            argv.append("--mute")
        try:
            subprocess.Popen(argv, cwd=config["project"], start_new_session=True)
        except OSError as error:
            messagebox.showerror("Could not start test build", str(error), parent=window)
            return
        window.destroy()

    ttk.Button(actions, text="Play test build", command=play).pack(side="left")
    window.bind("<Escape>", lambda event: window.destroy())
    maps.focus_set()
    window.mainloop()
    return 0


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--installation", type=Path, required=True)
    parser.add_argument("--launcher-info", action="store_true")
    parser.add_argument("--no-ui", action="store_true")
    parser.add_argument("--report-errors", action="store_true", help=argparse.SUPPRESS)
    parser.add_argument("--map", default="lockout")
    parser.add_argument("--local-players", type=int, choices=(1, 2, 3, 4), default=1)
    parser.add_argument("--width", type=int, default=1280)
    parser.add_argument("--height", type=int, default=720)
    parser.add_argument("--mute", action="store_true")
    parser.add_argument("--hidden", action="store_true")
    parser.add_argument("--frames", type=int, default=0)
    parser.add_argument("--screenshot", type=Path)
    args = parser.parse_args()
    try:
        config = installation(args.installation)
        if args.launcher_info:
            print(json.dumps(config, indent=2))
            return 0
        if not 320 <= args.width <= 7680 or not 240 <= args.height <= 4320 or args.frames < 0:
            raise RuntimeError("Invalid test window size or frame count")
        return run(config, args) if args.no_ui else show(config, args)
    except Exception as error:
        print(f"Halo 2 sandbox test: {error}", file=sys.stderr)
        if args.report_errors:
            show_error(str(error))
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
