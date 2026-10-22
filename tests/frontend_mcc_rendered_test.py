"""Bounded real Vulkan ONLINE captures from one explicitly admitted MCC cache.

Run through tools/run_test.py. Raw images, native service state, logs, display
and caches are disposable. Only a small contact JPEG and JSON receipt enter
the explicitly requested build report directory. No import/install/selection.
"""
import argparse
from contextlib import contextmanager
import hashlib
import io
import json
import os
from pathlib import Path
import re
import select
import shutil
import signal
import subprocess
import sys
import tempfile
import time

from helpers.mcc_online_ui import metadata, native_prompt_fonts, owned, ui_arguments

ASPECTS = ((640, 480), (1280, 720), (1280, 800), (1920, 1080),
           (2560, 1080), (3440, 1440), (3840, 1080), (2560, 1437), (1001, 601))
LIMIT = 1024 * 1024
SCENES = {
    "clans": (("clan-roster-scroll", ("clan-roster-scroll",)),),
    "parties": (("party-full-", ("party-full-members", "party-full-lobby")),),
    "social": (("friends-long-name-scroll", ("friends-long-name-scroll",)),
               ("friends-game-invitation-long-name", ("friends-game-invitation-long-name",
                                                    "friends-game-invitation-long-name-options"))),
}


def require(value, message):
    if not value:
        raise ValueError(message)


def digest(path):
    with Path(path).open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest()


def identity(path):
    path = Path(path)
    require(path.is_file(), "Missing capture input: " + str(path))
    return {"path": str(path), "bytes": path.stat().st_size, "sha256": digest(path)}


def rendered_input_identities(root, cache):
    """The exact source/cache witnesses retained before actual rendered cases."""
    source_names = ("tests/frontend_mcc_rendered_test.py", "tests/helpers/mcc_online_ui.py",
                    "tests/frontend_clans_http_test.py", "tests/frontend_parties_http_test.py",
                    "tests/frontend_online_tests.cpp", "src/ui/frontend_painter.cpp", "src/rendering/renderer/renderer.cpp",
                    "resources/frontend/mcc-native-layout.json", "tools/data/gameplay-font-reference.json")
    pins = {name: identity(root / name) for name in source_names}
    cache_pins = {name: identity(owned(cache, name)) for name in (
        "provenance.json", "ui/mcc-menu-assets.json", "presentation/ui/native-layout.json",
        "halo2/h2_fonts/font_table.txt")}
    provenance = metadata(owned(cache, "provenance.json"))
    menu = metadata(owned(cache, "ui/mcc-menu-assets.json"))
    # Share the genuine native-font validator with ui_arguments. Its fixed
    # source profile, selector table, copied-source records and pixel hashes
    # authenticate these owners; retained authored prompt banks are ignored.
    fonts = {}
    native_prompt_fonts(root, cache, provenance, menu.get("fonts"), fonts)
    for name, (size, sha256) in sorted(fonts.items()):
        row = identity(owned(cache, name))
        require(row["bytes"] == size and row["sha256"] == sha256,
                "Native MCC font changed during capture input selection: " + name)
        cache_pins[name] = row
    require(all(identity(row["path"]) == row for row in (*pins.values(), *cache_pins.values())),
            "Capture source/font/provenance inputs changed during selection")
    return pins, cache_pins


def stop(process):
    if process.poll() is None:
        process.terminate()
    try:
        process.wait(timeout=10)
    except subprocess.TimeoutExpired:
        process.kill()
        process.wait()


@contextmanager
def private_display(work):
    read, write = os.pipe()
    process = None
    try:
        with (work / "xvfb.log").open("wb") as log:
            process = subprocess.Popen(["Xvfb", "-displayfd", str(write), "-screen", "0",
                "3840x2160x24", "-nolisten", "tcp", "-noreset"], pass_fds=(write,),
                stdout=log, stderr=subprocess.STDOUT)
        os.close(write)
        write = None
        require(select.select([read], [], [], 10)[0], "Private X11 startup timed out")
        line = os.read(read, 64).decode().strip()
        require(line.isdigit() and process.poll() is None, "Private X11 display allocation failed")
        yield ":" + line
    finally:
        os.close(read)
        if write is not None:
            os.close(write)
        if process is not None:
            stop(process)


