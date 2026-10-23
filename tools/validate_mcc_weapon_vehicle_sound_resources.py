#!/usr/bin/env python3
"""Check literal prepared-MCC Classic weapon/vehicle resources and PC decoding.

The pinned Assembly schema and actual DAT bytes supply expectations separately
from the C++ loader. This verifies resource usability, not Xbox equivalence,
event admission, audibility or waveform/timing parity. Use tools/run_test.py.
"""
import argparse
from functools import lru_cache
import hashlib
import json
import os
from pathlib import Path
import subprocess
import tempfile
import xml.etree.ElementTree as ET

from audit_weapon_vehicle_sound_presence_tu15 import GROUPS, PIN, references
from inspect_multiplayer import ROOT
from mcc_content import MccCache
from original_xbox_pose_case import BASE, sha

SCHEMAS = {}
PLUGIN_FILES = set(subprocess.check_output(['git', '-C', str(ROOT/'tools/reference/assembly'),
    'ls-tree', '-r', '--name-only', PIN, 'src/Assembly/Plugins'], text=True).splitlines())


@lru_cache(None)
def schema(group):
    path = f'src/Assembly/Plugins/Halo2MCC/{group}.xml'
    if path not in PLUGIN_FILES:
        path = f'src/Assembly/Plugins/Halo2/{group}.xml'
    data = subprocess.check_output(['git', '-C', str(ROOT/'tools/reference/assembly'), 'show', f'{PIN}:{path}'])
    SCHEMAS[path] = hashlib.sha256(data).hexdigest()
    return ET.fromstring(data)


class Bank(MccCache):
    def u(self, at): return self.u32(at)


