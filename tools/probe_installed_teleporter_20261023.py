#!/usr/bin/env python3
"""Walk through Coagulation's actual teleporter in launcher split screen.

One declared B95D0 approach placement; actual XTest look/W, original traversal,
completed native serial, activation PCM and both GPU views remain observations.
Persistent pad hum has separate loop acceptance; this probe cannot certify it.
"""
import contextlib,hashlib,json,math,os,shlex,shutil,signal,struct,subprocess,time
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
NON_STOP
python
import gdb,json,struct
from pathlib import Path
c=json.loads(Path(CONFIG).read_text());out=Path(c['scratch']);s=c['symbols'];steps=0;world=0;unit=0;player=0;placed=False;observed=[];cues=[];submissions=[];active=0;loop_scopes={};loop_voices=[];consumed=[];pending={};positive_routes={}
def address(key):return int(gdb.parse_and_eval("(void*)&'"+s[key]+"'"))
def word(at):return int.from_bytes(gdb.selected_inferior().read_memory(at,4),'little')
def real(at):return struct.unpack('<f',bytes(gdb.selected_inferior().read_memory(at,4)))[0]
def save(name,value):
 p=out/(name+'.tmp');p.write_text(json.dumps(value));p.replace(out/(name+'.json'))
gdb.events.exited.connect(lambda event:save('inferior-exit',dict(exit_code=getattr(event,'exit_code',None))))
def ref(key,owner):return int(gdb.parse_and_eval('(unsigned long)((void*(*)(void*,unsigned))'+str(address(key))+')('+str(owner)+',0)'))
class WorldEnd(gdb.FinishBreakpoint):
 def stop(self):
  global world
  world=int(gdb.parse_and_eval('$rax'));return False
class World(gdb.Breakpoint):
 def stop(self):self.enabled=False;WorldEnd(gdb.newest_frame(),internal=True);return False
class Step(gdb.Breakpoint):
 def stop(self):
  global steps,unit,player,placed
  steps+=1
  if steps<20 or not world:return False
  try:
   if not unit:
    match=int(gdb.parse_and_eval('$rdi'));unit=ref('unit',match);player=ref('player',match)
   if not placed:
    datum=word(unit+c['layout']['source_datum']);point=c['start']
    if datum==0xffffffff:return False
    xy=struct.unpack('<d',struct.pack('<ff',*point[:2]))[0]
    gdb.parse_and_eval('((void(*)(void*,unsigned,double,float))'+str(address('position'))+')('+str(world)+','+str(datum)+','+repr(xy)+','+repr(point[2])+')')
    placed=True;save('placed',dict(step=steps,datum=datum,yaw=real(unit+c['layout']['yaw']),position=point,process_pid=gdb.selected_inferior().pid))
   controls=int(gdb.parse_and_eval('$rsi'));position=[real(unit+c['layout']['position']+i*4) for i in range(3)]
   observed.append(dict(step=steps,forward=real(controls),yaw=real(unit+c['layout']['yaw']),input_yaw=real(controls+c['layout']['input_yaw']),input_pitch=real(controls+c['layout']['input_pitch']),position=position,serial=word(player+c['layout']['serial']),sound=word(player+c['layout']['sound'])))
   if steps%5==0:save('observed',observed)
  except Exception as e:save('error',dict(error=str(e),step=steps));return True
  return False
class EndCue(gdb.FinishBreakpoint):
 def stop(self):
  global active
  active-=1;return False
 def out_of_scope(self):
  global active
  active-=1
class Cue(gdb.Breakpoint):
 def stop(self):
  global active
  if int(gdb.parse_and_eval('$esi'))&0xffffffff!=c['activation']:return False
  active+=1;cues.append(dict(step=steps,owner=int(gdb.parse_and_eval('$ecx'))&0xffffffff));EndCue(gdb.newest_frame(),internal=True);return False
class EndSubmit(gdb.FinishBreakpoint):
 def stop(self):submissions.append(dict(step=steps,result=int(gdb.parse_and_eval('$eax'))));return False
class Submit(gdb.Breakpoint):
 def stop(self):
  if active:EndSubmit(gdb.newest_frame(),internal=True)
  return False
class LoopEnd(gdb.FinishBreakpoint):
 def __init__(self,frame,thread):super().__init__(frame,internal=True);self.thread=thread
 def stop(self):loop_scopes[self.thread].pop();return False
 def out_of_scope(self):loop_scopes[self.thread].pop()
