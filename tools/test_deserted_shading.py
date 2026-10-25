#!/usr/bin/env python3
"""Matched native facade regression and isolated baked-texture control.

Run with tools/run_test.py. Diagnostic maps, PPMs and driver caches are owned
scratch. Only requested PNG evidence and bounded receipts are retained.
"""
import argparse
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import struct
import subprocess

import numpy as np
from PIL import Image
from mcc_content import MccCache
from prepare_dust2_native import publish


def digest(path):
    with path.open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def seam_metric(path):
    """Fixed same-plane diagonal, excluding window/door/architecture edges.

    Compare a narrow step across the visible diagonal to the local texture
    gradient on either side. The local second difference discounts the
    underlying stucco pattern without comparing unrelated facade regions.
    """
    rgb = np.asarray(Image.open(path).convert('RGB'), dtype=float)
    ys = np.arange(260, 329)
    xs = np.rint(.575 * ys + 213).astype(int)
    def strip(offset):
        return np.array([rgb[y-1:y+2, x+offset-1:x+offset+2].mean()
                         for x, y in zip(xs, ys)])
    left, right = strip(-3), strip(3)
    local = .5 * ((left-strip(-9)) + (strip(9)-right))
    step = right-left-local
    score = float(abs(np.median(step)))
    facade_low = float(np.percentile(rgb[250:329, 310:460].mean(axis=2), 10))
    return dict(median_diagonal_step=score, threshold=6., facade_p10=facade_low,
                facade_min_p10=8., passed=score < 6. and facade_low > 8.,
                samples=len(ys), scope='CT-exit facade fixed diagonal pixel regression; independent full-view review required')


