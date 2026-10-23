#!/usr/bin/env python3
"""Observe an actual finite Coagulation SMG dirt-impact history under GDB.

Run through tools/run_test.py with disk-backed scratch. All generated debugger,
input, savegame, cache and framebuffer files remain in HALO2_TEST_SCRATCH.
"""
import argparse
import json
import os
from pathlib import Path
import shutil
import subprocess
from mcc_content import MccCache
from validate_weapon_runtime_tu15 import private_display

GDB = r'''
set pagination off
set confirm off
set print thread-events off
set debuginfod enabled off
handle SIGPIPE nostop noprint pass
python
import gdb,json,struct,os
rows=[]
bitmaps=json.load(open(os.path.join(os.environ['HALO2_TEST_SCRATCH'],'bitmaps.json')))
def reg(name):return int(gdb.parse_and_eval('$'+name))
def raw(at,n):return gdb.selected_inferior().read_memory(at,n).tobytes()
class End(gdb.FinishBreakpoint):
 def __init__(self,p,row):
  super().__init__(gdb.newest_frame(),internal=True);self.p=p;self.row=row
 def stop(self):
  self.row['argb']=struct.unpack('<I',raw(self.p+32,4))[0];rows.append(self.row);return False
class Sprite(gdb.Breakpoint):
 def stop(self):
  d=reg('rsi');p=reg('rdx');flags,bitmap=struct.unpack('<II',raw(d,8))
  if str(bitmap) in bitmaps and len(rows)<500:
   row={'particle':bitmaps[str(bitmap)],'flags':flags,'seed':struct.unpack('<I',raw(p+72,4))[0],
        'color':struct.unpack('<4f',raw(p+108,16)),'position':struct.unpack('<3f',raw(p+76,12))}
   End(reg('rdi'),row)
  return False
Sprite('halo2::particle_sprite_submission(halo2::ParticleSpriteDefinition const&, halo2::ParticleSpriteInput const&, halo2::ShaderFunctionTables const&)')
end
run
python
open(os.path.join(os.environ['HALO2_TEST_SCRATCH'],'colors.json'),'w').write(json.dumps(rows,indent=2))
end
quit $_exitcode
'''

def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--runtime',type=Path,required=True)
    ap.add_argument('--map',type=Path,required=True)
    ap.add_argument('--report',type=Path,required=True)
    ap.add_argument('--timeout',type=float,default=600)
    ap.add_argument('--auxiliary-seed',type=int,default=0x12345678)
    args=ap.parse_args()
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve()
    maps=args.map.resolve().parent
    cache_directory=scratch/'cache'
    cache_directory.mkdir(exist_ok=True)
    for source in (Path.home()/'.cache/halo2-native/vulkan').glob('*.bin'):
        if source.is_file() and source.stat().st_size<=32*1024*1024:
            shutil.copyfile(source,cache_directory/source.name)
    c=MccCache(maps/'shared.map')
    bitmaps={}
    names={'dirt_burst','dirt_puff','dirt_burst_cone','impact_burst','impact_clumps',
           'impact_spray','smoke_soft_small_lit'}
    for tag in c.tags.values():
        if tag['group']=='prt3' and tag['offset'] is not None and tag['name'].split('\\')[-1] in names:
            for parameter in c.block(tag['offset']+0x14,40):
                if c.u16(parameter+4)==0:bitmaps[str(c.u32(parameter+12))]=tag['name']
    (scratch/'bitmaps.json').write_text(json.dumps(bitmaps))
    inputs=scratch/'smg.inputs'
    inputs.write_text(''.join(f'{tick} 0 {int(tick>=80)} 0 0 0 -0.65\n' for tick in range(96)))
    debugger=scratch/'colors.gdb';debugger.write_text(GDB)
    command=['gdb','-q','-batch','-x',str(debugger),'--args',str(args.runtime.resolve()),
        '--multiplayer','--map',str(args.map.resolve()),'--shared-maps',str(maps),
        '--local-players','1','--mp-starting-weapon','5','--mp-input-script',str(inputs),
        '--mp-capture-tick','96','--mp-reference-tick-cadence','--viewport','640','480',
        '--mp-reference-auxiliary-seed',str(args.auxiliary_seed),
        '--screenshot',str(scratch/'smg.ppm'),'--mute','--mp-source-object-world',str(scratch/'savegames')]
    with private_display(scratch) as display,(scratch/'runtime.log').open('w') as log:
        env=dict(os.environ,DISPLAY=display,SDL_VIDEODRIVER='x11',SDL_AUDIODRIVER='dummy',
            XDG_CACHE_HOME=str(scratch/'cache'),HALO2_PIPELINE_CACHE_DIRECTORY=str(scratch/'cache'))
        try:code=subprocess.run(command,env=env,stdout=log,stderr=subprocess.STDOUT,timeout=args.timeout).returncode
        except subprocess.TimeoutExpired:code=124
    rows=json.loads((scratch/'colors.json').read_text()) if (scratch/'colors.json').exists() else []
    def primary(row):
        channels=[(row['argb']>>shift)&255 for shift in (16,8,0)]
        return row['argb']>>24>20 and max(channels)>80 and min(channels)<max(channels)*.25
    bad=[row for row in rows if primary(row)]
    report={'runtime':str(args.runtime.resolve()),'map':str(args.map.resolve()),'command':command,
        'auxiliary_seed':args.auxiliary_seed,
        'exit_code':code,'particle_samples':len(rows),'primary_color_samples':len(bad),
        'samples':rows,'passed':code==0 and bool(rows) and not bad,'scratch':str(scratch)}
    args.report.parent.mkdir(parents=True,exist_ok=True)
    args.report.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({key:value for key,value in report.items() if key not in ('samples','command')}),flush=True)
    print('\n'.join((scratch/'runtime.log').read_text(errors='replace').splitlines()[-12:]),flush=True)
    return int(not report['passed'])

if __name__=='__main__':raise SystemExit(main())
