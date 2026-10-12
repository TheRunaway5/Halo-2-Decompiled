"""CPU-only decode of unchanged original20 replay depth/stencil snapshots."""
from pathlib import Path
import collections
import hashlib
import json
import struct

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / 'analysis/multiplayer-shading/original20-sky-stencil-analysis'
SOURCE = ROOT / 'analysis/multiplayer-shading/renderdoc-source/vk_replay.cpp'


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def f32(value):
    return struct.unpack('<f', struct.pack('<f', value))[0]


def blob(name, data):
    path = OUT / name
    path.write_bytes(data)
    return dict(file=str(path.relative_to(ROOT)), bytes=len(data), sha256=sha(path))


def main():
    OUT.mkdir(parents=True, exist_ok=True)
    source = SOURCE.read_text()
    # This is the source of the replay readback format, not a claim about the
    # title's native allocation or the separate OpenGL readback word layout.
    assert '*dst = (*dSrc & 0x00ffffff) | (uint32_t(*sSrc) << 24)' in source
    assert 'dataSize += (size_t)GetByteSize' in source
    inputs = [SOURCE, Path(__file__)]
    reports = [ROOT / 'analysis/multiplayer-shading/original20-sky-stencil/report.json',
               ROOT / 'analysis/multiplayer-shading/original20-moon-stencil/report.json']
    rows = []
    events = []
    all_controls = 0
    for report_path in reports:
        report = json.loads(report_path.read_text())
        assert report['status'] == 'complete' and report['capture_unchanged'] and not report['guest_IO']
        inputs.append(report_path)
        capture = ROOT / report['capture']
        assert sha(capture) == report['capture_sha256']
        if capture not in inputs:
            inputs.append(capture)
        snapshots = {}
        for snapshot in report['snapshots']:
            path = report_path.parent / snapshot['file']
            assert sha(path) == snapshot['sha256']
            inputs.append(path)
            raw = path.read_bytes()
            width, height = snapshot['width'], snapshot['height']
            count = width * height
            assert len(raw) == 5 * count
            packed = raw[:4*count]
            # RenderDoc reserves separate aspect-copy space, then repacks only
            # the first four bytes per pixel. Its unused tail is zero here.
            assert not any(raw[4*count:])
            words = struct.unpack(f'<{count}I', packed)
            depth = tuple(word & 0xffffff for word in words)
            stencil = bytes(word >> 24 for word in words)
            for control in snapshot['pixel_controls']:
                index = control['y'] * width + control['x']
                assert f32(depth[index] / 0xffffff) == control['floatValue'][0]
                assert f32(stencil[index] / 255) == control['floatValue'][1]
                all_controls += 1
            event = snapshot.get('draw_event', 8110)
            phase = snapshot['phase']
            row = dict(event=event, phase=phase, width=width, height=height,
                pixel_count=count, packed=blob(f'{event}-{phase}-packed-d24s8.bin', packed),
                depth_low24=blob(f'{event}-{phase}-depth-low24.bin', struct.pack(f'<{count}I', *depth)),
                stencil=blob(f'{event}-{phase}-stencil.bin', stencil),
                stencil_histogram={str(k): v for k, v in sorted(collections.Counter(stencil).items())},
                depth_min=min(depth), depth_max=max(depth), depth_unique=len(set(depth)),
                verified_pick_pixel_controls=len(snapshot['pixel_controls']))
            rows.append(row)
            snapshots.setdefault(event, {})[phase] = (depth, stencil, packed)
        for event, pair in snapshots.items():
            before, after = pair['before'], pair['after']
            changed = [i for i, (a,b) in enumerate(zip(before[1],after[1])) if a!=b]
            bounds = None
            if changed:
                xs = [i % width for i in changed]; ys = [i // width for i in changed]
                bounds = [min(xs),min(ys),max(xs),max(ys)]
            events.append(dict(event=event,
                depth_changed_pixels=sum(a!=b for a,b in zip(before[0],after[0])),
                stencil_changed_pixels=len(changed), stencil_changed_bounds_inclusive=bounds,
                stencil_transitions={f'{a:02X}->{b:02X}':n for (a,b),n in
                    sorted(collections.Counter((before[1][i],after[1][i]) for i in changed).items())},
                packed_changed_bytes=sum(a!=b for a,b in zip(before[2],after[2]))))
    hashes = {str(path.relative_to(ROOT)):sha(path) for path in inputs}
    result = dict(scope=__doc__, source_replay_format=str(SOURCE.relative_to(ROOT)),
        replay_format='first width*height little-endian DWORDs: D24 low24, S8 high8; remaining zero tail unused',
        pick_pixel_controls=all_controls, snapshots=rows, events=events, input_sha256=hashes,
        limits=['Original20 unchanged Vulkan replay, not physical Xbox precision.',
                'The intervals do not identify earlier clears or later/all-map stencil history.',
                'The separate Lockout OpenGL stencil population is not attributed to these sky draws.',
                'No native renderer, runtime, shader, CMake, guest or GPU action in this CPU tool.'])
    assert all(sha(ROOT/name)==value for name,value in hashes.items())
    (OUT/'report.json').write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps(dict(controls=all_controls,events=events,report_sha256=sha(OUT/'report.json')),indent=2))


if __name__=='__main__':
    main()
