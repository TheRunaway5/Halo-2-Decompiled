#!/usr/bin/env python3
"""Walk into and drive/fire a Ghost in the ordinary native executable.

A real neutral LAN peer observes completed host state. Actual XTest keyboard
and mouse events control the ordinary local player; no debugger, memory write,
placement, supplied seat result or substituted physics/render callback is used.
The enclosing run_test.py owner cleans up binaries, raw frames and driver caches.
"""
import argparse
import contextlib
import json
import math
import os
from pathlib import Path
import re
import shlex
import shutil
import socket
import subprocess
import sys
import time

from probe_installed_banshee_gameplay_20261022 import Inputs
from validate_weapon_runtime_tu15 import private_display
from weapon_test_cache import sha
from build_launcher_engine import source_hashes

ROOT=Path(__file__).resolve().parents[1]


def save(path,data):path.write_text(json.dumps(data,indent=2)+'\n')


def worker(args):
    owner=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    args.output=args.output.resolve();args.output.mkdir(parents=True,exist_ok=False)
    latest=owner/'actual-lan-state.json';frame=owner/'actual-game.ppm'
    with socket.socket(socket.AF_INET,socket.SOCK_DGRAM) as port_owner:
        port_owner.bind(('127.0.0.1',0));port=port_owner.getsockname()[1]
    random_choice=getattr(args,'vehicle_choice',None)
    fixed_case=getattr(args,'vehicle_case',None) or 'ghost'
    require_flying=getattr(args,'require_native_flying_selector',False)
    additional=getattr(args,'additional_trigger',False)
    takeoff=getattr(args,'banshee_takeoff',False)
    trace_failure=getattr(args,'trace_native_failure',False)
    if trace_failure and (not additional or args.native_kernel_counters):
        raise ValueError('Failure tracing is a separate additional-trigger diagnosis without counter acceptance')
    if takeoff and (additional or random_choice or fixed_case!='banshee'):
        raise ValueError('Takeoff acceptance requires the fixed Banshee primary-input case')
    if additional and (random_choice or fixed_case not in ('scorpion','banshee')):
        raise ValueError('Additional-trigger game proof currently covers the authored Scorpion and Banshee controls')
    if require_flying and (not args.native_kernel_counters or fixed_case!='banshee' or random_choice):
        raise ValueError('Flying-kernel process proof requires fixed Banshee input and native counters')
    stationary_case=random_choice=='turret' or fixed_case in ('h_turret_ap','c_turret_ap')
    walking_route_case=stationary_case or fixed_case=='banshee'
    submitted_frames=18000 # Safety limit; successful input closes the actual window normally.
    fixed_settings={'ghost':(76,3),'banshee':(79,1),'warthog-mg':(76,1),'warthog-gauss':(76,2),
        'spectre':(76,5),'scorpion':(78,1),'wraith':(78,2),'h_turret_ap':(81,1),'c_turret_ap':(81,4)}
    parameter,value={'light':(76,6),'heavy':(78,3),'turret':(81,5)}.get(random_choice,fixed_settings[fixed_case])
    map_name=getattr(args,'map_name','coagulation.map')
    if Path(map_name).name!=map_name or not map_name.endswith('.map'):raise ValueError('Use one owned MCC map filename')
    command=[str(args.runtime),'--multiplayer','--map',str(args.maps/map_name),
        '--shared-maps',str(args.maps),'--local-players','1','--host',str(port),
        '--frames',str(submitted_frames),'--viewport','800','450','--screenshot',str(frame),
        '--mp-starting-weapon','5','--mp-rule',str(parameter),str(value),
        '--mp-source-object-world',str(owner/'source-savegame')]
    if not args.native_audio_output:command.append('--mute')
    if trace_failure:
        script=owner/'native-failure.gdb'
        script.write_text("""set pagination off
set confirm off
set disable-randomization off
break 'halo2::(anonymous namespace)::missing(unsigned int)'
commands
silent
printf "\\nHALO2_NATIVE_BACKING_FAILURE\\n"
bt 32
info registers rdi rsi rdx rcx
quit 86
end
run
""")
        command=['gdb','--batch','--quiet','--nx','-x',str(script),'--args',*command]
    observe=[str(args.observer),str(args.maps/map_name),str(args.maps),
        '127.0.0.1:'+str(port),str(latest),'570']
    report=dict(passed=False,command=command,observer_command=observe,actions=[],
        runtime_sha256=sha(args.runtime),debugger_used=trace_failure,host_memory_written=None if trace_failure else False,
        diagnostic_only=trace_failure,host_gameplay_state_written=False,
        pose_velocity_or_admission_supplied=False,full_vehicle_parity=False,
        original_pixels_or_audio_compared=False,physical_speakers_verified=False)
    report['vehicle_setting']=dict(family=random_choice or 'fixed-'+fixed_case,parameter=parameter,value=value)
    phase='discover';target=None;entry=None;seated_tick=None;action_tick=None;action_input=0
    waypoints=[];waypoint=0
    progress_position=None;progress_tick=0
    initial_position=None;final_position=None;projectiles=set();expected_projectiles=set();samples=[];last_tick=None
    phase_samples={};maximum_horizontal=0.;maximum_climb=0.
    action_ticks=300 if takeoff else 60
    if takeoff:report['declared_takeoff_input']=dict(pitch_radians=.3,forward_ticks=action_ticks,
        jump_held=False,minimum_horizontal_exclusive=10,minimum_climb_exclusive=2,
        gate_source='tests/physics_multiplayer_acceptance.cpp: original retained normal-flight gates')
    held={};mouse=False;focused=False;code=124;observer_code=None
    released_tick=None;window_close_at=None
    start=time.monotonic();log=owner/'runtime.log';observer_log=owner/'observer.log'
    capture=None;game_pid=None;next_audio=0
    counters=None
    if args.native_kernel_counters:
        from native_gameplay_kernel_counters_tu15 import NativeGameplayKernelCounters
        counters=NativeGameplayKernelCounters(args.runtime,vehicle_choices=bool(random_choice),flying_selector=require_flying)
        report['native_kernel_counter_samples']=[]
    with contextlib.ExitStack() as owned,private_display(owner) as display,log.open('w') as game_log,observer_log.open('w') as peer_log:
        environment={key:value for key,value in os.environ.items() if not key.startswith(
            ('HALO2_REFERENCE_','HALO2_CAPTURE_','HALO2_TEST_')) or key=='HALO2_TEST_SCRATCH'}
        environment.update(DISPLAY=display,SDL_VIDEODRIVER='x11',SDL_AUDIODRIVER='dummy',
            XDG_CACHE_HOME=str(owner/'graphics-cache'),HALO2_PIPELINE_CACHE_DIRECTORY=str(owner/'vulkan-cache'))
        if args.native_audio_output:
            from native_gameplay_audio_capture import NativeGameplayAudioCapture
            capture=owned.enter_context(NativeGameplayAudioCapture(owner))
            environment.update(capture.environment())
        game=subprocess.Popen(command,env=environment,stdout=game_log,stderr=subprocess.STDOUT)
        game_pid=game.pid
        if trace_failure:
            deadline=time.monotonic()+10
            while time.monotonic()<deadline and game.poll() is None:
                try:
                    children=Path('/proc/'+str(game.pid)+'/task/'+str(game.pid)+'/children').read_text().split()
                    actual=[int(pid) for pid in children if Path('/proc/'+pid+'/exe').resolve()==args.runtime.resolve()]
                except FileNotFoundError:actual=[]
                if len(actual)==1:game_pid=actual[0];break
                time.sleep(.05)
            if game_pid==game.pid:raise ValueError('Debugger did not launch its owned native executable')
            report['diagnostic_script']=dict(sha256=sha(script),text=script.read_text(),actual_native_pid=game_pid)
        if capture:capture.bind_process(game,game_pid,args.runtime)
        if counters:counters.bind_process(game)
        peer=subprocess.Popen(observe,env=environment,stdout=peer_log,stderr=subprocess.STDOUT)
        inputs=Inputs(display)
        def key(name,down):
            if held.get(name,False)!=down:
                inputs.key(name,down);held[name]=down
                report['actions'].append(dict(key=name,down=down,at_seconds=time.monotonic()-start))
        def button(down):
            nonlocal mouse
            if action_input==2:
                key('f',down);return
            if mouse!=down:
                inputs.button(down,3 if action_input==1 else 1);mouse=down
                report['actions'].append(dict(button='secondary' if action_input==1 else 'primary',down=down,at_seconds=time.monotonic()-start))
        try:
            while game.poll() is None and time.monotonic()-start<570:
                if window_close_at is not None and time.monotonic()-window_close_at>10:
                    report['normal_window_close_timed_out']=True;break
                if capture and time.monotonic()>=next_audio:
                    capture.observe();next_audio=time.monotonic()+.5
                if peer.poll() is not None:
                    report['observer_stopped_early']=peer.returncode
                    if phase=='released':time.sleep(.05);continue
                    break
                if not latest.exists():time.sleep(.05);continue
                try:state=json.loads(latest.read_text())
                except (OSError,json.JSONDecodeError):time.sleep(.03);continue
                if state['tick']==last_tick:time.sleep(.03);continue
                last_tick=state['tick']
                player=next((p for p in state['players'] if p['slot']==0),None)
                if not player or player['dead']:time.sleep(.03);continue
                if not focused:
                    inputs.focus();inputs.button(True);time.sleep(.12);inputs.button(False);focused=True
                    report['actions'].append(dict(action='focus actual game window',tick=state['tick']))
                    if counters:report['native_kernel_counter_samples'].append(dict(
                        phase='first_actual_snapshot',tick=state['tick'],values=counters.read()))
                if target is None:
                    if random_choice:
                        classification={'light':1,'heavy':3,'turret':6}[random_choice]
                        choices=[(v,e)for v in state['vehicles']if v.get('netgame_classification')==classification
                            for e in v['entries']if e['flags']&12]
                        # Prefer the authored gunner seat when a chassis has
                        # separate driving and weapon owners.
                        if any(e['flags']&8 for _,e in choices):choices=[(v,e)for v,e in choices if e['flags']&8]
                    else:
                        family='warthog' if fixed_case.startswith('warthog-') else fixed_case
                        seat_flag=8 if family in ('warthog','spectre','h_turret_ap','c_turret_ap') else 4
                        choices=[(v,e) for v in state['vehicles'] if ('\\'+family+'\\')in v['name']
                            for e in v['entries'] if e['flags']&seat_flag]
                    if not choices:time.sleep(.03);continue
                    target,entry=min(choices,key=lambda item:math.dist(item[1]['approach'],player['position']))
                    report['target']=dict(vehicle=target,entry=entry,initial_player=player)
                    # Walk around the authored vehicle bounds to the entry
                    # side. A straight line to the seat marker hits the hull.
                    center=target['position'];approach=entry['approach']
                    ex=approach[0]-center[0];ey=approach[1]-center[1];length=math.hypot(ex,ey)
                    if length<=0:raise ValueError('Authored entry has no horizontal approach direction')
                    ex/=length;ey/=length
                    clearance=max(target['bounding_radius'],entry['radius'])+2*target['player_radius']
                    # Reach the entry's outer side before moving inward.
                    # Merely sharing its hemisphere can still send a straight
                    # approach through the front corner of a wide hull.
                    begin=math.atan2(player['position'][1]-center[1],player['position'][0]-center[0])
                    end=math.atan2(ey,ex);arc=(end-begin+math.pi)%(2*math.pi)-math.pi
                    steps=max(1,math.ceil(abs(arc)/(math.pi/8)))
                    waypoints=[[center[0]+clearance*math.cos(begin+arc*i/steps),
                                center[1]+clearance*math.sin(begin+arc*i/steps)]for i in range(steps+1)]
                    waypoints.append(approach[:2]);report['declared_walk_waypoints']=waypoints
                    if walking_route_case and entry.get('walking_route'):
                        waypoints=entry['walking_route'];report['declared_walk_waypoints']=waypoints
                        report['walking_route_source']=entry['route_source']
                    phase='approach'
                    progress_position=player['position'];progress_tick=state['tick']
                vehicle=next((v for v in state['vehicles'] if v['handle']==target['handle']),None)
                if vehicle is None:raise ValueError('Actual target vehicle retired before acceptance')
                report['last_actual_observation']=dict(tick=state['tick'],phase=phase,player=player,
                    vehicle_position=vehicle['position'],walking_waypoint=waypoint,
                    walking_destination=waypoints[waypoint] if waypoints else None)
                if phase=='approach' and player['parent']==target['handle']:
                    key('w',False);key('e',False);key('space',False);seated_tick=state['tick'];phase='ready'
                    report['actual_seat_admitted']=dict(tick=seated_tick,parent=player['parent'],unit=player['unit'])
                    if counters:report['native_kernel_counter_samples'].append(dict(
                        phase='seat_admitted',tick=state['tick'],values=counters.read()))
                if phase=='approach':
                    if math.dist(player['position'],progress_position)>.12:
                        progress_position=player['position'];progress_tick=state['tick']
                    def waypoint_distance(point):
                        horizontal=math.dist(point[:2],player['position'][:2])
                        # Continue across a sampled floor while airborne;
                        # chasing its old height steers back off the ramp.
                        if len(point)==3 and player['position'][2]<point[2]-.35:
                            return math.hypot(horizontal,point[2]-player['position'][2])
                        return horizontal
                    while waypoint+1<len(waypoints) and waypoint_distance(waypoints[waypoint])<(.3 if len(waypoints[waypoint])==3 else .65):
                        waypoint+=1
                    destination=waypoints[waypoint]
                    distance=math.dist(destination[:2],player['position'][:2])
                    at_entry=waypoint+1==len(waypoints) and distance<max(.65,entry['radius'])
                    look=entry['marker'] if at_entry else destination
                    dx=look[0]-player['position'][0];dy=look[1]-player['position'][1]
                    desired=math.atan2(dy,dx);error=(desired-player['yaw']+math.pi)%(2*math.pi)-math.pi
                    inputs.move(max(-25,min(25,round(-error/.0025))),0)
                    key('w',distance>(.12 if len(destination)==3 and waypoint+1<len(waypoints) else .55) and abs(error)<.35)
                    if len(destination)==3:
                        key('space',destination[2]-player['position'][2]>.12 and distance<.9 and
                            player.get('grounded',False) and state['tick']-progress_tick>=18)
                    key('e',at_entry)
                elif phase=='ready':
                    desired=math.atan2(vehicle['forward'][1],vehicle['forward'][0])
                    error=(desired-player['yaw']+math.pi)%(2*math.pi)-math.pi
                    inputs.move(max(-25,min(25,round(-error/.0025))),
                        max(-20,min(20,round(-((.3 if takeoff else .45 if fixed_case=='banshee' else .1)-player['pitch'])/.0025))))
                    if state['tick']>=seated_tick+90 and player['vehicle_weapon_present'] and abs(error)<.2:
                        expected_projectiles=set(player.get('vehicle_projectile_definitions',[]))
                        if additional:
                            authored=next((t for t in player.get('vehicle_triggers',[]) if t['input']>0),None)
                            if not authored or authored['input'] not in (1,2) or authored['primary_projectile']==0xffffffff:
                                raise ValueError('Actual seated weapon has no supported additional authored trigger/barrel')
                            action_input=authored['input'];expected_projectiles={authored['primary_projectile']}
                            report['actual_additional_trigger']=authored
                            # Complete original1889D0 suppresses input2 in a
                            # multiplayer game. A retained bomb barrel alone
                            # must not enable it; input1 remains an active gun.
                            report['additional_trigger_expectation']='suppressed by original multiplayer input2 gate' if action_input==2 else 'authored input1 projectile required'
                        if not expected_projectiles:raise ValueError('The actual seated weapon has no authored projectile definitions')
                        report['actual_weapon_projectile_definitions']=sorted(expected_projectiles)
                        action_tick=state['tick'];initial_position=vehicle['position'];phase='action'
                        if counters:report['native_kernel_counter_samples'].append(dict(
                            phase='action_started',tick=state['tick'],values=counters.read()))
                        key('w',bool(entry['flags']&4) and target.get('netgame_classification')!=6)
                        key('space',fixed_case=='banshee' and not takeoff);button(True)
                elif phase=='action':
                    for projectile in state['projectiles']+state.get('contact_projectiles',[]):
                        if projectile['definition']not in expected_projectiles:continue
                        if 'unit'in projectile and projectile['unit']!=player['unit']:continue
                        projectiles.add(projectile['datum'])
                    final_position=vehicle['position']
                    maximum_horizontal=max(maximum_horizontal,math.dist(initial_position[:2],final_position[:2]))
                    maximum_climb=max(maximum_climb,final_position[2]-initial_position[2])
                    if state['tick']>=action_tick+action_ticks:
                        key('w',False);key('space',False);button(False);phase='released'
                        released_tick=state['tick']
                        report['actual_action_completed']=dict(first_tick=action_tick,last_tick=state['tick'],
                            initial_position=initial_position,final_position=final_position,
                            displacement=math.dist(initial_position,final_position),projectile_generations=sorted(projectiles),
                            maximum_horizontal=maximum_horizontal,maximum_climb=maximum_climb)
                        if counters:report['native_kernel_counter_samples'].append(dict(
                            phase='action_completed',tick=state['tick'],values=counters.read()))
                elif phase=='released' and window_close_at is None and state['tick']>=released_tick+30:
                    # Use the normal desktop window-close path after actual
                    # source actions and a neutral interval. Render throughput
                    # must not decide how long a player can walk to a vehicle.
                    inputs.capture(frame)
                    from native_gameplay_window_close import close_actual_game_window
                    close_actual_game_window(inputs);window_close_at=time.monotonic()
                    report['normal_window_close_requested']=dict(tick=state['tick'],
                        after_neutral_ticks=state['tick']-released_tick,
                        capture_source='Actual X11 game drawable before WM_DELETE_WINDOW')
                # A long walk must not consume the entire diagnostic budget
                # before the vehicle's actual action starts.
                kept=phase_samples.setdefault(phase,[])
                kept.append(dict(tick=state['tick'],phase=phase,
                    player=player,vehicle_position=vehicle['position'],projectiles=state['weapon_projectiles']))
                if len(kept)>100:del kept[1]
                time.sleep(.03)
            if game.poll() is not None:code=game.returncode
        except Exception as error:report['probe_error']=str(error)
        finally:
            for name in tuple(held):key(name,False)
            button(False)
            if game.poll() is None:
                game.terminate()
                try:game.wait(timeout=10)
                except subprocess.TimeoutExpired:game.kill();game.wait()
            if peer.poll() is None:
                peer.terminate()
                try:peer.wait(timeout=5)
                except subprocess.TimeoutExpired:peer.kill();peer.wait()
            observer_code=peer.returncode
    tail=log.read_text(errors='replace')[-16000:]
    frames=re.search(r'Multiplayer frames=(\d+) ticks=(\d+) players=(\d+)',tail)
    action=report.get('actual_action_completed',{})
    movement_required=bool(entry and entry['flags']&4) and target.get('netgame_classification')!=6
    projectile_result=not projectiles if additional and action_input==2 else bool(projectiles)
    takeoff_result=not takeoff or maximum_horizontal>10 and maximum_climb>2
    samples=sorted((row for rows in phase_samples.values() for row in rows),key=lambda row:row['tick'])
    report.update(passed=code==0 and frames is not None and int(frames[1])>0 and window_close_at is not None and
        phase=='released' and (not movement_required or action.get('displacement',0)>=1) and projectile_result and takeoff_result,
        declared_takeoff_requested=takeoff,takeoff_motion_gate_passed=takeoff_result,
        additional_trigger_requested=additional,actual_trigger_input=action_input,
        projectile_expectation_passed=projectile_result,
        movement_required_by_actual_seat=movement_required,
        exit_code=code,observer_exit_code=observer_code,phase=phase,natural_game_shutdown=code==0,
        submitted_frame_safety_limit=submitted_frames,
        actual_submitted_frames=int(frames[1]) if frames else 0,elapsed_seconds=time.monotonic()-start,
        samples=samples,log_tail=tail,observer_log_tail=observer_log.read_text(errors='replace')[-6000:])
    if capture:
        summary=capture.summary or {};clients=summary.get('clients',[])
        actual_output=bool(clients and all(str(c['process_id'])==str(game_pid) for c in clients) and
            summary.get('nonzero_samples',0)>0 and summary.get('sink_removed') and summary.get('default_sink_preserved'))
        report.update(native_audio_output=summary,actual_host_output_verified=actual_output,
            native_audio_capture_tool_sha256=sha(ROOT/'tools/native_gameplay_audio_capture.py'))
        report['passed'] &= actual_output
    if counters:
        observed=report['native_kernel_counter_samples']
        starts=[row for row in observed if row['phase']=='action_started']
        ends=[row for row in observed if row['phase']=='action_completed']
        complete=bool(starts and ends and len(observed)==4)
        report['native_kernel_counters_read_only']=True
        report['native_kernel_counter_tool_sha256']=sha(ROOT/'tools/native_gameplay_kernel_counters_tu15.py')
        if complete:
            delta={name:ends[0]['values'][name]-starts[0]['values'][name] for name in starts[0]['values']}
            complete=all(value>=0 for value in delta.values())
            report['native_kernel_calls_during_actual_input']=delta
        report['native_kernel_counter_observation_complete']=complete
        report['passed'] &= complete
        if random_choice:
            actual_random=bool(observed and observed[-1]['values']['vehicle_selector']>0 and
                observed[-1]['values']['vehicle_random_draws']>0)
            report['actual_process_native_random_selector_observed']=actual_random
            report['passed'] &= actual_random
        if require_flying:
            actual_flying=bool(observed and observed[-1]['values']['flying_vehicle_selector']>0)
            report['actual_process_native_flying_selector_observed']=actual_flying
            report['passed'] &= actual_flying
    if frame.exists():
        from PIL import Image
        path=args.output/'reference.png'
        with Image.open(frame) as image:
            if image.size!=(800,450):raise ValueError('Unexpected ordinary-game capture size')
            image.convert('RGB').save(path)
        if path.stat().st_size>1024*1024:raise ValueError('Required ordinary-game reference exceeds bounded budget')
        report['retained_reference_image']=dict(path=str(path),bytes=path.stat().st_size,sha256=sha(path))
    save(args.output/'report.json',report)
    print(json.dumps(dict(passed=report['passed'],phase=phase,exit_code=code,report=str(args.output/'report.json'))),flush=True)
    return 0 if report['passed'] else 1


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    for name in ('prepared','cohort','refresh','validation','output','runtime','observer','maps'):
        parser.add_argument('--'+name,type=Path)
    parser.add_argument('--case-worker',action='store_true')
    parser.add_argument('--native-audio-output',action='store_true')
    parser.add_argument('--native-kernel-counters',action='store_true')
    parser.add_argument('--require-native-flying-selector',action='store_true')
    parser.add_argument('--additional-trigger',action='store_true')
    parser.add_argument('--banshee-takeoff',action='store_true')
    parser.add_argument('--trace-native-failure',action='store_true')
    parser.add_argument('--vehicle-choice',choices=('light','heavy','turret'),
        help='Real Random rule and authored row/seat input; usable with --case-worker')
    parser.add_argument('--vehicle-case',choices=('ghost','banshee','warthog-mg','warthog-gauss','spectre','scorpion','wraith','h_turret_ap','c_turret_ap'))
    parser.add_argument('--map-name',default='coagulation.map')
    parser.add_argument('--runtime-base-manifest',type=Path,
        help='Accept a qualified delivered superset only when every change from this baseline is present exactly')
    args=parser.parse_args()
    if args.vehicle_case and args.vehicle_choice:parser.error('Choose a fixed vehicle or Random family')
    if args.case_worker:return worker(args)
    if args.require_native_flying_selector or args.additional_trigger or args.banshee_takeoff or args.trace_native_failure:parser.error('Use the delivered-runtime wrapper for additional input/process proof')
    if not all((args.prepared,args.cohort,args.refresh,args.validation,args.output)):
        parser.error('Supply the current source receipts, completed build validation and output')
    owner=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    prepared=json.loads(args.prepared.read_text());cohort=json.loads(args.cohort.read_text())
    refresh=json.loads(args.refresh.read_text());validation=json.loads(args.validation.read_text())
    borrowed=Path(cohort['owner']);frozen=borrowed/'source';build=borrowed/'build'
    if owner==borrowed or Path(prepared['runner_scratch'])!=borrowed or Path(refresh['owner'])!=borrowed or Path(validation['build'])!=build:
        raise ValueError('Separate authenticated native build owner required')
    if not all(any(s['name']==name and s['exit_code']==0 for s in validation['steps']) for name in ('configure','build')):
        raise ValueError('The current native build must finish before borrowing it')
    canonical=bool(args.runtime)
    lineage=None
    if canonical:
        inventory=args.runtime.resolve(strict=True).parent/'source-hashes.json'
        installed=json.loads(inventory.read_text());expected=source_hashes(frozen)
        if expected!=installed:
            if not args.runtime_base_manifest:
                raise ValueError('Installed runtime and current canonical source inventory differ')
            base=json.loads(args.runtime_base_manifest.read_text())
            owned_changes={name for name in base.keys()|expected.keys() if base.get(name)!=expected.get(name)}
            additional={name for name in installed.keys()|expected.keys() if installed.get(name)!=expected.get(name)}
            if not owned_changes or any(installed.get(name)!=expected.get(name) for name in owned_changes) or any(
                    expected.get(name)!=base.get(name) for name in additional):
                raise ValueError('Delivered source does not preserve every qualified vehicle change')
            headers=('include/halo2/multiplayer.hpp','include/halo2/multiplayer_network.hpp',
                'include/halo2/content.hpp','include/halo2/native_unit_interaction.hpp',
                'include/halo2/model_markers.hpp','include/halo2/collision.hpp',
                'include/halo2/source_game_variant.hpp','include/halo2/source_model_regions.hpp')
            if any(installed.get(name)!=expected.get(name) for name in headers):
                raise ValueError('Native LAN observer public schema changed in the delivered package')
            qualification=args.runtime.resolve(strict=True).parent/'qualification.json'
            qualified=json.loads(qualification.read_text())
            if not qualified['passed'] or qualified['runtime_sha256']!=sha(args.runtime.resolve(strict=True)):
                raise ValueError('Delivered superset lacks a successful qualification for its actual binary')
            lineage=dict(base_manifest_sha256=sha(args.runtime_base_manifest),
                exact_vehicle_changes=sorted(owned_changes),additional_delivered_paths=sorted(additional),
                unchanged_observer_public_headers=list(headers),qualification_sha256=sha(qualification),
                scope='Vehicle changes preserved exactly; live input test also exercises the additional delivered source')
    else:
        expected=dict(prepared['registered_snapshot_source_files_sha256'])
        for row in cohort['changes']:expected[row['path']]=row['after']
        expected.update({'data/'+name:pin for name,pin in cohort['private_data_sha256'].items()})
        expected.update(refresh['refreshed_source_sha256'])
    if any(sha(frozen/name)!=pin for name,pin in expected.items()):raise ValueError('Frozen native source drifted')
    args.output=args.output.resolve();args.output.mkdir(parents=True,exist_ok=False)
    source=owner/'source';source.mkdir();entry=ROOT/'tools/observe_vehicle_game_lan_tu15.cpp'
    pin=sha(entry);shutil.copyfile(entry,source/'observer.cpp')
    if sha(source/'observer.cpp')!=pin or sha(entry)!=pin:raise ValueError('LAN observer changed while copying')
    runtime=(args.runtime or build/'halo2_native').resolve(strict=True);runtime_pin=sha(runtime)
    link_file=build/'CMakeFiles/native_tu15_world_memory_tests.dir/link.txt';link=shlex.split(link_file.read_text());at=link.index('-o')
    pins={};resolved=[]
    for name in link[at+2:]:
        if name.startswith('-'):resolved.append(name);continue
        path=(build/name).resolve(strict=True);pins[str(path)]=sha(path);resolved.append(str(path))
    q=lambda value:'"'+str(value).replace('\\','/').replace('"','\\"')+'"'
    maps=Path(prepared['runtime_cache_inputs']['maps_directory'])
    cmake='''cmake_minimum_required(VERSION 3.20)
project(OrdinaryNativeVehicleGame LANGUAGES CXX)
enable_testing()
find_package(Python3 COMPONENTS Interpreter REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED sdl2)
add_executable(vehicle_game_observer observer.cpp)
target_compile_features(vehicle_game_observer PRIVATE cxx_std_20)
target_compile_options(vehicle_game_observer PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
'''
    cmake+='target_include_directories(vehicle_game_observer PRIVATE ${SDL2_INCLUDE_DIRS} '+q(frozen/'include')+' '+q(frozen/'src')+')\n'
    cmake+='target_link_libraries(vehicle_game_observer PRIVATE "-Wl,--start-group" '+' '.join(map(q,resolved))+' "-Wl,--end-group")\n'
    cmake+='add_test(NAME ordinary_native_ghost COMMAND "${Python3_EXECUTABLE}" '+q(ROOT/'tools/run_test.py')+' -- "${Python3_EXECUTABLE}" -B '+q(Path(__file__))+' --case-worker --runtime '+q(runtime)+' --observer $<TARGET_FILE:vehicle_game_observer> --maps '+q(maps)+' --output '+q(args.output/'gameplay')+(' --native-audio-output' if args.native_audio_output else '')+(' --native-kernel-counters' if args.native_kernel_counters else '')+')\n'
    cmake+='set_tests_properties(ordinary_native_ghost PROPERTIES TIMEOUT 600 RUN_SERIAL TRUE)\n'
    (source/'CMakeLists.txt').write_text(cmake)
    command=[sys.executable,'-B',str(ROOT/'tools/run_tests.py'),'--build-worker','--source',str(source),
        '--output',str(args.output/'validation'),'--target','vehicle_game_observer','--filter','^ordinary_native_ghost$',
        '--jobs','2','--test-jobs','1']
    report=dict(passed=False,status='running',owner=str(owner),borrowed_owner=str(borrowed),
        borrowed_native_libraries_sha256=pins,runtime=str(runtime),runtime_sha256=runtime_pin,
        canonical_installed_runtime=canonical,
        delivered_superset_lineage=lineage,
        observer_source_sha256=pin,command=command,debugger_used=False,full_vehicle_parity=False)
    save(args.output/'report.json',report)
    code=subprocess.run(command,stdout=subprocess.DEVNULL,stderr=subprocess.STDOUT).returncode
    unchanged=sha(runtime)==runtime_pin and all(sha(Path(name))==pin for name,pin in pins.items())
    unchanged &= all(sha(frozen/name)==pin for name,pin in expected.items())
    report.update(passed=code==0 and unchanged,status='completed',validation_returncode=code,
        borrowed_inputs_unchanged=unchanged)
    save(args.output/'report.json',report)
    print(json.dumps(dict(passed=report['passed'],report=str(args.output/'report.json'))),flush=True)
    return 0 if report['passed'] else code or 1


if __name__=='__main__':raise SystemExit(main())
