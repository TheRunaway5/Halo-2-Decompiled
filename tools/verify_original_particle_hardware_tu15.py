#!/usr/bin/env python3
"""Replay an original-only particle probe on host x86 with untouched TU1.5 text.

The retained native source bank and declared frame caller remain explicit
inputs. No game services are replaced. Linux IF is the only masked platform
bit, and the observed original path must not read or control that flag.
Unicorn and hardware FP state are reported separately; hardware remains the
reference for later native implementation. This is not physical Xbox proof.
"""
import argparse,json,struct,subprocess
from pathlib import Path
from oracle_charged_effect_lifetime_bank_tu15 import regions
from oracle_smg_firing_1_1 import Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from audit_rendering import file_sha

ROOT=Path(__file__).resolve().parents[1]

def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--case',type=Path,required=True);ap.add_argument('--output',type=Path)
    ap.add_argument('--host-pool-pages',action='store_true',help='Use the explicit row13 native raw-pool mmap ABI; no Xbox kernel behavior claim')
    ap.add_argument('--host-physical-protection',action='store_true',help='Use the explicit row16 native mprotect ABI; no Xbox kernel or GPU cache policy claim')
    a=ap.parse_args();case=a.case.resolve();out=(a.output or case/'hardware').resolve();assert not out.exists();out.mkdir(parents=True)
    original=json.loads((case/'report.json').read_text());assert original['completed'] and not original['supplied_game_services'] and not original['instruction_patches'];assert original['original_xbe_sha256']==NEW_PIN==file_sha(NEW_DEFAULT)
    for name in ('before.bin','native-input.bin','original-after.bin','original-exit.bin'):assert file_sha(case/name)==original['files_sha256'][name]
    if original.get('captured_caller_fp') is not None:
        assert file_sha(case/'native-fp-input.bin')==original['files_sha256']['native-fp-input.bin']
        depth,status,cw,mxcsr=struct.unpack('<IHHI',(case/'native-fp-input.bin').read_bytes())
        assert dict(depth=depth,status=status,control=cw,mxcsr=mxcsr)==original['captured_caller_fp'] and depth==0
    platform=original.get('native_platform_services',[])
    assert not (a.host_pool_pages and a.host_physical_protection)
    if a.host_pool_pages:
        assert original['entry']==0x1a5410 and original['source_context']['actual_registered_startup_entry']
        assert len(platform)==2
        for index,service in enumerate(platform):
            assert service['import_target']==0x800000b8 and service['caller_instruction']==0x33b3bf
            assert service['requested']==(0xf0,0x1e4)[index] and service['rounded']==4096
            assert service['source_token']==0x10000000+4096*index
            assert service['actual_host_initial_bytes_zero']
        helper=ROOT/'tools/replay_original_host_pool_x86.c'
    elif a.host_physical_protection:
        assert original['entry']==0x126730 and original['source_context']['actual_registered_new_map_callback']
        assert original['source_context']['row_index']==16 and len(platform)==2
        base=original['source_context']['resource_cache_physical_base']
        for index,service in enumerate(platform):
            assert service['import_target']==0x800000b6 and service['caller_instruction']==0x2db4d1
            assert service['returned']==(0x126747,0x12675d)[index]
            assert service['address']==base+(0,0x3be000)[index]
            assert service['bytes']==(0x3be000,0x40000)[index]
            assert service['protection']==(4,0x404)[index] and service['actual_mprotect_succeeded']
        helper=ROOT/'tools/replay_original_host_physical_x86.c'
    else:
        assert not platform, 'This strict helper supplies no kernel services; select the explicit platform-aware mode'
        helper=ROOT/'tools/replay_charged_effect_x86.c'
    image=Xbe(NEW_DEFAULT);owners=[_Code(image,'.text'),_Code(image,'D3D')];before=list(regions(case/'before.bin'))
    for code in owners:assert any(p<=code.lo and code.hi<=p+len(b) and b[code.lo-p:code.hi-p]==code.bytes for p,b in before)
    forbidden={'pushf','pushfd','pushfw','popf','popfd','popfw','cli','sti','iret','iretd','iretw','hlt'}
    for address,raw in original['executed_instruction_bytes'].items():
        code=next(c for c in owners if c.inside(int(address,16)));instruction=code.at(int(address,16));assert instruction.bytes.hex()==raw and instruction.mnemonic not in forbidden
    # Separate the host helper/brk from actual source heaps, retaining strict
    # MAP_FIXED_NOREPLACE checks and the unchanged original code addresses.
    binary=out/'replay-x86';compile=['cc','-m32','-fno-pie','-no-pie','-Wl,-Ttext-segment=0x70000000','-O1',str(helper),str(ROOT/'tools/replay_charged_effect_x86.S'),'-o',str(binary)];subprocess.run(compile,check=True)
    command=[str(binary),str(case),f"{original['entry']:x}",str(out)]
    with (out/'hardware.log').open('w') as log:result=subprocess.run(command,stdout=log,stderr=subprocess.STDOUT,timeout=30)
    report=dict(scope=__doc__,tool_sha256=file_sha(Path(__file__)),original_report_sha256=file_sha(case/'report.json'),original_xbe_sha256=NEW_PIN,compile=compile,command=command,binary_sha256=file_sha(binary),helper_source_sha256={str(p.relative_to(ROOT)):file_sha(p) for p in (helper,ROOT/'tools/replay_charged_effect_x86.S')},returncode=result.returncode,source_instruction_patches=[],supplied_game_services=[],native_implementation_verified=False,physical_xbox_verified=False,full_weapon_parity=False,
        declared_native_host_pool_ABI=a.host_pool_pages,declared_native_physical_protection_ABI=a.host_physical_protection,
        Xbox_GPU_cache_coherency_verified=False,Xbox_kernel_address_or_clobber_behavior_verified=False)
    if result.returncode:
        report.update(completed=False,error=(out/'hardware.log').read_text())
    else:
        if a.host_pool_pages:
            receipts=list(struct.iter_unpack('<III',(out/'host-pool-allocations.bin').read_bytes()))
            assert receipts==[(r['requested'],r['source_token'],r['rounded']) for r in platform]
            report['independent_actual_host_allocations']=receipts
        if a.host_physical_protection:
            receipts=list(struct.iter_unpack('<6I',(out/'host-physical-protections.bin').read_bytes()))
            assert receipts==[tuple(r[k] for k in ('returned','address','bytes','protection','rounded_source','rounded_bytes')) for r in platform]
            report['independent_actual_host_protections']=receipts
        emulator=(case/'original-exit.bin').read_bytes();hardware=(out/'hardware-exit.bin').read_bytes();assert len(emulator)==len(hardware)==180
        normalized=bytearray(hardware);flags=struct.unpack_from('<I',hardware,160)[0];assert flags&0x200 and not struct.unpack_from('<I',emulator,160)[0]&0x200;struct.pack_into('<I',normalized,160,flags&~0x200)
        original_rows=list(regions(case/'original-after.bin'));hardware_rows=list(regions(out/'hardware-after.bin'));assert len(original_rows)==len(hardware_rows);compared=0;differences=[]
        for (p,b),(q,c) in zip(original_rows,hardware_rows):
            assert p==q and len(b)==len(c)
            if 0x60000000<=p<0x60200000:continue
            compared+=len(b)
            if b!=c:differences.append(dict(address=p,first_difference=next(i for i,(u,v) in enumerate(zip(b,c)) if u!=v)))
        report.update(completed=True,retained_bytes_compared=compared,retained_bank_differences=differences,
            gp_xmm_flags_fs_depth_equal=emulator[:172]==normalized[:172],raw_fp_exit_equal=emulator[172:]==hardware[172:],
            entire_exit_equal_except_platform_IF=emulator==normalized,
            emulator_fp=dict(status=struct.unpack_from('<H',emulator,172)[0],control=struct.unpack_from('<H',emulator,174)[0],mxcsr=struct.unpack_from('<I',emulator,176)[0]),
            hardware_fp=dict(status=struct.unpack_from('<H',hardware,172)[0],control=struct.unpack_from('<H',hardware,174)[0],mxcsr=struct.unpack_from('<I',hardware,176)[0]),
            hardware_exit_sha256=file_sha(out/'hardware-exit.bin'),hardware_after_sha256=file_sha(out/'hardware-after.bin'),
            emulator_retained_state_matches_hardware=not differences and emulator[:172]==normalized[:172])
    (out/'report.json').write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
    print(json.dumps({k:report[k] for k in ('completed','retained_bytes_compared','gp_xmm_flags_fs_depth_equal','raw_fp_exit_equal','emulator_retained_state_matches_hardware','hardware_fp') if k in report}));return 0 if report.get('emulator_retained_state_matches_hardware') else 1
if __name__=='__main__':raise SystemExit(main())
