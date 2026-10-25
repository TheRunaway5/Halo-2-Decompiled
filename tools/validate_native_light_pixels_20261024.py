#!/usr/bin/env python3
"""Compare actual first-person sphere-light pixels with a post-admission draw omission.

Native source state, light admission and raw packets remain identical. Only the
existing final GPU diagnostic omission removes the actual light draws. This is
owned component gameplay, not launcher-selected or physical Xbox pixel proof.
"""
import hashlib,json,os,subprocess,sys,tempfile
from pathlib import Path
from PIL import Image,ImageChops
from build_launcher_engine import source_hashes
from validate_weapon_runtime_tu15 import private_display
ROOT=Path(__file__).resolve().parents[1]
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def main():
    owner=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True);source=owner/'source';base=ROOT/'analysis/combat-presentation-audit-20261022/repair-native-attached-light-collection'
    pins=json.loads((ROOT/'analysis/combat-presentation-audit-20261022/repair-followup-20261023/source-hashes.json').read_text());assert source_hashes(source)==pins
    candidates=[p for p in base.glob('attempt-*/receipt.json') if json.loads(p.read_text()).get('local_players')==1]
    receipt=json.loads(max(candidates,key=lambda p:p.stat().st_mtime).read_text())
    binary=owner/'build/combat_native_light_query_audit'
    assert receipt['exit_code']==0 and sha(binary)==receipt['executable_sha256']
    assert sha(source/'tools/audit_combat_presentation_20261022.cpp')==receipt['fixture_sha256']
    assert sha(source/'tools/combat_native_light_query_runtime.cpp')==receipt['private_clone_sha256']
    out=base/'pixels';n=2
    while out.exists():out=base/f'pixels-{n}';n+=1
    out.mkdir();cmake=source/'CMakeLists.txt';saved=cmake.read_bytes();maps=ROOT/'assets/imported/mcc-dire-valley-menu-20261023/halo2/h2_maps_win64_dx11'
    code=1;images=[];cases=[]
    with tempfile.TemporaryDirectory(prefix='light-pixel-',dir=owner) as temporary:
      work=Path(temporary)
      try:
        with cmake.open('a') as f:
          for label in ['same-frame']:
            command=[str(binary),'fire',str(maps/'coagulation.map'),str(work/(label+'.json')),'plasma_pistol','0']
            f.write('\nadd_test(NAME native_fp_light_pixels_'+label+' COMMAND '+' '.join('"'+x+'"' for x in command)+')\n')
            env=['HALO2_AUDIT_LOCAL_PLAYERS=1','HALO2_AUDIT_CAPTURE_FP_LIGHT=1','HALO2_AUDIT_PAIRED_LIGHT_PIXELS=1','HALO2_AUDIT_LIGHT_FLOOR=1','HALO2_COMBAT_AUDIT_CAPTURE='+str(work/(label+'.ppm'))]
            if label=='without-light':env+=['HALO2_REFERENCE_DISABLE_MODEL_EFFECT_LIGHTS=1','HALO2_REFERENCE_DISABLE_UNIT_EFFECT_LIGHTS=1','HALO2_REFERENCE_DISABLE_BSP_EFFECT_LIGHTS=1']
            f.write('set_tests_properties(native_fp_light_pixels_'+label+' PROPERTIES ENVIRONMENT "'+';'.join(env)+'")\n')
        with private_display(work) as display:
          env=dict(os.environ,DISPLAY=display,SDL_VIDEODRIVER='x11',SDL_AUDIODRIVER='dummy',XDG_CACHE_HOME=str(owner/'graphics-cache'))
          command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--build-worker','--source',str(source),
              '--target','mcc_fp_sphere_material_tests','--filter','^native_fp_light_pixels_',
              '--jobs','2','--test-jobs','1','--output',str(out/'validation')]
          with (out/'command.log').open('w') as log:code=subprocess.run(command,env=env,stdout=log,stderr=subprocess.STDOUT).returncode
        for label in ['baseline','repeat','without-light']:
          p=work/'same-frame.json';d=json.loads(p.read_text());packets=d['actual_raw_first_person_light_packets']
          cases.append(dict(label=label,raw_packets=packets,tick=packets[-1]['tick'] if packets else None,status=d['runtime_status'],source_pitch=d['input_samples'][-1]['source_pitch']))
          with Image.open(work/('same-frame.ppm.'+label+'.ppm')) as image:images.append(image.convert('RGB'))
        repeat_equal=images[0].tobytes()==images[1].tobytes()
        diff=ImageChops.difference(images[0],images[2]);bbox=diff.getbbox();changed=sum(a!=b for a,b in zip(images[0].getdata(),images[2].getdata()))
        identical_packets=cases[0]['raw_packets']==cases[1]['raw_packets']==cases[2]['raw_packets']
        result=dict(scope=__doc__,exit_code=code,executable_sha256=sha(binary),fixture_sha256=receipt['fixture_sha256'],
            cases=cases,one_completed_frame_shared=True,source_and_raw_packet_collection_executed_once=True,repeat_pixels_equal=repeat_equal,identical_raw_packets=identical_packets,changed_pixels=changed,difference_box=bbox,
            passed=code==0 and repeat_equal and identical_packets and changed>0 and all(c['raw_packets'] and c['status']==0 and c['source_pitch']<-.5 for c in cases),
            original_Xbox_pixels_compared=False,launcher_gameplay_verified=False,retained_references=[])
        for label,image in [('active-first-person-light',images[0]),('without-final-light-draws',images[2])]:
          p=out/(label+'.png');image.save(p);assert p.stat().st_size<1024*1024
          result['retained_references'].append(dict(path=str(p),bytes=p.stat().st_size,sha256=sha(p),reason='Required paired rendered regression reference'))
      finally:cmake.write_bytes(saved)
    assert source_hashes(source)==pins
    result.update(private_pixel_scratch_removed=not Path(temporary).exists(),production_unchanged=True)
    (out/'receipt.json').write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps({k:v for k,v in result.items() if k not in ['scope','cases']}),flush=True)
    return 0 if result['passed'] else 1
if __name__=='__main__':raise SystemExit(main())
