#!/usr/bin/env python3
"""Compare read-only Rust admission with the unchanged TU1.5 variant validator.

Synthetic buffers and NULL locale globals are explicit inputs. Validator,
normalizer, reset initializer and all called instructions execute unchanged.
Matchmaking consumes only validity; it never substitutes the reset game.
Run through tools/run_test.py. Compiler outputs stay in HALO2_TEST_SCRATCH.
"""
import argparse
from collections import Counter
import hashlib
import json
import os
from pathlib import Path
import shutil
import struct
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path[:0] = [str(ROOT / 'tools'), str(ROOT / 'tools/reference/halo2-decompiled/tools')]
from oracle_game_variants import Source, VARIANT
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN
from unicorn import UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_EAX


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--report-output', type=Path, required=True)
    args = parser.parse_args()
    scratch = Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    paths = {
        'retained': ROOT / 'tests/fixtures/game_variant_normalization_original.tsv',
        'presets': ROOT / 'rust/online_protocol/data/tu15-result-presets.json',
        'catalog': ROOT / 'multiplayer/master_server/examples/matchmaking-online.json',
        'rust': ROOT / 'rust/online_protocol/src/matchmaking_variant.rs',
    }
    pins = {name: sha(path) for name, path in paths.items()}
    cases = []
    for number, line in enumerate(paths['retained'].read_text().splitlines(), 1):
        if line.startswith('N '):
            _, locale, label, before, after, expected = line.split()
            cases.append(('retained', str(number), before, bool(int(expected))))
    assert len(cases) == 4249
    presets = json.loads(paths['presets'].read_text())
    assert presets['original_code_sha256'] == NEW_PIN and len(presets['variants']) == 39
    for index, row in enumerate(presets['variants']):
        cases.append(('presets', str(index), row['settings'], True))
    catalog = json.loads(paths['catalog'].read_text())
    for playlist in catalog['playlists']:
        for game in playlist['games']:
            if game.get('settings') is not None:
                cases.append(('catalog', playlist['id'] + '/' + game['id'], game['settings'], True))
    source = Source(dict(path=str(NEW_DEFAULT), pin=NEW_PIN, globals=0x4ecc1c))
    executed = {}
    invocations = [0]

    def verify(memory, address, size, unused):
        assert bytes(memory.mem_read(address, size)) == source.image.read(address, size)
        executed[address] = size
        invocations[0] += 1

    source.g.hook_add(UC_HOOK_CODE, verify)
    answers = []
    digest = hashlib.sha256()
    for index, (group, label, encoded, expected) in enumerate(cases):
        raw = bytes.fromhex(encoded)
        assert len(raw) == 304
        source.g.mem_write(VARIANT, raw)
        valid = bool(source.call(0x1a7140, registers=((UC_X86_REG_EAX, VARIANT),)) & 255)
        assert bytes(source.g.mem_read(VARIANT, 304)) == raw, ('Original mutated caller', group, label)
        assert valid == expected, ('Original source contract changed', group, label, valid, expected)
        answers.append(valid)
        digest.update(json.dumps([group, label, hashlib.sha256(raw).hexdigest(), valid]).encode() + b'\n')
        if index and index % 1000 == 0:
            print(json.dumps(dict(original_controls=index)), flush=True)
    module = scratch / 'matchmaking_variant.rs'
    shutil.copyfile(paths['rust'], module)
    driver = scratch / 'probe.rs'
    driver.write_text('#[path=' + json.dumps(str(module)) + '] mod variant;\n' + r'''
use std::io::{self, Read};
fn main() {
    let mut text=String::new();io::stdin().read_to_string(&mut text).unwrap();
    for line in text.lines() {
        let raw:Vec<u8>=line.as_bytes().chunks_exact(2).map(|p|u8::from_str_radix(std::str::from_utf8(p).unwrap(),16).unwrap()).collect();
        println!("{}",u8::from(variant::valid(&raw)));
    }
}
''')
    version = subprocess.check_output(['rustc', '--version', '--verbose']).decode()
    assert '\nrelease: 1.96.0\n' in version
    binary = scratch / 'variant-probe'
    command = ['rustc', '--edition=2024', '--crate-name', 'variant_probe', '-O', str(driver), '-o', str(binary)]
    subprocess.run(command, check=True)
    output = subprocess.check_output([str(binary)], input=('\n'.join(c[2] for c in cases) + '\n').encode()).decode().splitlines()
    assert len(output) == len(cases)
    for case, original, actual in zip(cases, answers, output):
        assert actual in ('0', '1') and bool(int(actual)) == original, ('Rust/original mismatch', case[:2], original, actual)
    assert all(sha(paths[name]) == pin for name, pin in pins.items())
    report = dict(schema='H2_TU15_MATCHMAKING_VARIANT_ORIGINAL_V1', passed=True,
                  original_xbe_sha256=NEW_PIN, validator='0x1a7140', normalizer='0x1a7170',
                  no_locale_global='0x4ecc1c', controls=len(cases), groups=dict(Counter(c[0] for c in cases)),
                  original_valid=sum(answers), original_invalid=len(answers)-sum(answers),
                  controls_sha256=digest.hexdigest(), input_pins={name:dict(path=str(paths[name]),sha256=pin) for name,pin in pins.items()},
                  all_executed_instruction_bytes_verified=True, original_instructions=len(executed),
                  instruction_invocations=invocations[0], original_calls=len(cases),
                  original_executed_bytes_sha256=hashlib.sha256(b''.join(struct.pack('<IH',at,size)+bytes(source.g.mem_read(at,size)) for at,size in sorted(executed.items()))).hexdigest(),
                  instruction_patches=[], replaced_callees=[], caller_buffers_unchanged=True,
                  rustc=version.strip(), compiler_command=command, tool_sha256=sha(Path(__file__)),
                  scratch=str(scratch), retained_large_artifacts=[], full_native_flow_qualified=False,
                  scope=__doc__)
    args.report_output.parent.mkdir(parents=True, exist_ok=True)
    encoded=json.dumps(report,indent=2)+'\n';assert len(encoded.encode())<=1048576
    args.report_output.write_text(encoded)
    print(json.dumps(report), flush=True)


if __name__ == '__main__':
    main()