class LoopEntry(gdb.Breakpoint):
 def stop(self):
  thread=gdb.selected_thread().global_num;loop_scopes.setdefault(thread,[]).append(int(gdb.parse_and_eval('$rdi')))
  LoopEnd(gdb.newest_frame(),thread);return False
class PlayEnd(gdb.FinishBreakpoint):
 def __init__(self,frame,event):super().__init__(frame,internal=True);self.event=event
 def stop(self):
  voice=int(gdb.parse_and_eval('$rax'));loop_voices.append(dict(**self.event,voice=voice))
  if voice:pending[voice]=self.event
  return False
class Play(gdb.Breakpoint):
 def stop(self):
  scope=loop_scopes.get(gdb.selected_thread().global_num,[])
  if not scope:return False
  event=dict(step=steps,sound=int(gdb.parse_and_eval('$esi'))&0xffffffff,
   kind=int(gdb.parse_and_eval('$edx'))&0xffffffff,owner=int(gdb.parse_and_eval('$ecx'))&0xffffffff,
   view=int(gdb.parse_and_eval('$r8d'))&0xffffffff,audio=scope[-1])
  PlayEnd(gdb.newest_frame(),event);return False
class Update(gdb.Breakpoint):
 def stop(self):
  audio=int(gdb.parse_and_eval('$rdi'));layout=c['audio_layout'];buffer=0
  try:
   for voice,event in list(pending.items()):
    if event['audio']!=audio:continue
    live=int(gdb.parse_and_eval('((bool(*)(void*,unsigned long))'+str(address('audio_active'))+')('+str(audio)+','+str(voice)+')'))
    if not live:pending.pop(voice);continue
    if not buffer:buffer=int(gdb.parse_and_eval('(void*)malloc('+str(layout['bytes'])+')'))
    if not buffer:raise RuntimeError('Native playback observation allocation failed')
    gdb.parse_and_eval('((void(*)(void*,void*,unsigned long))'+str(address('audio_position'))+')('+str(buffer)+','+str(audio)+','+str(voice)+')')
    frames=struct.unpack('<d',bytes(gdb.selected_inferior().read_memory(buffer+layout['source_frames'],8)))[0]
    output=int.from_bytes(gdb.selected_inferior().read_memory(buffer+layout['output_frames'],8),'little')
    if frames>0 and output>0:consumed.append(dict(**event,voice=voice,source_frames=frames,output_frames=output));pending.pop(voice)
  except Exception as e:save('error',dict(error=str(e),scope='actual native audio observation',step=steps));return True
  finally:
   if buffer:gdb.parse_and_eval('((void(*)(void*))free)('+str(buffer)+')')
  return False
class Parameters(gdb.Breakpoint):
 def stop(self):
  try:
   layout=c['audio_layout'];voice=int(gdb.parse_and_eval('$rsi'));at=int(gdb.parse_and_eval('$rdx'))
   volume=struct.unpack('<i',bytes(gdb.selected_inferior().read_memory(at+layout['parameters_volume'],4)))[0]
   paused=bool(bytes(gdb.selected_inferior().read_memory(at+layout['parameters_paused'],1))[0])
   pointers=bytes(gdb.selected_inferior().read_memory(at+layout['parameters_routes'],24))
   begin=int.from_bytes(pointers[layout['vector_begin']:layout['vector_begin']+8],'little');end=int.from_bytes(pointers[layout['vector_end']:layout['vector_end']+8],'little')
   if end<begin or (end-begin)%layout['route_stride'] or (end-begin)//layout['route_stride']>32:raise RuntimeError('Actual native routes escape qualified layout')
   gains=[real(at+layout['route_gain']) for at in range(begin,end,layout['route_stride'])]
   if not paused and volume>-6400 and any(gain>0 for gain in gains):positive_routes[voice]=dict(volume_hundredths_db=volume,route_gains=gains)
  except Exception as e:save('error',dict(error=str(e),scope='actual native route observation'));return True
  return False
if 'audio_layout' in c:
 LoopEntry(s['loop_request'],internal=True);LoopEntry(s['loop_frame'],internal=True)
 Play(s['loop_play'],internal=True);Update(s['audio_update'],internal=True);Parameters(s['audio_parameters'],internal=True)
