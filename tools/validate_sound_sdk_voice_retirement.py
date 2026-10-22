#!/usr/bin/env python3
"""Compare retirement CPU owners with original instructions and reject wrong copies.

All cases also use actual OS storage and recursive locks. Forced retirement
composes the native gate, unlink and status owners. Kernel time/stall/DPC,
FIFO/MMIO and the reached381EF8 common-stop response are declared ABI
boundaries, not native kernel/DSP or audible timing proof. No failed or partial
emulator capture is counted.
"""
import argparse,hashlib,json,re,shutil,subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
sha=lambda p:hashlib.sha256(Path(p).read_bytes()).hexdigest()
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);args=p.parse_args();args.output.mkdir(parents=True,exist_ok=False)
 frozen=args.output/'source';pins={};inputs=['src/audio/sound_sdk_voice_retirement.cpp','src/audio/sound_sdk_notifications.cpp','src/audio/native_sound_storage.cpp','src/platform/native_synchronization.cpp','tests/sound_sdk_voice_retirement_tests.cpp'];pending=list(map(Path,inputs))
 while pending:
  path=pending.pop()
  if str(path)in pins:continue
  raw=(ROOT/path).read_bytes();pins[str(path)]=hashlib.sha256(raw).hexdigest();target=frozen/path;target.parent.mkdir(parents=True,exist_ok=True);target.write_bytes(raw)
  pending.extend(Path('include')/s.decode()for s in re.findall(rb'#include\s+"([^"]+)"',raw))
 fixture=args.output/'original.json';shutil.copy2(ROOT/'tests/fixtures/sound_sdk_voice_retirement.original.json',fixture)
 common=['c++','-std=c++20','-fno-fast-math','-ffp-contract=off','-frounding-math','-fno-associative-math','-Wall','-Wextra','-pthread','-I'+str(frozen/'include'), '-I'+str(frozen/'src'),*[str(frozen/p)for p in inputs]];commands=[]
 def run(argv,label,success=True):
  argv=list(map(str,argv));r=subprocess.run(argv,capture_output=True,text=True);(args.output/f'{label}.stdout').write_text(r.stdout);(args.output/f'{label}.stderr').write_text(r.stderr);commands.append(dict(label=label,argv=argv,exit_code=r.returncode));assert(r.returncode==0)==success,(label,r.stdout,r.stderr);return r
 for name,flags in [('normal',['-O2']),('sanitizers',['-O1','-g','-fsanitize=address,undefined','-fno-omit-frame-pointer'])]:
  binary=args.output/f'voice_retirement_{name}';run(common+flags+['-o',binary],name+'-build');print(run([binary,fixture],name+'-original').stdout.strip(),flush=True)
 owner=frozen/inputs[0];source=owner.read_text()
 reds=[
  ('gate_busy_mask','&0xe0u;result=0;','&0x80u;result=0;','gate_enable1_blocked20'),
  ('stall_retry_limit','counter=0x29bu','counter=0x29au','gate_completion668'),
  ('mapping_ready_literal','r.read(mapping+0xfu,1)!=1u','r.read(mapping+0xfu,1)!=0u','gate_enable1_blocked00_status08'),
  ('preserve_slot_upper','(r.read(at)&0xffff0000u)|next_slot','next_slot','unlink_g0_p1n0_count0'),
  ('previous_last_slot','*2u+0xau,2)','*2u+0xcu,2)','unlink_g0_p1n0_count0'),
  ('pending_before_group_store','if(pending)r.write','if((r.read(voice+0x12u,2)&0x8000u)!=0)r.write','unlink_pending_branch_before_group_store'),
  ('force_counter','r.read(b.force_counter_address)+1u','r.read(b.force_counter_address)+2u','force_flags0000'),
  ('release_gate','if(gated)source_sound_sdk_gate_voice_device(0','if(!gated)source_sound_sdk_gate_voice_device(0','force_flags0000'),
 ]
 for name,needle,replacement,case in reds:
  assert source.count(needle)==1,(name,source.count(needle));mutated=args.output/f'{name}-red.cpp';mutated.write_text(source.replace(needle,replacement));binary=args.output/f'voice_retirement_red_{name}'
  run([str(mutated)if a==str(owner)else a for a in common]+['-O2','-o',binary],f'red-{name}-build');r=run([binary,fixture],f'red-{name}-original',False)
  assert case in r.stderr and'retirement'in r.stderr.lower(),(name,r.stderr);print(f'RED {name} caught: {r.stderr.splitlines()[0]}',flush=True)
 for path,pin in pins.items():assert sha(ROOT/path)==pin,'Concurrent source change: '+path
 (args.output/'report.json').write_text(json.dumps(dict(boundary=__doc__,tool_sha256=sha(__file__),source_sha256=pins,fixture_sha256=sha(fixture),commands=commands,normal_and_sanitizers_pass=True,isolated_errors_caught=len(reds),production_source_mutated=False,selected_genuine_emulator_cases=0,native_timer_or_physical_DSP_claimed=False),indent=2)+'\n')
if __name__=='__main__':main()
