#!/usr/bin/env python3
"""Run the selected production engine with a declared original-position setup.

Debugger breakpoints observe completed owners. One B95D0 player placement and
one seated-vehicle placement are declared fixture inputs. Entry, flight, fire,
source physics, lens queries and the final GPU frame remain actual gameplay.
No instruction body, admission, hit, lifetime or GPU result is supplied.
"""
import contextlib,ctypes,hashlib,json,math,os,re,signal,struct,subprocess,time,shutil
from pathlib import Path
from PIL import Image,ImageGrab
from validate_weapon_runtime_tu15 import private_display
from run_combat_presentation_audit_20261022 import keep_log
ROOT=Path(__file__).resolve().parents[1]
GDB=r'''
set pagination off
set confirm off
set print thread-events off
set language c++
set debuginfod enabled off
set breakpoint pending on
NON_STOP
python
import gdb,json,struct
from pathlib import Path
config=json.loads(Path(CONFIG).read_text());root=Path(config['scratch']);setup=config['setup'];symbols=config['symbols']
world=0;match=0;steps=0;placed=False;seated=False;vehicle=0xffffffff;observations=[];vehicle_samples=[];publications=[];audio_requests=[];audio_voices=[];audio_consumption=[];pending_audio={};loop_scopes={};loop_voices=[];positive_routes={}
def pointer(key):return int(gdb.parse_and_eval("(void*)&'"+symbols[key]+"'"))
def call_pointer(key,obj,arg=None):
    args=str(obj)+('' if arg is None else ','+str(arg));types='void*'+('' if arg is None else ',unsigned')
    return int(gdb.parse_and_eval('(unsigned long)((void*(*)('+types+'))'+str(pointer(key))+')('+args+')'))
def read32(at):return int.from_bytes(gdb.selected_inferior().read_memory(at,4),'little')
def readfloat(at):return struct.unpack('<f',bytes(gdb.selected_inferior().read_memory(at,4)))[0]
def place(datum,point):
    xy=struct.unpack('<d',struct.pack('<ff',*point[:2]))[0]
    gdb.parse_and_eval('((void(*)(void*,unsigned,double,float))'+str(pointer('position'))+')('+str(world)+','+str(datum)+','+repr(xy)+','+repr(point[2])+')')
def record(name,data):
    path=root/(name+'.tmp');path.write_text(json.dumps(data));path.replace(root/(name+'.json'));print('[COMBAT-INSTALLED] '+name+' '+json.dumps(data),flush=True)
gdb.events.exited.connect(lambda event:record('inferior-exit',dict(exit_code=getattr(event,'exit_code',None))))
class WorldReturn(gdb.FinishBreakpoint):
    def stop(self):
        global world
        world=int(gdb.parse_and_eval('$rax'));return False
class WorldEntry(gdb.Breakpoint):
    def stop(self):
        self.enabled=False;WorldReturn(gdb.newest_frame(),internal=True);return False
class Step(gdb.Breakpoint):
    def stop(self):
        global match,steps,placed,seated,vehicle
        steps+=1
        if not world or steps<20:return False
        try:
            if seated:
                inputs=int(gdb.parse_and_eval('$rsi'))
                fire=bool(bytes(gdb.selected_inferior().read_memory(inputs+setup['controls_fire_offset'],1))[0])
                boost=bool(bytes(gdb.selected_inferior().read_memory(inputs+config['audio_layout']['controls_secondary'],1))[0]) if 'audio_layout' in config else False
                observations.append(dict(step=steps,fire=fire,boost=boost))
                if len(observations)%30==0:
                    if 'body' in symbols:
                        # NativeRuntimeAddress is one qualified64-bit word;
                        # object_body validates the full live object generation.
                        body=call_pointer('body',world,vehicle)
                        if body<=0xffffffff:raise RuntimeError('Native body observation lost its full host address')
                        vehicle_samples.append(dict(step=steps,body=body,
                            velocity=[readfloat(body+0x88+n*4) for n in range(3)],
                            boost_age=readfloat(body+0x334),boost_cooldown=readfloat(body+0x338),
                            original_wingtip_factor=readfloat(body+0x374)))
                    record('inputs-observed',dict(samples=len(observations),delivered_fire_steps=sum(o['fire'] for o in observations)))
                return False
            match=int(gdb.parse_and_eval('$rdi'));unit=call_pointer('unit',match,0)
            datum=read32(unit+setup['unit_source_datum_offset'])
            if datum==0xffffffff:return False
            if not placed:
                place(datum,setup['position']);placed=True
                record('placed',dict(unit=datum,position=setup['position'],yaw=readfloat(unit+setup['unit_yaw_offset']),step=steps,process_pid=gdb.selected_inferior().pid));return False
            parent=read32(unit+setup['unit_parent_offset'])
            if parent!=0xffffffff and not seated:
                place(parent,setup['airborne']);seated=True;vehicle=parent
                record('seated',dict(unit=datum,parent=parent,position=setup['airborne'],step=steps))
            return False
        except Exception as error:
            record('debugger-error',dict(error=str(error),step=steps));return True
WorldEntry(symbols['world'],internal=True)
Step(symbols['step'],internal=True)
class Published(gdb.Breakpoint):
    def stop(self):
        if seated:
            publications.append(dict(step=steps,count=int(gdb.parse_and_eval('$rdx'))))
            if len(publications)%20==0:record('publications-observed',dict(samples=len(publications),published_frames=sum(p['count']>0 for p in publications),maximum_live_projectiles=max(p['count'] for p in publications)))
        return False
Published(symbols['published'],internal=True)
class AudioReturn(gdb.FinishBreakpoint):
    def __init__(self,frame,event):
        super().__init__(frame,internal=True);self.event=event
    def stop(self):
        voice=int(gdb.parse_and_eval('$rax'));engaged=int(gdb.parse_and_eval('$rdx'))&255
        audio_voices.append(dict(**self.event,voice=voice,engaged=engaged))
        if engaged and voice and 'audio_layout' in config:pending_audio[voice]=self.event
        return False
class Audio(gdb.Breakpoint):
    def stop(self):
        if not seated:return False
        raw=int(gdb.parse_and_eval('$rdx'));owner=read32(raw+0x50)
        if owner!=vehicle:return False
        sound=int(gdb.parse_and_eval('$esi'))&0xffffffff
        view=int(gdb.parse_and_eval('$r8d'))&0xffffffff
        event=dict(step=steps,sound=sound,owner=owner,view=view,audio_pointer=int(gdb.parse_and_eval('$rdi')),object_loop=False)
        audio_requests.append(event);AudioReturn(gdb.newest_frame(),event);return False
if 'audio' in symbols:Audio(symbols['audio'],internal=True)
class LoopEnd(gdb.FinishBreakpoint):
    def __init__(self,frame,thread):super().__init__(frame,internal=True);self.thread=thread
    def stop(self):loop_scopes[self.thread].pop();return False
    def out_of_scope(self):loop_scopes[self.thread].pop()
class LoopEntry(gdb.Breakpoint):
    def stop(self):
        thread=gdb.selected_thread().global_num
        loop_scopes.setdefault(thread,[]).append(int(gdb.parse_and_eval('$rdi')))
        LoopEnd(gdb.newest_frame(),thread);return False
class LoopPlayEnd(gdb.FinishBreakpoint):
    def __init__(self,frame,event):super().__init__(frame,internal=True);self.event=event
    def stop(self):
        voice=int(gdb.parse_and_eval('$rax'));loop_voices.append(dict(**self.event,voice=voice))
        if voice:pending_audio[voice]=self.event
        return False
class LoopPlay(gdb.Breakpoint):
    def stop(self):
        scope=loop_scopes.get(gdb.selected_thread().global_num,[])
        if not scope:return False
        event=dict(step=steps,sound=int(gdb.parse_and_eval('$esi'))&0xffffffff,
            kind=int(gdb.parse_and_eval('$edx'))&0xffffffff,owner=int(gdb.parse_and_eval('$ecx'))&0xffffffff,
            view=int(gdb.parse_and_eval('$r8d'))&0xffffffff,audio_pointer=scope[-1],object_loop=True)
        LoopPlayEnd(gdb.newest_frame(),event);return False
if 'loop_play' in symbols:
    LoopEntry(symbols['loop_request'],internal=True);LoopEntry(symbols['loop_frame'],internal=True)
    LoopPlay(symbols['loop_play'],internal=True)
class Parameters(gdb.Breakpoint):
    def stop(self):
        try:
            layout=config['audio_layout'];voice=int(gdb.parse_and_eval('$rsi'));at=int(gdb.parse_and_eval('$rdx'))
            volume=struct.unpack('<i',bytes(gdb.selected_inferior().read_memory(at+layout['parameters_volume'],4)))[0]
            paused=bool(bytes(gdb.selected_inferior().read_memory(at+layout['parameters_paused'],1))[0])
            routes=at+layout['parameters_routes'];pointers=bytes(gdb.selected_inferior().read_memory(routes,24))
            begin=int.from_bytes(pointers[layout['vector_begin']:layout['vector_begin']+8],'little')
            end=int.from_bytes(pointers[layout['vector_end']:layout['vector_end']+8],'little')
            if end<begin or (end-begin)%layout['route_stride'] or (end-begin)//layout['route_stride']>32:raise RuntimeError('Actual native route extent escapes qualified layout')
            gains=[readfloat(at+layout['route_gain']) for at in range(begin,end,layout['route_stride'])]
            if not paused and volume>-6400 and any(gain>0 for gain in gains):positive_routes[voice]=dict(volume_hundredths_db=volume,route_gains=gains)
        except Exception as error:record('debugger-error',dict(error=str(error),scope='actual native route observation'));return True
        return False
if 'audio_parameters' in symbols:Parameters(symbols['audio_parameters'],internal=True)
class AudioUpdate(gdb.Breakpoint):
    def stop(self):
        if not pending_audio:return False
        audio=int(gdb.parse_and_eval('$rdi'));layout=config['audio_layout'];buffer=0
        try:
            for voice,event in list(pending_audio.items()):
                if event['audio_pointer']!=audio:continue
                active=int(gdb.parse_and_eval('((bool(*)(void*,unsigned long))'+str(pointer('audio_active'))+')('+str(audio)+','+str(voice)+')'))
                if not active:pending_audio.pop(voice);continue
                if not buffer:buffer=int(gdb.parse_and_eval('(void*)malloc('+str(layout['bytes'])+')'))
                if not buffer:raise RuntimeError('Native playback observation allocation failed')
                gdb.parse_and_eval('((void(*)(void*,void*,unsigned long))'+str(pointer('audio_position'))+')('+str(buffer)+','+str(audio)+','+str(voice)+')')
                frames=struct.unpack('<d',bytes(gdb.selected_inferior().read_memory(buffer+layout['source_frames'],8)))[0]
                output=int.from_bytes(gdb.selected_inferior().read_memory(buffer+layout['output_frames'],8),'little')
                if frames>0 and output>0:
                    audio_consumption.append(dict(**event,voice=voice,observed_step=steps,source_frames=frames,output_frames=output))
                    pending_audio.pop(voice);record('audio-consumption',dict(voices=len(audio_consumption),output_frames=sum(v['output_frames'] for v in audio_consumption)))
        except Exception as error:record('debugger-error',dict(error=str(error),step=steps,scope='actual native playback observation'));return True
        finally:
            if buffer:gdb.parse_and_eval('((void(*)(void*))free)('+str(buffer)+')')
        return False
if 'audio_layout' in config:AudioUpdate(symbols['audio_update'],internal=True)
end
run
python
record('finished',dict(steps=steps,placed=placed,seated=seated,delivered_fire_steps=sum(o['fire'] for o in observations),
    published_frames=sum(p['count']>0 for p in publications),maximum_live_projectiles=max((p['count'] for p in publications),default=0),
    audio_requests=audio_requests,audio_voices=audio_voices,audio_consumption=audio_consumption,loop_voices=loop_voices,
    delivered_boost_steps=sum(o['boost'] for o in observations),vehicle_samples=vehicle_samples,positive_routes=positive_routes))
end
thread apply all bt 8
'''
class Inputs:
 def __init__(self,display):
  self.display=display;self.window=None
  self.x=ctypes.CDLL('libX11.so.6');self.t=ctypes.CDLL('libXtst.so.6')
  self.x.XOpenDisplay.argtypes=[ctypes.c_char_p];self.x.XOpenDisplay.restype=ctypes.c_void_p;self.d=self.x.XOpenDisplay(display.encode());assert self.d
  self.x.XDefaultRootWindow.argtypes=[ctypes.c_void_p];self.x.XDefaultRootWindow.restype=ctypes.c_ulong
  self.x.XQueryTree.argtypes=[ctypes.c_void_p,ctypes.c_ulong,ctypes.POINTER(ctypes.c_ulong),ctypes.POINTER(ctypes.c_ulong),ctypes.POINTER(ctypes.POINTER(ctypes.c_ulong)),ctypes.POINTER(ctypes.c_uint)]
  self.x.XSetInputFocus.argtypes=[ctypes.c_void_p,ctypes.c_ulong,ctypes.c_int,ctypes.c_ulong]
  self.x.XFlush.argtypes=[ctypes.c_void_p];self.x.XFree.argtypes=[ctypes.c_void_p]
  self.x.XGetGeometry.argtypes=[ctypes.c_void_p,ctypes.c_ulong,ctypes.POINTER(ctypes.c_ulong),ctypes.POINTER(ctypes.c_int),ctypes.POINTER(ctypes.c_int),ctypes.POINTER(ctypes.c_uint),ctypes.POINTER(ctypes.c_uint),ctypes.POINTER(ctypes.c_uint),ctypes.POINTER(ctypes.c_uint)]
  self.x.XStringToKeysym.argtypes=[ctypes.c_char_p];self.x.XStringToKeysym.restype=ctypes.c_ulong
  self.x.XKeysymToKeycode.argtypes=[ctypes.c_void_p,ctypes.c_ulong];self.x.XKeysymToKeycode.restype=ctypes.c_uint
  self.t.XTestFakeKeyEvent.argtypes=[ctypes.c_void_p,ctypes.c_uint,ctypes.c_int,ctypes.c_ulong]
  self.t.XTestFakeButtonEvent.argtypes=[ctypes.c_void_p,ctypes.c_uint,ctypes.c_int,ctypes.c_ulong]
  self.t.XTestFakeRelativeMotionEvent.argtypes=[ctypes.c_void_p,ctypes.c_int,ctypes.c_int,ctypes.c_ulong]
 def focus(self):
  root=self.x.XDefaultRootWindow(self.d);r=ctypes.c_ulong();p=ctypes.c_ulong();children=ctypes.POINTER(ctypes.c_ulong)();count=ctypes.c_uint()
  assert self.x.XQueryTree(self.d,root,ctypes.byref(r),ctypes.byref(p),ctypes.byref(children),ctypes.byref(count)) and count.value
  window=children[count.value-1];self.window=window;self.x.XSetInputFocus(self.d,window,1,0);self.x.XFree(children);self.x.XFlush(self.d)
 def capture(self,path,expected_size=(800,450)):
  assert self.window
  root=ctypes.c_ulong();x=ctypes.c_int();y=ctypes.c_int();w=ctypes.c_uint();h=ctypes.c_uint();border=ctypes.c_uint();depth=ctypes.c_uint()
  assert self.x.XGetGeometry(self.d,self.window,ctypes.byref(root),ctypes.byref(x),ctypes.byref(y),ctypes.byref(w),ctypes.byref(h),ctypes.byref(border),ctypes.byref(depth))
  assert (w.value,h.value)==expected_size,'Capture must observe the actual game drawable dimensions'
  with ImageGrab.grab(xdisplay=self.display,bbox=(x.value,y.value,x.value+w.value,y.value+h.value)) as frame:
   assert frame.size==expected_size;frame.save(path)
  return dict(x=x.value,y=y.value,width=w.value,height=h.value)
 def key(self,name,down):self.t.XTestFakeKeyEvent(self.d,self.x.XKeysymToKeycode(self.d,self.x.XStringToKeysym(name.encode())),down,0);self.x.XFlush(self.d)
 def button(self,down,number=1):self.t.XTestFakeButtonEvent(self.d,number,down,0);self.x.XFlush(self.d)
 def move(self,x,y):self.t.XTestFakeRelativeMotionEvent(self.d,x,y,0);self.x.XFlush(self.d)
