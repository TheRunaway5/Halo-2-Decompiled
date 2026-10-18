#!/usr/bin/env python3
"""Execute original live weapon/first-person bytes on the host CPU, without game returns.

Actual captured virtual pages and CPU inputs form the fixture. Source bytes run
unchanged under the existing strict i386 single-step observer. Linux segment and
stop routing are explicit harness policy. This is not Xbox hardware evidence.
All binaries, banks, input fixtures and compiler products use runner scratch.
"""
import argparse
import gzip
import hashlib
import json
import os
from pathlib import Path
import shutil
from original_xbox_pose_case import ROOT,cpu,physical_ram,PageTableMemory,sha
from oracle_sandbox_first_person_hardware_tu15 import execute

def flattened(rows):
    result=[]
    for row in rows:
        old,new=bytes.fromhex(row['before']),bytes.fromhex(row['written'])
        assert len(old)==len(new)
        result.extend((row['instruction'],row['address']+i,a,b)for i,(a,b)in enumerate(zip(old,new)))
    return result

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    for name in ('case','oracle','native','output'):parser.add_argument('--'+name,type=Path,required=True)
    args=parser.parse_args();scratch=Path(os.environ['HALO2_TEST_SCRATCH']);out=args.output.absolute();out.mkdir(exist_ok=False)
    before=cpu(args.case,'before');call=json.loads((args.case/'call.json').read_text())
    oracle=json.loads((args.oracle/'report.json').read_text());assert oracle['passed']
    trace=json.loads((args.oracle/'instructions.json').read_text())
    pages={int(page)for page in json.loads((args.oracle/'mapped-pages.json').read_text())}
    state=call.get('first_person_state')
    if state:pages.update(range(state['address']&~4095,(state['address']+state['bytes']+4095)&~4095,4096))
    abi=dict(gp=before['gp'],eflags=before['eflags'],mxcsr=before['mxcsr'],fs_base=before['fs_base'],
        x87_control_word=before['x87_control'],x87_status=before['x87_status'],x87_tag=before['x87_tag'],
        x87_values=before['x87_register_bytes'],
        xmm=[b''.join(w.to_bytes(4,'little')for w in words)[::-1].hex()for words in before['xmm']])
    with physical_ram(args.case,'before')as ram:
        memory=PageTableMemory(ram,before);aliases={}
        for page in pages:aliases.setdefault(memory.translate(page)[0],[]).append(page)
        writes=json.loads((args.oracle/'writes.json').read_text())
        written_physical={memory.translate(w['address'])[0]&~4095 for w in writes}
        assert not any(len(v)>1 and p in written_physical for p,v in aliases.items()),'Mutable original aliases require shared host mappings'
        merged=[]
        for page in sorted(pages):
            if merged and page==merged[-1][1]:merged[-1][1]+=4096
            else:merged.append([page,page+4096])
        banks=[dict(address=left,hex=memory.read(left,right-left).hex())for left,right in merged]
    fixture=dict(image='1.5',cases=[dict(id=1,entry=call['entry'],entry_abi=abi,
        return_address=call['return_address'],exit_entry=call['return_address'],exit={},
        original_instructions=trace,initial_banks=banks)])
    pair=dict(address_map={},entries={f'{call["entry"]:08x}':call['entry']},
        instruction_map={f'{pc:08x}':pc for pc in sorted(set(trace))})
    actual=execute(fixture,pair,scratch/'host')
    case=actual['cases'][0];host=case['exit'];native=json.loads((args.native/'native-observations.json').read_text())
    host_xmm=[[int.from_bytes(bytes.fromhex(value)[::-1][i*4:i*4+4],'little')for i in range(4)]for value in host['xmm']]
    errors=[]
    for key,value in (('gp',host['gp']),('xmm',host_xmm),('x87_control',host['x87_control_word']),
        ('x87_status',host['x87_status']),('mxcsr',host['mxcsr'])):
        if native['CPU'][key]!=value:errors.append(dict(kind=key,native=native['CPU'][key],original_host=value))
    if host['eflags']&0xcd5!=native['CPU']['eflags']&0xcd5:errors.append(dict(kind='defined_flags'))
    if host['x87_tag']!=65535:errors.append(dict(kind='host_x87_tags_not_empty'))
    if case['instructions']!=native['instructions']:errors.append(dict(kind='instruction_trace'))
    host_stores=flattened(case['writes']);native_stores=flattened(native['memory_writes'])
    if host_stores!=native_stores:
        first=next((i for i,(a,b)in enumerate(zip(native_stores,host_stores))if a!=b),min(len(native_stores),len(host_stores)))
        errors.append(dict(kind='ordered_source_store_bytes',first=first,
            native=native_stores[max(0,first-2):first+3],original_host=host_stores[max(0,first-2):first+3]))
    host_estimates=[]
    for row in case['estimates']:
        inputs=row['input']if isinstance(row['input'],list)else[row['input']]
        outputs=row['output']if isinstance(row['output'],list)else[row['output']]
        host_estimates.extend(dict(instruction=row['instruction'],input=a,output=b)for a,b in zip(inputs,outputs))
    if native['actual_host_CPU_estimates']!=host_estimates:errors.append(dict(kind='actual_host_estimates'))
    def read_final(address,size):
        result=bytearray()
        while size:
            bank=next(b for b in case['final_banks']if b['address']<=address<b['address']+len(b['hex'])//2)
            raw=bytes.fromhex(bank['hex']);at=address-bank['address'];n=min(size,len(raw)-at)
            result.extend(raw[at:at+n]);size-=n;address+=n
        return bytes(result)
    state_sha=None
    if state:
        raw=read_final(state['address'],state['bytes']);state_sha=hashlib.sha256(raw).hexdigest()
        (out/'original-host-first-person-state.bin').write_bytes(raw)
    # Keep compact measured output and provenance, not duplicate RAM/image banks.
    observation=dict(exit=host,instructions=case['instructions'],writes=case['writes'],estimates=case['estimates'])
    (out/'original-host-observations.json.gz').write_bytes(gzip.compress(json.dumps(observation,separators=(',',':')).encode(),mtime=0))
    sources=(Path(__file__),ROOT/'tools/oracle_sandbox_first_person_hardware_tu15.py',
        ROOT/'tools/sandbox_first_person_hardware_trace_tu15.cpp',ROOT/'tools/original_i386_game_frame_bridge_20261011.py',
        ROOT/'tools/original_xbox_pose_case.py')
    for source in sources:shutil.copyfile(source,out/source.name)
    report=dict(passed=not errors,scope=__doc__,errors=errors,entry=call['entry'],
        original_host_instructions=len(case['instructions']),native_instructions=len(native['instructions']),
        original_host_ordered_stores=len(case['writes']),native_ordered_stores=len(native['memory_writes']),
        original_host_written_bytes=len(host_stores),native_written_bytes=len(native_stores),
        actual_host_estimate_lanes=len(host_estimates),
        original_host_x87_control=host['x87_control_word'],native_x87_control=native['CPU']['x87_control'],
        original_host_x87_status=host['x87_status'],native_x87_status=native['CPU']['x87_status'],
        original_host_first_person_state_sha256=state_sha,
        source_case_sha256=sha(args.case/'call.json'),source_oracle_sha256=sha(args.oracle/'report.json'),
        native_observations_sha256=sha(args.native/'native-observations.json'),
        original_instruction_patches=[],supplied_gameplay_returns=[],native_expected_data_used=False,
        initial_virtual_pages=len(pages),mutable_aliases=False,
        host_binary_sha256=actual['original_hardware_binary_sha256'],
        sources_sha256={p.name:sha(p)for p in sources},
        retained_files_sha256={p.name:sha(p)for p in out.iterdir()if p.is_file()},
        runner_scratch=str(scratch),large_artifact_retention=[],physical_xbox_verified=False,full_weapon_parity=False)
    (out/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({k:v for k,v in report.items()if k not in('scope','sources_sha256','retained_files_sha256')}),flush=True)
    assert report['passed'],str(out/'report.json')

if __name__=='__main__':main()
