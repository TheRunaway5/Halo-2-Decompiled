#!/usr/bin/env python3
"""Replay typed 1DA750 inputs against the unchanged TU1.5 i386 parent.

Run under tools/run_test.py. The manifest supplies real retained banks and
typed formals only. The hardware runner creates its own disposable caller
stack and compares the retained postimage and explicit FP output.
"""
import argparse
from collections import Counter
import hashlib
import json
import os
from pathlib import Path
import struct
import subprocess
import sys
import tempfile

sys.dont_write_bytecode = True

from oracle_smg_firing_1_1 import ROOT, Xbe, _Code
from replay_live_phantom_hardware_tu15 import reference_text_gaps
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN

ENTRY = 0x1da750
BRIDGE = 0x70000000
CONTRACT = 'native_tu15_lift_parent_v1'
FP_KEYS = {'control_word', 'status', 'mxcsr', 'x87_depth'}
PHASES = {
    'member_inputs': 0x1da8e7, 'first_marker': 0x1da9a6, 'marker_miss_transform': 0x1da9c2,
    'radial_scalar': 0x1dabbe, 'tangential_scalar': 0x1daf99, 'axial_scalar': 0x1db2d0,
    'angular_scalar': 0x1db4db, 'linear_delivery': 0x1db370, 'angular_delivery': 0x1db64f,
    'linear_motion_store': 0x3239f7, 'angular_motion_store': 0x323a07,
    'body_activation': 0x315cb0, 'island_activation': 0x316d40, 'array_release': 0x149e00,
}

ASM = r'''
.code32
.section .bridge,"awx"
.globl _start
_start:
 mov $harness_stack_end,%esp
 mov $243,%eax
 mov $tls,%ebx
 int $0x80
 test %eax,%eax
 jnz failure
 mov tls,%eax
 shl $3,%eax
 or $3,%eax
 mov %ax,%fs
 fninit
 fldenv environment
 ldmxcsr simd_control
 pushl $2
 popfl
 mov $source_stack_top-12,%esp
 int3
 jmp 0x1da750
.globl source_return
source_return:
 ud2
.globl capture
capture:
 mov %esp,output
 stmxcsr output+4
 fnstenv output+8
 mov $harness_stack_end,%esp
 mov $output,%ecx
 mov $36,%edx
 call write_exact
EMIT_BANKS
 xor %ebx,%ebx
 jmp terminate
write_exact:
 test %edx,%edx
 jz io_done
 mov $4,%eax
 mov $1,%ebx
 int $0x80
 cmp $-4,%eax
 je write_exact
 test %eax,%eax
 jle failure
 add %eax,%ecx
 sub %eax,%edx
 jmp write_exact
io_done:
 ret
failure:
 mov $1,%ebx
terminate:
 mov $1,%eax
 int $0x80
.balign 16
tls: .long -1,FS_BASE,0xfffff,0x51
environment: .incbin "FP_INPUT"
simd_control: .long MXCSR
output: .space 36
harness_stack: .space 65536
harness_stack_end:
.section .source_stack,"aw"
.space 65536-12
.long source_return,COMPONENT,FACTOR_BITS
source_stack_top:
'''


def sha(path):
    with Path(path).open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def uint(value, width=32):
    assert type(value) is int and 0 <= value < 1 << width, value
    return value


def assembler_path(path):
    return str(path).replace('\\', '\\\\').replace('"', '\\"')


def capture_manifest(capture):
    data = json.loads((capture/'capture.json').read_text())
    assert data['comparison_contract'] == CONTRACT and data['entry'] == ENTRY
    assert isinstance(data['source_phase'], str) and data['source_phase']
    assert not ({'gp', 'xmm', 'before', 'after', 'stack', 'private_stack', 'x87_values'} & data.keys())
    assert data['capture_origin'] in ('live_imported_mcc', 'declared_native_component')
    if data['capture_origin'] == 'live_imported_mcc':
        assert data['runtime_cache_source'] == 'MCC' and data['map']
        runtime = data['runtime_sha256']
        assert len(runtime) == 64 and all(c in '0123456789abcdef' for c in runtime)
    assert not data.get('newly_committed_regions')
    uint(data['component'])
    uint(data['factor_bits'])
    uint(data.get('fs_data_base', 0))
    for phase in ('before_fp', 'after_fp'):
        fp = data[phase]
        assert set(fp) == FP_KEYS
        uint(fp['control_word'], 16)
        uint(fp['status'], 16)
        uint(fp['mxcsr'])
        assert fp['x87_depth'] == 0, 'Typed lift proof admits empty x87 data at entry and exit'
    regions = sorted(data['regions'], key=lambda row: row['address'])
    assert regions
    end = 0
    for region in regions:
        address, size = uint(region['address']), uint(region['size'])
        assert size and end <= address and address + size <= 1 << 32
        end = address + size
        for field in ('before_file', 'after_file'):
            path = (capture/region[field]).resolve()
            assert path.is_relative_to(capture) and path.stat().st_size == size
        assert not region.get('private_stack')
    assert not any(r['address'] < BRIDGE + 0x20000 and r['address']+r['size'] > BRIDGE for r in regions)
    data['regions'] = regions
    return data


