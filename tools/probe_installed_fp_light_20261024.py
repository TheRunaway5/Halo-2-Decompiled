#!/usr/bin/env python3
"""Observe real launcher plasma-light draws after KBM look and charging input.

All native light admission, source records, ranges and GPU draws execute normally.
Debugger reads completed vector results only; no native state/result is supplied.
"""
import hashlib,json,os,shlex,shutil,subprocess,time
from pathlib import Path
from PIL import Image
from mcc_content import directory
from validate_mcc_weapon_vehicle_sound_resources import Bank,merge
from validate_weapon_runtime_tu15 import private_display
from probe_installed_banshee_gameplay_20261022 import Inputs
from run_combat_presentation_audit_20261022 import keep_log
from run_test import stop_process_tree
ROOT=Path(__file__).resolve().parents[1]
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
GDB=r'''
set pagination off
set confirm off
set print thread-events off
set language c++
set debuginfod enabled off
set non-stop on
python
import gdb,json,struct,time
from pathlib import Path
c=json.loads(Path(CONFIG).read_text());out=Path(c['scratch']);steps=0;samples=[];packets=[];captured=False;light_calls=0;raw_definitions={}
def save(name,d):
 p=out/(name+'.tmp');p.write_text(json.dumps(d));p.replace(out/(name+'.json'))
def word(p,n=8):return int.from_bytes(gdb.selected_inferior().read_memory(p,n),'little')
gdb.events.exited.connect(lambda event:save('inferior-exit',dict(exit_code=getattr(event,'exit_code',None))))
class Step(gdb.Breakpoint):
 def stop(self):
  global steps
  steps+=1;p=int(gdb.parse_and_eval('$rsi'))
  samples.append(dict(step=steps,fire=bool(word(p+c['layout']['fire'],1)),pitch=struct.unpack('<f',bytes(gdb.selected_inferior().read_memory(p+c['layout']['pitch'],4)))[0],zoom=int.from_bytes(gdb.selected_inferior().read_memory(p+c['layout']['zoom'],2),'little',signed=True)))
  if steps%5==0:save('inputs',dict(steps=steps,last=samples[-1],delivered_fire_steps=sum(x['fire'] for x in samples)))
  return False
Step(c['step'],internal=True)
class LightEnd(gdb.FinishBreakpoint):
 def __init__(self,frame,pointer):super().__init__(frame,internal=True);self.pointer=pointer
 def stop(self):
  global light_calls
  light_calls+=1
  begin,end=word(self.pointer),word(self.pointer+8);stride=c['layout']['stride']
  if end<begin or (end-begin)%stride or (end-begin)//stride>8192:raise RuntimeError('Actual light vector lost its qualified layout')
  for p in range(begin,end,stride):
   definition=word(p+c['layout']['definition'],4)
   raw_definitions[str(definition)]=raw_definitions.get(str(definition),0)+1
   if str(definition) not in c['first_person']:continue
   packets.append(dict(step=steps,definition=definition,identity=word(p+c['layout']['identity'],4),name=c['first_person'][str(definition)]))
  if packets:save('lights',dict(packets=packets))
  return False
class Light(gdb.Breakpoint):
 def stop(self):
  if len(packets)>=64:self.enabled=False;return False
  LightEnd(gdb.newest_frame(),int(gdb.parse_and_eval('$rdi')));return False
Light(c['light'],internal=True)
class DrawEnd(gdb.FinishBreakpoint):
 def stop(self):
  global captured
  if captured or not packets or packets[-1]['step']!=steps or not samples or samples[-1]['pitch']>=-.5:return False
  if not int(gdb.parse_and_eval('$rax'))&255:return False
  captured=True;save('capture-ready',dict(step=steps,packets=[p for p in packets if p['step']==steps],input=samples[-1]))
  deadline=time.monotonic()+20
  while not (out/'capture-ack').exists() and time.monotonic()<deadline:time.sleep(.01)
  return False
class Draw(gdb.Breakpoint):
 def stop(self):
  if not captured:DrawEnd(gdb.newest_frame(),internal=True)
  return False
if c['muzzle']:Draw(c['draw'],internal=True)
end
run
python
save('finished',dict(steps=steps,samples=samples,packets=packets,light_calls=light_calls,raw_definitions=raw_definitions))
end
'''
def main():
 owner=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True);work=owner/'fp-light';work.mkdir();muzzle=bool(os.environ.get('HALO2_FP_LIGHT_MUZZLE'))
 out=ROOT/'analysis/combat-presentation-audit-20261022/repair-installed-fp-light'/os.environ.get('HALO2_FP_LIGHT_LABEL','run');out.mkdir(parents=True,exist_ok=True)
 selection=ROOT/'gameplay-runtime-selection.json';saved=selection.read_bytes();selected=json.loads(saved);exe=ROOT/selected['runtime'];assert sha(exe)==selected['runtime_sha256']
 pins=json.loads((exe.parent/'source-hashes.json').read_text())
 source_root=Path(os.environ.get('HALO2_INSTALLED_LAYOUT_SOURCE',os.environ.get('HALO2_FP_LIGHT_SOURCE',str(ROOT)))).resolve(strict=True)
 maps=directory(ROOT/selected['cache'])/'h2_maps_win64_dx11';map_pin=sha(maps/'coagulation.map');assert map_pin==selected['maps']['coagulation.map']
 local=Bank((maps/'coagulation.map').resolve());shared=Bank((maps/'shared.map').resolve())
 try:
  tags=merge(local,shared);gb,gt=tags[local.u(local.index+16)];mb,mt=tags[gb.u(gt['offset']+0x16c)];runtime=mb.block(mt['offset']+8,0x568);assert len(runtime)==1
  refs=mb.block(runtime[0]+0x68,8);permutation=(3,0,1,6,7,5,2,4,12,9,10,13,11,15,14,8,16,17)
  choices=[i+3 for i,n in enumerate(permutation) if mb.u(refs[n]+4)!=0xffffffff and tags[mb.u(refs[n]+4)][1]['name'].endswith('\\plasma_pistol\\plasma_pistol')];assert len(choices)==1
  first_person={str(d):t['name'] for d,(b,t) in tags.items() if t['group']=='ligh' and t['offset'] is not None and b.u(t['offset'])&0x20 and (not muzzle or not b.u(t['offset'])&0x40000)}
 finally:local.close();shared.close()
 layout_source=work/'layout.cpp';layout_source.write_text('''#include "halo2/simulation.hpp"
#include "halo2/effect_light_draw.hpp"
#include <iostream>
int main(){std::cout<<"{\\"stride\\":"<<sizeof(halo2::EffectLightRawDraw)<<",\\"definition\\":"<<offsetof(halo2::EffectLightRawDraw,light_definition)<<",\\"identity\\":"<<offsetof(halo2::EffectLightRawDraw,light_identity)<<",\\"fire\\":"<<offsetof(halo2::Controls,fire)<<",\\"pitch\\":"<<offsetof(halo2::Controls,pitch)<<",\\"zoom\\":"<<offsetof(halo2::Controls,zoom)<<"}";}
''')
 from installed_gameplay_layout import compile_installed_layout
 raw_layout,layout_provenance=compile_installed_layout(source_root,pins,
  ['include/halo2/simulation.hpp','include/halo2/effect_light_draw.hpp','include/halo2/menu_world_source_binding.hpp','include/halo2/render_program.hpp'],layout_source,work)
 layout=json.loads(raw_layout);layout_inputs=layout_provenance['inputs']
 table=subprocess.check_output(['nm','--defined-only',str(exe)],text=True).splitlines()
 def symbol(fragment):
  names=[x.split()[2] for x in table if len(x.split())==3 and x.split()[1]=='T' and fragment in x.split()[2]];assert len(names)==1,(fragment,names);return names[0]
 config=dict(scratch=str(work),layout=layout,first_person=first_person,step=symbol('MultiplayerMatch4step'),light=symbol('SourceBspEffectLights4draw'),muzzle=muzzle,draw=symbol('_ZN5halo28Renderer4drawE'))
 p=work/'config.json';p.write_text(json.dumps(config));script=work/'probe.gdb';script.write_text(GDB.replace('CONFIG',repr(str(p))))
 frame=work/'frame.ppm';log=work/'runtime.log';command=['gdb','-q','-nx','-batch','-x',str(script),'--args',str(ROOT/'Halo2-Launcher'),'--multiplayer','--map',str(maps/'coagulation.map'),'--shared-maps',str(maps),'--local-players','1','--mp-starting-weapon',str(choices[0]),'--frames','180','--mute','--viewport','800','600','--screenshot',str(frame)]
 pipeline=None
 if reference:=os.environ.get('HALO2_GAMEPLAY_PIPELINE_REFERENCE'):
  reference=Path(reference).resolve(strict=True);assert reference.stat().st_size<=64*1024*1024;private=owner/'pipeline-reference-copy';private.mkdir();copy=private/reference.name;shutil.copyfile(reference,copy);assert sha(reference)==sha(copy)
  pipeline=dict(path=str(reference),sha256=sha(reference),bytes=copy.stat().st_size)
 actions=[];start=time.monotonic();code=124
 with private_display(work) as display,log.open('w') as stream:
  env=dict(os.environ,DISPLAY=display,SDL_VIDEODRIVER='x11',SDL_AUDIODRIVER='dummy',XDG_CACHE_HOME=str(owner/'graphics-cache'))
  if pipeline:env['HALO2_PIPELINE_CACHE_DIRECTORY']=str(private)
  process=subprocess.Popen(command,env=env,stdout=stream,stderr=subprocess.STDOUT,start_new_session=True);inputs=Inputs(display);charged=False;looked=False;next_zoom=40;next_look=50;captured_frame=work/'muzzle-frame.png'
  try:
   while process.poll() is None and time.monotonic()-start<600:
    observed=json.loads((work/'inputs.json').read_text()) if (work/'inputs.json').exists() else {}
    if muzzle and (work/'capture-ready.json').exists() and not (work/'capture-ack').exists():
     inputs.capture(captured_frame,expected_size=(800,600));(work/'capture-ack').write_text('Actual X11 drawable captured after completed renderer submission.\n')
    if muzzle and not looked and observed.get('steps',0)>=25:
     inputs.focus();inputs.button(True,3);time.sleep(.1);inputs.button(False,3);time.sleep(.2);inputs.move(0,400);looked=True
     actions.append(dict(action='Actual XTest right-button relative-input focus and look down without primary fire',step=observed['steps']))
    if muzzle and looked and not charged and observed.get('last',{}).get('zoom',-1)!=-1 and observed.get('steps',0)>=next_zoom:
     inputs.button(True,3);time.sleep(.09);inputs.button(False,3);next_zoom=observed['steps']+10
     actions.append(dict(action='Actual XTest cycle observed scope back to unzoomed',step=observed['steps']))
    if muzzle and looked and not charged and observed.get('last',{}).get('zoom',-1)==-1 and observed.get('last',{}).get('pitch',0)>=-.5 and observed.get('steps',0)>=next_look:
     inputs.move(0,400);next_look=observed['steps']+10
     actions.append(dict(action='Actual XTest look down after observed scope release',step=observed['steps']))
    if muzzle and looked and not charged and observed.get('steps',0)>=120 and observed.get('last',{}).get('pitch',0)<-.5 and observed.get('last',{}).get('zoom',-1)==-1:
     inputs.button(True);time.sleep(.09);inputs.button(False);charged=True
     actions.append(dict(action='Actual XTest short primary tap',step=observed['steps']))
    if not muzzle and not charged and observed.get('steps',0)>=25:
     inputs.focus();inputs.button(True);time.sleep(.1);inputs.button(False);time.sleep(.2);inputs.move(0,400);inputs.button(True);charged=True
     actions.append(dict(action='Actual XTest look down and hold primary charge',step=observed['steps']))
    time.sleep(.05)
   if process.poll() is not None:code=process.returncode
  finally:inputs.button(False);stop_process_tree(process)
 keep_log(log,out/'runtime.log');finished=json.loads((work/'finished.json').read_text()) if (work/'finished.json').exists() else {};inferior=json.loads((work/'inferior-exit.json').read_text()) if (work/'inferior-exit.json').exists() else {}
 result=dict(scope=__doc__,command=command,runtime=selected['runtime'],runtime_sha256=sha(exe),launcher_sha256=sha(ROOT/'Halo2-Launcher'),selection_unchanged=selection.read_bytes()==saved,map_unchanged=sha(maps/'coagulation.map')==map_pin,layout=layout,layout_source_root=str(source_root),layout_inputs=layout_inputs,actions=actions,exit_code=code,inferior_exit=inferior,source_admission_or_state_supplied=False,elapsed_seconds=time.monotonic()-start,finished=finished,original_Xbox_pixels_compared=False,muzzle_short_tap=muzzle)
 result['passed']=code==0 and inferior.get('exit_code')==0 and result['selection_unchanged'] and result['map_unchanged'] and charged and any(x['fire'] and x['pitch']<-.5 for x in finished.get('samples',[])) and bool(finished.get('packets')) and frame.exists()
 if muzzle:
  result['completed_draw_capture']=json.loads((work/'capture-ready.json').read_text()) if (work/'capture-ready.json').exists() else None
  result['passed']=result['passed'] and captured_frame.exists() and bool(result['completed_draw_capture'])
 reference_frame=captured_frame if muzzle else frame
 if reference_frame.exists():
  with Image.open(reference_frame) as image:image.convert('RGB').save(out/'active-light.png')
  p=out/'active-light.png';assert p.stat().st_size<1024*1024;result['retained_reference']=dict(path=str(p),bytes=p.stat().st_size,sha256=sha(p))
 if pipeline:result['pipeline_reference']=dict(**pipeline,source_unchanged=sha(reference)==pipeline['sha256'])
 (out/'report.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps({k:v for k,v in result.items() if k not in ['scope','finished','command','layout_inputs']}),flush=True)
 return 0 if result['passed'] else 1
if __name__=='__main__':raise SystemExit(main())
