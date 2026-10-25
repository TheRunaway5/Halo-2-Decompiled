#!/usr/bin/env python3
"""Observe actual KBM zoom in the launcher-selected two-player runtime.

The debugger reads delivered input only. No source camera, zoom selector,
foreground admission or rendered result is supplied. Keep a bounded screenshot
as required visual reference; no binary or raw frame survives its test owner.
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
set debuginfod enabled off
set non-stop on
python
import gdb,json
from pathlib import Path
c=json.loads(Path(CONFIG).read_text());out=Path(c['scratch']);steps=0;zooms=[]
gdb.events.exited.connect(lambda event:(out/'inferior-exit.json').write_text(json.dumps(dict(exit_code=getattr(event,'exit_code',None)))))
class Step(gdb.Breakpoint):
 def stop(self):
  global steps
  steps+=1;pointer=int(gdb.parse_and_eval('$rsi'))
  zoom=int.from_bytes(gdb.selected_inferior().read_memory(pointer+c['zoom_offset'],2),'little',signed=True)
  zooms.append(zoom)
  if steps%5==0:
   p=out/'inputs.tmp';p.write_text(json.dumps(dict(steps=steps,zoom=zoom,zooms=zooms)));p.replace(out/'inputs.json')
  return False
Step(c['step'],internal=True)
end
run
python
(out/'inputs.json').write_text(json.dumps(dict(steps=steps,zooms=zooms)))
end
'''

def main():
    owner=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    work=owner/'scope';work.mkdir();out=ROOT/'analysis/combat-presentation-audit-20261022/repair-installed-scope'/os.environ.get('HALO2_SCOPE_PROBE_LABEL','run')
    out.mkdir(parents=True,exist_ok=True)
    selection=ROOT/'gameplay-runtime-selection.json';raw_selection=selection.read_bytes();selected=json.loads(raw_selection)
    exe=ROOT/selected['runtime'];assert sha(exe)==selected['runtime_sha256']
    pins=json.loads((exe.parent/'source-hashes.json').read_text())
    from installed_gameplay_layout import compile_installed_layout,installed_layout_source
    maps=directory(ROOT/selected['cache'])/'h2_maps_win64_dx11'
    map_pin=sha(maps/'coagulation.map');assert map_pin==selected['maps']['coagulation.map']
    local=Bank((maps/'coagulation.map').resolve());shared=Bank((maps/'shared.map').resolve())
    try:
        tags=merge(local,shared);gb,gt=tags[local.u(local.index+16)]
        mb,mt=tags[gb.u(gt['offset']+0x16c)];runtime=mb.block(mt['offset']+8,0x568);assert len(runtime)==1
        refs=mb.block(runtime[0]+0x68,8);permutation=(3,0,1,6,7,5,2,4,12,9,10,13,11,15,14,8,16,17)
        choices=[dict(option=i+3,datum=mb.u(refs[n]+4),name=tags[mb.u(refs[n]+4)][1]['name']) for i,n in enumerate(permutation) if mb.u(refs[n]+4)!=0xffffffff]
        sniper=[r for r in choices if r['name'].endswith('\\sniper_rifle\\sniper_rifle')];assert len(sniper)==1
        sniper=sniper[0]
    finally:local.close();shared.close()
    layout=work/'layout.cpp';layout.write_text('#include "halo2/simulation.hpp"\n#include <iostream>\nint main(){std::cout<<offsetof(halo2::Controls,zoom);}\n')
    raw_layout,layout_provenance=compile_installed_layout(installed_layout_source(),pins,
        ('include/halo2/simulation.hpp',),layout,work)
    offset=int(raw_layout)
    (out/'layout-provenance.json').write_text(json.dumps(layout_provenance,indent=2)+'\n')
    table=subprocess.check_output(['nm','--defined-only',str(exe)],text=True).splitlines()
    steps=[l.split()[2] for l in table if len(l.split())==3 and l.split()[1]=='T' and 'MultiplayerMatch4step' in l.split()[2]];assert len(steps)==1
    config=work/'config.json';config.write_text(json.dumps(dict(scratch=str(work),step=steps[0],zoom_offset=offset)))
    script=work/'probe.gdb';script.write_text(GDB.replace('CONFIG',repr(str(config))))
    frame=work/'frame.ppm';log=work/'runtime.log'
    command=['gdb','-q','-nx','-batch','-x',str(script),'--args',str(ROOT/'Halo2-Launcher'),
        '--multiplayer','--map',str(maps/'coagulation.map'),'--shared-maps',str(maps),
        '--local-players','2','--mp-starting-weapon',str(sniper['option']),
        '--frames','180','--mute','--viewport','800','600','--screenshot',str(frame)]
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
    with private_display(work) as display,log.open('w') as stream:
        env=dict(os.environ,DISPLAY=display,SDL_VIDEODRIVER='x11',SDL_AUDIODRIVER='dummy',XDG_CACHE_HOME=str(owner/'graphics-cache'))
        if pipeline_reference:env['HALO2_PIPELINE_CACHE_DIRECTORY']=str(private)
        process=subprocess.Popen(command,env=env,stdout=stream,stderr=subprocess.STDOUT,start_new_session=True)
        try:
            inputs=Inputs(display);zoomed=False
            while process.poll() is None and time.monotonic()-started<int(os.environ.get('HALO2_SCOPE_PROBE_DEADLINE','600')):
                observed=json.loads((work/'inputs.json').read_text()) if (work/'inputs.json').exists() else {}
                if not zoomed and observed.get('steps',0)>=25:
                    inputs.focus();inputs.button(True);time.sleep(.1);inputs.button(False)
                    time.sleep(.2);inputs.t.XTestFakeButtonEvent(inputs.d,3,True,0);inputs.x.XFlush(inputs.d)
                    actions.append(dict(action='actual_XTest_right_mouse_zoom_hold',step=observed['steps']));zoomed=True
                if frame.exists() and frame.stat().st_size>=800*600*3:
                    try:
                        with Image.open(frame) as im:im.load();complete=im.size==(800,600)
                    except OSError:pass
                time.sleep(.05)
            if process.poll() is not None:code=process.returncode
        finally:
            inputs.t.XTestFakeButtonEvent(inputs.d,3,False,0);inputs.x.XFlush(inputs.d)
            stop_process_tree(process)
    keep_log(log,out/'runtime.log')
    observed=json.loads((work/'inputs.json').read_text()) if (work/'inputs.json').exists() else {}
    exited=json.loads((work/'inferior-exit.json').read_text()) if (work/'inferior-exit.json').exists() else {}
    receipt=dict(scope=__doc__,command=command,runtime_sha256=sha(exe),launcher_sha256=sha(ROOT/'Halo2-Launcher'),
        starting_weapon=sniper,actions=actions,delivered=observed,exit_code=code,
        complete_frame=complete,selection_unchanged=selection.read_bytes()==raw_selection,
        runtime_unchanged=sha(exe)==selected['runtime_sha256'],inferior_exit=exited,
        natural_shutdown_verified=code==0 and exited.get('exit_code')==0,
        original_Xbox_framebuffer_compared=False,visual_review_pending=True,gdb_non_stop=True)
    if complete:
        with Image.open(frame) as im:im.save(out/'two-player.png')
        receipt['retained_reference_image']=dict(path=str(out/'two-player.png'),bytes=(out/'two-player.png').stat().st_size,sha256=sha(out/'two-player.png'))
    if pipeline_reference:
        pipeline_reference['source_unchanged']=sha(reference)==pin
        receipt['pipeline_reference']=pipeline_reference
    receipt['map_unchanged']=sha(maps/'coagulation.map')==map_pin
    receipt['zoom_delivery_completed']=complete and any(z>=0 for z in observed.get('zooms',[])) and receipt['runtime_unchanged'] and receipt['selection_unchanged'] and receipt['natural_shutdown_verified'] and receipt['map_unchanged'] and (pipeline_reference is None or pipeline_reference['source_unchanged'])
    (out/'report.json').write_text(json.dumps(receipt,indent=2)+'\n')
    print(json.dumps({k:v for k,v in receipt.items() if k not in ('delivered','scope')}),flush=True)
    return 0 if receipt['zoom_delivery_completed'] else 1

if __name__=='__main__':raise SystemExit(main())
