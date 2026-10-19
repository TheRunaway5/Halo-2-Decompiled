"""Install verified prebuilt runtimes from this project's official GitHub releases.

Update never pulls sources or runs a compiler. Releases must publish the
halo2-runtime-update.json contract described by validate_manifest below.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path, PurePosixPath
import platform
import re
import shutil
import stat
import sys
import tempfile
from urllib.error import HTTPError, URLError
from urllib.parse import urlsplit
from urllib.request import Request, urlopen

sys.path.insert(0, str(Path(__file__).resolve().parent))
from packaged_shader_defaults import (readonly_literal_location,
                                     relative_compiled_default, shader_inventory)


REPOSITORY = "TheRunaway5/Halo-2-Decompiled"
RELEASE_API = f"https://api.github.com/repos/{REPOSITORY}/releases/latest"
MANIFEST_ASSET = "halo2-runtime-update.json"
MANIFEST_SCHEMA = "H2_PREBUILT_RUNTIME_UPDATE_V1"
SELECTION_SCHEMA = "H2_PREBUILT_RUNTIME_SELECTION_V1"
RECEIPT = "runtime-manifest.json"
MAX_METADATA = 2 * 1024 * 1024
MAX_FILE = 512 * 1024 * 1024
MAX_TOTAL = 2 * 1024 * 1024 * 1024
LAUNCHERS = {"Halo2-Launcher", "Halo2-Title-Launcher", "onboarding/Halo2-Launcher"}
SIDECARS = {"halo2_native.native-test.json", "halo2_native.frontend-test.json",
            "NATIVE_TEST_VALIDATION.json", "FRONTEND_TEST_VALIDATION.json"}
SLOT_WRAPPER = (
    '#!/bin/sh\nset -eu\n'
    'root=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)\n'
    'cd "$root"\n'
    'exec "$root/lib/ld-linux-x86-64.so.2" --library-path "$root/lib" '
    '"$root/bin/halo2_native.real" "$@"\n').encode()


def sha(path):
    with Path(path).open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest()


def platform_key():
    if platform.system() == "Linux" and platform.machine().lower() in ("x86_64", "amd64"):
        return "linux-x86_64"
    raise RuntimeError(f"No supported prebuilt update platform for {platform.system()} {platform.machine()}.")


def require(condition, message):
    if not condition:
        raise RuntimeError(message)


def safe_path(value):
    require(isinstance(value, str) and value and "\\" not in value and "\0" not in value,
            "Invalid release file path.")
    path = PurePosixPath(value)
    require(not path.is_absolute() and path.as_posix() == value and
            all(part not in (".", "..") for part in path.parts), "Unsafe release file path.")
    return path


def owned(path, root):
    """Reject both existing and dangling aliases before reading or writing."""
    path, root = Path(path), Path(root)
    require(path.is_relative_to(root), "Update path escaped its installation.")
    for candidate in [path, *path.parents]:
        require(not candidate.is_symlink(), "Update paths cannot contain symlinks.")
        if candidate == root:
            break
    return path


def request(url):
    response = urlopen(Request(url, headers={"User-Agent": "Halo2-prebuilt-updater/1",
        "X-GitHub-Api-Version": "2026-03-10",
        "Accept": "application/vnd.github+json" if url.startswith("https://api.github.com/") else "application/octet-stream"}), timeout=30)
    final = urlsplit(response.geturl())
    if final.scheme != "https" or final.hostname not in {
            "api.github.com", "github.com", "release-assets.githubusercontent.com", "objects.githubusercontent.com"}:
        response.close()
        raise RuntimeError("Release download redirected outside GitHub's trusted hosts.")
    return response


def metadata(url):
    with request(url) as response:
        data = response.read(MAX_METADATA + 1)
    require(len(data) <= MAX_METADATA, "Release metadata exceeds its size limit.")
    return data


def asset_url(asset):
    url = asset.get("browser_download_url", "")
    require(isinstance(url, str), "Invalid official release asset URL.")
    parsed = urlsplit(url)
    require(url.startswith(f"https://github.com/{REPOSITORY}/releases/download/") and
            parsed.hostname == "github.com" and not parsed.query and not parsed.fragment and
            parsed.username is None and parsed.password is None, "Release asset is not hosted by the official repository.")
    return url


def asset_table(release):
    assets = release.get("assets")
    require(isinstance(assets, list), "The official release has no asset inventory.")
    result = {}
    for row in assets:
        require(isinstance(row, dict) and isinstance(row.get("name"), str) and row["name"] not in result,
                "Release asset names must be unique.")
        asset_url(row)
        result[row["name"]] = row
    return result


def validate_manifest(manifest, release_id, key, assets):
    require(isinstance(manifest, dict) and manifest.get("schema") == MANIFEST_SCHEMA and manifest.get("release_id") == release_id,
            "Prebuilt manifest does not identify this official release.")
    require(isinstance(manifest.get("platforms"), dict), "Invalid prebuilt platform inventory.")
    target = manifest["platforms"].get(key)
    require(isinstance(target, dict), f"The latest release has no prebuilt binaries for {key}.")
    return validate_target(target, assets)


def validate_target(target, assets=None):
    require(isinstance(target, dict), "Invalid installed platform manifest.")
    require(target.get("layout") == "linux-packaged-slot-v1" and target.get("runtime") == "halo2_native",
            "Unsupported relocatable runtime layout.")
    relative_compiled_default(target.get("compiled_shader_default"))
    files = target.get("files")
    require(isinstance(files, dict) and 100 <= len(files) <= 2048, "Invalid prebuilt file inventory.")
    folded, total = set(), 0
    for name, row in files.items():
        path = safe_path(name)
        require(name in LAUNCHERS | SIDECARS or name == "halo2_native" or name == "bin/halo2_native.real" or
                (len(path.parts) > 1 and path.parts[0] == "lib") or
                (len(path.parts) > 2 and path.parts[:2] == ("bin", "shaders")),
                "Release files may contain only owned binaries, capability metadata, libraries and the preserved shaders.")
        require(name.casefold() not in folded, "Release file paths collide.")
        folded.add(name.casefold())
        require(isinstance(row, dict) and type(row.get("bytes")) is int and 0 < row["bytes"] <= MAX_FILE and
                isinstance(row.get("sha256"), str) and re.fullmatch("[0-9a-f]{64}", row["sha256"]) and
                type(row.get("executable")) is bool, "Invalid release file size, checksum or mode.")
        total += row["bytes"]
        if name in SIDECARS:
            require(row["bytes"] <= MAX_METADATA and not row["executable"], "Capability metadata must be bounded non-executable JSON.")
            if name.endswith("-test.json"):
                require(row["bytes"] <= 65536, "Runtime capability sidecar exceeds 64 KiB.")
        if name.startswith("bin/shaders/"):
            require(not row["executable"], "Preserved shaders cannot be executable update files.")
        if name in LAUNCHERS:
            require(row["executable"], "Release launchers must be executable binaries.")
        require(isinstance(row.get("asset"), str), "Invalid release asset identity.")
        if assets is not None:
            require(row["asset"] in assets, f"The official release is missing the asset for {name}.")
            require(assets[row["asset"]].get("size") == row["bytes"], "Release asset size disagrees with its manifest.")
    require(total <= MAX_TOTAL, "Prebuilt release exceeds its total size limit.")
    require(all(name in files and files[name]["executable"] for name in
                ["halo2_native", "bin/halo2_native.real", "lib/ld-linux-x86-64.so.2"]), "Prebuilt runtime or loader is missing.")
    shader_names = {name.removeprefix("bin/shaders/") for name in files if name.startswith("bin/shaders/")}
    require(len(shader_names) == 97 and {"world.vert.spv", "world.frag.spv", "hud.frag.spv"} <= shader_names,
            "Release must preserve the complete existing 97-file shader bank.")
    return target


def matching(path, row):
    return path.is_file() and not path.is_symlink() and path.stat().st_size == row["bytes"] and sha(path) == row["sha256"]


def verify_slot(slot, receipt, key):
    require(isinstance(receipt, dict) and receipt["schema"] == MANIFEST_SCHEMA and receipt["platform"] == key, "Wrong installed runtime manifest.")
    target = validate_target(receipt["target"])
    files = target["files"]
    for name, row in files.items():
        path = owned(slot / safe_path(name), slot)
        require(matching(path, row), f"Installed runtime file failed verification: {name}")
        require(stat.S_IMODE(path.stat().st_mode) == (0o755 if row["executable"] else 0o644),
                f"Installed runtime file mode changed: {name}")
        require(path.stat().st_nlink == 1, f"Installed runtime file has a shared owner: {name}")
    require({p.relative_to(slot).as_posix() for p in slot.rglob("*") if p.is_file()} == set(files) | {RECEIPT},
            "Installed runtime contains undeclared files.")
    require(not any(p.is_symlink() for p in slot.rglob("*")), "Installed runtime contains aliases.")
    require((slot / "halo2_native").read_bytes() == SLOT_WRAPPER, "Runtime wrapper is not the release-slot wrapper.")
    for name in LAUNCHERS & files.keys():
        with (slot / name).open("rb") as stream:
            header = stream.read(64)
        require(len(header) == 64 and header[:7] == b"\x7fELF\x02\x01\x01" and header[18:20] == b"\x3e\x00",
                "Release launcher is not an ELF64 x86-64 binary.")
    for name in SIDECARS & files.keys():
        require(isinstance(json.loads((slot / name).read_text()), dict), "Invalid capability metadata JSON.")
    payload = (slot / "bin/halo2_native.real").read_bytes()
    readonly_literal_location(payload, relative_compiled_default(target["compiled_shader_default"]).encode())
    require(shader_inventory(slot / "bin/shaders") == {
        name.removeprefix("bin/shaders/"): row["sha256"] for name, row in files.items() if name.startswith("bin/shaders/")},
        "Installed shader inventory changed.")


def selected_runtime(root):
    """Return a fully verified selected runtime, or None; never invoke it."""
    try:
        root = Path(root).resolve()
        selected = read_json(owned(root / "runtime-selection.json", root), 65536)
        key = platform_key()
        require(selected["schema"] == SELECTION_SCHEMA and selected["platform"] == key, "Wrong runtime selection platform.")
        release_id = selected["release_id"]
        require(isinstance(release_id, str) and re.fullmatch("[1-9][0-9]*", release_id), "Invalid selected release identity.")
        relative = f"bin/releases/{release_id}/halo2_native"
        require(selected["runtime"] == relative, "Selected runtime escaped its release owner.")
        slot = owned(root / "bin/releases" / release_id, root)
        receipt_path = owned(slot / RECEIPT, root)
        require(receipt_path.stat().st_size <= MAX_METADATA, "Installed runtime manifest exceeds its size limit.")
        require(sha(receipt_path) == selected["manifest_sha256"], "Installed runtime manifest changed.")
        receipt = read_json(receipt_path, MAX_METADATA)
        require(receipt["release_id"] == release_id, "Wrong installed release identity.")
        verify_slot(slot, receipt, key)
        runtime = slot / "halo2_native"
        require(sha(runtime) == selected["runtime_sha256"], "Selected runtime checksum changed.")
        return runtime
    except (OSError, ValueError, KeyError, TypeError, RuntimeError):
        return None


def read_json(path, limit):
    with Path(path).open("rb") as stream:
        data = stream.read(limit + 1)
    require(len(data) <= limit, "Installed runtime metadata exceeds its size limit.")
    return json.loads(data)


def existing_shaders(root, build, files):
    expected = {name.removeprefix("bin/shaders/"): row["sha256"] for name, row in files.items() if name.startswith("bin/shaders/")}
    candidates = [root / "bin/shaders"]
    if build is not None:
        candidates.append(Path(build).absolute() / "shaders")
    candidates.extend([root / "build/shaders", root / "build-multiplayer/shaders"])
    previous = selected_runtime(root)
    if previous:
        candidates.insert(0, previous.parent / "bin/shaders")
    for directory in candidates:
        try:
            if shader_inventory(directory) == expected:
                return directory
        except (OSError, ValueError):
            pass
    raise RuntimeError("Update requires the same existing 97 shaders. The release differs from the installed shader bank; nothing was replaced.")


def selected_launcher(root, name="Halo2-Launcher"):
    require(name in LAUNCHERS, "Unknown release launcher name.")
    runtime = selected_runtime(root)
    if runtime is None:
        return None
    candidate = runtime.parent / name
    return candidate if candidate.is_file() else None


def download(asset, destination, row):
    digest, size = hashlib.sha256(), 0
    with request(asset_url(asset)) as response, destination.open("xb") as stream:
        while chunk := response.read(1024 * 1024):
            size += len(chunk)
            require(size <= row["bytes"], "Downloaded release file exceeds its declared size.")
            digest.update(chunk)
            stream.write(chunk)
    require(size == row["bytes"] and digest.hexdigest() == row["sha256"], "Downloaded release file checksum failed.")


def update_runtime(root, build=None, progress=print):
    root = Path(root).resolve()
    key = platform_key()
    progress(f"Checking official prebuilt releases for {key}…")
    try:
        release = json.loads(metadata(RELEASE_API))
    except HTTPError as error:
        error.close()
        if error.code == 404:
            raise RuntimeError("No prebuilt release has been published at https://github.com/" + REPOSITORY + "/releases. Your installed runtime was kept.") from error
        raise RuntimeError(f"Could not check official prebuilt releases (HTTP {error.code}). Your installed runtime was kept.") from error
    except (URLError, OSError, ValueError) as error:
        raise RuntimeError("Could not read the official prebuilt release feed. Your installed runtime was kept.") from error
    require(isinstance(release, dict) and not release.get("draft") and not release.get("prerelease") and type(release.get("id")) is int and release["id"] > 0,
            "The official feed did not return a stable release.")
    release_id, assets = str(release["id"]), asset_table(release)
    require(MANIFEST_ASSET in assets, f"The latest release is missing {MANIFEST_ASSET}. Your installed runtime was kept.")
    manifest_asset = assets[MANIFEST_ASSET]
    manifest_bytes = metadata(asset_url(manifest_asset))
    require(manifest_asset.get("size") == len(manifest_bytes), "Release manifest size disagrees with GitHub's inventory.")
    require(manifest_asset.get("digest") == "sha256:" + hashlib.sha256(manifest_bytes).hexdigest(),
            "The release manifest has no matching GitHub SHA-256 digest.")
    target = validate_manifest(json.loads(manifest_bytes), release_id, key, assets)
    shader_source = existing_shaders(root, build, target["files"])
    releases = owned(root / "bin/releases", root)
    selected_path = owned(root / "runtime-selection.json", root)
    slot = owned(releases / release_id, root)
    receipt = dict(schema=MANIFEST_SCHEMA, release_id=release_id, platform=key,
        source=RELEASE_API, manifest_sha256=hashlib.sha256(manifest_bytes).hexdigest(), target=target)
    receipt_bytes = (json.dumps(receipt, indent=2, sort_keys=True) + "\n").encode()
    previous = selected_runtime(root)
    releases.mkdir(parents=True, exist_ok=True)
    created = False
    try:
        if slot.exists():
            require(owned(slot / RECEIPT, root).read_bytes() == receipt_bytes, "This immutable release slot already contains different bytes.")
            verify_slot(slot, receipt, key)
        else:
            with tempfile.TemporaryDirectory(prefix=".staging-", dir=releases) as temporary:
                stage = Path(temporary)
                for name, row in target["files"].items():
                    destination = stage / name
                    destination.parent.mkdir(parents=True, exist_ok=True)
                    sources = [shader_source / name.removeprefix("bin/shaders/")] if name.startswith("bin/shaders/") else []
                    if previous:
                        sources.append(previous.parent / name)
                    source = next((path for path in sources if matching(path, row)), None)
                    if source is not None:
                        shutil.copyfile(source, destination)
                    else:
                        progress(f"Downloading {name}…")
                        download(assets[row["asset"]], destination, row)
                    destination.chmod(0o755 if row["executable"] else 0o644)
                (stage / RECEIPT).write_bytes(receipt_bytes)
                verify_slot(stage, receipt, key)
                # The complete slot is installed first; only the tiny selection
                # is the activation point. Previous release slots are untouched.
                stage.rename(slot)
                created = True
        selection = dict(schema=SELECTION_SCHEMA, runtime=f"bin/releases/{release_id}/halo2_native",
            runtime_sha256=target["files"]["halo2_native"]["sha256"], release_id=release_id,
            platform=key, manifest_sha256=hashlib.sha256(receipt_bytes).hexdigest())
        with tempfile.NamedTemporaryFile(prefix=".runtime-selection-", dir=root, delete=False) as stream:
            pending = Path(stream.name)
            try:
                stream.write((json.dumps(selection, indent=2) + "\n").encode())
                stream.flush()
                os.fsync(stream.fileno())
            except BaseException:
                pending.unlink(missing_ok=True)
                raise
        try:
            os.replace(pending, selected_path)
        finally:
            pending.unlink(missing_ok=True)
    except BaseException:
        # A cancellation can arrive immediately after replace completed. Keep
        # the verified slot if the new selection is already the visible one.
        activated = False
        if created:
            try:
                activated = json.loads(selected_path.read_text()) == selection
            except (OSError, ValueError, UnboundLocalError):
                pass
        if created and not activated:
            shutil.rmtree(slot)
        raise
    progress("Prebuilt update complete. Select Play to start Halo 2; no source build was needed.")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=Path(__file__).resolve().parents[1])
    parser.add_argument("--build-dir", type=Path)
    modes = parser.add_mutually_exclusive_group()
    modes.add_argument("--selected-runtime", action="store_true")
    modes.add_argument("--verify-selection", action="store_true")
    modes.add_argument("--selected-launcher", choices=sorted(LAUNCHERS))
    args = parser.parse_args()
    if args.selected_runtime or args.verify_selection or args.selected_launcher:
        selected = selected_launcher(args.root, args.selected_launcher) if args.selected_launcher else selected_runtime(args.root)
        if selected is None:
            return 1
        if args.selected_runtime or args.selected_launcher:
            print(selected)
        return 0
    try:
        update_runtime(args.root, args.build_dir, progress=lambda message: print(message, flush=True))
        return 0
    except (OSError, RuntimeError, ValueError, KeyError, TypeError, URLError) as error:
        print(f"Halo 2 update: {error}", flush=True)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
