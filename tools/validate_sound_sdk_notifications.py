#!/usr/bin/env python3
"""Compare notification CPU owners with original instructions and reject wrong copies.

All cases also use actual OS storage and recursive locks. Time, timers, DPCs
and forced-stop calls are declared ABI boundaries, not native kernel/DSP or
audible timing proof. No failed or partial emulator capture is counted.
"""
import argparse,hashlib,json,re,shutil,subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
sha=lambda p:hashlib.sha256(Path(p).read_bytes()).hexdigest()
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);args=p.parse_args();args.output.mkdir(parents=True,exist_ok=False)
 frozen=args.output/'source';pins={};inputs=['src/sound_sdk_notifications.cpp','src/native_sound_storage.cpp','src/native_synchronization.cpp','tests/sound_sdk_notifications_tests.cpp'];pending=list(map(Path,inputs))
 while pending:
  path=pending.pop()
  if str(path)in pins:continue
  raw=(ROOT/path).read_bytes();pins[str(path)]=hashlib.sha256(raw).hexdigest();target=frozen/path;target.parent.mkdir(parents=True,exist_ok=True);target.write_bytes(raw)
  pending.extend(Path('include')/s.decode()for s in re.findall(rb'#include\s+"([^"]+)"',raw))
 fixture=args.output/'original.json';shutil.copy2(ROOT/'tests/fixtures/sound_sdk_notifications.original.json',fixture)
 common=['c++','-std=c++20','-fno-fast-math','-ffp-contract=off','-frounding-math','-fno-associative-math','-Wall','-Wextra','-pthread','-I'+str(frozen/'include'),*[str(frozen/p)for p in inputs]];commands=[]
 def run(argv,label,success=True):
  argv=list(map(str,argv));r=subprocess.run(argv,capture_output=True,text=True);(args.output/f'{label}.stdout').write_text(r.stdout);(args.output/f'{label}.stderr').write_text(r.stderr);commands.append(dict(label=label,argv=argv,exit_code=r.returncode));assert(r.returncode==0)==success,(label,r.stdout,r.stderr);return r
 for name,flags in [('normal',['-O2']),('sanitizers',['-O1','-g','-fsanitize=address,undefined','-fno-omit-frame-pointer'])]:
  binary=args.output/f'notifications_{name}';run(common+flags+['-o',binary],name+'-build');print(run([binary,fixture],name+'-original').stdout.strip(),flush=True)
 owner=frozen/inputs[0];source=owner.read_text()
 reds=[
  ('relative_time','const auto when=now-requested;','const auto when=now+requested;','arm_flags00000000_whenffffffffffffffff_'),
  ('due_equality','if(signed_time(combine(when_low,when_high))<=signed_time(now))','if(signed_time(combine(when_low,when_high))<signed_time(now))','arm_flags00000000_when0000000000000064_now0000000000000064'),
  ('stable_equal_deadlines','r.read(current+0x18u)>new_low','r.read(current+0x18u)>=new_low','sorted_peers(100, 100)_when100'),
  ('queued_flag_width','0x37f278u,notification+8u,4,','0x37f278u,notification+8u,2,','arm_flags00000000_when0000000000000001_nowffffffffffffff00'),
  ('cancel_flags_reload','r.read(notification+8u)&0xfffffffdu','flags&0xfffffffdu','reload_flags_after_timer_cancel'),
  ('detach_previous_reload','const auto previous_now=r.read(entry+4u),next_now=r.read(entry);','const auto previous_now=previous,next_now=r.read(entry);','reload_previous_after_detach_store'),
  ('arm_tail_reload','const auto last=r.read(head+4u);','const auto last=head;','reload_previous_after_insert_store'),
  ('arm_next_reload','0x37eafeu,r.read(notification)+4u','0x37eafeu,head+4u','reload_next_after_insert_store'),
  ('time_reload_after_cancel','const auto new_low=r.read(notification+0x18u),new_high=r.read(notification+0x1cu);','const auto new_low=initial_low,new_high=initial_high;','reload_time_after_nested_cancel'),
  ('pending_deadline','+0x4c4b40u','+0x4c4b41u','pending_flags8001_previous0000000000000000_delta0'),
  ('mapping_literal_one',',1)==1u?1u:0u;',',1)!=0u?1u:0u;','mapping_index0_flag02'),
  ('pending_signed_compare','if(signed_time(now)>=signed_time(limit))','if(now>=limit)','pending_signed_wrap_negative'),
 ]
 for name,needle,replacement,case in reds:
  assert source.count(needle)==1,(name,source.count(needle));mutated=args.output/f'{name}-red.cpp';mutated.write_text(source.replace(needle,replacement));binary=args.output/f'notifications_red_{name}'
  run([str(mutated)if a==str(owner)else a for a in common]+['-O2','-o',binary],f'red-{name}-build');r=run([binary,fixture],f'red-{name}-original',False)
  assert case in r.stderr and'notification'in r.stderr.lower(),(name,r.stderr);print(f'RED {name} caught: {r.stderr.splitlines()[0]}',flush=True)
 for path,pin in pins.items():assert sha(ROOT/path)==pin,'Concurrent source change: '+path
 (args.output/'report.json').write_text(json.dumps(dict(boundary=__doc__,tool_sha256=sha(__file__),source_sha256=pins,fixture_sha256=sha(fixture),commands=commands,normal_and_sanitizers_pass=True,isolated_errors_caught=len(reds),production_source_mutated=False,selected_genuine_emulator_cases=0,native_timer_or_physical_DSP_claimed=False),indent=2)+'\n')
if __name__=='__main__':main()
