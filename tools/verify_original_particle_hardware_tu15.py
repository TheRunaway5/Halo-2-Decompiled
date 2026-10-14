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
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--case',type=Path,required=True);ap.add_argument('--output',type=Path);a=ap.parse_args();case=a.case.resolve();out=(a.output or case/'hardware').resolve();assert not out.exists();out.mkdir(parents=True)
    original=json.loads((case/'report.json').read_text());assert original['completed'] and not original['supplied_game_services'] and not original['instruction_patches'];assert original['original_xbe_sha256']==NEW_PIN==file_sha(NEW_DEFAULT)
    for name in ('before.bin','native-input.bin','original-after.bin','original-exit.bin'):assert file_sha(case/name)==original['files_sha256'][name]
    image=Xbe(NEW_DEFAULT);owners=[_Code(image,'.text'),_Code(image,'D3D')];before=list(regions(case/'before.bin'))
    for code in owners:assert any(p<=code.lo and code.hi<=p+len(b) and b[code.lo-p:code.hi-p]==code.bytes for p,b in before)
    forbidden={'pushf','pushfd','pushfw','popf','popfd','popfw','cli','sti','iret','iretd','iretw','hlt'}
    for address,raw in original['executed_instruction_bytes'].items():
        code=next(c for c in owners if c.inside(int(address,16)));instruction=code.at(int(address,16));assert instruction.bytes.hex()==raw and instruction.mnemonic not in forbidden
    # Separate the host helper/brk from actual source heaps, retaining strict
    # MAP_FIXED_NOREPLACE checks and the unchanged original code addresses.
    binary=out/'replay-x86';compile=['cc','-m32','-fno-pie','-no-pie','-Wl,-Ttext-segment=0x70000000','-O1',str(ROOT/'tools/replay_charged_effect_x86.c'),str(ROOT/'tools/replay_charged_effect_x86.S'),'-o',str(binary)];subprocess.run(compile,check=True)
    command=[str(binary),str(case),f"{original['entry']:x}",str(out)]
    with (out/'hardware.log').open('w') as log:result=subprocess.run(command,stdout=log,stderr=subprocess.STDOUT,timeout=30)
    report=dict(scope=__doc__,tool_sha256=file_sha(Path(__file__)),original_report_sha256=file_sha(case/'report.json'),original_xbe_sha256=NEW_PIN,compile=compile,command=command,binary_sha256=file_sha(binary),helper_source_sha256={str(p.relative_to(ROOT)):file_sha(p) for p in (ROOT/'tools/replay_charged_effect_x86.c',ROOT/'tools/replay_charged_effect_x86.S')},returncode=result.returncode,source_instruction_patches=[],supplied_game_services=[],native_implementation_verified=False,physical_xbox_verified=False,full_weapon_parity=False)
    if result.returncode:
        report.update(completed=False,error=(out/'hardware.log').read_text())
    else:
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