def stack_location(regions):
    size = 0x10000
    for address in range(0x600e0000, 0x6e000000, 0x100000):
        if all(r['address'] >= address+size or r['address']+r['size'] <= address for r in regions):
            return address
    raise RuntimeError('No disjoint offline original caller-stack range')


def commands(image, capture, data, scratch):
    regions = data['regions']
    stack = stack_location(regions)
    text, gaps, authentication = reference_text_gaps(image, capture, regions, scratch)
    assembly, sections = [], []
    for index, region in enumerate(sorted([*regions,*gaps], key=lambda row: row['address'])):
        path = region.get('reference_file') or capture/region['before_file']
        assembly.append(f'.section .bank{index},"awx"\n.incbin "{assembler_path(path)}"')
        sections.append(f' .bank{index} 0x{region["address"]:x} : {{ *(.bank{index}) }}')
    emit = '\n'.join(f' mov ${r["address"]},%ecx\n mov ${r["size"]},%edx\n call write_exact' for r in regions)
    fp = data['before_fp']
    environment = bytearray(28)
    for offset,value in ((0,fp['control_word']),(4,fp['status']),(8,0xffff)):
        struct.pack_into('<H',environment,offset,value)
    env_path=scratch/'entry-fp.bin'
    env_path.write_bytes(environment)
    asm = ASM.replace('EMIT_BANKS',emit).replace('FS_BASE',str(data.get('fs_data_base',0)))
    asm = asm.replace('FP_INPUT',assembler_path(env_path)).replace('MXCSR',str(fp['mxcsr']))
    asm = asm.replace('COMPONENT',str(data['component'])).replace('FACTOR_BITS',str(data['factor_bits']))
    (scratch/'bridge.s').write_text(asm+'\n'+'\n'.join(assembly)+'\n')
    sections.extend((f' .source_stack 0x{stack:x} : {{ *(.source_stack) }}',
                     f' .bridge 0x{BRIDGE:x} : {{ *(.bridge) }}'))
    (scratch/'link.ld').write_text('ENTRY(_start)\nSECTIONS {\n'+'\n'.join(sections)+'\n}\n')
    return text, authentication, stack


def compare_output(stream, capture, manifest):
    differences=[]
    compared=0
    digests={phase:hashlib.sha256() for phase in ('before','native_after','original_after')}
    for region in manifest['regions']:
        first=region['address']
        for digest in digests.values():
            digest.update(struct.pack('<II',first,region['size']))
        with (capture/region['before_file']).open('rb') as before, (capture/region['after_file']).open('rb') as expected:
            offset=0
            first_mismatch=None
            while offset < region['size']:
                width=min(1<<20,region['size']-offset)
                initial=before.read(width)
                reference=expected.read(width)
                actual=stream.read(width)
                assert len(initial)==len(actual)==len(reference)==width
                for phase,raw in (('before',initial),('native_after',reference),('original_after',actual)):
                    digests[phase].update(raw)
                if first_mismatch is None and actual!=reference:
                    i=next(i for i,(a,b) in enumerate(zip(actual,reference,strict=True)) if a!=b)
                    first_mismatch=dict(address=first+offset+i,original=actual[i:i+8].hex(),
                                        native=reference[i:i+8].hex())
                offset+=width
                compared+=width
            if first_mismatch:
                differences.append(first_mismatch)
    assert stream.read(1)==b''
    return compared,differences,{phase:digest.hexdigest() for phase,digest in digests.items()}


