#!/usr/bin/env python3
"""Check unchanged whole1578A0 on host i386 and retain full MXCSR evidence.

Unicorn omits SSE sticky flags. Keep that original reference unchanged, verify
its GP/SIMD/flags/traces/stores/outputs against real CPU execution, and retain
only a small status sidecar. All ELF/image/bank/trace outputs are disposable.
Host CPU evidence does not establish physical Xbox CPU or visual acceptance.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import struct
from oracle_smg_firing_1_1 import Xbe,_Code,_trace
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
import oracle_physics_game_frame_hardware_20261011 as hardware


def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--input',type=Path,required=True);ap.add_argument('--fixture',type=Path,required=True)
    ap.add_argument('--report',type=Path,required=True);ap.add_argument('--verify',action='store_true');args=ap.parse_args()
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve()
    input_bytes=args.input.read_bytes();fixture=json.loads(input_bytes)
    assert fixture['original_xbe_sha256']==NEW_PIN and not fixture['instruction_patches'] and not fixture['supplied_dependency_returns']
    image=Xbe(NEW_DEFAULT);assert hashlib.sha256(image.data).hexdigest()==NEW_PIN
    code=_Code(image,'.text');body=_trace(code,0x1578a0,{0x1578a0})
    # Actual156A2E CALL1578A0's following instruction is a retained stop.
    call=code.at(0x156a2e);assert call.mnemonic=='call' and call.operands[0].imm==0x1578a0 and call.size==5
    stop=0x156a33
    paired=dict(entries={'001578a0':0x1578a0},address_map={},
        instruction_map={f'{pc:08x}':pc for pc in sorted(set(body.code)|{stop})})
    cases=[]
    for index,old in enumerate(fixture['cases']):
        banks={at:bytearray([old['poison']])*size for at,size in fixture['native_component_banks']}
        def write(at,data):
            start=next(p for p,raw in banks.items()if p<=at<at+len(data)<=p+len(raw))
            banks[start][at-start:at-start+len(data)]=data
        for at,bits in fixture.get('authored_operands',[])+old['patches']:write(at,bytes.fromhex(bits))
        write(old['gp'][4],struct.pack('<I',stop))
        cases.append(dict(id=index,entry=old['entry'],return_address=stop,exit_entry=stop,
            initial_banks=[dict(address=at,hex=raw.hex())for at,raw in banks.items()],exit={},entry_abi=dict(
                gp=old['gp'],eflags=0x202,mxcsr=0x1f80,fs_base=0,x87_control_word=0x27f,
                x87_status=0,x87_tag=0xffff,x87_values=[],
                xmm=[f'{int.from_bytes(struct.pack("<4I",*lanes),"little"):032x}'for lanes in old['xmm']])))
    result=hardware.execute(dict(image='1.5',cases=cases),paired,scratch/'original-camera-request')
    rows=[];differences=[]
    for old,actual in zip(fixture['cases'],result['cases']):
        index=actual['id'];state=actual['exit']
        assert state['gp']==old['final_gp'],(index,'GP')
        assert [list(struct.unpack('<4I',int(v,16).to_bytes(16,'little')))for v in state['xmm']]==old['final_xmm'],(index,'SIMD')
        assert (state['eflags']&~0x10)==(old['final_flags']&~0x10),(index,'flags')
        trace=[]
        for pc in actual['instructions']:
            if not trace or trace[-1]!=pc:trace.append(pc)
        assert trace==old['trace'],(index,'trace',trace,old['trace'])
        writes=[[w['instruction'],w['address'],w['before'],w['written']]for w in actual['writes']]
        assert writes==old['stores'],(index,'stores',writes,old['stores'])
        for at,bits in old['outputs']:
            expected=bytearray.fromhex(bits);sp=old['gp'][4]
            if at<=sp<at+len(expected):struct.pack_into('<I',expected,sp-at,stop)
            bank=next(b for b in actual['final_banks']if b['address']<=at<at+len(expected)<=b['address']+len(b['hex'])//2)
            start=at-bank['address'];observed=bytes.fromhex(bank['hex'])[start:start+len(expected)]
            assert observed==expected,(index,'output',hex(at))
        assert not state['x87_values'] and state['x87_control_word']==0x27f
        assert (state['mxcsr']^old['final_mxcsr'])&~0x3f==0,(index,'MXCSR control')
        row=dict(index=index,case_sha256=hashlib.sha256(json.dumps(old,separators=(',',':'),sort_keys=True).encode()).hexdigest(),final_mxcsr=state['mxcsr'])
        rows.append(row)
        if state['mxcsr']!=old['final_mxcsr']:differences.append(dict(index=index,emulator_mxcsr=old['final_mxcsr'],hardware_mxcsr=state['mxcsr']))
    sidecar=dict(original_xbe_sha256=NEW_PIN,component_reference_sha256=hashlib.sha256(input_bytes).hexdigest(),
        oracle_backend=result['oracle_backend'],instruction_patches=[],supplied_dependency_returns=[],cases=rows)
    raw=(json.dumps(sidecar,separators=(',',':'))+'\n').encode()
    if args.verify or args.fixture.exists():assert args.fixture.read_bytes()==raw,'Preserve original hardware status reference'
    else:args.fixture.parent.mkdir(parents=True,exist_ok=True);args.fixture.write_bytes(raw)
    report=dict(scope=__doc__,original_xbe_sha256=NEW_PIN,cases=len(rows),whole_original_hardware_cases=len(rows),
        fixture_bytes=len(raw),fixture_sha256=hashlib.sha256(raw).hexdigest(),component_reference_sha256=sidecar['component_reference_sha256'],
        original_instruction_patches=result['original_instruction_patches'],source_result_bits_traces_stores_outputs_agree=True,
        original_hardware_binary_sha256=result['original_hardware_binary_sha256'],hardware_tool_sha256=result['hardware_tool_sha256'],
        trace_observer_sha256=result['trace_observer_sha256'],single_stepped_original_instructions=result['single_stepped_original_instructions'],
        sticky_status_differences=differences,xbox_hardware_proof=False,retained_large_binaries_or_captures=[])
    args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({k:v for k,v in report.items()if k!='sticky_status_differences'}))


if __name__=='__main__':main()
