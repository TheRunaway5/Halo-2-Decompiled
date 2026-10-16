#!/usr/bin/env python3
"""Audit unchanged TU1.5 fixed-shadow uniform prefixes on retained live inputs.

The native C3960/C3AB0 projection is an explicitly declared completed caller
input, independently compared by the projection differential. No native uniform
output, GPU return, instruction patch or replacement game callee is supplied.
4A1D0 stops at3A540 before its first upload;48ED0 stops before its401410 tail,
or fails at the first actual SDK dependency. This is CPU math, not GPU proof.
Captured Root.uniforms and its feature field, when present, are comparison
outputs only: they are inspected after both original prefixes have executed.
"""
import argparse
import hashlib
import json
import struct
import subprocess
from pathlib import Path

from unicorn import Uc,UcError,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_INVALID
from unicorn import x86_const as R
from oracle_smg_firing_1_1 import Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from differential_physics_native_frame_tu15_20261012 import regions,segment

ROOT=Path(__file__).resolve().parents[1]
REGS=[R.UC_X86_REG_EAX,R.UC_X86_REG_ECX,R.UC_X86_REG_EDX,R.UC_X86_REG_EBX,
      R.UC_X86_REG_ESP,R.UC_X86_REG_EBP,R.UC_X86_REG_ESI,R.UC_X86_REG_EDI]
PROBE=r'''#include "halo2/source_shadow.hpp"
#include <array>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <type_traits>
#include <immintrin.h>
int main(){
 std::array<std::uint32_t,32> input;
 if(!std::cin.read(reinterpret_cast<char*>(input.data()),sizeof(input)))return 2;
 static_assert(sizeof(halo2::SourceShadowFrame)==29*4);
 static_assert(std::is_trivially_copyable_v<halo2::SourceShadowFrame>);
 halo2::SourceShadowFrame frame;std::memcpy(&frame,input.data(),sizeof(frame));
 const auto control=std::uint16_t(input[30]);
 asm volatile("fldcw %0"::"m"(control));_mm_setcsr(input[31]);
 const auto value=halo2::source_shadow_uniforms(frame,input[29]!=0);
 std::cout.write(reinterpret_cast<const char*>(value.projection.data()),sizeof(value.projection));
 std::cout.write(reinterpret_cast<const char*>(value.constants.data()),sizeof(value.constants));
 return std::cout?0:3;
}
'''

