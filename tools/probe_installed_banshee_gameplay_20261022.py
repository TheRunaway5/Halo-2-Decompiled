#!/usr/bin/env python3
"""Run the selected production engine with a declared original-position setup.

Debugger breakpoints observe completed owners. One B95D0 player placement and
one seated-vehicle placement are declared fixture inputs. Entry, flight, fire,
source physics, lens queries and the final GPU frame remain actual gameplay.
No instruction body, admission, hit, lifetime or GPU result is supplied.
"""
import ctypes,hashlib,json,math,os,struct,subprocess,time
from pathlib import Path
from PIL import Image
from validate_weapon_runtime_tu15 import private_display
from run_combat_presentation_audit_20261022 import keep_log
ROOT=Path(__file__).resolve().parents[1]
GDB=r'''
set pagination off
set confirm off
set print thread-events off
set breakpoint pending on
python
import gdb,json,struct
from pathlib import Path
config=json.loads(Path(CONFIG).read_text());root=Path(config['scratch']);setup=config['setup'];symbols=config['symbols']
world=0;match=0;steps=0;placed=False;seated=False;observations=[];publications=[]
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
    (root/(name+'.json')).write_text(json.dumps(data));print('[COMBAT-INSTALLED] '+name+' '+json.dumps(data),flush=True)
class WorldReturn(gdb.FinishBreakpoint):
    def stop(self):
        global world
        world=int(gdb.parse_and_eval('$rax'));return False
class WorldEntry(gdb.Breakpoint):
    def stop(self):
        self.enabled=False;WorldReturn(gdb.newest_frame(),internal=True);return False
class Step(gdb.Breakpoint):
    def stop(self):
        global match,steps,placed,seated
        steps+=1
        if not world or steps<20:return False
        try:
            if seated:
                inputs=int(gdb.parse_and_eval('$rsi'))
                fire=bool(bytes(gdb.selected_inferior().read_memory(inputs+setup['controls_fire_offset'],1))[0])
                observations.append(dict(step=steps,fire=fire))
                if len(observations)%30==0:record('inputs-observed',dict(samples=len(observations),delivered_fire_steps=sum(o['fire'] for o in observations)))
                return False
            match=int(gdb.parse_and_eval('$rdi'));unit=call_pointer('unit',match,0)
            datum=read32(unit+setup['unit_source_datum_offset'])
            if datum==0xffffffff:return False
            if not placed:
                place(datum,setup['position']);placed=True
                record('placed',dict(unit=datum,position=setup['position'],yaw=readfloat(unit+setup['unit_yaw_offset']),step=steps));return False
            parent=read32(unit+setup['unit_parent_offset'])
            if parent!=0xffffffff and not seated:
                place(parent,setup['airborne']);seated=True
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
end
run
python
record('finished',dict(steps=steps,placed=placed,seated=seated,delivered_fire_steps=sum(o['fire'] for o in observations),
    published_frames=sum(p['count']>0 for p in publications),maximum_live_projectiles=max((p['count'] for p in publications),default=0)))
end
'''
class Inputs:
 def __init__(self,display):
  self.x=ctypes.CDLL('libX11.so.6');self.t=ctypes.CDLL('libXtst.so.6')
  self.x.XOpenDisplay.argtypes=[ctypes.c_char_p];self.x.XOpenDisplay.restype=ctypes.c_void_p;self.d=self.x.XOpenDisplay(display.encode());assert self.d
  self.x.XDefaultRootWindow.argtypes=[ctypes.c_void_p];self.x.XDefaultRootWindow.restype=ctypes.c_ulong
  self.x.XQueryTree.argtypes=[ctypes.c_void_p,ctypes.c_ulong,ctypes.POINTER(ctypes.c_ulong),ctypes.POINTER(ctypes.c_ulong),ctypes.POINTER(ctypes.POINTER(ctypes.c_ulong)),ctypes.POINTER(ctypes.c_uint)]
  self.x.XSetInputFocus.argtypes=[ctypes.c_void_p,ctypes.c_ulong,ctypes.c_int,ctypes.c_ulong]
  self.x.XFlush.argtypes=[ctypes.c_void_p];self.x.XFree.argtypes=[ctypes.c_void_p]
  self.x.XStringToKeysym.argtypes=[ctypes.c_char_p];self.x.XStringToKeysym.restype=ctypes.c_ulong
  self.x.XKeysymToKeycode.argtypes=[ctypes.c_void_p,ctypes.c_ulong];self.x.XKeysymToKeycode.restype=ctypes.c_uint
  self.t.XTestFakeKeyEvent.argtypes=[ctypes.c_void_p,ctypes.c_uint,ctypes.c_int,ctypes.c_ulong]
  self.t.XTestFakeButtonEvent.argtypes=[ctypes.c_void_p,ctypes.c_uint,ctypes.c_int,ctypes.c_ulong]
  self.t.XTestFakeRelativeMotionEvent.argtypes=[ctypes.c_void_p,ctypes.c_int,ctypes.c_int,ctypes.c_ulong]
 def focus(self):
  root=self.x.XDefaultRootWindow(self.d);r=ctypes.c_ulong();p=ctypes.c_ulong();children=ctypes.POINTER(ctypes.c_ulong)();count=ctypes.c_uint()
  assert self.x.XQueryTree(self.d,root,ctypes.byref(r),ctypes.byref(p),ctypes.byref(children),ctypes.byref(count)) and count.value
  window=children[count.value-1];self.x.XSetInputFocus(self.d,window,1,0);self.x.XFree(children);self.x.XFlush(self.d)
 def key(self,name,down):self.t.XTestFakeKeyEvent(self.d,self.x.XKeysymToKeycode(self.d,self.x.XStringToKeysym(name.encode())),down,0);self.x.XFlush(self.d)
 def button(self,down):self.t.XTestFakeButtonEvent(self.d,1,down,0);self.x.XFlush(self.d)
 def move(self,x,y):self.t.XTestFakeRelativeMotionEvent(self.d,x,y,0);self.x.XFlush(self.d)
