#!/usr/bin/env python3
"""Independently verify the measured moving death, raw buffers and repeats."""
import argparse
import hashlib
import json
import struct
from pathlib import Path
from oracle_smg_firing_1_1 import ROOT,Xbe


def sha(path):return hashlib.sha256(Path(path).read_bytes()).hexdigest()
def encoded(value):return (json.dumps(value,indent=2,sort_keys=True)+'\n').encode()
def unit(snapshot):
    return next(row for row in snapshot['world']['objects'] if row['datum']=='e26f0000')
def fields(snapshot):
    row=unit(snapshot);raw=bytes.fromhex(row['raw_hex'])
    return dict(datum=row['datum'],pointer=row['pointer'],
        root_hex=raw[0x64:0x70].hex(),velocity_hex=raw[0x88:0x94].hex(),
        angular_velocity_hex=raw[0x94:0xa0].hex(),physical_state=raw[0x3dc],
        mode34b=raw[0x34b],contact_gate3fd3fe_hex=raw[0x3fd:0x3ff].hex(),
        component=row['component'],component_flags_hex=row['component_hex'][8:16],
        body_count=len(row['bodies']))
def segments(snapshot):
    result={}
    def add(name,p,raw):
        if raw is not None:result[name]=(int(p,16),bytes.fromhex(raw))
    for row in snapshot['raw_physical_backing']:add('raw-'+row['pointer'],row['pointer'],row['raw_hex'])
    for row in snapshot['world']['objects']:add('object-'+row['datum'],row['pointer'],row['raw_hex'])
    world=snapshot['world']
    for name,pkey,hkey in (('world','world','world260_hex'),
            ('object-context','object_world','object_world_hex'),
            ('player-pool','player_pool','player_pool_header_hex')):
        add(name,world[pkey],world[hkey])
    add('clock',snapshot['clock_pointer'],snapshot['clock0c_hex'])
    player=snapshot['player']
    for row in player['records']:add('player-'+row['datum'],row['pointer'],row['raw_hex'])
    for key,hkey in (('globals','globals130_hex'),('engine','engine_c9c_hex'),
            ('unit_queue','unit_queue1d88_hex'),('trigger_bits','trigger24_hex')):
        add('player-'+key,player[key],player[hkey])
    return result


def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--directory',type=Path,required=True)
    ap.add_argument('--repeat',type=Path,required=True)
    ap.add_argument('--output',type=Path,required=True)
    args=ap.parse_args()
    reports=[]
    for parent in (args.directory,args.repeat):
        reports.append(json.loads((parent/'report.json').read_text()))
    report=reports[0]
    files=sorted(p.relative_to(args.directory) for p in args.directory.rglob('*') if p.is_file())
    assert files==sorted(p.relative_to(args.repeat) for p in args.repeat.rglob('*') if p.is_file())
    # The real host context/stderr may contain process-specific platform values;
    # the gameplay report and every original instruction file must be identical.
    compared=[p for p in files if p.name not in ('raw-host-context.json','runtime-stderr.txt')]
    for p in compared:assert (args.directory/p).read_bytes()==(args.repeat/p).read_bytes(),p
    assert sha(ROOT/'tools/oracle_mp_spartan_death_physical_1_1.py')==report['oracle_sha256']
    for p,h in report['prior_pins'].items():assert sha(ROOT/p)==h,p
    image=Xbe(ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03')
    instruction_rows=0
    executed=set()
    for listing in report['instructions']:
        path=args.directory/listing['file'];assert sha(path)==listing['sha256']
        for line in path.read_text().splitlines():
            at,raw,*_=line.split(' ',2);at=int(at,16);raw=bytes.fromhex(raw)
            assert image.read(at,len(raw))==raw,(path,at)
            instruction_rows+=1
            if listing.get('kind')=='complete_new_execution':executed.add(at)
    for row in report['stores']+report['reads']:assert int(row['instruction'],16) in executed
    assert not report['instruction_patches'] and not report['new_supplied_gameplay_or_physics_results']
    assert report['source_movement']['prior_case_states_byte_identical']
    assert all(s['completed'] and not s['invalid'] for s in report['stages'])
    initial=fields(report['initial'])
    assert any(struct.unpack('<3f',bytes.fromhex(initial['velocity_hex'])))
    cleanup=next(s for s in report['stages'] if s['name']=='explicit_original_ordinary_death_cleanup')
    assert fields(cleanup['before'])['physical_state']==1
    assert fields(cleanup['after'])['physical_state']==3
    assert fields(cleanup['before'])['velocity_hex']==initial['velocity_hex']==fields(cleanup['after'])['velocity_hex']
    assert all(fields(s['after'])['mode34b']==0 and fields(s['after'])['body_count']==1 for s in report['stages'])
    assert not any(c['entry']=='001d9330' for c in report['calls'])
    assert sum(c['entry']=='001c70d0' for c in report['calls'])==2
    assert all(c.get('return_esp_delta')==12 for c in report['calls'] if c['entry']=='001c70d0')
    call_phase={c['id']:c['phase'] for c in report['calls']}
    intervals=reads=stores=changed_bytes=0
    phase_summaries=[]
    for phase in report['stages']:
        name=phase['name'];before=segments(phase['before']);after=segments(phase['after'])
        events=[(s['order'],'store',s) for s in report['stores'] if s['phase']==name]
        events += [(r['order'],'read',r) for r in report['reads'] if call_phase[r['call']]==name]
        events.sort()
        for key,(base,raw) in before.items():
            if key not in after or after[key][0]!=base or len(after[key][1])!=len(raw):continue
            retained=bytearray(raw);end=base+len(raw)
            for order,kind,event in events:
                address=int(event['address'],16);size=event['size']
                if not(base<=address and address+size<=end):continue
                offset=address-base
                expected=bytes.fromhex(event['before_hex'] if kind=='store' else event['raw_hex'])
                assert retained[offset:offset+size]==expected,(name,key,order,event['instruction'])
                if kind=='store':retained[offset:offset+size]=bytes.fromhex(event['written_hex']);stores+=1
                else:reads+=1
            assert retained==after[key][1],(name,key,'complete output buffer')
            changed_bytes+=sum(a!=b for a,b in zip(raw,retained));intervals+=1
        phase_summaries.append(dict(name=name,before=fields(phase['before']),after=fields(phase['after']),
            supplied_services=[{k:v for k,v in event.items() if k not in ('state','input_state')}
                for event in phase['used_services'] if event.get('supplied')]))
    result=dict(schema=1,tool_sha256=sha(Path(__file__)),report_sha256=sha(args.directory/'report.json'),
        byte_identical_files=len(compared),instruction_rows=instruction_rows,
        executed_addresses=len(executed),replayed_intervals=intervals,overlapping_reads=reads,
        overlapping_stores=stores,changed_bytes=changed_bytes,initial=initial,phases=phase_summaries,
        ordinary_death_velocity_preserved=True,ten_body_ragdoll_admitted=False,
        full_live_device_fatal_admission_or_successive_clock_owned_frames_proven=False)
    args.output.parent.mkdir(parents=True,exist_ok=True)
    raw=encoded(result)
    if args.output.exists():assert args.output.read_bytes()==raw
    else:args.output.write_bytes(raw)
    print(json.dumps({k:v for k,v in result.items() if k not in ('initial','phases')}))


if __name__=='__main__':main()
