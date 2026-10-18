#!/usr/bin/env python3
"""Compare an authenticated actual constructor trace, stores, CPU and bank with original x86.

The real native call supplies its own complete instruction trace. No separate
subroutine replay or captured result supplies the native output. The source
stack remains outside the ordinary non-stack bank comparison.
"""
import argparse,json,struct
from pathlib import Path
from verify_captured_source_api_tu15 import sha,regions,read
from validate_captured_shader_function_startup_tu15 import validate as validate_shader
from validate_captured_render_object_startup_tu15 import validate as validate_render


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--actual',type=Path,required=True);p.add_argument('--original',type=Path,required=True);p.add_argument('--output',type=Path,required=True)
    a=p.parse_args();actual,original=a.actual,a.original;assert not a.output.exists()
    metadata=json.loads((actual/'native.json').read_text());assert metadata['completed'] and metadata['entry'] in (0x10fa30,0x171e10,0x228260)
    validator=validate_shader if metadata['entry']==0x10fa30 else validate_render
    context=validator(actual,metadata,list(regions(actual/'before.bin')))
    report=json.loads((original/'report.json').read_text());hardware=json.loads((original/'hardware/report.json').read_text())
    assert report['completed'] and report['source_context']==context
    assert report['entry']==metadata['entry'] and not report['instruction_patches'] and not report['supplied_game_services'] and not report['native_platform_services']
    assert hardware['original_report_sha256']==sha(original/'report.json')
    assert hardware['emulator_retained_state_matches_hardware'] and hardware['entire_exit_equal_except_platform_IF']
    for name in ('before.bin','native-input.bin','original-after.bin','original-exit.bin','original-trace.bin','original-stores.bin'):
        assert sha(original/name)==report['files_sha256'][name]
    assert sha(original/'hardware/hardware-after.bin')==hardware['hardware_after_sha256']
    assert sha(original/'hardware/hardware-exit.bin')==hardware['hardware_exit_sha256']
    native= (actual/'native-input.bin').read_bytes();sp=struct.unpack_from('<I',native,16)[0]
    before=dict(regions(actual/'before.bin'));adapted=dict(regions(original/'before.bin'));differences=[]
    for base,data in before.items():
        expected=bytearray(data)
        if base<=sp and sp+4<=base+len(data):expected[sp-base:sp-base+4]=bytes.fromhex(report['caller_after_hex'])[:4]
        if bytes(expected)!=adapted.get(base):differences.append(base)
    cpu=bytearray((original/'hardware/hardware-exit.bin').read_bytes());assert len(cpu)==180;cpu[161]&=0xfd
    after=dict(regions(actual/'after.bin'));reference=dict(regions(original/'hardware/hardware-after.bin'));compared=0;bad=[]
    assert set(after)==set(reference)
    for base,data in reference.items():
        if 0x60000000<=base<0x60200000:continue
        compared+=len(data)
        if after[base]!=data:bad.append(base)
    checks=dict(authenticated_actual_native_constructor=True,
        sole_declared_return_input_adaptation=set(before)==set(adapted) and not differences,
        native_cpu_input_exact=native==(original/'native-input.bin').read_bytes(),
        native_fp_input_exact=(actual/'native-fp-input.bin').read_bytes()==(original/'native-fp-input.bin').read_bytes(),
        actual_complete_instruction_trace_exact=(actual/'native-trace.bin').read_bytes()==(original/'original-trace.bin').read_bytes(),
        actual_ordered_nonstack_stores_exact=(actual/'native-stores.bin').read_bytes()==(original/'original-stores.bin').read_bytes(),
        actual_full_hardware_cpu_fp_except_IF_exact=(actual/'native-exit.bin').read_bytes()==cpu,
        actual_complete_nonstack_bank_exact=not bad)
    result=dict(scope=__doc__,tool_sha256=sha(Path(__file__)),passed=all(checks.values()),checks=checks,
        entry=metadata['entry'],original_instructions=report['original_instructions'],original_nonstack_writes=report['retained_stores'],retained_nonstack_bytes=compared,
        actual_API_instruction_trace_available=True,actual_build_sha256=sha(actual/'build.json'),original_report_sha256=sha(original/'report.json'),hardware_report_sha256=sha(original/'hardware/report.json'),
        instruction_patches=[],supplied_game_services=[],physical_xbox_verified=False,full_weapon_parity=False)
    a.output.write_text(json.dumps(result,indent=2)+'\n');assert result['passed'],checks
    print(json.dumps({k:result[k] for k in ('passed','entry','original_instructions','original_nonstack_writes','retained_nonstack_bytes')}))


if __name__=='__main__':main()
