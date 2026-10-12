#!/usr/bin/env python3
"""Compare native sound variation kernels against frozen original instructions."""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import subprocess

ROOT=Path(__file__).resolve().parents[1]
BASE=ROOT/'analysis/multiplayer/map-pack/smg-audio-adapter'
def digest(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def require(value,message):
    if not value:raise ValueError(message)
def state(record):
    r=record['ranges'][0]
    return [record['local_seed'],r['range_runtime_hex'],r['runtime_flag_hex']]
def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--binary',type=Path,default=ROOT/'build-multiplayer/sound_variation_tests')
    p.add_argument('--output',type=Path,default=ROOT/'analysis/multiplayer/integration/sound-variation-differential.json')
    a=p.parse_args()
    pins={BASE/'source-fixture.json':'525ed0246e4271a14b23c8148739010f18e1c28955889631cc35ed6781300a22',
        BASE/'frozen/suite.json':'007109dc2ec4447c016b2c604856f9c2bbc0e4f051b7c8004475213d628ec34a',
        BASE/'audit.json':'1af37ba841eb0b983bb3bea7b23d1880fd47ed5c38c8f9f49c521ff283618146',
        ROOT/'tools/oracle_smg_audio_adapter_1_1.py':'5e0a4af31c9a7689da2d753a6bb0cb77f244ca35daa92cc702941e4f239c264d',
        ROOT/'tools/audit_smg_audio_adapter_1_1.py':'088b1fad8e90ed0f8b649bbeeb5fdeb92410c7f18a2f99e0259b3d846055f302',
        ROOT/'docs/SMG_AUDIO_ADAPTER_1_1_RESEARCH.md':'9bedf802da03ac8981a3b08b043ad3422a2bd9bfc2363e20d6803b44da24fdb3'}
    for path,sha in pins.items():require(digest(path)==sha,f'Changed original authority: {path}')
    audit=json.loads((BASE/'audit.json').read_text())
    for path,sha in audit['prior_pins_verified'].items():require(digest(ROOT/path)==sha,f'Changed preceding authority: {path}')
    suite=json.loads((BASE/'frozen/suite.json').read_text())
    for c in suite['cases']:
        path=ROOT/c['path'];require(digest(path)==c['sha256'],'Changed original report')
        require(path.read_bytes()==(ROOT/c['reexecution_path']).read_bytes(),'Original repeat differs')
        report=json.loads(path.read_text())
        inventory=ROOT/report['instruction_inventory']['path']
        require(digest(inventory)==report['instruction_inventory']['sha256'],'Changed original listing inventory')
        for routine in json.loads(inventory.read_text())['routines']:
            require(digest(ROOT/routine['path'])==routine['sha256'],'Changed original instruction listing')
    d=json.loads((BASE/'source-fixture.json').read_text())
    require(digest(ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03')==d['xbe_sha256'],'Changed disc image')
    require(digest(ROOT/'assets/disc/maps/lockout.map')==d['cache_sha256'],'Changed resident metadata')
    require(d['metadata']['constants']['004650c0']=='80008037','Changed source random scale')
    rows=[]
    for s in d['authored_sound']['sounds']:
        cls=d['metadata']['classes'][s['byte_2_class']]['raw_hex']
        for r in s['ranges']:
            rows.append(['M',s['tag']['datum'],r['pitch'],s['definition']['raw_hex'],s['playback_parameters']['raw_hex'],
                cls,r['range']['raw_hex'],','.join(v['record']['raw_hex'] for v in r['permutations'])])
    for name,cases in d['selector_probes'].items():
        for c in cases:
            ident=f'{name}-{c["iteration"]}'
            rows.append(['S',ident,c['before']['sound']['datum'],c['iteration'],*state(c['before']),c['selected_permutation'],*state(c['after'])])
            for store in c['rng_stores']:rows.append(['R',ident,store['instruction'],store['before'],store['after']])
    for c in d['creation_kernel_probes']:
        ident=c['scenario'];raw=bytes.fromhex(c['created_source_bc_hex'])
        # Copy the literal original stores. No native expected gain, pitch,
        # permutation or RNG value is computed by this exporter.
        datum=struct.unpack_from('<I',raw,0xc)[0]
        rows.append(['C',ident,f'{datum:08x}',c['before']['audio']['local_seed'],raw[0x94:0x98][::-1].hex(),
            raw[0x90:0x94][::-1].hex(),raw[0x98:0x9c][::-1].hex(),raw[0x9d],c['after']['audio']['local_seed']])
        for store in c['rng_stores']:rows.append(['R',ident,store['instruction'],store['before'],store['after']])
    fixture=ROOT/'analysis/multiplayer/integration/sound-variation-fixtures.tsv'
    fixture.write_text('\n'.join(' '.join(map(str,r)) for r in rows)+'\n')
    run=subprocess.run([str(a.binary.resolve()),str(ROOT),str(fixture)],text=True,capture_output=True)
    print(run.stdout,end='');print(run.stderr,end='')
    result=dict(passed=run.returncode==0,scope=__doc__,original_selector_calls=32,source_creation_scalar_cases=3,lcg_stores=83,
        live_audio_integration=False,accepted_scheduler_or_playback_proven=False,
        limits='Supplied seed, raw resident range state and source FPCW027F; scheduler admission, global audio RNG initialization/order, voice/device/decoder and audible parity remain open.',
        original_fixture_sha256=digest(BASE/'source-fixture.json'),native_fixture_sha256=digest(fixture),
        native_binary_sha256=digest(a.binary),tool_sha256=digest(Path(__file__)),stdout=run.stdout,stderr=run.stderr)
    a.output.write_text(json.dumps(result,indent=2)+'\n')
    raise SystemExit(run.returncode)
if __name__=='__main__':main()
