#!/usr/bin/env python3
"""Compare the campaign environment fog stages with their pinned TU1.5 bodies."""

import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess

from instrument_native_execution import instrument
import native_fog_accumulation_replacements as accumulation
import native_fog_cluster_finish_replacements as cluster_finish
import native_fog_cluster_sky_replacements as cluster_sky
import native_fog_plane_classifier_replacements as plane_classifier
import native_fog_plane_projection_replacements as plane_projection
import native_fog_plane_output_replacements as plane_output
import native_fog_plane_transform_replacements as plane_transform
import native_fog_projected_merge_replacements as projected_merge
import native_fog_remaining_replacements as remaining
import native_fog_resolution_replacements as resolution
import native_patchy_fog_replacements as patchy_fog
from specialize_native_operands import specialize


ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / 'src/campaign/source_campaign_environment_tu15_generated.cpp'
STAGES = {
    'fog_accumulation': accumulation,
    'fog_cluster_finish': cluster_finish,
    'fog_cluster_sky': cluster_sky,
    'fog_plane_classifier': plane_classifier,
    'fog_plane_projection': plane_projection,
    'fog_plane_output': plane_output,
    'fog_plane_transform': plane_transform,
    'fog_projected_merge': projected_merge,
    'fog_remaining': remaining,
    'fog_resolution': resolution,
    'patchy_fog': patchy_fog,
}


def run_stage(name, binder, source, prepared, compiler, cases, scratch):
    if binder is cluster_sky:
        references = [binder.reference_body(source, stage) for stage in binder.STAGES]
        cluster_stage = binder.STAGES[0]
        child = re.search(r'^static void run_133a70\(WorldState &s\) \{.*?^\}', source, re.M | re.S)
        if not child or hashlib.sha256(child[0].encode()).hexdigest() != (
                'a70d27ff183416f4cb95b3bbcea8373a49ee3612ca75ede2dcc012cc9aa16c2e'):
            raise ValueError('Campaign fog cluster fixture requires its pinned 133A70 child')
        direct = binder.reference_body(source, cluster_stage, direct_child=True).replace(
            binder.symbol(cluster_stage[0]), 'literal_fog_cluster_sky_cluster_131ac0', 1)
        helpers = child[0].replace('run_133a70', 'fixture_133a70', 1) + '\n' + direct
    elif binder is remaining:
        _, references = binder.coverage(source)
        helpers = '\n'.join(binder.reference_body(source, stage, direct_child=True).replace(
            binder.symbol(stage[0]), f'literal_fog_remaining_{stage[0]}_131ac0', 1) for stage in binder.STAGES)
    else:
        references = [binder.reference_body(source)]
        helpers = plane_transform.helper_bodies(source) if binder is plane_transform else ''
    if any(prepared.count(reference) != 1 for reference in references):
        raise ValueError(f'Expected one authenticated campaign fog {name} replay in prepared source')
    (scratch / f'{name}_body.inc').write_text(helpers + '\n'.join(references))
    test = ROOT / f'tests/native_tu15_{name}_tests.cpp'
    binary = scratch / name
    subprocess.run([compiler, '-std=c++20', '-O2', '-fno-fast-math', '-ffp-contract=off', '-frounding-math',
                    f'-I{ROOT / "include"}', f'-I{ROOT / "src"}', f'-I{ROOT / "tests"}', f'-I{scratch}',
                    str(test), '-o', str(binary)], cwd=scratch, check=True)
    results = {}
    for mode in ('literal', 'census', 'verify', 'reference'):
        completed = subprocess.run([str(binary), mode, str(cases)], cwd=scratch, text=True, capture_output=True)
        if completed.returncode:
            raise RuntimeError(f'Campaign fog {name} {mode}: {completed.stderr.strip()}')
        results[mode] = json.loads(completed.stdout)
    return results



