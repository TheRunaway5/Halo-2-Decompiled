#!/usr/bin/env python3
"""Record untouched TU1.5 CRT FCOS callback on the host's i386 CPU path."""
import argparse,hashlib,json,struct,subprocess,sys
from pathlib import Path
from oracle_smg_firing_1_1 import Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN

def main():
    a=argparse.ArgumentParser(description=__doc__);a.add_argument('--directory',required=True,type=Path);a.add_argument('--fixture',required=True,type=Path);a=a.parse_args()
    assert hashlib.sha256(NEW_DEFAULT.read_bytes()).hexdigest()==NEW_PIN
    image=Xbe(NEW_DEFAULT);assert image.read(0x382230,15).hex()=='dd05a0ab4600d9ffd91d70b15600c3'
    assert image.read(0x46aba0,8).hex()=='0000000000804640'
    a.directory.mkdir(parents=True,exist_ok=False);a.directory=a.directory.resolve()
    text=image.section('.text');(a.directory/'text.bin').write_bytes(image.section_bytes(text));cases=[]
    for precision in [0,0x200,0x300]:
        for rounding in [0,0x400,0x800,0xc00]:
            for depth in [0,2]:
                index=len(cases);folder=a.directory/f'case{index}';folder.mkdir();stack=0x60001000
                gp=[0x12345678,0x23456789,0x3456789a,0x456789ab,stack,0x56789abc,0x6789abcd,0x789abcde]
                live=[struct.pack('<QH',0xa000000000000000,0x3fff).hex(),struct.pack('<QH',0xa000000000000000,0xc000).hex()][:depth]
                before={'gp':gp,'xmm':[[0x11223344+n,0xaabbccdd,0x80000000,0x7fc00001]for n in range(8)],'eflags':0x8d7,'fs_base':0,'x87_values':live,'x87_status':0x4100|((5 if depth else 0)<<11),'x87_control_word':0x7f|precision|rounding,'mxcsr':0x1f80}
                scratch=bytearray(0x2000);struct.pack_into('<I',scratch,0x1000,0x60300000)
                regions=[]
                for address,data,name in [(text.va,None,'../text.bin'),(0x46aba0,image.read(0x46aba0,8),'operand.bin'),(0x56b170,bytes(4),'destination.bin'),(0x60000000,scratch,'stack.bin')]:
                    if data is not None:(folder/name).write_bytes(data)
                    regions.append({'address':address,'size':len(data)if data is not None else len(image.section_bytes(text)),'before_file':name})
                record={'entry':0x382230,'reference':'English TU1.5 only','boundary':'Original CRT callback, authored operand45.0, declared FP controls/live registers','before':before,'after':before,'regions':regions,'return':0x60300000}
                (folder/'capture.json').write_text(json.dumps(record,indent=2)+'\n')
                output=a.directory/f'original{index}'
                result=subprocess.run([sys.executable,str(Path(__file__).with_name('replay_live_phantom_hardware_tu15.py')),'--capture',str(folder),'--output',str(output),'--return-breakpoints'],capture_output=True,text=True)
                assert result.returncode in (0,1),(result.stdout,result.stderr)
                assert (output/'report.json').exists(),(result.stdout,result.stderr)
                report=json.loads((output/'report.json').read_text());raw=(output/'output.bin').read_bytes();offset=276;small=[]
                for region in sorted(regions,key=lambda r:r['address']):
                    after=raw[offset:offset+region['size']];offset+=region['size']
                    if region['address']!=text.va:small.append({'address':region['address'],'before':(folder/region['before_file']).read_bytes().hex(),'after':after.hex()})
                assert offset==len(raw)
                cases.append({'entry':0x382230,'input':before,'regions':small,'original':report['original_abi'],'eflags_mask':report['eflags_comparison_mask'],'original_instructions':report['original_instructions']})
                print(index,'CW',hex(before['x87_control_word']),'depth',depth,'status',hex(report['original_abi']['x87_status']),flush=True)
    fixture={'reference':'English TU1.5 only','original_sha256':NEW_PIN,'entry':0x382230,'complete_source_bytes':'dd05a0ab4600d9ffd91d70b15600c3','authored_operand_bytes':'0000000000804640','cases':cases,'boundary':'24 declared ABI/control cases of unchanged original i386 CPU code; not Xbox hardware or whole gameplay proof'}
    a.fixture.parent.mkdir(parents=True,exist_ok=True);a.fixture.write_text(json.dumps(fixture,indent=2)+'\n')
    print('PASS24 original CRT FCOS callback captures')
if __name__=='__main__':main()