def main():
 owner=Path(os.environ['HALO2_TEST_SCRATCH']);scratch=owner/('installed-banshee-'+str(time.time_ns()));scratch.mkdir(exist_ok=False);out=ROOT/'analysis/combat-presentation-audit-20261022/repair-installed-banshee'/os.environ.get('HALO2_COMBAT_INSTALLED_LABEL','v2');out.mkdir(parents=True,exist_ok=True)
 selection=ROOT/'gameplay-runtime-selection.json';selected_bytes=selection.read_bytes();selected=json.loads(selected_bytes);exe=ROOT/os.environ.get('HALO2_COMBAT_INSTALLED_RUNTIME',selected['runtime']);sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest();assert sha(exe)==os.environ.get('HALO2_COMBAT_INSTALLED_SHA256',selected['runtime_sha256'])
 pins=json.loads((exe.parent/'source-hashes.json').read_text());assert pins['include/halo2/simulation.hpp']==sha(ROOT/'include/halo2/simulation.hpp')
 setup=json.loads((ROOT/'analysis/combat-presentation-audit-20261022/repair-attachment-material/banshee-setup.json').read_text())
 table=subprocess.check_output(['nm','--defined-only',str(exe)],text=True).splitlines();symbols={}
 for key,fragment in [('world','MultiplayerObjectRuntime10tu15_world'),('unit','MultiplayerMatch4unitEj'),('step','MultiplayerMatch4step'),('position','PhysicsLiveWorldTu1519set_object_position'),('published','NativeWeaponProjectileScene11synchronize')]:
  found=[line.split()[2] for line in table if len(line.split())==3 and line.split()[1]=='T' and fragment in line.split()[2]];assert len(found)==1,(key,found);symbols[key]=found[0]
 audible=bool(os.environ.get('HALO2_COMBAT_INSTALLED_AUDIO'))
 native_capture=bool(os.environ.get('HALO2_COMBAT_INSTALLED_NATIVE_AUDIO'))
 assert not native_capture or audible,'Native output capture requires actual unmuted game audio'
 audio_layout=None
 if audible:
  found=[line.split()[2] for line in table if len(line.split())==3 and line.split()[1]=='T' and 'MultiplayerAudio14source_impulse' in line.split()[2]];assert len(found)==1,found;symbols['audio']=found[0]
 if native_capture:
  assert pins['include/halo2/native_runtime_address.hpp']==sha(ROOT/'include/halo2/native_runtime_address.hpp')
  audio_layout=json.loads((ROOT/'analysis/combat-presentation-audit-20261022/repair-installed-banshee/native-audio-observation-layout.json').read_text())
  assert pins['include/halo2/sdl_sound_platform.hpp']==audio_layout['header_sha256'],'Installed native audio layout has not been qualified'
  assert pins['include/halo2/simulation.hpp']==audio_layout['simulation_header_sha256']
  for key,fragment in [('audio_active','MultiplayerAudio12voice_activeEm'),('audio_position','MultiplayerAudio17playback_positionEm'),('audio_update','MultiplayerAudio6updateEv'),('audio_parameters','SdlSoundPlatform15parameters_impl')]:
   found=[line.split()[2] for line in table if len(line.split())==3 and line.split()[1]=='T' and fragment in line.split()[2]];assert len(found)==1,(key,found);symbols[key]=found[0]
  for key,fragment in [('loop_request','MultiplayerAudio19object_loop_request'),('loop_frame','MultiplayerAudio17object_loop_frame'),('loop_play','MultiplayerAudio5State4Core4play')]:
   found=[line.split()[2] for line in table if len(line.split())==3 and line.split()[1] in ('T','W') and fragment in line.split()[2]];assert len(found)==1,(key,found);symbols[key]=found[0]
  found=[line.split()[2] for line in table if len(line.split())==3 and line.split()[1]=='T' and 'PhysicsLiveWorldTu1511object_bodyEj' in line.split()[2]];assert len(found)==1,found;symbols['body']=found[0]
 config_value=dict(scratch=str(scratch),setup=setup,symbols=symbols)
 if audio_layout:config_value['audio_layout']=audio_layout
 config=scratch/'config.json';config.write_text(json.dumps(config_value));script=scratch/'probe.gdb';script.write_text(GDB.replace('CONFIG',repr(str(config))).replace('NON_STOP','set non-stop on' if native_capture else ''))
 from mcc_content import directory
 maps=directory(ROOT/selected['cache'])/'h2_maps_win64_dx11';capture=scratch/'final.ppm';log=scratch/'runtime.log'
 map_pin=sha(maps/'coagulation.map');assert map_pin==selected['maps']['coagulation.map'],'Selected gameplay map changed before the probe'
 command=['gdb','-q','-nx','-batch','-x',str(script),'--args',str(exe),'--multiplayer','--map',str(maps/'coagulation.map'),'--shared-maps',str(maps),'--local-players','2','--frames','240' if native_capture else '120','--mute','--viewport','800','450','--screenshot',str(capture)]
 through_launcher=bool(os.environ.get('HALO2_COMBAT_INSTALLED_LAUNCHER'))
 if through_launcher:
  assert exe==ROOT/selected['runtime'],'Launcher probe requires the actual selected runtime'
  command[command.index('--args')+1]=str(ROOT/'Halo2-Launcher')
 if audible:command.remove('--mute')
 actions=[];start=time.monotonic();code=124;controlled_stop=None
 deadline_seconds=float(os.environ.get('HALO2_COMBAT_INSTALLED_TIMEOUT','600'))
 level_boost=bool(os.environ.get('HALO2_COMBAT_INSTALLED_LEVEL_BOOST'))
 maneuver_boost=bool(os.environ.get('HALO2_COMBAT_INSTALLED_MANEUVER_BOOST'))
 turn_phases=int(os.environ.get('HALO2_COMBAT_INSTALLED_TURN_PHASES','4'))
 assert 4<=turn_phases<=16,'Bound actual XTest turn phases'
 maneuver_sample=0
 assert 1<=deadline_seconds<=600,'Declare a finite installed-gameplay probe deadline'
 pipeline_reference=None
 if reference:=os.environ.get('HALO2_COMBAT_INSTALLED_PIPELINE_REFERENCE'):
  reference=Path(reference).resolve(strict=True);assert reference.is_file() and reference.stat().st_size<=64*1024*1024
  pin=sha(reference);private_cache=owner/'pipeline-reference-copy';private_cache.mkdir()
  copied=private_cache/reference.name;shutil.copyfile(reference,copied)
  assert sha(copied)==pin and sha(reference)==pin,'Pipeline reference changed while copying'
  pipeline_reference=dict(path=str(reference),sha256=pin,bytes=copied.stat().st_size,
      private_copy=str(copied),normal_driver_cache_admission_preserved=True)
 capture_audio=None;flight_images=[];flight_samples=set()
 with private_display(scratch) as display,log.open('w') as stream,contextlib.ExitStack() as owned:
  env=dict(os.environ,DISPLAY=display,SDL_VIDEODRIVER='x11',SDL_AUDIODRIVER='dummy',XDG_CACHE_HOME=str(owner/'graphics-cache'))
  if native_capture:
   from native_gameplay_audio_capture import NativeGameplayAudioCapture
   capture_audio=owned.enter_context(NativeGameplayAudioCapture(scratch));env.update(capture_audio.environment())
  if pipeline_reference:env.update(HALO2_PIPELINE_CACHE_DIRECTORY=str(private_cache),HALO2_PIPELINE_CACHE_TRACE='1')
  process=subprocess.Popen(command,env=env,stdout=stream,stderr=subprocess.STDOUT);input_=Inputs(display);turned=False;fire=False;boost=False;boost_released=False
  try:
   while process.poll() is None and time.monotonic()-start<deadline_seconds:
    if (scratch/'debugger-error.json').exists():break
    if capture_audio and (scratch/'placed.json').exists() and not capture_audio.clients:
     placed=json.loads((scratch/'placed.json').read_text())
     if capture_audio.game is None:capture_audio.bind_process(process,placed['process_pid'],exe)
     capture_audio.observe()
    if not turned and (scratch/'placed.json').exists():
     placed=json.loads((scratch/'placed.json').read_text());input_.focus();input_.button(True);time.sleep(.15);input_.button(False)
     delta=(setup['yaw']-placed['yaw']+math.pi)%(2*math.pi)-math.pi;remaining=round(-delta/.0025)
     while remaining:
      n=max(-35,min(35,remaining));input_.move(n,0);remaining-=n;time.sleep(.12)
     input_.key('e',True);turned=True;actions.append(dict(action='actual_XTest_mouse_look_and_E_hold',delta=delta,at=time.monotonic()-start))
    if not fire and (scratch/'seated.json').exists():
     input_.key('e',False);input_.key('w',True);input_.key('space',True);input_.move(0,-80);input_.button(True);fire=True;actions.append(dict(action='actual_XTest_flight_and_continuous_primary_fire',at=time.monotonic()-start))
    if native_capture and fire and (scratch/'inputs-observed.json').exists():
     sample=json.loads((scratch/'inputs-observed.json').read_text())
     if maneuver_boost and sample['samples'] in (30,60,90) and sample['samples'] not in flight_samples:
      flight_samples.add(sample['samples']);path=scratch/('flight-'+str(sample['samples'])+'.png')
      try:
       rectangle=input_.capture(path);flight_images.append(dict(path=path,sample=sample['samples'],rectangle=rectangle))
      except Exception as error:actions.append(dict(action='flight_capture_diagnostic_failed',error=str(error),sample=sample['samples']))
     if maneuver_boost and sample['samples']>=30 and sample['samples']//30>maneuver_sample:
      maneuver_sample=sample['samples']//30
      if maneuver_sample<=turn_phases:
       dx=160 if maneuver_sample%2 else -160;input_.move(dx,-80 if maneuver_sample==1 else 0)
       actions.append(dict(action='actual_XTest_boost_turn',sample=sample['samples'],mouse=[dx,-80 if maneuver_sample==1 else 0],at=time.monotonic()-start))
     if not boost and sample['samples']>=30:
      if level_boost:
       input_.key('space',False);input_.move(0,80);actions.append(dict(action='actual_XTest_release_climb_and_level_for_forward_boost',at=time.monotonic()-start))
      input_.button(True,3);boost=True;actions.append(dict(action='actual_XTest_right_mouse_boost_hold',at=time.monotonic()-start))
     if boost and not boost_released and sample['samples']>=90:
      input_.button(False,3);boost_released=True;actions.append(dict(action='actual_XTest_right_mouse_boost_release',at=time.monotonic()-start))
    if not (native_capture or os.environ.get('HALO2_COMBAT_INSTALLED_WAIT_FOR_EXIT')) and capture.exists() and capture.stat().st_size>=800*450*3:
     try:
      with Image.open(capture) as im:im.load();assert im.size==(800,450)
     except (OSError,AssertionError):pass
     else:
      controlled_stop=dict(reason='Controlled debugger stop after the complete finite GPU capture; natural process shutdown is not acceptance',at_seconds=time.monotonic()-start)
      (scratch/'capture-stop.json').write_text(json.dumps(controlled_stop,indent=2)+'\n')
      process.send_signal(signal.SIGINT)
      try:code=process.wait(timeout=15)
      except subprocess.TimeoutExpired:pass
      break
    time.sleep(.05)
   if process.poll() is None:
    if time.monotonic()-start>=deadline_seconds:
     controlled_stop=dict(reason='Finite gameplay probe deadline before required actions and submitted frames',at_seconds=time.monotonic()-start)
     process.send_signal(signal.SIGINT)
    else:process.terminate()
   try:code=process.wait(timeout=15)
   except subprocess.TimeoutExpired:
    controlled_stop=dict(reason='Finite probe deadline/shutdown timeout; complete gameplay and natural process shutdown are not accepted',at_seconds=time.monotonic()-start)
    process.kill();process.wait();code=124
  finally:
   for key in ['e','w','space']:input_.key(key,False)
   input_.button(False)
   input_.button(False,3)
   if process.poll() is None:process.kill();process.wait()
 raw_log=log.read_text(errors='replace')
 lens_samples=[tuple(map(int,row)) for row in re.findall(r'Source lens queries: frame=(\d+) view=(\d+) issued=(\d+) flares=(\d+)',raw_log)]
 keep_log(log,out/'runtime.log');result=dict(scope=__doc__,command=command,runtime_sha256=sha(exe),through_launcher=through_launcher,launcher_sha256=sha(ROOT/'Halo2-Launcher') if through_launcher else None,selected_at_start=exe==ROOT/selected['runtime'],selection_changed=selection.read_bytes()!=selected_bytes,source_header_sha256=pins['include/halo2/simulation.hpp'],exit_code=code,actions=actions,elapsed_seconds=time.monotonic()-start,original_Xbox_pixels_compared=False,controlled_stop=controlled_stop,natural_process_shutdown_verified=controlled_stop is None and code==0 and capture.exists(),actual_view_samples={str(view):sum(row[1]==view for row in lens_samples) for view in (0,1)},actual_top_lens_queries=sum(row[2] for row in lens_samples if row[1]==0),actual_top_lens_flares=sum(row[3] for row in lens_samples if row[1]==0))
 result['actual_XTest_maneuver_configuration']=dict(level_boost=level_boost,maneuver_boost=maneuver_boost,maximum_turn_phases=turn_phases)
 if pipeline_reference:
  pipeline_reference['source_unchanged']=sha(reference)==pipeline_reference['sha256'];result['pipeline_reference']=pipeline_reference
 for name in ['placed','seated','finished','debugger-error','inputs-observed','publications-observed','inferior-exit']:
  if(scratch/(name+'.json')).exists():result[name]=json.loads((scratch/(name+'.json')).read_text())
 if capture.exists():
  with Image.open(capture) as im:im.convert('RGB').save(scratch/'two-player.png')
  p=scratch/'two-player.png';assert p.stat().st_size<1024*1024;target=out/'two-player.png';target.write_bytes(p.read_bytes());result['retained_reference_image']=dict(path=str(target),bytes=target.stat().st_size,sha256=sha(target))
 result['retained_flight_images']=[]
 for row in flight_images:
  path=row['path'];assert path.stat().st_size<1024*1024;target=out/path.name;shutil.copyfile(path,target)
  result['retained_flight_images'].append(dict(path=str(target),bytes=target.stat().st_size,sha256=sha(target),sample=row['sample'],rectangle=row['rectangle'],source='Actual private X11 game drawable; no runtime state or render result supplied'))
 result['natural_process_shutdown_verified']&=result.get('inferior-exit',{}).get('exit_code')==0
 result['gameplay_completed']=code==0 and fire and capture.exists() and all(result['actual_view_samples'].values()) and result.get('finished',{}).get('delivered_fire_steps',0)>0 and result.get('finished',{}).get('published_frames',0)>0 and (not native_capture or result['natural_process_shutdown_verified'])
 if pipeline_reference:result['gameplay_completed']&=pipeline_reference['source_unchanged']
 if audible:
  voices=result.get('finished',{}).get('audio_voices',[])
  from collections import Counter
  from probe_weapon_audio_delivery import classify_observation
  requests=Counter(v['sound'] for v in result.get('finished',{}).get('audio_requests',[]))
  accepted=Counter(v['sound'] for v in voices if v['engaged']==1 and v['voice']>0 and v['view']==0)
  observation={'requests':[dict(sound=f'{sound:08x}',kind=0,requests=count,accepted=accepted[sound]) for sound,count in sorted(requests.items())]}
  mode_passed,classes,mode_errors=classify_observation(maps/'coagulation.map',observation)
  result['sound_mode_observation']=observation;result['sound_mode_errors']=mode_errors
  result['sound_mode_policy']='Classic: actual prepared MCC snd! bit11 excludes Play Only In Remastered Mode'
  result['audio_completed']=mode_passed and not mode_errors and any(row['requests']>2 and not row['remastered_only'] for row in observation['requests'])
  result['gameplay_completed']&=result['audio_completed']
 if capture_audio:
  result['native_audio_output']=capture_audio.summary
  pid=str(result.get('placed',{}).get('process_pid',''))
  clients=result['native_audio_output']['clients'];consumed=result.get('finished',{}).get('audio_consumption',[])
  positive=result.get('finished',{}).get('positive_routes',{})
  fire_consumed=[v for v in consumed if not v['object_loop'] and str(v['voice']) in positive]
  vehicle_consumed=[v for v in consumed if v['object_loop'] and v['owner']==result.get('seated',{}).get('parent') and v['kind']==2 and str(v['voice']) in positive]
  from validate_mcc_weapon_vehicle_sound_resources import Bank,merge
  local=Bank((maps/'coagulation.map').resolve());shared=Bank((maps/'shared.map').resolve())
  try:names={datum:tag['name'] for datum,(_,tag) in merge(local,shared).items() if tag['group']=='snd!'}
  finally:local.close();shared.close()
  boost_sides=0;contrail_sides=0
  for event in vehicle_consumed:
   event['name']=names.get(event['sound'],'')
   if '\\banshee_boost_left\\' in event['name']:boost_sides|=1
   if '\\banshee_boost_right\\' in event['name']:boost_sides|=2
   if '\\banshee_contrail_left\\' in event['name']:contrail_sides|=1
   if '\\banshee_contrail_right\\' in event['name']:contrail_sides|=2
  result['actual_vehicle_consumed_loop_resources']=vehicle_consumed
  result['actual_boost_consumed_sides']=boost_sides
  result['actual_contrail_consumed_sides']=contrail_sides
  result['actual_vehicle_loop_consumed_voices']=len(vehicle_consumed)
  result['remaining_object_audio_gaps']=sorted(set(re.findall(r'^Object loop [^\n]+',raw_log,re.M)))
  result['actual_native_audio_consumption_verified']=bool(len(fire_consumed)>2 and clients and
      all(str(c['process_id'])==pid for c in clients) and result['native_audio_output']['nonzero_samples']>0 and
      result['native_audio_output']['sink_removed'] and result['native_audio_output']['default_sink_preserved'])
  result['gameplay_completed']&=result['actual_native_audio_consumption_verified'] and len(vehicle_consumed)>=2 and boost_sides==3 and result.get('finished',{}).get('delivered_boost_steps',0)>0 and not result['remaining_object_audio_gaps']
  if level_boost or maneuver_boost:result['gameplay_completed']&=contrail_sides==3
 result['mcc_map_sha256']=map_pin;result['mcc_map_unchanged']=sha(maps/'coagulation.map')==map_pin
 result['gameplay_completed']&=result['mcc_map_unchanged']
 result['probe_tool_sha256']=sha(Path(__file__))
 if capture_audio:result['native_audio_capture_tool_sha256']=sha(ROOT/'tools/native_gameplay_audio_capture.py')
 (out/'report.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps({k:v for k,v in result.items() if k not in ('scope','finished')}),flush=True);return 0 if result['gameplay_completed'] else 1
if __name__=='__main__':raise SystemExit(main())