def merge(local, shared):
    result = {datum: (local, tag) for datum, tag in local.tags.items()}
    slots = {datum & 65535: datum for datum in result}
    for datum, tag in shared.tags.items():
        if tag['offset'] is None: continue
        prior_id = slots.get(datum & 65535)
        if prior_id is not None:
            before = result[prior_id][1]
            if (before['datum'], before['group'], before['name']) != (datum, tag['group'], tag['name']):
                raise ValueError('Shared full-datum/group/path collision')
        if prior_id is None or result[datum][1]['offset'] is None:
            result[datum] = shared, tag
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--maps', type=Path, required=True)
    parser.add_argument('--binary', type=Path)
    parser.add_argument('--metadata-only', action='store_true', help='Check literal metadata/ranges without native loading or decoding')
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args(); args.maps = args.maps.resolve()
    if not args.metadata_only and not args.binary: parser.error('--binary is required for native resource validation')
    scratch = Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    args.output.mkdir(parents=True, exist_ok=True)
    scope = json.loads((BASE/'authored-scope.json').read_text())
    report = dict(passed=False, scope=__doc__, schema_commit=PIN, schema_sha256=SCHEMAS,
        maps=[], unpaired=[], source_files={}, tool_sha256=sha(Path(__file__)),
        binary_sha256=sha(args.binary) if args.binary else None, runner_scratch=str(scratch), large_artifacts_retained=[],
        original_xbox_payload_equality_verified=False, runtime_event_selection_verified=False,
        mode_policy='Classic: exclude snd! bit11 Play Only In Remastered Mode')
    with tempfile.TemporaryDirectory(prefix='mcc-weapon-resources-', dir=scratch) as owned:
        work = Path(owned); fixture = work/'fixtures.tsv'; shared = Bank(args.maps/'shared.map')
        try:
            paths = set(); owners = set(); cases = 0; remastered = 0; excluded = set()
            report['source_files'][str(shared.path)] = sha(shared.path)
            with fixture.open('w') as out:
                for item in scope['maps']:
                    path = args.maps/Path(item['path']).name
                    if not path.is_file():
                        report['unpaired'].append(Path(item['path']).name); continue
                    report['source_files'][str(path)] = sha(path)
                    local = Bank(path)
                    try:
                        # initialize_mcc admits shared.map only for a level
                        # with actual nonresident directory entries. An unused
                        # shared weapon is not automatically part of a map.
                        needs_shared = local.u32(local.index+12)!=0xffffffff and any(
                            tag['offset'] is None for tag in local.tags.values())
                        tags = merge(local, shared) if needs_shared else {
                            datum:(local,tag) for datum,tag in local.tags.items()}
                        visited = set(); root_count = 0
                        def visit(datum):
                            if datum in visited: return
                            visited.add(datum); bank, tag = tags[datum]
                            if tag['group'] not in GROUPS: return
                            if tag['offset'] is None: raise ValueError('Nonresident MCC dependency')
                            for child in references(bank, schema(tag['group']), tag['offset']): visit(child)
                        for datum, (_, tag) in tags.items():
                            if tag['group'] in ('weap', 'vehi'):
                                root_count += 1; owners.add((tag['group'],tag['name'])); visit(datum)
                        # The directory's actual globals datum selects the
                        # owning matg; a merged bank can contain other globals.
                        globals_id = local.u32(local.index+16)
                        gb, gt = tags[globals_id]
                        if gt['group']!='matg' or gt['offset'] is None:
                            raise ValueError('Directory globals datum has no resident matg owner')
                        sg = gb.block(gt['offset']+0xc0,0x2c)
                        if len(sg) != 1: raise ValueError('Invalid MCC Sound Globals count')
                        count = 0; sound_count = 0
                        for datum in sorted(visited):
                            bank, tag = tags[datum]
                            if tag['group'] != 'snd!': continue
                            s = tag['offset']
                            if s is None: raise ValueError('Missing resident MCC sound')
                            if bank.u16(s)&0x800:
                                remastered += 1; excluded.add(tag['name']); continue
                            sound_count += 1; paths.add(tag['name'])
                            gestalts = [t for t in bank.tags.values() if t['group']=='ugh!' and t['offset'] is not None]
                            if len(gestalts) != 1: raise ValueError('Ambiguous MCC directory-owner sound gestalt')
                            g = gestalts[0]['offset']; codecs = bank.block(g,3)
                            codec_row = codecs[bank.data[s+3]]; rate, encoding, codec = bank.data[codec_row:codec_row+3]
                            channels = (1,2,2,4)[encoding]; sample_rate = (22050,44100,32000,48000)[rate]
                            if encoding == 2: codec = 4
                            cb, ct = tags[gb.u32(sg[0]+12)]
                            cls = cb.block(ct['offset'],0x78)[bank.data[s+2]]
                            data_name = 'sounds_en.dat' if cb.u16(cls+8)&0x80 else 'sounds_neutral.dat'
                            resource = args.maps/data_name
                            if str(resource) not in report['source_files']: report['source_files'][str(resource)] = sha(resource)
                            ranges, permutations, languages = bank.block(g+0x28,12), bank.block(g+0x30,16), bank.block(g+0x60,8)
                            chunks = bank.block(languages[0],16); first_range = bank.u16(s+8)
                            for pitch in range(bank.data[s+10]):
                                r = ranges[first_range+pitch]; first, size = bank.u16(r+8), bank.u16(r+10)
                                for permutation in range(size):
                                    p = permutations[first+permutation]; info = bank.block(p+8,8)[0]
                                    pcm_bytes, first_chunk, chunk_count = bank.u32(info), bank.u16(info+4), bank.u16(info+6)
                                    assert pcm_bytes > 0 and chunk_count > 0
                                    samples = 0 if codec==4 else pcm_bytes//(channels*2)
                                    out.write('S '+json.dumps(str(path))+' '+' '.join(map(str,(
                                        f'{datum:08x}',pitch,permutation,codec,channels,sample_rate,bank.data[s+2],samples,chunk_count)))+'\n')
                                    for at in chunks[first_chunk:first_chunk+chunk_count]:
                                        offset, flags = bank.u32(at), bank.u32(at+4); length = flags&0x1fffffff
                                        assert length > 0 and offset+length <= resource.stat().st_size
                                        out.write('C '+json.dumps(str(resource))+f' {offset} {length} {offset:08x} {flags:08x}\n')
                                    count += 1
                        cases += count
                        report['maps'].append(dict(name=path.name,shared_namespace_admitted=needs_shared,
                            roots=root_count,sound_definitions=sound_count,encoded_cases=count))
                        print(json.dumps(report['maps'][-1]),flush=True)
                    finally: local.close()
            report.update(sound_paths=len(paths),weapon_paths=sum(g=='weap' for g,_ in owners),
                vehicle_paths=sum(g=='vehi' for g,_ in owners),encoded_cases=cases,
                excluded_remastered_sound_definitions=remastered,excluded_remastered_paths=sorted(excluded),
                fixture_sha256=sha(fixture),fixture_bytes=fixture.stat().st_size)
            unchanged = all(sha(Path(p))==h for p,h in report['source_files'].items())
            if args.metadata_only:
                report.update(status='metadata_checked',validation_not_run=True,inputs_unchanged=unchanged)
            else:
                command = [str(args.binary.resolve()),str(fixture),str(ROOT),str(args.maps)]; report['command']=command
                result = subprocess.run(command,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True,timeout=900)
                unchanged = unchanged and sha(args.binary)==report['binary_sha256'] and all(sha(Path(p))==h for p,h in report['source_files'].items())
                report.update(exit_code=result.returncode,native_output=result.stdout[-16384:],inputs_unchanged=unchanged,
                              passed=result.returncode==0 and unchanged and len(report['maps'])==37)
        except Exception as error: report['error']=str(error)
        finally: shared.close()
    report['fixture_scratch_removed']=not work.exists()
    (args.output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({k:report.get(k) for k in ('passed','error','native_output','fixture_scratch_removed')}),flush=True)
    return 0 if report['passed'] or (args.metadata_only and report.get('status')=='metadata_checked' and
                                   report.get('inputs_unchanged') and len(report['maps'])==37) else 1


if __name__=='__main__': raise SystemExit(main())