def run(root, runtime, control, output, imported, main_hud):
    scratch = Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    output.mkdir(parents=True, exist_ok=True)
    original = root/'content/halo2/Mods/deserted/deserted.map'
    cache_path = original
    shared = root/'content/halo2/h2_maps_win64_dx11'
    content = root/'content'
    changed = []
    if control != 'original':
        c = MccCache(original)
        if control == 'unique-faces':
            helper = root/'evidence/basis-audit/unique_faces_control.py'
            spec = importlib.util.spec_from_file_location('deserted_unique_faces_control', helper)
            module = importlib.util.module_from_spec(spec)
            spec.loader.exec_module(module)
            changed = module.unique_faces_control(c, root)
        if control == 'incident-compatible':
            helper = root/'evidence/lightmap-audit/reconcile_incident.py'
            private_incident = scratch/'incident-control.map'
            subprocess.run(['python3', str(helper), '--source', str(original),
                '--output', str(private_incident), '--report', str(output/'incident-compatible-data.json'),
                '--incident-compatible'], check=True)
            c = MccCache(private_incident)
            changed = json.loads((output/'incident-compatible-data.json').read_text())
        if control in ('world-gradient', 'gutter-only'):
            helper = root/'evidence/lightmap-audit/world_gradient.py'
            spec = importlib.util.spec_from_file_location('deserted_world_gradient_control', helper)
            module = importlib.util.module_from_spec(spec)
            spec.loader.exec_module(module)
            private_gradient = scratch/'world-gradient-control.map'
            module.make_world_gradient(original, private_gradient, output/(control+'-data.json'),
                                       gutter_only=control == 'gutter-only')
            c = MccCache(private_gradient)
            changed = json.loads((output/(control+'-data.json')).read_text())
        if control == 'flip-lightmap-v':
            bsp = next(t for t in c.tags.values() if t['group'] == 'sbsp')
            changed = []
            for section in c.block(bsp['offset']+156, 176)+c.block(bsp['offset']+312, 200):
                if not c.u16(section):
                    continue
                base = c.u32(section+40)+c.u32(section+48)+8
                for resource in c.block(section+56, 16):
                    if (c.u16(resource+4), c.u16(resource+6)) != (56, 3):
                        continue
                    offset, size = base+c.u32(resource+12), c.u32(resource+8)
                    values = np.frombuffer(c.data, dtype='<f4', count=size//4, offset=offset).reshape(-1, 2).copy()
                    values[:, 1] = 1-values[:, 1]
                    c.data[offset:offset+size] = values.tobytes()
                    changed.append(dict(section=section, vertices=len(values)))
        l = next(t for t in c.tags.values() if t['group'] == 'ltmp')
        for group in c.block(l['offset']+128, 104) if control == 'constant-atlas' else []:
            handle = c.u32(group+28)
            if handle not in c.tags:
                continue
            bitmap = c.tags[handle]
            assert bitmap['group'] == 'bitm'
            for at in c.block(bitmap['offset']+68, 168):
                w, h, fmt = (c.u16(at+x) for x in (4, 6, 12))
                assert fmt == 11 and c.u16(at+20) == 0
                rgba = bytes((96, 96, 96, 255))*w*h
                payload = struct.pack('<ii', 1, -len(rgba))+rgba
                c.data.extend(bytes((-len(c.data)) % 16))
                offset = len(c.data)
                c.data.extend(payload)
                struct.pack_into('<I', c.data, at+56, offset)
                struct.pack_into('<I', c.data, at+80, len(payload))
                changed.append(dict(width=w, height=h, format=fmt))
        assert changed
        struct.pack_into('<I', c.data, 8, len(c.data))
        private = scratch/(control+'.map')
        private.write_bytes(c.data)
        del c
        content = scratch/'content'
        publish(private, None, imported, content, scratch/'publication.json',
                main_hud, compiled_lighting=True, output_name='deserted')
        cache_path = content/'halo2/Mods/deserted/deserted.map'
        shared = content/'halo2/h2_maps_win64_dx11'
    script = scratch/'idle.input'
    script.write_text(''.join(f'{i} 0 0 0 0 0 -0.08 move 0 0\n' for i in range(30)))
    frame = scratch/'frame.ppm'
    command = [str(runtime), '--multiplayer', '--map', str(cache_path),
        '--shared-maps', str(shared), '--local-players', '1', '--custom-slayer',
        'fiesta', '--graphics-mode', 'Original', '--viewport', '1280', '720',
        '--mp-input-script', str(script), '--mp-capture-tick', '30',
        '--screenshot', str(frame), '--mp-reference-auxiliary-seed', '305419896',
        '--mp-reference-camera', '6.8', '17.64', '.45', '1', '0', '-.08', '0', '0', '1',
        '--mp-rule', '10', '0']
    env = dict(os.environ, __GL_SHADER_DISK_CACHE_PATH=str(scratch/'driver-cache'),
               MESA_SHADER_CACHE_DIR=str(scratch/'mesa-cache'))
    with (scratch/'capture.log').open('w') as log:
        result = subprocess.run(command, cwd=root, env=env, stdout=log,
                                stderr=subprocess.STDOUT, timeout=150)
    log = (scratch/'capture.log').read_text(errors='replace')
    (output/(control+'.log')).write_text(log[-65536:])
    assert result.returncode == 0 and frame.exists(), log[-3000:]
    png = output/(control+'.png')
    Image.open(frame).save(png)
    metric = seam_metric(png)
    report = dict(control=control, source_cache_sha256=digest(original),
        tested_cache_sha256=digest(cache_path), runtime_sha256=digest(runtime),
        command=command, scratch=str(scratch), changed_atlases=changed, metric=metric,
        retained_png=str(png), retained_bytes=png.stat().st_size)
    (output/(control+'.json')).write_text(json.dumps(report, indent=2)+'\n')
    print(json.dumps({key:report[key] for key in ('control','tested_cache_sha256','metric','retained_png','retained_bytes')}), flush=True)
    return 0 if metric['passed'] else 1


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('root', 'runtime', 'output', 'imported', 'main-hud'):
        parser.add_argument('--'+name, type=Path, required=True)
    parser.add_argument('--control', choices=('original', 'constant-atlas', 'unique-faces', 'incident-compatible', 'flip-lightmap-v', 'world-gradient', 'gutter-only'), default='original')
    raise SystemExit(run(**vars(parser.parse_args())))
