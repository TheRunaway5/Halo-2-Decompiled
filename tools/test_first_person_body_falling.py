#!/usr/bin/env python3
"""Run the native falling-body fixture and reject Vulkan validation output."""
import json
import hashlib
import os
from pathlib import Path
import re
import subprocess
import struct
import sys


def body_draw_diagnostics(scratch):
    """Reduce private draw/uniform/SPIR-V captures to bounded binding facts."""
    facts = []
    errors = []
    files = sorted(scratch.glob('body-draw-body-*.json'))
    if len(files) > 200:
        raise ValueError('Primary body draw observation exceeded its finite draw budget')
    for path in files:
        value = json.loads(path.read_text())
        push = bytes.fromhex(value.pop('camera_push_hex'))
        material = bytes.fromhex(value.pop('material_uniform_hex'))
        bank = bytes.fromhex(value.pop('vertex_bank_uniform_hex'))
        expected = bytes.fromhex(value.pop('expected_world_camera_hex'))
        eye = bytes.fromhex(value.pop('expected_eye_hex'))
        if len(push) != 112 or len(material) != 3712 or len(bank) != 2544 or len(expected) != 112 or len(eye) != 12:
            raise ValueError('Primary body actual draw observation lost a uniform ABI')
        camera = struct.unpack('<28f', push)
        control = struct.unpack_from('<4I', material, 3680)
        bank_range = struct.unpack_from('<4I', bank)
        if bank_range[3]:
            selected = bank[16:80]
            route = 'explicit c0..11 vertex bank'
        elif control[0]:
            if camera[23] == 3:
                selected = material[3504:3568]
                route = 'reflection source camera'
            elif camera[23] == 2:
                selected = material[3616:3680]
                route = 'foreground source camera'
            else:
                selected = material[3392:3456]
                route = 'ordinary WORLD source camera'
        else:
            selected = b''
            route = 'generic pushed projection'
        value.update(camera_push=list(camera), source_camera_control=list(control), vertex_bank_range=list(bank_range),
                     material_flags=list(struct.unpack_from('<4I', material, 288)),
                     material_uniform_sha256=hashlib.sha256(material).hexdigest(),
                     vertex_bank_uniform_sha256=hashlib.sha256(bank).hexdigest(),
                     effective_camera_route=route,
                     effective_c0_c3=list(struct.unpack('<16f', selected)) if selected else [],
                     expected_world_c0_c3=list(struct.unpack('<16f', expected[:64])),
                     expected_eye=list(struct.unpack('<3f', eye)),
                     camera_rows_match_expected=selected == expected[:64],
                     pushed_eye_matches_expected=push[80:92] == eye)
        for shader in value['shaders']:
            filename = shader.pop('file')
            if Path(filename).name != filename:
                raise ValueError('Primary body SPIR-V capture escaped private scratch')
            words = (scratch / filename).read_bytes()
            if len(words) % 4 or words[:4] != b'\x03\x02\x23\x07':
                raise ValueError('Primary body observed pipeline has invalid SPIR-V')
            shader.update(sha256=hashlib.sha256(words).hexdigest(), word_count=len(words) // 4)
        if value['body_material_match'] and (not value['camera_rows_match_expected'] or not value['pushed_eye_matches_expected'] or camera[23] != 1):
            errors.append(f"point {value['point']} draw {value['serial']} did not bind the submitted primary WORLD camera")
        facts.append(value)
    coverage = {point: max((value['primary_covered_occurrences'] for value in facts if value['point'] == point), default=0)
                for point in range(5)}
    if any(count != 2892 for count in coverage.values()):
        errors.append('Actual primary draw ranges did not cover all2892 submitted body vertices at each fractional point')
    return dict(draws=facts, primary_vertex_coverage=coverage, errors=errors,
                proof_boundary='Actual vkCmdDraw bindings and completed nondeferred GPU vertex readback; vertex-shader outputs and visible body fragments are not captured')


def main():
    if len(sys.argv) not in (5, 6) or (len(sys.argv) == 6 and sys.argv[5] not in ('--controlled-sweep', '--without-validation')):
        raise SystemExit('Supply FIXTURE MAP SHARED REPORT and optional --controlled-sweep or --without-validation')
    scratch = Path(os.environ['HALO2_TEST_SCRATCH'])
    log = scratch / 'falling-body-native.log'
    with log.open('w') as output:
        result = subprocess.run(sys.argv[1:], stdout=output, stderr=subprocess.STDOUT)
    diagnostics = []
    uploads = []
    tail = []
    with log.open(errors='replace') as output:
        for line in output:
            if line.startswith('Vulkan validation: '):
                diagnostics.append(line.rstrip())
            upload = re.fullmatch(r'Verified completed frame upload: vertices=(\d+) deferred=(\d+) verified=(\d+) fnv1a=(\d+)\n?', line)
            if upload:
                uploads.append(dict(zip(('vertices', 'deferred', 'verified', 'fnv1a'), map(int, upload.groups()))))
            tail.append(line.rstrip())
            if len(tail) > 40:
                tail.pop(0)
    path = Path(sys.argv[4])
    receipt = json.loads(path.read_text()) if path.is_file() else {'passed': False, 'error': 'Native fixture wrote no receipt'}
    receipt['native_exit_code'] = result.returncode
    receipt['vulkan_validation_diagnostics'] = len(diagnostics)
    receipt['vulkan_validation_messages'] = diagnostics[:20]
    receipt['native_log_tail'] = '\n'.join(tail)[-8000:]
    receipt['passed'] = receipt.get('passed', False) and result.returncode == 0 and not diagnostics
    if len(sys.argv) == 6 and sys.argv[5] == '--controlled-sweep':
        receipt['completed_vertex_uploads'] = uploads[-5:]
        receipt['completed_vertex_uploads_observed'] = len(uploads)
        body_vertices = receipt.get('primary_body_vertices_per_frame', 0)
        if not body_vertices or len(uploads) < 5 or any(upload['verified'] < body_vertices for upload in uploads[-5:]):
            if receipt['passed']:
                receipt['error'] = 'Controlled sweep omitted completed nondeferred GPU vertex readback at a fractional point'
            receipt['passed'] = False
            receipt['completed_vertex_uploads_insufficient'] = True
        try:
            receipt['primary_body_actual_draws'] = body_draw_diagnostics(scratch)
            if receipt['primary_body_actual_draws']['errors']:
                if receipt['passed']:
                    receipt['error'] = receipt['primary_body_actual_draws']['errors'][0]
                receipt['passed'] = False
        except (OSError, ValueError, KeyError, struct.error) as error:
            receipt['primary_body_actual_draws_error'] = str(error)
            receipt['passed'] = False
    path.write_text(json.dumps(receipt, indent=2) + '\n')
    print(receipt['native_log_tail'])
    if diagnostics:
        print('Native falling-body fixture emitted Vulkan validation diagnostics', file=sys.stderr)
    return 0 if receipt['passed'] else result.returncode or 1


if __name__ == '__main__':
    raise SystemExit(main())
