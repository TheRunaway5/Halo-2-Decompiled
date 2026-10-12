#!/usr/bin/env python3
"""Compare captured carrier glow inputs and rendered stage images without tuning.

The fixed original frame has three groups of 32 fan quads in marker order
2, 0, 1. The native path submits two triangles for each of those same quads.
Screen/depth, packed-color conversion, UV, and post-VS comparisons remain
separate from the framebuffer comparison and the rest of the title screen.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct

import numpy as np
from PIL import Image

ROOT = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--original', type=Path, default=ROOT/'analysis/frontend/xemu-capture/original-title-glow-group.json')
    parser.add_argument('--native', type=Path, required=True)
    parser.add_argument('--original-image-prefix', type=Path, default=ROOT/'analysis/frontend/xemu-capture/original-glow')
    parser.add_argument('--native-image-prefix', type=Path, required=True)
    parser.add_argument('--original-oracle', type=Path, default=ROOT/'analysis/frontend/title-light-volume-differential.json')
    parser.add_argument('--interleaving', type=Path, default=ROOT/'analysis/frontend/xemu-capture/original-glow-interleaving.json')
    parser.add_argument('--original-pipeline', type=Path, default=ROOT/'analysis/frontend/xemu-capture/title-light-volume-capture.json')
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    original = json.loads(args.original.read_text())
    native = json.loads(args.native.read_text())
    oracle = json.loads(args.original_oracle.read_text())
    source_pipeline = json.loads(args.original_pipeline.read_text())['passes'][0]
    source_sampler = source_pipeline['resources'][0]['sampler']
    sampler_fields = ('addressU','addressV','addressW','filter','minLOD','maxLOD',
        'mipBias','maxAnisotropy','borderColorType','borderColorValue','unnormalized')
    blend_fields = ('enabled','colorBlend','alphaBlend','writeMask','logicOperationEnabled')
    assert len(original['draws']) == 96 and len(native['draws']) == 3
    checks = []
    for i, source in enumerate(original['draws']):
        group = native['draws'][i//32]
        assert group['action']['numIndices'] == 192
        at = i%32*6
        indices = [at, at+1, at+2, at+5]
        vertex = np.asarray([group['vertices'][j] for j in indices])
        post = np.asarray([group['postvs_rows'][j] for j in indices])
        expected = np.asarray(source['postvs'])
        screen = np.asarray(source['screen'])
        byte_equal = lambda a,b: struct.pack('<'+str(a.size)+'f', *a.flat) == struct.pack('<'+str(b.size)+'f', *b.flat)
        checks.append({'marker': [2,0,1][i//32], 'sprite': i%32,
            'original_event': source['event'], 'native_event': group['event'],
            'screen_xyzw_byte_exact': byte_equal(vertex[:,:4], screen),
            'vertex_color_byte_exact': byte_equal(vertex[:,4:8], expected[:,8:12]),
            'uv_byte_exact': byte_equal(vertex[:,8:10], expected[:,28:30]),
            'postvs_clip_byte_exact': byte_equal(post[:,:4], expected[:,:4]),
            'maximum_screen_component_error': np.abs(vertex[:,:4]-screen).max(axis=0).tolist(),
            'maximum_clip_component_error': np.abs(post[:,:4]-expected[:,:4]).max(axis=0).tolist(),
            'maximum_ndc_error': np.abs(post[:,:3]/post[:,3:4]-expected[:,:3]/expected[:,3:4]).max(axis=0).tolist()})
    report = {'original_capture':original['capture'],'native_capture':native['capture'],
        'original_extract_sha256':hashlib.sha256(args.original.read_bytes()).hexdigest(),
        'native_extract_sha256':hashlib.sha256(args.native.read_bytes()).hexdigest(),
        'compared_quads':96,'compared_vertices':384,'checks':checks,
        'all_screen_xyzw_byte_exact':all(c['screen_xyzw_byte_exact']for c in checks),
        'all_vertex_colors_byte_exact':all(c['vertex_color_byte_exact']for c in checks),
        'all_uv_byte_exact':all(c['uv_byte_exact']for c in checks),
        'all_postvs_clip_byte_exact':all(c['postvs_clip_byte_exact']for c in checks),
        'all_pixel_program_words_exact':all(d['program']==oracle['original_pixel_program_words']for d in native['draws']),
        'all_depth_states_exact':all(d['depth']==source_pipeline['depth']for d in native['draws']),
        'all_active_blend_states_exact':all(all(d['blend']['blends'][0][k]==source_pipeline['blend']['blends'][0][k]
            for k in blend_fields)for d in native['draws']),
        'all_sampler_filter_address_lod_states_exact':all(all(d['resources'][0]['sampler'][k]==source_sampler[k]
            for k in sampler_fields)for d in native['draws']),
        'maximum_screen_component_error':np.asarray([c['maximum_screen_component_error']for c in checks]).max(axis=0).tolist(),
        'maximum_clip_component_error':np.asarray([c['maximum_clip_component_error']for c in checks]).max(axis=0).tolist(),
        'whole_frame_parity_verified':False}
    images={}
    report['framebuffer_stages']={}
    for phase in ('before','after'):
        for label,prefix in (('original',args.original_image_prefix),('native',args.native_image_prefix)):
            images[label,phase]=np.asarray(Image.open(str(prefix)+'-'+phase+'.png').convert('RGB')).astype(np.int16)
        diff=np.abs(images['native',phase]-images['original',phase])
        report['framebuffer_stages'][phase]={'maximum_byte_error':int(diff.max()),
            'mean_byte_error':float(diff.mean()),'exact_pixels':int(np.all(diff==0,axis=2).sum()),
            'within_one_byte_pixels':int(np.all(diff<=1,axis=2).sum())}
    delta={label:images[label,'after']-images[label,'before']for label in ('original','native')}
    mask=np.any(delta['original']!=0,axis=2)|np.any(delta['native']!=0,axis=2)
    diff=np.abs(delta['original']-delta['native'])
    report['stage_delta']={'changed_pixels':int(mask.sum()),
        'exact_changed_pixels':int(np.all(diff==0,axis=2)[mask].sum()),
        'within_one_byte_changed_pixels':int(np.all(diff<=1,axis=2)[mask].sum()),
        'maximum_byte_error':int(diff[mask].max()),
        'isolated_glow_framebuffer_parity_verified':False,
        'limit':'Stage deltas include the original transparent scheduler between glow groups.'}
    interleaving=json.loads(args.interleaving.read_text())
    report['other_original_draws_between_glow_groups']=len(interleaving['other_draws'])
    report['other_original_fragment_shaders']=sorted({d['fragment_shader']for d in interleaving['other_draws']})
    args.output.parent.mkdir(parents=True,exist_ok=True)
    args.output.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({k:v for k,v in report.items()if k!='checks'},indent=2))
    return 0 if all(report[k]for k in ('all_screen_xyzw_byte_exact','all_vertex_colors_byte_exact',
        'all_uv_byte_exact','all_postvs_clip_byte_exact','all_pixel_program_words_exact',
        'all_depth_states_exact','all_active_blend_states_exact','all_sampler_filter_address_lod_states_exact')) else 1


if __name__ == '__main__':
    raise SystemExit(main())
