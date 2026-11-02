#!/usr/bin/env python3
"""Same native-only production31D930 helper versus whole original profiles."""
import inspect
from pathlib import Path
import validate_native_joint_constraint_build_tu15 as parent
ROOT=Path(__file__).resolve().parents[1]
def main():
 text=inspect.getsource(parent.main)
 text=text.replace('halo2/native_tu15_joint_constraint_build.hpp','halo2/native_tu15_joint_constraint_build_binding.hpp')
 text=text.replace("'NativeTu15JointConstraintBuildStats stats;'","'NativeTu15BoundJointConstraintBuildActivity stats;'")
 text=text.replace('native_tu15_build_joint_constraint(m,c.at("receiver"),{c.at("arguments")[0]},{c.at("arguments")[1]},fp,0,services,observer,&stats);','native_tu15_build_bound_joint_constraint(m,c.at("receiver"),{c.at("arguments")[0]},{c.at("arguments")[1]},fp,0,observer,&stats);')
 # The original fixture's operand faults remain strict; this checks the
 # wrapper's additional resource refusals before any DATA access separately.
 guards='''
 for(unsigned mode=0;mode<4;++mode){Memory empty;empty.log_reads=true;NativeTu15PhysicsFrameFp fp{0x27f,0x20,0x1f80};unsigned depth{};if(mode==0)depth=1;if(mode==1)fp.control_word=0x27e;if(mode==2)fp.status=0x8080;if(mode==3)fp.mxcsr=0;NativeTu15BoundJointConstraintBuildActivity out;bool refused{};try{native_tu15_build_bound_joint_constraint(empty,0,{0},{0},fp,depth,{},&out);}catch(const std::runtime_error&){refused=true;}check(refused&&out.attempts==1&&out.completed==0&&out.refusals==1&&out.unsupported_fp==1&&empty.reads.empty(),"Native bound joint guard touched DATA or fell back");}
'''
 guards=' '.join(guards.splitlines())
 # Inject using repr through the existing generation layer to preserve exact
 # C++ source newlines, rather than adding another escaped literal level.
 needle="(project/'driver.cpp').write_text(driver);variants={}"
 extra="driver=driver.replace('}++count;',"+repr(guards+'}++count;')+");"+needle
 text=text.replace(needle,extra)
 env=dict(parent.__dict__,__file__=__file__,__doc__=__doc__);exec(text,env);return env['main']()
if __name__=='__main__':raise SystemExit(main())
