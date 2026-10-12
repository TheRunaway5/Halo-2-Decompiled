#!/usr/bin/env python3
"""Read-only inventory of existing scene-copy GPU-oracle prerequisites.

Only files are read. No process/socket/debugger/QMP/RenderDoc/VM is operated;
no disk, EEPROM, configuration or existing capture is written. QCOW snapshot
names are raw metadata observations, not a restore test or proof of GPU state.
"""
import argparse
import hashlib
import json
import struct
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / 'analysis/multiplayer/game-variants/scene-copy-gpu-artifacts-20261012/report.json'
SESSIONS = [
    'analysis/multiplayer-shading/original-weather-20261009/session1',
    'analysis/multiplayer-shading/registration-snapshot-restore-20261008',
    'analysis/multiplayer-shading/original-xemu',
    'analysis/multiplayer-shading/original-clear-scheduling/original-session',
    'analysis/multiplayer-shading/world-effects-goal-20261011/original-session',
]
WEATHER = Path(SESSIONS[0])
CAPTURES = [
    WEATHER / 'original-lockout-gl-source_capture.rdc',
    WEATHER / 'original-lockout-gl_frame13537.rdc',
    Path(SESSIONS[2]) / 'original_capture_20.rdc',
]
SMALL = [
    WEATHER / name for name in (
        'launch.json', 'xemu.toml', 'preparation.json',
        'source-gl-capture-provenance.json', 'gl-manual-load-and-run.json',
        'manual-source-capture-guest-run.json', 'host-gl-start.gdb',
        'export-source-snow-surfaces.py', 'export-source-snow-draw.py',
        'replay-source-summary.py', 'lockout-weather-particle-entry/export.json',
        'lockout-weather-particle-entry/qmp-export.json',
        'source-snow-event215873/surfaces.json')
] + [Path('tools/xemu_gl_capture_control.c'),
     Path('analysis/multiplayer/game-variants/camouflage-scene-copy-20261011/report.json'),
     Path('analysis/multiplayer/game-variants/scene-copy-rectangles-20261011/report.json')]
SOURCES = [
    'hw/xbox/nv2a/pgraph/pgraph.c', 'hw/xbox/nv2a/pgraph/gl/surface.c',
    'hw/xbox/nv2a/pgraph/gl/texture.c', 'hw/xbox/nv2a/pgraph/vk/renderer.c',
    'hw/xbox/nv2a/pgraph/debug_renderdoc.c',
    'hw/xbox/nv2a/pgraph/glsl/vsh-prog.c', 'hw/xbox/nv2a/pgraph/glsl/vsh.c',
    'ui/xemu-snapshots.c',
]


def digest(path):
    h = hashlib.sha256()
    with path.open('rb') as f:
        while data := f.read(1024 * 1024):
            h.update(data)
    return h.hexdigest()


def identity(relative):
    path = ROOT / relative
    before = path.stat()
    result = dict(path=str(relative), bytes=before.st_size, sha256=digest(path))
    after = path.stat()
    assert (before.st_size, before.st_mtime_ns) == (after.st_size, after.st_mtime_ns), path
    return result


