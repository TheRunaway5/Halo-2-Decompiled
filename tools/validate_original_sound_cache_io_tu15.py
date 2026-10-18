#!/usr/bin/env python3
"""Validate the production sound-cache I/O boundary with original firing state."""
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
    parser.add_argument('--binary',type=Path)
    parser.add_argument('--output',type=Path)
    args=parser.parse_args();scratch=Path(os.environ['HALO2_TEST_SCRATCH'])
    output=args.output.absolute()if args.output else scratch/'sound-cache-io-result';output.mkdir(exist_ok=False)
    (output/'attempt.json').write_text(json.dumps(dict(runner_scratch=str(scratch),tool_sha256=sha(Path(__file__))),indent=2)+'\n')
    case=BASE/'smg-original-fire-v1';prefix=BASE/'smg-original-sound-read-entry-v1'
    request_path=BASE/'smg-original-fire-v1-io-request-v3/report.json'
    if not all(path.is_file()for path in (case/'call.json',prefix/'report.json',request_path)):return 77
    report=json.loads((prefix/'report.json').read_text());assert report['declared_prefix_stop']==0x1a3b70 and report['declared_prefix_stop_reached']
    assert not report['passed']and not report['supplied_gameplay_returns']and not report['source_instruction_patches']
    request=json.loads(request_path.read_text());match=request['owned_map_content_matches'];assert len(match)==1
    owned=Path(match[0]['path'])
    if not owned.is_file():return 77
    assert sha(owned)==match[0]['whole_file_sha256']
    with reconstructed_replay_ram(case,prefix)as(bank,memory,entry_report):
        initial=cpu(case,'before');initial.update(entry_report['actual_CPU'])
        initial['owned_file']=str(owned)
        sp=initial['gp'][4];stack=struct.unpack('<7I',memory.read(sp,28));assert stack[0]==0x21f4c2
        assert stack[1:]==(request['bytes'],request['buffer'],0,initial['gp'][6],5,4)
        (scratch/'entry-ram.bin').write_bytes(bank[:]);(scratch/'entry-cpu.json').write_text(json.dumps(initial)+'\n')
    sources=[ROOT/'tests/source_sound_cache_io_tu15_tests.cpp',ROOT/'src/source_sound_cache_io_tu15.cpp']
    header=ROOT/'include/halo2/source_sound_cache_io_tu15.hpp';source_hashes={str(path):sha(path)for path in sources+[header]}
    binary=args.binary.absolute()if args.binary else scratch/'sound-cache-io'
    command=None
    if not args.binary:
        command=['c++','-std=c++20','-O2','-I'+str(ROOT/'include'),*[str(path)for path in sources],'-o',str(binary)]
        build=subprocess.run(command,capture_output=True,text=True);(output/'compile.log').write_text((build.stdout+build.stderr)[-65536:]);assert build.returncode==0,build.stderr[-3000:]
    run_command=[str(binary),str(scratch/'entry-cpu.json'),str(scratch/'entry-ram.bin'),str(scratch/'native.json'),str(scratch/'native-ram.bin')]
    run=subprocess.run(run_command,capture_output=True,text=True,timeout=30);(output/'run.log').write_text((run.stdout+run.stderr)[-65536:]);assert run.returncode==0,run.stderr
    native=json.loads((scratch/'native.json').read_text());comparisons=[]
    with (scratch/'native-ram.bin').open('rb')as file,mmap.mmap(file.fileno(),0,access=mmap.ACCESS_READ)as bank,physical_ram(case,'after')as original:
        actual=PageTableMemory(bank,initial);expected=PageTableMemory(original,cpu(case,'after'))
        for name,address,size in [('resource_buffer',stack[2],stack[1]),('sound_cache_done',stack[4],1)]:
            left,right=actual.read(address,size),expected.read(address,size)
            comparisons.append(dict(name=name,address=address,bytes=size,equal=left==right,native_sha256=hashlib.sha256(left).hexdigest(),original_sha256=hashlib.sha256(right).hexdigest()))
    for path,pin in source_hashes.items():assert sha(path)==pin
    result=dict(passed=native['passed']and all(row['equal']for row in comparisons),native=native,comparisons=comparisons,
        source_files_sha256=source_hashes,compile_command=command,run_command=run_command,binary_sha256=sha(binary),
        original_call_sha256=sha(case/'call.json'),original_prefix_sha256=sha(prefix/'report.json'),original_request_sha256=sha(request_path),
        native_policy='Synchronous real bound resource read and retained native receipt before source completion byte; Xbox worker/kernel timeline remains unverified',
        full_native_world_or_audio_output_verified=False,full_weapon_parity=False,runner_scratch=str(scratch),new_large_reference_inputs=False,
        tool_sha256=sha(Path(__file__)),helper_sha256=sha(ROOT/'tools/original_xbox_pose_case.py'))
    (output/'report.json').write_text(json.dumps(result,indent=2)+'\n')
    for path in [Path(__file__),ROOT/'tools/original_xbox_pose_case.py',*sources,header]:shutil.copyfile(path,output/path.name)
    print(json.dumps(dict(passed=result['passed'],negative_controls=len(native['negative_controls']),comparisons=comparisons,runner_scratch=str(scratch))),flush=True)
    assert result['passed'];return 0


if __name__=='__main__':raise SystemExit(main())
