#!/usr/bin/env python3
"""Observe a placed sound-scenery object's real launcher audio in two views.

The declared approach comes from the retained source-world fixture. Resource
membership is independently read from the selected map's authored attachment
closure. No sound request, admission, gain, readiness or voice is supplied.
"""
import contextlib,json,os,re,shlex,shutil,struct,subprocess,time
from pathlib import Path
from PIL import Image
from audit_weapon_vehicle_sound_presence_tu15 import references
from validate_mcc_weapon_vehicle_sound_resources import Bank,merge,schema
from mcc_content import directory
from native_gameplay_audio_capture import NativeGameplayAudioCapture
from probe_installed_teleporter_20261023 import GDB,sha
from validate_weapon_runtime_tu15 import private_display
from run_combat_presentation_audit_20261022 import keep_log
from run_test import stop_process_tree

ROOT=Path(__file__).resolve().parents[1]
BASE=ROOT/'analysis/combat-presentation-audit-20261022'

def main():
    owner=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    work=owner/'object-ambience';work.mkdir()
    label=os.environ.get('HALO2_OBJECT_AMBIENCE_LABEL','extra-rocks')
    assert re.fullmatch(r'[a-zA-Z0-9_-]+',label)
    out=BASE/'repair-installed-object-ambience'/label;out.mkdir(parents=True,exist_ok=True)
    selection=ROOT/'gameplay-runtime-selection.json';selected_bytes=selection.read_bytes();selected=json.loads(selected_bytes)
    exe=ROOT/selected['runtime'];assert sha(exe)==selected['runtime_sha256']
    pins=json.loads((exe.parent/'source-hashes.json').read_text())
    from installed_gameplay_layout import compile_installed_layout,installed_layout_source
    layout_source=installed_layout_source()
    audio=json.loads((BASE/'repair-installed-banshee/native-audio-observation-layout.json').read_text())
    assert pins['include/halo2/sdl_sound_platform.hpp']==audio['header_sha256']
    maps=directory(ROOT/selected['cache'])/'h2_maps_win64_dx11'
    case=os.environ.get('HALO2_OBJECT_AMBIENCE_CASE','extra-rocks')
    cases={'extra-rocks':('coagulation.map','runtime-56/extra-rocks-native-object-loop.json','extra_rocks',600),
        'lockout-green-hum':('lockout.map','runtime-86/green-hum-native-object-loop.json','green_hum',180),
        'lockout-wind':('lockout.map','runtime-88/sound-scenery-native-object-loop.json','particulate_wind',600)}
    map_name,setup_relative,suffix,frames=cases[case]
    map_pin=sha(maps/map_name);assert map_pin==selected['maps'][map_name]
    setup_path=BASE/'repair-scenery-audio'/setup_relative
    setup=json.loads(setup_path.read_text());assert setup['passed']
    local=Bank((maps/map_name).resolve());shared=Bank((maps/'shared.map').resolve())
    try:
        tags=merge(local,shared)
        candidates=[d for d,(_,t) in tags.items() if t['group']=='ssce' and t['name'].endswith('\\'+suffix)]
        assert len(candidates)==1,candidates
        target=candidates[0];seen=set();sounds=set();loops=set()
        def visit(datum):
            if datum in seen:return
            seen.add(datum);bank,tag=tags[datum]
            if tag['group']=='snd!':sounds.add(datum);return
            assert tag['group'] in ('ssce','lsnd'),tag
            if tag['group']=='lsnd':loops.add(datum)
            for child in references(bank,schema(tag['group']),tag['offset']):visit(child)
        visit(target);assert sounds and loops
        resource_names={d:tags[d][1]['name'] for d in sounds}
        target_name=tags[target][1]['name']
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
    for key,fragment in [('world','MultiplayerObjectRuntime10tu15_world'),('unit','MultiplayerMatch4unitEj'),
        ('player','MultiplayerMatch6playerEj'),('step','MultiplayerMatch4step'),
        ('position','PhysicsLiveWorldTu1519set_object_position'),('loop_request','MultiplayerAudio19object_loop_request'),
        ('loop_frame','MultiplayerAudio17object_loop_frame'),('loop_play','MultiplayerAudio5State4Core4play'),
        ('audio_active','MultiplayerAudio12voice_activeEm'),('audio_position','MultiplayerAudio17playback_positionEm'),
        ('audio_update','MultiplayerAudio6updateEv'),('audio_parameters','SdlSoundPlatform15parameters_impl')]:
        found=[line[2] for line in table if len(line)==3 and line[1] in ('T','W') and fragment in line[2]]
        assert len(found)==1,(key,found);symbols[key]=found[0]
    config=work/'config.json';config.write_text(json.dumps(dict(scratch=str(work),symbols=symbols,
        layout=offsets,audio_layout=audio,start=setup['declared_entry_spawn'],target_sounds=sorted(sounds))))
    # Reuse the actual native playback/route observers. The teleporter-specific
    # cue hooks are excluded, and only this object's authored sound resources
    # enter the playback observer. No runtime sound decision is changed.
    script_text=GDB
    a=script_text.index('class EndCue(');b=script_text.index('class LoopEnd(',a)
    script_text=script_text[:a]+script_text[b:]
    old="World(s['world'],internal=True);Step(s['step'],internal=True);Cue(s['cue'],internal=True);Submit(s['submit'],internal=True)"
    assert script_text.count(old)==1
    script_text=script_text.replace(old,"World(s['world'],internal=True);Step(s['step'],internal=True)")
    old='  if not scope:return False\n  event=dict('
    assert script_text.count(old)==1
    script_text=script_text.replace(old,"  if not scope:return False\n  if int(gdb.parse_and_eval('$esi'))&0xffffffff not in c['target_sounds']:return False\n  event=dict(")
    # This stationary case needs one placement and one positively routed,
    # consumed authored cue. Retire observers after those real observations;
    # leave the full finite gameplay run and native audio stream untouched.
    old="    placed=True;save('placed',"
    assert script_text.count(old)==1
    script_text=script_text.replace(old,"    self.enabled=False;placed=True;save('placed',")
    old="   layout=c['audio_layout'];voice=int(gdb.parse_and_eval('$rsi'));at=int(gdb.parse_and_eval('$rdx'))"
    assert script_text.count(old)==1
    script_text=script_text.replace(old,old+"\n   if not any(event['voice']==voice for event in loop_voices):return False")
    old="   if not paused and volume>-6400 and any(gain>0 for gain in gains):positive_routes[voice]=dict(volume_hundredths_db=volume,route_gains=gains)"
    assert script_text.count(old)==1
    script_text=script_text.replace(old,old+";save('target-routes',positive_routes);self.enabled=False")
    old="if frames>0 and output>0:consumed.append(dict(**event,voice=voice,source_frames=frames,output_frames=output));pending.pop(voice)"
    assert script_text.count(old)==1
    script_text=script_text.replace(old,old+";save('target-consumed',consumed);self.enabled=False")
    old="  voice=int(gdb.parse_and_eval('$rax'));loop_voices.append(dict(**self.event,voice=voice))"
    assert script_text.count(old)==1
    script_text=script_text.replace(old,old+";save('target-created',loop_voices)")
    script=work/'probe.gdb';script.write_text(script_text.replace('CONFIG',repr(str(config))).replace('NON_STOP','set non-stop on'))
    frame=work/'frame.ppm';log=work/'runtime.log'
    command=['gdb','-q','-nx','-batch','-x',str(script),'--args',str(ROOT/'Halo2-Launcher'),
        '--multiplayer','--map',str(maps/map_name),'--shared-maps',str(maps),
        '--local-players','2','--frames',str(frames),'--viewport','800','450','--screenshot',str(frame)]
    started=time.monotonic();code=124
    pipeline_reference=None
    if reference:=os.environ.get('HALO2_GAMEPLAY_PIPELINE_REFERENCE'):
        reference=Path(reference).resolve(strict=True)
        assert reference.is_file() and reference.stat().st_size<=64*1024*1024
        pin=sha(reference);private=owner/'pipeline-reference-copy';private.mkdir()
        copied=private/reference.name;shutil.copyfile(reference,copied)
        assert sha(copied)==pin and sha(reference)==pin
        pipeline_reference=dict(path=str(reference),sha256=pin,bytes=copied.stat().st_size,
            private_copy=str(copied),normal_driver_cache_admission_preserved=True)
    with private_display(work) as display,log.open('w') as stream,contextlib.ExitStack() as owned:
        capture=owned.enter_context(NativeGameplayAudioCapture(work))
        env=dict(os.environ,DISPLAY=display,SDL_VIDEODRIVER='x11',XDG_CACHE_HOME=str(owner/'graphics-cache'))
        if pipeline_reference:env['HALO2_PIPELINE_CACHE_DIRECTORY']=str(private)
        env.update(capture.environment())
        process=subprocess.Popen(command,env=env,stdout=stream,stderr=subprocess.STDOUT,start_new_session=True)
        try:
            while process.poll() is None and time.monotonic()-started<600:
                if (work/'error.json').exists():break
                if (work/'placed.json').exists() and not capture.clients:
                    placed=json.loads((work/'placed.json').read_text())
                    if capture.game is None:capture.bind_process(process,placed['process_pid'],exe)
                    capture.observe()
                time.sleep(.1)
            if process.poll() is not None:code=process.returncode
        finally:stop_process_tree(process)
    raw_log=log.read_text(errors='replace');keep_log(log,out/'runtime.log')
    actual=json.loads((work/'finished.json').read_text()) if (work/'finished.json').exists() else {}
    exited=json.loads((work/'inferior-exit.json').read_text()) if (work/'inferior-exit.json').exists() else {}
    placed=json.loads((work/'placed.json').read_text()) if (work/'placed.json').exists() else {}
    positive=actual.get('positive_routes',{})
    consumed=[dict(v,name=resource_names[v['sound']]) for v in actual.get('consumed',[])
        if v['sound'] in sounds and str(v['voice']) in positive]
    receipt=dict(scope=__doc__,command=command,runtime_sha256=sha(exe),launcher_sha256=sha(ROOT/'Halo2-Launcher'),
        declared_setup_reference=dict(path=str(setup_path),sha256=sha(setup_path),position=setup['declared_entry_spawn']),
        authored_owner=dict(datum=target,name=target_name,sounds=resource_names,loops=sorted(loops)),
        actual=actual,placed=placed,inferior_exit=exited,positive_consumed_object_voices=consumed,native_audio_output=capture.summary,
        exit_code=code,elapsed_seconds=time.monotonic()-started,selection_unchanged=selection.read_bytes()==selected_bytes,
        runtime_unchanged=sha(exe)==selected['runtime_sha256'],map_unchanged=sha(maps/map_name)==map_pin,
        remaining_object_audio_gaps=sorted(set(re.findall(r'^Object loop [^\n]+',raw_log,re.M))),
        physical_Xbox_pixels_or_speaker_waveform_compared=False,large_artifacts_retained=[])
    complete=False
    if frame.exists():
        with Image.open(frame) as image:
            image.load();complete=image.size==(800,450)
    receipt['both_views_completed']=complete
    if pipeline_reference:
        pipeline_reference['source_unchanged']=sha(reference)==pin
        receipt['pipeline_reference']=pipeline_reference
    receipt['passed']=bool(code==0 and exited.get('exit_code')==0 and complete and consumed and capture.summary['nonzero_samples'] and
        capture.summary['clients'] and all(str(c['process_id'])==str(placed.get('process_pid')) for c in capture.summary['clients']) and
        capture.summary['sink_removed'] and capture.summary['default_sink_preserved'] and
        receipt['selection_unchanged'] and receipt['runtime_unchanged'] and receipt['map_unchanged'] and
        (pipeline_reference is None or pipeline_reference['source_unchanged']) and not receipt['remaining_object_audio_gaps'])
    (out/'report.json').write_text(json.dumps(receipt,indent=2)+'\n')
    print(json.dumps({k:v for k,v in receipt.items() if k not in ('scope','actual')}),flush=True)
    return 0 if receipt['passed'] else 1

if __name__=='__main__':raise SystemExit(main())
