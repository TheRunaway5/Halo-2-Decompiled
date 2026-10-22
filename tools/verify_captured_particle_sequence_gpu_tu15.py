#!/usr/bin/env python3
"""Verify isolated native graphics for the actual240-tick Needler draw sequence.

No original NV2A output or complete game framebuffer is supplied or inferred.
"""
import argparse
import json
from pathlib import Path

from audit_rendering import file_sha

ROOT=Path(__file__).resolve().parents[1]


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--input',type=Path,required=True)
    parser.add_argument('--run',type=Path,required=True)
    parser.add_argument('--build',type=Path,required=True)
    parser.add_argument('--shaders',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args()
    data=json.loads(args.input.read_text());report=json.loads((args.run/'report.json').read_text())
    receipt_path=Path(str(args.run)+'-receipt.json');receipt=json.loads(receipt_path.read_text())
    build=json.loads((args.build/'build.json').read_text());shaders=json.loads((args.shaders/'build.json').read_text())
    assert data['passed'] and report['passed'] and receipt['returncode']==0 and build['build_passed'] and shaders['passed']
    assert receipt['report_sha256']==file_sha(args.run/'report.json') and receipt['input_manifest_sha256']==file_sha(args.input)
    assert receipt['build_sha256']==file_sha(args.build/'build.json')
    assert receipt['binary_sha256']==file_sha(args.build/'capture')==build['binary_sha256']
    assert data['all_240_gameplay_rows_unchanged'] and not data['expanded_bank_written']
    assert not data['source_state_mutated'] and not report['source_inputs_modified']
    assert file_sha(Path(data['source_draw_file']))==data['source_draw_sha256']
    assert len(data['samples'])==len(report['samples'])==report['source_samples']==810
    for source,native in zip(data['samples'],report['samples']):
        for field in ('tick','view','first_person'):assert source[field]==native[field]
        assert native['draws']==len(source['draws']) and native['repeat_exact']
        if not source['draws']:assert native['changed_pixels']==0
    assert report['draws']==data['draws']==sum(row['draws'] for row in report['samples'])
    assert report['observed_descriptor_backed_draw_states']==report['draws']*2
    assert report['complete_bound_vertex_depth_inputs_exact'] and report['recorded_native_program_selection_exact']
    assert report['repeat_rgba_exact_all_samples'] and report['empty_source_samples_exact']
    assert report['validation_errors']==report['validation_warnings']==0
    for fp,key in ((True,'first_person_positive_samples'),(False,'world_positive_samples')):
        assert report[key]==sum(s['first_person']==fp and s['changed_pixels']>0 for s in report['samples'])>0
    images={}
    baseline=(args.run/'baseline.ppm').read_bytes().split(b'\n',3)[3]
    for peak in report['peaks']:
        image=args.run/peak['readback'];repeat=args.run/peak['repeat_readback']
        assert image.read_bytes()==repeat.read_bytes()
        magic,size,depth,rgb=image.read_bytes().split(b'\n',3)
        assert (magic,size,depth)==(b'P6',b'640 480',b'255') and len(rgb)==len(baseline)==640*480*3
        assert sum(rgb[n:n+3]!=baseline[n:n+3] for n in range(0,len(rgb),3))==peak['changed_pixels']
        assert any(s['tick']==peak['tick'] and s['view']==peak['view'] and s['first_person']==peak['first_person']
                   and s['changed_pixels']==peak['changed_pixels'] for s in report['samples'])
        for path in (image,repeat):images[path.name]=file_sha(path)
    module=[r for r in build['override_sources'].values() if r['source']==str(ROOT/'src/rendering/effects/particle_renderer.cpp')]
    assert len(module)==1
    assert file_sha(ROOT/'src/rendering/effects/particle_renderer.cpp')==module[0]['source_sha256']
    for name,digest in shaders['original_source_sha256'].items():
        if Path(name).name!='CMakeLists.txt':assert file_sha(Path(name))==digest,name
    assert all(file_sha(args.shaders/name)==digest for name,digest in shaders['shader_files_sha256'].items())
    populated_zero=[dict(tick=s['tick'],view=s['view'],first_person=s['first_person'],draws=s['draws'])
        for s in report['samples'] if s['draws'] and not s['changed_pixels']]
    result=dict(passed=True,scope=__doc__,source_samples=810,source_ticks=240,draws=report['draws'],
        authored_particle_definitions=report['authored_particle_materials'],effect_totals=data['effect_totals'],
        world_positive_samples=report['world_positive_samples'],first_person_positive_samples=report['first_person_positive_samples'],
        empty_source_samples=report['empty_source_samples'],empty_source_samples_exact=True,
        populated_source_samples_without_changed_pixels=populated_zero,original_expected_visibility_of_these_samples_verified=False,
        all_240_gameplay_rows_unchanged=True,all_samples_repeat_rgba_exact=True,
        complete_descriptor_backed_vertex_depth_inputs_exact=True,recorded_native_program_selection_exact=True,
        observed_draw_states=report['observed_descriptor_backed_draw_states'],
        current_particle_renderer_module_matches_tested=True,current_particle_renderer_module_sha256=module[0]['source_sha256'],
        current_shader_inputs_match_tested=True,peaks=report['peaks'],readback_sha256=images,
        validation_errors=0,validation_warnings=0,gpu=report['gpu'],
        input_sha256=file_sha(args.input),source_report_sha256=data['source_report_sha256'],
        graphics_report_sha256=file_sha(args.run/'report.json'),graphics_receipt_sha256=file_sha(receipt_path),
        graphics_build_sha256=file_sha(args.build/'build.json'),shader_build_sha256=file_sha(args.shaders/'build.json'),
        native_isolated_particle_sequence_verified=True,new_full_source_banks_written=False,
        original_all_calls_compared=False,original_vertex_arithmetic_verified=False,original_framebuffer_comparison_verified=False,
        rendered_sequence_verified=False,audible_playback_verified=False,physical_xbox_verified=False,full_weapon_parity=False,
        remaining='Complete original feature/owner comparisons, BSP/models/hands/occlusion, complete transparent/glow/contrail/postprocess composition, matched TU1.5 framebuffer/audio sequences, cold startup and all scoped variants.',
        tool_sha256=file_sha(Path(__file__)))
    assert not args.output.exists();args.output.write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps({key:result[key] for key in ('passed','source_samples','draws','world_positive_samples','first_person_positive_samples','observed_draw_states')}))


if __name__=='__main__':main()
