#!/usr/bin/env python3
"""Record source/captured haze comparisons and their explicit parity limits."""
import argparse,hashlib,json
from pathlib import Path
import numpy as np
from PIL import Image,ImageDraw

ROOT=Path(__file__).resolve().parents[1]
base=ROOT/'analysis/frontend'
capture=base/'xemu-capture'
fixture=capture/'haze-fixture'
results=ROOT/'build-frontend/haze-capture-results'
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--inspection',type=Path,default=capture/'native-haze-cpu-constants-quads-state.json')
parser.add_argument('--aspects',type=Path,default=base/'title-haze-cpu-constants-final-aspects/validation.json')
parser.add_argument('--test-log',type=Path,default=base/'title-haze-cpu-constants-tests.log')
parser.add_argument('--output-name',default='title-haze-cpu-constants')
args=parser.parse_args()

def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def compare_paths(expected_path,native_path,width,height,channels):
    expected=np.frombuffer(expected_path.read_bytes(),np.uint8).reshape(height,width,4)
    actual=np.frombuffer(native_path.read_bytes(),np.uint8).reshape(height,width,4)
    delta=np.abs(actual.astype(np.int16)-expected.astype(np.int16))[:,:,:channels]
    differences=[{'x':int(x),'y':int(y),'channel':int(c),'original':int(expected[y,x,c]),'native':int(actual[y,x,c])}
                 for y,x,c in np.argwhere(delta)[:8]]
    return {'pixels':width*height,'compared_channels':channels,'different_channels':int(np.count_nonzero(delta)),
        'channels_above_one_byte':int(np.count_nonzero(delta>1)),'maximum_byte_error':int(delta.max()),
        'all_compared_bytes_exact':not bool(delta.any()),'examples':differences,
        'expected_sha256':sha(expected_path),'native_sha256':sha(native_path)}

def compare(name,width,height,channels):
    return compare_paths(fixture/f'{name}-output.rgba',results/f'{name}.rgba',width,height,channels)