LINKED_FIXTURE = r'''
#include "native_tu15_frame_misc_harness.hpp"
#include "halo2/source_campaign_environment_tu15.hpp"
#include "physics/runtime/native_tu15_stack_probe_abi.hpp"
using namespace frame_misc_harness;
int main(int argc,char **argv) try {
    constexpr std::uint32_t stack=0x810000u, output=0x610000u, bsp=0x500000u, clusters=0x510000u;
    const std::string mode = argc > 1 ? argv[1] : "bloom";
    const bool patchy = mode == "patchy", uncovered = mode == "uncovered";
    const bool environment = mode == "environment", missing = mode == "missing", bloom = mode == "active_bloom";
    Case c;
    std::mt19937 rng{0x131ac0u};
    random_abi(rng,c,stack);
    c.abi.gp[1]=0x620000u;
    c.abi.eflags=0x202u;
    c.abi.floating_point=SourcePhysicsFactoryFloatingPoint{0x37fu,0x1f80u};
    for(std::uint32_t p=stack-0x6000u;p<=stack;p+=0x1000u)c.memory.put(p,0u);
    c.memory.put(stack,0x131173u);
    c.memory.put(stack+4,0u);
    c.memory.put(stack+8,0x620000u);
    c.memory.put(0x620000u,0u);
    c.memory.put(stack+12,output);
    c.memory.put(output,0u);
    c.memory.put(0x46a0d0u,0x3f800000u);
    c.memory.put(0x46a110u,0x3f800000u);
    c.memory.put(0x556e10u,uncovered ? 0u : 0x3f800000u);
    c.memory.put(0x4ecc18u,bsp);
    c.memory.put(0x4ecc20u,0x520000u);
    c.memory.put(0x474c24u,0u,2);
    c.memory.put(0x520214u,0x530000u);
    c.memory.put(0x53003cu,0u,1);
    c.memory.put(bsp+0xa0u,clusters);
    c.memory.put(clusters+0x6cu,missing || environment ? 255u : 0u,1);
    c.memory.put(clusters+0x6eu,255u,1);
    c.memory.put(clusters+0x6fu,environment ? 0u : 255u,1);
    c.memory.put(0x520340u,1u);
    c.memory.put(0x520344u,0x540000u);
    for(std::uint32_t p=0x540000u;p<=0x5400f0u;p+=4u)c.memory.put(p,0x3f800000u);
    c.memory.put(0x5400d8u,0u);
    c.memory.put(clusters+0x8cu,0u);
    c.memory.put(0x520008u,1u);
    c.memory.put(0x52000cu,0x550000u);
    c.memory.put(0x550004u,0u);
    c.memory.put(0x4f0414u,0x551000u);
    c.memory.put(0x551008u,0x560000u);
    c.memory.put(0x560044u,uncovered ? 0u : 0x3f800000u);
    constexpr std::array groups{0x570000u,0x571000u,0x572000u};
    for (unsigned index=0;index<groups.size();++index) {
        c.memory.put(0x560048u+8u*index,1u);
        c.memory.put(0x56004cu+8u*index,groups[index]);
        for(unsigned offset=0;offset<=0x14u;offset+=4u)c.memory.put(groups[index]+offset,0x3f000000u);
    }
    c.memory.put(0x560060u,patchy?1u:0u);
    if (bloom)
        for (unsigned offset : {0x68u,0x6cu,0x70u,0x74u})
            c.memory.put(0x560000u + offset,0x3f000000u);
    c.memory.put(0x560064u,0x580000u);
    for(unsigned offset=0;offset<=0x24u;offset+=4u)c.memory.put(0x580000u+offset,0x3f000000u);
    c.memory.put(0x58004cu,0x42u);
    std::uint32_t cursor{};
    std::size_t required{};
    std::vector<SourcePhysicsContactRecordStore> stores;
    SourcePhysicsContactRecordObserver observe=[&](const auto &event){stores.push_back(event);};
    constexpr std::array sites{0x38846eu,0x21f590u,0x33b595u};
    SourcePhysicsWorldRuntimeServices services;
    services.instruction=[&](std::uint32_t pc){cursor=pc;};
    auto &policy=services.instruction_cursor;
    policy.position=&cursor;
    policy.callback_type=&services.instruction.target_type();
    policy.executor=std::this_thread::get_id();
    policy.service_sites=sites;
    policy.literal_service_sites=true;
    policy.observation_only_stores=true;
    services.required_owner=[&](auto &m,const auto &call,auto &abi,const auto &obs) {
        if(call.target!=0x32a660u || call.instruction!=0x131ac5u || call.tail || ++required!=1)
            throw std::runtime_error("Unexpected required owner "+std::to_string(call.target));
        WorldState state{m,obs,services,SourcePhysicsContactImage::TitleUpdate15};
        state.load(abi);
        if(!halo2::native_stack_probe_detail::run(state))
            throw std::runtime_error("Real native stack probe refused");
        abi=state.save();
    };
    if(!source_campaign_environment_tu15_owns_entry(0x131ac0u))
        throw std::runtime_error("Production campaign owner missing");
    try {source_run_campaign_environment_tu15(c.memory,0x131ac0u,c.abi,*c.abi.floating_point,services,observe);} catch(...) {std::fprintf(stderr,"fault cursor %x\n",cursor); throw;}
    if(required!=1 || c.abi.gp[4]!=stack+16 || cursor!=0x1329a6u)
        throw std::runtime_error("Unexpected final campaign ABI");
    const auto geometry_stores=std::ranges::count_if(stores,[](const auto &s){return s.instruction>=0x1342b0u&&s.instruction<0x1343b0u;});
    if (geometry_stores != (patchy || environment ? 12u : 0u)) throw std::runtime_error("Unexpected genuine patchy fog accumulation child");
    const auto source_group_stores=std::ranges::count_if(stores,[](const auto &s){return s.instruction>=0x1321d3u&&s.instruction<=0x132429u;});
    if(source_group_stores!=(missing || uncovered || environment ? 0 : 22))throw std::runtime_error("Expected three actual projected fog groups");
    std::uint64_t digest=0;
    for(const auto word:c.abi.gp)digest=mix(digest,word);
    for(const auto &lanes:c.abi.xmm)for(const auto word:lanes)digest=mix(digest,word);
    digest=mix(digest,c.abi.eflags);
    digest=mix(digest,c.abi.floating_point->x87_control_word);
    digest=mix(digest,c.abi.floating_point->mxcsr);
    digest=mix(digest,c.abi.x87_status);
    digest=mix(digest,c.abi.x87_depth);
    digest=mix(digest,c.abi.fs_base);
    for (unsigned i=0;i<c.abi.x87_depth;++i) {
        const auto *bytes=reinterpret_cast<const unsigned char *>(&c.abi.x87[i]);
        for (unsigned byte=0;byte<10;++byte)
            digest=mix(digest,bytes[byte]);
    }
    for(const auto &event:stores) {
        digest=mix(digest,event.address);
        digest=mix(digest,event.instruction);
        for(const auto byte:event.before)digest=mix(digest,std::to_integer<unsigned>(byte));
        for(const auto byte:event.written)digest=mix(digest,std::to_integer<unsigned>(byte));
    }
    for(const auto &[k,page]:c.memory.pages) {
        digest=mix(digest,k);
        for(const auto byte:*page)digest=mix(digest,std::to_integer<unsigned>(byte));
    }
    std::printf("{\"required_owner_32a660\":%zu,\"stores\":%zu,\"abi_memory_digest\":\"%llx\"}\n",
            required,stores.size(),static_cast<unsigned long long>(digest));
    return 0;
} catch(const std::exception &e) {std::fprintf(stderr,"%s\n",e.what());return 1;}
'''