def run(command, work, env, name, timeout):
    """No detached case children: run_test's supervisor owns their whole tree."""
    log_path = work / (name + ".log")
    started = time.monotonic()
    with log_path.open("wb") as log:
        process = subprocess.Popen(command, env=env, stdout=log, stderr=subprocess.STDOUT)
        timed_out = False
        try:
            status = process.wait(timeout=timeout)
        except subprocess.TimeoutExpired:
            timed_out, status = True, 124
        finally:
            stop(process)
    # Scan the entire disposable log. A validation error before the retained
    # tail must still fail the case; only bounded diagnostics are retained.
    errors, error_count, counters, mcc_completed = [], 0, None, False
    with log_path.open(errors="replace") as log:
        for line in log:
            if re.search(r"VUID-|validation\s*error", line, re.IGNORECASE):
                error_count += 1
                if len(errors) < 12:
                    errors.append(line.strip()[:512])
            counts = re.search(r"(\d+) bounded HD text vertices; (\d+) mouse target projections across 9 aspect ratios", line)
            if counts:
                counters = [int(value) for value in counts.groups()]
            mcc_completed |= "using selected MCC-only assets and native MCC font prompts" in line
    with log_path.open("rb") as log:
        log.seek(max(0, log_path.stat().st_size - 24 * 1024))
        tail = log.read().decode(errors="replace")
    return {"command": command, "exit_code": status, "seconds": time.monotonic() - started,
            "timed_out": timed_out, "validation_error_lines": error_count, "validation_errors": errors,
            "geometry_counters": counters, "mcc_assertions_completed": mcc_completed,
            "log_sha256": digest(log_path), "log_bytes": log_path.stat().st_size, "log_tail": tail}


def hardware_inventory(text):
    require("VK_LAYER_KHRONOS_validation" in text, "Hardware inventory did not load the required validation layer")
    devices = []
    for match in re.finditer(r"^GPU\d+:\s*\n(.*?)(?=^GPU\d+:|\Z)", text, re.MULTILINE | re.DOTALL):
        fields = dict(re.findall(r"^\s*(deviceName|deviceType|vendorID|deviceID|driverVersion|driverName)\s*=\s*(.*?)\s*$",
                                 match.group(1), re.MULTILINE))
        require("NVIDIA" in fields.get("deviceName", "") and
                fields.get("deviceType") == "PHYSICAL_DEVICE_TYPE_DISCRETE_GPU" and
                int(fields.get("vendorID", "0"), 0) == 0x10DE,
                "Capture inventory includes a non-NVIDIA or software device; no software fallback is accepted")
        devices.append(fields)
    require(devices, "Pinned NVIDIA driver exposed no real discrete GPU")
    return devices