World(s['world'],internal=True);Step(s['step'],internal=True);Cue(s['cue'],internal=True);Submit(s['submit'],internal=True)
end
run
python
save('finished',dict(steps=steps,observations=observed,cues=cues,submissions=submissions,loop_voices=loop_voices,consumed=consumed,positive_routes=positive_routes))
thread=gdb.selected_thread()
if thread is not None and thread.is_stopped():save('debugger-stop',dict(backtrace=gdb.execute('bt 16',to_string=True)))
end
'''

def main():
    owner=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True);work=owner/'teleporter';work.mkdir()
    out=ROOT/'analysis/combat-presentation-audit-20261022/repair-installed-teleporter'/os.environ.get('HALO2_TELEPORTER_PROBE_LABEL','run');out.mkdir(parents=True,exist_ok=True)
    selection=ROOT/'gameplay-runtime-selection.json';selection_bytes=selection.read_bytes();selected=json.loads(selection_bytes)
    exe=ROOT/selected['runtime'];assert sha(exe)==selected['runtime_sha256']
    pins=json.loads((exe.parent/'source-hashes.json').read_text())
    from installed_gameplay_layout import compile_installed_layout,installed_layout_source
    layout_source=installed_layout_source()
    maps=directory(ROOT/selected['cache'])/'h2_maps_win64_dx11'
    native_capture=bool(os.environ.get('HALO2_TELEPORTER_NATIVE_AUDIO'))
    local=Bank((maps/'coagulation.map').resolve());shared=Bank((maps/'shared.map').resolve())
    try:
        tags=merge(local,shared);scenario=local.tag(local.u(local.index+12),'scnr')[1]
        flags=[dict(position=struct.unpack_from('<3f',local.data,p),facing=struct.unpack_from('<f',local.data,p+12)[0],
            type=struct.unpack_from('<h',local.data,p+16)[0],channel=struct.unpack_from('<h',local.data,p+20)[0]) for p in local.block(scenario+0x118,0x20)]
        pairs=[(next((f for f in flags if f['channel']==channel and f['type']==7),None),next((f for f in flags if f['channel']==channel and f['type']==8),None)) for channel in range(16)]
        entrance,exit=next(pair for pair in pairs if all(pair));yaw=1.5005064328485211
        # Independently established walkable south-side route in MP_TELEPORTER_TU15_GOAL.md.
        start=list(entrance['position']);start[0]-=1.25*math.cos(yaw);start[1]-=1.25*math.sin(yaw)
        activation=[d for d,(_,t) in tags.items() if t['group']=='snd!' and t['name']=='sound\\game_sfx\\multiplayer\\teleporter_activate'];assert len(activation)==1
        hum_sounds=[d for d,(_,t) in tags.items() if t['group']=='snd!' and '\\teleport_loop\\' in t['name']]
        assert hum_sounds,'Actual authored teleporter hum resources are absent'
    finally:local.close();shared.close()
    layout=work/'layout.cpp';layout.write_text('''#include "halo2/simulation.hpp"
