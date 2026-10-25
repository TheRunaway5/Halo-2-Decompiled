#!/usr/bin/env python3
"""Verify selected-launcher replica lenses in actual two-player top-KBM gameplay.

The separately built real authority owns a declared B95D0 approach/airborne
setup and typed seat request. The actual selected launcher owns the joined
game, XTest inputs, received projectiles and every GPU query/draw/result.
No projectile, admission, visibility, readiness or GPU result is supplied.
"""
import argparse,hashlib,json,math,os,re,shutil,subprocess,time
from pathlib import Path
from PIL import Image
from build_launcher_engine import source_hashes
from mcc_content import directory
from probe_installed_banshee_gameplay_20261022 import Inputs
from run_combat_presentation_audit_20261022 import keep_log
from run_test import stop_process_tree
from validate_weapon_runtime_tu15 import private_display
ROOT=Path(__file__).resolve().parents[1]
BASE=ROOT/'analysis/combat-presentation-audit-20261022'
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def load(p):return json.loads(p.read_text()) if p.is_file() else {}
def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--source',type=Path,required=True);ap.add_argument('--host-binary',type=Path,required=True);ap.add_argument('--qualification',type=Path,required=True);ap.add_argument('--label',required=True);ap.add_argument('--case',choices=['banshee','rocket','needler','spark'],default='banshee');a=ap.parse_args()
    owner=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True);work=owner/('installed-joined-'+str(time.time_ns()));work.mkdir()
    source=a.source.resolve(strict=True);host_exe=a.host_binary.resolve(strict=True)
    selected_path=ROOT/'gameplay-runtime-selection.json';saved=selected_path.read_bytes();selected=json.loads(saved);exe=ROOT/selected['runtime']
    assert sha(exe)==selected['runtime_sha256']
    pins=json.loads((exe.parent/'source-hashes.json').read_text());assert source_hashes(source)==pins,'Authority fixture must use the exact selected production snapshot'
    admission=load(BASE/'repair-followup-20261023/diagnostic-admission.json')
    assert admission['build_completed'] and admission['executables'][host_exe.name]==sha(host_exe)
    assert sha(source/'tools/audit_joined_effects_20261022.cpp')==admission['joined_fixture_sha256']
    qualification=load(a.qualification.resolve(strict=True));assert qualification['passed'] and qualification['runtime_sha256']==sha(exe) and qualification['runtime']==selected['runtime']
    out=BASE/'repair-installed-joined-lens'/a.label;out.mkdir(parents=True,exist_ok=False)
    maps=directory(ROOT/selected['cache'])/'h2_maps_win64_dx11';map_name='elongation.map' if a.case=='spark' else 'coagulation.map'
    map_pin=sha(maps/map_name)
    if map_name in selected['maps']:assert map_pin==selected['maps'][map_name]
    host_report=work/'authority.json';host_log=work/'authority.log';client_log=work/'client.log';frame=work/'final.ppm'
    setup=BASE/'repair-attachment-material/banshee-setup.json'
    host_command=[str(host_exe),str(maps/map_name),str(host_report),str(work/'unused-host-frame.ppm')]
    if a.case=='banshee':host_command.append(str(setup))
    start=time.monotonic();client=None;host=None;client_code=124;host_code=124;actions=[];references=[];progress=[];probe_error=None;spark_mouse_events=0
    with private_display(work) as display,host_log.open('w') as hs,client_log.open('w') as cs:
        env=dict(os.environ,DISPLAY=display,SDL_VIDEODRIVER='x11',SDL_AUDIODRIVER='dummy',XDG_CACHE_HOME=str(work/'graphics-cache'))
        pipeline=None
        if reference:=os.environ.get('HALO2_GAMEPLAY_PIPELINE_REFERENCE'):
            reference=Path(reference).resolve(strict=True);assert reference.stat().st_size<=64*1024*1024
            private=work/'pipeline';private.mkdir();copy=private/reference.name;shutil.copyfile(reference,copy);assert sha(copy)==sha(reference)
            env['HALO2_PIPELINE_CACHE_DIRECTORY']=str(private);pipeline=dict(path=str(reference),sha256=sha(reference),bytes=copy.stat().st_size)
        host_env=dict(env,HALO2_JOINED_SERVER_ONLY='1',HALO2_JOINED_BANSHEE_DECLARED_SEAT='1')
        if a.case=='rocket':host_env['HALO2_JOINED_ROCKET']='1'
        if a.case=='needler':host_env['HALO2_JOINED_NEEDLER']='1'
        if a.case=='spark':host_env['HALO2_JOINED_MAP_SPARK']='1'
        host=subprocess.Popen(host_command,env=host_env,stdout=hs,stderr=subprocess.STDOUT,start_new_session=True)
        try:
            ready=Path(str(host_report)+'.ready.json')
            while not ready.exists() and host.poll() is None and time.monotonic()-start<120:time.sleep(.05)
            assert ready.exists(),'Actual authority listener did not become ready'
            address=load(ready)['join'];assert address.startswith('127.0.0.1:')
            command=[str(ROOT/'Halo2-Launcher'),'--multiplayer','--map',str(maps/map_name),'--shared-maps',str(maps),'--join',address,'--local-players','2','--frames','900','--mute','--viewport','800','600','--screenshot',str(frame)]
            client=subprocess.Popen(command,env=env,stdout=cs,stderr=subprocess.STDOUT,start_new_session=True)
            inputs=Inputs(display);firing=False;released=False;captures=set();base_fire=0;last_progress=-1;spark_mouse_events=0;spark_focused=False
            while client.poll() is None and host.poll() is None and time.monotonic()-start<600:
                observed=load(Path(str(host_report)+'.progress.json'))
                if observed and observed['tick']!=last_progress:progress.append(observed);last_progress=observed['tick']
                raw=client_log.read_text(errors='replace')
                if 'outside their qualification' in raw:
                    actions.append(dict(action='Launcher rejected this unqualified map; gameplay not reached'));break
                if a.case=='spark':
                    if observed.get('spark_setup_complete') and not spark_focused:
                        inputs.focus();inputs.button(True);time.sleep(.05);inputs.button(False);spark_focused=True
                        actions.append(dict(action='Actual XTest primary focus click/release enters SDL relative mouse capture',authority_tick=observed['tick']))
                    fresh=observed.get('spark_setup_complete') and (not actions or observed['tick']!=actions[-1].get('authority_tick'))
                    if fresh:
                        eye=observed['pilot_eye'];marker=observed['spark_marker'];delta=[b-a for a,b in zip(eye,marker)]
                        yaw=math.atan2(delta[1],delta[0]);pitch=math.atan2(delta[2],math.hypot(*delta[:2]))
                        dx=max(-80,min(80,round(-math.remainder(yaw-observed['pilot_yaw'],2*math.pi)/.0025)))
                        dy=max(-80,min(80,round(-(pitch-observed['pilot_pitch'])/.0025)))
                        if dx or dy:
                            try:inputs.focus();inputs.move(dx,dy);spark_mouse_events+=1
                            except AssertionError:
                                if client.poll() is None:raise
                                client_code=client.returncode;actions.append(dict(action='Actual client window retired at natural process exit'));break
                        actions.append(dict(action='Actual XTest mouse aim using current authority pose and original ordinary-eye arithmetic',authority_tick=observed['tick'],x=dx,y=dy))
                    visible=[tuple(map(int,x)) for x in re.findall(r'Native replica lens queries: frame=(\d+) view=0 issued=(\d+) flares=(\d+)',raw)]
                    if visible and visible[-1][2]>0 and len(references)<3 and (not references or visible[-1][0]>references[-1]['frame']+10):
                        p=out/('spark-'+str(len(references)+1)+'.png');rectangle=inputs.capture(p,expected_size=(800,600));assert p.stat().st_size<1024*1024
                        references.append(dict(path=str(p),bytes=p.stat().st_size,sha256=sha(p),rectangle=rectangle,frame=visible[-1][0],reason='Required selected-launcher scenery flare gameplay reference'))
                    time.sleep(.05);continue
                ready=observed.get('airborne_setup_complete') if a.case=='banshee' else observed.get('connected')
                if not firing and ready and 'Native replica lens queries:' in raw:
                    inputs.focus();inputs.button(True)
                    if a.case=='banshee':inputs.key('w',True);inputs.key('space',True)
                    time.sleep(.1);inputs.move(0,-80 if a.case=='banshee' else 120 if a.case=='needler' else 20)
                    base_fire=observed.get('received_fire_steps',0);firing=True;actions.append(dict(action='Actual joined top-KBM XTest primary/flight/climb' if a.case=='banshee' else 'Actual joined top-KBM XTest '+a.case+' primary and downward look',authority_tick=observed['tick']))
                delivered=observed.get('received_fire_steps',0)-base_fire
                if firing:
                    for marker in (30,60,90):
                        if delivered>=marker and marker not in captures:
                            captures.add(marker);p=out/('firing-'+str(marker)+'.png')
                            rectangle=inputs.capture(p,expected_size=(800,600));assert p.stat().st_size<1024*1024
                            references.append(dict(path=str(p),bytes=p.stat().st_size,sha256=sha(p),rectangle=rectangle,received_fire_steps=delivered,reason='Required actual selected-launcher joined gameplay reference'))
                    if delivered>=120 and not released:
                        inputs.button(False);inputs.key('w',False);inputs.key('space',False);released=True
                        actions.append(dict(action='Actual joined XTest primary/flight release after authority received fire',authority_tick=observed['tick'],received_fire_steps=delivered))
                time.sleep(.05)
            if client.poll() is not None:client_code=client.returncode
        except Exception as error:
            probe_error=str(error) or type(error).__name__
            if client is not None and client.poll() is not None:client_code=client.returncode
            actions.append(dict(action='Probe failed; actual runtime diagnostics retained',error=probe_error))
        finally:
            if 'inputs' in locals():inputs.button(False);inputs.key('w',False);inputs.key('space',False)
            if client is not None:stop_process_tree(client)
            Path(str(host_report)+'.stop').write_text('The finite actual launcher client has finished.\n')
            try:host_code=host.wait(timeout=20)
            except subprocess.TimeoutExpired:pass
            stop_process_tree(host)
    raw=client_log.read_text(errors='replace');keep_log(client_log,out/'client.log');keep_log(host_log,out/'authority.log')
    views=[tuple(map(int,x)) for x in re.findall(r'Native replica lens queries: frame=(\d+) view=(\d+) issued=(\d+) flares=(\d+)',raw)]
    completions=[tuple(map(int,x)) for x in re.findall(r' completed=(\d+):(\d+):(\d+)',raw)]
    authority=load(host_report);(out/'authority.json').write_text(json.dumps(authority,indent=2)+'\n')
    (out/'authority-progress.json').write_text(json.dumps(progress,indent=2)+'\n')
    result=dict(scope=__doc__,runtime=selected['runtime'],runtime_sha256=sha(exe),launcher_sha256=sha(ROOT/'Halo2-Launcher'),authority_executable_sha256=sha(host_exe),authority_source_matches_runtime=True,
        client_command=command if 'command' in locals() else None,host_command=host_command,client_exit_code=client_code,authority_exit_code=host_code,actions=actions,
        selection_unchanged=selected_path.read_bytes()==saved,map_unchanged=sha(maps/map_name)==map_pin,query_draws=sum(x[2] for x in views),flare_draws=sum(x[3] for x in views),actual_view_samples={str(i):sum(x[1]==i for x in views) for i in (0,1)},
        positive_actual_GPU_completions=sum(x[2]>0 for x in completions),actual_GPU_completions=len(completions),authority_received_fire_steps=max((x['received_fire_steps'] for x in progress),default=0),retained_references=references,original_Xbox_pixels_compared=False,
        projectile_admission_readiness_or_GPU_result_supplied=False,declared_setup='Original B95D0 approach/airborne placement and typed live-vehicle/seat input on the actual authority; subsequent joined firing/flight are actual top-KBM XTest/UDP inputs' if a.case=='banshee' else 'Original B95D0 collision-supported scenery approach; actual XTest aiming uses observed source pose, authored marker and direct-CLI default KBM settings' if a.case=='spark' else 'Actual authored multiplayer starting-weapon choice and ordinary spawn; joined primary/look are actual top-KBM XTest/UDP inputs',elapsed_seconds=time.monotonic()-start,case=a.case)
    result['query_draws_per_view']={str(i):sum(x[2] for x in views if x[1]==i) for i in (0,1)}
    result['flare_draws_per_view']={str(i):sum(x[3] for x in views if x[1]==i) for i in (0,1)}
    result['actual_authority_detonations']=len(authority.get('authority',{}).get('detonations',[]))
    result['actual_XTest_spark_mouse_events']=spark_mouse_events
    result['probe_error']=probe_error
    result['actual_encounter_completed']=client_code==0 and host_code==0 and result['selection_unchanged'] and result['map_unchanged'] and all(result['actual_view_samples'].values()) and (spark_mouse_events>0 if a.case=='spark' else result['authority_received_fire_steps']>90) and len(references)==3
    if pipeline:result['pipeline_reference']=dict(**pipeline,source_unchanged=sha(reference)==pipeline['sha256'])
    result['passed']=not probe_error and result['actual_encounter_completed'] and result['query_draws_per_view']['0']>0 and result['flare_draws_per_view']['0']>0 and result['positive_actual_GPU_completions']>0 and (a.case not in ('rocket','needler') or result['actual_authority_detonations']>0)
    shutil.rmtree(work);result['private_work_removed']=not work.exists()
    (out/'report.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps({k:v for k,v in result.items() if k not in ['scope','client_command','host_command','retained_references']}),flush=True)
    return 0 if result['passed'] else 1
if __name__=='__main__':raise SystemExit(main())
