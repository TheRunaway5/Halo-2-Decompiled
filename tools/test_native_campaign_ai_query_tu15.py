#!/usr/bin/env python3
"""Validate the production typed AI query and compile its real campaign callers.

Run under tools/run_test.py. The component cohort freezes its effective sources;
build products and an intentionally wrong private counter mutation are removed
by the enclosing owner. This does not qualify a full campaign client.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
FILES = ['cmake/CampaignNativeAiQuery.cmake', 'cmake/TestStorage.cmake', 'tools/run_test.py',
         'src/campaign/ai/native_campaign_ai_query_tu15.cpp', 'src/campaign/campaign_source_runtime.cpp',
         'src/campaign/ai/native_campaign_ai_fighting_count_tu15.cpp',
         'src/gameplay/simulation.cpp', 'tests/native_campaign_ai_query_tu15_tests.cpp',
         'tests/fixtures/native_campaign_ai_living_count_tu15_v3.json',
         'tests/fixtures/native_campaign_ai_count_family_tu15.json']
FILES.append('tests/fixtures/native_campaign_ai_fighting_count_tu15.json')
FILES += ['tests/fixtures/native_campaign_ai_living_fraction_tu15.json.gz','tools/test_native_campaign_screen_lifecycle_tu15.py']


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--output', type=Path, required=True)
    ap.add_argument('--sanitizers', action='store_true')
    ap.add_argument('--compiler',default='c++')
    args = ap.parse_args()
    scratch = Path(os.environ['HALO2_TEST_SCRATCH']).resolve()
    project = scratch/'native-campaign-ai-query'
    project.mkdir()
    sources = [ROOT/p for p in FILES]
    sources += sorted(p for p in (ROOT/'include').rglob('*') if p.is_file())
    sources += sorted((ROOT/'src').rglob('*.hpp'))
    sources += sorted((ROOT/'src').rglob('*.inc'))
    hashes = {}
    for source in sources:
        relative = source.relative_to(ROOT)
        data = source.read_bytes()
        target = project/relative
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_bytes(data)
        hashes[str(relative)] = hashlib.sha256(data).hexdigest()
    (project/'CMakeLists.txt').write_text('''cmake_minimum_required(VERSION 3.24)
project(CampaignNativeAiQuery LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
option(HALO2_SANITIZERS "Instrument focused native campaign query" OFF)
if(HALO2_SANITIZERS)
    add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address,undefined)
endif()
find_package(Python3 REQUIRED COMPONENTS Interpreter)
find_package(nlohmann_json REQUIRED)
find_package(SDL2 REQUIRED CONFIG)
include(CTest)
include(cmake/TestStorage.cmake)
add_library(halo2_systems STATIC)
target_include_directories(halo2_systems PRIVATE include)
include(cmake/CampaignNativeAiQuery.cmake)
add_library(native_campaign_ai_query_callers OBJECT
    src/gameplay/simulation.cpp src/campaign/campaign_source_runtime.cpp)
target_include_directories(native_campaign_ai_query_callers PRIVATE include src)
target_link_libraries(native_campaign_ai_query_callers PRIVATE SDL2::SDL2 nlohmann_json::nlohmann_json)
''')
    args.output.mkdir(parents=True, exist_ok=True)
    command = [sys.executable, str(ROOT/'tools/run_tests.py'), '--source', str(project),
               '--scratch-root', str(scratch/'builds'), '--output', str(args.output/'normal'),
               '--target', 'native_campaign_ai_query_tu15_tests',
               '--filter', '^original_native_campaign_ai_(query|count_family|fighting_count|living_fraction)_tu15$', '--jobs', '2',
               '--cmake-arg=-DCMAKE_CXX_COMPILER='+args.compiler]
    if args.sanitizers:
        command += ['--sanitizers']
    else:
        command += ['--target', 'native_campaign_ai_query_callers']
    status = subprocess.run(command).returncode
    changed = [name for name, digest in hashes.items() if hashlib.sha256((project/name).read_bytes()).hexdigest() != digest]
    drift = [name for name, digest in hashes.items() if not (ROOT/name).exists() or hashlib.sha256((ROOT/name).read_bytes()).hexdigest() != digest]
    mutation_rejected = None
    fighting_mutations = []
    fraction_mutations = []
    if status == 0 and not args.sanitizers:
        kernel = project/'src/campaign/ai/native_campaign_ai_query_tu15.cpp'
        text = kernel.read_text()
        before = 'else contribution=std::int16_t(read(squad+0xau,2));'
        after = before.replace('squad+0xau', 'squad+8u')
        assert text.count(before) == 1
        kernel.write_text(text.replace(before, after))
        mutation = command.copy()
        mutation[mutation.index('--output')+1] = str(args.output/'mutation')
        # The normal caller object compilation already used the qualified source.
        at = mutation.index('native_campaign_ai_query_callers')
        del mutation[at-1:at+1]
        mutation_status = subprocess.run(mutation).returncode
        mutation_log=args.output/'mutation/tests.log'
        mutation_rejected = mutation_status != 0 and mutation_log.exists() and 'ordered source reads differ' in mutation_log.read_text()
        kernel.write_text(text)
        combat = project/'src/campaign/ai/native_campaign_ai_fighting_count_tu15.cpp'
        combat_text = combat.read_text()
        for name,before,after in [
            ('age-equality','short_at(row+0x306u))>threshold','short_at(row+0x306u))>=threshold'),
            ('inclusive-behavior','std::bit_cast<std::int16_t>(index)<=last','std::bit_cast<std::int16_t>(index)<last')]:
            assert combat_text.count(before)==1
            combat.write_text(combat_text.replace(before,after))
            mutant=mutation.copy()
            mutant[mutant.index('--output')+1]=str(args.output/('mutation-'+name))
            mutant[mutant.index('--filter')+1]='^original_native_campaign_ai_fighting_count_tu15$'
            mutant_status=subprocess.run(mutant).returncode
            log=args.output/('mutation-'+name)/'tests.log'
            failed=mutant_status!=0 and log.exists() and any(reason in log.read_text() for reason in
                ['ordered source reads differ','signed SHORT count differs'])
            fighting_mutations.append(dict(name=name,rejected=failed))
            combat.write_text(combat_text)
    if status == 0 and not args.sanitizers:
        kernel=project/'src/campaign/ai/native_campaign_ai_query_tu15.cpp'
        text=kernel.read_text()
        variants=[
            ('fraction-short-numerator','living_fraction(std::bit_cast<std::int32_t>(count),population)',
             'living_fraction(std::int32_t(std::uint16_t(count)),population)'),
            ('fraction-mean-substitution','living_fraction(std::bit_cast<std::int32_t>(count),population)',
             'mean(weighted_vitality,population)'),
            ('fraction-direct-read','const auto vitality=read(body+0xecu);',
             'const auto vitality=operation==NativeCampaignAiQuery::living_fraction?0u:read(body+0xecu);'),
            ('fraction-required-weight','weighted_vitality=weighted_add(population,value,weighted_vitality);',
             'weighted_vitality=operation==NativeCampaignAiQuery::living_fraction?_mm_setzero_ps():weighted_add(population,value,weighted_vitality);'),
            ('fraction-negative-population','if(operation==NativeCampaignAiQuery::living_fraction) {\n      const auto population=std::bit_cast<std::int32_t>(population_total);\n      if(population<=0)return 0;',
             'if(operation==NativeCampaignAiQuery::living_fraction) {\n      const auto population=std::bit_cast<std::int32_t>(population_total);\n      if(population==0)return 0;')]
        for name,before,after in variants:
            assert text.count(before)==1,(name,before)
            kernel.write_text(text.replace(before,after))
            mutant=command.copy();mutant[mutant.index('--output')+1]=str(args.output/name)
            at=mutant.index('native_campaign_ai_query_callers');del mutant[at-1:at+1]
            mutant[mutant.index('--filter')+1]='^original_native_campaign_ai_living_fraction_tu15$'
            failed_status=subprocess.run(mutant).returncode
            log=args.output/name/'tests.log'
            rejected=failed_status!=0 and log.exists() and any(reason in log.read_text()for reason in
                ['ordered source reads differ','REAL living fraction differs','floating-point status differs'])
            fraction_mutations.append(dict(name=name,rejected=rejected))
            kernel.write_text(text)
    report = dict(scope=__doc__, commands=[command], source_inputs_sha256=hashes,
                  snapshot_changed=changed, checkout_drift=drift,
                  passed=status == 0 and not changed and mutation_rejected is not False and all(m['rejected'] for m in fighting_mutations) and all(m['rejected'] for m in fraction_mutations),
                  mutation_rejected=mutation_rejected, native_semantic_kernel=True,
                  lifted_or_emulated_runtime_calls=0, complete_client_validated=False,
                  compiler=args.compiler,
                  fighting_mutations=fighting_mutations,
                  fraction_mutations=fraction_mutations,
                  owned_scratch=str(scratch),
                  sanitizers=args.sanitizers,
                  live_campaign_or_coop_validated=False, retained_large_artifacts=[])
    (args.output/'source-cohort.json').write_text(json.dumps(report, indent=2, sort_keys=True)+'\n')
    return status or bool(changed) or mutation_rejected is False or any(not m['rejected'] for m in fighting_mutations) or any(not m['rejected']for m in fraction_mutations)


if __name__ == '__main__':
    raise SystemExit(main())
