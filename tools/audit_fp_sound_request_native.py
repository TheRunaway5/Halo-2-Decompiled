#!/usr/bin/env python3
"""Read-only original evidence verification and new bounded sound checkpoint."""
import argparse, hashlib, json, os, shutil, subprocess, sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'))
from xbe import Xbe

def sha(path):
    h=hashlib.sha256()
    with path.open('rb') as stream:
        for chunk in iter(lambda:stream.read(1024*1024),b''):h.update(chunk)
    return h.hexdigest()
def save(path,value):
    raw=(json.dumps(value,indent=2,sort_keys=True)+'\n').encode()
    path.parent.mkdir(parents=True,exist_ok=True)
    if path.exists():assert path.read_bytes()==raw,'Existing checkpoint differs: '+str(path)
    else:path.write_bytes(raw)
def pin(path):return str(path.relative_to(ROOT)) if path.is_relative_to(ROOT) else str(path)
def copy(source,target):
    value=sha(source);target.parent.mkdir(parents=True,exist_ok=True)
    if target.exists():assert sha(target)==value,'Existing snapshot differs'
    else:shutil.copy2(source,target)
    assert sha(source)==value and sha(target)==value
    return value
def run(command,out,environment=None):
    env=os.environ.copy();env.update(environment or {})
    result=subprocess.run(command,cwd=ROOT,env=env,capture_output=True,text=True)
    save(out,dict(command=command,environment=environment or {},exit_code=result.returncode,
                  stdout=result.stdout,stderr=result.stderr))
    assert result.returncode==0,result.stdout+result.stderr

