"""Fast-forward this checkout and rebuild its runtime without replacing local data."""
import argparse
import os
from pathlib import Path
import shutil
import subprocess
import sys


def update_runtime(root, build=None, progress=print):
    root = Path(root).resolve()
    build = Path(build).absolute() if build is not None else root / "build"
    git = shutil.which("git")
    if git is None:
        raise RuntimeError("Git is required to update Halo 2. Install Git and try again.")
    environment = {**os.environ, "GIT_TERMINAL_PROMPT": "0", "GIT_MERGE_AUTOEDIT": "no"}

    def inspect(*arguments):
        result = subprocess.run([git, "-C", str(root), *arguments], env=environment,
                                stdin=subprocess.DEVNULL, capture_output=True, text=True)
        if result.returncode:
            raise RuntimeError(result.stderr.strip() or "Could not inspect the Git checkout.")
        return result.stdout.strip()

    if Path(inspect("rev-parse", "--show-toplevel")).resolve() != root:
        raise RuntimeError("Update requires a Git checkout of the Halo 2 project.")
    if inspect("status", "--porcelain", "--untracked-files=no"):
        raise RuntimeError("Update stopped because this checkout has local changes. "
                           "Commit or save those changes before updating; your files were kept.")
    try:
        inspect("rev-parse", "--abbrev-ref", "--symbolic-full-name", "@{upstream}")
    except RuntimeError as error:
        raise RuntimeError("This branch has no upstream. Configure its Git tracking branch before updating.") from error

    log_path = root / "launcher-update.log"
    with log_path.open("w") as log:
        def report(message):
            log.write(message + "\n")
            log.flush()
            progress(message)

        def run(command, stage):
            report(stage + "…")
            with subprocess.Popen(command, cwd=root, env=environment, stdin=subprocess.DEVNULL,
                                  stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                                  text=True, errors="replace") as process:
                for line in process.stdout:
                    report(line.rstrip())
                status = process.wait()
            if status:
                raise RuntimeError(f"{stage} failed (exit {status}). See {log_path}")

        run([git, "-c", "credential.interactive=never", "pull", "--ff-only"], "Downloading updates")
        pending = root / "launcher-update.pending"
        pending.write_text("The updated sources need a successful runtime build.\n")
        # Run the newly pulled helper, rather than a module imported before the
        # update. Rebuild even when Git is current: a prior pull may be unbuilt.
        run([sys.executable, str(root / "tools/build_runtime.py"), "--root", str(root),
             "--build-dir", str(build), "--no-gui"], "Building the updated game")
        pending.unlink(missing_ok=True)
        report("Update complete. Select Play to start Halo 2.")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=Path(__file__).resolve().parents[1])
    parser.add_argument("--build-dir", type=Path)
    args = parser.parse_args()
    try:
        update_runtime(args.root, args.build_dir, progress=lambda message: print(message, flush=True))
        return 0
    except (OSError, RuntimeError) as error:
        print(f"Halo 2 update: {error}", flush=True)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
