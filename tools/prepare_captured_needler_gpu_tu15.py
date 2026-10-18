#!/usr/bin/env python3
"""Select real captured Needler GPU inputs and authenticate their material owners.

Read the completed source bank directly from its verified compressed chunks.
No expanded memory-bank file or invented material selection is required.
"""
import argparse
import hashlib
import importlib.util
import io
import json
from pathlib import Path
import struct

from audit_rendering import Cache, file_sha


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--flow',type=Path,required=True)
    parser.add_argument('--draw-evidence',type=Path,required=True)
    parser.add_argument('--queue-evidence',type=Path,required=True)
    parser.add_argument('--original',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args()
    flow=args.flow.absolute();base=flow.parent
    report=json.loads((flow/'report.json').read_text())
    receipt=json.loads((flow/'run.json').read_text())
    evidence=json.loads(args.draw_evidence.read_text())
    queue=json.loads(args.queue_evidence.read_text())
    original=json.loads((args.original/'report.json').read_text())
    native=json.loads((flow/'captured-entry/native.json').read_text())
    assert evidence['passed'] and queue['passed'] and report['passed']
    assert evidence['source_report_sha256']==file_sha(flow/'report.json')==receipt['actual_report_sha256']
    assert evidence['source_draw_sha256']==file_sha(flow/'particle-draws.bin')
    assert evidence['original_queue_api_evidence_sha256']==file_sha(args.queue_evidence)
    assert queue['original_report_sha256']==file_sha(args.original/'report.json')
    assert queue['actual_API_build_sha256']==file_sha(flow/'captured-entry/build.json')
    assert native['completed'] and native['entry']==0x1770c0 and not native['first_person']
    selected=[s for s in report['particle_draw_samples'] if not s['first_person']
              and s['tick']==native['sequence_tick'] and s['view']==native['sequence_view']]
    assert len(selected)==1 and selected[0]['draws']
    selected=selected[0]
    cameras=[v for v in report['native_view_samples'] if v['tick']==selected['tick'] and v['view']==selected['view']]
    assert len(cameras)==1

    # Reconstruct in memory, verifying the entire bank against the same-input
    # unchanged-original result. Keep every full bank off the filesystem.
    spec=importlib.util.spec_from_file_location('retained_banks',base/'bank-chunk-retention-v1.py')
    banks=importlib.util.module_from_spec(spec);spec.loader.exec_module(banks)
    index=json.loads(banks.INDEX.read_text())
    member=str((flow/'captured-entry/after.bin').relative_to(base))
    record=index['files'][member];path=banks.STORE/record['manifest']
    assert file_sha(path)==record['manifest_sha256']
    manifest=json.loads(path.read_text())
    assert manifest['member']==member and manifest['raw_sha256']==record['raw_sha256']
    assert record['raw_sha256']==original['files_sha256']['original-after.bin']
    output=io.BytesIO();banks.reconstruct(manifest,output)
    memory=output.getbuffer();regions=[];offset=0
    while offset<len(memory):
        address,size=struct.unpack_from('<2I',memory,offset);offset+=8
        assert size>0 and offset+size<=len(memory)
        regions.append((address,size,offset));offset+=size
    assert offset==len(memory)

    def read(address,size):
        found=[o+address-a for a,n,o in regions if a<=address and address+size<=a+n]
        assert len(found)==1,(hex(address),size)
        return memory[found[0]:found[0]+size]

    def u32(address):return struct.unpack('<I',read(address,4))[0]
    def u16(address):return struct.unpack('<H',read(address,2))[0]
    def pool_record(global_address,datum,stride):
        pool=u32(global_address);capacity=u32(pool+0x20);high=u32(pool+0x38)
        assert 0<capacity<=65536 and high<=capacity and u32(pool+0x24)==stride
        assert datum!=0xffffffff and datum&65535<high
        slot=datum&65535;bits=u32(pool+0x48)
        assert u32(bits+(slot//32)*4)&(1<<(slot%32))
        body=u32(pool+0x44)+slot*stride
        assert u16(body)==datum>>16 and datum>>16
        return body

    cache=Cache(Path(receipt['command'][1]));assert file_sha(Path(receipt['command'][1]))==receipt['map_sha256']
    draws=[]
    for draw in selected['draws']:
        owner=draw['sort_owner'];system=pool_record(0x51df74,owner['system'],84)
        pool_record(0x52d71c,owner['location'],52)
        seen=set();current=u32(system+0x30)
        while current!=0xffffffff:
            assert current not in seen;seen.add(current)
            location=pool_record(0x52d71c,current,52);current=u32(location+0xc)
        assert owner['location'] in seen and u32(system+0x10)==draw['effect']
        event,number=u32(system+0x14),u16(system+0xe)
        _,root=cache.tag(draw['effect'],'effe')
        events=list(cache.block(root+0x14,0x38));assert event<len(events)
        systems=list(cache.block(events[event]+0x30,0x38));assert number<len(systems)
        particle=cache.u32(systems[number]+4)
        tag,_=cache.tag(particle,'prt3')
        draws.append(dict(draw,particle_datum=particle,particle_name=tag['name'],event=event,system_index=number))
    assert {d['effect'] for d in draws}>={3819242032,3819438643}
    result=dict(passed=True,scope=__doc__,flow=str(flow),map=str(Path(receipt['command'][1]).absolute()),
        map_sha256=receipt['map_sha256'],source_report_sha256=file_sha(flow/'report.json'),
        source_draw_file=str(flow/'particle-draws.bin'),source_draw_sha256=evidence['source_draw_sha256'],
        source_bank_member=member,source_bank_sha256=record['raw_sha256'],
        queue_evidence_sha256=file_sha(args.queue_evidence),draw_evidence_sha256=file_sha(args.draw_evidence),
        tick=selected['tick'],view=selected['view'],first_person=False,camera=cameras[0],draws=draws,
        material_selection='Actual full system/location allocation identities and authored event/system prt3 reference',
        source_state_mutated=False,expanded_bank_written=False,tool_sha256=file_sha(Path(__file__)),
        graphics_execution_verified=False,rendered_sequence_verified=False,physical_xbox_verified=False,full_weapon_parity=False)
    assert not args.output.exists();args.output.parent.mkdir(parents=True,exist_ok=True)
    args.output.write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps(dict(passed=True,tick=result['tick'],view=result['view'],draws=len(draws),
        authored_particle_definitions=len({d['particle_datum'] for d in draws}),expanded_bank_written=False)))


if __name__=='__main__':main()
