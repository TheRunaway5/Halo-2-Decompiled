"""MCC-owned inputs for the retained loading animation and renderer handoff.

The glow mask is an explicitly authored reconstruction from MCC logo/glow
artwork, authorized by the user on 2026-10-20. It is not an exact Vista raster.
V2 and its original-tone APIs remain legacy/reference compatibility only.
The explicit V3 APIs import genuine MCC media and never fall back to that tone.
V3's finite one-shot policy is native policy, not authored frontend loop timing.
"""
import hashlib
import io
import json
import math
import os
from pathlib import Path
from pathlib import PurePosixPath
import struct
import subprocess
import sys
import tempfile

from audit_rendering import FormatError, file_sha
from mcc_movies import _cancellation, _lock, _owned, _publish_new

SCHEMA = "H2_MCC_NATIVE_LOADING_V2"
PROFILE = "mcc-logo-glow-original-tone-v1"
BANK = "presentation/loading-v2"
AMBIENCE = "resources/audio/loading-ambience"
TONE = "loading-tone.xbox-adpcm"
TONE_SHA = "4f61e827be9b54b71e3d37a145c9b88fb32eaf517e23746af85236ca41ba8fc9"
PCM_SHA = "b740acf4ce447da6583b4f6c7fd6696fabbb249664a53fb780b18c4dd58cd73d"
FREQUENCIES = [21738, 16290, 29008, 10876]
CODEBOOK = "external/ww2ogg/packed_codebooks_aoTuV_603.bin"
CODEBOOK_SHA = "00a93eab267d281401b1efd54e888a2e183299b9e6c446c48d09f701a89d9d27"
MEDIA = "MCC/Content/WwiseAudio/Windows/236657329.wem"
MEDIA_SHA = "7ecbdce187ffda00077a593db8ef65a5e9e5ef72d0cd3d4474c523246f7b1695"
V3_SCHEMA = "H2_MCC_NATIVE_LOADING_V3"
V3_PROFILE = "mcc-logo-glow-mcc-track-oneshot-v1"
V3_BANK = "presentation/loading-v3"
V3_WAV = "loading.wav"
V3_FRAMES = 2744320
V3_PCM_SHA = "4170c17c50ed19f4fd1fcd292431a78564c139ff6f1324e3ea56fdefcac34d6e"
V3_POLICY = "full-decoded-track-once-then-silence-until-end"
ARCHIVE = "MCC/Content/Paks/MCC-WindowsNoEditor.pak"
CONVERTER_MANIFEST = "resources/mcc-loading-converter.json"
CONVERTER_SCHEMA = "H2_MCC_LOADING_CONVERTER_RUNTIME_V1"
CONVERTER_WRAPPER = "bin/halo2_mcc_loading_audio"
CONVERTER_PAYLOAD = CONVERTER_WRAPPER + ".real"
CONVERTER_LOADER = "lib/ld-linux-x86-64.so.2"


def converter_wrapper():
    return ('#!/bin/sh\nset -eu\n'
            'root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)\n'
            'exec "$root/lib/ld-linux-x86-64.so.2" --library-path "$root/lib" '
            '"$root/bin/halo2_mcc_loading_audio.real" "$@"\n').encode()


class Reader:
    def __init__(self, data):
        self.data, self.at = data, 0

    def take(self, size):
        if not 0 <= size <= len(self.data) - self.at:
            raise FormatError("Truncated MCC loading archive metadata")
        result = self.data[self.at:self.at + size]
        self.at += size
        return result

    def unpack(self, fmt):
        return struct.unpack(fmt, self.take(struct.calcsize(fmt)))

    def string(self):
        count, = self.unpack("<i")
        if not -65536 < count < 65536:
            raise FormatError("Excessive MCC archive filename")
        if not count:
            return ""
        data = self.take(abs(count) * (2 if count < 0 else 1))
        terminator = b"\0\0" if count < 0 else b"\0"
        if not data.endswith(terminator):
            raise FormatError("Unterminated MCC archive filename")
        return data[:-len(terminator)].decode("utf-16le" if count < 0 else "latin-1")


def entry(reader):
    start = reader.at
    offset, size, decoded, method = reader.unpack("<qqqi")
    digest = reader.take(20)
    if min(offset, size, decoded) < 0 or method not in (0, 1, 2, 4):
        raise FormatError("Unsupported MCC archive entry")
    blocks = []
    if method:
        count, = reader.unpack("<i")
        if not 0 <= count <= 100000:
            raise FormatError("Excessive MCC archive block count")
        blocks = [reader.unpack("<qq") for _ in range(count)]
    flags, block_size = reader.unpack("<BI")
    if flags & ~3:
        raise FormatError("Unsupported MCC archive entry flags")
    return dict(offset=offset, size=size, decoded=decoded, method=method,
                digest=digest.hex(), blocks=blocks, flags=flags,
                block_size=block_size, header_bytes=reader.at - start)


