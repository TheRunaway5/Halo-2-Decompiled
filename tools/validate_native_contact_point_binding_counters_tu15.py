#!/usr/bin/env python3
"""Compile exact staged point diagnostic sums/getter/JSON/gate under both compilers.

All authored values here are diagnostic sentinels, never physics DATA or supplied
physics outcomes. The production service builder has an independent physical
actual-input qualification.
"""
import argparse, hashlib, json, os, re, shutil, subprocess
from pathlib import Path
from prepare_native_contact_point_binding_tu15 import FIELDS
ROOT=Path(__file__).resolve().parents[1]

def sha(raw): return hashlib.sha256(raw).hexdigest()

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--stage',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args(); args.output=args.output.resolve()
    args.output.mkdir(parents=True,exist_ok=False)
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve()
    stage=args.stage.resolve()/'staged'
    report=dict(passed=False,owned_scratch=str(scratch),retained_large_artifacts=[],
        supplied_physics_state_or_result=False)
    try:
        project=scratch/'project'; project.mkdir()
        shutil.copytree(ROOT/'include',project/'include')
        for header in (stage/'include').rglob('*.hpp'):
            target=project/'include'/header.relative_to(stage/'include')
            target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(header,target)
        world=(stage/'src/physics/native_tu15_world_integration_residuals.cpp').read_text()
        start=world.index('      const auto collect_point=[&]{if(stats){')
        finish=world.index('      try{',start)
        aggregate=world[start:finish]
        branch=world[start:world.index('      return;',finish)]
        assert branch.count('collect_point();')==2
        assert 'catch(...){collect_point();throw;}' in branch
        getter=(stage/'src/physics/physics_live_world_tu15.cpp').read_text()
        getter_lines=[]
        for field in FIELDS:
            matches=re.findall(r'^    result\.'+field+r'=.*;$',getter,re.M)
            assert len(matches)==1,(field,matches)
            getter_lines+=matches
        fragment='\n'.join(getter_lines)
        acceptance=(stage/'tests/physics_multiplayer_acceptance.cpp').read_text()
        start=acceptance.index('  if(std::getenv("HALO2_REQUIRE_NATIVE_CONTACT_POINT_PIPELINE"))')
        finish=acceptance.index('  if(std::getenv("HALO2_REQUIRE_NATIVE_CONTACT_MANAGER_CLEANUP"))',start)
        gate=acceptance[start:finish]
        serializers=[]
        for occurrence in (0,1):
            rows=[]
            for field in FIELDS:
                matches=re.findall(r'^\s*\{"'+field+r'",native_frames\.'+field+r'\},$',acceptance,re.M)
                assert len(matches)==2,(field,len(matches))
                rows.append(matches[occurrence].strip())
            serializers.append('\n'.join(rows))
        initializers=[]; assertions=[]; expected={}; sentinel=100
        # Unique per-leaf sentinels catch aliasing, omission and wrong stats source.
        for field,expression in FIELDS.items():
            value=0
            for term in expression.split('+'):
                term=term.removeprefix('activity.');sentinel+=7;value+=sentinel
                initializers.append('activity.'+term+'='+str(sentinel)+'u;')
            expected[field]=value
            assertions.append('check(stats.'+field+'=='+str(value)+'u,"Wrong point branch aggregation: '+field+'");')
            assertions.append('check(Harness{&impl}.get().'+field+'==stats.'+field+',"Wrong getter: '+field+'");')
        source='''#include "halo2/native_tu15_contact_point_binding.hpp"
#include "halo2/native_tu15_world_integration.hpp"
#include "halo2/native_tu15_physics_frame_residuals.hpp"
#include <nlohmann/json.hpp>
#include <cstdlib>
#include <stdexcept>
using namespace halo2;using J=nlohmann::json;
void check(bool b,const char*why){if(!b)throw std::runtime_error(why);}
struct Impl{NativeTu15WorldIntegrationStats native_physics_integrations;};
struct Harness{Impl*impl_;NativeTu15PhysicsFrameExecutionStats get(){NativeTu15PhysicsFrameExecutionStats result;
'''+fragment+'''
return result;}};
void collect(NativeTu15WorldIntegrationStats*stats,const NativeTu15ContactPointBindingActivity&activity){
'''+aggregate+'''
collect_point();}
bool passes(NativeTu15PhysicsFrameExecutionStats native_frames,bool enabled=true){
if(enabled)setenv("HALO2_REQUIRE_NATIVE_CONTACT_POINT_PIPELINE","1",1);else unsetenv("HALO2_REQUIRE_NATIVE_CONTACT_POINT_PIPELINE");
J report;bool passed=true;
'''+gate+'''
if(enabled)check(report.contains("native_contact_point_pipeline_gate"),"Point gate result missing");return passed;}
J serialize0(NativeTu15PhysicsFrameExecutionStats native_frames){return J{
'''+serializers[0]+'''};}
J serialize1(NativeTu15PhysicsFrameExecutionStats native_frames){return J{
'''+serializers[1]+'''};}
int main(){NativeTu15ContactPointBindingActivity activity;'''+''.join(initializers)+'''
Impl impl;auto&stats=impl.native_physics_integrations;collect(&stats,activity);
'''+''.join(assertions)+'''
auto frame=Harness{&impl}.get();auto one=serialize0(frame),two=serialize1(frame);
check(one==two&&one.size()==16,"Point JSON blocks differ");
'''+''.join('check(one.at("'+f+'") == '+str(v)+'u,"Wrong serialized diagnostic: '+f+'");'for f,v in expected.items())+'''
// The source uses this same aggregator on normal completion and caught failure.
// Independent second activity values prove accumulation rather than replacement.
collect(&stats,activity);
'''+''.join('check(stats.'+f+'=='+str(2*v)+'u,"Point repeated aggregation lost counters");'for f,v in expected.items())+'''
collect(nullptr,activity);
NativeTu15PhysicsFrameExecutionStats good;good.native_contact_point_attempts=1;good.native_contact_point_calls=1;
good.native_contact_point_gameplay_listeners=1;good.native_contact_point_gameplay_rows=1;good.native_contact_point_world_updates=1;
good.native_contact_point_island_detaches=1;good.native_contact_point_info_queries=1;good.native_contact_point_reference_releases=1;
good.native_contact_point_metadata_calls=2;check(passes(good),"Qualified positive point gate rejected");
check(passes({},false),"Disabled point gate changed admission");
unsigned rejected=0;
'''
        negative_values={
            'native_contact_point_attempts':2,'native_contact_point_calls':0,
            'native_contact_point_refusals':1,'native_contact_point_profile_refusals':1,
            'native_contact_point_unqualified_services':1,'native_contact_agent_point_residuals':1,
            'native_contact_point_gameplay_listeners':0,'native_contact_point_gameplay_rows':0,
            'native_contact_point_world_updates':0,'native_contact_point_island_detaches':0,
            'native_contact_point_info_queries':0,'native_contact_point_reference_releases':0,
            'native_contact_point_metadata_calls':1,'native_contact_point_effect_requests':1,
            'native_contact_point_part_requests':1}
        for field,value in negative_values.items():
            source+='{auto bad=good;bad.'+field+'='+str(value)+';check(!passes(bad),"Point gate admitted '+field+'");++rejected;}\n'
        owners=(0x322f70,0x281d40,0x1d8880,0x1d8970,0x325d90,0x320100,0x3201b0,0x325e90)
        for owner in owners:
            source+='{auto bad=good;bad.residual_owner_calls['+str(owner)+']=1;check(!passes(bad),"Point gate admitted old owner");++rejected;}\n'
        source+='check(rejected==23,"Gate negative coverage differs");return 0;}\n'
        (project/'counters.cpp').write_text(source)
        commands=[]
        for compiler in ('g++','clang++'):
            target=project/('counters-'+compiler.replace('+','x'))
            command=[compiler,'-std=c++20','-O2','-I'+str(project/'include'),str(project/'counters.cpp'),'-o',str(target)]
            commands.append(command);result=subprocess.run(command,capture_output=True,text=True)
            (args.output/(compiler.replace('+','x')+'.log')).write_text(result.stdout+result.stderr)
            assert result.returncode==0,(compiler,result.stderr[-4000:])
            subprocess.run([str(target)],check=True)
        report.update(passed=True,commands=commands,fields=len(FIELDS),serializer_occurrences_per_field=2,
            distinct_activity_leaves=len(initializers),metadata_sum_terms=2,
            cumulative_normal_or_throw_aggregations=2,null_stats_checked=True,
            negative_gate_controls=len(negative_values)+len(owners),positive_gate_controls=2,
            actual_aggregate_sha256=sha(aggregate.encode()),actual_getter_sha256=sha(fragment.encode()),
            actual_gate_sha256=sha(gate.encode()),actual_serializers_sha256=[sha(s.encode())for s in serializers],
            stage_source_pins_sha256=sha((args.stage/'source-pins.json').read_bytes()),
            scope='Exact staged point stats-source sum, repeated normal/error diagnostic aggregation, getter, both JSON values and gate truth controls under GCC/Clang. Diagnostic-only authored sentinels; no physics output/frame supplied.')
    except BaseException as error:
        report['error']=str(error)or repr(error);raise
    finally:
        report['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in scratch.rglob('*')if p.is_file())
        (args.output/'report.json').write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
    return 0

if __name__=='__main__':raise SystemExit(main())