def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--output',type=Path,default=ROOT/'analysis/multiplayer/integration/fp-sound-request-native')
    ap.add_argument('--build-native',action='store_true')
    args=ap.parse_args();out=args.output.resolve();protected={}
    for path,keys in [
        (ROOT/'analysis/multiplayer/integration/animation-variant/checkpoint-manifest.json',
         ('files','source_pins','verified_prior_pins')),
        (ROOT/'analysis/multiplayer/integration/reported-failures-20261008/frozen/manifest.json',
         ('artifact_pins','protected_prior_pins','owned_input_pins'))]:
        protected[pin(path)]=sha(path);manifest=json.loads(path.read_text())
        for key in keys:
            for filename,value in manifest[key].items():
                assert filename not in protected or protected[filename]==value
                protected[filename]=value
    for filename,value in protected.items():assert sha(ROOT/filename)==value,filename
    repeated={}
    for a,b in [('controls-canonical','controls-canonical-repeat'),('u5-final','u5-final-repeat')]:
        left={str(x.relative_to(out/a)):sha(x) for x in (out/a).rglob('*') if x.is_file()}
        right={str(x.relative_to(out/b)):sha(x) for x in (out/b).rglob('*') if x.is_file()}
        assert left==right
        repeated[a]=left
    report=json.loads((out/'u5-final/report.json').read_text());images=[]
    for filename,value in report['image_sha256'].items():
        path=Path(filename);assert sha(path)==value
        images.append(Xbe(path));protected[filename]=value
    # OLD/NEW report paths are not sorted image order; choose by pinned hash.
    old=next(x for x in images if hashlib.sha256(x.data).hexdigest().startswith('aaea4c9c'))
    new=next(x for x in images if hashlib.sha256(x.data).hexdigest().startswith('21b207e7'))
    def read(image,address,size):
        section=next(x for x in image.sections if x.va<=address and address+size<=x.va+len(image.section_bytes(x)))
        return image.section_bytes(section)[address-section.va:address-section.va+size]
    instructions=0
    for file in (out/'u5-final/instructions').glob('*.asm'):
        image=old if file.stem.endswith('-old') else new
        for line in file.read_text().splitlines():
            address,encoded,*_=line.split();raw=bytes.fromhex(encoded)
            assert read(image,int(address,16),len(raw))==raw
            instructions+=1
    providers=json.loads((out/'u5-final/providers.json').read_text())
    for v in providers['vectors']:
        for image,pointer in [(old,'old_pointer'),(new,'u5_pointer')]:
            assert read(image,int(v[pointer],16),12).hex()==v['raw12_hex']
    for c in providers['callbacks']:
        for image,address,table in [(old,'old_address','old_table32_hex'),(new,'u5_address','u5_table32_hex')]:
            assert read(image,int(c[address],16),32).hex()==c[table]
    assert providers['current_bsp_word']['runtime_owner_required']
    source_files=['include/halo2/fp_sound_request.hpp','src/fp_sound_request.cpp',
        'tests/fp_sound_request_tests.cpp','tools/research_fp_sound_request_u5.py',
        'tools/oracle_fp_sound_request_controls_1_1.py','tools/export_fp_sound_request_fixtures.py',
        'tools/audit_fp_sound_request_native.py','docs/MP_FP_SOUND_REQUEST_NATIVE_INTEGRATION.md']
    sources={}
    for filename in source_files:
        sources[filename]=copy(ROOT/filename,out/'native/source'/filename)
    # Observed complete header tree for the new native compilation. It does
    # not claim freshness/provenance of every object in the archived Cache.
    headers={}
    for path in (ROOT/'include/halo2').rglob('*'):
        if path.is_file():headers[pin(path)]=copy(path,out/'native/source'/path.relative_to(ROOT))
    if args.build_native:
        for name,cxx,flags,cache in [
            ('normal-pure','g++',['-O2'],False),
            ('clang-asan','clang++',['-O1','-fsanitize=address,undefined','-fno-omit-frame-pointer'],False),
            ('normal','g++',['-O2'],True),
            ('gcc-asan','g++',['-O1','-fsanitize=address,undefined','-fno-omit-frame-pointer'],True)]:
            path=out/'native'/name;path.mkdir(parents=True,exist_ok=True)
            command=[cxx,'-std=c++20','-g','-Wall','-Wextra','-Wpedantic','-Werror','-ffp-contract=off',*flags,
                '-I'+str(out/'native/source/include'),str(out/'native/source/src/fp_sound_request.cpp'),
                str(out/'native/source/tests/fp_sound_request_tests.cpp')]
            if cache:command+=['-DHALO2_FP_SOUND_CACHE_TESTS=1',str(path/'libhalo2_cache.a'),
                '/usr/lib/libglm.a','-lSDL2','-lavformat','-lavcodec','-lswresample','-lswscale','-lavutil']
            command+=['-o',str(path/'fp_sound_request_tests')]
            run(command,path/'compile.json')
            environment={} if 'asan' not in name else dict(ASAN_OPTIONS='detect_leaks=1:halt_on_error=1',UBSAN_OPTIONS='halt_on_error=1')
            command=[str(path/'fp_sound_request_tests'),str(out/'canonical-fixtures.tsv')]
            if cache:command+=[str(ROOT/'assets/disc/maps/lockout.map')]
            run(command,path/'run.json',environment)
    # Repeat fixture export uses independent control/provider report paths.
    run([str(ROOT/'.tools/python/bin/python'),str(ROOT/'tools/export_fp_sound_request_fixtures.py'),
         str(out/'controls-canonical-repeat/report.json'),str(out/'u5-final-repeat/providers.json'),
         str(out/'repeat-fixtures.tsv')],out/'repeat-export.json')
    assert (out/'canonical-fixtures.tsv').read_bytes()==(out/'repeat-fixtures.tsv').read_bytes()
    assert (out/'canonical-fixtures.json').read_bytes()==(out/'repeat-fixtures.json').read_bytes()
    for filename,value in protected.items():assert sha(ROOT/filename)==value,filename
    save(out/'audit.json',dict(schema='fp-sound-request-native-audit-v1',instruction_rows=instructions,
        complete_original_bodies=11,independent_repeat_files=sum(len(x) for x in repeated.values()),
        original_controls=21,authored_requests=3,source_snapshots=sources,observed_header_snapshots=headers,
        protected_prior_pins=protected,repeat_files=repeated,
        native_tests=['normal-pure','clang-asan','normal','gcc-asan'],
        live_world_location_pose_provider=False,live_admission_selection_voice_owner=False,
        live_first_person_sound_playback=False,all_map_event_sound_parity=False,mismatches=[]))
    files={pin(path):sha(path) for path in out.rglob('*') if path.is_file() and path.name!='checkpoint-manifest.json'}
    save(out/'checkpoint-manifest.json',dict(schema='fp-sound-request-native-checkpoint-v1',files=files,
        source_pins=sources,verified_prior_pins=protected,
        canonical_inputs=['controls-canonical','controls-canonical-repeat','u5-final','u5-final-repeat','canonical-fixtures.tsv','canonical-fixtures.json'],
        exploratory_outputs_not_authority=['u5','u5-repeat','u5-body-initial','controls','controls-final','controls-repeat','fixtures.tsv','fixtures.json'],
        live_first_person_sound_playback=False,scope='Bounded native producer with required real lower-owner and source scheduler providers. No available audio/system success, RNG initialization, listener, voice or world pose fabricated. Observed Cache archives are exact copied inputs, not whole current tree reproducibility claims.'))
    print(json.dumps({'files':len(files),'protected_prior_pins':len(protected),'instructions':instructions,
                      'manifest_sha256':sha(out/'checkpoint-manifest.json')}))

if __name__=='__main__':main()
