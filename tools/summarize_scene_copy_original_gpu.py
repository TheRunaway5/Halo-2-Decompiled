#!/usr/bin/env python3
"""Recompute private-session CPU evidence without opening disks, VM or capture replay."""
from pathlib import Path
import argparse, csv, hashlib, json, re, subprocess, sys
ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools'))
from differential_source_object_admission_snapshot import Snapshot, uint, words
from oracle_camouflage_scene_copy import decode_packets, OWNERS
from xbe import Xbe, FUNCTIONS_CSV

def sha(raw): return hashlib.sha256(raw).hexdigest()
def file_sha(path):
    h = hashlib.sha256()
    with path.open('rb') as f:
        for raw in iter(lambda: f.read(8 << 20), b''): h.update(raw)
    return h.hexdigest()
def run(args):
    p = subprocess.run(args, capture_output=True, text=True)
    return {'argv': args, 'returncode': p.returncode, 'stdout': p.stdout, 'stderr': p.stderr}
def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--session', type=Path, required=True)
    ap.add_argument('--output', type=Path, required=True)
    args = ap.parse_args(); session, out = args.session.resolve(), args.output.resolve()
    assert session.parent == ROOT / 'analysis/multiplayer/game-variants/scene-copy-original-gpu-20261012'
    out.mkdir(parents=True, exist_ok=False)
    snapshots = {}
    for name in ['before', 'copy-return', 'submitted']:
        s = Snapshot.__new__(Snapshot); s.cr3 = 0xf000
        s.ram = (session / name / 'physical-ram.bin').read_bytes(); snapshots[name] = s
    before, after = snapshots['before'], snapshots['copy-return']
    start, end = uint(before.read(0x404fe0, 4)), uint(after.read(0x404fe0, 4))
    raw = after.read(start, end - start); packets = decode_packets(raw)
    golden_path = ROOT / 'analysis/multiplayer/game-variants/camouflage-scene-copy-20261011/report.json'
    golden = json.loads(golden_path.read_text())
    assert packets == golden['result']['decoded_method_packets']
    assert raw.hex() == golden['result']['command_hex']
    (out / 'copy-commands.bin').write_bytes(raw)
    image_path = ROOT / 'assets/disc/default.xbe'; image = Xbe(image_path)
    rows = {int(row['va'], 16): row for row in csv.DictReader(open(FUNCTIONS_CSV))}
    owners = dict(OWNERS, submit=0x3fac30, visibility_flush=0x3fade0,
                  get_query=0x3fa880, queue_reference_query=0x3fa900,
                  reference_insert=0x3fa680, reference_wait=0x3fa970)
    code = []
    for name, address in owners.items():
        size = int(rows[address]['size']); original = image.read(address, size)
        matches = {label: s.read(address, size) == original for label, s in snapshots.items()}
        assert all(matches.values()), (name, matches)
        code.append({'name': name, 'address': address, 'bytes': size,
                     'sha256': sha(original), 'retail_bytes_match': matches})
    resource_records = []
    for label, s in snapshots.items():
        item = {'state': label, 'source_header_hex': s.read(0x7ff20000, 20).hex(),
                'target19_record_hex': s.read(0x4b56a0, 0x98).hex(),
                'pixel_payload_hex': s.read(0x484f68, 240).hex(),
                'screen_program_hex': s.read(0x801a194c, 116).hex(),
                'ram_backing_is_not_gpu_resident_pixel_proof': True}
        assert item['screen_program_hex'] == golden['screen_vertex_program']['program_hex']
        for role, address, size in [('source', 0x3db0000, 640 * 480 * 4),
                                     ('target19', 0x2b48000, 320 * 240 * 4)]:
            backing = s.ram[address:address + size]
            name = f'{label}-{role}-cpu-backing.bin'; (out / name).write_bytes(backing)
            item[role + '_cpu_backing'] = {'file': name, 'bytes': size,
                'physical_address': address, 'sha256': sha(backing)}
        resource_records.append(item)
    library = ROOT / '.tools/renderdoc/usr/lib/librenderdoc.so'
    replay = ROOT / '.tools/renderdoc/usr/bin/qrenderdoc'
    command = ROOT / '.tools/renderdoc/usr/bin/renderdoccmd'
    with (session / 'scene-copy_capture.rdc').open('rb') as f: header = f.read(32)
    renderdoc = {'capture_header_hex': header.hex(),
        'capture_header_build': header[16:32].split(b'\0')[0].decode(),
        'injected_library': {'path': str(library), 'sha256': file_sha(library)},
        'replay_executable': {'path': str(replay), 'sha256': file_sha(replay)},
        'command_executable': {'path': str(command), 'sha256': file_sha(command)},
        'replay_ldd': run(['ldd', str(replay)]),
        'replay_dynamic': run(['readelf', '-d', str(replay)]),
        'version': run([str(command), 'version']),
        'same_build': True,
        'same_build_evidence': 'Capture header1.45 2fc0bc; replay log full2fc0bc04cb95499635f63986a55bc6f67849dd9f; local executable RUNPATH and ldd resolve the identical preloaded library.'}
    preparation = json.loads((session / 'preparation.json').read_text())
    sources = []
    for row in preparation['source_files']:
        observed = file_sha(Path(row['source']))
        assert observed == row['sha256']
        sources.append({'path': row['source'], 'sha256': observed, 'unchanged': True})
    injected_sha = preparation['pinned_inputs'][str(library)]
    assert injected_sha == renderdoc['injected_library']['sha256']
    replay_result = json.loads((session / 'replay/report.json').read_text())
    report = {'schema': 'original-scene-copy-private-live-evidence-v1',
        'session': str(session.relative_to(ROOT)), 'tool_sha256': file_sha(Path(__file__)),
        'source_xbe': {'path': str(image_path), 'sha256': sha(image.data)},
        'source_files_unchanged': sources, 'unchanged_named_owner_ranges': code,
        'live_context': json.loads((session / 'live-command-context.json').read_text()),
        'copy': {'entry': 0x352e0, 'arguments': [0xffffffff, 1],
            'external_stack': 0xd00b3000, 'return_original_address': 0x391f0,
            'command_begin': start, 'command_end': end, 'command_bytes': len(raw),
            'command_sha256': sha(raw), 'packets': len(packets),
            'immutable_cpu_oracle': str(golden_path.relative_to(ROOT)),
            'immutable_cpu_oracle_sha256': file_sha(golden_path),
            'all_command_bytes_equal': True, 'whole_original_return_observed': True,
            'whole_trace_not_recorded_in_xemu': True,
            'restored_after_call': 'Saved original stack bytes,GPRs,EFLAGS,MXCSR; volatile XMM and empty x87 register residues retain original call results.',
            'instruction_patches': [], 'supplied_returns': [], 'readiness_replacements': []},
        'publication': {'actual_store_pc': 0x3fac58, 'device': 0x404fe0,
            'register_base': 0xfd800000, 'put_before': 0x03ef61e4,
            'put_written': 0x03efdf20, 'get_after_single_step': 0x03ef64dc,
            'later_put': 0x03f120d0, 'later_get': 0x03f120d0,
            'wait_entry': 0x3fa970, 'wait_target_reference': 0x20708f,
            'wait_return_address': 0x3f9da0, 'reference_completed_at_wait': 0x2070b1,
            'wait_target_precedes_copy_context_latest_reference': True,
            'scope': 'Original publication/GET progression and naturally returned original wait; not an identified fence after this copy draw or GPU framebuffer completion.'},
        'resources': resource_records, 'renderdoc': renderdoc,
        'capture': {'path': str((session / 'scene-copy_capture.rdc').relative_to(ROOT)),
            'bytes': (session / 'scene-copy_capture.rdc').stat().st_size,
            'sha256': replay_result['capture_sha256'], 'start_return': 1, 'end_return': 1,
            'observed_host_gl_thread': 37, 'observed_context': '555557D1E1B0',
            'replay': replay_result,
            'boundary': 'Same-build OpenCapture failed before action/resource enumeration: end-of-stream in MakeContextCurrent. No linked draw, sampled GPU source or before/after GPU destination export.'},
        'cleanup': json.loads((session / 'cleanup.json').read_text()),
        'limits': ['Declared external-call composition,not natural camouflage scheduling',
            'No GPU draw/pixel/framebuffer-center or physicalNV2A parity claim',
            'CPU backing bytes are retained RAM observations,not GPU resident surface exports',
            'No native changes or gamemode admission']}
    (out / 'report.json').write_text(json.dumps(report, indent=2, sort_keys=True) + '\n')
    print(json.dumps({'report': str(out / 'report.json'), 'sha256': file_sha(out / 'report.json'),
                      'owners': len(code), 'command_bytes': len(raw), 'packets': len(packets)}))

if __name__ == '__main__': main()
