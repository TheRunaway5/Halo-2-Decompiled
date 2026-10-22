#!/usr/bin/env python3
"""Hash completed standalone simulation proofs; no build, replay, GPU or guest."""
from pathlib import Path
import argparse
import hashlib
import json

ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/'analysis/multiplayer-shading/weather-particle-simulation-original'

def record(path):
    raw=path.read_bytes();return dict(bytes=len(raw),sha256=hashlib.sha256(raw).hexdigest())

def main():
    p=argparse.ArgumentParser();p.add_argument('normal_build',type=Path);p.add_argument('asan_build',type=Path);args=p.parse_args()
    expected='46 complete ready38450 calls (1actual,remainingcontrolled); 33306 native words exact; 2134 explicitRSQRT pairs; 7479 auxiliary draws; 378 ownership/control/failed-producer guards'
    for name in ['native-normal.log','native-asan.log']:
        assert (OUT/name).read_text().strip()==expected
    for name in ['build-normal.log','build-asan.log']:assert not (OUT/name).read_text().strip()
    report=json.loads((OUT/'report.json').read_text());controls=json.loads((OUT/'controls.json').read_text())
    assert report['replay']['completed_whole38450'] and report['replay']['nonstack_differing_written_bytes']==0
    assert report['replay']['nonstack_written_bytes']==41120 and all(v['equal'] for v in report['replay']['CPU'].values())
    assert controls['controlled_complete_original_calls']==45 and controls['expected_native_words']==33306
    paths=[ROOT/x for x in ['include/halo2/source_weather_particle_simulation.hpp','src/rendering/weather/source_weather_particle_simulation.cpp',
        'include/halo2/source_weather_exclusion.hpp','src/rendering/weather/source_weather_exclusion.cpp','tests/source_weather_particle_simulation_tests.cpp',
        'tools/audit_source_weather_particle_simulation_original.py','tools/freeze_source_weather_particle_simulation_fixture.py',
        'tools/differential_source_weather_particle_simulation.py','tools/build_source_weather_particle_simulation.sh',
        'tests/fixtures/retail_source_weather_particle_simulation.tsv','tests/fixtures/retail_source_weather_particle_simulation.provenance.json',
        'tests/fixtures/retail_source_weather_particle_simulation_controls.tsv','docs/SOURCE_WEATHER_PARTICLE_SIMULATION_PARITY.md',
        'cmake/SourceWeatherSimulation.cmake']]
    paths+=[Path(__file__).resolve()]
    paths+=[p for p in sorted(OUT.iterdir())if p.is_file() and p.suffix in ['.bin','.json','.log'] and p.name!='manifest.json']
    for row in report['inputs'].values():
        directory=ROOT/row['directory'];paths+=[directory/'export.json',directory/'exports/physical-ram.bin']
    paths+=[args.normal_build.resolve()/'source_weather_particle_simulation_tests',args.asan_build.resolve()/'source_weather_particle_simulation_tests']
    result=dict(version=1,actual_original_calls=1,controlled_unchanged_original_calls=45,
        actual_lasting_written_bytes_exact=41120,native_output_control_words_exact=33306,
        required_source_RSQRT_pairs=2134,auxiliary_draws=7479,rejection_checks=378,
        normal=True,fully_instrumented_ASAN_UBSAN=True,detect_leaks=True,
        live_activation=False,CMake_edits_by_this_proof=False,
        CPU_CMake_registration='cmake/SourceWeatherSimulation.cmake',GPU_jobs=0,guest_IO=False,
        authority='Captured actualxemu call and explicit unchanged-source controls; source-ready and ISA answers remain caller producers. No physicalXbox RSQRT precision or native timeline claim.',
        files={str(p.relative_to(ROOT)):record(p)for p in paths})
    target=OUT/'manifest.json';target.write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps(dict(manifest=str(target.relative_to(ROOT)),files=len(result['files']),**record(target))))

if __name__=='__main__':main()
