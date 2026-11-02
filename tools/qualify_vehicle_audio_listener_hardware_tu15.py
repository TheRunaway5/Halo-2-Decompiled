#!/usr/bin/env python3
"""Compare the seat-listener component with unchanged XBE code on real i386.

A child-only CPU debug-register breakpoint observes12A308 before the next
original owner. ptrace redirects that stopped PC to the external capture
bridge, without changing original instructions or supplying callee results.
Raw images, ELF, object and captures remain in run_test.py owned storage.
"""
import argparse
import ctypes
import hashlib
import json
import os
from pathlib import Path
import signal
import struct
import subprocess
from oracle_smg_firing_1_1 import Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN
from qualify_loop_combined_publication_hardware_tu15 import ASM


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--input',type=Path,required=True);parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args();scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    if args.output.exists():parser.error('Preserve existing reference reports')
    reference=json.loads(args.input.read_text());assert reference['original_xbe_sha256']==NEW_PIN
    assert reference['entry']=='0012a05c' and reference['stop']=='0012a308'
    assert reference['every_executed_site_has_rooted_function_provenance']
    assert not reference['original_instruction_patches'] and not reference['substituted_calls']
    image=Xbe(NEW_DEFAULT);assert hashlib.sha256(image.data).hexdigest()==NEW_PIN
    original=bytearray(0x800000)
    for section in image.sections:
        raw=image.section_bytes(section);original[section.va-0x10000:section.va-0x10000+len(raw)]=raw
    (scratch/'original.bin').write_bytes(original)
    camera,unit,seat,stack=0x1001000,0x1002000,0x1003000,0x1005000
    assembly=ASM.replace('entry: .long 0x2216d0','entry: .long 0x12a05c')
    assembly=assembly.replace('ORIGINAL_IMAGE',str(scratch/'original.bin')).replace('OUTPUT_ADDRESS',str(stack+0x10)).replace('CALLER_ARGUMENTS',str(stack+0x6c))
    (scratch/'reference.s').write_text(assembly)
    (scratch/'link.ld').write_text('ENTRY(_start)\nSECTIONS {\n.original 0x10000 : { *(.original) }\n'
        '.banks 0x1000000 : { *(.banks) }\n.return 0x103f000 : { *(.return) }\n.bridge 0x70000000 : { *(.bridge) }\n}\n')
    commands=[['as','--32',str(scratch/'reference.s'),'-o',str(scratch/'reference.o')],
        ['ld','-melf_i386','-T',str(scratch/'link.ld'),str(scratch/'reference.o'),'-o',str(scratch/'original-hardware')],
        ['objcopy','--dump-section','.original='+str(scratch/'verified.bin'),str(scratch/'original-hardware')]]
    for command in commands:subprocess.run(command,check=True,capture_output=True)
    assert (scratch/'verified.bin').read_bytes()==original
    def patches(writes):
        data=bytearray(struct.pack('<I',len(writes)))
        for address,raw in writes:
            assert 0x1000000<=address and address+len(raw)<0x103f000
            data+=struct.pack('<II',address,len(raw))+raw
        return data
    def vector(values):return struct.pack('<'+'I'*len(values),*values)
    payload=patches([(camera,bytes(0x100))])
    for row in reference['cases']:
        payload+=patches([(camera,vector(row['position'])),(camera+0x20,vector(row['forward'])),
            (camera+0x2c,vector(row['up'])),(unit+0x180,vector(row['unit_aim'])),
            (seat+0x40,vector([row['factor']])),(stack,bytes(0x200)),
            (stack+0x84,vector(row['unit_point'])),(stack+0x7c,vector([unit])),(stack+0x34,vector([0xcafe0000]))])
        payload+=struct.pack('<I',0)
        environment=bytearray(108);rounding=row['rounding']
        struct.pack_into('<H',environment,0,0x027f|(rounding<<10));struct.pack_into('<H',environment,8,0xffff)
        gp=[0,0,0,seat,stack,0,0,camera]
        payload+=struct.pack('<10I',*gp,2,0x1f80|(rounding<<13))+environment+bytes(128)
    payload+=struct.pack('<I',0);(scratch/'inputs.bin').write_bytes(payload)
    libc=ctypes.CDLL(None,use_errno=True);libc.ptrace.restype=ctypes.c_long
    libc.ptrace.argtypes=[ctypes.c_uint,ctypes.c_uint,ctypes.c_void_p,ctypes.c_void_p]
    def trace(request,pid,address=0,data=0):
        result=libc.ptrace(request,pid,ctypes.c_void_p(address),ctypes.c_void_p(data))
        if result==-1:raise OSError(ctypes.get_errno(),'child ptrace failed')
    def child():trace(0,0)
    class Registers(ctypes.Structure):
        _fields_=[(name,ctypes.c_ulonglong) for name in ('r15','r14','r13','r12','rbp','rbx','r11','r10','r9','r8',
            'rax','rcx','rdx','rsi','rdi','orig_rax','rip','cs','eflags','rsp','ss','fs_base','gs_base','ds','es','fs','gs')]
    stops=0
    with (scratch/'inputs.bin').open('rb') as incoming,(scratch/'capture.bin').open('wb') as outgoing:
        process=subprocess.Popen([str(scratch/'original-hardware')],stdin=incoming,stdout=outgoing,stderr=subprocess.PIPE,preexec_fn=child)
        try:
            pid,status=os.waitpid(process.pid,0);assert os.WIFSTOPPED(status) and os.WSTOPSIG(status)==signal.SIGTRAP
            # Linux x86_64 struct user.u_debugreg offsets, including compat children.
            trace(6,pid,848,0x12a308);trace(6,pid,904,1);trace(7,pid)
            while True:
                _,status=os.waitpid(pid,0)
                if os.WIFEXITED(status):assert os.WEXITSTATUS(status)==0;process.returncode=0;break
                assert os.WIFSTOPPED(status) and os.WSTOPSIG(status)==signal.SIGTRAP,status
                registers=Registers();trace(12,pid,0,ctypes.addressof(registers));assert registers.rip==0x12a308,hex(registers.rip)
                registers.rip=0x103f000;trace(13,pid,0,ctypes.addressof(registers));stops+=1;trace(7,pid)
        finally:
            if process.returncode is None:process.kill();process.wait()
    raw=(scratch/'capture.bin').read_bytes();assert len(raw)==372*len(reference['cases'])
    for index,row in enumerate(reference['cases']):
        result=raw[index*372:(index+1)*372]
        assert list(struct.unpack_from('<9I',result,276))==row['result'],('listener',index)
        assert list(struct.unpack_from('<4I',result,356))==row['rotation'],('rotation',index)
    assert stops==len(reference['cases'])
    result=dict(scope=__doc__,original_xbe_sha256=NEW_PIN,cases=stops,all_listener_and_rotation_bits_equal=True,
        reference_sha256=hashlib.sha256(args.input.read_bytes()).hexdigest(),
        hardware_tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        physical_original_image_sha256=hashlib.sha256(original).hexdigest(),
        physical_elf_sha256=hashlib.sha256((scratch/'original-hardware').read_bytes()).hexdigest(),
        original_instructions_patched=[],substituted_calls=[],retained_large_artifacts=[],commands=commands,
        physical_xbox_or_audible_device_comparison=False)
    args.output.parent.mkdir(parents=True,exist_ok=True);args.output.write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps(dict(physical_i386_cases=stops,all_listener_and_rotation_bits_equal=True)))


if __name__=='__main__':main()
