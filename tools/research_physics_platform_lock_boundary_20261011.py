#!/usr/bin/env python3
"""Retain the actual first platform-lock call state without supplying a result."""
import argparse
import hashlib
import json
import zlib
from pathlib import Path
from oracle_smg_firing_1_1 import ROOT, Xbe, _Code, PIN
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    evidence = ROOT / 'analysis/multiplayer/sandbox-source/physics-world-bootstrap-runtime-20261011'
    checkpoint_path = evidence / 'disc-original-verified/checkpoint.json'
    fixture_path = evidence / 'disc-original-verified/fixture.json'
    pair_path = evidence / 'correspondence-v19/complete-report.json'
    assert sha(checkpoint_path) == '77ee5d4f83e9fb4db1b8b3966b363c683db936ec7ba19b50492e3be1a31b6b52'
    assert sha(fixture_path) == 'ab9249f2defb3ac9aee9829a0e28c2c216167ff2939873e4af1cae8ce7f849d4'
    assert sha(pair_path) == '7b2500b397173b0a250aed96be86b3a11848b3872fa9382d2a0e11daca6002ec'
    checkpoint = json.loads(checkpoint_path.read_text())
    fixture = json.loads(fixture_path.read_text())
    pair = json.loads(pair_path.read_text())
    case = fixture['cases'][0]
    assert not case['complete'] and case['exit_entry'] == 0x80000115
    memory = []
    for region in checkpoint['captured_entry']['regions']:
        path = checkpoint_path.parent / 'memory' / region['file']
        assert sha(path) == region['compressed_sha256']
        raw = zlib.decompress(path.read_bytes())
        assert hashlib.sha256(raw).hexdigest() == region['sha256'] and len(raw) == region['size']
        memory.append((region['start'], raw))
    changed = {}
    for write in case['writes']:
        for offset, value in enumerate(bytes.fromhex(write['written'])):
            changed[write['address'] + offset] = value

    def read(address, size):
        result = bytearray()
        for p in range(address, address + size):
            if p in changed:
                result.append(changed[p])
            else:
                matches = [raw[p - start] for start, raw in memory if start <= p < start + len(raw)]
                assert len(matches) == 1, ('Boundary input lacks actual backing', hex(p))
                result.append(matches[0])
        return bytes(result)

    sp = case['exit']['gp'][4]
    return_address = int.from_bytes(read(sp, 4), 'little')
    critical = int.from_bytes(read(sp + 4, 4), 'little')
    old_path = ROOT / 'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03'
    assert sha(old_path) == PIN and sha(NEW_DEFAULT) == NEW_PIN
    old, new = Xbe(old_path), Xbe(NEW_DEFAULT)
    oc, nc = _Code(old, '.text'), _Code(new, '.text')
    site = case['instructions'][-1]
    later_site = pair['instruction_map'][f'{site:08x}']
    left, right = oc.at(site), nc.at(later_site)
    assert left.mnemonic == right.mnemonic and left.mnemonic in ('call', 'jmp')
    assert left.operands[0].type == right.operands[0].type == 3
    assert not left.operands[0].mem.base and not right.operands[0].mem.base
    old_iat, later_iat = left.operands[0].mem.disp, right.operands[0].mem.disp
    assert int.from_bytes(read(old_iat, 4), 'little') == 0x80000115
    assert int.from_bytes(new.read(later_iat, 4), 'little') == 0x80000115
    primary_directory = ROOT / 'analysis/multiplayer/integration/spawn-source/kernel-sync-world'
    original_report = json.loads((primary_directory / 'report.json').read_text())
    primary = original_report['compiled_compatibility_initializer']
    assert sha(primary_directory / 'sources.json') == primary['source_manifest_sha256']
    for source in primary['primary_sources']:
        assert sha(primary_directory / source['saved']) == source['sha256']
    result = dict(schema=1, tool_sha256=sha(__file__), checkpoint_sha256=sha(checkpoint_path),
        fixture_sha256=sha(fixture_path), correspondence_sha256=sha(pair_path),
        original_sha256=PIN, later_original_sha256=NEW_PIN, ordinal=277,
        source_call=dict(old_site=site, later_site=later_site, old_bytes=left.bytes.hex(),
            later_bytes=right.bytes.hex(), old_import_operand=old_iat, later_import_operand=later_iat),
        actual_callee_entry_abi=case['exit'], actual_return_address=return_address,
        actual_stack_pointer=sp, actual_argument_pointer=critical,
        actual_argument28_hex=read(critical, 28).hex(),
        critical_section_layout_source_manifest_sha256=primary['source_manifest_sha256'],
        scope='Exact original pre-import input only. The28-byte layout belongs to the pinned compatibility implementation, not recovered original Xbox kernel bytes.',
        kernel_executed=False, kernel_result_supplied=False, host_context_is_hardware_xbox=False,
        native_platform_owner_implemented=False, live_gameplay_proof=False)
    args.output.mkdir(parents=True, exist_ok=False)
    path = args.output / 'report.json'
    path.write_text(json.dumps(result, sort_keys=True, indent=2) + '\n')
    print(json.dumps(dict(report_sha256=sha(path), ordinal=277, call=hex(site),
        original_return=hex(return_address), actual_argument=hex(critical))))


if __name__ == '__main__':
    main()
