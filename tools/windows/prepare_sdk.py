#!/usr/bin/env python3
"""Install the locked native Windows dependency SDK used by Linux cross builds."""
import argparse
import concurrent.futures
import hashlib
import json
from pathlib import Path
import shutil
import subprocess
import tarfile
import tempfile
import urllib.request


def prepare(root, sdk, *, lock_path=None):
    lock_path = lock_path or root / "tools/windows/sdk-lock.json"
    lock_bytes = lock_path.read_bytes()
    lock = json.loads(lock_bytes)
    identity = hashlib.sha256(lock_bytes).hexdigest()
    receipt = sdk / "halo2-sdk.json"
    if receipt.is_file() and json.loads(receipt.read_text()).get("lock_sha256") == identity:
        return
    sdk.parent.mkdir(parents=True, exist_ok=True)
    cache = sdk.parent / "cache"
    cache.mkdir(exist_ok=True)

    def download(row):
        path = cache / row["filename"]
        if path.is_file() and hashlib.sha256(path.read_bytes()).hexdigest() == row["sha256"]:
            return path
        print("Windows SDK: " + row["filename"], flush=True)
        with urllib.request.urlopen(lock["source"] + row["filename"], timeout=120) as response:
            with tempfile.NamedTemporaryFile(dir=cache, delete=False) as stream:
                temporary = Path(stream.name)
                try:
                    shutil.copyfileobj(response, stream)
                except BaseException:
                    temporary.unlink(missing_ok=True)
                    raise
        if hashlib.sha256(temporary.read_bytes()).hexdigest() != row["sha256"]:
            temporary.unlink()
            raise RuntimeError("Windows SDK checksum mismatch: " + row["filename"])
        temporary.replace(path)
        return path

    with concurrent.futures.ThreadPoolExecutor(max_workers=8) as pool:
        archives = list(pool.map(download, lock["packages"]))
    # Temporary extraction cannot leave a partially installed SDK on failure.
    with tempfile.TemporaryDirectory(prefix="halo2-sdk-", dir=sdk.parent) as owned:
        staging = Path(owned)
        for archive in archives:
            with subprocess.Popen(["zstd", "-dc", str(archive)], stdout=subprocess.PIPE) as process:
                with tarfile.open(fileobj=process.stdout, mode="r|") as package:
                    for member in package:
                        if member.name.startswith("mingw64/"):
                            package.extract(member, staging, filter="data")
                # tar stops at its end markers; drain archive padding before
                # waiting so the decompressor cannot block on a full pipe.
                while process.stdout.read(1024 * 1024):
                    pass
                if process.wait():
                    raise RuntimeError("Cannot extract " + str(archive))
        extracted = staging / "mingw64"
        (extracted / "halo2-sdk.json").write_text(json.dumps({
            "lock_sha256": identity, "source": lock["source"],
            "packages": lock["packages"],
        }, indent=2) + "\n")
        if sdk.exists():
            raise RuntimeError("SDK has a different lock; preserve it and choose a fresh --sdk path: " + str(sdk))
        extracted.rename(sdk)
    # The installed SDK is the reusable build input. Download archives are
    # disposable duplicates once installation and its receipt have succeeded.
    for archive in archives:
        archive.unlink(missing_ok=True)
    try:
        cache.rmdir()
    except OSError:
        pass
    print("Windows SDK ready: " + str(sdk), flush=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=Path(__file__).resolve().parents[2])
    parser.add_argument("--sdk", type=Path)
    args = parser.parse_args()
    root = args.root.resolve()
    prepare(root, args.sdk.resolve() if args.sdk else root / ".tools/windows-sdk/mingw64")


if __name__ == "__main__":
    main()