def qcow(relative):
    path = ROOT / relative
    before = path.stat()
    with path.open('rb') as f:
        header = f.read(104)
        magic, version, backing_at, backing_size, cluster_bits, virtual_size, crypt, l1n, l1at, refat, refn, count, at = struct.unpack('>IIQIIQIIQQIIQ', header[:72])
        assert magic == 0x514649fb and version in (2, 3) and count < 1000
        f.seek(backing_at)
        backing = f.read(backing_size).decode('utf8') if backing_size else None
        rows = []
        for _ in range(count):
            f.seek(at)
            raw = f.read(40)
            snapshot_l1, snapshot_l1n, id_size, name_size, seconds, nseconds, clock, state_size, extra_size = struct.unpack('>QIHHIIQII', raw)
            assert extra_size < 65536 and id_size < 65536 and name_size < 65536
            extra = f.read(extra_size)
            ident, name = f.read(id_size), f.read(name_size)
            rows.append(dict(id=ident.decode('utf8'), name=name.decode('utf8'),
                             vmstate_size=struct.unpack_from('>Q', extra)[0] if extra_size >= 8 else state_size,
                             snapshot_table_offset=at, metadata_sha256=hashlib.sha256(raw + extra + ident + name).hexdigest()))
            at = (at + 40 + extra_size + id_size + name_size + 7) & ~7
    after = path.stat()
    assert (before.st_size, before.st_mtime_ns) == (after.st_size, after.st_mtime_ns), path
    return dict(path=str(relative), file_bytes=before.st_size, version=version,
                virtual_bytes=virtual_size, backing_path=backing, snapshots=rows,
                header_sha256=hashlib.sha256(header).hexdigest(),
                boundary='Read-only raw QCOW metadata; no full disk hash, restore or saved device-content verification.')


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--output', type=Path, default=OUT)
    args = ap.parse_args()
    disks = [qcow(Path(directory) / name) for directory in SESSIONS
             for name in ('base-hdd.qcow2', 'session-hdd.qcow2')]
    captures = []
    for relative in CAPTURES:
        row = identity(relative)
        summary = relative.with_suffix('.json')
        j = json.loads((ROOT / summary).read_text())
        row.update(summary=identity(summary), action_count=len(j.get('actions', [])),
                   four_index_actions=sum(a.get('indices') == 4 for a in j.get('actions', [])),
                   texture_count=len(j.get('textures', [])),
                   textures320x240=[t for t in j.get('textures', []) if (t.get('width'), t.get('height')) == (320, 240)],
                   textures640x480=[t for t in j.get('textures', []) if (t.get('width'), t.get('height')) == (640, 480)],
                   boundary='Existing summary only; no new replay or pipeline/resource/event identification.')
        captures.append(row)
    source_root = ROOT / 'tools/reference/xemu-0.8.136'
    tree = json.loads((source_root / 'tree.json').read_text())
    assert tree['sha'] == 'fc24584ce88f0915ad7f04775bb7712c2e3f49ee'
    sources = []
    for relative in SOURCES:
        path = source_root / relative
        raw = path.read_bytes()
        blob = hashlib.sha1(b'blob ' + str(len(raw)).encode() + b'\0' + raw).hexdigest()
        expected = next(row['sha'] for row in tree['tree'] if row['path'] == relative)
        assert blob == expected
        sources.append(dict(**identity(path.relative_to(ROOT)), git_blob=blob, commit=tree['sha']))
    config = ROOT / WEATHER / 'xemu.toml'
    import tomllib
    files = tomllib.loads(config.read_text())['sys']['files']
    runtime = []
    for role, value in files.items():
        p = Path(value)
        runtime.append(dict(role=role, path=str(p), exists=p.is_file(), bytes=p.stat().st_size if p.is_file() else None,
                            boundary='Existence/size only; no firmware/disc execution or content verification.'))
    for role, relative in [('xemu', '.tools/xemu-app/squashfs-root/usr/bin/xemu'),
                           ('renderdoc_library', '.tools/renderdoc/usr/lib/librenderdoc.so')]:
        p = ROOT / relative
        runtime.append(dict(role=role, path=str(p), exists=p.is_file(), bytes=p.stat().st_size if p.is_file() else None,
                            boundary='Existence/size only; not executed or loaded.'))
    report = dict(schema='scene-copy-gpu-artifact-inventory-v1', boundary=__doc__,
                  disks=disks, captures=captures, artifacts=[identity(p) for p in SMALL],
                  sources=sources, runtime_paths=runtime, tool=identity(Path(__file__).resolve().relative_to(ROOT)),
                  observations=['Retained Lockout baseline/outdoor VM snapshot metadata exists in independent copied sessions; no restore is tested.',
                                'Inspected saved summaries have no320x240 texture row; no existing target19 mode1 draw/event is identified.',
                                'CPU oracle source/target backing hashes describe RAM, not a same-call GPU-resident scene or completed destination.',
                                'Existing actual GL capture and source export scripts establish a capture/readback mechanism, not the missing copy event.',
                                'Pinned PGRAPH delegates save hooks; local Vulkan renderer requests/waits dirty surface download. Exact GL save hook and full NV2A VMState serialization owners are absent from this selected local source set.',
                                'Original SDK command buffer emission is not DMA submission/completion; future private-session proof must capture real consumption and before/after resources.'],
                  process_operations=[], disk_writes=[], eeprom_writes=[], vm_launches=[], instruction_patches=[], supplied_returns=[])
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, sort_keys=True, indent=2) + '\n')
    print(json.dumps(dict(disks=len(disks), captures=len(captures), sources=len(sources),
                          snapshots=[s['name'] for s in disks[1]['snapshots']],
                          report_sha256=digest(args.output)), indent=2))


if __name__ == '__main__':
    main()