#include "halo2/multiplayer.hpp"
#include <iostream>
int main(){using namespace halo2;std::cout<<"{\\\"source_datum\\\":"<<offsetof(Unit,source_datum)<<",\\\"position\\\":"<<offsetof(Unit,body)+offsetof(Body,position)<<",\\\"yaw\\\":"<<offsetof(Unit,yaw)<<",\\\"input_yaw\\\":"<<offsetof(Controls,yaw)<<",\\\"input_pitch\\\":"<<offsetof(Controls,pitch)<<",\\\"serial\\\":"<<offsetof(MultiplayerPlayer,source_teleport_serial)<<",\\\"sound\\\":"<<offsetof(MultiplayerPlayer,source_teleport_sound)<<"}";}
''')
    raw_layout,layout_provenance=compile_installed_layout(layout_source,pins,
        ('include/halo2/simulation.hpp','include/halo2/multiplayer.hpp'),layout,work)
    offsets=json.loads(raw_layout)
    (out/'layout-provenance.json').write_text(json.dumps(layout_provenance,indent=2)+'\n')
    table=[line.split() for line in subprocess.check_output(['nm','--defined-only',str(exe)],text=True).splitlines()]
    symbols={}
    for key,fragment in [('world','MultiplayerObjectRuntime10tu15_world'),('unit','MultiplayerMatch4unitEj'),('player','MultiplayerMatch6playerEj'),('step','MultiplayerMatch4step'),('position','PhysicsLiveWorldTu1519set_object_position'),('cue','MultiplayerAudio7impulse'),('submit','SdlSoundPlatform11submit_impl')]:
        found=[l[2] for l in table if len(l)==3 and l[1]=='T' and fragment in l[2]];assert len(found)==1,(key,found);symbols[key]=found[0]
    pad_only=bool(os.environ.get('HALO2_TELEPORTER_PAD_ONLY'))
    config_value=dict(scratch=str(work),layout=offsets,start=start,activation=activation[0],symbols=symbols)
    if native_capture:
        audio_layout=json.loads((ROOT/'analysis/combat-presentation-audit-20261022/repair-installed-banshee/native-audio-observation-layout.json').read_text())
        assert pins['include/halo2/sdl_sound_platform.hpp']==audio_layout['header_sha256']
        config_value['audio_layout']=audio_layout
        for key,fragment in [('loop_request','MultiplayerAudio19object_loop_request'),('loop_frame','MultiplayerAudio17object_loop_frame'),('loop_play','MultiplayerAudio5State4Core4play'),('audio_active','MultiplayerAudio12voice_activeEm'),('audio_position','MultiplayerAudio17playback_positionEm'),('audio_update','MultiplayerAudio6updateEv'),('audio_parameters','SdlSoundPlatform15parameters_impl')]:
            found=[l[2] for l in table if len(l)==3 and l[1] in ('T','W') and fragment in l[2]];assert len(found)==1,(key,found);symbols[key]=found[0]
    config=work/'config.json';config.write_text(json.dumps(config_value))
    non_stop=native_capture or bool(os.environ.get('HALO2_TELEPORTER_NON_STOP'))
    script=work/'probe.gdb';script.write_text(GDB.replace('CONFIG',repr(str(config))).replace('NON_STOP','set non-stop on' if non_stop else ''))
    frame=work/'frame.ppm';log=work/'runtime.log'
    command=['gdb','-q','-nx','-batch','-x',str(script),'--args',str(ROOT/'Halo2-Launcher'),'--multiplayer','--map',str(maps/'coagulation.map'),
        '--shared-maps',str(maps),'--local-players','2','--frames','120','--viewport','800','600','--screenshot',str(frame)]
    actions=[];code=124;complete=False;started=time.monotonic()
    pipeline_reference=None
    if reference:=os.environ.get('HALO2_GAMEPLAY_PIPELINE_REFERENCE'):
        reference=Path(reference).resolve(strict=True)
        assert reference.is_file() and reference.stat().st_size<=64*1024*1024
        pin=sha(reference);private=owner/'pipeline-reference-copy';private.mkdir()
        copied=private/reference.name;shutil.copyfile(reference,copied)
        assert sha(copied)==pin and sha(reference)==pin
        pipeline_reference=dict(path=str(reference),sha256=pin,bytes=copied.stat().st_size,
            private_copy=str(copied),normal_driver_cache_admission_preserved=True)
    capture_audio=None
    with private_display(work) as display,log.open('w') as stream,contextlib.ExitStack() as owned:
        env=dict(os.environ,DISPLAY=display,SDL_VIDEODRIVER='x11',SDL_AUDIODRIVER='dummy',XDG_CACHE_HOME=str(owner/'graphics-cache'))
        if pipeline_reference:env['HALO2_PIPELINE_CACHE_DIRECTORY']=str(private)
        if native_capture:
            from native_gameplay_audio_capture import NativeGameplayAudioCapture
            capture_audio=owned.enter_context(NativeGameplayAudioCapture(work));env.update(capture_audio.environment())
        process=subprocess.Popen(command,env=env,stdout=stream,stderr=subprocess.STDOUT,start_new_session=True)
        inputs=Inputs(display);focused=False;walking=False;last_turn_step=0
        try:
            while process.poll() is None and time.monotonic()-started<int(os.environ.get('HALO2_TELEPORTER_PROBE_DEADLINE','480')):
                if (work/'error.json').exists():break
                if capture_audio and (work/'placed.json').exists() and not capture_audio.clients:
                    placed=json.loads((work/'placed.json').read_text())
                    if capture_audio.game is None:capture_audio.bind_process(process,placed['process_pid'],exe)
                    capture_audio.observe()
                if not focused and (work/'placed.json').exists():
                    placed=json.loads((work/'placed.json').read_text());inputs.focus();inputs.button(True);time.sleep(.1);inputs.button(False)
                    focused=True;actions.append(dict(action='actual_XTest_focus',at=time.monotonic()-started))
                observed=json.loads((work/'observed.json').read_text()) if (work/'observed.json').exists() else []
                if focused and not walking and observed:
                    sample=observed[-1];delta=(yaw-sample['input_yaw']+math.pi)%(2*math.pi)-math.pi
                    pitch_delta=-.5-sample['input_pitch'] if pad_only else 0
                    if abs(delta)<.025 and abs(pitch_delta)<.025:
                        if not pad_only:inputs.key('w',True)
                        walking=True
                        actions.append(dict(action='actual_XTest_pad_view_heading' if pad_only else 'actual_XTest_W_after_observed_heading',step=sample['step'],
                            actual_input_yaw=sample['input_yaw'],actual_input_pitch=sample['input_pitch'],heading_error=delta,pitch_error=pitch_delta,at=time.monotonic()-started))
                    elif sample['step']>=last_turn_step+5:
                        pixels=max(-120,min(120,round(-delta/.0025)))
                        vertical=max(-120,min(120,round(-pitch_delta/.0025)))
                        inputs.move(pixels,vertical);last_turn_step=sample['step']
                        actions.append(dict(action='actual_XTest_relative_mouse',step=sample['step'],pixels=pixels,vertical_pixels=vertical,heading_error=delta,pitch_error=pitch_delta))
                if observed and observed[-1]['serial']:inputs.key('w',False)
                if frame.exists() and frame.stat().st_size>=800*600*3:
                    try:
                        with Image.open(frame) as im:im.load();complete=im.size==(800,600)
                    except OSError:pass
                    if complete and not native_capture:
                        process.send_signal(signal.SIGINT)
                        try:code=process.wait(timeout=10)
                        except subprocess.TimeoutExpired:pass
                        break
                time.sleep(.05)
            if process.poll() is not None:code=process.returncode
        finally:inputs.key('w',False);stop_process_tree(process)
    keep_log(log,out/'runtime.log')
    actual=json.loads((work/'finished.json').read_text()) if (work/'finished.json').exists() else {}
    observed=actual.get('observations',[])
    receipt=dict(scope=__doc__,command=command,runtime_sha256=sha(exe),launcher_sha256=sha(ROOT/'Halo2-Launcher'),
        entrance=entrance,exit=exit,declared_start=start,yaw=yaw,actions=actions,actual=actual,
        complete_frame=complete,exit_code=code,natural_shutdown_verified=False,gdb_non_stop=non_stop,
        selection_unchanged=selection.read_bytes()==selection_bytes,runtime_unchanged=sha(exe)==selected['runtime_sha256'],
        pad_hum_verified=False,original_Xbox_framebuffer_or_waveform_compared=False)
    if (work/'debugger-stop.json').exists():receipt['debugger_stop']=json.loads((work/'debugger-stop.json').read_text())
    receipt['activation_completed']=complete and any(r['serial']==1 and r['sound']==activation[0] for r in observed) and len(actual.get('cues',[]))==1 and len(actual.get('submissions',[]))==1 and actual['submissions'][0]['result']==0
    if pipeline_reference:
        pipeline_reference['source_unchanged']=sha(reference)==pin
        receipt['pipeline_reference']=pipeline_reference
        receipt['activation_completed']&=pipeline_reference['source_unchanged']
    if capture_audio:
        receipt['native_audio_output']=capture_audio.summary
        receipt['pad_hum_consumed_voices']=[v for v in actual.get('consumed',[]) if v['sound'] in hum_sounds and v['kind']==2 and str(v['voice']) in actual.get('positive_routes',{})]
        placed=json.loads((work/'placed.json').read_text()) if (work/'placed.json').exists() else {}
        receipt['pad_hum_verified']=bool(receipt['pad_hum_consumed_voices'] and capture_audio.summary['clients'] and
            all(str(client['process_id'])==str(placed.get('process_pid')) for client in capture_audio.summary['clients']) and
            capture_audio.summary['nonzero_samples'] and capture_audio.summary['sink_removed'] and capture_audio.summary['default_sink_preserved'])
        exited=json.loads((work/'inferior-exit.json').read_text()) if (work/'inferior-exit.json').exists() else {}
        receipt['inferior_exit']=exited
        receipt['natural_shutdown_verified']=code==0 and complete and exited.get('exit_code')==0
    receipt['pad_visual_observation_completed']=pad_only and complete and any(
        action['action']=='actual_XTest_pad_view_heading' for action in actions)
    if complete:
        with Image.open(frame) as im:im.save(out/'two-player.png')
        receipt['retained_reference_image']=dict(path=str(out/'two-player.png'),bytes=(out/'two-player.png').stat().st_size,sha256=sha(out/'two-player.png'))
    (out/'report.json').write_text(json.dumps(receipt,indent=2)+'\n')
    print(json.dumps({k:v for k,v in receipt.items() if k not in ('actual','scope')}),flush=True)
    return 0 if (receipt['pad_visual_observation_completed'] if pad_only else receipt['activation_completed']) and receipt['selection_unchanged'] and receipt['runtime_unchanged'] and (not native_capture or (receipt['pad_hum_verified'] and receipt['natural_shutdown_verified'])) else 1

if __name__=='__main__':raise SystemExit(main())