def main():
    constructors=[json.loads((base/f'title-haze-{name}.json').read_text())for name in ('quad','direct-quad')]
    assert all(c['all_attribute_bytes_exact']for c in constructors)
    sampled={'noise':compare('noise',320,240,4),'direct':compare('composite',640,480,4)}
    assert all(s['all_compared_bytes_exact']for s in sampled.values())
    bloom={str(event):compare_paths(capture/f'bloom-fixture/{event}.rgba',ROOT/f'build-frontend/bloom-capture-results/{event}.rgba',
        640 if event>=8118 else 160,480 if event>=8118 else 120,4)
        for event in (8048,8062,8076,8090,8104,8118,8132)}
    assert all(s['all_compared_bytes_exact']for s in bloom.values())
    bloom_manifest=json.loads((capture/'bloom-fixture/manifest.json').read_text())
    assert bloom_manifest['includes_authored_post_bloom_filter']
    filter_pass=[p for p in bloom_manifest['passes']if p['event']==8132]
    assert len(filter_pass)==1 and filter_pass[0]['actual_input_matches_bloom_output']
    native=json.loads(args.inspection.read_text())
    assert all(p['float_constants_match_cpu_expansion']and p['inline_alpha_one_before_interpolation']for p in native['passes'])
    history=json.loads((capture/'title-haze-348-268-cpu-constants-history.json').read_text())
    original_pixel=history['original']['history'][-1]
    native_pixel=history['native']['history'][-1]
    assert all(original_pixel[key]['col']['uintValue']==native_pixel[key]['col']['uintValue']
               for key in ('preMod','shaderOut','postMod'))
    aspects=json.loads(args.aspects.read_text())
    runtime=sha(ROOT/'build-frontend/halo2_native')
    assert aspects['passed'] and all(c['binary_sha256']==runtime for c in aspects['checks'])
    source_paths=('src/patchy_fog.cpp','include/halo2/atmosphere.hpp','shaders/patchy_noise.vert',
        'shaders/patchy_noise.frag','shaders/patchy_composite.frag','src/renderer.cpp',
        'shaders/patchy_constants.glsl','shaders/combiner.glsl',
        'tests/patchy_fog_quad_tests.cpp','tests/menu_haze_capture_gpu_tests.cpp',
        'tools/export_title_haze_fixture.py','tests/menu_bloom_capture_gpu_tests.cpp',
        'tools/export_title_bloom_fixture.py','shaders/bloom.vert','shaders/bloom.frag','shaders/bloom_color.glsl')
    shaders=('patchy_noise.vert','patchy_noise.frag','patchy_composite.frag','bloom.vert','bloom.frag')
    report={'scope':'completed source haze quads, production sampled GPU passes and live title presentation; original camera/RNG history and complete scene/frontend parity remain unverified',
        'goal_scope_file':'docs/FRONTEND_PARITY_GOAL.md','coverage_file':'analysis/frontend/menu-parity-coverage.json',
        'constructors':constructors,'source_attribute_bytes':sum(c['original_bytes']for c in constructors),
        'sampled_gpu_passes':sampled,'runtime_sha256':runtime,
        'captured_bloom_and_filter_passes':bloom,'captured_bloom_and_filter_rgba_pixels':sum(s['pixels']for s in bloom.values()),
        'original_filter_input_matches_bloom_output':True,
        'gpu_test_sha256':sha(ROOT/'build-frontend/menu_haze_capture_gpu_tests'),
        'bloom_filter_gpu_test_sha256':sha(ROOT/'build-frontend/menu_bloom_capture_gpu_tests'),
        'production_shader_sha256':{s:sha(ROOT/f'build-frontend/shaders/{s}.spv')for s in shaders},
        'source_sha256':{s:sha(ROOT/s)for s in source_paths},
        'native_pipeline_inspection':str(args.inspection.relative_to(ROOT)),
        'native_capture_sha256':native['capture_sha256'],
        'native_capture_file':native['capture_file'],
        'focused_tests_log':str(args.test_log.relative_to(ROOT)),
        'aspect_validation_report':str(args.aspects.relative_to(ROOT)),
        'live_corner_submission_verified':True,'source_noise_corner_constructor_verified':True,
        'source_direct_corner_constructor_verified':True,'captured_noise_rgba_exact':True,
        'captured_composition_rgb_exact':sampled['direct']['all_compared_bytes_exact'],
        'captured_composition_rgba_exact':sampled['direct']['all_compared_bytes_exact'],
        'captured_combiner_constant_bytes_exact':True,'live_cpu_expanded_constant_upload_verified':True,
        'captured_bloom_filter_chain_rgba_exact':True,
        'hardware_operand_probe':'analysis/frontend/xemu-capture/title-haze-348-268-sampled-operands.json',
        'hardware_fixed_pixel_history':'analysis/frontend/xemu-capture/title-haze-348-268-cpu-constants-history.json',
        'hardware_shader_output_bits_exact_at_previous_failure':True,
        'original_live_random_and_camera_history_verified':False,'whole_frame_parity_verified':False,
        'extended_scene_aspect_support':False,'all_menu_layout_flow_verified':False,
        'remaining':['original live fog random/camera history','complete source scene lighting/effects/scheduling',
            'full-width scenery','all required frontend layout/navigation routes and aspect comparisons']}
    (base/f'{args.output_name}-evidence.json').write_text(json.dumps(report,indent=2)+'\n')
    gallery=Image.new('RGB',(1280,510),(12,16,27));draw=ImageDraw.Draw(gallery)
    for index,(label,path) in enumerate((('Original Xemu Vulkan haze pass',fixture/'composite-output.rgba'),
        ('Native haze on captured inputs',results/'composite.rgba'))):
        draw.text((index*640+12,8),label,fill=(231,237,246))
        gallery.paste(Image.frombytes('RGBA',(640,480),path.read_bytes()).convert('RGB'),(index*640,30))
    gallery.save(base/f'{args.output_name}-comparison.png')
    print(json.dumps({'runtime_sha256':runtime,'source_attribute_bytes':report['source_attribute_bytes'],
        'sampled_gpu_passes':sampled,'presentation_passed':aspects['passed'],
        'whole_frame_parity_verified':False,'all_menu_layout_flow_verified':False},indent=2))

if __name__=='__main__':main()
