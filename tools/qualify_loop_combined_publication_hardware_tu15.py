#!/usr/bin/env python3
"""Qualify complete2216D0 with unchanged original Linux i386 CPU execution.

The libc-free harness loads declared component inputs before each invocation
and observes only its completed RET. Original instructions/calls are never
patched. This reference is not a game runtime, Xbox hardware or audio device.
All rebuildable images/ELFs/objects live inside tools/run_test.py scratch.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import struct
import subprocess

from oracle_smg_firing_1_1 import Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN

ASM=r'''
.code32
.section .bridge,"awx"
.globl _start
_start:
 mov $harness_stack_end,%esp
 call patches
next_case:
 mov $harness_stack_end,%esp
 call patches
 cmpl $0,patch_count
 je finished
 mov $guard_count,%ecx
 mov $4,%edx
 call read_exact
 mov $guard_pages,%ecx
 mov guard_count,%edx
 shl $2,%edx
 call read_exact
 mov $input,%ecx
 mov $276,%edx
 call read_exact
 movl $0,guard_mode
 call protect_guards
 frstor input+40
 ldmxcsr input+36
 movdqu input+148,%xmm0
 movdqu input+164,%xmm1
 movdqu input+180,%xmm2
 movdqu input+196,%xmm3
 movdqu input+212,%xmm4
 movdqu input+228,%xmm5
 movdqu input+244,%xmm6
 movdqu input+260,%xmm7
 push input+32
 popfl
 mov input,%eax
 mov input+4,%ecx
 mov input+8,%edx
 mov input+12,%ebx
 mov input+20,%ebp
 mov input+24,%esi
 mov input+28,%edi
 mov input+16,%esp
 jmp *entry
patches:
 mov $patch_count,%ecx
 mov $4,%edx
 call read_exact
 mov patch_count,%esi
patch_loop:
 test %esi,%esi
 jz patches_done
 mov $patch_header,%ecx
 mov $8,%edx
 call read_exact
 mov patch_header,%ecx
 mov patch_header+4,%edx
 call read_exact
 dec %esi
 jmp patch_loop
patches_done:
 ret
protect_guards:
 xor %esi,%esi
guard_loop:
 cmp guard_count,%esi
 je guards_done
 mov guard_pages(,%esi,4),%ebx
 mov $4096,%ecx
 mov guard_mode,%edx
 mov $125,%eax
 int $0x80
 test %eax,%eax
 jnz failure
 inc %esi
 jmp guard_loop
guards_done:
 ret
read_exact:
 test %edx,%edx
 jz io_done
 mov $3,%eax
 xor %ebx,%ebx
 int $0x80
 cmp $-4,%eax
 je read_exact
 test %eax,%eax
 jle failure
 add %eax,%ecx
 sub %eax,%edx
 jmp read_exact
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
 jmp terminate
finished:
 xor %ebx,%ebx
terminate:
 mov $1,%eax
 int $0x80
.balign 16
entry: .long 0x2216d0
patch_count: .long 0
patch_header: .space 8
guard_count: .long 0
guard_mode: .long 0
guard_pages: .space 64
.balign 16
input: .space 276
result: .space 276
harness_stack: .space 65536
harness_stack_end:
.section .return,"awx"
capture:
 mov %eax,result
 mov %ecx,result+4
 mov %edx,result+8
 mov %ebx,result+12
 mov %esp,result+16
 mov %ebp,result+20
 mov %esi,result+24
 mov %edi,result+28
 mov $harness_stack_end,%esp
 pushfl
 pop result+32
 stmxcsr result+36
 fnsave result+40
 movdqu %xmm0,result+148
 movdqu %xmm1,result+164
 movdqu %xmm2,result+180
 movdqu %xmm3,result+196
 movdqu %xmm4,result+212
 movdqu %xmm5,result+228
 movdqu %xmm6,result+244
 movdqu %xmm7,result+260
 movl $7,guard_mode
 call protect_guards
 mov $result,%ecx
 mov $276,%edx
 call write_exact
 mov $OUTPUT_ADDRESS,%ecx
 mov $80,%edx
 call write_exact
 mov $CALLER_ARGUMENTS,%ecx
 mov $16,%edx
 call write_exact
 jmp next_case
.section .banks,"aw",@nobits
.space 0x3f000
.section .original,"awx"
.incbin "ORIGINAL_IMAGE"
'''


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--input',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args()
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    if args.output.exists():parser.error('Use a new report path; original evidence is never overwritten')
    reference=json.loads(args.input.read_text())
    assert reference['original_xbe_sha256']==NEW_PIN and reference['entry']==0x2216d0
    assert len(reference['cases'])==320 and not reference['source_instruction_patches'] and not reference['substituted_calls']
    image=Xbe(NEW_DEFAULT);assert hashlib.sha256(image.data).hexdigest()==NEW_PIN
    original=bytearray(0x800000)
    for section in image.sections:
        data=image.section_bytes(section)
        assert 0x10000<=section.va and section.va+len(data)<=0x810000
        original[section.va-0x10000:section.va-0x10000+len(data)]=data
    source=scratch/'original-image.bin';source.write_bytes(original)
    text=next(section for section in image.sections if section.name=='.text')
    stack=reference['stack'];output=reference['output_address'];stop=0x103f000
    assert stack==0x103e000
    assembly=ASM.replace('ORIGINAL_IMAGE',str(source)).replace('OUTPUT_ADDRESS',str(output)).replace('CALLER_ARGUMENTS',str(stack+4))
    (scratch/'reference.s').write_text(assembly)
    (scratch/'link.ld').write_text('ENTRY(_start)\nSECTIONS {\n.original 0x10000 : { *(.original) }\n'
        '.banks 0x1000000 : { *(.banks) }\n.return 0x103f000 : { *(.return) }\n.bridge 0x70000000 : { *(.bridge) }\n}\n')
    commands=[['as','--32',str(scratch/'reference.s'),'-o',str(scratch/'reference.o')],
        ['ld','-melf_i386','-T',str(scratch/'link.ld'),str(scratch/'reference.o'),'-o',str(scratch/'original-hardware')],
        ['objcopy','--dump-section','.original='+str(scratch/'verified-image.bin'),str(scratch/'original-hardware')]]
    for command in commands:subprocess.run(command,check=True,capture_output=True)
    assert (scratch/'verified-image.bin').read_bytes()==original

    def patches(writes):
        stream=bytearray(struct.pack('<I',len(writes)))
        for address,hexadecimal in writes:
            raw=bytes.fromhex(hexadecimal)
            assert address+len(raw)<=text.va or address>=text.va+text.vsize,'Input overlaps original instructions'
            assert (0x1000000<=address and address+len(raw)<=stop) or any(
                p<=address and address+len(raw)<=p+len(bytes.fromhex(h)) for p,h in reference['memory'])
            stream+=struct.pack('<II',address,len(raw))+raw
        return stream

    payload=patches(reference['memory'])
    for row in reference['cases']:
        payload+=patches(row['memory'])
        # The original must reject these parents before touching a sound
        # bank or output. Whole real pages become inaccessible for the call.
        guards=[]
        if not (row['expected_gp'][0]&255):
            guards=[0x1003000+n*0x1000 for n in range(11)]
            datum=int.from_bytes(bytes.fromhex(row['memory'][0][1]),'little')
            if datum==0xffffffff:guards+=[0x1001000,0x1002000]
            elif (datum&65535)>=4:guards+=[0x1002000]
        row['unmapped_guard_pages']=guards
        payload+=struct.pack('<I',len(guards))+b''.join(struct.pack('<I',p) for p in guards)
        environment=bytearray(108)
        struct.pack_into('<H',environment,0,row['control']);struct.pack_into('<H',environment,8,0xffff)
        payload+=struct.pack('<10I',*row['gp'],2,row['mxcsr'])+environment
        payload+=b''.join(struct.pack('<4I',*x) for x in row['xmm'])
    payload+=struct.pack('<I',0)
    completed=subprocess.run([str(scratch/'original-hardware')],input=payload,capture_output=True,timeout=60)
    assert completed.returncode==0,(completed.returncode,completed.stderr[:512])
    assert len(completed.stdout)==372*len(reference['cases']),len(completed.stdout)
    differences=0
    for index,row in enumerate(reference['cases']):
        result=completed.stdout[index*372:(index+1)*372]
        gp=list(struct.unpack_from('<8I',result))
        xmm=[list(struct.unpack_from('<4I',result,148+n*16)) for n in range(8)]
        assert gp==row['expected_gp'] and xmm==row['expected_xmm'],('GP/SIMD',index)
        assert result[276:356].hex()==row['output'],('output',index)
        arguments={stack+4:row['memory'][0][1],stack+8:struct.pack('<I',0x76540000).hex(),
            stack+12:struct.pack('<I',0xabc10203).hex(),stack+16:struct.pack('<I',output).hex()}
        for pc,address,size,value in row['stores']:
            if address in arguments:arguments[address]=int(value).to_bytes(size,'little').hex()
        assert result[356:372].hex()==''.join(arguments[p] for p in sorted(arguments)),('caller backing',index)
        control=struct.unpack_from('<H',result,40)[0];status=struct.unpack_from('<H',result,44)[0]
        assert struct.unpack_from('<H',result,48)[0]==0xffff,('live x87 tag',index)
        mxcsr=struct.unpack_from('<I',result,36)[0]
        differences+=(status,mxcsr)!=(row['expected_status'],row['expected_mxcsr'])
        row['unicorn_status']=row['expected_status'];row['unicorn_mxcsr']=row['expected_mxcsr']
        row['expected_status']=status;row['expected_mxcsr']=mxcsr;row['expected_control']=control
    reference.update(reference_backend='unchanged original Linux i386 CPU; Unicorn ordered-write provenance',
        hardware_tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        unicorn_fixture_sha256=hashlib.sha256(args.input.read_bytes()).hexdigest(),
        physical_fp_differences_from_unicorn=differences,
        physical_original_image_sha256=hashlib.sha256(original).hexdigest(),
        physical_reference_elf_sha256=hashlib.sha256((scratch/'original-hardware').read_bytes()).hexdigest(),
        hardware_commands=commands,hardware_final_caller_backing_equal=True,
        hardware_unmapped_rejection_cases=sum(bool(row['unmapped_guard_pages']) for row in reference['cases']),
        original_instructions_patched=[],retained_large_artifacts=[])
    raw=json.dumps(reference,separators=(',',':'))+'\n';assert len(raw.encode())<1024*1024
    args.output.parent.mkdir(parents=True,exist_ok=True);args.output.write_text(raw)
    print(json.dumps(dict(cases=len(reference['cases']),physical_fp_differences_from_unicorn=differences,
        all_gp_simd_output_and_caller_backing_equal=True,bytes=len(raw.encode()),
        sha256=hashlib.sha256(raw.encode()).hexdigest())),flush=True)


if __name__=='__main__':main()
