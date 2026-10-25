#!/usr/bin/env python3
"""Retry real native driver construction with the existing owned EEPROM resource.

The actual UserSettings input is bound through its existing public API. No
console keys, initialized flags, listener records, source returns or device
success are supplied. SDL must open a real callback device. This is a driver
component probe, not object-loop playback acceptance.
"""
import hashlib,json,os,subprocess,sys,tempfile
from pathlib import Path
from build_launcher_engine import source_hashes
ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/'analysis/combat-presentation-audit-20261022/repair-scenery-audio'
def sha(data):return hashlib.sha256(data).hexdigest()
def main():
    owner=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True);source=owner/'source'
    pins=json.loads((OUT.parent/'repair-followup-20261023/source-hashes.json').read_text());assert source_hashes(source)==pins
    eeprom=Path.home()/'.local/share/xemu/xemu/eeprom.bin';data=eeprom.read_bytes();assert len(data)==256
    original=source/'tests/mcc_weapon_sound_startup_tests.cpp';text=original.read_text()
    needle='        if(host_output) {\n';assert text.count(needle)==1
    text=text.replace(needle,'''        const auto *settings=std::getenv("HALO2_AUDIT_ACTUAL_EEPROM");
        if(!settings||!*settings)throw FormatError("Actual owned UserSettings resource is required");
        world->bind_sound_user_settings(settings);
        report["actual_settings_resource_bound"]=true;
'''+needle)
    clone=source/'tools/object_audio_native_startup.cpp';clone.write_text(text)
    out=OUT/'native-startup';attempt=2
    while out.exists():out=OUT/f'native-startup-{attempt}';attempt+=1
    out.mkdir(exist_ok=False)
    cmake=source/'CMakeLists.txt';saved=cmake.read_bytes()
    cmake.write_bytes(saved+b'''
add_executable(object_audio_native_startup EXCLUDE_FROM_ALL tools/object_audio_native_startup.cpp)
target_link_libraries(object_audio_native_startup PRIVATE halo2_systems SDL2::SDL2)
''')
    command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--build-worker','--source',str(source),
        '--target','object_audio_native_startup','--filter','^original_model_render_build$',
        '--jobs','4','--test-jobs','1','--output',str(out/'validation')]
    try:
        with (out/'build-command.log').open('w') as log:code=subprocess.run(command,stdout=log,stderr=subprocess.STDOUT).returncode
    finally:cmake.write_bytes(saved)
    result=dict(scope=__doc__,validation_exit_code=code,fixture_sha256=sha(clone.read_bytes()),
        actual_user_settings_sha256=sha(data[96:192]),eeprom_bytes=len(data),console_keys_retained=False,
        source_results_or_ready_flags_supplied=False)
    if not code:
        exe=owner/'build/object_audio_native_startup';pin=sha(exe.read_bytes())
        maps=ROOT/'assets/imported/mcc-dire-valley-menu-20261023/halo2/h2_maps_win64_dx11'
        report=out/'actual.json'
        with tempfile.TemporaryDirectory(prefix='native-loop-startup-',dir=owner) as temporary:
            scratch=Path(temporary)
            run=[str(exe),str(maps/'coagulation.map'),str(scratch/'save'),str(report),'native-output']
            env=dict(os.environ,HALO2_AUDIT_ACTUAL_EEPROM=str(eeprom),SDL_AUDIODRIVER='pulseaudio')
            with (out/'runtime.log').open('w') as log:
                try:result['runtime_exit_code']=subprocess.run(run,env=env,stdout=log,stderr=subprocess.STDOUT,timeout=300).returncode
                except subprocess.TimeoutExpired:result['runtime_exit_code']=124
            result['command']=run
        result.update(executable_sha256=pin,executable_unchanged=sha(exe.read_bytes())==pin,scratch_removed=not scratch.exists())
        if report.exists():
            actual=json.loads(report.read_text());result.update(driver_call_completed=actual.get('driver_call_completed'),
                actual_error=actual.get('runtime_error'),at_failure=actual.get('at_failure'),native_output=actual.get('native_output'))
    result['frozen_source_unchanged']=source_hashes(source)==pins
    (out/'receipt.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result),flush=True)
    return code or result.get('runtime_exit_code',1)
if __name__=='__main__':raise SystemExit(main())