def replay(capture, scratch, report, require_linear, require_angular):
    data = capture_manifest(capture)
    assert sha(NEW_DEFAULT) == NEW_PIN
    image = Xbe(NEW_DEFAULT)
    text, auth, stack = commands(image,capture,data,scratch)
    def run(command, **options):
        result=subprocess.run(command,cwd=scratch,capture_output=True,text=True,**options)
        if result.returncode:
            raise RuntimeError(f'{command[0]} returned {result.returncode}: {result.stderr[-2500:]}')
        return result.stdout
    run(['as','--32','bridge.s','-o','bridge.o'])
    run(['ld','-melf_i386','-T','link.ld','bridge.o','-o','original-hardware'])
    symbols={name:int(address,16) for address,kind,name in (line.split() for line in run(['nm','original-hardware']).splitlines())}
    tracer=ROOT/'tools/live_phantom_hardware_trace_tu15.cpp'
    run(['c++','-std=c++20','-O2',str(tracer),'-o','observer'])
    trace_path=scratch/'trace.json'
    try:
        run(['./observer','./original-hardware','output.bin',hex(symbols['source_return']),
             hex(symbols['capture']),hex(text.va+text.vsize),str(trace_path)],timeout=120)
    except RuntimeError:
        if trace_path.exists():
            trace=json.loads(trace_path.read_text())
            report['physical_failure']={key:trace[key] for key in (
                'signal','fault_instruction','fault_address','escaped_address','instructions_tail') if key in trace}
        raise
    trace=json.loads(trace_path.read_text())
    instructions=trace['instructions']
    assert instructions[0]==ENTRY and instructions[-1] in (0x1db7dc,0x1db7f0)
    last=_Code(image,'.text').at(instructions[-1])
    assert last.bytes==bytes.fromhex('c20800') and trace['original_return']==symbols['source_return']
    counts=Counter(instructions)
    phases={label:counts[pc] for label,pc in PHASES.items()}
    with (scratch/'output.bin').open('rb') as stream:
        record=stream.read(36)
        assert len(record)==36
        sp,mxcsr=struct.unpack_from('<II',record)
        cw,sw,tag=struct.unpack_from('<H2xH2xH',record,8)
        fp=dict(control_word=cw,status=sw,mxcsr=mxcsr,x87_depth=sum((tag>>(i*2)&3)!=3 for i in range(8)))
        stack_delta=sp-(stack+0x10000-12)
        compared,differences,bank_digests=compare_output(stream,capture,data)
    report.update(
        capture_manifest_sha256=sha(capture/'capture.json'),capture_origin=data['capture_origin'],
        source_phase=data['source_phase'],component=data['component'],factor_bits=data['factor_bits'],
        before_fp=data['before_fp'],fs_data_base=data.get('fs_data_base',0),runtime_sha256=data.get('runtime_sha256'),
        runtime_cache_source=data.get('runtime_cache_source'),map=data.get('map'),
        source_code_authentication=auth,original_hardware_sha256=sha(scratch/'original-hardware'),
        original_instruction_count=len(instructions),phase_counts=phases,
        original_return_pc=instructions[-1],original_ret_argument_bytes=8,original_stack_delta=stack_delta,
        compared_retained_bytes=compared,retained_region_count=len(data['regions']),
        retained_cohort_sha256=bank_digests,
        source_fp=fp,native_fp=data['after_fp'],bank_differences=differences,
        original_stack_generated_offline=True,raw_gp_or_simd_supplied=False)
    assert stack_delta==12,'Actual original RET8 caller cleanup differs'
    assert fp==data['after_fp'],'Typed source FP postimage differs'
    assert not differences,'Typed source retained bank differs'
    if require_linear:
        assert phases['linear_delivery'] and phases['linear_motion_store']
    if require_angular:
        assert phases['angular_scalar'] and phases['angular_delivery'] and phases['angular_motion_store']
    report['passed']=True


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--capture',required=True,type=Path)
    parser.add_argument('--report-output',required=True,type=Path)
    parser.add_argument('--require-linear',action='store_true')
    parser.add_argument('--require-angular',action='store_true')
    args=parser.parse_args()
    owner=Path(os.environ['HALO2_TEST_SCRATCH']).resolve()
    scratch=Path(tempfile.mkdtemp(prefix='typed-lift-original-',dir=owner))
    output=args.report_output.resolve()
    assert not output.exists()
    output.parent.mkdir(parents=True,exist_ok=True)
    report=dict(passed=False,comparison_contract=CONTRACT,original_sha256=NEW_PIN,
        scratch_cleanup_owner=str(scratch.parent),retained_large_artifacts=[],
        source_sha256={str(p.relative_to(ROOT)):sha(p) for p in (
            Path(__file__).resolve(),ROOT/'tools/replay_live_phantom_hardware_tu15.py',
            ROOT/'tools/live_phantom_hardware_trace_tu15.cpp')})
    try:
        replay(args.capture.resolve(),scratch,report,args.require_linear,args.require_angular)
    except BaseException as error:
        report['failure']=repr(error)
        raise
    finally:
        report['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in scratch.rglob('*') if p.is_file())
        output.write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')


if __name__=='__main__':
    main()