def read_media(source):
    """Seek-only UE4 v7 reader; every selected payload has a SHA1 owner."""
    source = Path(source)
    pak = source.parent / "MCC/Content/Paks/MCC-WindowsNoEditor.pak"
    if pak.is_symlink() or not pak.is_file():
        raise FormatError("MCC loading audio archive is missing: " + str(pak))
    before = (pak.stat().st_size, pak.stat().st_mtime_ns)
    with pak.open("rb") as stream:
        def read_at(offset, count):
            if not 0 <= count <= 32 * 1024**2 or offset < 0 or offset + count > before[0]:
                raise FormatError("MCC loading archive extent is outside its owner")
            stream.seek(offset)
            data = stream.read(count)
            if len(data) != count:
                raise FormatError("Incomplete MCC loading archive read")
            return data
        footer = read_at(before[0] - 61, 61)
        magic, version, offset, size, digest = struct.unpack_from("<Iiqq20s", footer, 17)
        if magic != 0x5a6f12e1 or version != 7 or footer[16] or offset + size > before[0] - 61:
            raise FormatError("Unsupported MCC loading archive index")
        index = read_at(offset, size)
        if hashlib.sha1(index).digest() != digest:
            raise FormatError("MCC archive index checksum differs")
        reader = Reader(index)
        if reader.string() != "../../../":
            raise FormatError("Unsupported MCC archive mount")
        count, = reader.unpack("<i")
        if not 0 < count < 1000000:
            raise FormatError("Invalid MCC archive entry count")
        selected = None
        names = set()
        for _ in range(count):
            name, row = reader.string(), entry(reader)
            if not name or name in names:
                raise FormatError("Ambiguous MCC archive filename")
            names.add(name)
            if name == MEDIA:
                selected = row
        if reader.at != len(index) or selected is None:
            raise FormatError("MCC loading media is missing from the complete archive index")
        row = selected
        if row["method"] or row["flags"] or row["size"] != row["decoded"] or not 0 < row["size"] < 16 * 1024**2:
            raise FormatError("Unsupported MCC loading media storage")
        header = entry(Reader(read_at(row["offset"], row["header_bytes"])))
        if any(header[k] != row[k] for k in row if k != "offset"):
            raise FormatError("MCC loading media header differs from its index")
        payload = read_at(row["offset"] + row["header_bytes"], row["size"])
        if hashlib.sha1(payload).hexdigest() != row["digest"]:
            raise FormatError("MCC loading media checksum differs")
    if (pak.stat().st_size, pak.stat().st_mtime_ns) != before:
        raise FormatError("MCC changed during loading audio import")
    # This profile has an independently authenticated event->switch->track
    # graph and normal decoder admission. A changed media profile needs review.
    if hashlib.sha256(payload).hexdigest() != MEDIA_SHA:
        raise FormatError("MCC loading track differs from the supported media profile")
    return payload, dict(path=MEDIA, sha256=MEDIA_SHA, pak_index_sha1=digest.hex(),
                         event="Play_Music", switch_group=2949940656,
                         switch_key=1792793154, media_id=236657329,
                         archive=dict(path=ARCHIVE, bytes=before[0], mtime_ns=before[1],
                             index_offset=offset, index_bytes=size, index_sha1=digest.hex(),
                             entry_offset=row["offset"], entry_bytes=row["size"], entry_sha1=row["digest"]))


def _bitmap(cache, inventory, name, ordinal=0):
    from PIL import Image
    from export_vista_menu_ui import dds_image
    matches = [b for b in inventory["banks"] if b["map"] == "mainmenu.map" and b["tag"] == name]
    if len(matches) != 1 or not 0 <= ordinal < len(matches[0]["frames"]):
        raise FormatError("MCC loading reconstruction has no unique bitmap owner: " + name)
    frame = matches[0]["frames"][ordinal]
    if frame["type"] or frame["dimensions"][2] != 1 or not frame["partitions"][0] or any(frame["partitions"][1:]):
        raise FormatError("Unsupported MCC loading reconstruction bitmap")
    resource = inventory["resources"][frame["partitions"][0]]
    path = _owned(cache / resource["path"], cache)
    if file_sha(path) != resource["sha256"]:
        raise FormatError("MCC loading bitmap differs from its imported owner")
    width, height, _ = frame["dimensions"]
    data, _ = dds_image(width, height, frame["additional_mips"], frame["format"], path.read_bytes())
    return Image.open(io.BytesIO(data)).convert("RGBA"), dict(tag=name, frame=ordinal,
        resource_sha256=resource["sha256"], datum=matches[0]["datum"])


