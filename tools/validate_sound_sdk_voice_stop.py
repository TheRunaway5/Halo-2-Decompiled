#!/usr/bin/env python3
"""Compare complete voice stop CPU owners with unchanged original expectations.

Run ordinary and ASan/UBSan builds, including real OS storage and recursive
locks. Isolated incorrect copies must fail. Declared kernel, forced retirement,
parent buffer and FIFO/MMIO inputs are separate from physical DSP/audio proof.
"""
import argparse,hashlib,json,os,shutil,subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def sha(p):
 with Path(p).open('rb') as f:return hashlib.file_digest(f,'sha256').hexdigest()
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);p.add_argument('--fixture',type=Path,required=True);a=p.parse_args();a.output.mkdir(parents=True,exist_ok=False)
 files=['tests/sound_sdk_voice_stop_tests.cpp','include/halo2/sound_sdk_voice_stop.hpp','src/sound_sdk_voice_stop.cpp','include/halo2/sound_sdk_notifications.hpp','src/sound_sdk_notifications.cpp','include/halo2/sound_sdk_device.hpp','include/halo2/sound_driver_startup.hpp','include/halo2/sound_cache_startup.hpp','include/halo2/sound_map_activation.hpp','include/halo2/native_synchronization.hpp','src/native_synchronization.cpp','include/halo2/native_sound_storage.hpp','src/native_sound_storage.cpp']
 pins={n:sha(ROOT/n)for n in files};source=a.output/'source'
 for n in files:t=source/n;t.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(ROOT/n,t)
 fixture=a.output/'original.json';shutil.copy2(a.fixture,fixture);commands=[]
 def run(args,label,success=True):
  args=list(map(str,args));env=os.environ.copy();env['ASAN_OPTIONS']='detect_leaks=1';r=subprocess.run(args,env=env,text=True,stdout=subprocess.PIPE,stderr=subprocess.PIPE);(a.output/(label+'.stdout')).write_text(r.stdout);(a.output/(label+'.stderr')).write_text(r.stderr);commands.append(dict(label=label,args=args,exit_code=r.returncode))
  assert (r.returncode==0)==success,(label,r.stdout[-1500:],r.stderr[-1500:]);return r
 units=['src/sound_sdk_voice_stop.cpp','src/sound_sdk_notifications.cpp','src/native_sound_storage.cpp','src/native_synchronization.cpp','tests/sound_sdk_voice_stop_tests.cpp']
 base=['c++','-std=c++20','-fno-fast-math','-ffp-contract=off','-frounding-math','-pthread','-I'+str(source/'include')]
 for label,flags in [('normal',['-O2']),('sanitizers',['-O1','-g','-fno-omit-frame-pointer','-fsanitize=address,undefined'])]:
  binary=a.output/label;run(base+flags+[source/n for n in units]+['-o',binary],label+'-build');r=run([binary,fixture],label+'-original');print(r.stdout.strip(),flush=True)
 native=(source/'src/sound_sdk_voice_stop.cpp').read_text()
 reds=[('count_limit','if(count<0x1eu)','if(count<=0x1eu)'),('release_count','r.read(device+0x53cu)-r.read(voice+0x64u,1)','r.read(device+0x53cu)+r.read(voice+0x64u,1)'),('pause_command','requested?0x40000u:0u','requested?0x20000u:0u'),('stop_flags','&0x3111u','&0x3113u'),('sample_flags','&0xfd7fu','&0xffffu'),('secondary_format','index?0x400000u:0u','index?0x200000u:0u'),('reload_count','index<r.read(voice+0x64u,1)','index<initial_count'),('parent_stop','need(s.stop_buffer,','if(false)need(s.stop_buffer,')]
 # Compile shared dependencies once for isolated ordinary incorrect copies.
 objects=[]
 for n in units[1:]:
  obj=a.output/(Path(n).stem+'.o');run(base+['-O2','-c',source/n,'-o',obj],Path(n).stem+'-object');objects.append(obj)
 for name,needle,replacement in reds:
  # reload_count intentionally selects only the complete shared slot-stop body.
  text=native
  if name=='reload_count':text=text.replace(needle,replacement,1)
  else:assert text.count(needle)==1,(name,text.count(needle));text=text.replace(needle,replacement)
  changed=a.output/(name+'-red.cpp');changed.write_text(text);binary=a.output/('red-'+name)
  run(base+['-O2',changed,*objects,'-o',binary],'red-'+name+'-build');r=run([binary,fixture],'red-'+name+'-original',False);assert 'voice stop' in r.stderr.lower(),(name,r.stderr);print('RED',name,'caught:',r.stderr.splitlines()[0],flush=True)
 for n,h in pins.items():assert sha(ROOT/n)==h,n
 (a.output/'report.json').write_text(json.dumps(dict(boundary=__doc__,tool_sha256=sha(__file__),source_pins=pins,fixture_sha256=sha(fixture),commands=commands,normal_and_sanitizers_pass=True,isolated_wrong_copies_caught=len(reds),required_owner_controls=15,selected_genuine_emulator_cases=0,production_source_mutated=False),indent=2)+'\n')
if __name__=='__main__':main()
