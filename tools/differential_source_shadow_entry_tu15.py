#!/usr/bin/env python3
"""Unchanged TU1.5 per-entry12D30 and44360 table proof; CPU/SDK-shell scope."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import struct
import subprocess

from oracle_smg_firing_1_1 import Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn.x86_const import *

ROOT=Path(__file__).resolve().parents[1]
def packed(values):return struct.pack('<'+'I'*len(values),*values)
def words(raw):return list(struct.unpack('<'+'I'*(len(raw)//4),raw))
def bits(value):return words(struct.pack('<f',value))[0]
def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,required=True);a=ap.parse_args()
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve()
    if not scratch.is_dir():raise RuntimeError('Owned HALO2_TEST_SCRATCH required')
    probe=scratch/'source-shadow-entry-probe'
    sources=['tools/probe_source_shadow_entry_tu15.cpp','src/rendering/shadows/source_shadow_raw_draw.cpp',
        'src/gameplay/camera/source_view_projection.cpp','src/rendering/renderer/render_view.cpp',
        'src/core/transform.cpp','src/rendering/source_shader_lod.cpp','src/rendering/source_geometry_lod.cpp']
    subprocess.run(['c++','-std=c++20','-O2','-ffp-contract=off','-ffunction-sections','-fdata-sections',
        '-I'+str(ROOT/'include'),*[str(ROOT/p) for p in sources],'-Wl,--gc-sections','-o',str(probe)],check=True)
    image=Xbe(NEW_DEFAULT);assert hashlib.sha256(image.data).hexdigest()==NEW_PIN
    assert words(image.read(0x44a8a0,4))==[0x3e19999a]
    cpu=Uc(UC_ARCH_X86,UC_MODE_32);cpu.mem_map(0x10000,0x600000)
    for section in image.sections:
        raw=image.section_bytes(section)
        if section.va+len(raw)<=0x610000:cpu.mem_write(section.va,raw)
    cpu.mem_map(0x2100000,0x100000);cpu.mem_map(0x3000000,0x1000)
    bank,out,device,push,stack,stop=0x2100000,0x2101000,0x2102000,0x2106000,0x21fef00,0x3000000
    executed={}
    def instruction(machine,pc,n,_):
        if pc==stop:machine.emu_stop();return
        raw=bytes(machine.mem_read(pc,n));assert image.read(pc,n)==raw,hex(pc)
        assert executed.setdefault(pc,raw)==raw
    cpu.hook_add(UC_HOOK_CODE,instruction)
    def put(p,v):cpu.mem_write(p,packed([v]))
    def run(entry,regs,args=(),until=stop):
        for reg in (UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,
            UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP):cpu.reg_write(reg,0)
        for reg,value in regs.items():cpu.reg_write(reg,value)
        for reg,value in ((UC_X86_REG_FPCW,0x27f),(UC_X86_REG_MXCSR,0x1f80),
            (UC_X86_REG_EFLAGS,2),(UC_X86_REG_FPTAG,0xffff),(UC_X86_REG_FPSW,0)):cpu.reg_write(reg,value)
        cpu.reg_write(UC_X86_REG_ESP,stack);cpu.mem_write(stack,packed([stop,*args]))
        try:cpu.emu_start(entry,until,count=2000000)
        except Exception as error:raise RuntimeError(f'Original entry{entry:x} failed at{cpu.reg_read(UC_X86_REG_EIP):x}; gp='+
            ','.join(hex(cpu.reg_read(r)) for r in (UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI))) from error
        assert cpu.reg_read(UC_X86_REG_EIP)==until
    fixture=ROOT/'tests/fixtures/retail_source_view_projection.tsv'
    inputs=[];expected=[]
    for row in [list(map(int,line.split())) for line in fixture.read_text().splitlines()]:
        assert len(row)==109
        raw_bank=row[33:81];distance=row[32]
        # Actual completed native projection banks are immutable reference
        # operands. The SDK push shell is declared caller storage only.
        minimum=bits((1.0/struct.unpack('<f',packed([distance]))[0])*16777215.0*.01)
        for foreground in (0,1):
            cpu.mem_write(bank,packed(raw_bank));cpu.mem_write(device,bytes(0x3000))
            put(0x411568,device);put(device,push);put(device+4,push+0x3000)
            put(device+0x794,0x2105000);put(0x2105004,0)
            put(0x4922fc,minimum);put(0x492300,bits(16777215.0));put(0x491e7c,distance)
            run(0x12d30,{UC_X86_REG_EAX:bank,UC_X86_REG_EBX:foreground},[out])
            expected.append([*words(cpu.mem_read(out,64)),*words(cpu.mem_read(device+0x1c10,4))])
            inputs.append([foreground,*raw_bank,distance,minimum,0x3e19999a])
    output=subprocess.run([str(probe)],input=''.join(' '.join(map(str,row))+'\n' for row in inputs),
        text=True,capture_output=True,check=True)
    actual=[list(map(int,line.split())) for line in output.stdout.splitlines()]
    assert len(actual)==len(expected)
    failures=[i for i,(x,y) in enumerate(zip(actual,expected)) if x!=y]
    # Exact44360 slice loads the authored per-layer AX selector immediately
    # before calling15660. Entry fade never writes that enable selector.
    layers=[]
    for frame in range(9):
        for layer in (10,11):
            put(0x4920b8,frame)
            run(0x444a2,{UC_X86_REG_ESI:layer*36},until=0x444bc)
            selector=cpu.reg_read(UC_X86_REG_EAX)&65535;assert selector==0
            layers.append({'frame_type':frame,'layer':layer,'native_15660_mode':selector})
    assert subprocess.run([str(probe),'--layers'],text=True,capture_output=True,check=True).stdout.split()==['0','0']
    # Execute complete15660 mode0 and every4D630/4D710 alpha transition.
    # The SDK may cache/upload32rows, but no entry may enable that pattern.
    run(0x1c190,{})
    fade_cases=0;pattern_uploads=0
    for layer in (10,11):
        for alpha in range(256):
            cpu.mem_write(device,bytes(0x3000));put(0x411568,device)
            put(device,push);put(device+4,push+0x3000)
            put(0x40f0c0,push+0x1000);put(0x40f0c4,push+0x2000)
            run(0x15660,{UC_X86_REG_EAX:0})
            assert words(cpu.mem_read(0x4c4b68,4))==[0] and words(cpu.mem_read(0x4116c4,4))==[0]
            pattern_begin=words(cpu.mem_read(device,4))[0]
            simple_begin=words(cpu.mem_read(0x40f0c0,4))[0]
            cpu.mem_write(0x4c6958,bytes(32));cpu.mem_write(0x4c6972,bytes([alpha]))
            put(0x4c4cb4,bits(.625))
            run(0x4d630,{UC_X86_REG_EDI:layer},[0,0])
            assert words(cpu.mem_read(0x4c4b68,4))==[0] and words(cpu.mem_read(0x4116c4,4))==[0]
            run(0x4d710,{UC_X86_REG_EDI:layer},[0])
            assert words(cpu.mem_read(0x4c4b68,4))==[0] and words(cpu.mem_read(0x4116c4,4))==[0]
            pattern_end=words(cpu.mem_read(device,4))[0]
            assert words(cpu.mem_read(0x40f0c0,4))[0]==simple_begin
            commands=words(cpu.mem_read(pattern_begin,pattern_end-pattern_begin))
            if alpha==255:assert not commands
            else:
                assert len(commands)==66 and commands[0]==0x801480 and commands[33]==0x801480
                pattern_uploads+=2
            fade_cases+=1
    report={'passed':not failures,'source_image_sha256':NEW_PIN,'cases':len(inputs),'compared_words':len(inputs)*17,
        'failures':failures[:16],'source_entries':['12D30 whole with explicit output (no constant-upload call)',
            '44360 slice444A2..444BC before15660','15660 whole mode0','1C190 whole',
            '4D630/4D710 whole with layers10/11, all256fade bytes'],'unchanged_instructions':len(executed),
        'source_instruction_bytes_verified':True,'arithmetic_hooks':[],'layer_modes':layers,
        'authenticated_44a8a0':'3e19999a','graphics_submission':False,'hardware_framebuffer_parity':False,
        'fade_cases':fade_cases,'pattern_uploads':pattern_uploads,'entry_stipple_enables':0,
        'declared_caller_storage':'Private SDK device/push/output shell; no completion or renderer readiness supplied',
        'sha256':{p:hashlib.sha256((ROOT/p).read_bytes()).hexdigest() for p in sources+
            ['tools/differential_source_shadow_entry_tu15.py','include/halo2/source_shadow_raw_draw.hpp',
             'src/rendering/shadows/source_shadow_scene.cpp','tests/fixtures/retail_source_view_projection.tsv']}}
    a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({k:report[k] for k in ('passed','cases','compared_words','failures','unchanged_instructions')}))
    if failures:
        i=failures[0];raise RuntimeError(f'Native entry mismatch{i}: original={expected[i]} native={actual[i]}')
if __name__=='__main__':main()