def run_linked(source, prepared, compiler, scratch):
    accounted, manifest = instrument(prepared, source, SOURCE.relative_to(ROOT).as_posix())
    owner = next(row for row in manifest['functions'] if row['symbol'] == '131ac0')
    references = {row['symbol'] for row in manifest['functions'] if row.get('canonical_reference_owner') == '131ac0'}
    reference_sites = sum(row['literal_sites'] for row in manifest['functions'] if row['symbol'] in references)
    if len(references) != 14 or reference_sites != owner['literal_sites'] or reference_sites != 786:
        raise ValueError('Campaign fog accounting does not cover all 786 original 131AC0 sites')
    alias = scratch / 'campaign_fog_accounted.cpp'
    fixture = scratch / 'campaign_fog_linked.cpp'
    binary = scratch / 'campaign_fog_linked'
    alias.write_text(accounted)
    fixture.write_text(LINKED_FIXTURE)
    flags = ['-std=c++20', '-O2', '-fno-fast-math', '-ffp-contract=off', '-frounding-math']
    includes = [f'-I{ROOT / directory}' for directory in ('include', 'src', 'tests')]
    subprocess.run([compiler, *flags, *includes, str(alias), str(fixture),
                    str(ROOT / 'src/runtime/native_execution_accounting.cpp'), '-o', str(binary)],
                   cwd=scratch, check=True)

    toggles = ('REMAINING', 'RESOLUTION', 'PROJECTED_MERGE', 'ACCUMULATION', 'CLUSTER_SKY', 'CLUSTER_FINISH')
    children = ('131470', '131610', '133a70', '133b40', '1342b0')
    results = {}
    for case in ('bloom', 'active_bloom', 'patchy', 'environment', 'missing', 'uncovered'):
        comparisons = {}
        for mode in ('native', 'verify', 'reference'):
            report = scratch / f'{case}-{mode}.json'
            env = dict(os.environ, HALO2_NATIVE_EXECUTION_REPORT=str(report))
            for toggle in toggles:
                for control in ('verify', 'reference'):
                    flag = f'HALO2_{control.upper()}_FOG_{toggle}'
                    env.pop(flag, None)
                    if mode == control:
                        env[flag] = '1'
            completed = subprocess.run([str(binary), case], cwd=scratch, env=env, text=True, capture_output=True)
            if completed.returncode:
                raise RuntimeError(f'Linked campaign fog {case} {mode}: {completed.stderr.strip()}')
            data = json.loads(report.read_text())
            if any(data[key] for key in ('orphan_literal_events', 'registration_failures', 'active_scopes')):
                raise ValueError('Incomplete linked campaign fog accounting')
            rows = {row['symbol']: row for row in (dict(zip(data['row_fields'], item)) for item in data['rows'])}
            parent = rows['131ac0']
            body = parent['literal_statements']
            replay = sum(rows[symbol]['literal_statements'] for symbol in references & rows.keys())
            child_rows = {child: rows[child] for child in children if child in rows}
            if parent['entries'] != 1 or parent['returns'] != 1 or (
                    mode == 'reference' and (not body or replay or parent['returns_without_body'])) or (
                    mode != 'reference' and (body or parent['returns_without_body'] != 1)):
                raise ValueError(f'Wrong linked 131AC0 execution shape for {case} {mode}')
            calls = {child: child_rows.get(child, {}).get('entries', 0) for child in children}
            expected_calls = (1, 1, 1, int(case in ('bloom', 'active_bloom', 'patchy')),
                              int(case in ('patchy', 'environment')))
            if tuple(calls.values()) != expected_calls or any(row['returns'] != row['entries'] for row in child_rows.values()):
                raise ValueError(f'Wrong linked 131AC0 child routes for {case} {mode}')
            comparisons[mode] = {'matched': json.loads(completed.stdout), 'body_literals': body,
                                 'replay_literals': replay, 'child_calls': calls}
        native, verify, reference = (comparisons[mode] for mode in ('native', 'verify', 'reference'))
        if (not native['matched'] == verify['matched'] == reference['matched'] or
                native['replay_literals'] or verify['replay_literals'] != reference['body_literals']):
            raise ValueError(f'Linked campaign fog parity differs for {case}')
        results[case] = comparisons
    return {'owner': '131ac0', 'literal_sites': reference_sites, 'reference_bodies': len(references), 'cases': results}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--compiler', default='c++')
    parser.add_argument('--cases', type=int, default=4000)
    parser.add_argument('--report', required=True, type=Path)
    args = parser.parse_args()
    owned = os.environ.get('HALO2_TEST_SCRATCH')
    if not owned or args.cases < 8:
        parser.error('Run through tools/run_test.py with at least eight cases')
    scratch = Path(owned)
    source = SOURCE.read_text()
    prepared, metadata = specialize(source)
    complete_owners = {'131ac0', '133b40', '1342b0'}
    if ('131ac0' in metadata.get('native_partial_frame_owners', {}) or
            not complete_owners <= set(metadata.get('native_spatial_owners', []))):
        raise ValueError('Campaign fog production binding is missing')
    results = {name: run_stage(name, binder, source, prepared, args.compiler, args.cases, scratch)
               for name, binder in STAGES.items()}
    linked = run_linked(source, prepared, args.compiler, scratch)
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps({'passed': True, 'results': results, 'linked': linked,
        'complete_owners': sorted(complete_owners), 'owned_scratch': str(scratch),
        'canonical_sha256': hashlib.sha256(source.encode()).hexdigest(),
        'prepared_sha256': hashlib.sha256(prepared.encode()).hexdigest()}, indent=2) + '\n')
    print('PASS native campaign fog stages against the pinned source')


if __name__ == '__main__':
    main()
