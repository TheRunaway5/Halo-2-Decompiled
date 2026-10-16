#!/usr/bin/env python3
"""Verify live TU1.5 shadow calls with unchanged host i386 execution.

All retained native before-state is input. Native after-state is read only after
original execution; no instruction, callee, selection or floating-point status
is replaced. This is original CPU evidence, not physical Xbox GPU/frame proof.
"""
import argparse,hashlib,json
from pathlib import Path
from differential_physics_native_frame_tu15_20261012 import regions
from generate_combat_action_tu15 import CombatCode
import oracle_physics_game_frame_hardware_20261011 as hardware


def sha(path):
    with Path(path).open('rb') as stream:return hashlib.file_digest(stream,'sha256').hexdigest()


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--case',type=Path,required=True)
    p.add_argument('--instruction-trace',type=Path,required=True);p.add_argument('--output',type=Path,required=True)
    a=p.parse_args();assert not a.output.exists()
    call=json.loads((a.case/'call.json').read_text());assert call['entry'] in (0x333f0,0x134c00,0x168900,0x44710,0x40e80,0x2c470,0x1be90,0x1e1a0,0x22690,0x3baa0) and call['authority']=='OG Xbox TU1.5'
    entry=json.loads((a.case/'before-abi.json').read_text());assert entry['x87_depth']==0
    banks=[dict(address=p,hex=b.hex())for p,b in regions(a.case/'before.bin')]
    sp=entry['gp'][4]
    stop=next(int.from_bytes(bytes.fromhex(b['hex'])[sp-b['address']:sp-b['address']+4],'little')for b in banks if b['address']<=sp< b['address']+len(b['hex'])//2)
    frame=dict(id=0,entry=call['entry'],return_address=stop,exit_entry=stop,initial_banks=banks,exit={},
        entry_abi=dict(gp=entry['gp'],eflags=entry['eflags'],mxcsr=entry['mxcsr'],fs_base=entry['fs_base'],
                       x87_control_word=entry['x87_control'],x87_status=entry['x87_status'],x87_tag=0xffff,x87_values=[],
                       xmm=[f'{sum(value<<(32*k)for k,value in enumerate(lanes)):032x}'for lanes in entry['xmm']]))
    trace=json.loads(a.instruction_trace.read_text())
    pair=dict(address_map={},entries={f"{call['entry']:08x}":call['entry']},instruction_map={f'{p:08x}':p for p in trace})
    hardware._Code=lambda image,section:CombatCode(image)
    original=hardware.execute(dict(image='1.5',cases=[frame]),pair,a.output)
    # No native expected return/bank/status was available to original execution.
    expected=json.loads((a.case/'after-abi.json').read_text())
    observed=original['cases'][0];state=observed['exit']
    xmm=[[(int(value,16)>>(32*k))&0xffffffff for k in range(4)]for value in state['xmm']]
    result={b['address']:bytes.fromhex(b['hex'])for b in observed['final_banks']}
    compared=0;differences=[]
    for address,data in regions(a.case/'after.bin'):
        actual=result[address];compared+=len(data)
        if actual!=data:
            first=next(n for n,(x,y)in enumerate(zip(actual,data))if x!=y)
            differences.append(dict(address=address+first,original=actual[first],native=data[first]))
    checks=dict(gp=state['gp']==expected['gp'],xmm=xmm==expected['xmm'],
                defined_flags=(state['eflags']&0xcd5)==(expected['eflags']&0xcd5),
                x87_status=state['x87_status']==expected['x87_status'],x87_depth=not state['x87_values'],
                x87_control=state['x87_control_word']==expected['x87_control'],mxcsr=state['mxcsr']==expected['mxcsr'],
                retained_memory=not differences,ordered_instruction_path=observed['instructions']==trace)
    report=dict(scope=__doc__,passed=all(checks.values()),checks=checks,compared_bytes=compared,
                instructions=original['single_stepped_original_instructions'],native_after_abi=expected,original_after_abi=state,
                memory_differences=differences[:20],original_sha256=original['original_sha256'],
                tool_sha256=sha(__file__),original_hardware_binary_sha256=original['original_hardware_binary_sha256'],
                hardware_tool_sha256=original['hardware_tool_sha256'],instruction_patches=original['original_instruction_patches'],
                native_expected_used_as_original_input=original['native_expected_data_used'],
                before_sha256=sha(a.case/'before.bin'),native_after_sha256=sha(a.case/'after.bin'),
                oracle_backend=original['oracle_backend'],physical_xbox_gpu_proof=False,original_framebuffer_parity=False,full_goal_complete=False)
    (a.output/'comparison.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({k:report[k]for k in ['passed','checks','instructions','compared_bytes']}))
    assert report['passed'],'Native selection differs from unchanged i386 execution'


if __name__=='__main__':main()
