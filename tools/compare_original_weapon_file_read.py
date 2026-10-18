#!/usr/bin/env python3
"""Check the current native read owner using an actual original SMG request.

The file binding and observed request-size chunk limit are explicit procedure
inputs. No expected bytes or original kernel result reach native execution.
This does not install a native queue/worker/device owner or certify its timeline.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import struct
import subprocess
from original_xbox_pose_case import ROOT, cpu, physical_ram, PageTableMemory, reconstructed_replay_ram, sha


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    for key in ('case','read-oracle','request','output'):
        parser.add_argument('--'+key,type=Path,required=True)
    args=parser.parse_args();scratch=Path(os.environ['HALO2_TEST_SCRATCH'])
    output=args.output.absolute();output.mkdir(exist_ok=False)
    (output/'attempt.json').write_text(json.dumps(dict(runner_scratch=str(scratch),tool_sha256=sha(Path(__file__)),
        full_weapon_parity=False),indent=2)+'\n')
    request=json.loads(args.request.read_text());matches=request['owned_map_content_matches']
    assert len(matches)==1 and request['original_case_sha256']==sha(args.case/'call.json')
    assert request['read_oracle_sha256']==sha(args.read_oracle/'report.json')
    owned=Path(matches[0]['path']);assert sha(owned)==matches[0]['whole_file_sha256']
    candidates=request['observed_matching_task_candidates'];assert candidates
    observed=candidates[0]['observed_words']
    assert all(row['observed_words']==observed for row in candidates)
    assert observed[:4]==[request['observed_file_handle'],request['buffer'],request['bytes'],request['seek_offset']]
    assert observed[5]==0
    with reconstructed_replay_ram(args.case,args.read_oracle)as(bank,memory,report):
        initial=memory.read(request['buffer'],request['bytes'])
        assert hashlib.sha256(initial).hexdigest()==request['original_entry_buffer_sha256']
        (scratch/'entry-buffer.bin').write_bytes(initial)
    entry=dict(observed_task=observed,bytes=request['bytes'],owned_path=str(owned),
        declared_chunk_limit=request['bytes'],file_binding_authority='Explicit procedure binding selected by unique exact original exit-buffer content match')
    (scratch/'input.json').write_text(json.dumps(entry)+'\n')
    cpp=ROOT/'tools/probe_original_weapon_file_read.cpp'
    sources=[cpp,ROOT/'src/source_async_jobs.cpp',ROOT/'src/source_bitmap_async_read.cpp']
    headers=[ROOT/'include/halo2/source_async_jobs.hpp',ROOT/'include/halo2/source_bitmap_async_read.hpp']
    source_hashes={str(path):sha(path)for path in sources+headers}
    command=['c++','-std=c++20','-O2','-I'+str(ROOT/'include'),*[str(path)for path in sources],'-o',str(scratch/'native-read')]
    build=subprocess.run(command,capture_output=True,text=True)
    (output/'compile.log').write_text((build.stdout+build.stderr)[-65536:])
    assert build.returncode==0,build.stderr[-3000:]
    run=subprocess.run([str(scratch/'native-read'),str(scratch/'input.json'),str(scratch/'entry-buffer.bin'),
                        str(scratch/'native.json'),str(scratch/'native-buffer.bin')],capture_output=True,text=True,timeout=30)
    (output/'run.log').write_text((run.stdout+run.stderr)[-65536:]);assert run.returncode==0,run.stderr
    native=json.loads((scratch/'native.json').read_text());actual=(scratch/'native-buffer.bin').read_bytes()
    assert len(actual)==request['bytes']
    expected=request['original_full_call_exit_buffer_sha256'];equal=hashlib.sha256(actual).hexdigest()==expected
    original_task_exits=[]
    with physical_ram(args.case,'after')as bank:
        for row in candidates:
            address=row['physical_address'];words=list(struct.unpack('<10I',bank[address:address+40]))
            original_task_exits.append(dict(physical_address=address,observed_after_words=words,
                                           native_task_after_equal=words==native['task_after']))
    for path,digest in source_hashes.items():assert sha(path)==digest
    assert sha(owned)==matches[0]['whole_file_sha256']
    task_equal=any(row['native_task_after_equal']for row in original_task_exits)
    io_equal=native['actual_seek_low']==request['seek_offset']and native['actual_bytes_read']==request['bytes']
    result=dict(passed=equal and task_equal and io_equal,scope=__doc__,native=native,exact_original_exit_buffer=equal,
        exact_original_task_exit_observed=task_equal,actual_native_IO_matches_request=io_equal,
        compared_bytes=len(actual),native_buffer_sha256=hashlib.sha256(actual).hexdigest(),original_exit_buffer_sha256=expected,
        observed_task_inputs=observed,original_full_call_exit_task_observations=original_task_exits,
        declared_procedure_inputs=entry,original_queue_node_identity_verified=False,
        native_worker_queue_or_kernel_device_binding_installed=False,I_O_completion_timeline_verified=False,
        current_root_sources_compiled=True,source_files_sha256=source_hashes,
        compile_command=command,native_binary_sha256=sha(scratch/'native-read'),
        case_sha256=sha(args.case/'call.json'),request_report_sha256=sha(args.request),
        runner_scratch=str(scratch),persistent_binaries_or_RAM_retained=False,full_weapon_parity=False,
        physical_xbox_verified=False,tool_sha256=sha(Path(__file__)),helper_sha256=sha(ROOT/'tools/original_xbox_pose_case.py'))
    (output/'report.json').write_text(json.dumps(result,indent=2)+'\n')
    for path in [Path(__file__),ROOT/'tools/original_xbox_pose_case.py',*sources,*headers]:
        shutil.copyfile(path,output/path.name)
    print(json.dumps({k:result[k]for k in ('passed','compared_bytes','exact_original_exit_buffer','original_full_call_exit_task_observations','runner_scratch')}),flush=True)
    assert result['passed']


if __name__=='__main__':main()
