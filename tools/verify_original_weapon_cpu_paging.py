#!/usr/bin/env python3
"""Compare actual paging execution with the earlier original alias replay."""
import json
import os
from pathlib import Path
import shutil
from original_xbox_pose_case import BASE, cpu, physical_ram, PageTableMemory, sha


def main():
    scratch=Path(os.environ['HALO2_TEST_SCRATCH'])
    output=BASE/'original-weapon-cpu-paging-equivalence-v1'
    output.mkdir(exist_ok=False)
    (output/'attempt.json').write_text(json.dumps(dict(runner_scratch=str(scratch)),indent=2)+'\n')
    cases=[('plasma-original-charge-start-v1','plasma-original-charge-start-v1-replay-v1','plasma-original-charge-start-v1-paging-v3'),
           ('first-person-original-fire-call-v2','first-person-original-fire-call-v2-replay-v1','first-person-original-fire-call-v2-paging-v1')]
    results=[]
    for case,alias,paging in cases:
        old=json.loads((BASE/alias/'report.json').read_text());new=json.loads((BASE/paging/'report.json').read_text())
        assert old['passed']and new['passed']and new['architectural_CPU_paging']
        assert json.loads((BASE/alias/'instructions.json').read_text())==json.loads((BASE/paging/'instructions.json').read_text())
        old_rows=json.loads((BASE/alias/'writes.json').read_text());new_rows=json.loads((BASE/paging/'writes.json').read_text())
        with physical_ram(BASE/case,'before')as bank:
            memory=PageTableMemory(bank,cpu(BASE/case,'before'))
            def normalized(rows,virtual):
                result=[]
                for row in rows:
                    old_bytes=bytes.fromhex(row['before']);written=bytes.fromhex(row['written'])
                    assert len(old_bytes)==len(written)==row['bytes']
                    for i,(before,after)in enumerate(zip(old_bytes,written)):
                        address=memory.translate(row['address']+i)[0]if virtual else row['address']+i
                        result.append((row['instruction'],address,before,after))
                return result
            left,right=normalized(old_rows,True),normalized(new_rows,False)
            assert left==right
        for key in ('eip','gp','xmm','eflags','x87_control','x87_status','x87_tag','mxcsr'):
            assert old['actual_CPU'][key]==new['actual_CPU'][key]
        assert new['final_control_registers']==cpu(BASE/case,'after')['control_registers']
        results.append(dict(case=case,original_alias_report=alias+'/report.json',original_alias_report_sha256=sha(BASE/alias/'report.json'),
            architectural_paging_report=paging+'/report.json',architectural_paging_report_sha256=sha(BASE/paging/'report.json'),
            instructions=new['instruction_count'],ordered_stores=len(new_rows),source_written_bytes=len(right),
            instruction_sequence_equal=True,physical_ordered_byte_writes_equal=True,CPU_and_control_registers_equal=True,
            new_complete_RAM_reconstruction_sha256=new['complete_intermediate_RAM_reconstruction_sha256']))
    report=dict(passed=True,cases=results,paging_TLB_authority='https://github.com/unicorn-engine/unicorn/blob/master/docs/FAQ.md',
        original_instruction_patches=[],supplied_kernel_or_gameplay_returns=[],synthetic_memory_banks=[],
        runner_scratch=str(scratch),new_large_reference_inputs=False,physical_xbox_verified=False,full_weapon_parity=False,
        tool_sha256=sha(Path(__file__)),helper_sha256=sha(Path(__file__).with_name('original_xbox_pose_case.py')))
    (output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    shutil.copyfile(Path(__file__),output/Path(__file__).name)
    shutil.copyfile(Path(__file__).with_name('original_xbox_pose_case.py'),output/'original_xbox_pose_case.py')
    print(json.dumps(dict(passed=True,cases=results,runner_scratch=str(scratch))),flush=True)


if __name__=='__main__':main()