def capture_rows(directory, expected):
    from PIL import Image
    path = directory / "report.json"
    require(path.is_file() and 0 < path.stat().st_size <= LIMIT, "Native capture report is missing or excessive")
    report = json.loads(path.read_text())
    require(report.get("vulkan_validation") is True and
            report.get("viewports") == [list(value) for value in ASPECTS], "Native capture changed its validation/aspect coverage")
    rows, seen = [], set()
    for row in report.get("captures", []):
        name, width, height, tick = (row.get(key) for key in ("name", "width", "height", "ring_tick"))
        require(name in expected and type(width) is int and type(height) is int and
                (width, height) in ASPECTS and type(tick) is int and tick in (0, 30), "Unexpected native capture scene/extent/tick")
        key = (name, width, height, tick)
        require(key not in seen, "Duplicate native captured scene/extent/tick")
        seen.add(key)
        filename = row.get("file")
        require(isinstance(filename, str) and Path(filename).name == filename and filename.endswith(".ppm"),
                "Native capture escaped its disposable image owner")
        image_path = directory / filename
        require(image_path.is_file() and not image_path.is_symlink() and
                image_path.resolve().is_relative_to(directory.resolve()), "Native captured image has a foreign owner")
        require(type(row.get("vertices")) is int and row["vertices"] > 0, "Native capture submitted no actual UI vertices")
        with Image.open(image_path) as image:
            require(image.format == "PPM" and image.size == (width, height) and image.mode == "RGB",
                    "Actual GPU capture has a different decoded image extent/format")
            extrema = image.getextrema()
        require(any(low != high for low, high in extrema), "Actual GPU capture is entirely flat")
        require(type(row.get("ring_views")) is int and row["ring_views"] >= 0, "Invalid native ring view count")
        require(row.get("ring_model_source") == ("Selected imported MCC mainmenu cache" if row["ring_views"] else "none"),
                "Native capture used an unexpected model donor")
        rows.append({**row, "bytes": image_path.stat().st_size, "sha256": digest(image_path),
                     "rgb_extrema": extrema, "_path": image_path})
    for name in expected:
        for width, height in ASPECTS:
            require((name, width, height, 0) in seen, "Missing actual selected scene at one of nine aspects: " + name)
            base = next(row for row in rows if (row["name"], row["width"], row["height"], row["ring_tick"]) == (name, width, height, 0))
            require(((name, width, height, 30) in seen) == bool(base["ring_views"]),
                    "Native ring animation coverage differs from actual submitted ring ownership")
    return rows


