#!/usr/bin/env python3
"""Finite live native/original44710 large-sphere render qualification.

Use run_test.py. Captures, previews and complete command histories remain in
HALO2_TEST_SCRATCH; only hashes, coverage and bounded diagnostics survive.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import struct
from perf_4k60 import parity
from validate_weapon_runtime_tu15 import private_display


def sha(path):
    with Path(path).open('rb')as stream:return hashlib.file_digest(stream,'sha256').hexdigest()


def command_records(data, transparent=None):
    """Decode each full-capacity record, independently of its emitted counts."""
    at=0;contexts={}
    while at<len(data):
        if len(data)-at<180:raise ValueError('Truncated complete command-bank trace')
        header=struct.unpack_from('<6I',data,at)
        if header[0] not in (0x40e80001,0x40e80002):raise ValueError('Unknown command-bank trace schema')
        capacity=[struct.unpack_from('<I',data,at+24+24*b)[0]for b in range(3)]
        if any(n>16*1024*1024//24 for n in capacity):raise ValueError('Command-bank capacity escaped its recorded format')
        size=180+24*sum(capacity)
        if size>len(data)-at:raise ValueError('Incomplete retained command-bank capacity')
        if header[0]==0x40e80002:
            if len(data)-at-size<9:raise ValueError('Missing transparent-pool trace header')
            pointer,extent=struct.unpack_from('<II',data,at+size)
            count=struct.unpack_from('<I',data,at+104)[0]
            if extent!=(0x400*0x70 if pointer else 0) or count>0x400 or (count and not pointer):
                raise ValueError('Transparent-pool trace has no actual retained capacity')
            size+=9+extent
            if size>len(data)-at:raise ValueError('Incomplete retained transparent-pool capacity')
            if transparent is not None:transparent.append((header[2],count,bool(pointer)))
        contexts[header[2]]=contexts.get(header[2],0)+1;at+=size
    return contexts


def runtime_evidence(log,bank):
    run={}
    with log.open('rb')as stream:
        stream.seek(max(0,log.stat().st_size-4*1024*1024));text=stream.read().decode(errors='replace')
    rows=re.findall(r'Sphere mode0 stats: receivers=(\d+) entries=(\d+) model_entries=(\d+) '
        r'exhausted=(\d+) reference=(\d+) mismatches=(\d+)',text)
    for n,key in enumerate(('receivers','entries','model_entries','exhausted','reference_calls','mismatches')):
        run[key]=sum(int(row[n])for row in rows)
    commands=re.findall(r'Light40E80 stats: native=(\d+) reference=(\d+)',text)
    run['native_command_calls']=sum(int(row[0])for row in commands)
    run['original_command_calls']=sum(int(row[1])for row in commands)
    parts=re.findall(r'Part45CD0 stats: native=(\d+) reference=(\d+) residual=(\d+) models=(\d+) plane_hits=(\d+) comparisons=(\d+) mismatches=(\d+)',text)
    for n,key in enumerate(('native','reference','residual','models','plane_hits','comparisons','mismatches')):
        run['part_'+key]=sum(int(row[n])for row in parts)
    emits=re.findall(r'Emit41030 stats: native=(\d+) reference=(\d+) residual=(\d+) banks=(\d+) transparent=(\d+)',text)
    for n,key in enumerate(('native','reference','residual','banks','transparent')):
        run['emit_'+key]=sum(int(row[n])for row in emits)
    main=re.findall(r'Native main cull: views=(\d+) native_entries=(\d+) residual_entries=(\d+) fallbacks=(\d+) '
        r'unit_mismatches=(\d+) mask_mismatches=(\d+) fp_mismatches=(\d+) native_model_entries=(\d+) residual_model_entries=(\d+)',text)
    for n,key in enumerate(('views','native_entries','residual_entries','fallbacks','unit_mismatches',
        'mask_mismatches','fp_mismatches','native_model_entries','residual_model_entries')):
        run['main_'+key]=sum(int(row[n])for row in main)
    run['diagnostic_errors']=[line[:1200]for line in text.splitlines()if
        any(marker in line for marker in('Large-sphere','Source large-sphere','Ordered native44E80',
            'Source linear frustum','Source frustum cull','source BSP frustum','live model cull','Native45CD0','Native41030'))][-12:]
    run['vulkan_validation_errors']=[line[:1600]for line in text.splitlines()
        if 'Vulkan validation:'in line and ('Validation Error'in line or 'VUID-'in line)][-8:]
    data=bank.read_bytes()if bank.is_file()else b''
    try:
        transparent=[];contexts=command_records(data,transparent);run['bank_context_records']={f'{k:08x}':v for k,v in contexts.items()}
        run['main_bank_records']=contexts.get(0x556e18,0);run['light_bank_records']=contexts.get(0x5598f8,0)
        run['all_transparent_capacities_traced']=bool(transparent) and len(transparent)==sum(contexts.values()) and all(t[2] for t in transparent)
        run['transparent_main_max_count']=max((count for ctx,count,_ in transparent if ctx==0x556e18),default=0)
        run['every_light_command_traced']=run['main_bank_records']>0 and run['light_bank_records']>0 and \
            run['light_bank_records']==run['native_command_calls']+run['original_command_calls']
    except ValueError as error:
        run.update(every_light_command_traced=False,bank_format_error=str(error))
    run.update(bank_bytes=len(data),bank_sha256=hashlib.sha256(data).hexdigest())
    return run,data


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--runtime',type=Path,required=True);p.add_argument('--maps',type=Path,required=True)
    p.add_argument('--xbe',type=Path,required=True)
    p.add_argument('--map',default='coagulation');p.add_argument('--case',default='1:170:10:110')
    p.add_argument('--size',type=int,nargs=2,default=(1920,1080))
    p.add_argument('--input-script',type=Path)
    p.add_argument('--auxiliary-seed',type=int,default=1)
    p.add_argument('--starting-weapon',type=int,choices=(0,*range(3,21)),default=8)
    p.add_argument('--look',type=float,nargs=2,default=(1.35,-.6))
    p.add_argument('--camera',type=float,nargs=9,help='Explicit diagnostic position, forward and up; same view in every run')
    p.add_argument('--probe-only',action='store_true');p.add_argument('--report',type=Path,required=True)
    p.add_argument('--require-main-model',action='store_true');p.add_argument('--require-light-model',action='store_true')
    p.add_argument('--require-native-part-prepare',action='store_true')
    p.add_argument('--require-native-command-emit',action='store_true')
    p.add_argument('--validation',action='store_true')
    a=p.parse_args();players,tick,first,last=map(int,a.case.split(':'))
    if not(1<=players<=4 and tick>0 and 0<=first<=last and -1.5<=a.look[1]<=1.5):p.error('Invalid finite input case')
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    runtime=a.runtime.resolve(strict=True);maps=a.maps.resolve(strict=True)
    xbe=a.xbe.resolve(strict=True)
    pins={str(path):sha(path)for path in(runtime,xbe,maps/(a.map+'.map'),Path(__file__),Path(parity.__file__),
        *(maps/name for name in ('shared.map','single_player_shared.map','mainmenu.map')if (maps/name).is_file()))}
    script='# tick player fire reload grenade yaw pitch\n'+''.join(
        f'{t} {n} {int(first<=t<last and last>0)} 0 0 {a.look[0]} {a.look[1]}\n'
        for t in range(tick)for n in range(players))
    if a.input_script:
        script=a.input_script.read_text();pins[str(a.input_script.resolve())]=sha(a.input_script)
    extra=['--gameplay-xbe',str(xbe)]
    if a.starting_weapon:extra+=['--mp-starting-weapon',str(a.starting_weapon)]
    if a.validation:extra+=['--validation']
    if a.camera:extra+=['--mp-reference-camera',*map(str,a.camera)]
    entry=dict(case=a.case,runs=[]);banks=[]
    roles=('reference',)if a.probe_only else('reference','reference-repeat','candidate','candidate-repeat')
    for role in roles:
        name='large-sphere-'+role;bank=scratch/(name+'.banks');reference=role.startswith('reference')
        env={'HALO2_REFERENCE_LIGHT_FRUSTUM_CULL':'reference'if reference else'',
            'HALO2_NATIVE_LARGE_SPHERE_STATS':'1','HALO2_COMMAND_BANKS_PATH':str(bank),
            'HALO2_REFERENCE_LIGHT_FLAGS_31650':'','HALO2_REFERENCE_BANK_DRAW_UNITS':'',
            'HALO2_REFERENCE_MODEL_DRAW_UNITS':'','HALO2_REFERENCE_LIGHT_PARENT_BIPED':'',
            'HALO2_REFERENCE_MAIN_CULL':'','HALO2_MAIN_CULL_ORACLE':'',
            'HALO2_REFERENCE_MAIN_COMMANDS':'','HALO2_REFERENCE_RENDER_PREPARE':''}
        env.update(HALO2_REFERENCE_LIGHT_COMMANDS='1'if reference else'',HALO2_NATIVE_LIGHT_COMMANDS_STATS='1')
        if a.require_native_part_prepare:env.update(HALO2_REFERENCE_PART_PREPARE='1'if reference else'',
            HALO2_PART_PREPARE_ORACLE='',HALO2_NATIVE_PART_PREPARE_STATS='1')
        if a.require_main_model:env.update(HALO2_MAIN_CULL_ORACLE='1'if reference else'',HALO2_NATIVE_CULL_WALK_STATS='1')
        if a.require_native_command_emit:env.update(HALO2_REFERENCE_COMMAND_EMIT='1'if reference else'',
            HALO2_NATIVE_COMMAND_EMIT_STATS='1',HALO2_COMMAND_BANKS_TRACE_TRANSPARENT='1')
        with private_display(scratch) as display:
            run=parity.capture(runtime,name,players,tick,first,last,env,scratch=scratch,map_name=a.map,
                maps=maps,size=tuple(a.size),timeout_seconds=300,display=display,auxiliary_seed=a.auxiliary_seed,
                extra_arguments=extra,script_text=script)
        run.update(role=role,environment=env)
        rgb=run.get('_payload',b'').split(b'\n',3)[-1]
        run['nonuniform_image']=bool(rgb)and any(rgb[i:i+3]!=rgb[:3]for i in range(3,len(rgb),3))
        if run.get('_payload')and role in('reference','candidate'):
            preview=scratch/(name+'-preview.ppm');preview.write_bytes(run['_payload']);run['scratch_preview']=str(preview)
        log=scratch/(name+'.log')
        evidence,data=runtime_evidence(log,bank);run.update(evidence);banks.append(data);entry['runs'].append(run)
        print(role,run['valid'],run.get('sha256'),run['receivers'],'mode0 receivers',run['entries'],
            'entries',run['reference_calls'],'original comparisons',run['mismatches'],'mismatches',flush=True)
    positive=all(r['receivers']>0 and r['entries']>0 for r in entry['runs'])
    reference_selected=all(r['reference_calls']==r['receivers']and r['reference_calls']>0
        for r in entry['runs']if r['role'].startswith('reference'))
    clean=all(r['mismatches']==0 and not r['diagnostic_errors'] and not r['vulkan_validation_errors']for r in entry['runs'])
    command_selection=all((r['original_command_calls']>0 and r['native_command_calls']==0)if r['role'].startswith('reference')
        else(r['native_command_calls']>0 and r['original_command_calls']==0)for r in entry['runs'])
    commands_traced=all(r['every_light_command_traced']for r in entry['runs'])
    coverage=all((not a.require_native_part_prepare or r['part_residual']==r['part_mismatches']==0 and
        (r['part_reference']>0 and r['part_native']==0 if r['role'].startswith('reference')else r['part_native']>0 and r['part_models']>0 and r['part_reference']==0))and
        (not a.require_main_model or r['main_native_model_entries']>0 and r['main_residual_model_entries']==0 and
        r['main_unit_mismatches']==r['main_mask_mismatches']==r['main_fp_mismatches']==0)and
        (not a.require_light_model or r['model_entries']>0)for r in entry['runs'])
    emit_coverage=all(not a.require_native_command_emit or r['emit_residual']==0 and
        r['all_transparent_capacities_traced'] and r['transparent_main_max_count']>0 and
        (r['emit_reference']>0 and r['emit_native']==0 if r['role'].startswith('reference') else
         r['emit_native']>0 and r['emit_reference']==0 and r['emit_banks']>0 and r['emit_transparent']>0)
        for r in entry['runs'])
    if a.probe_only:
        for run in entry['runs']:run.pop('_payload',None)
        passed=False;probe_completed=entry['runs'][0]['valid']and positive and reference_selected and clean and coverage and emit_coverage and command_selection and commands_traced
    else:
        parity.compare_runs(entry);probe_completed=None
        entry.update(positive_mode0_work=positive,reference_selected=reference_selected,diagnostics_clean=clean,
            native_selected=all(r['reference_calls']==0 for r in entry['runs']if r['role'].startswith('candidate')),
            all_images_nonuniform=all(r['nonuniform_image']for r in entry['runs']),
            complete_banks_and_fp_equal=bool(banks[0])and all(b==banks[0]for b in banks))
        entry['bank_first_differences']=[]
        for run,data in zip(entry['runs'],banks,strict=True):
            if data==banks[0]:continue
            at=next((i for i,(x,y)in enumerate(zip(banks[0],data))if x!=y),min(len(banks[0]),len(data)))
            entry['bank_first_differences'].append(dict(role=run['role'],offset=at,
                reference=banks[0][max(0,at-16):at+32].hex(),actual=data[max(0,at-16):at+32].hex()))
        passed=coverage and emit_coverage and command_selection and commands_traced and entry['passed']and all(entry[k]for k in('positive_mode0_work','reference_selected',
            'diagnostics_clean','native_selected','all_images_nonuniform','complete_banks_and_fp_equal'))
    unchanged=all(sha(path)==digest for path,digest in pins.items());passed=passed and unchanged
    report=dict(passed=passed,validation_not_run=a.probe_only,probe_completed=probe_completed,case=entry,
        inputs_unchanged=unchanged,source_runtime_map_sha256=pins,helper_sha256=sha(__file__),
        starting_weapon=a.starting_weapon,look=a.look,camera=a.camera,input_script=script,retained_large_artifacts=[],
        main_model_required=a.require_main_model,light_model_required=a.require_light_model,requested_model_coverage=coverage,
        command_emit_required=a.require_native_command_emit,command_emit_coverage=emit_coverage,vulkan_validation_requested=a.validation,
        command_selection_verified=command_selection,
        every_light_command_traced=commands_traced,
        whole_renderer_xbox_visual_parity_verified=False,
        scope='Finite same-engine large-sphere native/original cull comparison. Original44710 uses actual resident '
            'geometry and poses with the same pre-cull draw/visibility inputs, checks complete units/masks/FP and '
            'restores its writes without replacing World leases. Reference roles also select whole original secondary40E80; '
            'candidate roles use the native secondary frame and, when required, native45CD0 preparation. Four-run acceptance requires positive mode0 '
            'work, nonuniform repeatable images and identical complete command histories plus main FP. '
            'Model cull work is counted separately. No physical Xbox framebuffer or whole-renderer parity claim.')
    a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2)+'\n')
    return int(not(probe_completed if a.probe_only else passed))


if __name__=='__main__':raise SystemExit(main())
