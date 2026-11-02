#!/usr/bin/env python3
"""Compare unchanged title lifecycle13F160 on i386 CPU and emulator.

All state, timing records and clock words are declared inputs. No original
instruction or dependency is replaced. This is component evidence, not live
title presentation, scheduler integration or mission acceptance. Run through
run_test.py; only the bounded report may leave the owned scratch directory.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import struct
import subprocess

from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_WRITE
from unicorn import x86_const as R
from oracle_smg_firing_1_1 import Xbe, _Code, _trace
from oracle_workshop_titles_tu15 import GP, STATE, CLOCK, STACK, STOP
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--output',type=Path,required=True)
    a=p.parse_args();scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    assert a.output.resolve().is_relative_to(scratch)
    image=Xbe(NEW_DEFAULT);assert hashlib.sha256(image.data).hexdigest()==NEW_PIN
    code=_Code(image,'.text');body=_trace(code,0x13f160,{0x13f160})
    assert len(body.code)==80 and code.at(0x13ab71).mnemonic=='call'
    assert code.at(0x13ab71).operands[0].imm==0x13f160
    assert not any(code.at(pc).mnemonic=='call' for pc in body.code)
    g=Uc(UC_ARCH_X86,UC_MODE_32)
    for at,size in ((0x10000,0x600000),(STATE,0x100000),(STOP,4096)):g.mem_map(at,size)
    for section in image.sections:g.mem_write(section.va,image.section_bytes(section))
    def put(at,value):g.mem_write(at,struct.pack('<I',value&0xffffffff))
    def bits(value):return struct.unpack('<I',struct.pack('<f',value))[0]
    trace=[];stores=[];covered=set()
    def instruction(vm,pc,size,_):
        assert pc in body.code and bytes(vm.mem_read(pc,size))==image.read(pc,size),hex(pc)
        trace.append(pc);covered.add(pc)
    def store(vm,_kind,at,size,value,_):
        if STATE<=at<STATE+36:
            stores.append(dict(pc=vm.reg_read(R.UC_X86_REG_EIP),address=at,bytes=size,
                before=bytes(vm.mem_read(at,size)).hex(),after=(value&((1<<(size*8))-1)).to_bytes(size,'little').hex()))
    g.hook_add(UC_HOOK_CODE,instruction);g.hook_add(UC_HOOK_MEM_WRITE,store)
    patterns=[(-1,-1),(-2,0),(0,-2),(0,0),(1,29),(2,59),(3,0),(0,32767),(2,-32768),(32767,9)]
    deltas=[1/30,1/60,0.,-1/30,.5,2.]
    timings=[(bits(1.),bits(.5)),(bits(0.),bits(0.)),(bits(1.),0x7fc12345)]
    rows=[];packets=[]
    for active in (False,True):
        for pattern,(title,ticks) in enumerate(patterns):
            for index,delta in enumerate(deltas):
                before=bytearray(range(36));struct.pack_into('<f',before,0,(-.25,.5,1.25)[index%3])
                before[4]=int(active)
                for slot in range(4):
                    selected=patterns[(pattern+slot)%len(patterns)]
                    struct.pack_into('<hh',before,8+slot*4,*selected)
                struct.pack_into('<If',before,24,42 if index%2 else 0,(-.5,.01,1.)[index%3])
                count=0 if pattern==9 else 3
                g.mem_write(STATE,bytes(before));put(0x51df50,STATE);put(0x51df54,CLOCK)
                put(CLOCK+4,bits(delta));put(0x4ecc20,STATE+0x4000)
                put(STATE+0x41f0,count);put(STATE+0x41f4,STATE+0x5000)
                for n,(hold,fade) in enumerate(timings):
                    put(STATE+0x501c+n*36,hold);put(STATE+0x5020+n*36,fade)
                initial=[0x11223344,0x22334455,0x33445566,0x44556677,STACK,0x55667788,0x66778899,0x778899aa]
                for register,value in zip(GP,initial):g.reg_write(register,value)
                for n in range(8):g.reg_write(R.UC_X86_REG_XMM0+n,int.from_bytes(struct.pack('<4I',*[0x3f000000+n*4+j for j in range(4)]),'little'))
                for reg,value in ((R.UC_X86_REG_EFLAGS,0x202),(R.UC_X86_REG_FPCW,0x27f),
                    (R.UC_X86_REG_FPSW,0),(R.UC_X86_REG_FPTAG,0xffff),(R.UC_X86_REG_MXCSR,0x1f80)):g.reg_write(reg,value)
                put(STACK,STOP);trace.clear();stores.clear();g.emu_start(0x13f160,STOP,count=10000)
                assert g.reg_read(R.UC_X86_REG_EIP)==STOP
                row=dict(entry=0x13f160,before=before.hex(),after=bytes(g.mem_read(STATE,36)).hex(),
                    delta_bits=bits(delta),title_count=count,timings=timings,initial_registers=initial,
                    registers=[g.reg_read(r) for r in GP],eflags=g.reg_read(R.UC_X86_REG_EFLAGS),
                    xmm=[g.reg_read(R.UC_X86_REG_XMM0+n).to_bytes(16,'little').hex() for n in range(8)],
                    fpu_status=g.reg_read(R.UC_X86_REG_FPSW),trace=list(trace),stores=list(stores))
                rows.append(row)
                packets.append(f'2 0 0 30 {count:x} {bits(delta):x} {before.hex()} '+
                    ' '.join(f'{v:x}' for pair in timings for v in pair))
    root=Path(__file__).resolve().parents[1];harness=root/'tools/original_workshop_titles_host_x86.cpp'
    executable=scratch/'original-title-cpu'
    compile_command=['g++','-m32','-msse2','-O2','-fomit-frame-pointer','-fno-pie','-no-pie',str(harness),'-o',str(executable)]
    subprocess.run(compile_command,check=True)
    result=subprocess.run([str(executable),str(NEW_DEFAULT)],input='\n'.join(packets)+'\n',text=True,capture_output=True,check=True)
    lines=result.stdout.splitlines();assert len(lines)==len(rows)
    for row,line in zip(rows,lines):
        values=[int(v,16) for v in line.split()];assert len(values)==53
        assert values[:8]==row['registers'] and values[8]==row['eflags'] and values[9]==row['fpu_status']
        assert b''.join(v.to_bytes(4,'little') for v in values[10:42])==b''.join(bytes.fromhex(v) for v in row['xmm'])
        assert b''.join(v.to_bytes(4,'little') for v in values[42:51]).hex()==row['after']
        assert values[51:]==[0,0]
    report=dict(scope=__doc__,xbe_sha256=NEW_PIN,cases=rows,instruction_patches=[],intercepted_calls=[],
        image_inputs=[dict(address=0x46a0d0,bytes=image.read(0x46a0d0,4).hex())],
        executed_instructions=len(covered),body_instructions=[[pc,bytes(code.at(pc).bytes).hex()] for pc in sorted(body.code)],
        original_call_site=dict(pc=0x13ab71,target=0x13f160,bytes=bytes(code.at(0x13ab71).bytes).hex()),
        host_cpu_cases=len(rows),host_harness_sha256=hashlib.sha256(harness.read_bytes()).hexdigest(),
        host_output_sha256=hashlib.sha256(result.stdout.encode()).hexdigest(),compile_command=compile_command,
        source_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest())
    raw=(json.dumps(report,indent=2)+'\n').encode();assert len(raw)<=1024*1024
    a.output.write_bytes(raw);print(json.dumps(dict(cases=len(rows),host_cpu_cases=len(rows),instructions=len(covered),bytes=len(raw))))


if __name__=='__main__':main()