def contact_sheet(rows, budget):
    from PIL import Image, ImageDraw, ImageFont
    names = list(dict.fromkeys(row["name"] for row in rows))
    width, block = 1536, 1680
    sheet = Image.new("RGB", (width, block * len(names)), (12, 18, 28))
    draw = ImageDraw.Draw(sheet)
    try:
        font = ImageFont.truetype("DejaVuSans.ttf", 18)
    except OSError:
        font = ImageFont.load_default()
    details = []
    for section, name in enumerate(names):
        top = section * block
        draw.text((12, top + 6), name + " | actual GPU images, aspect preserved", fill="white", font=font)
        for index, (w, h) in enumerate(ASPECTS):
            row = next(r for r in rows if (r["name"], r["width"], r["height"], r["ring_tick"]) == (name, w, h, 0))
            left, y = (index % 3) * 512, top + 36 + (index // 3) * 296
            draw.text((left + 8, y), str(w) + " x " + str(h), fill="white", font=font)
            with Image.open(row["_path"]) as image:
                image.thumbnail((496, 264), Image.Resampling.LANCZOS)
                sheet.paste(image, (left + (512 - image.width) // 2, y + 26))
        native = next(r for r in rows if (r["name"], r["width"], r["height"], r["ring_tick"]) == (name, 1280, 720, 0))
        draw.text((12, top + 926), "1280 x 720 at native pixel size for reading (no UI reconstruction)", fill="white", font=font)
        with Image.open(native["_path"]) as image:
            sheet.paste(image, (128, top + 954))
        details.append({"scene": name, "native_pixel_detail_source": native["file"],
                        "source_sha256": native["sha256"], "ring_tick": 0})
    # Fixed content, no image/geometry omission to meet the retention bound.
    for quality in (88, 82, 76, 70, 64, 58):
        output = io.BytesIO()
        sheet.save(output, "JPEG", quality=quality, optimize=True)
        if output.tell() <= budget:
            return output.getvalue(), {"width": sheet.width, "height": sheet.height, "quality": quality,
                                      "all_aspects_per_scene": 9, "details": details}
    raise ValueError("Complete readable contact sheet exceeds its bounded evidence budget")


def shader_identity(directory):
    require(directory.is_dir(), "Compiled default shader root has already retired")
    for name in ("ui.vert.spv", "ui.frag.spv", "ui_frame.frag.spv", "world.vert.spv", "world.frag.spv"):
        require((directory / name).is_file(), "Compiled default shader bundle is incomplete: " + name)
    aggregate = hashlib.sha256()
    count = size = 0
    for path in sorted(directory.rglob("*.spv")):
        require(path.is_file() and not path.is_symlink(), "Shader bundle contains a foreign resource owner")
        row = {"name": path.relative_to(directory).as_posix(), "bytes": path.stat().st_size, "sha256": digest(path)}
        aggregate.update(json.dumps(row, sort_keys=True).encode() + b"\n")
        count += 1
        size += row["bytes"]
    return {"path": str(directory), "files": count, "logical_bytes": size, "bundle_sha256": aggregate.hexdigest(),
            "selection": "caller-declared compiled default, verified in place; no native override or copying"}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--case", required=True, choices=SCENES)
    for option in ("native", "root", "output", "mcc-root", "shader-root"):
        parser.add_argument("--" + option, type=Path, required=True)
    args = parser.parse_args()
    root, binary, output = args.root.resolve(), args.native.resolve(), args.output.resolve()
    require(root == Path(__file__).resolve().parents[1], "Render supervisor and explicit source root belong to different cohorts")
    require(os.environ.get("HALO2_TEST_SCRATCH"), "Rendered MCC test requires tools/run_test.py storage ownership")
    require(not output.is_relative_to(root / "assets") and not output.is_relative_to(args.mcc_root.resolve()),
            "Retained evidence may not change selected/imported assets")
    output.mkdir(parents=True, exist_ok=False)
    report = {"schema": "H2_MCC_ONLINE_RENDERED_V1", "case": args.case, "passed": False,
              "runs": [], "captures": [], "limits": ["Existing complete MCC import; no fresh raw extraction/onboarding",
              "Synthetic loopback account/friend/invitation records, no real authorization or external delivery",
              "Native CPU geometry assertions and run-wide counters; no per-image glyph/hit rectangle telemetry",
              "Contact sheets require human review; no original framebuffer/hardware display parity or complete U01/U02 claim"]}
    status, contact, work_path = 1, None, None
    try:
        from PIL import Image
        require(binary.is_file() and os.access(binary, os.X_OK), "Qualified frontend binary is unavailable")
        require(shutil.which("Xvfb") and shutil.which("vulkaninfo"), "Private Xvfb and hardware Vulkan inventory tools are required")
        source = ui_arguments(root, ("--mcc-root", str(args.mcc_root)))
        cache = source.mcc_cache
        icd = Path("/usr/share/vulkan/icd.d/nvidia_icd.json")
        validation = root / ".tools/validation/usr"
        layer = validation / "share/vulkan/explicit_layer.d/VkLayer_khronos_validation.json"
        library = validation / "lib/libVkLayer_khronos_validation.so"
        require(json.loads(icd.read_text())["ICD"]["library_path"] == "libGLX_nvidia.so.0", "Unexpected hardware ICD owner")
        require(json.loads(layer.read_text())["layer"]["name"] == "VK_LAYER_KHRONOS_validation", "Required validation layer metadata differs")
        pins, cache_pins = rendered_input_identities(root, cache)
        pins.update({"native": identity(binary), "hardware_icd": identity(icd), "validation_metadata": identity(layer),
                     "validation_library": identity(library)})
        shaders = shader_identity(args.shader_root.resolve())
        report.update(inputs=pins, imported_cache=str(cache), imported_metadata=cache_pins, shaders=shaders,
                      actual_aspects=[list(value) for value in ASPECTS], explicit_switches=["--validation", "1"])
        with tempfile.TemporaryDirectory(prefix="mcc-rendered-", dir=os.environ["HALO2_TEST_SCRATCH"]) as temporary:
            work = work_path = Path(temporary)
            env = dict(os.environ, SDL_VIDEODRIVER="x11", SDL_VIDEO_DRIVER="x11", SDL_AUDIODRIVER="dummy",
                       VK_DRIVER_FILES=str(icd), VK_ICD_FILENAMES=str(icd),
                       VK_LAYER_PATH=str(layer.parent), VK_INSTANCE_LAYERS="VK_LAYER_KHRONOS_validation",
                       VK_LAYER_VALIDATE_SYNC="1", XDG_CACHE_HOME=str(work / "cache"),
                       __GL_SHADER_DISK_CACHE_PATH=str(work / "nvidia-cache"),
                       XDG_CONFIG_HOME=str(work / "config"), XDG_DATA_HOME=str(work / "data"),
                       XDG_RUNTIME_DIR=str(work / "runtime"))
            (work / "runtime").mkdir(mode=0o700)
            env["LD_LIBRARY_PATH"] = str(validation / "lib") + (":" + env["LD_LIBRARY_PATH"] if env.get("LD_LIBRARY_PATH") else "")
            with private_display(work) as display:
                env["DISPLAY"] = display
                inventory = run([shutil.which("vulkaninfo"), "--summary"], work, env, "gpu-inventory", 30)
                require(inventory["exit_code"] == 0 and not inventory["validation_error_lines"],
                        "Pinned hardware Vulkan inventory failed: " + inventory["log_tail"])
                report["gpu_inventory"] = hardware_inventory(inventory.pop("log_tail"))
                report["gpu_inventory_run"] = inventory
                rows = []
                for index, (prefix, expected) in enumerate(SCENES[args.case]):
                    captures = work / ("captures-" + str(index))
                    captures.mkdir()
                    options = ["--capture", str(captures), "--capture-prefix", prefix, "--validation", "1"]
                    if args.case in ("clans", "parties"):
                        wrapper = root / ("tests/frontend_" + args.case + "_http_test.py")
                        native = [sys.executable, str(wrapper), str(binary), "--mcc-root", str(cache), *options]
                    else:
                        native = [str(binary), *source.source, *options]
                    command = [sys.executable, str(root / "tools/run_test.py"), "--scratch-root", str(work / "children"), "--", *native]
                    timeout = {"clans": 280, "parties": 290, "social": 135}[args.case]
                    result = run(command, work, env, "native-" + str(index), timeout)
                    counts = result["geometry_counters"]
                    result.update(capture_prefix=prefix, expected_scenes=list(expected))
                    report["runs"].append(result)
                    require(result["exit_code"] == 0, "Actual native MCC rendered flow failed")
                    require(not result["validation_error_lines"] and counts and all(value > 0 for value in counts) and
                            result["mcc_assertions_completed"],
                            "Native MCC byte/geometry/validation assertions did not complete")
                    result["geometry_counters_scope"] = "entire native run, not individual captured frames"
                    result["bounded_text_vertices"], result["pointer_projections"] = counts
                    rows.extend(capture_rows(captures, expected))
                report["captures"] = [{key: value for key, value in row.items() if key != "_path"} for row in rows]
                # Reserve bounded metadata/failure space as well as the actual
                # existing receipt. Every scene/extent stays on the sheet.
                budget = LIMIT - len(json.dumps(report, indent=2).encode()) - 16 * 1024
                require(budget > 0, "Complete native receipt exhausts the bounded evidence budget")
                contact, info = contact_sheet(rows, budget)
                report["contact_sheet"] = dict(info, file="contact.jpg", bytes=len(contact), sha256=hashlib.sha256(contact).hexdigest())
            require(all(identity(row["path"]) == row for row in pins.values()) and
                    all(identity(row["path"]) == row for row in cache_pins.values()) and
                    shader_identity(args.shader_root.resolve()) == shaders, "Capture source/binary/resource inputs changed")
        report["passed"] = True
        status = 0
    except Exception as error:
        report["error"] = str(error)[-24 * 1024:]
    except KeyboardInterrupt:
        status = 130
        report["error"] = "Interrupted; owned native/display processes and raw storage retired"
    finally:
        report["raw_workspace"] = str(work_path) if work_path else None
        report["raw_workspace_removed"] = work_path is None or not work_path.exists()
        payload = (json.dumps(report, indent=2) + "\n").encode()
        require(len(payload) + (len(contact) if contact else 0) <= LIMIT, "Rendered evidence exceeds combined 1MiB retention bound")
        if contact:
            (output / "contact.jpg").write_bytes(contact)
        (output / "report.json").write_bytes(payload)
    print(("PASS" if status == 0 else "FAIL") + " MCC rendered " + args.case + ": " + str(output / "report.json"), flush=True)
    return status


if __name__ == "__main__":
    def interrupted(_number, _frame):
        raise KeyboardInterrupt()
    signal.signal(signal.SIGTERM, interrupted)
    raise SystemExit(main())