def main():
 owner=Path(os.environ['HALO2_TEST_SCRATCH']);scratch=owner/('installed-banshee-'+str(time.time_ns()));scratch.mkdir(exist_ok=False);out=ROOT/'analysis/combat-presentation-audit-20261022/repair-installed-banshee'/os.environ.get('HALO2_COMBAT_INSTALLED_LABEL','v2');out.mkdir(parents=True,exist_ok=True)
 selection=ROOT/'gameplay-runtime-selection.json';selected_bytes=selection.read_bytes();selected=json.loads(selected_bytes);exe=ROOT/os.environ.get('HALO2_COMBAT_INSTALLED_RUNTIME',selected['runtime']);sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest();assert sha(exe)==os.environ.get('HALO2_COMBAT_INSTALLED_SHA256',selected['runtime_sha256'])
 pins=json.loads((exe.parent/'source-hashes.json').read_text());assert pins['include/halo2/simulation.hpp']==sha(ROOT/'include/halo2/simulation.hpp')
 setup=json.loads((ROOT/'analysis/combat-presentation-audit-20261022/repair-attachment-material/banshee-setup.json').read_text())
 table=subprocess.check_output(['nm','--defined-only',str(exe)],text=True).splitlines();symbols={}
 for key,fragment in [('world','MultiplayerObjectRuntime10tu15_world'),('unit','MultiplayerMatch4unitEj'),('step','MultiplayerMatch4step'),('position','PhysicsLiveWorldTu1519set_object_position'),('published','NativeWeaponProjectileScene11synchronize')]:
  found=[line.split()[2] for line in table if len(line.split())==3 and line.split()[1]=='T' and fragment in line.split()[2]];assert len(found)==1,(key,found);symbols[key]=found[0]
 config=scratch/'config.json';config.write_text(json.dumps(dict(scratch=str(scratch),setup=setup,symbols=symbols)));script=scratch/'probe.gdb';script.write_text(GDB.replace('CONFIG',repr(str(config))))
 from mcc_content import directory
 maps=directory(ROOT/selected['cache'])/'h2_maps_win64_dx11';capture=scratch/'final.ppm';log=scratch/'runtime.log'
 command=['gdb','-q','-nx','-batch','-x',str(script),'--args',str(exe),'--multiplayer','--map',str(maps/'coagulation.map'),'--shared-maps',str(maps),'--local-players','2','--frames','120','--mute','--viewport','800','450','--screenshot',str(capture)]
 actions=[];start=time.monotonic();code=124
 with private_display(scratch) as display,log.open('w') as stream:
  env=dict(os.environ,DISPLAY=display,SDL_VIDEODRIVER='x11',SDL_AUDIODRIVER='dummy',XDG_CACHE_HOME=str(owner/'graphics-cache'))
  process=subprocess.Popen(command,env=env,stdout=stream,stderr=subprocess.STDOUT);input_=Inputs(display);turned=False;fire=False
  try:
   while process.poll() is None and time.monotonic()-start<600:
    if (scratch/'debugger-error.json').exists():break
    if not turned and (scratch/'placed.json').exists():
     placed=json.loads((scratch/'placed.json').read_text());input_.focus();input_.button(True);time.sleep(.15);input_.button(False)
     delta=(setup['yaw']-placed['yaw']+math.pi)%(2*math.pi)-math.pi;remaining=round(-delta/.0025)
     while remaining:
      n=max(-35,min(35,remaining));input_.move(n,0);remaining-=n;time.sleep(.12)
     input_.key('e',True);turned=True;actions.append(dict(action='actual_XTest_mouse_look_and_E_hold',delta=delta,at=time.monotonic()-start))
    if not fire and (scratch/'seated.json').exists():
     input_.key('e',False);input_.key('w',True);input_.key('space',True);input_.move(0,-80);input_.button(True);fire=True;actions.append(dict(action='actual_XTest_flight_and_continuous_primary_fire',at=time.monotonic()-start))
    time.sleep(.05)
   if process.poll() is None:process.terminate()
   code=process.wait(timeout=15)
  finally:
   for key in ['e','w','space']:input_.key(key,False)
   input_.button(False)
   if process.poll() is None:process.kill();process.wait()
 keep_log(log,out/'runtime.log');result=dict(scope=__doc__,command=command,runtime_sha256=sha(exe),selected_at_start=exe==ROOT/selected['runtime'],selection_changed=selection.read_bytes()!=selected_bytes,source_header_sha256=pins['include/halo2/simulation.hpp'],exit_code=code,actions=actions,elapsed_seconds=time.monotonic()-start,original_Xbox_pixels_compared=False)
 for name in ['placed','seated','finished','debugger-error','inputs-observed','publications-observed']:
  if(scratch/(name+'.json')).exists():result[name]=json.loads((scratch/(name+'.json')).read_text())
 if capture.exists():
  with Image.open(capture) as im:im.convert('RGB').crop((0,0,800,225)).save(scratch/'pilot.png')
  p=scratch/'pilot.png';assert p.stat().st_size<1024*1024;target=out/'pilot.png';target.write_bytes(p.read_bytes());result['retained_reference_image']=dict(path=str(target),bytes=target.stat().st_size,sha256=sha(target))
 result['gameplay_completed']=code==0 and fire and capture.exists() and result.get('finished',{}).get('delivered_fire_steps',0)>0 and result.get('finished',{}).get('published_frames',0)>0
 (out/'report.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result),flush=True);return 0 if result['gameplay_completed'] else 1
if __name__=='__main__':raise SystemExit(main())
