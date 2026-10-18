#!/usr/bin/env python3
"""Run the current native worker on an actual queued original SMG resource job.

Source bank inputs are reconstructed before the producer semaphore release.
Native mutexes/semaphore/thread and the real file backend own actual outcomes.
This is a component composition; it does not replace the live world's existing
resource policy or certify Xbox thread/priority/device scheduling.
"""
import argparse
import hashlib
import json
import mmap
import os
from pathlib import Path
import shutil
import struct
import subprocess
from original_xbox_pose_case import BASE,ROOT,cpu,physical_ram,PageTableMemory,reconstructed_replay_ram,sha


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    for key in ('case','producer','consumer','request','output'):parser.add_argument('--'+key,type=Path,required=True)
    args=parser.parse_args();scratch=Path(os.environ['HALO2_TEST_SCRATCH']);output=args.output.absolute();output.mkdir(exist_ok=False)
    (output/'attempt.json').write_text(json.dumps(dict(runner_scratch=str(scratch),tool_sha256=sha(Path(__file__))),indent=2)+'\n')
    authority=ROOT/'analysis/multiplayer/integration/spawn-source/animation-async-worker/u5'
    manifest=json.loads((authority/'report.json').read_text());instruction_map={};authorities={str(authority/'report.json'):sha(authority/'report.json')}
    for path,pin in manifest['source_pins'].items():assert sha(path)==pin
    for row in manifest['owners']:
        paths=[authority/listing['file']for listing in row['listings']]
        for path,listing in zip(paths,row['listings']):assert sha(path)==listing['sha256'];authorities[str(path)]=sha(path)
        old,new=[path.read_text().splitlines()for path in paths];assert len(old)==len(new)
        for left,right in zip(old,new):instruction_map[str(int(left.split()[0],16))]=int(right.split()[0],16)
    request=json.loads(args.request.read_text());assert request['original_case_sha256']==sha(args.case/'call.json')
    matches=request['owned_map_content_matches'];assert len(matches)==1;owned=Path(matches[0]['path']);assert sha(owned)==matches[0]['whole_file_sha256']
    bindings=dict(nodes=0x4ecc38,free_head=0x4ef1b8,work_head=0x4ef1bc,free_mutex_slot=0x4ecc28,
        work_mutex_slot=0x4ecc2c,semaphore_slot=0x4ecc30,counter=0x4ecc34,requests_added=0x4ef1c0,
        current_node=0x51db10,read_callback=0x1aaa80)
    bank_start=0x4ecc24;bank_bytes=0x4ef1c4-bank_start
    with reconstructed_replay_ram(args.case,args.consumer)as(bank,memory,consumer):
        current=struct.unpack('<I',memory.read(bindings['current_node'],4))[0]
        callback,done=struct.unpack('<2I',memory.read(current+52,8))
        payload=current+8;task=memory.read(payload,40)
        assert callback==bindings['read_callback']and bindings['nodes']<=current<bindings['nodes']+150*64 and (current-bindings['nodes'])%64==0
        assert struct.unpack('<10I',task)==tuple(request['observed_matching_task_candidates'][0]['observed_words'])
        assert memory.translate(payload)[0]in [r['physical_address']for r in request['observed_matching_task_candidates']]
        chunk_limit=struct.unpack('<I',memory.read(0x47b148,4))[0];assert chunk_limit==0x40000
    with reconstructed_replay_ram(args.case,args.producer)as(bank,memory,producer):
        assert producer['exit']==0x80013c0b
        handles={key:struct.unpack('<I',memory.read(bindings[key+'_mutex_slot'if key!='semaphore'else 'semaphore_slot'],4))[0]for key in ('free','work','semaphore')}
        stack=struct.unpack('<4I',memory.read(producer['actual_CPU']['gp'][4],16))
        assert stack[1:3]==(handles['semaphore'],1)
        head=struct.unpack('<I',memory.read(bindings['work_head'],4))[0]
        assert head==current and struct.unpack('<I',memory.read(current+60,4))[0]==0
        assert memory.read(payload,40)==task
        job=struct.unpack('<I',memory.read(current+4,4))[0];assert job!=0xffffffff
        initial_bank=memory.read(bank_start,bank_bytes)
        (scratch/'original-worker-ram.bin').write_bytes(bank[:])
        entry=dict(control=cpu(args.case,'before')['control_registers'],bindings=bindings,original_handles=handles,
            task_payload=payload,buffer=request['buffer'],bytes=request['bytes'],actual_chunk_limit=chunk_limit,
            file_token=request['observed_file_handle'],owned_path=str(owned),instruction_map=instruction_map,observed_release_count=stack[2])
    (scratch/'input.json').write_text(json.dumps(entry)+'\n')
    cpp=ROOT/'tools/probe_original_weapon_worker.cpp'
    sources=[cpp,*[ROOT/'src'/name for name in ('animation_async_worker.cpp','source_async_jobs.cpp','source_bitmap_async_read.cpp','native_synchronization.cpp','native_threads.cpp','native_execution_gate.cpp')]]
    headers=[ROOT/'include/halo2'/name for name in ('animation_async_worker.hpp','animation_async_constructor.hpp','source_async_jobs.hpp','source_bitmap_async_read.hpp','native_synchronization.hpp','native_threads.hpp','native_execution_gate.hpp','unit_animation_resolution.hpp')]
    source_hashes={str(path):sha(path)for path in sources+headers}
    command=['c++','-std=c++20','-O2','-pthread','-I'+str(ROOT/'include'),*[str(path)for path in sources],'-o',str(scratch/'native-worker')]
    build=subprocess.run(command,capture_output=True,text=True);(output/'compile.log').write_text((build.stdout+build.stderr)[-65536:]);assert build.returncode==0,build.stderr[-3000:]
    run_command=[str(scratch/'native-worker'),str(scratch/'input.json'),str(scratch/'original-worker-ram.bin'),str(scratch/'native.json'),str(scratch/'native-ram.bin')]
    run=subprocess.run(run_command,capture_output=True,text=True,timeout=30);(output/'run.log').write_text((run.stdout+run.stderr)[-65536:]);assert run.returncode==0,run.stderr
    native=json.loads((scratch/'native.json').read_text());comparisons=[]
    with (scratch/'native-ram.bin').open('rb')as file,mmap.mmap(file.fileno(),0,access=mmap.ACCESS_READ)as bank,physical_ram(args.case,'after')as original:
        actual=PageTableMemory(bank,cpu(args.case,'before'));expected=PageTableMemory(original,cpu(args.case,'after'))
        for name,address,size in [('complete_queue_bank',bank_start,bank_bytes),('current_node',bindings['current_node'],4),
                ('complete_task',payload,40),('read_buffer',request['buffer'],request['bytes'])]+([('caller_done',done,1)]if done else []):
            left,right=actual.read(address,size),expected.read(address,size)
            comparisons.append(dict(name=name,address=address,bytes=size,equal=left==right,
                different_bytes=sum(a!=b for a,b in zip(left,right)),native_sha256=hashlib.sha256(left).hexdigest(),original_sha256=hashlib.sha256(right).hexdigest()))
    for path,digest in source_hashes.items():assert sha(path)==digest
    assert sha(owned)==matches[0]['whole_file_sha256']
    result=dict(passed=all(r['equal']for r in comparisons),scope=__doc__,original_node_identity_verified=True,
        original_node=current,original_job=job,original_task_payload=payload,actual_chunk_limit=chunk_limit,
        original_handles=handles,source_bindings=bindings,native=native,comparisons=comparisons,
        observed_producer_notification_stack=list(stack),source_authorities_sha256=authorities,
        source_files_sha256=source_hashes,compile_command=command,run_command=run_command,native_binary_sha256=sha(scratch/'native-worker'),
        source_case_sha256=sha(args.case/'call.json'),producer_sha256=sha(args.producer/'report.json'),consumer_sha256=sha(args.consumer/'report.json'),
        request_report_sha256=sha(args.request),original_input_queue_sha256=hashlib.sha256(initial_bank).hexdigest(),
        native_sync_initialization='Explicit native unlocked mutexes and zero-count semaphore; actual native producer release and worker acquisition',
        native_world_integration_verified=False,original_constructor_kernel_priority_TLS_or_device_verified=False,
        original_expected_bytes_or_kernel_returns_supplied=False,new_large_reference_inputs=False,
        runner_scratch=str(scratch),persistent_binaries_or_RAM_retained=False,physical_xbox_verified=False,full_weapon_parity=False,
        tool_sha256=sha(Path(__file__)),helper_sha256=sha(ROOT/'tools/original_xbox_pose_case.py'))
    (output/'report.json').write_text(json.dumps(result,indent=2)+'\n')
    for path in [Path(__file__),ROOT/'tools/original_xbox_pose_case.py',*sources,*headers]:shutil.copyfile(path,output/path.name)
    print(json.dumps({k:result[k]for k in ('passed','original_node','original_job','actual_chunk_limit','comparisons','runner_scratch')}),flush=True)
    assert result['passed']


if __name__=='__main__':main()