def sha(path):
    with Path(path).open('rb')as file:return hashlib.file_digest(file,'sha256').hexdigest()

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--capture',type=Path,required=True)
    p.add_argument('--owner',type=int,required=True)
    p.add_argument('--output',type=Path,required=True)
    p.add_argument('--native-source',type=Path,default=ROOT)
    p.add_argument('--skip-native-probe',action='store_true')
    a=p.parse_args()
    assert not a.output.exists() and sha(NEW_DEFAULT)==NEW_PIN
    a.output.mkdir(parents=True)
    (a.output/'tool.py').write_bytes(Path(__file__).read_bytes())
    case=a.capture/'shadow-admission'/str(a.owner)/str(0x40e80)
    call=json.loads((case/'call.json').read_text())
    abi=json.loads((case/'before-abi.json').read_text())
    assert call['authority']=='OG Xbox TU1.5' and call['entry']==0x40e80 and abi['x87_depth']==0
    row=next(r for r in json.loads((a.capture/'report.json').read_text())['shadow_root_frames']
             if r['owner']==a.owner and r['tick']==call['tick'] and r['view']==call['view'])
    assert row['view_admitted'] and len(row['projection_words'])==31 and len(row['frame_words'])==29
    image=Xbe(NEW_DEFAULT);text_section=next(s for s in image.sections if s.name=='.text')
    guest=Uc(UC_ARCH_X86,UC_MODE_32);mapped=set()
    def mount(address,data):
        pages=[n for n in range(address&~4095,(address+len(data)+4095)&~4095,4096)if n not in mapped]
        begin=0
        while begin<len(pages):
            end=begin+1
            while end<len(pages)and pages[end]==pages[end-1]+4096:end+=1
            guest.mem_map(pages[begin],(end-begin)*4096);begin=end
        mapped.update(pages);guest.mem_write(address,bytes(data))
    for address,data in regions(case/'before.bin'):mount(address,data)
    def words(address,count):return list(struct.unpack(f'<{count}I',guest.mem_read(address,count*4)))
    def put(address,values):guest.mem_write(address,struct.pack(f'<{len(values)}I',*values))
    sp=abi['gp'][4];projection=sp+0x100
    projection_input=struct.pack('<31I',*row['projection_words'])
    guest.mem_read(projection,len(projection_input)) # Existing caller-owned source stack only.
    guest.mem_write(projection,projection_input)
    assert words(0x4c6834,1)==[0x14]
    for address,indices in ((0x48c530,(0,1,2)),(0x48c558,(3,4,5)),(0x48c564,(6,7,8)),
                            (0x48c5d8,(23,)),(0x48c5e4,(24,)),(0x48c5e8,(25,))):
        assert words(address,len(indices))==[row['frame_words'][n]for n in indices]
    feature=bool(words(0x4ce2a0,1)[0]&(1<<10))
    before_projection=words(0x49219c,36)
    gdt=0x7fff0000;assert gdt not in mapped;mount(gdt,bytes(4096))
    guest.mem_write(gdt+8,segment(0,0x9b)+segment(0,0x93)+segment(abi['fs_base'],0x93))
    guest.reg_write(R.UC_X86_REG_GDTR,(0,gdt,0xfff,0))
    for r,v in ((R.UC_X86_REG_CS,8),(R.UC_X86_REG_DS,16),(R.UC_X86_REG_ES,16),
                (R.UC_X86_REG_SS,16),(R.UC_X86_REG_FS,24)):guest.reg_write(r,v)
    trace=[];faults=[];code_bytes={};sdk=[]
    def code(cpu,pc,size,_):
        raw=bytes(cpu.mem_read(pc,size))
        assert raw==image.read(pc,size),hex(pc)
        if not text_section.va<=pc<text_section.va+text_section.vsize:
            sdk.append(dict(pc=pc,gp=[cpu.reg_read(r)for r in REGS],
                            stack=words(cpu.reg_read(R.UC_X86_REG_ESP),8)))
            raise RuntimeError(f'Actual original SDK dependency {pc:08x}; no return supplied')
        trace.append(pc);code_bytes[pc]=raw.hex()
    def fault(cpu,access,address,size,value,_):
        faults.append(dict(pc=cpu.reg_read(R.UC_X86_REG_EIP),access=access,address=address,size=size));return False
    guest.hook_add(UC_HOOK_CODE,code);guest.hook_add(UC_HOOK_MEM_INVALID,fault)
    def state():
        return dict(pc=guest.reg_read(R.UC_X86_REG_EIP),gp=[guest.reg_read(r)for r in REGS],
                    eflags=guest.reg_read(R.UC_X86_REG_EFLAGS),x87_control=guest.reg_read(R.UC_X86_REG_FPCW),
                    x87_status=guest.reg_read(R.UC_X86_REG_FPSW),mxcsr=guest.reg_read(R.UC_X86_REG_MXCSR))
    def run(entry,stop,gp,arguments):
        for r,v in zip(REGS,abi['gp']):guest.reg_write(r,v)
        for n,value in gp.items():guest.reg_write(REGS[n],value)
        for n,lanes in enumerate(abi['xmm']):
            guest.reg_write(R.UC_X86_REG_XMM0+n,sum(v<<(32*k)for k,v in enumerate(lanes)))
        for r,v in ((R.UC_X86_REG_EFLAGS,abi['eflags']),(R.UC_X86_REG_FPCW,abi['x87_control']),
                    (R.UC_X86_REG_FPSW,abi['x87_status']),(R.UC_X86_REG_FPTAG,0xffff),
                    (R.UC_X86_REG_MXCSR,abi['mxcsr'])):guest.reg_write(r,v)
        put(sp+4,arguments);before=state();before['pc']=entry;first=len(trace);error=None
        try:
            guest.emu_start(entry,stop,count=1000000)
            assert guest.reg_read(R.UC_X86_REG_EIP)==stop,'Original uniform prefix did not reach boundary'
        except(UcError,RuntimeError,AssertionError)as e:error=str(e)
        path=trace[first:]
        (a.output/f'{entry:08x}-instructions.json').write_text(json.dumps(path)+'\n')
        return dict(entry=entry,boundary=stop,completed=error is None,error=error,
                    instructions=len(path),before_abi=before,after_abi=state(),arguments=arguments)
    projection_result=run(0x4a1d0,0x3a540,{0:0x48c530,1:projection,2:0xffffffff},
                          [0x48c5b8,0x3f800000,1,0x3f800000])
    projection_words=words(0x49219c,36)
    projection_result.update(bank=0x49219c,words=projection_words,
                             retained_before_words=before_projection,upload_register=31,upload_count_words=36)
    constants_result=None
    if projection_result['completed']:
        constants_result=run(0x48ed0,0x49492,{1:projection,3:0x48c530},[0x48c5b8])
        constants_result.update(bank=0x4d5fb8,words=words(0x4d5fb8,36),upload_register=40,upload_count_words=36)
        if constants_result['completed']:
            assert guest.reg_read(R.UC_X86_REG_ECX)==40 and guest.reg_read(R.UC_X86_REG_EDX)==0x4d5fb8
            assert words(guest.reg_read(R.UC_X86_REG_ESP)+4,1)==[36]
    captured=None
    captured_fields=('uniform_projection_words','uniform_constants_words','shadow_depth_feature')
    if any(field in row for field in captured_fields):
        captured=dict(fields=list(captured_fields),completed=False,passed=False,
                      used_as_original_input=False,compared_after_original_execution=True)
        missing=[field for field in captured_fields if field not in row]
        if missing:captured['error']='Incomplete captured uniform fields: '+','.join(missing)
        elif (len(row['uniform_projection_words'])!=24 or len(row['uniform_constants_words'])!=36 or
              not isinstance(row['shadow_depth_feature'],bool) or
              any(type(v)is not int or not 0<=v<=0xffffffff for v in
                  row['uniform_projection_words']+row['uniform_constants_words'])):
            captured['error']='Invalid captured60-word uniform/feature shape'
        else:
            actual=row['uniform_projection_words']+row['uniform_constants_words']
            captured.update(words=actual,shadow_depth_feature=row['shadow_depth_feature'],
                            original_input_feature=feature,feature_matches=row['shadow_depth_feature']==feature)
            if projection_result['completed'] and constants_result and constants_result['completed']:
                expected=projection_words[:24]+constants_result['words']
                differences=[dict(register=31+n//4 if n<24 else 40+(n-24)//4,lane=n%4,
                                  original=o,captured=v)for n,(o,v)in enumerate(zip(expected,actual))if o!=v]
                captured.update(completed=True,compared_words=60,differences=differences,
                                passed=not differences and captured['feature_matches'])
            else:captured['error']='Original uniform prefixes did not both complete'
    native=None
    if not a.skip_native_probe:
        directory=a.output/'native-probe';directory.mkdir()
        source=directory/'probe.cpp';source.write_text(PROBE)
        frame=directory/'input.bin';frame.write_bytes(struct.pack('<32I',*row['frame_words'],int(feature),abi['x87_control'],abi['mxcsr']))
        helper=a.native_source/'src/source_shadow.cpp';header=a.native_source/'include/halo2/source_shadow.hpp'
        pins={str(path.resolve()):sha(path)for path in (helper,header,source,frame)}
        binary=directory/'probe'
        command=['c++','-std=c++20','-O2','-g0','-ffunction-sections','-fdata-sections','-ffp-contract=off',
                 '-fno-fast-math','-msse2','-mfpmath=sse','-I'+str((a.native_source/'include').resolve()),
                 str(source.resolve()),str(helper.resolve()),'-Wl,--gc-sections','-o',str(binary.resolve())]
        built=subprocess.run(command,capture_output=True,text=True)
        (directory/'build.log').write_text(built.stdout+built.stderr)
        native=dict(command=command,files=pins,build_returncode=built.returncode,completed=False)
        if built.returncode==0:
            observed=subprocess.run([str(binary.resolve())],input=frame.read_bytes(),capture_output=True)
            native.update(returncode=observed.returncode,binary_sha256=sha(binary))
            (directory/'output.bin').write_bytes(observed.stdout);(directory/'stderr.txt').write_bytes(observed.stderr)
            if observed.returncode==0 and len(observed.stdout)==240:
                actual=list(struct.unpack('<60I',observed.stdout));native.update(completed=True,words=actual)
                expected=projection_words[:24]
                if constants_result and constants_result['completed']:expected+=constants_result['words']
                differences=[dict(register=31+n//4 if n<24 else 40+(n-24)//4,lane=n%4,
                                  original=o,native=v)for n,(o,v)in enumerate(zip(expected,actual))if o!=v]
                native.update(compared_words=len(expected),differences=differences,passed=not differences,
                              complete60_word_comparison=len(expected)==60)
                if captured and captured.get('completed'):
                    differences=[dict(register=31+n//4 if n<24 else 40+(n-24)//4,lane=n%4,
                                      helper=v,captured=c)for n,(v,c)in enumerate(zip(actual,captured['words']))if v!=c]
                    native.update(captured_compared_words=60,captured_differences=differences,
                                  captured_passed=not differences)
        assert all(sha(path)==pin for path,pin in pins.items()),'Native source changed during isolated probe'
    complete=bool(projection_result['completed'] and constants_result and constants_result['completed'] and
                  (captured is None or (captured['completed'] and captured['passed'])) and
                  (native is None or (native['completed'] and native.get('passed') and native.get('complete60_word_comparison'))))
    report=dict(authority='OG Xbox TU1.5',scope=__doc__,complete_uniform_math_comparison=complete,
                capture=str(a.capture.resolve()),owner=a.owner,
                tick=call['tick'],view=call['view'],input_bank_sha256=sha(case/'before.bin'),
                input_abi_sha256=sha(case/'before-abi.json'),capture_report_sha256=sha(a.capture/'report.json'),
                original_sha256=NEW_PIN,tool_sha256=sha(__file__),projection_input=row['projection_words'],
                projection_source_address=projection,shadow_depth_feature=feature,projection=projection_result,
                constants=constants_result,captured_uniforms=captured,native=native,sdk_dependencies=sdk,faults=faults,
                instruction_patches=[],replaced_game_services=[],supplied_gpu_returns=[],
                gpu_submission=False,original_framebuffer_parity=False)
    (a.output/'instruction-bytes.json').write_text(json.dumps({f'{p:08x}':b for p,b in sorted(code_bytes.items())},indent=2)+'\n')
    (a.output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(dict(projection_completed=projection_result['completed'],
                         constants_completed=bool(constants_result and constants_result['completed']),
                         native_completed=bool(native and native['completed']),
                         native_passed=None if native is None else native.get('passed'),
                         captured_uniforms_passed=None if captured is None else captured['passed'],sdk_dependencies=sdk)))
    return 0 if complete else 1

if __name__=='__main__':raise SystemExit(main())
