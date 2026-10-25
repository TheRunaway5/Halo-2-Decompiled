#!/usr/bin/env python3
"""Probe whole original map activation before object births in owned scratch.

This substitutes no callback, readiness, listener, admission or voice result.
The private world clone chooses191DF0 after actual native option/resource input
delivery, in place of the manually composed new-map subset. A dependency stop
is a diagnostic failure, not a production startup or playback qualification.
"""
import hashlib,json,os,subprocess,sys
from pathlib import Path
from build_launcher_engine import source_hashes

ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/'analysis/combat-presentation-audit-20261022/repair-scenery-audio'
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def main():
    owner=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True);source=owner/'source'
    pins=json.loads((OUT.parent/'repair-followup-20261023/source-hashes.json').read_text())
    assert source_hashes(source)==pins
    original=source/'src/physics/physics_live_world_tu15.cpp'
    clone=source/'tools/object_loop_activation_world.cpp';text=original.read_text()
    needle='        call_actual(0x1490b0u);\n        call_actual(0x159ff0u);'
    assert text.count(needle)==1
    text=text.replace(needle,'''        if(std::getenv("HALO2_AUDIT_WHOLE_MAP_ACTIVATION")) {
            auto activation=frame({});activation.gp[0]=game+8u;
            run_actual(0x191df0u,activation);
            if(!(activation.gp[0]&255u) || !word(bank,game+0x1120u,1))
                throw FormatError("Whole original191DF0 did not complete actual map activation");
            session_configured=true;return;
        }
'''+needle)
    needle='    void initialize_map() {\n'
    assert text.count(needle)==1
    text=text.replace(needle,needle+'''        if(std::getenv("HALO2_AUDIT_WHOLE_MAP_ACTIVATION")) {
            if(!session_configured || !word(bank,0x52be48u) ||
                !word(bank,word(bank,0x4f3218u)+0x1120u,1))
                throw FormatError("Whole original map activation lacks actual physics/readiness");
            return;
        }
''')
    clone.write_text(text)
    fixture=source/'tools/object_loop_activation_fixture.cpp'
    fixture_text=(source/'tests/object_loop_audio_runtime_tu15_tests.cpp').read_text()
    needle='    options.session_random_seed=0x22a0d10u;'
    assert fixture_text.count(needle)==1
    fixture_text=fixture_text.replace(needle,needle+'''
    if(const auto *reference=std::getenv("HALO2_AUDIT_OBJECT_AUDIO_XBE"))options.gameplay_xbe=reference;
''')
    fixture.write_text(fixture_text)
    cmake=source/'CMakeLists.txt';saved=cmake.read_bytes()
    validation=OUT/'activation-build';n=2
    while validation.exists():validation=OUT/('activation-build-'+str(n));n+=1
    cmake.write_bytes(saved+b'''
add_executable(object_loop_activation_audit EXCLUDE_FROM_ALL tools/object_loop_activation_fixture.cpp tools/object_loop_activation_world.cpp)
target_include_directories(object_loop_activation_audit PRIVATE src)
target_link_libraries(object_loop_activation_audit PRIVATE halo2_renderer)
''')
    command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--build-worker','--source',str(source),
        '--target','object_loop_activation_audit','--filter','^original_model_render_build$',
        '--jobs','4','--test-jobs','1','--output',str(validation)]
    try:
        with (OUT/(validation.name+'.log')).open('w') as log:
            code=subprocess.run(command,stdout=log,stderr=subprocess.STDOUT).returncode
    finally:cmake.write_bytes(saved)
    result=dict(scope=__doc__,build_exit_code=code,source_sha256=sha(original),
        private_clone_sha256=sha(clone),original_production_unchanged=source_hashes(source)==pins,
        private_fixture_sha256=sha(fixture),full_image_reference='--full-image-reference' in sys.argv,
        source_readiness_or_platform_result_supplied=False)
    if not code:
        exe=owner/'build/object_loop_activation_audit';result['executable_sha256']=sha(exe)
        maps=ROOT/'assets/imported/mcc-dire-valley-menu-20261023/halo2/h2_maps_win64_dx11'
        report=OUT/(validation.name+'-runtime.json')
        run=[str(exe),str(maps/'coagulation.map'),str(maps),str(report),'listeners-and-source-frame','teleporter']
        result['command']=run
        env=dict(os.environ,HALO2_AUDIT_WHOLE_MAP_ACTIVATION='1')
        if '--full-image-reference' in sys.argv:
            image=Path.home()/'.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe'
            assert sha(image)=='21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd'
            env['HALO2_AUDIT_OBJECT_AUDIO_XBE']=str(image);result['reference_image_sha256']=sha(image)
        with (OUT/(validation.name+'-runtime.log')).open('w') as log:
            try:result['runtime_exit_code']=subprocess.run(run,env=env,stdout=log,stderr=subprocess.STDOUT,timeout=180).returncode
            except subprocess.TimeoutExpired:result['runtime_exit_code']=124
        if report.exists():
            observed=json.loads(report.read_text());result['observed_error']=observed.get('error')
            result['source_game_loaded']=observed.get('source_game_loaded')
            result['target_original_request_count']=observed.get('target_original_request_count')
        result['executable_unchanged']=sha(exe)==result['executable_sha256']
    (OUT/(validation.name+'-receipt.json')).write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps(result),flush=True)
    return code or result.get('runtime_exit_code',1)
if __name__=='__main__':raise SystemExit(main())
