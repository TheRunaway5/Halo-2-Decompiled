#!/usr/bin/env python3
"""Exact staged31D930 callback fragment on genuine1478 input, no CPU fallback.

The source-owned four-word packet publication/recovery surrounds the SAME
native helper. Its readonly observation sees genuine current DATA and FP.
All captured bytes and kernel stores remain strict; four existing recovery
reads are compared separately from the original callee's retained reads.
"""
import argparse,inspect,json,sys
from pathlib import Path
import validate_native_joint_constraint_build_actual_tu15 as parent
ROOT=Path(__file__).resolve().parents[1]
STAGE_WORLD=None
def observer_source():
 text=parent.observer_source()
 source=(STAGE_WORLD/'staged/src/physics/native_tu15_constraint_step_residuals.cpp').read_text()
 begin=source.index('    put(m,packet,build.jacobians);');end=source.index('    try{',begin);publication=source[begin:end]
 begin=source.index('        native_tu15_observe_constraint_frame_build_input',end);end=source.index('\n      }else{',begin);branch=source[begin:end]
 prefix='''
std::uint32_t word(SourcePhysicsContactMemory&m,std::uint32_t p){std::array<std::byte,4>b{};m.read(p,b);std::uint32_t value{};for(unsigned n=0;n<4;++n)value|=std::to_integer<unsigned>(b[n])<<(8u*n);return value;}
void put(SourcePhysicsContactMemory&m,std::uint32_t p,std::uint32_t value){std::array<std::byte,4>b{};for(unsigned n=0;n<4;++n)b[n]=std::byte(value>>(8u*n));m.write(p,b);}
void invoke_joint_route(ReorientationMemory&m,const J&typed,NativeTu15PhysicsFrameFp&context,bool read_pass,const SourcePhysicsContactRecordObserver&observe,NativeTu15ConstraintBindingActivity&out){
 const auto packet=typed.at("builder_packet").get<std::uint32_t>(),jacobian_input=typed.at("jacobian_input").get<std::uint32_t>();
 const NativeTu15Constraint constraint{typed.at("constraint")};const unsigned depth=0;auto*activity=&out;
 NativeTu15ConstraintBuildContext build{typed.at("builder_words")[0],typed.at("builder_words")[1],typed.at("builder_words")[2],typed.at("builder_words")[3]};
 bool observed{};m.track=false;
 NativeTu15ConstraintFrameBuildInputObserverBinding input_observer(m,[&](const auto&owner,const auto&input){
  check(&owner==&m&&input.constraint==constraint.identity&&input.jacobian_input==jacobian_input&&input.builder_packet==packet&&J(input.builder_words)==typed.at("builder_words")&&input.preserved_x87_depth==0&&input.floating_point.control_word==context.control_word&&input.floating_point.status==context.status&&input.floating_point.mxcsr==context.mxcsr,"Staged native callback lost actual BEFORE");observed=true;m.track=read_pass;
 });
'''+publication+''' try{
'''+branch+'''
 }catch(...){recover();throw;}recover();m.track=false;
 check(observed&&out.residual_calls==0,"Staged joint callback fell back or lost observer");
 const std::array actual{word(m,packet),word(m,packet+4),word(m,packet+8),word(m,packet+12)};
 check(actual==std::array{build.jacobians,build.schemas,build.jacobian_limit,build.secondary_jacobians},"Native callback lost builder cursor recovery");
}
'''
 text=text.replace('}\nint main(int argc,char**argv)',prefix+'}\nint main(int argc,char**argv)')
 text=text.replace('NativeTu15PhysicsFrameFp fp{before_fp.cwd,before_fp.swd,before_fp.mxcsr};NativeTu15BoundJointConstraintBuildActivity activity;','NativeTu15PhysicsFrameFp fp{before_fp.cwd,before_fp.swd,before_fp.mxcsr};NativeTu15ConstraintBindingActivity route_activity;auto&activity=route_activity.joint_builder;')
 text=text.replace('native_tu15_build_bound_joint_constraint(memory,index,{forward},{typed.at("builder_packet")},fp,0,observe,&activity);','invoke_joint_route(memory,typed,fp,read_pass,observe,route_activity);')
 text=text.replace('const auto&expected=read_pass?reads:stores;', 'J route_reads=reads;if(read_pass){const auto packet=typed.at("builder_packet").get<std::uint32_t>();for(unsigned n=0;n<4;++n)route_reads.push_back(J::array({packet+n*4,4}));}const auto&expected=read_pass?route_reads:stores;')
 return text
def main():
 global STAGE_WORLD
 parser=argparse.ArgumentParser(add_help=False);parser.add_argument('--stage',required=True,type=Path);args,remaining=parser.parse_known_args();STAGE_WORLD=args.stage.resolve();sys.argv=[sys.argv[0],*remaining]
 text=inspect.getsource(parent.main)
 text=text.replace("text=inspect.getsource(common.main)","text=inspect.getsource(common.main).replace(\"'-I'+str(ROOT/'include')\",\"'-I'+str(STAGE_WORLD/'staged/include'),'-I'+str(ROOT/'include'),'-include',str(ROOT/'include/halo2/native_tu15_constraint_frame_build_input_observer.hpp')\")")
 text=text.replace('halo2/native_tu15_joint_constraint_build_binding.hpp','halo2/native_tu15_constraint_step_residuals.hpp')
 text=text.replace("environment=dict(common.__dict__,parent=parent,STAGE=STAGE,observer_source=observer_source,__doc__=__doc__)","environment=dict(common.__dict__,parent=parent,STAGE=STAGE,STAGE_WORLD=STAGE_WORLD,observer_source=observer_source,__doc__=__doc__)")
 environment=dict(parent.__dict__,STAGE_WORLD=STAGE_WORLD,observer_source=observer_source,__doc__=__doc__);exec(text,environment);rc=environment['main']();path=Path(sys.argv[sys.argv.index('--output')+1])/'report.json';report=json.loads(path.read_text());report.update(callback_stage_source_hashes={r:parent.common.sha(STAGE_WORLD/'staged'/r)for r in('src/physics/native_tu15_constraint_step_residuals.cpp','include/halo2/native_tu15_constraint_step_residuals.hpp')},callback_validation_tool_sha256=parent.common.sha(Path(__file__)),exact_selected_callback_source_fragment=True,packet_recovery_reads_per_profile=4,no_residual_call=True,scope=__doc__);report['source_hashes'].pop('include/halo2/native_tu15_constraint_step_residuals.hpp',None);report['source_hashes'][str((STAGE_WORLD/'staged/include/halo2/native_tu15_constraint_step_residuals.hpp').relative_to(ROOT))]=parent.common.sha(STAGE_WORLD/'staged/include/halo2/native_tu15_constraint_step_residuals.hpp');report['source_hashes']['include/halo2/native_tu15_joint_constraint_build_binding.hpp']=parent.common.sha(ROOT/'include/halo2/native_tu15_joint_constraint_build_binding.hpp');parent.common.save(path,report);return rc
if __name__=='__main__':raise SystemExit(main())
