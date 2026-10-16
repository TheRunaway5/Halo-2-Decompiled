#!/usr/bin/env python3
"""Trace unchanged TU1.5 AI code on the host CPU, retaining estimate boundaries.

The existing ptrace observer single-steps original instructions and reads
actual stores. Unicorn's approximate-SSE outputs remain a separate model;
neither game functions nor CPU estimate results are substituted. This does
not establish the Xbox Pentium III's estimate lookup or a physical Xbox run.
"""
import argparse,hashlib,json,struct
from pathlib import Path
from oracle_smg_firing_1_1 import Xbe,_Code,_trace
from oracle_charged_effect_lifetime_bank_tu15 import regions
from oracle_sandbox_first_person_hardware_tu15 import execute
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from verify_particle_frame_evidence_tu15 import equal_banks

def sha(p):return hashlib.file_digest(Path(p).open('rb'),'sha256').hexdigest()

def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--case',type=Path,required=True);ap.add_argument('--output',type=Path,required=True);a=ap.parse_args()
    case=a.case.resolve();out=a.output.resolve();assert not out.exists()
    original=json.loads((case/'report.json').read_text());hardware=json.loads((case/'hardware/report.json').read_text())
    assert original['completed'] and original['entry'] in (0x1d0670,0xb9090,0x26f260) and not original['instruction_patches'] and not original['supplied_game_services']
    assert hardware['completed'] and hardware['original_report_sha256']==sha(case/'report.json')
    assert sha(NEW_DEFAULT)==original['original_xbe_sha256']==NEW_PIN
    for name in ('before.bin','native-input.bin','original-trace.bin'):assert sha(case/name)==original['files_sha256'][name]
    image=Xbe(NEW_DEFAULT);code=_Code(image,'.text')
    trace=[p for p, in struct.iter_unpack('<I',(case/'original-trace.bin').read_bytes())]
    entries={original['entry'],*(int(p,16) for p in original['original_functions'])};reached=set(trace)
    for pc,target in zip(trace,trace[1:]):
        instruction=code.at(pc);assert instruction.bytes.hex()==original['executed_instruction_bytes'][f'{pc:08x}']
        if instruction.mnemonic=='call':entries.add(target)
    while True:
        more=set()
        for entry in entries:more.update(p for p in _trace(code,entry,entries).tail_jumps if p in reached)
        if more<=entries:break
        entries.update(more)
    sites=set(reached)
    for entry in entries:sites.update(_trace(code,entry,entries).code)
    paired=dict(entries={f'{p:08x}':p for p in entries},instruction_map={f'{p:08x}':p for p in sites},address_map={})
    raw=(case/'native-input.bin').read_bytes();assert len(raw)==168
    abi=dict(gp=list(struct.unpack_from('<8I',raw)),eflags=struct.unpack_from('<I',raw,160)[0],fs_base=struct.unpack_from('<I',raw,164)[0],
        x87_control_word=0x27f,x87_status=0,x87_tag=0xffff,x87_values=[],mxcsr=0x1f80,
        xmm=[f'{int.from_bytes(raw[32+n*16:48+n*16],"little"):032x}' for n in range(8)])
    if original['entry'] in (0x1d0670,0xb9090):
        assert abi['gp']==[0,0,0,0,0x600f0000,0,0,0] and abi['eflags']==2 and abi['fs_base']==0x60200000
    else:
        assert original['source_context']['actual_nested_caller']==0x26e5e4
        assert sha(case/'native-fp-input.bin')==original['files_sha256']['native-fp-input.bin']
        depth,status,cw,mxcsr=struct.unpack('<IHHI',(case/'native-fp-input.bin').read_bytes())
        assert depth==0 and cw==0x27f and abi['fs_base']==0x60200000
        assert dict(depth=depth,status=status,control=cw,mxcsr=mxcsr)==original['captured_caller_fp']
        abi.update(x87_control_word=cw,x87_status=status,mxcsr=mxcsr)
    fixture=dict(image='1.5',cases=[dict(id=1,entry=original['entry'],return_address=0x60300000,exit_entry=0x60300000,
        entry_abi=abi,exit=abi,original_instructions=trace,
        initial_banks=[dict(address=p,hex=data.hex()) for p,data in regions(case/'before.bin')])])
    result=execute(fixture,paired,out,include_reciprocal_estimates=True)
    observed=result['cases'][0];exit_=observed['exit'];depth=len(exit_['x87_values'])
    cpu=struct.pack('<8I',*exit_['gp'])+b''.join(int(p,16).to_bytes(16,'little') for p in exit_['xmm'])
    cpu+=struct.pack('<IIIHHI',exit_['eflags'],exit_['fs_base'],depth,exit_['x87_status'],exit_['x87_control_word'],exit_['mxcsr'])
    (out/'hardware-exit.bin').write_bytes(cpu)
    with (out/'hardware-after.bin').open('wb') as stream:
        for bank in observed['final_banks']:
            data=bytes.fromhex(bank['hex']);stream.write(struct.pack('<II',bank['address'],len(data)));stream.write(data)
    (out/'hardware-trace.bin').write_bytes(b''.join(struct.pack('<I',p) for p in observed['instructions']))
    with (out/'hardware-stores.bin').open('wb') as stream:
        for store in observed['writes']:
            if 0x60000000<=store['address']<0x60200000:continue
            before,written=bytes.fromhex(store['before']),bytes.fromhex(store['written']);assert len(before)==len(written)
            stream.write(struct.pack('<III',store['instruction'],store['address'],len(written)));stream.write(before);stream.write(written)
    expected=(case/'hardware/hardware-exit.bin').read_bytes();assert cpu==expected,(cpu[160:].hex(),expected[160:].hex())
    compared=equal_banks(out/'hardware-after.bin',case/'hardware/hardware-after.bin')
    from oracle_charged_effect_lifetime_bank_tu15 import stores
    layout=lambda p:[(pc,address,len(after)) for pc,address,before,after in stores(p)]
    assert layout(out/'hardware-stores.bin')==layout(case/'original-stores.bin'),'Hardware store coverage or control flow differs'
    estimates=[dict(row,mnemonic=code.at(row['instruction']).mnemonic) for row in observed['estimates']]
    comparison_flags=None
    if original['entry']==0x26f260:
        modeled=bytearray((case/'original-exit.bin').read_bytes());actual=bytearray(cpu)
        modeled_flags=struct.unpack_from('<I',modeled,160)[0]
        actual_flags=struct.unpack_from('<I',actual,160)[0]
        assert (modeled_flags^actual_flags)&~0x200==0x80
        struct.pack_into('<I',modeled,160,modeled_flags&~0x280)
        struct.pack_into('<I',actual,160,actual_flags&~0x280)
        assert modeled==actual
        assert 0x26f2c3 in trace and code.at(0x26f2c3).mnemonic=='fcompi'
        comparison_flags=dict(instruction=0x26f2c3,original_bytes=bytes(code.at(0x26f2c3).bytes).hex(),
            unicorn_eflags=modeled_flags,hardware_eflags=actual_flags,difference_except_IF=0x80,
            complete_instruction_path_equal=True,all_other_CPU_FP_bytes_equal=True,
            intel_architecture_reference='https://cdrdv2-public.intel.com/868137/325462-089-sdm-vol-1-2abcd-3abcd-4.pdf',
            scope='Original host FCOMIP clears SF; Unicorn retains SF. No source instruction or reference result is changed.')
    report=dict(scope=__doc__,tool_sha256=sha(__file__),original_report_sha256=sha(case/'report.json'),
        ordinary_hardware_report_sha256=sha(case/'hardware/report.json'),original_xbe_sha256=NEW_PIN,
        hardware_trace_equals_unicorn=(out/'hardware-trace.bin').read_bytes()==(case/'original-trace.bin').read_bytes(),
        ordinary_hardware_full_cpu_exit_equal=True,ordinary_hardware_retained_bytes_equal=compared,
        instructions=len(observed['instructions']),writes=sum(not 0x60000000<=s['address']<0x60200000 for s in observed['writes']),
        estimate_boundaries=estimates,fp_comparison_flags=comparison_flags,tracer_source_sha256=result['trace_observer_sha256'],
        hardware_store_layout_equals_unicorn=True,
        source_instruction_patches=[],supplied_game_services=[],cpu_estimates_observed_without_replacement=True,
        xbox_estimate_bits_verified=False,physical_xbox_verified=False,full_weapon_parity=False,
        files_sha256={p.name:sha(p) for p in out.iterdir() if p.is_file()},passed=True)
    (out/'report.json').write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
    print(json.dumps({k:report[k] for k in ('passed','instructions','writes','hardware_trace_equals_unicorn','ordinary_hardware_retained_bytes_equal')}))

if __name__=='__main__':main()
