#!/usr/bin/env python3
"""Check complete original2B9450 on the host's real i386 CPU execution path.

Reuse the libc-free input/capture harness, with the original code image and
every executed callee unchanged. Raw images/objects/ELFs remain owned scratch.
This is component routing evidence, not an Xbox or audible-device comparison.
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
from qualify_loop_combined_publication_hardware_tu15 import ASM


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--input',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args()
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    if args.output.exists():parser.error('Use a new reference output path')
    reference=json.loads(args.input.read_text())
    assert reference['original_xbe_sha256']==NEW_PIN and reference['entry']==0x2b9450
    assert len(reference['cases'])==384 and not reference['source_instruction_patches'] and not reference['substituted_calls']
    image=Xbe(NEW_DEFAULT);assert hashlib.sha256(image.data).hexdigest()==NEW_PIN
    original=bytearray(0x800000)
    for section in image.sections:
        raw=image.section_bytes(section);assert 0x10000<=section.va and section.va+len(raw)<=0x810000
        original[section.va-0x10000:section.va-0x10000+len(raw)]=raw
    (scratch/'original.bin').write_bytes(original)
    text=next(s for s in image.sections if s.name=='.text')
    stack,output,stop=reference['stack'],reference['output_address'],0x103f000
    assert stack==0x103e000
    assembly=ASM.replace('entry: .long 0x2216d0','entry: .long 0x2b9450')
    assembly=assembly.replace('ORIGINAL_IMAGE',str(scratch/'original.bin')).replace('OUTPUT_ADDRESS',str(output)).replace('CALLER_ARGUMENTS',str(stack+4))
    (scratch/'reference.s').write_text(assembly)
    (scratch/'link.ld').write_text('ENTRY(_start)\nSECTIONS {\n.original 0x10000 : { *(.original) }\n'
        '.banks 0x1000000 : { *(.banks) }\n.return 0x103f000 : { *(.return) }\n.bridge 0x70000000 : { *(.bridge) }\n}\n')
    commands=[['as','--32',str(scratch/'reference.s'),'-o',str(scratch/'reference.o')],
        ['ld','-melf_i386','-T',str(scratch/'link.ld'),str(scratch/'reference.o'),'-o',str(scratch/'original-hardware')],
        ['objcopy','--dump-section','.original='+str(scratch/'verified.bin'),str(scratch/'original-hardware')]]
    for command in commands:subprocess.run(command,check=True,capture_output=True)
    assert (scratch/'verified.bin').read_bytes()==original
    def patches(writes):
        stream=bytearray(struct.pack('<I',len(writes)))
        for address,hexadecimal in writes:
            raw=bytes.fromhex(hexadecimal)
            assert address+len(raw)<=text.va or address>=text.va+text.vsize
            assert (0x1000000<=address and address+len(raw)<=stop) or any(
                p<=address and address+len(raw)<=p+len(bytes.fromhex(h)) for p,h in reference['memory'])
            stream+=struct.pack('<II',address,len(raw))+raw
        return stream
    payload=patches(reference['memory'])
    for row in reference['cases']:
        payload+=patches(row['memory'])
        # Only class1 reads the plugin's class-send gain. Make that actual
        # page inaccessible for all other categories during original RET.
        guards=[] if row['kind']==1 else [reference['plugin']]
        row['unmapped_guard_pages']=guards
        payload+=struct.pack('<I',len(guards))+b''.join(struct.pack('<I',p) for p in guards)
        environment=bytearray(108)
        struct.pack_into('<H',environment,0,row['control']);struct.pack_into('<H',environment,8,0xffff)
        payload+=struct.pack('<10I',*row['gp'],2,row['mxcsr'])+environment
        payload+=b''.join(struct.pack('<4I',*x) for x in row['xmm'])
    payload+=struct.pack('<I',0)
    completed=subprocess.run([str(scratch/'original-hardware')],input=payload,capture_output=True,timeout=60)
    assert completed.returncode==0,(completed.returncode,completed.stderr[:512])
    assert len(completed.stdout)==372*len(reference['cases'])
    differences=0
    for index,row in enumerate(reference['cases']):
        result=completed.stdout[index*372:(index+1)*372]
        assert list(struct.unpack_from('<8I',result))==row['expected_gp'],('GP',index)
        assert [list(struct.unpack_from('<4I',result,148+n*16)) for n in range(8)]==row['expected_xmm'],('SIMD',index)
        assert result[276:356].hex()==row['output'] and result[356:372].hex()==row['caller'],('backing',index)
        control,status=struct.unpack_from('<H',result,40)[0],struct.unpack_from('<H',result,44)[0]
        assert struct.unpack_from('<H',result,48)[0]==0xffff,('live x87 tag',index)
        mxcsr=struct.unpack_from('<I',result,36)[0]
        differences+=(status,mxcsr)!=(row['expected_status'],row['expected_mxcsr'])
        row['unicorn_status']=row['expected_status'];row['unicorn_mxcsr']=row['expected_mxcsr']
        row.update(expected_control=control,expected_status=status,expected_mxcsr=mxcsr)
    reference.update(reference_backend='unchanged original Linux i386 CPU; Unicorn ordered-write provenance',
        hardware_tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        hardware_harness_tool_sha256=hashlib.sha256(Path(__file__).with_name('qualify_loop_combined_publication_hardware_tu15.py').read_bytes()).hexdigest(),
        unicorn_fixture_sha256=hashlib.sha256(args.input.read_bytes()).hexdigest(),
        physical_fp_differences_from_unicorn=differences,physical_original_image_sha256=hashlib.sha256(original).hexdigest(),
        physical_reference_elf_sha256=hashlib.sha256((scratch/'original-hardware').read_bytes()).hexdigest(),
        hardware_commands=commands,hardware_final_caller_backing_equal=True,
        hardware_guarded_plugin_cases=sum(bool(row['unmapped_guard_pages']) for row in reference['cases']),
        original_instructions_patched=[],retained_large_artifacts=[])
    raw=json.dumps(reference,separators=(',',':'))+'\n';assert len(raw.encode())<1024*1024
    args.output.parent.mkdir(parents=True,exist_ok=True);args.output.write_text(raw)
    print(json.dumps(dict(cases=len(reference['cases']),physical_fp_differences_from_unicorn=differences,
        all_gp_simd_output_and_caller_backing_equal=True,bytes=len(raw.encode()))),flush=True)


if __name__=='__main__':main()
