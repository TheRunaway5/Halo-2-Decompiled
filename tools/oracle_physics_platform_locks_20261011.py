#!/usr/bin/env python3
"""Independent pinned compatibility lock transitions, not original kernel execution."""
import argparse
import hashlib
import json
import struct
import threading
from pathlib import Path
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn import x86_const as R

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / 'analysis/multiplayer/integration/spawn-source'
REPORT_PIN = '4cff6712198ba763550115d184030d25f69eeb46b414b2a5e73f229e05d2a7f8'
BOUNDARY_PIN = 'ddbf22c6cf09518f34f55e4f0a1628e9a9804a0ec62bc12b312c984ef24973f7'


def sha(p):
    return hashlib.sha256(Path(p).read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    tool_pin = sha(__file__)
    report_path = SOURCE / 'host-lock-new-map-order/report.json'
    assert sha(report_path) == REPORT_PIN
    report = json.loads(report_path.read_text())
    locks = report['compiled_compatibility_locks']
    initializer_report_path = SOURCE / 'kernel-sync-world/report.json'
    initializer_report_pin = 'ad0fb0b253d9fbf5b8c40b79ec2a4c142271be284c3570656b8325a7e9b92c54'
    assert sha(initializer_report_path) == initializer_report_pin
    initial_report = json.loads(initializer_report_path.read_text())
    initial = initial_report['compiled_compatibility_initializer']
    assert sha(SOURCE / 'kernel-sync-world/sources.json') == locks['source_manifest_sha256'] == initial['source_manifest_sha256']
    for row in initial['primary_sources']:
        assert sha(SOURCE / 'kernel-sync-world' / row['saved']) == row['sha256']
    lock_path = SOURCE / 'host-lock-new-map-order/locks.bin'
    init_path = SOURCE / 'kernel-sync-world/initializer.bin'
    assert sha(lock_path) == locks['compiled_text_sha256']
    assert sha(init_path) == initial['compiled_text_sha256']
    assert sha(SOURCE / 'host-lock-new-map-order/locks.cpp') == locks['generated_Cpp_sha256']
    assert sha(SOURCE / 'kernel-sync-world/initializer.c') == initial['generated_C_sha256']
    boundary_path = ROOT / 'analysis/multiplayer/sandbox-source/physics-platform-lock-runtime-20261011/boundary-v1/report.json'
    assert sha(boundary_path) == BOUNDARY_PIN
    boundary = json.loads(boundary_path.read_text())
    # Keep two real host threads alive concurrently. Only equality tokens cross
    # the platform seam; no current-thread structure or guest FS fields appear.
    host_ids = []
    ready, finish = threading.Event(), threading.Event()

    def second_thread():
        host_ids.append(threading.get_native_id())
        ready.set()
        finish.wait()

    thread = threading.Thread(target=second_thread)
    thread.start()
    ready.wait()
    assert host_ids[0] != threading.get_native_id()
    identities = [0x96f0000, 0x96f0004]
    rows = []
    try:
        for address in (boundary['actual_argument_pointer'], 0x2100210, 0x2100fe4):
            g = Uc(UC_ARCH_X86, UC_MODE_32)
            for p, size in ((0x9000000, 0x2000), (0x9400000, 0x2000), (0x9500000, 0x1000),
                            (0x2100000, 0x3000), (0x2200000, 0x3000), (0x8000000, 0x10000)):
                g.mem_map(p, size)
            g.mem_write(0x9000000, init_path.read_bytes())
            g.mem_write(0x9400000, lock_path.read_bytes())
            stop = 0x2202000
            current = dict(token=identities[0], boundary=None)

            def hook(_g, p, _size, _user):
                if p == stop:
                    g.emu_stop()
                    return
                if not 0x9500000 <= p < 0x9500100:
                    return
                sp = g.reg_read(R.UC_X86_REG_ESP)
                ret = struct.unpack('<I', g.mem_read(sp, 4))[0]
                n = (p - 0x9500000) // 16
                if n == 0:
                    result, cleanup = current['token'], 0
                elif n in (1, 2):
                    target = struct.unpack('<I', g.mem_read(sp + 4, 4))[0]
                    before = struct.unpack('<I', g.mem_read(target, 4))[0]
                    result = (before + (1 if n == 1 else -1)) & 0xffffffff
                    g.mem_write(target, struct.pack('<I', result))
                    cleanup = 4
                else:
                    assert n in (3, 4), 'Unexpected compatibility abort'
                    target = struct.unpack('<I', g.mem_read(sp + 4, 4))[0]
                    arguments = list(struct.unpack('<' + 'I' * (5 if n == 3 else 3),
                        g.mem_read(sp + 4, 20 if n == 3 else 12)))
                    assert arguments == ([address, 0, 0, 0, 0] if n == 3 else [address, 1, 0])
                    current['boundary'] = 'wait' if n == 3 else 'wake'
                    g.emu_stop()
                    return
                g.reg_write(R.UC_X86_REG_EAX, result)
                g.reg_write(R.UC_X86_REG_ESP, sp + 4 + cleanup)
                g.reg_write(R.UC_X86_REG_EIP, ret)

            g.hook_add(UC_HOOK_CODE, hook)

            def operation(ordinal, token):
                current.update(token=token, boundary=None)
                before = bytes(g.mem_read(address, 28)).hex()
                sp = 0x2201000
                g.mem_write(sp, struct.pack('<II', stop, address))
                g.reg_write(R.UC_X86_REG_ESP, sp)
                entry = int(initial['entry'], 16) if ordinal == 291 else int(locks['entries'][
                    'RtlEnterCriticalSection' if ordinal == 277 else 'RtlLeaveCriticalSection'], 16)
                g.emu_start(entry, 0, count=10000)
                assert current['boundary'] is not None or g.reg_read(R.UC_X86_REG_EIP) == stop
                rows.append(dict(address=address, ordinal=ordinal, token=token,
                    before_hex=before, after_hex=bytes(g.mem_read(address, 28)).hex(),
                    boundary=current['boundary']))

            # Actual heap lock input remains independently retained, not a
            # native output or a replacement for the owning heap constructor.
            if address == boundary['actual_argument_pointer']:
                g.mem_write(address, bytes.fromhex(boundary['actual_argument28_hex']))
                operation(277, identities[0])
                operation(294, identities[0])
            for fill in (0, 0xa5, 0xff):
                g.mem_write(address, bytes([fill]) * 28)
                operation(291, identities[0])
                for token in identities:
                    for depth in (1, 2, 8, 32):
                        for _ in range(depth):
                            operation(277, token)
                        for _ in range(depth):
                            operation(294, token)
                operation(277, identities[0])
                operation(277, identities[1])  # Stops before unprovided wait.
                operation(294, identities[0])  # Stops before unprovided wake.
                # Preserve original 32-bit underflow instead of adding an
                # unsupported ownership check to the leave operation.
                operation(294, identities[0])
    finally:
        finish.set()
        thread.join()
    assert sha(__file__) == tool_pin and sha(report_path) == REPORT_PIN
    assert sha(initializer_report_path) == initializer_report_pin
    assert sha(lock_path) == locks['compiled_text_sha256'] and sha(init_path) == initial['compiled_text_sha256']
    for row in initial['primary_sources']:
        assert sha(SOURCE / 'kernel-sync-world' / row['saved']) == row['sha256']
    result = dict(schema=1, tool_sha256=tool_pin, prior_report_sha256=REPORT_PIN,
        initializer_report_sha256=initializer_report_pin,
        actual_boundary_sha256=BOUNDARY_PIN, source_manifest_sha256=locks['source_manifest_sha256'],
        compiled_lock_sha256=sha(lock_path), compiled_initializer_sha256=sha(init_path),
        real_distinct_host_threads_verified=True, opaque_equality_tokens=identities,
        original_kernel_execution=False, original_kernel_register_clobbers_proven=False,
        wait_wake_results_supplied=False, live_gameplay_proof=False, cases=rows,
        scope='Pinned compatibility implementation field transitions and missing-service boundaries. Real identity inputs are opaque tokens; no Xbox thread records, native outputs, kernel waits or successful kernel returns are supplied.')
    args.output.mkdir(parents=True, exist_ok=False)
    path = args.output / 'fixture.json'
    path.write_text(json.dumps(result, sort_keys=True, indent=2) + '\n')
    print(json.dumps(dict(cases=len(rows), wait_boundaries=sum(r['boundary'] == 'wait' for r in rows),
        wake_boundaries=sum(r['boundary'] == 'wake' for r in rows), fixture_sha256=sha(path))))


if __name__ == '__main__':
    main()
