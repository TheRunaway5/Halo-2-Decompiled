#!/usr/bin/env python3
"""Replay an actual source-owned charge-effect bank on host x86 hardware.

The owned Xbox TU1.5 text executes unchanged in an isolated32-bit process.
Native construction and the explicit caller ABI supply the input bank. Source
instructions receive no replacement service results. Linux user-mode IF is a
platform boundary: the replay cannot clear it, and this original path contains
no PUSHF/POPF/interrupt-control instruction. Arithmetic flags and all FP state
remain compared. This is host x86 evidence, not physical Xbox or whole boot.
"""
import argparse,hashlib,json,struct,subprocess
from pathlib import Path
from oracle_charged_effect_lifetime_bank_tu15 import regions
from oracle_smg_firing_1_1 import Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from validate_charge_effect_bank_tu15 import validate
ROOT=Path(__file__).resolve().parents[1]
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--case',type=Path,required=True);ap.add_argument('--output',type=Path);ap.add_argument('--original-report',default='original-comparison-v2.json');a=ap.parse_args();case=a.case.resolve();out=(a.output or case/'hardware-verification').resolve();out.mkdir(parents=True,exist_ok=True)
 assert sha(NEW_DEFAULT)==NEW_PIN;image=Xbe(NEW_DEFAULT);before=list(regions(case/'before.bin'))
 def read(address,size):
  for p,b in before:
   if p<=address and address+size<=p+len(b):return b[address-p:address-p+size]
  raise AssertionError(('Missing native source region',hex(address),size))
 text=next(s for s in image.sections if s.name=='.text');expected=image.section_bytes(text);assert read(text.va,len(expected))==expected
 original=json.loads((case/a.original_report).read_text());assert original['original_pc']==0x60300000 and not original['services'];assert not original['instruction_patches']
 forbidden={'9c','9d','fa','fb','cf','f4'};assert not any(raw in forbidden for raw in original['executed_instruction_bytes'].values())
 metadata=json.loads((case/'native.json').read_text());context=validate(case,metadata,before)
 # Keep the Linux helper and its randomized brk away from the source's
 # 08100000 heap bank. MAP_FIXED_NOREPLACE continues to reject any overlap.
 binary=out/'replay-x86';compile=['cc','-m32','-fno-pie','-no-pie','-Wl,-Ttext-segment=0x70000000','-O1',str(ROOT/'tools/replay_charged_effect_x86.c'),str(ROOT/'tools/replay_charged_effect_x86.S'),'-o',str(binary)]
 subprocess.run(compile,check=True);command=[str(binary),str(case),f"{metadata['entry']:x}",str(out)]
 with (out/'hardware.log').open('w') as log:subprocess.run(command,stdout=log,stderr=subprocess.STDOUT,check=True,timeout=30)
 native=(case/'native-exit.bin').read_bytes();hardware=(out/'hardware-exit.bin').read_bytes();assert len(native)==len(hardware)==180
 flags_a,flags_b=struct.unpack_from('<I',native,160)[0],struct.unpack_from('<I',hardware,160)[0]
 normalized=bytearray(hardware);struct.pack_into('<I',normalized,160,flags_b&~0x200)
 assert not flags_a&0x200 and flags_b&0x200
 compared=0;differences=[];nrows=list(regions(case/'after.bin'));hrows=list(regions(out/'hardware-after.bin'));assert len(nrows)==len(hrows)
 for (np,nb),(hp,hb) in zip(nrows,hrows):
  assert np==hp and len(nb)==len(hb)
  if 0x60000000<=np<0x60200000:continue
  compared+=len(nb)
  if nb!=hb:differences.append(dict(address=f'{np:08x}',first_difference=next(i for i,(u,v) in enumerate(zip(nb,hb)) if u!=v)))
 report=dict(scope=__doc__,validated_source_context=context,original_xbe_sha256=NEW_PIN,original_text_bytes_compared=len(expected),input_sha256=sha(case/'before.bin'),
  caller_input_sha256=sha(case/'native-input.bin'),native_exit_sha256=sha(case/'native-exit.bin'),hardware_exit_sha256=sha(out/'hardware-exit.bin'),
  raw_exit_bit_identical=native==hardware,exit_identical_except_platform_IF=native==normalized,
  native_x87_status=struct.unpack_from('<H',native,172)[0],hardware_x87_status=struct.unpack_from('<H',hardware,172)[0],
  retained_bank_bytes_compared=compared,retained_bank_differences=differences,compile=compile,command=command,
  binary_sha256=sha(binary),tool_sha256=sha(__file__),helper_source_sha256={str(p.relative_to(ROOT)):sha(p) for p in (ROOT/'tools/replay_charged_effect_x86.c',ROOT/'tools/replay_charged_effect_x86.S')},
  source_instruction_patches=[],supplied_game_services=[],full_weapon_parity=False,physical_xbox_verified=False,
  passed=native==normalized and not differences)
 (out/'report.json').write_text(json.dumps(report,indent=2,sort_keys=True)+'\n');print(json.dumps({k:report[k] for k in ('passed','retained_bank_bytes_compared','native_x87_status','hardware_x87_status','exit_identical_except_platform_IF')}));return 0 if report['passed'] else 1
if __name__=='__main__':raise SystemExit(main())
