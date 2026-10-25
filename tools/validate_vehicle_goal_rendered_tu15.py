#!/usr/bin/env python3
"""Run input-driven rendered firing/audio cases from the pinned native build.

Each case owns a private display, raw frame, driver cache and report under
run_test.py. Only bounded diagnostics and explicitly retained PNG references
survive. The probe observes the existing native fixture's real seat admission,
SDL button input, projectiles, visual submissions and authored Classic voices.
"""
import argparse
import contextlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import time

from validate_weapon_runtime_tu15 import private_display
from weapon_test_cache import sha

ROOT=Path(__file__).resolve().parents[1]
CASES=('ghost','banshee','scorpion','wraith','warthog-mg','warthog-gauss',
       'spectre','h_turret_ap','c_turret_ap','scorpion-secondary')


def save(path,value):path.write_text(json.dumps(value,indent=2)+'\n')


def worker(args):
    owner=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    started=time.monotonic()
    primary='scorpion' if args.case=='scorpion-secondary' else args.case
    name='warthog' if primary.startswith('warthog-') else primary
    raw_report=owner/'gameplay.json';raw_frame=owner/'frame.ppm';log=owner/'runtime.log'
    command=[str(args.runtime),str(args.maps/'coagulation.map'),str(args.maps),'native-mcc',str(raw_frame)]
    environment={key:value for key,value in os.environ.items() if
        not key.startswith(('HALO2_REFERENCE_','HALO2_CAPTURE_','HALO2_TEST_')) or key=='HALO2_TEST_SCRATCH'}
    environment.update(SDL_VIDEODRIVER='x11',SDL_AUDIODRIVER='dummy',
        XDG_CACHE_HOME=str(owner/'graphics-cache'),HALO2_TEST_TURRET_AUDIO='1',
        HALO2_TEST_VEHICLE_WEAPON_HUD_CASE=primary,HALO2_VEHICLE_HUD_GPU_NAME=name,
        HALO2_TEST_VEHICLE_WEAPON_HUD_REPORT=str(raw_report))
    if args.case=='scorpion-secondary':environment['HALO2_TEST_VEHICLE_WEAPON_SECONDARY']='1'
    if args.generic_combiner_control:environment['HALO2_REFERENCE_GENERIC_COMBINERS']='1'
    pipeline_control=None
    if args.disable_pipeline_optimization_control:
        source=owner/'pipeline-control.cpp';library=owner/'pipeline-control.so'
        source.write_text('''#include <vulkan/vulkan.h>
#include <dlfcn.h>
#include <vector>
#include <atomic>
#include <cstdio>
#include <cstdlib>
extern "C" VKAPI_ATTR VkResult VKAPI_CALL vkCreateGraphicsPipelines(VkDevice device,
    VkPipelineCache cache,unsigned count,const VkGraphicsPipelineCreateInfo* info,
    const VkAllocationCallbacks* allocation,VkPipeline* output) {
  static auto actual=reinterpret_cast<PFN_vkCreateGraphicsPipelines>(
      dlsym(RTLD_NEXT,"vkCreateGraphicsPipelines"));
  if(!actual)std::abort();
  if(!count || !info)return actual(device,cache,count,info,allocation,output);
  std::vector<VkGraphicsPipelineCreateInfo> copy(info,info+count);
  unsigned changed{};
  for(auto& row:copy) {
    bool selected=!SPECIALIZED_ONLY;
    for(unsigned n=0;n<row.stageCount;++n) {
      const auto& stage=row.pStages[n];const auto* spec=stage.pSpecializationInfo;
      if(stage.stage==VK_SHADER_STAGE_FRAGMENT_BIT && spec && spec->mapEntryCount==61 &&
          spec->dataSize==61*sizeof(unsigned) && spec->pData &&
          static_cast<const unsigned*>(spec->pData)[0]==1)selected=true;
    }
    if(selected){row.flags|=VK_PIPELINE_CREATE_DISABLE_OPTIMIZATION_BIT;++changed;}
  }
  static std::atomic<unsigned> calls{};
  if(changed && calls.fetch_add(1)<4)std::fprintf(stderr,"[VEHICLE-PIPELINE-CONTROL] native graphics creation; optimization disabled; count=%u changed=%u\\n",count,changed);
  return actual(device,cache,count,copy.data(),allocation,output);
}
'''.replace('SPECIALIZED_ONLY','true' if args.specialized_pipeline_control else 'false'))
        compile_command=['g++','-std=c++20','-O2','-shared','-fPIC',str(source),'-o',str(library),'-ldl']
        compiler=subprocess.run(compile_command,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True)
        if compiler.returncode:raise ValueError('Private Vulkan control compile failed: '+compiler.stdout[-6000:])
        environment['LD_PRELOAD']=str(library)+(' '+environment['LD_PRELOAD'] if environment.get('LD_PRELOAD') else '')
        pipeline_control=dict(source_sha256=sha(source),library_sha256=sha(library),command=compile_command,
            native_pipeline_create_info_copied=True,only_disable_optimization_flag_added=True,
            native_API_results_supplied=False,production_default_qualified=False)
        pipeline_control['only_existing_61_word_fragment_specializations']=args.specialized_pipeline_control
    capture=None;game_pid=None
    with contextlib.ExitStack() as owned,private_display(owner) as display,log.open('w') as stream:
        environment['DISPLAY']=display
        if args.native_audio_output:
            from native_gameplay_audio_capture import NativeGameplayAudioCapture
            capture=owned.enter_context(NativeGameplayAudioCapture(owner))
            environment.update(capture.environment())
        game=subprocess.Popen(command,env=environment,stdout=stream,stderr=subprocess.STDOUT)
        if capture:capture.bind_process(game)
        game_pid=game.pid;deadline=time.monotonic()+300;next_audio_observation=0
        try:
            while game.poll() is None and time.monotonic()<deadline:
                if capture and time.monotonic()>=next_audio_observation:
                    capture.observe();next_audio_observation=time.monotonic()+.5
                time.sleep(.1)
            result=game.returncode if game.poll() is not None else 124
        finally:
            if game.poll() is None:
                game.terminate()
                try:game.wait(timeout=10)
                except subprocess.TimeoutExpired:game.kill();game.wait()
    data=json.loads(raw_report.read_text()) if raw_report.exists() else {}
    rows=data.get('cases',[]);row=rows[0] if len(rows)==1 else {};audio=row.get('turret_audio',{})
    passed=(result==0 and data.get('passed') is True and row.get('passed') is True and
        row.get('cache_is_mcc') is True and row.get('actual_Vulkan_frames',0)>=90 and
        audio.get('passed') is True and audio.get('constructed_shots',0)>0 and
        audio.get('sound_submissions',0)>0 and not audio.get('submission_violations',[1]) and
        not data.get('source_results_or_ready_flags_supplied',True))
    args.output.mkdir(parents=True,exist_ok=False)
    if raw_report.exists():
        if raw_report.stat().st_size>1024*1024:raise ValueError('Gameplay report exceeds bounded budget')
        shutil.copyfile(raw_report,args.output/'gameplay.json')
    runtime_log=log.read_text(errors='replace')
    if pipeline_control:
        pipeline_control['observed_interception_messages']=runtime_log.count('[VEHICLE-PIPELINE-CONTROL]')
        passed &= pipeline_control['observed_interception_messages']>0
    report=dict(passed=passed,case=args.case,exit_code=result,command=command,
        actual_Vulkan_frames=row.get('actual_Vulkan_frames',0),
        constructed_projectiles=audio.get('constructed_shots',0),
        Classic_voice_submissions=audio.get('sound_submissions',0),
        authored_effects=row.get('turret_effects'),actual_input=row.get('projectile_render'),
        gameplay_report_sha256=sha(args.output/'gameplay.json') if raw_report.exists() else None,
        natural_shutdown=result==0,native_XBE_argument_supplied=False,
        physical_speakers_verified=False,original_pixels_or_audio_compared=False,
        full_vehicle_parity=False,log_tail=runtime_log[-12000:],
        generic_combiner_reference_control=args.generic_combiner_control,
        private_pipeline_optimization_control=pipeline_control,elapsed_seconds=time.monotonic()-started,
        private_progress_boundaries=[line[:256] for line in runtime_log.splitlines()
            if re.match(r'^\[VEHICLE-(INPUT|FRAME|RUN-ENTER)\]',line)][:512])
    if capture:
        summary=capture.summary or {};clients=summary.get('clients',[])
        actual_output=bool(clients and all(str(client['process_id'])==str(game_pid) for client in clients) and
            summary.get('nonzero_samples',0)>0 and summary.get('sink_removed') and summary.get('default_sink_preserved'))
        report.update(native_audio_output=summary,actual_host_output_verified=actual_output,
            native_audio_capture_tool_sha256=sha(ROOT/'tools/native_gameplay_audio_capture.py'))
        report['passed'] &= actual_output
        passed=report['passed']
    if raw_frame.exists():
        from PIL import Image
        frame_path=args.output/'reference.png'
        with Image.open(raw_frame) as frame:
            if frame.size!=(800,600):raise ValueError('Unexpected actual seated viewport')
            frame.convert('RGB').save(frame_path)
        if frame_path.stat().st_size>1024*1024:raise ValueError('Required reference image exceeds bounded budget')
        report['retained_reference_image']=dict(path=str(frame_path),bytes=frame_path.stat().st_size,sha256=sha(frame_path))
    save(args.output/'report.json',report)
    print(json.dumps({key:report[key] for key in ('case','passed','actual_Vulkan_frames',
        'constructed_projectiles','Classic_voice_submissions')}),flush=True)
    return 0 if passed else 1


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    for name in ('prepared','cohort','refresh','validation','output','runtime','maps'):
        parser.add_argument('--'+name,type=Path)
    parser.add_argument('--case',choices=CASES)
    parser.add_argument('--only-case',action='append',choices=CASES)
    parser.add_argument('--case-worker',action='store_true')
    parser.add_argument('--native-audio-output',action='store_true',
        help='Require actual game PCM at its own temporary Pulse monitor; preserve the desktop default and remove raw PCM')
    parser.add_argument('--generic-combiner-control',action='store_true',
        help='Diagnostic control using the existing generic material path; never qualifies the production default')
    parser.add_argument('--disable-pipeline-optimization-control',action='store_true',
        help='Private native Vulkan control adding only DISABLE_OPTIMIZATION; never qualifies the production default')
    parser.add_argument('--specialized-pipeline-control',action='store_true',
        help='Restrict that private flag control to the existing61-word fragment specialization format')
    args=parser.parse_args()
    if args.specialized_pipeline_control:args.disable_pipeline_optimization_control=True
    if args.case_worker:return worker(args)
    if not all((args.prepared,args.cohort,args.refresh,args.validation,args.output)):
        parser.error('Supply all source receipts, completed native build validation and output')
    owner=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    prepared=json.loads(args.prepared.read_text());cohort=json.loads(args.cohort.read_text())
    refresh=json.loads(args.refresh.read_text());validation=json.loads(args.validation.read_text())
    borrowed=Path(cohort['owner']);source=borrowed/'source';build=borrowed/'build'
    if owner==borrowed or Path(prepared['runner_scratch'])!=borrowed or Path(refresh['owner'])!=borrowed or Path(validation['build'])!=build:
        raise ValueError('Separate authenticated native build owner required')
    if not all(any(row['name']==name and row['exit_code']==0 for row in validation['steps']) for name in ('configure','build')):
        raise ValueError('Native build has not completed')
    expected=dict(prepared['registered_snapshot_source_files_sha256'])
    for row in cohort['changes']:
        if expected[row['path']]!=row['before']:raise ValueError('Cohort preimage differs')
        expected[row['path']]=row['after']
    expected.update({'data/'+name:pin for name,pin in cohort['private_data_sha256'].items()})
    expected.update(refresh['refreshed_source_sha256'])
    if any(sha(source/name)!=pin for name,pin in expected.items()):raise ValueError('Frozen native source drifted')
    runtime=build/'vehicle_weapon_hud_runtime_tests';pin=sha(runtime)
    args.output=args.output.resolve();args.output.mkdir(parents=True,exist_ok=False)
    maps=Path(prepared['runtime_cache_inputs']['maps_directory'])
    chosen=args.only_case or CASES
    receipts={str(path.resolve()):sha(path) for path in (args.prepared,args.cohort,args.refresh,args.validation)}
    report=dict(passed=False,status='running',owner=str(owner),borrowed_owner=str(borrowed),
        executable_sha256=pin,source_receipts_sha256=receipts,selected_cases=list(chosen),cases=[],
        runtime_opcode_execution=False,full_vehicle_parity=False,launcher_promoted=False,
        required_reference_images_explicitly_retained=True,raw_captures_or_binaries_retained=False,
        native_audio_output_required=args.native_audio_output,
        generic_combiner_reference_control=args.generic_combiner_control,
        private_pipeline_optimization_control=args.disable_pipeline_optimization_control,
        tool_sha256=sha(Path(__file__)))
    save(args.output/'report.json',report)
    for case in chosen:
        command=[sys.executable,'-B',str(ROOT/'tools/run_test.py'),'--',sys.executable,'-B',str(Path(__file__)),
            '--case-worker','--case',case,'--runtime',str(runtime),'--maps',str(maps),'--output',str(args.output/case)]
        if args.native_audio_output:command+=['--native-audio-output']
        if args.generic_combiner_control:command+=['--generic-combiner-control']
        if args.disable_pipeline_optimization_control:command+=['--disable-pipeline-optimization-control']
        if args.specialized_pipeline_control:command+=['--specialized-pipeline-control']
        result=subprocess.run(command)
        path=args.output/case/'report.json';data=json.loads(path.read_text()) if path.exists() else {}
        report['cases'].append(dict(case=case,passed=result.returncode==0 and data.get('passed') is True,
            report=str(path),report_sha256=sha(path) if path.exists() else None,
            actual_Vulkan_frames=data.get('actual_Vulkan_frames',0),
            constructed_projectiles=data.get('constructed_projectiles',0),
            Classic_voice_submissions=data.get('Classic_voice_submissions',0),
            actual_host_output_verified=data.get('actual_host_output_verified'),
            reference_image=data.get('retained_reference_image')))
        save(args.output/'report.json',report)
    unchanged=sha(runtime)==pin and all(sha(source/name)==value for name,value in expected.items())
    # The validation report can finish concurrently; only immutable source
    # receipts and this exact executable are borrowed for these gameplay cases.
    unchanged &= all(sha(path)==receipts[str(path.resolve())] for path in (args.prepared,args.cohort,args.refresh))
    report.update(passed=unchanged and all(row['passed'] for row in report['cases']),status='completed',
        borrowed_inputs_unchanged=unchanged)
    save(args.output/'report.json',report)
    print(json.dumps(dict(passed=report['passed'],cases=len(report['cases']),report=str(args.output/'report.json'))),flush=True)
    return 0 if report['passed'] else 1


if __name__=='__main__':raise SystemExit(main())