def reconstruct_mask(glow, logo):
    """Authored effect recipe, not stored reference pixels or a Vista bank."""
    from PIL import Image, ImageChops, ImageEnhance, ImageFilter
    canvas = Image.new("L", (512, 512))
    glow = glow.convert("RGB").convert("L")
    # Two differently stretched instances retain the MCC glow's soft falloff.
    for width, height, gain in ((220, 1100, .72), (1050, 240, .50)):
        layer = Image.new("L", canvas.size)
        image = ImageEnhance.Brightness(glow.resize((width, height), Image.Resampling.BICUBIC)).enhance(gain)
        layer.paste(image, ((512 - width) // 2, (512 - height) // 2))
        canvas = ImageChops.screen(canvas, layer)
    silhouette = Image.new("L", canvas.size)
    silhouette.paste(logo.getchannel("A").resize((410, 68), Image.Resampling.LANCZOS), (51, 222))
    canvas = ImageChops.screen(canvas, silhouette.filter(ImageFilter.GaussianBlur(3.5)))
    result = Image.merge("RGBA", (canvas, canvas, canvas, Image.new("L", canvas.size, 255)))
    header = [124, 0x100f, 512, 512, 2048, 0, 0] + [0] * 11
    header += [32, 0x41, 0, 32, 0xff0000, 0xff00, 0xff, 0xff000000, 0x1000, 0, 0, 0, 0]
    return b"DDS " + struct.pack("<31I", *header) + result.tobytes("raw", "BGRA")


def converter(root):
    from installation_paths import resource_root
    root = resource_root(root)
    candidates = (("halo2_mcc_loading_audio.exe", "bin/halo2_mcc_loading_audio.exe",
                   "dist/builds/windows-x86_64/halo2_mcc_loading_audio.exe",
                   "build-windows/halo2_mcc_loading_audio.exe", "build/halo2_mcc_loading_audio.exe")
                  if sys.platform == "win32" else
                  ("bin/halo2_mcc_loading_audio", "dist/builds/linux-x86_64/halo2_mcc_loading_audio", "build/halo2_mcc_loading_audio"))
    for relative in candidates:
        path = _owned(root / relative, root)
        if path.is_file() and os.access(path, os.X_OK):
            return path
    raise FormatError("The bundled MCC loading audio converter is missing; update the port binaries")


def bundled_ambience(root):
    """Authenticate the small approved resource without an original install."""
    from installation_paths import resource_root
    root = resource_root(root)
    manifest = _owned(root / AMBIENCE / "manifest.json", root)
    if not manifest.is_file() or manifest.stat().st_size > 8192:
        raise FormatError("Bundled loading ambience manifest is missing or excessive")
    value = json.loads(manifest.read_bytes())
    expected = dict(schema="H2_BUNDLED_LOADING_AMBIENCE_V1", file=TONE,
                    bytes=18396, sha256=TONE_SHA, decoded_pcm_sha256=PCM_SHA,
                    decoded_frames=32704, sample_rate=22050, channels=1,
                    playback_frequencies=FREQUENCIES)
    if any(value.get(key) != item for key, item in expected.items()):
        raise FormatError("Bundled loading ambience profile changed")
    path = _owned(root / AMBIENCE / TONE, root)
    if not path.is_file() or path.stat().st_size != 18396 or file_sha(path) != TONE_SHA:
        raise FormatError("Bundled loading ambience is missing or changed")
    return path.read_bytes(), dict(path=AMBIENCE + "/" + TONE, sha256=TONE_SHA,
        manifest_sha256=file_sha(manifest), kind="port-bundled-original-loading-tone",
        user_approved_exception="2026-10-20")


def preflight(root, source):
    """Legacy V2 preflight; not admission for the MCC-only V3 runtime route."""
    bundled_ambience(root)


def verify(cache):
    target = _owned(cache / BANK, cache, directory=True)
    path = _owned(target / "manifest.json", cache)
    if not path.is_file() or path.stat().st_size > 65536:
        raise FormatError("MCC loading presentation manifest is missing or excessive")
    value = json.loads(path.read_bytes())
    if value.get("schema") != SCHEMA or value.get("profile") != PROFILE or value.get("complete") is not True:
        raise FormatError("Unrecognized MCC loading presentation; existing files retained")
    if set(value.get("files", {})) != {"glow-mask.dds", TONE}:
        raise FormatError("MCC loading presentation is missing a required role")
    if value["files"][TONE] != dict(bytes=18396, sha256=TONE_SHA):
        raise FormatError("MCC loading presentation changed its original ambience; existing files retained")
    for name, row in value["files"].items():
        if Path(name).name != name:
            raise FormatError("Loading manifest requires local filenames")
        p = _owned(target / name, cache)
        if not p.is_file() or p.stat().st_size != row["bytes"] or file_sha(p) != row["sha256"]:
            raise FormatError("MCC loading presentation changed; existing files retained: " + name)
    return dict(schema=SCHEMA, complete=True, path=BANK + "/manifest.json", sha256=file_sha(path),
                profile=PROFILE, mask_reconstructed_from_mcc=True, exact_vista_pixels=False,
                original_loading_tone=True, bundled_ambience_exception=True)


def _prepare(root, cache, source, progress):
    target = _owned(cache / BANK, cache, directory=True)
    if target.exists():
        return verify(cache)
    tone, tone_owner = bundled_ambience(root)
    inventory = json.loads(_owned(cache / "bitmap-resources.json", cache).read_bytes())
    glow, glow_owner = _bitmap(cache, inventory, "rasterizer\\glow")
    logo, logo_owner = _bitmap(cache, inventory, "globals\\loading_screen\\loading_screen_logos")
    mask = reconstruct_mask(glow, logo)
    _owned(cache / "presentation", cache, directory=True).mkdir(exist_ok=True)
    with tempfile.TemporaryDirectory(prefix=".mcc-loading-", dir=cache) as temporary:
        temporary = Path(temporary)
        bank = temporary / "bank"
        bank.mkdir()
        (bank / "glow-mask.dds").write_bytes(mask)
        progress("Preparing MCC loading artwork and the bundled original ambience…")
        (bank / TONE).write_bytes(tone)
        files = {p.name: dict(bytes=p.stat().st_size, sha256=file_sha(p)) for p in bank.iterdir()}
        metadata = dict(schema=SCHEMA, profile=PROFILE, complete=True, files=files,
            mask=dict(path="glow-mask.dds", source_width=512, source_height=512,
                      inputs=[glow_owner, logo_owner], authored_reconstruction=True, exact_vista_pixels=False),
            audio=dict(path=TONE, channels=1, sample_rate=22050, decoded_frames=32704,
                       decoded_pcm_sha256=PCM_SHA, playback_frequencies=FREQUENCIES,
                       source=tone_owner, original_tone_bytes=True,
                       native_policy="recovered four-voice owner/envelopes; retained graphics fade gain and lifetime"),
            shader_programs="unchanged", animation="retained vista_loading_geometry/desktop_loading")
        (bank / "manifest.json").write_text(json.dumps(metadata, indent=2) + "\n")
        # One rename publishes the complete bank. Unknown/custom existing
        # destinations are never merged into or overwritten by repair.
        if target.exists():
            raise FormatError("MCC loading presentation changed during import")
        _publish_new(bank, target)
    return verify(cache)


def prepare(root, cache, source, *, progress=print):
    root, cache = Path(root).resolve(), Path(cache)
    with _cancellation(), _lock(cache):
        return _prepare(root, cache, source, progress)


def repair(root, cache, *, progress=print):
    root, cache = Path(root).resolve(), Path(cache)
    with _cancellation(), _lock(cache):
        provenance = _owned(cache / "provenance.json", cache)
        before = provenance.read_bytes()
        value = json.loads(before)
        if value.get("external_installation_requirement") != "Halo MCC only":
            raise FormatError("Loading repair requires an owned MCC-only cache")
        result = _prepare(root, cache, Path(value["source"]), progress)
        updated = {**value, "native_loading_presentation": result,
                   "port_bundled_loading_ambience": dict(user_approved_exception="2026-10-20",
                       path=BANK + "/" + TONE)}
        if updated != value:
            from mcc_ui_layout import _atomic
            if provenance.read_bytes() != before:
                raise FormatError("MCC provenance changed during loading conversion")
            _atomic(provenance, (json.dumps(updated, indent=2) + "\n").encode())
        return cache, updated


def _hex(value, size):
    return isinstance(value, str) and len(value) == size and all(c in "0123456789abcdef" for c in value)


def _v3_dependency(path, root):
    path = _owned(path, root)
    if not path.is_file() or not 0 < path.stat().st_size < 64 * 1024**2:
        raise FormatError("MCC loading converter dependency is missing or excessive")
    before = (path.stat().st_size, path.stat().st_mtime_ns)
    result = dict(path=path.relative_to(root).as_posix(), bytes=before[0], sha256=file_sha(path))
    if (path.stat().st_size, path.stat().st_mtime_ns) != before:
        raise FormatError("MCC loading converter dependency changed during admission")
    return result


def _v3_dependencies(root, source):
    from installation_paths import resource_root
    root = resource_root(root)
    _owned(Path(source).parent / ARCHIVE, Path(source).parent)
    payload, owner = read_media(source)
    dependencies = converter_dependencies(root)
    executable = root / dependencies["converter"]["path"]
    decoder, codebook = dependencies["converter"], dependencies["codebook"]
    return payload, owner, executable, decoder, codebook


def _converter_file_records(files):
    required = {CONVERTER_WRAPPER, CONVERTER_PAYLOAD, CONVERTER_LOADER, CODEBOOK}
    if not isinstance(files, dict) or not required <= files.keys() or not 4 <= len(files) <= 256:
        raise FormatError("Invalid packaged MCC loading converter closure")
    total = 0
    for name, record in files.items():
        if not isinstance(name, str):
            raise FormatError("Invalid packaged MCC loading converter path")
        relative = PurePosixPath(name)
        if (not name or relative.is_absolute() or relative.as_posix() != name or
                any(p in ("", ".", "..") for p in relative.parts) or "\\" in name or
                (name not in required and (len(relative.parts) != 2 or relative.parts[0] != "lib"))):
            raise FormatError("Invalid packaged MCC loading converter path")
        if (not isinstance(record, dict) or set(record) != {"bytes", "sha256"} or
                type(record["bytes"]) is not int or not 0 < record["bytes"] < 64 * 1024**2 or
                not _hex(record["sha256"], 64)):
            raise FormatError("Invalid packaged MCC loading converter file identity")
        total += record["bytes"]
    if total > 256 * 1024**2:
        raise FormatError("Excessive packaged MCC loading converter closure")


def _converter_runtime_metadata(decoder, codebook):
    runtime = decoder.get("runtime")
    if runtime is None:
        return
    if isinstance(runtime, dict) and runtime.get("schema") == "H2_MACOS_MCC_LOADING_CONVERTER_RUNTIME_V1":
        from macos.loading_converter import validate_metadata
        validate_metadata(decoder, codebook)
        return
    if not isinstance(runtime, dict) or runtime.get("schema") != CONVERTER_SCHEMA:
        raise FormatError("Invalid packaged MCC loading converter provenance")
    _converter_file_records(runtime.get("files"))
    manifest = runtime.get("manifest", {})
    if (not isinstance(manifest, dict) or manifest.get("path") != CONVERTER_MANIFEST or
            type(manifest.get("bytes")) is not int or not 0 < manifest["bytes"] <= 65536 or
            not _hex(manifest.get("sha256"), 64)):
        raise FormatError("Invalid packaged MCC loading converter manifest identity")
    for row, name in ((decoder, CONVERTER_WRAPPER), (codebook, CODEBOOK)):
        if row["path"] != name or runtime["files"][name] != {k: row[k] for k in ("bytes", "sha256")}:
            raise FormatError("Packaged MCC loading converter provenance owners disagree")


def converter_dependencies(root):
    """Pin a direct development decoder or its complete owned portable runtime."""
    from installation_paths import resource_root
    root = resource_root(root)
    manifest = _owned(root / CONVERTER_MANIFEST, root)
    if manifest.is_file() and manifest.stat().st_size <= 65536:
        value = json.loads(manifest.read_bytes())
        if isinstance(value, dict) and value.get("schema") == "H2_MACOS_MCC_LOADING_CONVERTER_RUNTIME_V1":
            from macos.loading_converter import dependencies
            return dependencies(root, manifest, value)
    executable = converter(root)
    decoder = _v3_dependency(executable, root)
    codebook = _v3_dependency(root / CODEBOOK, root)
    if codebook["sha256"] != CODEBOOK_SHA:
        raise FormatError("MCC loading Vorbis codebook differs from its supported owner")
    manifest = _owned(root / CONVERTER_MANIFEST, root)
    if manifest.exists() or executable.read_bytes()[:len(converter_wrapper())] == converter_wrapper():
        if (decoder["path"] != CONVERTER_WRAPPER or not manifest.is_file() or
                manifest.stat().st_size > 65536):
            raise FormatError("Packaged MCC loading converter requires its bounded runtime manifest")
        before = manifest.read_bytes()
        value = json.loads(before)
        files = value.get("files") if isinstance(value, dict) else None
        if not isinstance(value, dict) or value.get("schema") != CONVERTER_SCHEMA:
            raise FormatError("Invalid packaged MCC loading converter closure")
        _converter_file_records(files)
        checked = {}
        for name, record in files.items():
            actual = _v3_dependency(root / name, root)
            if record != {k: actual[k] for k in ("bytes", "sha256")}:
                raise FormatError("Packaged MCC loading converter dependency changed: " + name)
            checked[name] = record
        if executable.read_bytes() != converter_wrapper():
            raise FormatError("Packaged MCC loading converter wrapper differs")
        for name in (CONVERTER_WRAPPER, CONVERTER_PAYLOAD, CONVERTER_LOADER):
            if not os.access(root / name, os.X_OK):
                raise FormatError("Packaged MCC loading converter owner is not executable")
        actual_libraries = {p.relative_to(root).as_posix() for p in (root / "lib").iterdir()}
        if actual_libraries != {name for name in files if name.startswith("lib/")}:
            raise FormatError("Packaged MCC loading converter library inventory differs")
        if manifest.read_bytes() != before:
            raise FormatError("Packaged MCC loading converter manifest changed during admission")
        decoder = {**decoder, "runtime": dict(schema=CONVERTER_SCHEMA,
            manifest=_v3_dependency(manifest, root), files=checked)}
        _converter_runtime_metadata(decoder, codebook)
    return dict(converter=decoder, codebook=codebook)


def preflight_v3(root, source):
    """Explicit genuine-MCC dependency check; never reads bundled ambience."""
    root = Path(root).resolve()
    _, source_owner, _, decoder, codebook = _v3_dependencies(root, source)
    return dict(schema=V3_SCHEMA, profile=V3_PROFILE, source=source_owner,
                converter=decoder, codebook=codebook)


def _v3_pcm(data):
    data_bytes = V3_FRAMES * 2 * 4
    expected = (b"RIFF" + struct.pack("<I", data_bytes + 36) + b"WAVEfmt " +
                struct.pack("<IHHIIHH", 16, 3, 2, 44100, 44100 * 8, 8, 32) +
                b"data" + struct.pack("<I", data_bytes))
    if len(data) != 44 + data_bytes or data[:44] != expected:
        raise FormatError("MCC loading track requires exact canonical stereo44100Hz IEEE-F32 WAV")
    if hashlib.sha256(memoryview(data)[44:]).hexdigest() != V3_PCM_SHA:
        raise FormatError("MCC loading PCM differs from the authenticated source profile")
    if any(not math.isfinite(sample) for (sample,) in struct.iter_unpack("<f", memoryview(data)[44:])):
        raise FormatError("MCC loading PCM contains nonfinite samples")


def _v3_metadata(value):
    if (not isinstance(value, dict) or value.get("schema") != V3_SCHEMA or
            value.get("profile") != V3_PROFILE or value.get("complete") is not True):
        raise FormatError("Unrecognized MCC V3 loading profile; existing files retained")
    files = value.get("files")
    if not isinstance(files, dict) or set(files) != {"glow-mask.dds", V3_WAV}:
        raise FormatError("MCC V3 loading presentation is missing a required role")
    for row in files.values():
        if (not isinstance(row, dict) or set(row) != {"bytes", "sha256"} or
                type(row.get("bytes")) is not int or row["bytes"] <= 0 or not _hex(row.get("sha256"), 64)):
            raise FormatError("Invalid MCC V3 loading file identity")
    mask, audio = value.get("mask", {}), value.get("audio", {})
    if (not isinstance(mask, dict) or mask.get("path") != "glow-mask.dds" or
            mask.get("source_width") != 512 or mask.get("source_height") != 512 or
            mask.get("authored_reconstruction") is not True or mask.get("exact_vista_pixels") is not False):
        raise FormatError("MCC V3 loading mask profile differs")
    expected = dict(path=V3_WAV, encoding="ieee-float32-le", channels=2, sample_rate=44100,
                    decoded_frames=V3_FRAMES, decoded_pcm_sha256=V3_PCM_SHA,
                    native_policy=V3_POLICY, authored_frontend_timing=False)
    if not isinstance(audio, dict) or any(audio.get(key) != item for key, item in expected.items()):
        raise FormatError("MCC V3 loading audio profile differs; legacy ambience is not a fallback")
    source = audio.get("source", {})
    source_fields = dict(path=MEDIA, sha256=MEDIA_SHA, event="Play_Music", media_id=236657329,
                         switch_group=2949940656, switch_key=1792793154)
    if not isinstance(source, dict) or any(source.get(key) != item for key, item in source_fields.items()):
        raise FormatError("MCC V3 loading media provenance differs")
    archive = source.get("archive", {})
    if (not isinstance(archive, dict) or archive.get("path") != ARCHIVE or
            any(type(archive.get(k)) is not int or archive[k] < 0 for k in
                ("bytes", "mtime_ns", "index_offset", "index_bytes", "entry_offset", "entry_bytes")) or
            not archive["bytes"] or not archive["index_bytes"] or not archive["entry_bytes"] or
            archive["index_offset"] + archive["index_bytes"] > archive["bytes"] - 61 or
            archive["entry_offset"] + archive["entry_bytes"] > archive["bytes"] or
            not _hex(archive.get("index_sha1"), 40) or not _hex(archive.get("entry_sha1"), 40) or
            source.get("pak_index_sha1") != archive["index_sha1"]):
        raise FormatError("MCC V3 loading archive provenance is incomplete")
    for role in ("converter", "codebook"):
        row = audio.get(role, {})
        path = Path(row.get("path", "")) if isinstance(row, dict) else Path()
        if (not isinstance(row, dict) or not path.parts or path.is_absolute() or ".." in path.parts or
                type(row.get("bytes")) is not int or row["bytes"] <= 0 or not _hex(row.get("sha256"), 64)):
            raise FormatError("MCC V3 loading decoder provenance is incomplete")
    if audio["codebook"]["path"] != CODEBOOK or audio["codebook"]["sha256"] != CODEBOOK_SHA:
        raise FormatError("MCC V3 loading codebook provenance differs")
    _converter_runtime_metadata(audio["converter"], audio["codebook"])


def verify_v3(cache):
    """Validate a complete V3 bank without an installation or legacy fallback."""
    cache = Path(cache)
    target = _owned(cache / V3_BANK, cache, directory=True)
    manifest = _owned(target / "manifest.json", cache)
    if not manifest.is_file() or manifest.stat().st_size > 65536:
        raise FormatError("MCC V3 loading manifest is missing or excessive")
    before = manifest.read_bytes()
    try:
        metadata = json.loads(before)
    except (ValueError, UnicodeError) as error:
        raise FormatError("Invalid MCC V3 loading manifest") from error
    _v3_metadata(metadata)
    expected_sizes = {"glow-mask.dds": 128 + 512 * 512 * 4, V3_WAV: 44 + V3_FRAMES * 8}
    for name, row in metadata["files"].items():
        path = _owned(target / name, cache)
        if row["bytes"] != expected_sizes[name] or not path.is_file() or path.stat().st_size != row["bytes"]:
            raise FormatError("MCC V3 loading extent differs; existing files retained: " + name)
        data = path.read_bytes()
        if len(data) != row["bytes"] or hashlib.sha256(data).hexdigest() != row["sha256"]:
            raise FormatError("MCC V3 loading identity differs; existing files retained: " + name)
        if name == V3_WAV:
            _v3_pcm(data)
        elif data[:4] != b"DDS " or struct.unpack_from("<II", data, 12) != (512, 512):
            raise FormatError("MCC V3 loading mask shape differs")
    if manifest.read_bytes() != before:
        raise FormatError("MCC V3 loading manifest changed during verification")
    return dict(schema=V3_SCHEMA, profile=V3_PROFILE, complete=True, path=V3_BANK + "/manifest.json",
                sha256=hashlib.sha256(before).hexdigest(), mcc_loading_track=True,
                original_loading_tone=False, authored_frontend_timing=False, native_policy=V3_POLICY)


def _prepare_v3(root, cache, source, progress):
    from installation_paths import resource_root
    root = resource_root(root)
    target = _owned(cache / V3_BANK, cache, directory=True)
    if target.exists():
        return verify_v3(cache)
    payload, media, executable, decoder, codebook = _v3_dependencies(root, source)
    inventory = json.loads(_owned(cache / "bitmap-resources.json", cache).read_bytes())
    glow, glow_owner = _bitmap(cache, inventory, "rasterizer\\glow")
    logo, logo_owner = _bitmap(cache, inventory, "globals\\loading_screen\\loading_screen_logos")
    mask = reconstruct_mask(glow, logo)
    _owned(cache / "presentation", cache, directory=True).mkdir(exist_ok=True)
    with tempfile.TemporaryDirectory(prefix=".mcc-loading-v3-", dir=cache) as temporary:
        temporary = Path(temporary)
        bank = temporary / "bank"
        bank.mkdir()
        (bank / "glow-mask.dds").write_bytes(mask)
        wem, wav, log = temporary / "source.wem", bank / V3_WAV, temporary / "converter.log"
        wem.write_bytes(payload)
        progress("Preparing genuine MCC loading track (native finite one-shot policy)…")
        try:
            with log.open("wb") as output:
                command = ([str(root / CONVERTER_LOADER), "--library-path", str(root / "lib"),
                            str(root / CONVERTER_PAYLOAD)]
                           if decoder.get("runtime", {}).get("schema") == CONVERTER_SCHEMA
                           else [str(executable)])
                environment = {k: v for k, v in os.environ.items()
                               if not k.startswith(("LD_", "DYLD_")) and k != "GLIBC_TUNABLES"}
                result = subprocess.run(command + [str(wem), str(wav), str(root / CODEBOOK), str(V3_FRAMES)],
                                        stdout=output, stderr=subprocess.STDOUT, timeout=120,
                                        check=False, env=environment)
        except (OSError, subprocess.TimeoutExpired) as error:
            raise FormatError("MCC loading converter could not complete") from error
        if log.stat().st_size > 256 * 1024:
            raise FormatError("MCC loading converter produced excessive diagnostics")
        if result.returncode:
            raise FormatError("MCC loading converter failed: " + log.read_bytes()[-4096:].decode(errors="replace"))
        wav = _owned(wav, cache)
        if not wav.is_file() or wav.stat().st_size != 44 + V3_FRAMES * 8:
            raise FormatError("MCC loading converter returned an incomplete WAV")
        _v3_pcm(wav.read_bytes())
        if dict(converter=decoder, codebook=codebook) != converter_dependencies(root):
            raise FormatError("MCC loading decoder changed during conversion")
        archive = _owned(Path(source).parent / ARCHIVE, Path(source).parent)
        if (not archive.is_file() or (archive.stat().st_size, archive.stat().st_mtime_ns) !=
                (media["archive"]["bytes"], media["archive"]["mtime_ns"])):
            raise FormatError("MCC archive changed during loading conversion")
        metadata = dict(schema=V3_SCHEMA, profile=V3_PROFILE, complete=True,
            files={p.name: dict(bytes=p.stat().st_size, sha256=file_sha(p)) for p in bank.iterdir()},
            mask=dict(path="glow-mask.dds", source_width=512, source_height=512,
                      inputs=[glow_owner, logo_owner], authored_reconstruction=True, exact_vista_pixels=False),
            audio=dict(path=V3_WAV, encoding="ieee-float32-le", channels=2, sample_rate=44100,
                       decoded_frames=V3_FRAMES, decoded_pcm_sha256=V3_PCM_SHA, source=media,
                       converter=decoder, codebook=codebook, native_policy=V3_POLICY,
                       authored_frontend_timing=False),
            shader_programs="unchanged", animation="retained vista_loading_geometry/desktop_loading")
        _v3_metadata(metadata)
        (bank / "manifest.json").write_text(json.dumps(metadata, indent=2) + "\n")
        _publish_new(bank, target)
    return verify_v3(cache)


def prepare_v3(root, cache, source, *, progress=print):
    """Explicit V3 assembly only; broad cache provenance/selection is unchanged."""
    root, cache, source = Path(root).resolve(), Path(cache), Path(source)
    with _cancellation(), _lock(cache):
        return _prepare_v3(root, cache, source, progress)


def repair_v3(root, cache, source=None, *, progress=print):
    """Select V3 in broad provenance, preserving all legacy banks as references."""
    root, cache = Path(root).resolve(), Path(cache)
    with _cancellation(), _lock(cache):
        provenance = _owned(cache / "provenance.json", cache)
        before = provenance.read_bytes()
        value = json.loads(before)
        if not isinstance(value, dict) or value.get("external_installation_requirement") != "Halo MCC only":
            raise FormatError("V3 loading repair requires an owned MCC-only cache")
        previous_loading = value.get("native_loading_presentation")
        if previous_loading is not None and not isinstance(previous_loading, dict):
            raise FormatError("Invalid existing loading provenance; current files retained")
        source = Path(source if source is not None else value["source"])
        result = _prepare_v3(root, cache, source, progress)
        updated = {**value, "native_loading_presentation": result}
        if previous_loading is not None and previous_loading.get("profile") != V3_PROFILE:
            updated["legacy_loading_presentation_reference"] = previous_loading
        previous = updated.pop("port_bundled_loading_ambience", None)
        if previous is not None:
            updated["legacy_loading_ambience_reference"] = previous
        if provenance.read_bytes() != before:
            raise FormatError("MCC provenance changed during V3 loading conversion; verified bank retained unselected")
        if updated != value:
            from mcc_ui_layout import _atomic
            # A failed publication leaves an authenticated additive bank, but
            # neither cache selection nor existing provenance is upgraded.
            _atomic(provenance, (json.dumps(updated, indent=2) + "\n").encode())
        return cache, updated
